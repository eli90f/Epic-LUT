// DDS envelope/section ordering follows Filediver:
// extractor/texture/extractor.go::ExtractDDSData,
// stingray/archive.go::DataType and stingray/unit/texture/texture.go::DecodeInfo.
// Copyright 2026 Filediver contributors, BSD-3-Clause. Full notice is retained
// with the application. Pixel bytes remain in their original format/order.
#include "native_texture.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace omega { namespace {
using Bytes=std::vector<uint8_t>;
void need(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
size_t add(size_t a,size_t b){need(b<=std::numeric_limits<size_t>::max()-a,"Texture byte count overflow");return a+b;}
size_t mul(size_t a,size_t b){need(!a||b<=std::numeric_limits<size_t>::max()/a,"Texture byte count overflow");return a*b;}
uint32_t u32(const Bytes& b,size_t at){need(at<=b.size()&&b.size()-at>=4,"Truncated native DDS integer");uint32_t value;std::memcpy(&value,b.data()+at,4);return value;}
constexpr uint32_t four(char a,char b,char c,char d){return uint8_t(a)|(uint32_t(uint8_t(b))<<8)|(uint32_t(uint8_t(c))<<16)|(uint32_t(uint8_t(d))<<24);}
uint32_t legacyFormat(const Bytes& d,size_t h){
 const auto flags=u32(d,h+80),code=u32(d,h+84),bits=u32(d,h+88);
 if(flags&4){switch(code){
  case four('D','X','T','1'):return 71;
  case four('D','X','T','3'):return 74;
  case four('D','X','T','5'):return 77;
  case four('A','T','I','1'):case four('B','C','4','U'):return 80;
  case four('B','C','4','S'):return 81;
  case four('A','T','I','2'):case four('B','C','5','U'):return 83;
  case four('B','C','5','S'):return 84;
  case 36:return 11; // D3DFMT_A16B16G16R16
  case 110:return 13; // D3DFMT_Q16W16V16U16
  case 111:return 54;case 112:return 34;case 113:return 10;
  case 114:return 41;case 115:return 16;case 116:return 2;
  default:throw std::runtime_error("Unsupported legacy DDS FourCC");
 }}
 const auto r=u32(d,h+92),g=u32(d,h+96),b=u32(d,h+100),a=u32(d,h+104);
 if((flags&0x40)&&bits==32){
  if(r==0xff&&g==0xff00&&b==0xff0000&&a==0xff000000)return 28;
  if(r==0xff0000&&g==0xff00&&b==0xff&&a==0xff000000)return 87;
  if(r==0xff0000&&g==0xff00&&b==0xff&&a==0)return 88;
  if(r==0x3ff&&g==0xffc00&&b==0x3ff00000&&a==0xc0000000)return 24;
 }
 if((flags&0x40)&&bits==16){
  if(r==0xf800&&g==0x7e0&&b==0x1f&&a==0)return 85;
  if(r==0x7c00&&g==0x3e0&&b==0x1f&&a==0x8000)return 86;
  if(r==0xf00&&g==0xf0&&b==0xf&&a==0xf000)return 115;
 }
 if((flags&0x20000)&&bits==8&&r==0xff&&g==0&&b==0&&a==0)return 61;
 if((flags&0x20000)&&bits==16&&r==0xffff&&g==0&&b==0&&a==0)return 56;
 if((flags&2)&&bits==8&&a==0xff)return 65;
 throw std::runtime_error("Unsupported legacy DDS channel masks");
}
unsigned bitsPerPixel(uint32_t f){
 if(f>=1&&f<=4)return 128;
 if(f>=5&&f<=8)return 96;
 if(f>=9&&f<=22)return 64;
 if(f>=23&&f<=47)return 32;
 if(f>=48&&f<=59)return 16;
 if(f>=60&&f<=65)return 8;
 if(f==67)return 32;
 if(f==85||f==86||f==115)return 16;
 if(f>=87&&f<=93)return 32;
 if(f==100||f==101)return 32;
 if(f==102)return 64;
 if(f>=111&&f<=113)return 8;
 if(f==114)return 16;
 throw std::runtime_error("Unsupported DDS DXGI format or planar layout");
}
}

