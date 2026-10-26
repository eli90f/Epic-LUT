#define NOMINMAX
#include <windows.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cassert>
#include "../native/game_assets.h"
#include "../native/preview_cache.h"
#include "../native/fullscreen_bars.h"
#include "../native/gpu_material_preview.h"
using namespace omega;namespace fs=std::filesystem;
int main(int argc,char**argv){try{
 auto bars=defaultFullscreenBars();for(auto b:bars){assert(b.x>=0&&b.y>=0&&b.x+b.w<=1800&&b.y+b.h<=1000);moveFullscreenBar(b,-500,5000);assert(b.x>=8&&b.y+b.h<=992);}
 auto records=scanGameLuts(L"D:\\SteamLibrary\\steamapps\\common\\Helldivers 2\\data");std::cout<<"Indexed "<<records.size()<<" LUTs\n";
 const bool high=argc>1;const int w=high?3840:900,h=high?1260:600;HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;uint32_t*bits;HBITMAP bmp=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,(void**)&bits,nullptr,0);auto old=SelectObject(dc,bmp);RECT rect{0,0,w,h};int successes=0;
 for(auto category:{L"Armor",L"Weapons",L"Vehicles"}){int attempts=0;bool ok=false;for(auto&r:records){if(r.category.find(category)==std::wstring::npos)continue;if(++attempts>12)break;try{
  auto start=std::chrono::steady_clock::now();auto model=loadGameItemPreview(L"D:\\SteamLibrary\\steamapps\\common\\Helldivers 2\\data",r.resourceId);double load=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();size_t textures=0;for(auto&part:model.parts)textures+=part.textures.size();assert(textures>0);auto doc=loadGameLut(r);auto before=doc.pixels;start=std::chrono::steady_clock::now();renderItemPreview(dc,rect,doc,0,model,.4f,.12f,1,true,false,true);double render=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(!gpuPreviewHasFrame()){auto reason=gpuPreviewStatus();throw std::runtime_error(std::string(reason.begin(),reason.end()));}GdiFlush();std::vector<uint32_t> first(bits,bits+w*h);int picked=-1,px=0,py=0;for(int y=0;y<h&&picked<0;y+=3)for(int x=0;x<w;x+=3)if((picked=pickItemPreview(x,y))>=0){px=x;py=y;break;}assert(picked>=0);renderItemPreview(dc,rect,doc,0,model,.4f,.12f,1,true,false,true,picked);GdiFlush();assert(first!=std::vector<uint32_t>(bits,bits+w*h));assert(pickItemPreview(-1,-1)==-1);assert(doc.pixels==before);auto cacheStart=std::chrono::steady_clock::now();auto again=loadGameItemPreview(L"D:\\SteamLibrary\\steamapps\\common\\Helldivers 2\\data",r.resourceId);double cached=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-cacheStart).count();assert(again.vertices.size()==model.vertices.size());std::wstring cat(category);std::string name(cat.begin(),cat.end());BITMAPFILEHEADER bf{};bf.bfType=0x4d42;bf.bfOffBits=sizeof(bf)+sizeof(BITMAPINFOHEADER);bf.bfSize=bf.bfOffBits+w*h*4;std::ofstream image;if(!high)image.open(fs::path("build/dev-preview")/(name+".bmp"),std::ios::binary);image.write((char*)&bf,sizeof(bf));image.write((char*)&bi.bmiHeader,sizeof(BITMAPINFOHEADER));image.write((char*)bits,w*h*4);std::cout<<"PASS "<<name<<" lut="<<r.resourceId<<" vertices="<<model.vertices.size()<<" triangles="<<model.triangles.size()<<" parts="<<model.parts.size()<<" textures="<<textures<<" load_ms="<<load<<" render_ms="<<render<<" cached_load_ms="<<cached<<" pick="<<picked<<" at="<<px<<","<<py<<"\n";ok=true;++successes;break;
 }catch(const std::exception&e){std::cout<<"MODEL FAILED "<<r.resourceId<<": "<<e.what()<<"\n";}}
 if(!ok)std::wcout<<L"NO VERIFIED PREVIEW FOR "<<category<<L"\n";
 }
 bool rejected=false;try{decodeItemPreview({}, {},0,[](uint64_t,size_t){return std::vector<uint8_t>{};},{});}catch(...){rejected=true;}assert(rejected);SelectObject(dc,old);DeleteObject(bmp);DeleteDC(dc);std::cout<<"Raster "<<w<<"x"<<h<<"; six toolbar panels within canvas; capture-loss reset in source\n";std::cout<<"Verified categories "<<successes<<"/3; invalid mesh rejected\n";return successes==3?0:2;
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
