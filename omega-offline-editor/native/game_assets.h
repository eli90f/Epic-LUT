#pragma once
#include "lut_types.h"
#include "item_preview.h"
#include <functional>
#include <map>
namespace omega {
struct GameAssetScanStats {bool baseArchiveAvailable=false;size_t archives=0,textureResources=0,duplicateResources=0,supportedLuts=0,namedUnits=0,materialLinks=0,namedLuts=0,namedWeaponLuts=0,namedVehicleLuts=0,namedArmorLuts=0;std::map<std::string,size_t> skipped;};
// File-only scanning. False from progress cancels and returns no partial index.
std::vector<TextureRecord> scanGameLuts(const std::wstring& dataFolder,
 std::function<bool(size_t,size_t,const std::wstring&)> progress={});
LutDocument loadGameLut(const TextureRecord& record);
GameAssetScanStats gameAssetScanStats();
// Release file handles before an owned-patch install/restore transaction.
void clearGameAssetCache();
std::vector<std::wstring> gameItemPreviewOwners(const std::string& lut);
ItemPreviewModel loadGameItemPreview(const std::wstring& folder,const std::string& lut,size_t ownerIndex=0,bool singlePiece=false);
}
