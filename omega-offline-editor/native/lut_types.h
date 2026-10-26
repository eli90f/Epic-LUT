#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace omega {
using Pixel=std::array<float,4>;
struct LutDocument {
 uint32_t width=23,height=8;
 std::vector<Pixel> pixels,original;
 std::string resourceId,archiveId;
 std::wstring label,sourcePath;
 std::vector<uint8_t> nativeHeader;
 Pixel& at(size_t x,size_t y){return pixels.at(y*width+x);}
 const Pixel& at(size_t x,size_t y)const{return pixels.at(y*width+x);}
 bool changed()const{return pixels!=original;}
};
struct TextureRecord {
 std::string resourceId,archiveId;
 std::wstring archivePath,name;
 std::wstring category=L"Unknown";
 uint64_t mainOffset=0,mainSize=0,gpuOffset=0,gpuSize=0,streamOffset=0,streamSize=0;
 uint32_t width=0,height=0,format=0;
 std::vector<uint8_t> nativeHeader;
};
}
