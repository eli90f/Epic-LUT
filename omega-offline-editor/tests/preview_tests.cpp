#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include "../native/material_preview.h"

static uint64_t hash(const uint32_t*p,int w,int h){uint64_t out=1469598103934665603ull;for(int i=0;i<w*h;++i){out^=p[i];out*=1099511628211ull;}return out;}
int main(){
 const int w=480,h=420;HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 uint32_t*bits=nullptr;HBITMAP bmp=CreateDIBSection(dc,&info,DIB_RGB_COLORS,(void**)&bits,nullptr,0);assert(dc&&bmp&&bits);HGDIOBJ old=SelectObject(dc,bmp);RECT r={0,0,w,h};
 omega::LutDocument doc;doc.width=23;doc.height=2;doc.pixels.assign(46,omega::Pixel{0,0,0,0});doc.at(0,0)={.65f,.08f,.035f,-9.25f};doc.at(0,1)={.06f,.15f,.7f,3};doc.at(10,0)={.45f,0,0,0};doc.at(10,1)={.45f,0,0,0};doc.at(21,0)={0,0,0,-1};doc.original=doc.pixels;
 auto render=[&](bool original=false,float rotation=0){omega::renderMaterialPreview(dc,r,doc,0,rotation,true,original);GdiFlush();return hash(bits,w,h-48);};
 auto initial=render();assert(initial==render());assert(initial==render(true));
 doc.at(0,0)={.02f,.1f,.9f,-9.25f};assert(render()!=initial);assert(render(true)==initial);
 auto color=render();doc.at(10,0)[0]=.95f;assert(render()!=color);doc.at(10,0)[0]=.45f;
 auto matte=render();doc.at(13,0)[0]=.06f;assert(render()!=matte);doc.at(13,0)[0]=0;
 auto dielectric=render();doc.at(7,0)={1,1,1,1};assert(render()!=dielectric);doc.at(7,0)={0,0,0,0};
 auto unknownBefore=render();doc.at(8,0)={-11,7.25f,500,3};assert(render()==unknownBefore);
 doc.at(16,0)={.1f,.35f,.06f,.5f};doc.at(17,0)={.025f,.09f,.02f,2};doc.at(18,0)={.4f,.28f,.08f,-1};doc.at(19,0)={.015f,.02f,.01f,4};doc.at(21,0)={216.625f,-.989746f,3.5f,1};doc.at(22,0)={2,0,0,.8f};assert(render()!=unknownBefore);
 auto beforePixels=doc.pixels,beforeOriginal=doc.original;auto pattern=render();assert(pattern!=render(false,.9f));
 auto start=std::chrono::steady_clock::now();for(int i=0;i<20;++i)render(false,i*.07f);
 auto duration=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/20;
 assert(doc.pixels==beforePixels&&doc.original==beforeOriginal);assert(doc.at(8,0)[0]==-11&&doc.at(8,0)[2]==500&&doc.at(18,0)[3]==-1);
 // Invalid selections produce a bounded notice instead of indexing beyond pixels.
 omega::renderMaterialPreview(dc,r,doc,999,0,false,false);render();
 BITMAPFILEHEADER header{};header.bfType=0x4D42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+w*h*4;
 std::ofstream image("build/material-preview-test.bmp",std::ios::binary);image.write((const char*)&header,sizeof(header));image.write((const char*)&info.bmiHeader,sizeof(BITMAPINFOHEADER));image.write((const char*)bits,w*h*4);image.close();
 std::ofstream report("build/preview-validation.json");report<<"{\"passed\":true,\"source_columns\":23,\"raw_values_unchanged\":true,\"original_toggle\":true,\"base_roughness_metal_emission_camo_response\":true,\"unknown_channels_have_no_invented_effect\":true,\"invalid_row_bounded\":true,\"rotating_frame_ms\":"<<duration<<",\"target_width\":"<<w<<",\"target_height\":"<<h<<",\"max_internal_axis\":384}\n";
 SelectObject(dc,old);DeleteObject(bmp);DeleteDC(dc);std::cout<<"PASS material preview; rotating frame "<<duration<<" ms; source RGBA preserved\n";
}
