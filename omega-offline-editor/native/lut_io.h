#pragma once
#include "lut_types.h"
namespace omega {
// Ports Epic LUT R5.9.3 src/core/dds.lua and src/presets/patch_export.lua.
// Inputs are data only. All failures throw std::runtime_error.
LutDocument decodeDDS(const std::vector<uint8_t>& bytes,const std::wstring& label=L"");
std::vector<uint8_t> encodeDDS(const LutDocument& document);
LutDocument loadDDS(const std::wstring& path);
void saveDDS(const LutDocument& document,const std::wstring& path);
void savePreset(const LutDocument& document,const std::wstring& path);
LutDocument loadPreset(const std::wstring& path);
// Publishes the complete original-base patch triplet in a new destination folder.
void exportPatch(const LutDocument& document,const std::wstring& folder);
struct EpicRowSetting {std::string resourceId;uint32_t row=0;bool enabled=false;std::string rgbHex;};
struct EpicPreset {
 std::vector<LutDocument> documents;
 std::string targetKind,targetId,body;
 bool enabled=false,preserveEffects=true;
 std::vector<EpicRowSetting> rowSettings;
};
// Original Epic-LUT v2 .dbflut format. Every target, row flag and RGBA cell is
// retained. Imported documents lack archive/header provenance until rebound to
// the matching current-game resource. RGB-only v1 is explicitly unsupported.
EpicPreset loadEpicPreset(const std::wstring& path);
}
