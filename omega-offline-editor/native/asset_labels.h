#pragma once
#include <string>
#include <vector>
namespace omega {
struct GameAssetOwner {std::string resourceId;std::wstring owner,category;};
std::wstring gameUnitAliases(const std::string& resourceId);
std::wstring gameUnitPath(const std::string& resourceId);
std::wstring gameUnitCategory(const std::wstring& path);
void setGameAssetOwners(const std::vector<GameAssetOwner>& owners);
// A texture resource is not its owning unit/archive. Do not infer owners from IDs.
std::wstring gameAssetLabel(const std::string& resourceId);
std::wstring gameAssetCategory(const std::string& resourceId);
}
