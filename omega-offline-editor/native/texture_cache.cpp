#define NOMINMAX
#include "texture_cache.h"
#include <algorithm>
#include <stdexcept>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
namespace omega {namespace {
namespace fs=std::filesystem;uint64_t hash(const void*p,size_t n){auto b=(const unsigned char*)p;uint64_t h=1469598103934665603ULL;for(size_t i=0;i<n;i++){h^=b[i];h*=1099511628211ULL;}return h;}
fs::path path(const std::wstring&key){wchar_t exe[32768];auto n=GetModuleFileNameW(nullptr,exe,32768);if(!n||n>=32768)throw std::runtime_error("Cache executable path unavailable");std::wostringstream s;s<<std::hex<<std::setw(16)<<std::setfill(L'0')<<hash(key.data(),key.size()*2);return fs::path(exe).parent_path()/L"lut-editor-cache"/(s.str()+L".texturecache");}
struct Header{uint64_t magic=0x315845544147454dULL,key=0,checksum=0,bytes=0;uint32_t width=0,height=0,layers=0,mips=0,format=0,reserved=0;};
uint64_t expected(const NativeTexture&t){uint64_t total=0;for(uint32_t i=0;i<t.mips;i++)total+=nativeTextureMipBytes(std::max(1u,t.width>>i),std::max(1u,t.height>>i),t.format);return total*t.layers;}
}
std::shared_ptr<NativeTexture> readTextureCache(const std::wstring&key){try{auto file=path(key);if(!fs::exists(file))return {};std::ifstream f(file,std::ios::binary);Header h;f.read((char*)&h,sizeof(h));if(!f||h.magic!=Header{}.magic||h.key!=hash(key.data(),key.size()*2)||h.width==0||h.height==0||h.width>32768||h.height>32768||h.layers==0||h.layers>2048||h.mips==0||h.mips>16||h.bytes>4ULL*1024*1024*1024||h.bytes!=fs::file_size(file)-sizeof(h))return {};auto t=std::make_shared<NativeTexture>();t->width=h.width;t->height=h.height;t->layers=h.layers;t->mips=h.mips;t->format=h.format;if(expected(*t)>h.bytes)return {};t->pixels.resize(size_t(h.bytes));f.read((char*)t->pixels.data(),t->pixels.size());if(!f||hash(t->pixels.data(),t->pixels.size())!=h.checksum)return {};return t;}catch(...){return {};}}
void writeTextureCache(const std::wstring&key,const NativeTexture&t){try{auto file=path(key);fs::create_directories(file.parent_path());Header h;h.key=hash(key.data(),key.size()*2);h.checksum=hash(t.pixels.data(),t.pixels.size());h.bytes=t.pixels.size();h.width=t.width;h.height=t.height;h.layers=t.layers;h.mips=t.mips;h.format=t.format;auto tmp=file;tmp+=L".tmp";{std::ofstream f(tmp,std::ios::binary|std::ios::trunc);f.write((char*)&h,sizeof(h));f.write((char*)t.pixels.data(),t.pixels.size());if(!f)return;}MoveFileExW(tmp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING);}catch(...){}}
}
