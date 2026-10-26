#define NOMINMAX
#include <windows.h>
#include "../native/game_assets.h"
#include "../native/lut_io.h"
#include "../native/gpu_material_preview.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>
using namespace omega;
void image(const char* name,ItemPreviewModel& m,LutDocument d){int w=1000,h=1200;HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};bi.bmiHeader.biSize=40;bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;uint32_t*bits;auto bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,(void**)&bits,nullptr,0);auto old=SelectObject(dc,bitmap);for(int i=0;i<w*h;++i)bits[i]=0x20232b;RECT r{0,0,w,h};renderItemPreview(dc,r,d,0,m,.25f,.04f,1,true,false,true);if(!gpuPreviewHasFrame()){auto s=gpuPreviewStatus();throw std::runtime_error(std::string(s.begin(),s.end()));}GdiFlush();BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+40;file.bfSize=file.bfOffBits+w*h*4;std::ofstream out(std::string("build/armor-preview/")+name+".bmp",std::ios::binary);out.write((char*)&file,sizeof(file));out.write((char*)&bi.bmiHeader,40);out.write((char*)bits,w*h*4);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);}
int main(){try{const std::wstring folder=L"D:/SteamLibrary/steamapps/common/Helldivers 2/data";auto records=scanGameLuts(folder);std::string armor,cape;size_t ai=0,ci=0;LutDocument armorDoc,capeDoc;std::wstring selectedArmor;
for(auto&r:records){auto owners=gameItemPreviewOwners(r.resourceId);for(size_t i=0;i<owners.size();++i)if(owners[i].rfind(L"[Armor]",0)==0&&r.width==23&&armor.empty()){armor=r.resourceId;ai=i;armorDoc=loadGameLut(r);selectedArmor=owners[i];}else if(owners[i].rfind(L"[Cape]",0)==0&&r.width==16&&cape.empty()){cape=r.resourceId;ci=i;capeDoc=loadGameLut(r);}}
std::wstring chosenHelmet;for(auto& r:records){auto owners=gameItemPreviewOwners(r.resourceId);bool found=false;for(size_t k=0;k<owners.size();++k)if(owners[k].rfind(L"[Helmet]",0)==0){try{auto h=loadGameItemPreview(folder,r.resourceId,k);chosenHelmet=owners[k].substr(0,owners[k].find(L" / "));found=true;break;}catch(...){}}if(found)break;}
 auto outfit=loadGameItemPreview(folder,armor,ai);assert(!chosenHelmet.empty());assert(std::any_of(outfit.parts.begin(),outfit.parts.end(),[&](auto& p){return p.name.rfind(chosenHelmet,0)==0;}));std::wcout<<L"ARMOR "<<selectedArmor<<L" meshes="<<outfit.sourceMeshes<<L" parts="<<outfit.parts.size()<<L" vertices="<<outfit.vertices.size()<<L" "<<outfit.notice<<std::endl;assert(outfit.parts.size()>5);image("assembled-armour",outfit,armorDoc);
auto withCape=loadGameItemPreview(folder,cape,ci);std::wcout<<L"CAPE "<<withCape.notice<<L" meshes="<<withCape.sourceMeshes<<L" parts="<<withCape.parts.size()<<L" vertices="<<withCape.vertices.size()<<std::endl;assert(withCape.vertices.size()>outfit.vertices.size());assert(std::any_of(withCape.parts.begin(),withCape.parts.end(),[&](auto& p){return p.name.rfind(chosenHelmet,0)==0;}));image("cape-with-selected-armour",withCape,capeDoc);
auto piece=loadGameItemPreview(folder,armor,ai,true);assert(piece.vertices.size()<outfit.vertices.size());std::cout<<"PASS outfit assembly, retained armour for cape, advanced single-piece preview and GPU rendering\n";return 0;}catch(std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
