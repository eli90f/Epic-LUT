#pragma once
#include "item_preview.h"
namespace omega {
bool renderGpuItemPreview(HDC,const RECT&,const LutDocument&,size_t,const ItemPreviewModel&,float,float,float,bool,bool,bool,int,size_t column=0);
bool gpuPreviewHasFrame();
int pickGpuPreviewPart(int x,int y);
int pickGpuPreviewRow(int x,int y);
std::wstring gpuPreviewStatus();
}
