#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <set>
#include <map>
#include <memory>
#include "native_texture.h"
#include "lut_types.h"
#include "source_pose.h"
namespace omega {
struct ItemPreviewVertex {float x=0,y=0,z=0,nx=0,ny=0,nz=1,u=0,v=0,u1=0,v1=0,u2=0,v2=0,tx=1,ty=0,tz=0,tw=1,bx=0,by=1,bz=0;};
struct ItemPreviewTriangle {uint32_t a=0,b=0,c=0;bool affected=false;uint32_t part=0;};
struct ItemPreviewPart {std::wstring name;std::string lutResourceId;uint32_t materialKey=0;uint64_t baseMaterial=0;bool transparent=false;std::map<std::string,std::shared_ptr<const NativeTexture>> textures;std::map<std::string,std::vector<float>> parameters;};
struct ItemPreviewModel {
 std::wstring name,notice;
 std::vector<ItemPreviewPart> parts;
 std::vector<ItemPreviewVertex> vertices;
 std::vector<ItemPreviewTriangle> triangles;
 uint64_t identity=0;
 size_t sourceMeshes=0;
 bool exactGameShader=false;
};
std::string previewSourceName(uint32_t hash);
using PreviewRead=std::function<std::vector<uint8_t>(uint64_t,size_t)>;
ItemPreviewModel decodeItemPreview(const std::vector<uint8_t>& unit,const std::vector<uint8_t>& geometry,
 uint64_t unitId,const PreviewRead& gpu,const std::set<uint32_t>& materialKeys,const SourcePose& pose={});
void renderItemPreview(HDC,const RECT&,const LutDocument&,size_t row,const ItemPreviewModel&,
 float yaw,float pitch,float zoom,bool checker,bool showOriginal,bool shading,int selectedPart=-1,size_t column=0);
int pickItemPreview(int screenX,int screenY);
int pickItemPreviewRow(int screenX,int screenY);
std::wstring itemPreviewRenderStatus();
}
