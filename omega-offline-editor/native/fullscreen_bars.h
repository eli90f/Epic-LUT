#pragma once
#include <array>
#include <algorithm>
struct FullscreenBar {const wchar_t*title;int x,y,w,h;};
inline std::array<FullscreenBar,6> defaultFullscreenBars(){return {{{L"ITEMS",20,12,1160,112},{L"VIEW",1200,12,580,112},{L"MATERIAL",20,736,1120,100},{L"COLOR / VALUES",20,850,980,130},{L"HISTORY",1020,850,340,130},{L"PRESETS / FILES",1380,850,400,130}}};}
inline void moveFullscreenBar(FullscreenBar&b,int x,int y){b.x=std::clamp(x,8,1800-b.w-8);b.y=std::clamp(y,8,1000-b.h-8);}
