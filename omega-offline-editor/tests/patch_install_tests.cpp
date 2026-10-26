#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../native/patch_install.h"
#include "../native/lut_io.h"
#include "../native/game_assets.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace omega;namespace fs=std::filesystem;using Bytes=std::vector<uint8_t>;
static Bytes read(const fs::path& p){std::ifstream f(p,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(f),{});}
static void write(const fs::path& p,const Bytes& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));assert(f);}
template<class F>static void reject(F&& f){bool caught=false;try{f();}catch(const std::exception& e){caught=true;std::cout<<"EXPECTED "<<e.what()<<"\n";}assert(caught);}
static void loose(const fs::path& root,const std::wstring& stem,const std::string& id,float value){
 LutDocument d;d.width=23;d.height=2;d.pixels.assign(46,Pixel{value,-1,2,1});d.original=d.pixels;d.resourceId=id;d.archiveId="9ba626afa44a3aa3";auto dds=encodeDDS(d);d.nativeHeader.assign(192,0);d.nativeHeader.insert(d.nativeHeader.end(),dds.begin(),dds.begin()+148);auto temp=root/(L"fixture_"+stem);exportPatch(d,temp.wstring());
 for(auto suffix:{L"",L".gpu_resources",L".stream"})fs::rename(temp/(std::wstring(L"9ba626afa44a3aa3.patch_0")+suffix),root/(stem+suffix));fs::remove(temp);
}
static std::map<std::wstring,Bytes> inventory(const fs::path& folder){std::map<std::wstring,Bytes> result;for(auto& f:fs::directory_iterator(folder))if(f.is_regular_file())result[f.path().filename().wstring()]=read(f.path());return result;}
int wmain(){try{
 auto root=fs::absolute(L"build/install-fixture_"+std::to_wstring(GetTickCount64()));auto game=root/L"data",app=root/L"app";fs::create_directories(game);fs::create_directories(app);
 loose(game,L"9ba626afa44a3aa3","1111111111111111",.25f);loose(game,L"0123456789abcdef","2222222222222222",.5f);loose(game,L"9ba626afa44a3aa3.patch_7","3333333333333333",.75f);
 auto original=inventory(game);auto records=scanGameLuts(game.wstring());assert(records.size()==3);std::vector<LutDocument> docs;
 for(auto& r:records)if(r.resourceId!="3333333333333333"){auto d=loadGameLut(r);d.pixels[0][0]+=.125f;docs.push_back(d);}assert(docs.size()==2);
 std::wcout<<installLutPatches(docs,game.wstring(),app.wstring())<<L"\n";auto installed=loadInstalledLutPatches(game.wstring(),app.wstring());assert(installed.size()==2);
 for(size_t i=0;i<docs.size();++i)assert(installed[i].pixels==docs[i].pixels&&installed[i].original==docs[i].original&&installed[i].nativeHeader==docs[i].nativeHeader);
 assert(fs::exists(game/L"9ba626afa44a3aa3.patch_8")&&fs::exists(game/L"9ba626afa44a3aa3.patch_9"));for(auto& p:original)assert(read(game/p.first)==p.second);
 // Open the active own patches through the cached archive reader, then reinstall.
 auto activeRecords=scanGameLuts(game.wstring());std::vector<LutDocument> fromOwn;
 for(auto& r:activeRecords)if(r.resourceId!="3333333333333333"){auto d=loadGameLut(r);d.pixels[0][1]+=.25f;fromOwn.push_back(d);}assert(fromOwn.size()==2);
 installLutPatches(fromOwn,game.wstring(),app.wstring());auto after=loadInstalledLutPatches(game.wstring(),app.wstring());assert(after.size()==2);for(auto& d:after){auto f=std::find_if(fromOwn.begin(),fromOwn.end(),[&](const auto& v){return v.resourceId==d.resourceId;});assert(f!=fromOwn.end()&&f->pixels==d.pixels);}
 // Wrong source data fails after old owned removal and must restore the complete set.
 auto beforeFailure=inventory(game);auto bad=after;bad[0].nativeHeader[0]^=1;reject([&]{installLutPatches(bad,game.wstring(),app.wstring());});assert(inventory(game)==beforeFailure);assert(loadInstalledLutPatches(game.wstring(),app.wstring()).size()==2);
 // A manifest write failure happens after new triplets: original owned set must return.
 fs::path manifest;for(auto& f:fs::directory_iterator(app/L"State"))if(f.path().extension()==L".bin")manifest=f.path();assert(!manifest.empty());assert(SetFileAttributesW(manifest.c_str(),FILE_ATTRIBUTE_READONLY));after[0].pixels[0][0]+=.125f;reject([&]{installLutPatches(after,game.wstring(),app.wstring());});assert(inventory(game)==beforeFailure);assert(SetFileAttributesW(manifest.c_str(),FILE_ATTRIBUTE_NORMAL));
 // An externally changed owned file is never overwritten or deleted.
 auto ownedMain=game/L"9ba626afa44a3aa3.patch_8";auto pristine=read(ownedMain),tampered=pristine;tampered.back()^=1;write(ownedMain,tampered);reject([&]{restoreLutPatch(game.wstring(),app.wstring());});reject([&]{installLutPatches(docs,game.wstring(),app.wstring());});assert(read(ownedMain)==tampered);write(ownedMain,pristine);
 std::wcout<<restoreLutPatch(game.wstring(),app.wstring())<<L"\n";assert(inventory(game)==original);assert(loadInstalledLutPatches(game.wstring(),app.wstring()).empty());
 std::cout<<"PASS file-only install/restore:2LUTs, next numeric layers, unowned preservation, metadata baseline guards, active-own reinstallation, manifest-failure rollback, changed-own refusal, verified startup baseline reload. No real game writes.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
