#include "../native/native_texture.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
using B=std::vector<uint8_t>;
void set(B&b,size_t at,uint32_t v){std::memcpy(b.data()+at,&v,4);}
B header(uint32_t w,uint32_t h,uint32_t mip,uint32_t layers,uint32_t format){B b(192+148,0);size_t p=192;set(b,p,0x20534444);set(b,p+4,124);set(b,p+8,0x21007);set(b,p+12,h);set(b,p+16,w);set(b,p+28,mip);set(b,p+76,32);set(b,p+80,4);set(b,p+84,0x30315844);set(b,p+108,0x401008);set(b,p+128,format);set(b,p+132,3);set(b,p+140,layers);return b;}
template<class F>void rejects(F fn){bool rejected=false;try{fn();}catch(const std::runtime_error&){rejected=true;}assert(rejected);}
int main(){
 assert(omega::nativeTextureMipBytes(8,8,98)==64);assert(omega::nativeTextureMipBytes(1,1,98)==16);assert(omega::nativeTextureMipBytes(5,3,71)==16);assert(omega::nativeTextureMipBytes(23,8,2)==2944);
 auto main=header(8,8,4,2,98);B pixels(224);for(size_t i=0;i<pixels.size();++i)pixels[i]=uint8_t(i);main.insert(main.end(),pixels.begin(),pixels.begin()+27);B stream(pixels.begin()+27,pixels.begin()+48),gpu(pixels.begin()+48,pixels.end());auto out=omega::decodeNativeTexture(main,stream,gpu);assert(out.width==8&&out.height==8&&out.layers==2&&out.mips==4&&out.format==98&&out.pixels==pixels);
 auto trailing=gpu;trailing.insert(trailing.end(),{5,6,7});out=omega::decodeNativeTexture(main,stream,trailing);assert(out.pixels.size()==227&&out.pixels[224]==5&&out.pixels[226]==7);
 auto shortGpu=gpu;shortGpu.pop_back();rejects([&]{omega::decodeNativeTexture(main,stream,shortGpu);});auto bad=main;set(bad,192+140,0);rejects([&]{omega::decodeNativeTexture(bad,stream,gpu);});bad=main;set(bad,192+28,5);rejects([&]{omega::decodeNativeTexture(bad,stream,gpu);});bad=main;set(bad,192+136,4);rejects([&]{omega::decodeNativeTexture(bad,stream,gpu);});rejects([&]{omega::decodeNativeTexture({}, {}, {});});
 main=header(4,4,1,1,71);main.resize(192+128);set(main,192+84,0x31545844);gpu={1,2,3,4,5,6,7,8};out=omega::decodeNativeTexture(main,{},gpu);assert(out.format==71&&out.layers==1&&out.pixels==gpu);
 std::cout<<"PASS native texture: main/stream/GPU exact order, BC7 arrays+mips, BC1 legacy, source tail retained, malformed data rejected\n";
}
