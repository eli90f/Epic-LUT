#pragma once
#include <vector>
#include <string>
#include <cstdint>
namespace omega {
struct NativeTexture {uint32_t width=0,height=0,layers=1,mips=1,format=0;std::vector<uint8_t> pixels;std::string resourceId;uint64_t identity=0;};
NativeTexture decodeNativeTexture(const std::vector<uint8_t>&main,const std::vector<uint8_t>&stream,const std::vector<uint8_t>&gpu);
size_t nativeTextureMipBytes(uint32_t width,uint32_t height,uint32_t format);
}
