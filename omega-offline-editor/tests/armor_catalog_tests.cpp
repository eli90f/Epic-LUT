#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../native/game_assets.h"
#include "../native/gear_catalog.h"
#include "../native/lut_io.h"
#include <iostream>
#include <set>
#include <cassert>
int wmain(){try{
 const std::wstring game=L"D:/SteamLibrary/steamapps/common/Helldivers 2/data";
 auto kits=omega::readGearCatalog(game);std::wcout<<L"Kit pieces "<<kits.size()<<std::endl;
 auto records=omega::scanGameLuts(game);std::set<std::wstring> armor,capes,helmets;int cape16=0;bool armorTest=false,capeTest=false;
 std::ofstream names("build/armor-preview/verified-items.tsv");
 for(auto& r:records){auto owners=omega::gameItemPreviewOwners(r.resourceId);for(auto& n:owners){auto kit=n.substr(0,n.find(L" / "));if(n.rfind(L"[Armor]",0)==0)armor.insert(kit);if(n.rfind(L"[Cape]",0)==0)capes.insert(kit);if(n.rfind(L"[Helmet]",0)==0)helmets.insert(kit);names<<r.resourceId<<'\t'<<std::string(n.begin(),n.end())<<'\n';}
  bool a=false,c=false;for(auto& n:owners){a|=n.rfind(L"[Armor]",0)==0;c|=n.rfind(L"[Cape]",0)==0;}if(c&&r.width==16)++cape16;
  if((a&&!armorTest)||(c&&!capeTest&&r.width==16)){auto d=omega::loadGameLut(r);d.pixels[0][0]+=.125f;auto folder=std::filesystem::absolute(a?L"build/armor-preview/body-patch":L"build/armor-preview/cape-patch");std::filesystem::create_directories(folder);omega::savePreset(d,(folder/L"test.omegalut").wstring());assert(omega::loadPreset((folder/L"test.omegalut").wstring()).pixels==d.pixels);auto patch=folder/std::to_wstring(GetTickCount64());omega::exportPatch(d,patch.wstring());auto check=omega::scanGameLuts(patch.wstring());assert(check.size()==1);assert(omega::loadGameLut(check[0]).pixels==d.pixels);if(a)armorTest=true;else capeTest=true;omega::clearGameAssetCache();break;}
 }
 // Separate checks avoid invalidating scanner preview state while iterating.
 records=omega::scanGameLuts(game);armor.clear();capes.clear();helmets.clear();cape16=0;
 for(auto& r:records){auto owners=omega::gameItemPreviewOwners(r.resourceId);bool c=false;for(auto& n:owners){auto kit=n.substr(0,n.find(L" / "));if(n.rfind(L"[Armor]",0)==0)armor.insert(kit);if(n.rfind(L"[Cape]",0)==0){capes.insert(kit);c=true;}if(n.rfind(L"[Helmet]",0)==0)helmets.insert(kit);names<<r.resourceId<<'\t'<<std::string(n.begin(),n.end())<<'\n';}if(c&&r.width==16)++cape16;}
 std::set<std::wstring> expected;for(auto& k:kits)if(k.name.rfind(L"[Armor]",0)==0)expected.insert(k.name.substr(0,k.name.find(L" / ")));for(auto& name:expected)if(!armor.count(name))std::wcerr<<L"MISSING BODY "<<name<<std::endl;assert(std::includes(armor.begin(),armor.end(),expected.begin(),expected.end()));
 std::wcout<<L"Verified kits: armor="<<armor.size()<<L" capes="<<capes.size()<<L" helmets="<<helmets.size()<<L" cape16="<<cape16<<std::endl;assert(armor.size()>=100&&capes.size()>=100);
 for(auto& r:records)if(r.width==16){auto owners=omega::gameItemPreviewOwners(r.resourceId);if(std::none_of(owners.begin(),owners.end(),[](auto& n){return n.rfind(L"[Cape]",0)==0;}))continue;auto d=omega::loadGameLut(r);d.pixels[0][0]+=.125f;auto folder=std::filesystem::absolute(L"build/armor-preview/cape-patch");std::filesystem::create_directories(folder);omega::savePreset(d,(folder/L"test.omegalut").wstring());assert(omega::loadPreset((folder/L"test.omegalut").wstring()).pixels==d.pixels);auto patch=folder/std::to_wstring(GetTickCount64());omega::exportPatch(d,patch.wstring());auto check=omega::scanGameLuts(patch.wstring());assert(check.size()==1&&omega::loadGameLut(check[0]).pixels==d.pixels);capeTest=true;break;}
 assert(armorTest&&capeTest);std::cout<<"PASS current kit discovery and body/cape preset + patch round trips; no game writes\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
