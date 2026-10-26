#include "asset_labels.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <mutex>
namespace omega {
namespace {
struct UnitName {const char* id;const wchar_t* path;};
const UnitName unitNames[]={
#include "asset_unit_names.inc"
};
std::mutex lock;
std::map<std::string,std::set<std::wstring>> labels,categories;
std::string exactId(std::string value){
 if(value.size()==18&&value[0]=='0'&&(value[1]=='x'||value[1]=='X'))value.erase(0,2);
 if(value.size()!=16)return {};
 for(char& c:value){if(!std::isxdigit(static_cast<unsigned char>(c)))return {};c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));}
 return value;
}
}
std::wstring gameUnitAliases(const std::string& resourceId){static const UnitName aliases[]={
#include "unit_aliases.inc"
};static const auto values=[](){std::map<std::string,std::wstring> m;for(auto&a:aliases)m.emplace(a.id,a.path);return m;}();auto it=values.find(exactId(resourceId));return it==values.end()?L"":it->second;}
std::wstring gameUnitPath(const std::string& resourceId){
 static const auto names=[](){std::map<std::string,std::wstring> out;for(const auto& n:unitNames)out.emplace(n.id,n.path);return out;}();
 auto it=names.find(exactId(resourceId));return it==names.end()?L"":it->second;
}
std::wstring gameUnitCategory(const std::wstring& path){
 if(path.rfind(L"[Armor]",0)==0||path.rfind(L"[Helmet]",0)==0||path.rfind(L"[Cape]",0)==0)return L"Armor";
 if(path.find(L"/vehicles/")!=std::wstring::npos)return L"Vehicles";
 if(path.find(L"/equipment/")!=std::wstring::npos||path.find(L"/weapons/")!=std::wstring::npos)return L"Weapons";
 if(path.find(L"/capes/")!=std::wstring::npos)return L"Armor";
 if(path.find(L"/cha_avatar/")!=std::wstring::npos)return L"Armor";
 return L"Unknown";
}
void setGameAssetOwners(const std::vector<GameAssetOwner>& owners){std::lock_guard<std::mutex> guard(lock);labels.clear();categories.clear();for(const auto& owner:owners){labels[owner.resourceId].insert(owner.owner);categories[owner.resourceId].insert(owner.category);}}
static std::wstring join(const std::set<std::wstring>& values,const wchar_t* separator){std::wstring out;for(const auto& s:values){if(!out.empty())out+=separator;out+=s;}return out;}
std::wstring gameAssetLabel(const std::string& resourceId){
 const auto id=exactId(resourceId);
 if(id.empty())return L"Unknown texture (invalid resource ID)";
 std::lock_guard<std::mutex> guard(lock);auto found=labels.find(id);if(found!=labels.end())return join(found->second,L" | ")+L" / LUT "+std::wstring(id.begin(),id.end());
 return L"Unknown owner / LUT "+std::wstring(id.begin(),id.end());
}
std::wstring gameAssetCategory(const std::string& id){std::lock_guard<std::mutex> guard(lock);auto found=categories.find(exactId(id));return found==categories.end()?L"Unknown":join(found->second,L"/");}
}
