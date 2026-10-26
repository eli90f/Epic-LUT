// Column meanings: Epic LUT R5.9.3 src/core/semantics.lua (S.columns/S.hints).
// Camo activation/UV/roughness channels also checked against src/editor/basic_camo.lua.
// Lighting, geometry and procedural masks below are preview approximations.
#include "material_preview.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace omega {
namespace {
const wchar_t* const names[23]={
 L"Primary color / shader mode",L"Bump map / inversion controls",
 L"Bump mask 1 color",L"Bump mask 1",L"Bump mask 2",
 L"Bump mask 2 inner color",L"Bump mask 2 outer color",L"Metallic mask",
 L"Unknown 9",L"Gloss / roughness mask",L"Roughness / rim",L"Unknown 12",
 L"Curvature gradient",L"Emissive strength",L"Lighting tint override",L"Unknown 16",
 L"Camo color 1",L"Camo color 2",L"Camo color 3",L"Camo color 4",
 L"Mask 5 inversion",L"Camo controls",L"Bump scaling / matte-gloss"
};
const wchar_t* const help[23]={
 L"RGB: primary color. A: shader behavior; curvature, emission and camo can depend on it. Alpha is not generic transparency.",
 L"R: bump-map index 0-25. G: intensity. B/A: mask inversion controls. Native bump textures are not included in this sample.",
 L"RGB: first bump-mask color. Zero RGB disables its color contribution. Other channel behavior is not further confirmed.",
 L"First bump-mask layer. Signed values can reverse channel influence; exact texture/mask evaluation is shader dependent.",
 L"Second bump-mask layer, above the first. Exact texture/mask evaluation is shader dependent.",
 L"Inner RGB color of the second bump mask, strongest where that mask is strong.",
 L"Outer RGB color of the second bump mask. A affects metallic-mask inversion.",
 L"RGBA: metallic masks. Negative mask values may invert influence. These are masks, not four independently verified metal percentages.",
 L"Unconfirmed mapping. All raw RGBA values are retained; no effect is invented for the material sample.",
 L"RGBA: gloss/roughness masks. Signed values alter influence; the sample uses an illustrative mask response.",
 L"R: roughness. B: rim effect, dependent on primary-alpha mode. G/A behavior is not confirmed here.",
 L"Unconfirmed mapping. All raw RGBA values are retained; no effect is invented for the material sample.",
 L"RGB: curvature gradient. A: intensity. Requires a compatible primary-alpha mode in the game.",
 L"R: emissive strength, commonly 0.001-0.06. Other channels are unconfirmed. Glow tint can come from primary RGB.",
 L"RGBA: lighting tint override. Usually zero; nonzero tint depends on lighting. Exact shader behavior is not reproduced.",
 L"Unconfirmed mapping. All raw RGBA values are retained; no effect is invented for the material sample.",
 L"RGB: first camo color. A influences camo roughness; behavior depends on primary-alpha mode.",
 L"RGB: second camo color. A also participates in the fifth bump mask.",
 L"RGB: third camo color. A also participates in the fifth bump mask.",
 L"RGB: fourth camo color; use depends on pattern type. A also affects the fifth mask.",
 L"A: fifth-mask inversion. RGB behavior is unconfirmed.",
 L"R: camo-mask sharpness. G: mask bias. B: camo UV scale. A: pattern -1 (off) through 5. Large values can be valid.",
 L"R: detail UV scale. A: camo roughness blending; zero is valid. Other effects depend on overlapping masks."
};

struct V3{float x,y,z;};
V3 operator+(V3 a,V3 b){return{a.x+b.x,a.y+b.y,a.z+b.z};}
V3 operator*(V3 a,float b){return{a.x*b,a.y*b,a.z*b};}
V3 operator*(V3 a,V3 b){return{a.x*b.x,a.y*b.y,a.z*b.z};}
V3 mix(V3 a,V3 b,float t){return a*(1-t)+b*t;}
float dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
float sat(float v){return std::max(0.f,std::min(1.f,v));}
float finite(float v,float fallback=0){return std::isfinite(v)?v:fallback;}
float bounded(float v,float lo,float hi){return std::max(lo,std::min(hi,finite(v)));}
V3 normal(V3 v){float n=std::sqrt(std::max(1e-12f,dot(v,v)));return v*(1/n);}
V3 rgb(const Pixel&p){return{bounded(p[0],-100000.f,100000.f),bounded(p[1],-100000.f,100000.f),bounded(p[2],-100000.f,100000.f)};}
float lengthColor(V3 v){return std::abs(v.x)+std::abs(v.y)+std::abs(v.z);}
float fract(float x){return x-std::floor(x);}
float smooth(float x){x=sat(x);return x*x*(3-2*x);}
float noise(int x,int y,unsigned seed){
 uint32_t n=uint32_t(x)*0x9e3779b9u^uint32_t(y)*0x85ebca6bu^seed*0xc2b2ae35u;
 n^=n>>16;n*=0x7feb352du;n^=n>>15;n*=0x846ca68bu;n^=n>>16;return(n&0xffffff)/16777215.f;
}
float valueNoise(float x,float y,unsigned seed){
 int ix=(int)std::floor(x),iy=(int)std::floor(y);float fx=smooth(fract(x)),fy=smooth(fract(y));
 float a=noise(ix,iy,seed)*(1-fx)+noise(ix+1,iy,seed)*fx;
 float b=noise(ix,iy+1,seed)*(1-fx)+noise(ix+1,iy+1,seed)*fx;return a*(1-fy)+b*fy;
}
float pattern(float u,float v,int type){
 // Deliberately illustrative: these are not recovered HD2 camo textures.
 switch(type){
 case 0:return .70f*valueNoise(u*5,v*5,17)+.30f*valueNoise(u*11,v*11,5);
 case 1:return .55f+.28f*std::sin(u*14+std::sin(v*9)*1.7f)+.12f*std::sin(v*18);
 case 2:return .8f*valueNoise(std::floor(u*12),std::floor(v*12),29)+.2f*valueNoise(u*3,v*3,2);
 case 3:return .65f*valueNoise(u*7,v*5,37)+.35f*valueNoise(u*17,v*17,3);
 case 4:return .5f+.32f*std::sin(v*17+std::sin(u*7)*2.3f);
 default:return valueNoise(std::floor(u*18),std::floor(v*18),73);
 }
}
float signedMask(float value,float mask){
 // Sign inversion is known; the spatial mask itself is a sample placeholder.
 return value>=0?std::min(value,8.f)*mask:std::min(-value,8.f)*(1-mask);
}
float display(float value){
 value=std::max(0.f,finite(value));
 // ACES-like display curve. It never writes back to HDR/negative LUT values.
 value=sat(value*(2.51f*value+.03f)/(value*(2.43f*value+.59f)+.14f));
 return value<=.0031308f?12.92f*value:1.055f*std::pow(value,1/2.4f)-.055f;
}
uint32_t pixel(V3 c){
 auto byte=[](float x){return(uint32_t)std::lround(sat(display(x))*255);};
 return byte(c.x)<<16|byte(c.y)<<8|byte(c.z);
}
struct Cache{
 uint64_t key=0;int width=0,height=0;std::vector<uint32_t> pixels;
};
thread_local Cache cache;
uint64_t hashRow(const LutDocument&doc,const std::vector<Pixel>&data,size_t row,int width,int height,float rotation,bool checker){
 uint64_t h=1469598103934665603ull;
 auto bytes=[&](const void*p,size_t n){const auto*b=(const unsigned char*)p;for(size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ull;}};
 bytes(&doc.width,sizeof(doc.width));bytes(&doc.height,sizeof(doc.height));bytes(&row,sizeof(row));
 bytes(&width,sizeof(width));bytes(&height,sizeof(height));bytes(&rotation,sizeof(rotation));bytes(&checker,sizeof(checker));
 size_t start=row*doc.width;if(start<data.size())bytes(data.data()+start,std::min<size_t>(doc.width,data.size()-start)*sizeof(Pixel));return h;
}
void renderPixels(const LutDocument&doc,const std::vector<Pixel>&data,size_t row,int w,int h,float rotation,bool checker){
 auto get=[&](size_t column,Pixel fallback=Pixel{0,0,0,0}){return column<doc.width&&row*doc.width+column<data.size()?data[row*doc.width+column]:fallback;};
 const bool material=doc.width==23,patternTable=doc.width==3;
 Pixel primary=get(0,Pixel{.45f,.14f,.10f,0}),bump=get(1),metal=get(7),gloss=get(9),rough=get(10,Pixel{.5f,0,0,0}),camo=get(21),scale=get(22),curvature=get(12),emission=get(13),tint=get(14);
 if(patternTable){metal=get(1);rough=Pixel{get(2)[3],0,0,0};}
 V3 base=rgb(primary),detail=rgb(get(2)),inner=rgb(get(5)),outer=rgb(get(6)),curv=rgb(curvature);
 std::array<V3,4> colors={rgb(get(16)),rgb(get(17)),rgb(get(18)),rgb(get(19))};
 int camoType=(int)bounded(camo[3],-1,5);
 bool camoOn=material&&finite(camo[3])>=0&&finite(camo[3])<=5&&std::floor(finite(camo[3]))==finite(camo[3])&&finite(camo[0])!=0&&finite(camo[2])!=0;
 float uv=std::max(.2f,std::min(24.f,std::abs(finite(camo[2],1))));
 float detailUV=std::max(.4f,std::min(40.f,std::abs(finite(scale[0],4))));
 float c=std::cos(rotation),s=std::sin(rotation);
 V3 light=normal({-.5f,.65f,.75f}),half=normal(light+V3{0,0,1});
 const float radius=.82f*std::min(w,h)*.5f,centerX=w*.5f,centerY=h*.47f;
 cache.pixels.resize(size_t(w)*h);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){
  float grad=float(y)/std::max(1,h-1);V3 back={.022f+.012f*grad,.022f+.010f*grad,.029f+.012f*grad};
  if(checker){float tile=((x/18+y/18)&1)?1.f:.68f;back={.045f*tile,.045f*tile,.055f*tile};}
  // Soft contact shadow, unrelated to material data.
  float shx=(x-centerX)/(radius*1.13f),shy=(y-(centerY+radius*.9f))/(radius*.18f);
  float shadow=std::exp(-(shx*shx+shy*shy)*2.f)*.55f;back=back*(1-shadow);
  float nx=(x-centerX)/radius,ny=(centerY-y)/radius,r2=nx*nx+ny*ny;
  if(r2>1){cache.pixels[size_t(y)*w+x]=pixel(back);continue;}
  float nz=std::sqrt(std::max(0.f,1-r2));V3 n={nx,ny,nz};
  float ox=c*nx-s*nz,oz=s*nx+c*nz;
  float u=std::atan2(ox,oz)*.1591549431f+.5f,v=std::asin(std::max(-1.f,std::min(1.f,ny)))*.3183098862f+.5f;
  float m1=.5f+.5f*std::sin(u*detailUV*12+std::sin(v*detailUV*8));
  float m2=valueNoise(u*detailUV*4,v*detailUV*4,31);
  V3 albedo=base;float roughness=material||patternTable?bounded(rough[0],.035f,1):.5f;
  if(material){
   // Detail color placement is illustrative because mesh masks were not loaded.
   if(lengthColor(detail)>1e-7f)albedo=mix(albedo,detail,.24f*m1);
   if(lengthColor(inner)>1e-7f)albedo=mix(albedo,inner,.21f*m2);
   if(lengthColor(outer)>1e-7f)albedo=mix(albedo,outer,.17f*(1-m2));
   if(camoOn){
    float p=pattern(u*uv,v*uv,camoType);
    p=sat(p+std::tanh(finite(camo[1])*.2f)*.15f);
    float sharp=std::max(.4f,std::min(12.f,std::abs(finite(camo[0]))*.03f));
    float bands=sat((p-.5f)*sharp+.5f)*3.999f;int idx=std::min(3,(int)bands);
    albedo=colors[idx];
    // Only C17 alpha's roughness role is known; C18-C20 alpha mask roles are not invented here.
    roughness=roughness*(1-sat(scale[3]))+bounded(get(16)[3],.035f,1)*sat(scale[3]);
   }
   float curvWeight=sat(std::abs(finite(curvature[3]))*.25f)*std::pow(1-nz,2.f);
   if(lengthColor(curv)>1e-7f)albedo=mix(albedo,curv,curvWeight);
   float intensity=bounded(bump[1],-4,4)*.025f;
   n=normal({nx+intensity*std::cos(u*detailUV*47),ny+intensity*std::sin(v*detailUV*53),nz});
   float maskRough=(signedMask(gloss[0],m1)+signedMask(gloss[1],m2)+signedMask(gloss[2],1-m1)+signedMask(gloss[3],1-m2))*.025f;
   roughness=bounded(roughness+maskRough,.035f,1);
  }
  float metalness=0;
  if(material)metalness=sat((signedMask(metal[0],m1)+signedMask(metal[1],m2)+signedMask(metal[2],1-m1)+signedMask(metal[3],1-m2))*.25f);
  else if(patternTable)metalness=sat((finite(metal[0])+finite(metal[1])+finite(metal[2]))/3);
  float nl=sat(dot(n,light)),nh=sat(dot(n,half)),nv=sat(n.z);
  float exponent=std::min(400.f,2.f/(roughness*roughness+.005f));
  float spec=std::pow(nh,exponent)*(.4f+3.7f*(1-roughness));
  float fresnel=1-nv;fresnel=fresnel*fresnel*fresnel*fresnel*fresnel;
  V3 f0=mix(V3{.04f,.04f,.04f},albedo,metalness);V3 fresnelColor=f0*(1-fresnel)+V3{1,1,1}*fresnel;
  V3 shaded=albedo*((.18f+.72f*nl)*(1-metalness*.76f))+fresnelColor*spec;
  shaded=shaded+V3{.14f,.17f,.22f}*(std::pow(sat(-n.x*.65f+n.z*.2f),4.f)*(.2f+.8f*metalness));
  if(material){
   float rim=std::pow(1-nv,3.f)*bounded(rough[2],0,20)*.1f;
   shaded=shaded+albedo*(bounded(emission[0],0,100000)*12.f+rim);
   V3 lightingTint=rgb(tint);if(lengthColor(lightingTint)>1e-7f)shaded=shaded+lightingTint*(.06f+.10f*nl);
  }
  if(patternTable){float opacity=sat(get(1)[3]);shaded=mix(back,shaded,opacity);}
  cache.pixels[size_t(y)*w+x]=pixel(shaded);
 }
}
void text(HDC dc,const RECT&box,const wchar_t*label,COLORREF color,int height){
 HFONT font=CreateFontW(-height,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 HGDIOBJ old=font?SelectObject(dc,font):nullptr;int mode=SetBkMode(dc,TRANSPARENT);COLORREF previous=SetTextColor(dc,color);RECT r=box;
 DrawTextW(dc,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
 SetTextColor(dc,previous);SetBkMode(dc,mode);if(old)SelectObject(dc,old);if(font)DeleteObject(font);
}
}

const wchar_t*lutColumnName(size_t column){return column<23?names[column]:L"Unclassified column";}
const wchar_t*lutColumnHelp(size_t column){return column<23?help[column]:L"No verified mapping. Raw channels are retained.";}

void renderMaterialPreview(HDC dc,const RECT&rect,const LutDocument&doc,size_t row,float rotation,bool checker,bool showOriginal){
 int outW=rect.right-rect.left,outH=rect.bottom-rect.top;if(!dc||outW<=0||outH<=0)return;
 const bool original=showOriginal&&doc.original.size()==doc.pixels.size();const auto&data=original?doc.original:doc.pixels;
 if(!doc.width||!doc.height||row>=doc.height||size_t(doc.width)*doc.height>data.size()){
  HBRUSH brush=CreateSolidBrush(RGB(15,15,19));FillRect(dc,&rect,brush);DeleteObject(brush);text(dc,rect,L"Select a valid LUT row for the material sample",RGB(155,151,163),13);return;
 }
 int drawH=std::max(1,outH-48);float factor=std::min(1.f,384.f/std::max(outW,drawH));
 int w=std::max(1,(int)std::lround(outW*factor)),h=std::max(1,(int)std::lround(drawH*factor));rotation=finite(rotation);
 auto key=hashRow(doc,data,row,w,h,rotation,checker);
 if(cache.key!=key||cache.width!=w||cache.height!=h){renderPixels(doc,data,row,w,h,rotation,checker);cache.key=key;cache.width=w;cache.height=h;}
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 int stretch=SetStretchBltMode(dc,HALFTONE);POINT origin{};SetBrushOrgEx(dc,0,0,&origin);
 StretchDIBits(dc,rect.left,rect.top,outW,drawH,0,0,w,h,cache.pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
 SetBrushOrgEx(dc,origin.x,origin.y,nullptr);SetStretchBltMode(dc,stretch);
 RECT footer={rect.left,rect.top+drawH,rect.right,rect.bottom};HBRUSH brush=CreateSolidBrush(RGB(15,15,19));FillRect(dc,&footer,brush);DeleteObject(brush);
 RECT first={footer.left+4,footer.top+2,footer.right-4,footer.top+24},second={footer.left+4,footer.top+23,footer.right-4,footer.bottom-2};
 const wchar_t*title=original?L"ORIGINAL VALUES - APPROXIMATE MATERIAL SAMPLE":L"EDITED VALUES - APPROXIMATE MATERIAL SAMPLE";
 text(dc,first,title,RGB(238,118,123),12);
 text(dc,second,doc.width==23?L"No game mesh/shader. Detail and camo masks are illustrative.":doc.width==3?L"Pattern-LUT sample; masks and shading are approximate.":L"Color-only sample; this LUT's material shader is not reproduced.",RGB(145,141,154),11);
}
}
