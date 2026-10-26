#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace omega {
// UI-only HSV picker. Conversions never modify the source LUT automatically.
struct InlinePickerColor {float hue=0,saturation=0,value=1;};
inline float pickerUnit(float x){return std::isfinite(x)?std::clamp(x,0.f,1.f):0.f;}
inline std::array<float,3> pickerRgb(const InlinePickerColor& color){
 const float h=(color.hue-std::floor(color.hue))*6.f,s=pickerUnit(color.saturation),v=pickerUnit(color.value);
 const int sector=int(h);const float f=h-sector,p=v*(1-s),q=v*(1-s*f),t=v*(1-s*(1-f));
 switch(sector){case 0:return {v,t,p};case 1:return {q,v,p};case 2:return {p,v,t};case 3:return {p,q,v};case 4:return {t,p,v};default:return {v,p,q};}
}
inline void pickerReadRgb(InlinePickerColor& color,float r,float g,float b){
 r=pickerUnit(r);g=pickerUnit(g);b=pickerUnit(b);const float hi=std::max({r,g,b}),lo=std::min({r,g,b}),d=hi-lo;
 color.value=hi;color.saturation=hi>0?d/hi:0;
 if(d>1e-7f){float h=hi==r?(g-b)/d:hi==g?(b-r)/d+2.f:(r-g)/d+4.f;h/=6.f;color.hue=h-std::floor(h);}
}
inline void drawInlineColorPicker(HDC dc,const RECT& sv,const RECT& hue,const InlinePickerColor& color,bool enabled,bool focused){
 struct Raster {int width=0,height=0;float hue=-1;bool enabled=true;std::vector<unsigned int> pixels;};
 static thread_local Raster square,strip;
 auto draw=[&](const RECT& r,bool isHue){
  const int w=int(r.right-r.left),h=int(r.bottom-r.top);if(w<=0||h<=0)return;Raster& cache=isHue?strip:square;
  if(cache.width!=w||cache.height!=h||cache.enabled!=enabled||(!isHue&&cache.hue!=color.hue)){
   cache.width=w;cache.height=h;cache.hue=color.hue;cache.enabled=enabled;cache.pixels.resize(size_t(w)*h);
   for(int y=0;y<h;++y)for(int x=0;x<w;++x){InlinePickerColor sample=isHue?InlinePickerColor{float(x)/std::max(1,w-1),1,1}:InlinePickerColor{color.hue,float(x)/std::max(1,w-1),1.f-float(y)/std::max(1,h-1)};auto rgb=pickerRgb(sample);
    if(!enabled){float l=(rgb[0]+rgb[1]+rgb[2])/3;for(auto&v:rgb)v=.12f+l*.18f;}
    auto byte=[](float v){return unsigned(std::lround(pickerUnit(v)*255));};cache.pixels[size_t(y)*w+x]=(byte(rgb[0])<<16)|(byte(rgb[1])<<8)|byte(rgb[2]);
   }
  }
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  SetDIBitsToDevice(dc,r.left,r.top,w,h,0,0,0,h,cache.pixels.data(),&info,DIB_RGB_COLORS);
  HBRUSH border=CreateSolidBrush(enabled&&focused?RGB(249,91,108):RGB(108,116,134));FrameRect(dc,&r,border);DeleteObject(border);
 };
 draw(sv,false);draw(hue,true);
 const int x=sv.left+int(std::lround(pickerUnit(color.saturation)*std::max(0L,sv.right-sv.left-1)));
 const int y=sv.top+int(std::lround((1-pickerUnit(color.value))*std::max(0L,sv.bottom-sv.top-1)));
 int saved=SaveDC(dc);IntersectClipRect(dc,sv.left,sv.top,sv.right,sv.bottom);auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));HPEN outer=CreatePen(PS_SOLID,3,RGB(18,20,25));auto oldPen=SelectObject(dc,outer);Ellipse(dc,x-4,y-4,x+5,y+5);HPEN inner=CreatePen(PS_SOLID,1,RGB(250,252,255));SelectObject(dc,inner);Ellipse(dc,x-4,y-4,x+5,y+5);SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(outer);DeleteObject(inner);RestoreDC(dc,saved);
 const int marker=hue.left+int(std::lround(pickerUnit(color.hue)*std::max(0L,hue.right-hue.left-1)));
 saved=SaveDC(dc);IntersectClipRect(dc,hue.left,hue.top,hue.right,hue.bottom);HPEN outline=CreatePen(PS_SOLID,3,RGB(12,14,18));oldPen=SelectObject(dc,outline);MoveToEx(dc,marker,hue.top,nullptr);LineTo(dc,marker,hue.bottom);HPEN white=CreatePen(PS_SOLID,1,RGB(255,255,255));SelectObject(dc,white);MoveToEx(dc,marker,hue.top,nullptr);LineTo(dc,marker,hue.bottom);SelectObject(dc,oldPen);DeleteObject(outline);DeleteObject(white);RestoreDC(dc,saved);
}
}
