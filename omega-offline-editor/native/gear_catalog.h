#pragma once
// Disk-only customization kit reader. Layout: Filediver datalibrary/armor_sets.go;
// sealed-box format: SHODAN gamedata.lua. No game modules or process access.
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <sstream>
namespace omega {
struct GearPiece {
 std::string unit, archive;
 uint32_t kit=0,set=0,kind=0,slot=0,body=3,type=0;bool contextOnly=false;
 std::wstring name;
 std::map<std::string,std::string> overrides;
};
inline std::vector<GearPiece> readGearCatalog(const std::filesystem::path& folder){
 namespace fs=std::filesystem;
 auto path=folder/L"game"/L"generated_customization_armor_sets.dl_bin";
 if(!fs::exists(path))return {};
 auto require=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
 auto read=[](const fs::path& p){std::ifstream f(p,std::ios::binary);return std::vector<unsigned char>(std::istreambuf_iterator<char>(f),{});};
 auto encrypted=read(path);require(encrypted.size()>48&&encrypted.size()<64*1024*1024,"Invalid customization table size");
 static HMODULE sodium=[&](){
  auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(103),MAKEINTRESOURCEW(10));
  require(resource!=nullptr,"Customization reader dependency missing from this test build");
  auto n=SizeofResource(nullptr,resource);auto data=static_cast<const unsigned char*>(LockResource(LoadResource(nullptr,resource)));
  require(data&&n,"Cannot load customization reader dependency");
  wchar_t exe[32768];require(GetModuleFileNameW(nullptr,exe,32768)>0,"Cannot locate editor folder");
  auto dir=fs::path(exe).parent_path()/L"lut-editor-cache"/L"runtime";fs::create_directories(dir);
  auto dll=dir/L"libsodium-gear.dll";
  auto old=read(dll);if(old.size()!=n||!std::equal(old.begin(),old.end(),data)){
   std::ofstream f(dll,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(data),n);require(bool(f),"Cannot write LUT reader dependency cache");
  }
  auto module=LoadLibraryExW(dll.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  require(module!=nullptr,"Cannot load LUT table reader");return module;
 }();
 using Init=int(*)();using Open=int(*)(unsigned char*,const unsigned char*,unsigned long long,const unsigned char*,const unsigned char*);
 auto init=reinterpret_cast<Init>(GetProcAddress(sodium,"sodium_init"));auto open=reinterpret_cast<Open>(GetProcAddress(sodium,"crypto_box_seal_open"));
 require(init&&open&&init()>=0,"Invalid LUT table reader exports");
 auto unhex=[](const std::string& h){std::vector<unsigned char> v;for(size_t i=0;i<h.size();i+=2)v.push_back(static_cast<unsigned char>(std::stoul(h.substr(i,2),nullptr,16)));return v;};
 const auto pk=unhex("2dc9aa7c30e6eed5f7fd0e0e95c7287299b126f79520cc41de750982ee7ff50c"),sk=unhex("a0296b81310b58f013d8a9b343a1c292008003dfd9ab15fcf351543048b49b64");
 std::vector<unsigned char> b(encrypted.size()-48);require(open(b.data(),encrypted.data(),encrypted.size(),pk.data(),sk.data())==0,"Cannot authenticate customization table");
 auto u32=[&](size_t a){require(a<=b.size()&&b.size()-a>=4,"Customization table read outside bounds");uint32_t v;memcpy(&v,b.data()+a,4);return v;};
 auto u64=[&](size_t a){require(a<=b.size()&&b.size()-a>=8,"Customization table read outside bounds");uint64_t v;memcpy(&v,b.data()+a,8);return v;};
 auto hex=[](uint64_t v){std::ostringstream s;s<<std::hex<<std::setw(16)<<std::setfill('0')<<v;return s.str();};
 struct Name{const char* id;const wchar_t* name;};static const Name names[]={
#include "gear_archive_names.inc"
 };
 static const auto labels=[](){std::map<std::string,std::wstring> m;for(auto& n:names)m.emplace(n.id,n.name);return m;}();
 const wchar_t* slots[]={L"Helmet",L"Cape",L"Torso",L"Hips",L"Left leg",L"Right leg",L"Left arm",L"Right arm",L"Left shoulder",L"Right shoulder"};
 std::vector<GearPiece> result;size_t at=4;auto count=u32(0);require(count<=10000,"Excessive customization kit count");
 for(size_t i=0;i<count;++i){
  require(u32(at)==0x444c444c&&u32(at+4)==1&&b.at(at+16)==1,"Unsupported customization block format");
  size_t base=at+24,size=u32(at+12);require(base<=b.size()&&size<=b.size()-base&&size>=64,"Invalid customization block size");size_t end=base+size;
  auto kind=u32(base+40);require(kind<=2,"Unknown customization kit type");auto archive=hex(u64(base+32));auto label=labels.find(archive);
  std::wstring name=label==labels.end()?(kind==0?L"[Armor] Kit ":kind==1?L"[Helmet] Kit ":L"[Cape] Kit ")+std::to_wstring(u32(base)):label->second;
  auto array=[&](size_t field,size_t stride){auto off=u64(field),n=u64(field+8);require(n<=4096&&off<=size&&n<=(size-off)/stride,"Customization array outside block");return std::make_pair(base+size_t(off),size_t(n));};
  auto bodies=array(base+48,24);
  for(size_t j=0;j<bodies.second;++j){size_t body=bodies.first+j*24;auto bodyType=u32(body);auto pieces=array(body+8,112);
   for(size_t k=0;k<pieces.second;++k){size_t p=pieces.first+k*112;auto slot=u32(p+8);
    if(!((kind==0&&slot>=1&&slot<=9)||(kind==1&&slot==0)||(kind==2&&slot==1)))continue;
    GearPiece piece;piece.kit=u32(base);piece.set=u32(base+8);piece.kind=kind;piece.slot=slot;piece.body=bodyType;piece.type=u32(p+12);piece.contextOnly=kind==0&&slot==1;piece.unit=hex(u64(p));piece.archive=archive;piece.name=name+L" / "+slots[slot];if(bodyType<2)piece.name+=bodyType==0?L" (stocky)":L" (slim)";
    const char* channels[]={"material_lut","pattern_lut","cape_lut","cape_gradient","cape_nac","decal_scalar_fields","base_data","decal_sheet"};
    for(size_t c=0;c<8;++c){auto value=u64(p+24+c*8);if(value)piece.overrides[channels[c]]=hex(value);}
    result.push_back(std::move(piece));
   }
  }
  at=end;
 }
 require(at==b.size(),"Trailing customization table data");return result;
}
}

