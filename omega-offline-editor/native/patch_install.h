#pragma once
#include "lut_types.h"
namespace omega {
std::wstring installLutPatches(const std::vector<LutDocument>& documents,const std::wstring& gameData,const std::wstring& appFolder);
std::wstring installLutPatch(const LutDocument& document,const std::wstring& gameData,const std::wstring& appFolder);
std::wstring restoreLutPatch(const std::wstring& gameData,const std::wstring& appFolder);
std::vector<LutDocument> loadInstalledLutPatches(const std::wstring& gameData,const std::wstring& appFolder);
}
