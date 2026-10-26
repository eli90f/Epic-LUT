#define NOMINMAX
#include "../native/steam_location.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>
using namespace omega;namespace fs=std::filesystem;
void write(const fs::path&p,const std::string&s){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary)<<s;}
std::string quoted(const fs::path&p){auto s=p.u8string();std::string out="\"";for(char c:s){if(c=='\\'||c=='"')out+='\\';out+=c;}return out+'"';}
int main(){
 auto paths=steamLibrariesFromVdf(R"vdf("libraryfolders" { "0" { "path" "C:\\Program Files (x86)\\Steam" "apps" { "553850" "1" } } "1" { "path" "D:\\Games\\Steam" } })vdf");assert(paths.size()==2&&paths[0].wstring()==L"C:\\Program Files (x86)\\Steam"&&paths[1].wstring()==L"D:\\Games\\Steam");
 auto old=steamLibrariesFromVdf("// old format\n\"LibraryFolders\" { \"1\" \"E:\\\\Steam Library\" }");assert(old.size()==1&&old[0].wstring()==L"E:\\Steam Library");
 assert(hd2InstallDirFromManifest(R"("AppState" {"appid" "553850" "installdir" "HD2 custom folder"})")=="HD2 custom folder");assert(hd2InstallDirFromManifest(R"("AppState" {"appid" "42" "installdir" "Helldivers 2"})").empty());assert(hd2InstallDirFromManifest(R"("AppState" {"appid" "553850" "installdir" "../other"})").empty());
 auto root=fs::absolute(fs::path("build/steam-location-tests")/std::to_string(GetCurrentProcessId()));auto steam=root/L"Steam",stale=root/L"missing library",library=root/fs::u8path("Library with spaces ü");auto game=library/L"steamapps"/L"common"/fs::u8path("HD2 relocated ü");write(game/L"bin"/L"helldivers2.exe","");write(game/L"data"/L"bundles.nxa","");write(library/L"steamapps"/L"appmanifest_553850.acf","\"AppState\" {\"appid\" \"553850\" \"installdir\" \"HD2 relocated ü\"}");write(steam/L"steamapps"/L"libraryfolders.vdf","\"libraryfolders\" {\"0\" {\"path\" "+quoted(stale)+"} \"1\" {\"path\" "+quoted(library)+"}}");
 auto detected=findHd2InSteamRoots({steam});assert(detected.dataFolder==fs::weakly_canonical(game/L"data").wstring());assert(validateHd2Folder(game.wstring())==detected.dataFolder);assert(validateHd2Folder((game/L"data").wstring())==detected.dataFolder);assert(validateHd2Folder(root.wstring()).empty());assert(findHd2InSteamRoots({root/L"not Steam"}).dataFolder.empty());
 auto live=findInstalledHd2();std::wcout<<L"PASS C/D/E drive parsing, legacy/current VDF, app ID, relocated install, Unicode/spaces, stale library, root/data picker validation.\nDetected installed HD2: "<<live.dataFolder<<L"\nSource: "<<live.source<<L"\n";
}
