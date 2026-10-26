#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstddef>
#include "lut_types.h"

namespace omega {
// Zero-based columns; help states the recovered per-RGBA source meaning.
const wchar_t* lutColumnName(size_t column);
const wchar_t* lutColumnHelp(size_t column);

// Zero-based selected material row. This is an explicitly approximate material
// sample, not recovered game geometry or the authoritative HD2 shader.
// Original/HDR/negative data is never changed; display tonemapping is local.
void renderMaterialPreview(HDC dc,const RECT& rect,const LutDocument& doc,
                           size_t row,float rotation,bool checker,
                           bool showOriginal);
}
