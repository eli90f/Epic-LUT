#include "item_preview.h"
#include "gpu_material_preview.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <execution>
#include <numeric>
namespace omega {namespace {
struct V{float x,y,z;};V add(V a,V b){return{a.x+b.x,a.y+b.y,a.z+b.z};}V mul(V a,float v){return{a.x*v,a.y*v,a.z*v};}V mix(V a,V b,float t){return add(mul(a,1-t),mul(b,t));}float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}V norm(V a){return mul(a,1/std::sqrt(std::max(1e-12f,dot(a,a))));}float sat(float v){return std::clamp(v,0.f,1.f);}float safe(float v,float fallback=0){return std::isfinite(v)?v:fallback;}
V turn(V p,float yaw,float pitch){float c=std::cos(yaw),s=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch);float x=c*p.x+s*p.z,z=-s*p.x+c*p.z;return{x,cp*p.y-sp*z,sp*p.y+cp*z};}
float display(float x){x=std::max(0.f,safe(x));x=sat(x*(2.51f*x+.03f)/(x*(2.43f*x+.59f)+.14f));return std::pow(x,1/2.2f);}
uint32_t color(V c){return(uint32_t(display(c.x)*255)<<16)|(uint32_t(display(c.y)*255)<<8)|uint32_t(display(c.z)*255);}
struct Cache{uint64_t key=0;int w=0,h=0;RECT rect{};std::vector<uint32_t>pixels,overlay;std::vector<int>parts;};Cache cache; // UI-thread renderer; worker tiles own disjoint rows.
void label(HDC dc,const RECT&r,const wchar_t*s){SetTextColor(dc,RGB(205,211,224));SetBkMode(dc,TRANSPARENT);RECT b=r;InflateRect(&b,-18,-18);DrawTextW(dc,s,-1,&b,DT_WORDBREAK|DT_CENTER|DT_NOPREFIX);}
uint64_t hash(uint64_t h,const void*p,size_t n){auto b=(const uint8_t*)p;for(size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ULL;}return h;}
struct P{float x,y,z;V n;};
}
int pickItemPreview(int x,int y){if(gpuPreviewHasFrame())return pickGpuPreviewPart(x,y);if(x<cache.rect.left||x>=cache.rect.right||y<cache.rect.top||y>=cache.rect.bottom||cache.parts.empty())return -1;int px=int(int64_t(x-cache.rect.left)*cache.w/std::max(1L,cache.rect.right-cache.rect.left)),py=int(int64_t(y-cache.rect.top)*cache.h/std::max(1L,cache.rect.bottom-cache.rect.top));return cache.parts[size_t(py)*cache.w+px];}
int pickItemPreviewRow(int x,int y){return gpuPreviewHasFrame()?pickGpuPreviewRow(x,y):-1;}
std::wstring itemPreviewRenderStatus(){return gpuPreviewStatus();}
void renderItemPreview(HDC dc,const RECT&r,const LutDocument&doc,size_t row,const ItemPreviewModel&model,float yaw,float pitch,float zoom,bool checker,bool original,bool shading,int selected,size_t column){
 if(renderGpuItemPreview(dc,r,doc,row,model,yaw,pitch,zoom,checker,original,shading,selected,column))return;
 int rw=r.right-r.left,rh=r.bottom-r.top;if(rw<=0||rh<=0)return;
 if(model.vertices.empty()||model.triangles.empty()){cache.parts.clear();HBRUSH brush=CreateSolidBrush(RGB(18,20,27));FillRect(dc,&r,brush);DeleteObject(brush);label(dc,r,model.notice.empty()?L"Select a named game item to load its source mesh.":model.notice.c_str());return;}
 int w=rw,h=rh;uint64_t key=1469598103934665603ULL;auto bytes=[&](const void*p,size_t n){key=hash(key,p,n);};bytes(&model.identity,8);bytes(&w,4);bytes(&h,4);bytes(&yaw,4);bytes(&pitch,4);bytes(&zoom,4);bytes(&checker,sizeof(checker));bytes(&shading,sizeof(shading));bytes(&row,sizeof(row));bytes(doc.resourceId.data(),doc.resourceId.size());const auto&data=original?doc.original:doc.pixels;bytes(data.data(),data.size()*sizeof(Pixel));
 if(cache.key!=key||cache.w!=w||cache.h!=h){cache.key=key;cache.w=w;cache.h=h;cache.pixels.assign(size_t(w)*h,0);cache.parts.assign(size_t(w)*h,-1);std::vector<float>depth(size_t(w)*h,-std::numeric_limits<float>::infinity());
  V lo={1e30f,1e30f,1e30f},hi={-1e30f,-1e30f,-1e30f};for(auto&v:model.vertices){lo.x=std::min(lo.x,v.x);lo.y=std::min(lo.y,v.y);lo.z=std::min(lo.z,v.z);hi.x=std::max(hi.x,v.x);hi.y=std::max(hi.y,v.y);hi.z=std::max(hi.z,v.z);}V center=mul(add(lo,hi),.5f);float extent=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z,1e-6f});float fit=.78f*std::min(w,h)/extent*std::clamp(zoom,.15f,8.f);
  std::vector<P>points;points.reserve(model.vertices.size());for(auto&v:model.vertices){V p=turn({v.x-center.x,v.y-center.y,v.z-center.z},yaw,pitch);points.push_back({w*.5f+p.x*fit,h*.5f-p.y*fit,p.z,turn({v.nx,v.ny,v.nz},yaw,pitch)});}
  for(int y=0;y<h;++y){float g=float(y)/std::max(h-1,1);V back{.015f+.008f*g,.017f+.008f*g,.023f+.012f*g};uint32_t shades[2]={color(back),color(mul(back,1.2f))};for(int x=0;x<w;x+=24)std::fill_n(cache.pixels.begin()+size_t(y)*w+x,std::min(24,w-x),shades[checker?((x/24+y/24)&1):0]);}
  auto get=[&](size_t c,Pixel fallback=Pixel{0,0,0,0}){return row<doc.height&&c<doc.width&&row*doc.width+c<data.size()?data[row*doc.width+c]:fallback;};auto c=get(0,Pixel{.5f,.1f,.15f,0});V albedo={std::max(0.f,safe(c[0])),std::max(0.f,safe(c[1])),std::max(0.f,safe(c[2]))};float rough=std::clamp(safe(get(10,Pixel{.5f,0,0,0})[0],.5f),.04f,1.f);auto met=get(7);float metal=sat((safe(met[0])+safe(met[1])+safe(met[2])+safe(met[3]))*.25f);float emit=std::clamp(safe(get(13)[0]),0.f,100.f);V light=norm({-.45f,.7f,1}),view={0,0,1},half=norm(add(light,view));
  const int tiles=(h+63)/64;std::vector<int> tileIds(tiles);std::iota(tileIds.begin(),tileIds.end(),0);
  std::for_each(std::execution::par,tileIds.begin(),tileIds.end(),[&](int tile){
  for(auto&t:model.triangles){if(t.a>=points.size()||t.b>=points.size()||t.c>=points.size())continue;auto a=points[t.a],b=points[t.b],c=points[t.c];float area=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);if(std::abs(area)<.0001f)continue;int x0=std::max(0,int(std::floor(std::min({a.x,b.x,c.x})))),x1=std::min(w-1,int(std::ceil(std::max({a.x,b.x,c.x})))),y0=std::max(tile*64,int(std::floor(std::min({a.y,b.y,c.y})))),y1=std::min(std::min(h-1,tile*64+63),int(std::ceil(std::max({a.y,b.y,c.y}))));if(x0>x1||y0>y1)continue;V base=t.affected?albedo:V{.16f,.18f,.22f};float metallic=t.affected?metal:0,roughness=t.affected?rough:.55f;
   for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){float px=x+.5f,py=y+.5f;float wa=((b.x-px)*(c.y-py)-(b.y-py)*(c.x-px))/area,wb=((c.x-px)*(a.y-py)-(c.y-py)*(a.x-px))/area,wc=1-wa-wb;if(wa<0||wb<0||wc<0)continue;size_t at=size_t(y)*w+x;float z=wa*a.z+wb*b.z+wc*c.z;if(z<=depth[at])continue;depth[at]=z;cache.parts[at]=t.part;V n=norm(add(add(mul(a.n,wa),mul(b.n,wb)),mul(c.n,wc)));if(n.z<0)n=mul(n,-1);V col=base;if(shading){float nl=sat(dot(n,light)),nv=std::max(.001f,n.z),nh=sat(dot(n,half)),vh=sat(dot(view,half));float alpha=roughness*roughness,a2=alpha*alpha,den=nh*nh*(a2-1)+1;float distribution=a2/(3.14159265f*den*den+.000001f);float k=(roughness+1)*(roughness+1)/8;float geometry=(nl/(nl*(1-k)+k+.00001f))*(nv/(nv*(1-k)+k));V f0=mix({.04f,.04f,.04f},base,metallic);V fresnel=mix(f0,{1,1,1},std::pow(1-vh,5.f));col=add(mul(base,.16f+.8f*nl*(1-metallic)),mul(fresnel,distribution*geometry/(4*nv+.00001f)));if(t.affected)col=add(col,mul(base,emit));}cache.pixels[at]=color(col);}
  }
  });
 }
 cache.rect=r;const uint32_t*p=cache.pixels.data();if(selected>=0){cache.overlay=cache.pixels;for(int y=0;y<h;y++)for(int x=0;x<w;x++){size_t at=size_t(y)*w+x;if(cache.parts[at]!=selected)continue;bool edge=x==0||y==0||x==w-1||y==h-1||cache.parts[at-1]!=selected||cache.parts[at+1]!=selected||cache.parts[at-w]!=selected||cache.parts[at+w]!=selected;auto c=cache.pixels[at];if(edge)cache.overlay[at]=0xff667e;else cache.overlay[at]=((((c>>16)&255)*7+255)/8<<16)|((((c>>8)&255)*7+60)/8<<8)|((c&255)*7+95)/8;}p=cache.overlay.data();}
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;SetStretchBltMode(dc,HALFTONE);StretchDIBits(dc,r.left,r.top,rw,rh,0,0,w,h,p,&bi,DIB_RGB_COLORS,SRCCOPY);
}
}
