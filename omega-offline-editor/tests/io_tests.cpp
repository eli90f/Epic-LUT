#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../native/lut_io.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <sstream>
using namespace omega;
using Bytes=std::vector<uint8_t>;
static void put(Bytes& b,size_t at,uint32_t v){for(unsigned i=0;i<4;++i)b.at(at+i)=uint8_t(v>>(8*i));}
static uint32_t get(const Bytes& b,size_t at){return uint32_t(b.at(at))|uint32_t(b.at(at+1))<<8|uint32_t(b.at(at+2))<<16|uint32_t(b.at(at+3))<<24;}
static Bytes read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(f),{});}
static void write(const std::filesystem::path& p,const Bytes& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));assert(bool(f));}
template<class F>static void rejected(F&& call){bool failed=false;try{call();}catch(const std::runtime_error&){failed=true;}assert(failed);}
static LutDocument sample(uint32_t width,uint32_t height){LutDocument d;d.width=width;d.height=height;for(uint32_t i=0;i<width*height;++i)d.pixels.push_back({float(i)*.1234567f,-float(i)*.25f,float(i%9)*3.125f,float(i%5)*.375f});d.original=d.pixels;return d;}
int wmain(int argc,wchar_t** argv){
 const std::filesystem::path folder=argc>1?argv[1]:L"build/io-tests";std::filesystem::create_directories(folder);
 auto cape=sample(16,8);assert(decodeDDS(encodeDDS(cape)).pixels==cape.pixels);savePreset(cape,(folder/L"cape.omegalut").wstring());assert(loadPreset((folder/L"cape.omegalut").wstring()).pixels==cape.pixels);
 auto d=sample(23,4);d.label=L"OMEGA \u03a9 material";d.sourcePath=L"D:\\original\\source \u03a9.dds";d.resourceId="0123456789abcdef";d.archiveId="0f1e2d3c4b5a6978";
 const auto base=encodeDDS(d);assert(base.size()==148+23*4*16);assert(get(base,128)==2&&get(base,132)==3&&get(base,140)==1);
 auto decoded=decodeDDS(base);assert(decoded.pixels==d.pixels&&decoded.original==d.pixels);assert(decoded.width==23&&decoded.height==4);
 const auto ddsPath=folder/L"sample.dds";saveDDS(d,ddsPath.wstring());assert(loadDDS(ddsPath.wstring()).pixels==d.pixels);
 auto originalDDS=encodeDDS(sample(23,3));d.nativeHeader.resize(192,0x5a);d.nativeHeader[0]=0xef;d.nativeHeader[1]=0xbe;d.nativeHeader[2]=0xad;d.nativeHeader[3]=0xde;d.nativeHeader.insert(d.nativeHeader.end(),originalDDS.begin(),originalDDS.begin()+148);d.original=sample(23,3).pixels;
 const auto presetPath=folder/L"sample.omegalut";savePreset(d,presetPath.wstring());auto restored=loadPreset(presetPath.wstring());assert(restored.pixels==d.pixels&&restored.original==d.original&&restored.nativeHeader==d.nativeHeader&&restored.resourceId==d.resourceId&&restored.archiveId==d.archiveId&&restored.label==d.label&&restored.sourcePath==d.sourcePath);
 auto halfDDS=encodeDDS(sample(3,1));halfDDS.resize(148);put(halfDDS,128,10);
 const uint16_t halves[]={0x3c00,0xc000,0x3800,0x4000,0x0001,0x8001,0x7bff,0x0000,0x3555,0x8000,0x0400,0xbc00};
 for(uint16_t h:halves){halfDDS.push_back(uint8_t(h));halfDDS.push_back(uint8_t(h>>8));}
 auto halfDoc=decodeDDS(halfDDS);assert(halfDoc.pixels[0]==Pixel({1,-2,.5f,2}));assert(halfDoc.pixels[1][0]==std::ldexp(1.0f,-24)&&halfDoc.pixels[1][1]==-std::ldexp(1.0f,-24)&&halfDoc.pixels[1][2]==65504);assert(std::signbit(halfDoc.pixels[2][1]));
 auto legacy=base;legacy.erase(legacy.begin()+128,legacy.begin()+148);put(legacy,84,116);assert(decodeDDS(legacy).pixels==d.pixels);
 auto legacyHalf=halfDDS;legacyHalf.erase(legacyHalf.begin()+128,legacyHalf.begin()+148);put(legacyHalf,84,113);assert(decodeDDS(legacyHalf).pixels==halfDoc.pixels);
 auto mip=encodeDDS(sample(3,1));put(mip,28,2);mip.resize(mip.size()+16,0);assert(decodeDDS(mip).width==3);auto badMip=mip;put(badMip,badMip.size()-4,0x7f800000);rejected([&]{decodeDDS(badMip);});
 auto bad=base;bad.pop_back();rejected([&]{decodeDDS(bad);});bad=base;bad.push_back(0);rejected([&]{decodeDDS(bad);});bad=base;put(bad,16,22);rejected([&]{decodeDDS(bad);});bad=base;put(bad,28,10);rejected([&]{decodeDDS(bad);});bad=base;put(bad,140,2);rejected([&]{decodeDDS(bad);});bad=base;put(bad,112,0x200);rejected([&]{decodeDDS(bad);});bad=base;put(bad,24,2);rejected([&]{decodeDDS(bad);});bad=halfDDS;bad[148]=0;bad[149]=0x7c;rejected([&]{decodeDDS(bad);});
 auto before=read(ddsPath);auto invalid=d;invalid.pixels[0][3]=std::numeric_limits<float>::infinity();rejected([&]{saveDDS(invalid,ddsPath.wstring());});assert(read(ddsPath)==before);
 assert(SetFileAttributesW(ddsPath.c_str(),FILE_ATTRIBUTE_READONLY));rejected([&]{saveDDS(d,ddsPath.wstring());});assert(read(ddsPath)==before);assert(SetFileAttributesW(ddsPath.c_str(),FILE_ATTRIBUTE_NORMAL));
 auto presetBytes=read(presetPath);presetBytes[presetBytes.size()/2]^=1;write(folder/L"corrupt.omegalut",presetBytes);rejected([&]{loadPreset((folder/L"corrupt.omegalut").wstring());});
 auto invalidPreset=d;invalidPreset.resourceId="not an ID";auto beforePreset=read(presetPath);rejected([&]{savePreset(invalidPreset,presetPath.wstring());});assert(read(presetPath)==beforePreset);
 auto patchFolder=folder/L"patch";if(std::filesystem::exists(patchFolder)){for(auto& e:std::filesystem::directory_iterator(patchFolder))std::filesystem::remove(e.path());std::filesystem::remove(patchFolder);}exportPatch(d,patchFolder.wstring());
 auto main=read(patchFolder/L"9ba626afa44a3aa3.patch_0"),gpu=read(patchFolder/L"9ba626afa44a3aa3.patch_0.gpu_resources"),stream=read(patchFolder/L"9ba626afa44a3aa3.patch_0.stream");
 assert(main.size()==524&&gpu.size()==d.pixels.size()*16&&stream.empty());assert(get(main,0)==0xf0000011&&get(main,8)==1&&get(main,120)==184&&get(main,160)==340&&get(main,168)==gpu.size()&&get(main,172)==16&&get(main,176)==64&&get(main,180)==1);assert(main[104]==0xef&&main[111]==0x01);assert(get(main,184)==0xdeadbeef&&get(main,188)==0&&get(main,192)==0xffffffffu);
 for(size_t i=196;i<184+192;++i)assert(main[i]==0);assert(std::equal(base.begin(),base.begin()+148,main.begin()+184+192));assert(std::equal(base.begin()+148,base.end(),gpu.begin()));
 auto oldPatch=main;rejected([&]{exportPatch(d,patchFolder.wstring());});assert(read(patchFolder/L"9ba626afa44a3aa3.patch_0")==oldPatch);
 invalid=d;invalid.nativeHeader.clear();rejected([&]{exportPatch(invalid,(folder/L"invalid-patch").wstring());});assert(!std::filesystem::exists(folder/L"invalid-patch"));invalid=d;put(invalid.nativeHeader,208,3);rejected([&]{exportPatch(invalid,(folder/L"invalid-patch").wstring());});
 // Artifacts consumed by the original Lua writer parity check.
 write(folder/L"native-source.bin",d.nativeHeader);write(folder/L"replacement.dds",base);
 // Original Epic-LUT v2 .dbflut record grammar, including multiple LUT targets.
 auto material=sample(23,2),pattern=sample(3,1);material.resourceId="a123456789abcdef";pattern.resourceId="b123456789abcdef";
 std::ostringstream epic;epic<<"EPIC-LUT\t2\ntarget\tarmor\tdeadbeef\nbody\t2\nenabled\t1\neffects\t0\n"<<std::setprecision(9);
 for(const auto* doc:{&material,&pattern}){
  epic<<"lut\t"<<doc->resourceId<<'\t'<<doc->width<<'\t'<<doc->height<<'\n';
  for(uint32_t r=1;r<=doc->height;++r){epic<<"row\t"<<doc->resourceId<<'\t'<<r<<"\t"<<(r==1?1:0)<<"\t#3F7E81\n";for(uint32_t c=1;c<=doc->width;++c){epic<<"float\t"<<doc->resourceId<<'\t'<<r<<'\t'<<c;for(float v:doc->at(c-1,r-1))epic<<'\t'<<v;epic<<'\n';}}
 }
 const auto text=epic.str();write(folder/L"source-v2.dbflut",Bytes(text.begin(),text.end()));auto imported=loadEpicPreset((folder/L"source-v2.dbflut").wstring());assert(imported.targetKind=="armor"&&imported.targetId=="deadbeef"&&imported.body=="2"&&imported.enabled&&!imported.preserveEffects&&imported.documents.size()==2&&imported.rowSettings.size()==3);assert(imported.documents[0].pixels==material.pixels&&imported.documents[1].pixels==pattern.pixels);assert(imported.documents[0].resourceId==material.resourceId&&imported.documents[1].resourceId==pattern.resourceId&&imported.documents[0].nativeHeader.empty()&&imported.documents[0].archiveId.empty());assert(imported.rowSettings[1].row==2&&!imported.rowSettings[1].enabled&&imported.rowSettings[1].rgbHex=="#3F7E81");
 auto malformed=text.substr(0,text.find_last_of('\n',text.size()-2)+1);write(folder/L"missing-cell.dbflut",Bytes(malformed.begin(),malformed.end()));rejected([&]{loadEpicPreset((folder/L"missing-cell.dbflut").wstring());});
 auto extra=text+"float\t"+pattern.resourceId+"\t1\t1\t0\t0\t0\t0\n";write(folder/L"duplicate.dbflut",Bytes(extra.begin(),extra.end()));rejected([&]{loadEpicPreset((folder/L"duplicate.dbflut").wstring());});
 const std::string legacyEpic="DBF-ARMOR-LUT\t1\narmor\tdeadbeef\nbody\t2\nenabled\t1\n";write(folder/L"rgb-only.dbflut",Bytes(legacyEpic.begin(),legacyEpic.end()));rejected([&]{loadEpicPreset((folder/L"rgb-only.dbflut").wstring());});
 std::cout<<"PASS DDS32F/16F/legacy/mips, all RGBA channels, exact presets, malformed data, atomic failure preservation, native patch triplet/provenance guards\n";
 std::cout<<"PASS original Epic-LUT v2 multi-LUT import: RGBAfloat32, exact target IDs, body/effects/row flags; missing/duplicate cells and RGB-only v1 rejected\n";
 return 0;
}
