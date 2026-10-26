#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>
namespace omega {
struct Hd2Location {std::wstring dataFolder,source;};
std::vector<std::filesystem::path> steamLibrariesFromVdf(const std::string&text);
std::string hd2InstallDirFromManifest(const std::string&text);
std::wstring validateHd2Folder(const std::wstring&folder);
Hd2Location findHd2InSteamRoots(const std::vector<std::filesystem::path>&roots);
Hd2Location findInstalledHd2();
std::wstring chooseHd2Folder(HWND owner,const std::wstring&initial={});
}
