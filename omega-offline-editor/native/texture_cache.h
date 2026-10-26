#pragma once
#include "native_texture.h"
#include <memory>
namespace omega {
std::shared_ptr<NativeTexture> readTextureCache(const std::wstring&key);
void writeTextureCache(const std::wstring&key,const NativeTexture&texture);
}