size_t nativeTextureMipBytes(uint32_t width,uint32_t height,uint32_t format){
 need(width&&height,"Zero-sized DDS mip");
 if((format>=70&&format<=84)||(format>=94&&format<=99)){
  const size_t block=(format>=70&&format<=72)||(format>=79&&format<=81)?8:16;
  return mul(mul((size_t(width)+3)/4,(size_t(height)+3)/4),block);
 }
 if(format==66)return mul((size_t(width)+7)/8,height);
 if(format==68||format==69||format==107)return mul(mul((size_t(width)+1)/2,4),height);
 if(format==108||format==109)return mul(mul((size_t(width)+1)/2,8),height);
 return mul((mul(width,bitsPerPixel(format))+7)/8,height);
}

NativeTexture decodeNativeTexture(const Bytes& main,const Bytes& stream,const Bytes& gpu){
 // The first 0xC0 bytes belong to Stingray, not DDS. The streamable-section
 // descriptors are bookkeeping; Filediver consumes them but never reorders data.
 constexpr size_t prefix=0xc0;
 need(main.size()>=prefix,"Truncated native texture envelope");
 const size_t total=add(add(main.size(),stream.size()),gpu.size());
 Bytes d;need(total-prefix<=d.max_size(),"Native texture exceeds addressable vector size");d.reserve(total-prefix);
 d.insert(d.end(),main.begin()+prefix,main.end());d.insert(d.end(),stream.begin(),stream.end());d.insert(d.end(),gpu.begin(),gpu.end());
 need(d.size()>=128,"Truncated DDS header");need(u32(d,0)==four('D','D','S',' '),"Native texture has no DDS magic at offset 0xC0");
 need(u32(d,4)==124&&u32(d,76)==32,"Invalid DDS header size");
 NativeTexture result;result.width=u32(d,16);result.height=u32(d,12);result.mips=std::max(1u,u32(d,28));
 need(result.width&&result.height,"Invalid native DDS dimensions");
 uint32_t maximumMips=1;for(uint32_t n=std::max(result.width,result.height);n>1;n>>=1)++maximumMips;
 need(result.mips<=maximumMips,"DDS mip count exceeds dimensions");
 size_t pixelStart=128;
 const uint32_t caps2=u32(d,112),depth=u32(d,24),pixelFlags=u32(d,80);
 need(!(caps2&0x200000)&&depth<=1,"Volume DDS needs a 3D texture representation");
 need(!(caps2&0x200),"Cubemap DDS needs a cubemap texture representation");
 if((pixelFlags&4)&&u32(d,84)==four('D','X','1','0')){
  need(d.size()>=148,"Truncated DX10 DDS header");result.format=u32(d,128);
  need(u32(d,132)==3,"Preview requires a two-dimensional DDS resource");
  need(!(u32(d,136)&4),"Cubemap DDS needs a cubemap texture representation");
  result.layers=u32(d,140);need(result.layers>0,"DDS array has no layers");pixelStart=148;
 }else result.format=legacyFormat(d,0);
 size_t layerBytes=0;
 for(uint32_t mip=0;mip<result.mips;++mip)layerBytes=add(layerBytes,nativeTextureMipBytes(std::max(1u,result.width>>mip),std::max(1u,result.height>>mip),result.format));
 const size_t expected=mul(layerBytes,result.layers);
 need(d.size()-pixelStart>=expected,"Native DDS is missing layer or mip pixel data");
 // Keep trailing bytes, if any, rather than silently dropping source data. GPU
 // upload uses the validated mip ranges; callers can retain the original tail.
 result.pixels.assign(d.begin()+pixelStart,d.end());
 return result;
}
}
