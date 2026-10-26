// DSAR/DSAA and native-texture layout adapted from Epic LUT R5.9.3
// tools/original_snapshots.cs (CowboyBingus format research, retained notices).
// Disk-only reader. Does not load game DLLs, attach to a process or install files.
#include "game_assets.h"
#include "lut_io.h"
#include "asset_labels.h"
#include "gear_catalog.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include "preview_cache.h"
#include "texture_cache.h"
namespace omega { namespace {
namespace fs=std::filesystem;using Bytes=std::vector<uint8_t>;
constexpr uint64_t TextureType=0xcd4238c6a0c69e32ULL;
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint32_t u32(const Bytes& b,size_t p){need(p<=b.size()&&b.size()-p>=4,"Truncated asset integer");uint32_t v;std::memcpy(&v,b.data()+p,4);return v;}
uint64_t u64(const Bytes& b,size_t p){need(p<=b.size()&&b.size()-p>=8,"Truncated asset offset");uint64_t v;std::memcpy(&v,b.data()+p,8);return v;}
std::string hex(uint64_t v){std::ostringstream s;s<<std::hex<<std::setw(16)<<std::setfill('0')<<v;return s.str();}
std::wstring wide(const std::string& s){return std::wstring(s.begin(),s.end());}
bool archiveName(const std::wstring& s){return s.size()==16&&s.find_first_not_of(L"0123456789abcdefABCDEF")==std::wstring::npos;}
struct File {
 fs::path path;uint64_t size;fs::file_time_type stamp;std::ifstream f;
 explicit File(fs::path p):path(std::move(p)),size(fs::file_size(path)),stamp(fs::last_write_time(path)),f(path,std::ios::binary){need(bool(f),"Cannot open game data file");}
 Bytes read(uint64_t at,size_t n){need(n<=128*1024*1024&&at<=size&&n<=size-at,"Game asset read outside file bounds");Bytes b(n);f.clear();f.seekg(static_cast<std::streamoff>(at));if(n)f.read(reinterpret_cast<char*>(b.data()),n);need(bool(f),"Short game asset read");return b;}
 void verify(){need(fs::file_size(path)==size&&fs::last_write_time(path)==stamp,"Game data changed while reading; rescan");}
};
Bytes lz4(const Bytes& s,size_t size){
 need(size<=64*1024*1024,"Oversized game chunk");Bytes out(size);size_t p=0,k=0;
 auto length=[&](size_t n){if(n==15){uint8_t x;do{need(p<s.size(),"Truncated LZ4 length");x=s[p++];need(n<=size&&x<=size-n,"Invalid LZ4 length");n+=x;}while(x==255);}return n;};
 while(p<s.size()){auto token=s[p++];size_t n=length(token>>4);need(n<=s.size()-p&&n<=size-k,"Invalid LZ4 literals");std::copy_n(s.begin()+p,n,out.begin()+k);p+=n;k+=n;if(p==s.size())break;need(s.size()-p>=2,"Truncated LZ4 distance");size_t back=s[p]|(size_t(s[p+1])<<8);p+=2;need(back&&back<=k,"Invalid LZ4 distance");n=length(token&15)+4;need(n<=size-k,"Invalid LZ4 match");for(size_t j=0;j<n;++j){out[k]=out[k-back];++k;}}
 need(k==size,"Short LZ4 game chunk");return out;
}
struct Chunk {uint64_t start,packed;uint32_t size,packedSize;uint8_t compression;};
struct Bundle {
 File file;std::vector<Chunk> chunks;uint64_t length=0;std::map<size_t,Bytes> cache;
 explicit Bundle(const fs::path& path):file(path){auto h=file.read(0,32);need(std::memcmp(h.data(),"DSAR",4)==0,"Invalid DSAR header");size_t n=u32(h,8);need(n&&n<=1000000,"Invalid DSAR chunk count");auto table=file.read(32,n*32);for(size_t i=0;i<n;++i){size_t p=i*32;Chunk c{u64(table,p),u64(table,p+8),u32(table,p+16),u32(table,p+20),table[p+24]};need(c.size&&c.start<=UINT64_MAX-c.size,"Invalid DSAR chunk sizes");need(chunks.empty()||c.start>=chunks.back().start+chunks.back().size,"Unordered DSAR chunks");need(c.packed<=file.size&&c.packedSize<=file.size-c.packed,"Packed game chunk outside file");chunks.push_back(c);}length=chunks.back().start+chunks.back().size;}
 Bytes read(uint64_t at,size_t n){need(n<=128*1024*1024&&at<=length&&n<=length-at,"Read outside logical game bundle");Bytes out;out.reserve(n);while(n){auto it=std::upper_bound(chunks.begin(),chunks.end(),at,[](uint64_t p,const Chunk& c){return p<c.start;});need(it!=chunks.begin(),"Game chunk missing");--it;auto& c=*it;need(at<c.start+c.size,"Game chunk gap");size_t skip=size_t(at-c.start),take=std::min(n,size_t(c.size)-skip);Bytes b;if(c.compression==0)b=file.read(c.packed+skip,take);else if(c.compression==3){size_t index=size_t(it-chunks.begin());auto found=cache.find(index);if(found==cache.end()){if(cache.size()>=3)cache.clear();found=cache.emplace(index,lz4(file.read(c.packed,c.packedSize),c.size)).first;}b.assign(found->second.begin()+skip,found->second.begin()+skip+take);}else throw std::runtime_error("Unsupported game compression");out.insert(out.end(),b.begin(),b.end());at+=take;n-=take;}return out;}
};
struct Archive {uint64_t size,spans;uint32_t count;};
struct Archives {
 fs::path folder;Bundle index;Bytes data;std::vector<std::string> names;std::map<std::string,Archive> entries;std::map<size_t,std::unique_ptr<Bundle>> bundles;std::map<std::string,Bytes> spans;
 explicit Archives(const fs::path& p):folder(p),index(p/L"bundles.nxa"){
  need(index.length<=128*1024*1024,"Oversized archive index");data=index.read(0,size_t(index.length));need(data.size()>=24&&std::memcmp(data.data(),"DSAA",4)==0,"Invalid DSAA index");size_t nb=u32(data,12),count=u32(data,16);need(nb&&nb<=256&&count&&count<=1000000&&24+count*24+nb*4<=data.size(),"Invalid DSAA counts");
  auto name=[&](size_t at){need(at<data.size(),"Invalid archive name offset");size_t end=at;while(end<data.size()&&end-at<256&&data[end])++end;need(end<data.size()&&end-at<256,"Invalid archive name");return std::string(reinterpret_cast<char*>(data.data()+at),end-at);};
  for(size_t i=0;i<nb;++i){auto s=name(u32(data,24+count*24+i*4));need(!s.empty()&&s.find_first_of("/\\:")==std::string::npos&&s!="..","Invalid bundle filename");names.push_back(s);}
  for(size_t i=0;i<count;++i){size_t at=24+i*24;auto s=name(u32(data,at+8));Archive e{u64(data,at),u64(data,at+16),u32(data,at+12)};need(e.count&&e.count<=1000000,"Invalid archive spans");need(entries.emplace(s,e).second,"Duplicate archive name");}
 }
 Bytes read(const std::string& name,uint64_t at,size_t n){auto ei=entries.find(name);need(ei!=entries.end(),"Resource sidecar absent from game bundles");const auto& e=ei->second;need(at<=e.size&&n<=e.size-at&&n<=128*1024*1024,"Read outside resource archive");auto cached=spans.find(name);if(cached==spans.end()){if(spans.size()>=16)spans.clear();cached=spans.emplace(name,index.read(e.spans,size_t(e.count)*16)).first;}const auto& table=cached->second;Bytes out;out.reserve(n);
  while(n){size_t low=0,high=e.count;while(low<high){size_t mid=low+(high-low)/2;if(u32(table,mid*16)<=at)low=mid+1;else high=mid;}need(low>0,"Archive span missing");size_t p=(low-1)*16;uint64_t start=u32(table,p),stop=low<e.count?u32(table,p+16):e.size;need(stop>at,"Invalid archive span order");size_t take=size_t(std::min<uint64_t>(n,stop-at));size_t number=table[p+15];need(number<names.size(),"Invalid game bundle number");auto& bundle=bundles[number];if(!bundle)bundle=std::make_unique<Bundle>(folder/wide(names[number]));auto b=bundle->read(u32(table,p+8)+at-start,take);out.insert(out.end(),b.begin(),b.end());n-=take;at+=take;}return out;
 }
};
struct Session {fs::path folder;std::unique_ptr<Archives> bundled;std::map<std::wstring,std::unique_ptr<File>> loose;explicit Session(fs::path p):folder(std::move(p)){if(fs::exists(folder/L"bundles.nxa"))bundled=std::make_unique<Archives>(folder);}
 Bytes read(const std::wstring& name,uint64_t at,size_t n){if(fs::exists(folder/name)){auto& file=loose[name];if(!file)file=std::make_unique<File>(folder/name);return file->read(at,n);}need(bool(bundled),"Game resource archive not found");return bundled->read(std::string(name.begin(),name.end()),at,n);}
 void verify(){for(auto& pair:loose)pair.second->verify();if(bundled){bundled->index.file.verify();for(auto& pair:bundled->bundles)pair.second->file.verify();}}
};
std::mutex gate;std::map<std::wstring,std::shared_ptr<Session>> sessions;
std::shared_ptr<Session> session(const fs::path& folder){auto key=fs::absolute(folder).lexically_normal().wstring();auto it=sessions.find(key);if(it!=sessions.end()){try{it->second->verify();return it->second;}catch(...){sessions.erase(it);}}auto value=std::make_shared<Session>(folder);sessions[key]=value;return value;}
bool metadata(TextureRecord& r,Bytes h){if(h.size()!=340||std::memcmp(h.data()+192,"DDS ",4)||std::memcmp(h.data()+276,"DX10",4)||u32(h,196)!=124||u32(h,268)!=32)return false;r.width=u32(h,208);r.height=u32(h,204);r.format=u32(h,320);auto mips=std::max(1u,u32(h,220));if(!(((r.width==23||r.width==16)&&r.height>=1&&r.height<=64)||(r.width==3&&r.height==1))||(r.format!=2&&r.format!=10)||u32(h,324)!=3||u32(h,328)!=0||u32(h,332)!=1||mips>7)return false;uint32_t maxMips=1;for(auto n=std::max(r.width,r.height);n>1;n>>=1)++maxMips;if(mips>maxMips)return false;r.nativeHeader=std::move(h);return true;}
size_t pixelBytes(const TextureRecord& r){size_t total=0;for(uint32_t i=0,n=std::max(1u,u32(r.nativeHeader,220));i<n;++i)total+=std::max(1u,r.width>>i)*std::max(1u,r.height>>i)*(r.format==2?16:8);return total;}
std::wstring signature(const fs::path& folder){std::vector<std::wstring> rows;for(auto& entry:fs::directory_iterator(folder)){if(!entry.is_regular_file())continue;auto name=entry.path().filename().wstring();if(name==L"bundles.nxa"||name.rfind(L"bundles.",0)==0||archiveName(name)||name.find(L".patch_")!=std::wstring::npos)rows.push_back(name+L":"+std::to_wstring(entry.file_size())+L":"+std::to_wstring(entry.last_write_time().time_since_epoch().count()));}auto gear=folder/L"game"/L"generated_customization_armor_sets.dl_bin";if(fs::exists(gear))rows.push_back(gear.wstring()+L":"+std::to_wstring(fs::file_size(gear))+L":"+std::to_wstring(fs::last_write_time(gear).time_since_epoch().count()));std::sort(rows.begin(),rows.end());std::wstring out=fs::absolute(folder).lexically_normal().wstring();for(auto& row:rows)out+=L"\n"+row;return out;}
struct PreviewResource{std::wstring archive;uint64_t at=0,size=0,gpuAt=0,gpuSize=0,streamAt=0,streamSize=0;};
struct PreviewOwner{std::string id;std::wstring name;std::set<uint32_t> keys;std::map<std::string,std::string> overrides;};
std::map<std::string,PreviewResource> poseResources;
std::wstring gearContextFolder;std::vector<GearPiece> previewGear;uint32_t previewLastArmorKit=0,previewLastArmorBody=0,previewLastHelmetKit=0;
std::map<std::string,PreviewResource> previewUnits,previewGeometry,previewTextures;
std::map<std::string,std::map<uint32_t,PreviewResource>> previewMaterials;
std::map<std::wstring,std::weak_ptr<NativeTexture>> textureMemoryCache;
std::map<std::string,std::vector<PreviewOwner>> previewOwners;
std::map<std::string,std::map<uint32_t,std::string>> previewPartLuts;
std::map<std::wstring,ItemPreviewModel> modelCache;
GameAssetScanStats latestStats;struct Cache {std::wstring stamp;std::vector<TextureRecord> records;GameAssetScanStats stats;};std::map<std::wstring,Cache> scanCache;
} // namespace
std::vector<TextureRecord> scanGameLuts(const std::wstring& dataFolder,std::function<bool(size_t,size_t,const std::wstring&)> progress){
 std::lock_guard<std::mutex> lock(gate);fs::path folder(dataFolder);need(fs::is_directory(folder),"Select the game's data folder");if(progress&&!progress(0,1,L"Reading game archive index"))return {};auto stamp=signature(folder),key=fs::absolute(folder).lexically_normal().wstring();auto ci=scanCache.find(key);if(ci!=scanCache.end()&&ci->second.stamp==stamp){if(progress&&!progress(1,1,L"Unchanged game LUT index"))return {};latestStats=ci->second.stats;return ci->second.records;}
 poseResources.clear();previewUnits.clear();previewGeometry.clear();previewTextures.clear();previewMaterials.clear();textureMemoryCache.clear();previewOwners.clear();previewPartLuts.clear();modelCache.clear();auto gear=readGearCatalog(folder);previewGear=gear;if(gearContextFolder!=key){previewLastArmorKit=0;previewLastArmorBody=0;previewLastHelmetKit=0;gearContextFolder=key;}auto reader=session(folder);std::vector<std::wstring> names;std::map<std::wstring,uint64_t> patchRanks;
 for(auto& entry:fs::directory_iterator(folder)){if(!entry.is_regular_file())continue;auto n=entry.path().filename().wstring();if(archiveName(n)){names.push_back(n);patchRanks[n]=0;}else if(n.size()>23&&archiveName(n.substr(0,16))&&n.substr(16,7)==L".patch_"){auto tail=n.substr(23);if(tail.find_first_not_of(L"0123456789")==std::wstring::npos){try{patchRanks[n]=std::stoull(tail)+1;names.push_back(n);}catch(...){throw std::runtime_error("Invalid patch layer number");}}}}
 if(reader->bundled)for(auto& pair:reader->bundled->entries)if(archiveName(wide(pair.first))&&std::find(names.begin(),names.end(),wide(pair.first))==names.end())names.push_back(wide(pair.first));
 std::sort(names.begin(),names.end(),[&](const auto&a,const auto&b){auto pa=patchRanks[a],pb=patchRanks[b];if(pa!=pb)return pa>pb;bool sa=a.substr(0,16)==L"9ba626afa44a3aa3",sb=b.substr(0,16)==L"9ba626afa44a3aa3";if(sa!=sb)return sa;return a<b;});
 struct MainResource {std::wstring archive;std::string id;uint64_t at,size;};
 std::vector<MainResource> units;std::map<std::wstring,std::map<std::string,MainResource>> materials;
 GameAssetScanStats stats;std::map<std::string,TextureRecord> found;std::set<std::string> inspected;
 for(size_t ni=0;ni<names.size();++ni){const auto& name=names[ni];if(progress&&!progress(ni,names.size(),L"Indexing "+name))return {};++stats.archives;auto head=reader->read(name,0,72);if(u32(head,0)!=0xf0000011)continue;if(name==L"9ba626afa44a3aa3")stats.baseArchiveAvailable=true;size_t types=u32(head,4),count=u32(head,8);need(types<=4096&&count<=100000,"Invalid resource TOC counts");size_t start=72+types*32;auto toc=reader->read(name,0,start+count*80);
  for(size_t i=0;i<count;++i){size_t p=start+i*80;auto resourceType=u64(toc,p+8);auto resourceId=hex(u64(toc,p));
   if((resourceType==0x931e336d7646cc26ULL&&resourceId=="e32b270524630569")||(resourceType==0x18dead01056b72e9ULL&&resourceId=="4d1c334d294dfa97"))poseResources.emplace(resourceId,PreviewResource{name,u64(toc,p+16),u32(toc,p+56)});
   if(resourceType==TextureType)previewTextures.emplace(resourceId,PreviewResource{name,u64(toc,p+16),u32(toc,p+56),u64(toc,p+32),u32(toc,p+64),u64(toc,p+24),u32(toc,p+60)});
   if(resourceType==0xe0a48d0be9a7453fULL)previewUnits.emplace(resourceId,PreviewResource{name,u64(toc,p+16),u32(toc,p+56),u64(toc,p+32),u32(toc,p+64)});
   if(resourceType==0xc4f0f4be7fb0c8d6ULL)previewGeometry.emplace(resourceId,PreviewResource{name,u64(toc,p+16),u32(toc,p+56),u64(toc,p+32),u32(toc,p+64)});
   if(resourceType==0xe0a48d0be9a7453fULL&&!gameUnitPath(resourceId).empty())units.push_back({name,resourceId,u64(toc,p+16),u32(toc,p+56)});
   if(resourceType==0xeac0b497876adedfULL)materials[name].emplace(resourceId,MainResource{name,resourceId,u64(toc,p+16),u32(toc,p+56)});
   if(resourceType!=TextureType)continue;auto id=hex(u64(toc,p));if(!inspected.insert(id).second){++stats.duplicateResources;continue;}++stats.textureResources;if(u32(toc,p+56)<340){++stats.skipped["native_header_under_340_bytes"];continue;}TextureRecord r;r.resourceId=id;r.archiveId=std::string(name.begin(),name.begin()+16);r.archivePath=(folder/name).wstring();r.name=wide(id)+L" / "+name;r.mainOffset=u64(toc,p+16);r.mainSize=u32(toc,p+56);r.streamOffset=u64(toc,p+24);r.streamSize=u32(toc,p+60);r.gpuOffset=u64(toc,p+32);r.gpuSize=u32(toc,p+64);auto native=reader->read(name,r.mainOffset,340);if(!metadata(r,native)){std::string reason="unsupported_native_DDS_metadata";if(std::memcmp(native.data()+192,"DDS ",4)||std::memcmp(native.data()+276,"DX10",4))reason="native_DX10_DDS_header_missing";else if(u32(native,320)!=2&&u32(native,320)!=10)reason="not_RGBA16F_or_RGBA32F";else if(!(((u32(native,208)==23||u32(native,208)==16)&&u32(native,204)>=1&&u32(native,204)<=64)||(u32(native,208)==3&&u32(native,204)==1)))reason="not_material_or_pattern_LUT_dimensions";++stats.skipped[reason];continue;}need(r.gpuSize+r.streamSize>=pixelBytes(r),"Short native LUT pixel data");found.emplace(id,std::move(r));need(found.size()<=16384,"LUT index budget exceeded");}
 }
 // Current serialized graph: Unit material resource IDs -> Material texture slots.
 // Source: Epic tools/game_catalog.py. Every offset/count/range and target TOC type
 // is checked in the selected game's files before accepting an owner relationship.
 std::vector<GameAssetOwner> owners;std::set<std::pair<std::string,std::string>> ownerPairs;
 std::map<std::pair<std::wstring,std::string>,std::vector<std::string>> materialLuts;
 auto materialFor=[&](const MainResource& unit,const std::string& id)->const MainResource*{
  for(const auto& archive:{unit.archive,std::wstring(L"18235e0c9ec0e636"),std::wstring(L"ee6b1ba7e22d71ed"),std::wstring(L"9ba626afa44a3aa3")}){
   auto a=materials.find(archive);if(a==materials.end())continue;auto item=a->second.find(id);if(item!=a->second.end())return &item->second;
  }return nullptr;
 };
 for(size_t ui=0;ui<units.size();++ui){auto& unit=units[ui];if(progress&&ui%32==0&&!progress(names.size()+ui,names.size()+units.size(),L"Resolving named gun / vehicle / armor materials"))return {};
  if(unit.size<116){++stats.skipped["unit_header_under_116_bytes"];continue;}auto head=reader->read(unit.archive,unit.at,116);uint32_t at=u32(head,112);
  if(!at)continue;if(at>unit.size||unit.size-at<4){++stats.skipped["unit_material_table_outside_resource"];continue;}auto countBytes=reader->read(unit.archive,unit.at+at,4);size_t count=u32(countBytes,0);
  if(count>64||4+count*12>unit.size-at){++stats.skipped["unit_material_table_count_or_bounds"];continue;}auto table=reader->read(unit.archive,unit.at+at,4+count*12);++stats.namedUnits;
  auto owner=gameUnitPath(unit.id);auto category=gameUnitCategory(owner);auto alias=gameUnitAliases(unit.id);if(!alias.empty())owner=alias+L" / "+owner;
  for(size_t i=0;i<count;++i){auto materialId=hex(u64(table,4+count*4+i*8));auto material=materialFor(unit,materialId);if(!material){++stats.skipped["named_unit_material_not_in_owning_or_shared_archives"];continue;}
   previewMaterials[unit.id][u32(table,4+i*4)]={material->archive,material->at,material->size};
   auto key=std::make_pair(material->archive,materialId);auto cached=materialLuts.find(key);
   if(cached==materialLuts.end()){
    std::vector<std::string> refs;
    if(material->size<136){++stats.skipped["material_header_under_136_bytes"];cached=materialLuts.emplace(key,refs).first;}
    else{auto header=reader->read(material->archive,material->at,136);size_t textures=u32(header,64);
     if(textures>64||136+textures*12>material->size){++stats.skipped["material_texture_table_count_or_bounds"];}
     else{auto slots=reader->read(material->archive,material->at,136+textures*12);for(size_t j=0;j<textures;++j){auto slot=u32(slots,136+j*4);if(slot!=0x7e662968&&slot!=0x81d4c49d&&slot!=0x0e494183)continue;auto textureId=hex(u64(slots,136+textures*4+j*8));if(found.count(textureId)){refs.push_back(textureId);++stats.materialLinks;}}}
     cached=materialLuts.emplace(key,std::move(refs)).first;
    }
   }
   for(const auto& lut:cached->second){
    uint32_t materialKey=u32(table,4+i*4);
    auto& link=previewPartLuts[unit.id][materialKey];if(link.empty()||found.at(lut).width==23)link=lut;
    auto& list=previewOwners[lut];auto oi=std::find_if(list.begin(),list.end(),[&](const PreviewOwner&o){return o.id==unit.id;});
    if(oi==list.end()){list.push_back({unit.id,owner,{}});oi=list.end()-1;}oi->keys.insert(materialKey);
    if(ownerPairs.emplace(lut,unit.id).second)owners.push_back({lut,owner,category});
   }
  }
 }
 // Current kit overrides are authoritative for body/cape appearance. Validate
 // every unit, material and LUT against the selected installation before listing.
 for(const auto& piece:gear){
  if(piece.contextOnly)continue;
  // A zero unit means the kit inherits its mesh. Texture references remain
  // explicit and editable; do not fabricate an equipped body's mesh identity.
  for(const auto& override:piece.overrides){if(override.first!="material_lut"&&override.first!="pattern_lut"&&override.first!="cape_lut")continue;if(!found.count(override.second))continue;
   auto& list=previewOwners[override.second];auto existing=std::find_if(list.begin(),list.end(),[&](const PreviewOwner&o){return o.id==piece.unit&&o.name==piece.name;});
   if(existing==list.end()){list.push_back({piece.unit,piece.name,{},piece.overrides});owners.push_back({override.second,piece.name,L"Armor"});}
  }
  auto ui=previewUnits.find(piece.unit);if(ui==previewUnits.end()){++stats.skipped["gear_unit_absent"];continue;}
  auto& ur=ui->second;if(ur.size<116)continue;auto head=reader->read(ur.archive,ur.at,116);auto at=u32(head,112);if(!at||at>ur.size||ur.size-at<4)continue;
  auto count=u32(reader->read(ur.archive,ur.at+at,4),0);if(count>64||4+count*12>ur.size-at)continue;auto table=reader->read(ur.archive,ur.at+at,4+count*12);
  MainResource context{wide(piece.archive),piece.unit,ur.at,ur.size};
  for(size_t i=0;i<count;++i){auto key=u32(table,4+i*4);auto material=materialFor(context,hex(u64(table,4+count*4+i*8)));if(!material){context.archive=ur.archive;material=materialFor(context,hex(u64(table,4+count*4+i*8)));}if(!material||material->size<136)continue;
   previewMaterials[piece.unit][key]={material->archive,material->at,material->size};
   auto header=reader->read(material->archive,material->at,136);auto nt=u32(header,64);if(nt>64||136+nt*12>material->size)continue;auto textureTable=reader->read(material->archive,material->at,136+nt*12);
   for(size_t t=0;t<nt;++t){auto slot=u32(textureTable,136+t*4);const char* channel=slot==0x7e662968?"material_lut":slot==0x81d4c49d?"pattern_lut":slot==0x0e494183?"cape_lut":nullptr;if(!channel)continue;
    auto override=piece.overrides.find(channel);auto lut=override==piece.overrides.end()?hex(u64(textureTable,136+nt*4+t*8)):override->second;
    if(!found.count(lut)){++stats.skipped["gear_lut_unavailable_or_unsupported"];continue;}
    auto& list=previewOwners[lut];auto oi=std::find_if(list.begin(),list.end(),[&](const PreviewOwner&o){return o.id==piece.unit&&o.name==piece.name;});
    if(oi==list.end()){list.push_back({piece.unit,piece.name,{},piece.overrides});oi=list.end()-1;owners.push_back({lut,piece.name,L"Armor"});}oi->keys.insert(key);
   }
  }
 }
 // Prefer current kit identities over duplicate legacy SDK unit labels.
 for(auto& pair:previewOwners){auto& list=pair.second;std::set<std::string> current;for(const auto& o:list)if(std::any_of(previewGear.begin(),previewGear.end(),[&](const GearPiece&p){return p.name==o.name&&p.unit==o.id;}))current.insert(o.id);list.erase(std::remove_if(list.begin(),list.end(),[&](const PreviewOwner&o){return current.count(o.id)&&std::none_of(previewGear.begin(),previewGear.end(),[&](const GearPiece&p){return p.name==o.name&&p.unit==o.id;});}),list.end());}
 setGameAssetOwners(owners);
 for(auto& pair:found){auto category=gameAssetCategory(pair.first);pair.second.category=category;if(category!=L"Unknown"){++stats.namedLuts;pair.second.name=gameAssetLabel(pair.first);if(category.find(L"Weapons")!=std::wstring::npos)++stats.namedWeaponLuts;if(category.find(L"Vehicles")!=std::wstring::npos)++stats.namedVehicleLuts;if(category.find(L"Armor")!=std::wstring::npos)++stats.namedArmorLuts;}}
 reader->verify();need(stamp==signature(folder),"Game archives changed during scan; retry");std::vector<TextureRecord> result;for(auto& pair:found)result.push_back(std::move(pair.second));if(progress&&!progress(names.size(),names.size(),L"Game LUT index ready"))return {};stats.supportedLuts=result.size();latestStats=stats;scanCache[key]={stamp,result,stats};return result;
}
void clearGameAssetCache(){std::lock_guard<std::mutex> lock(gate);scanCache.clear();sessions.clear();previewOwners.clear();poseResources.clear();previewUnits.clear();previewGeometry.clear();previewPartLuts.clear();modelCache.clear();}
GameAssetScanStats gameAssetScanStats(){std::lock_guard<std::mutex> lock(gate);return latestStats;}
LutDocument loadGameLut(const TextureRecord& record){std::lock_guard<std::mutex> lock(gate);fs::path path(record.archivePath);auto reader=session(path.parent_path());auto name=path.filename().wstring();TextureRecord actual=record;need(metadata(actual,reader->read(name,record.mainOffset,340)),"Selected texture is no longer a supported LUT");need(actual.nativeHeader==record.nativeHeader,"Selected LUT metadata changed; rescan");size_t total=pixelBytes(actual);Bytes pixels;if(record.gpuSize>=total)pixels=reader->read(name+L".gpu_resources",record.gpuOffset,total);else if(record.streamSize>=total)pixels=reader->read(name+L".stream",record.streamOffset,total);else{need(record.streamSize+record.gpuSize>=total,"Short native LUT data");size_t take=size_t(std::min<uint64_t>(record.streamSize,total));pixels=reader->read(name+L".stream",record.streamOffset,take);if(take<total){auto rest=reader->read(name+L".gpu_resources",record.gpuOffset,total-take);pixels.insert(pixels.end(),rest.begin(),rest.end());}}
 Bytes dds(record.nativeHeader.begin()+192,record.nativeHeader.end());dds.insert(dds.end(),pixels.begin(),pixels.end());auto result=decodeDDS(dds,record.name);result.resourceId=record.resourceId;result.archiveId=record.archiveId;result.nativeHeader=record.nativeHeader;result.sourcePath=record.archivePath;reader->verify();return result;}


namespace {
uint64_t previewHash(const void*p,size_t n){auto b=(const unsigned char*)p;uint64_t h=1469598103934665603ULL;for(size_t i=0;i<n;i++){h^=b[i];h*=1099511628211ULL;}return h;}
std::string bindingName(uint32_t value){struct Entry{uint32_t id;const char*text;};static const Entry names[]={
#include "material_binding_names.inc"
};auto it=std::lower_bound(std::begin(names),std::end(names),value,[](const Entry&a,uint32_t b){return a.id<b;});return it!=std::end(names)&&it->id==value?it->text:hex(value);}
Bytes readPreviewBytes(Session&reader,const std::wstring&archive,uint64_t at,size_t n){Bytes result;result.reserve(n);while(n){auto take=std::min<size_t>(n,32*1024*1024);auto chunk=reader.read(archive,at,take);result.insert(result.end(),chunk.begin(),chunk.end());n-=take;at+=take;}return result;}
std::shared_ptr<const NativeTexture> fetchPreviewTexture(Session&reader,const std::wstring&stamp,const std::string&id){
 auto found=previewTextures.find(id);need(found!=previewTextures.end(),"Material texture absent from current archive index");auto key=stamp+L"/native-texture-v1/"+wide(id);auto cached=textureMemoryCache[key].lock();if(cached)return cached;auto texture=readTextureCache(key);
 if(!texture){auto&r=found->second;auto main=readPreviewBytes(reader,r.archive,r.at,size_t(r.size));Bytes stream,gpu;if(r.streamSize)stream=readPreviewBytes(reader,r.archive+L".stream",r.streamAt,size_t(r.streamSize));if(r.gpuSize)gpu=readPreviewBytes(reader,r.archive+L".gpu_resources",r.gpuAt,size_t(r.gpuSize));texture=std::make_shared<NativeTexture>(decodeNativeTexture(main,stream,gpu));writeTextureCache(key,*texture);}
 texture->resourceId=id;texture->identity=previewHash(key.data(),key.size()*sizeof(wchar_t));textureMemoryCache[key]=texture;return texture;
}
void bindPreviewMaterials(Session&reader,ItemPreviewModel&model,const std::string&unitId,const std::wstring&stamp,const std::map<std::string,std::string>& overrides){
 size_t loaded=0;std::vector<std::string> missing;
 for(auto&part:model.parts){auto material=previewMaterials[unitId].find(part.materialKey);if(material==previewMaterials[unitId].end()){missing.push_back("material "+hex(part.materialKey));continue;}auto&r=material->second;auto b=readPreviewBytes(reader,r.archive,r.at,size_t(r.size));need(b.size()>=136,"Truncated current material");part.baseMaterial=u64(b,24);part.transparent=part.baseMaterial==0x8e7efcdf45ac19cfULL;size_t nt=u32(b,64),ns=u32(b,104);need(nt<=4096&&ns<=4096&&136+nt*12+ns*20<=b.size(),"Invalid material texture/settings table");
  std::map<std::string,std::string> textures;for(size_t i=0;i<nt;i++)textures[bindingName(u32(b,136+i*4))]=hex(u64(b,136+nt*4+i*8));
  if(textures.count("material_lut")&&textures.count("id_masks_array")){textures.emplace("customization_material_detail_tiler_array","5015467d918378bf");textures.emplace("pattern_lut","cf0cc31b981786c9");}
  for(const auto& override:overrides){auto slot=override.first=="cape_nac"?"base_data":override.first; if(textures.count(slot))textures[slot]=override.second;}
  for(auto&t:textures){if(t.second=="0000000000000000")continue;try{part.textures[t.first]=fetchPreviewTexture(reader,stamp,t.second);++loaded;}catch(const std::exception&e){missing.push_back(t.first+" "+t.second+": "+e.what());}}
  size_t definitions=136+nt*12,values=definitions+ns*20;for(size_t i=0;i<ns;i++){size_t at=definitions+i*20;uint32_t type=u32(b,at),count=u32(b,at+4),usage=u32(b,at+8),offset=u32(b,at+12),stride=u32(b,at+16);size_t components=type<=3?type+1:type==12?uint64_t(count)*stride/4:0;if(!components)continue;need(components<=1048576&&values+offset<=b.size()&&components*4<=b.size()-values-offset,"Material values outside resource");std::vector<float> data(components);std::memcpy(data.data(),b.data()+values+offset,components*4);part.parameters[bindingName(usage)]=std::move(data);}
 }
 model.notice=std::to_wstring(model.parts.size())+L" materials / "+std::to_wstring(loaded)+L" source textures. Static pose; preview lighting differs from game.";
 if(!missing.empty()){model.notice+=L" Missing "+std::to_wstring(missing.size())+L" bindings.";wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);auto logs=fs::path(exe).parent_path()/L"Logs";fs::create_directories(logs);std::ofstream log(logs/L"material-bindings.log",std::ios::app);log<<"Unit "<<unitId<<"\n";for(auto&line:missing)log<<line<<"\n";}
}
}
std::vector<std::wstring> gameItemPreviewOwners(const std::string& lut){std::lock_guard<std::mutex> lock(gate);std::vector<std::wstring> out;auto it=previewOwners.find(lut);if(it!=previewOwners.end())for(auto&o:it->second)out.push_back(o.name);return out;}
ItemPreviewModel loadGameItemPreview(const std::wstring& folder,const std::string& lut,size_t index,bool singlePiece){
 std::lock_guard<std::mutex> lock(gate);auto oi=previewOwners.find(lut);need(oi!=previewOwners.end()&&index<oi->second.size(),"No verified mesh owner for this LUT. Select a named game item.");auto owner=oi->second[index];auto stamp=signature(folder);auto reader=session(folder);
 auto selected=std::find_if(previewGear.begin(),previewGear.end(),[&](const GearPiece& p){return p.name==owner.name&&p.unit==owner.id;});
 if(selected==previewGear.end())selected=std::find_if(previewGear.begin(),previewGear.end(),[&](const GearPiece&p){return p.unit==owner.id&&!p.contextOnly&&std::any_of(p.overrides.begin(),p.overrides.end(),[&](const auto& o){return o.second==lut;});});
 if(selected!=previewGear.end()&&selected->kind==0&&!singlePiece){previewLastArmorKit=selected->kit;previewLastArmorBody=selected->body<2?selected->body:0;}
 if(selected!=previewGear.end()&&selected->kind==1)previewLastHelmetKit=selected->kit;
 auto cacheKey=stamp+L"/outfit-preview-v6-salute-upper-body/"+owner.name+L"/"+wide(lut)+(singlePiece?L"/piece":L"/outfit")+L"/armor-context/"+std::to_wstring(previewLastArmorKit)+L"/"+std::to_wstring(previewLastArmorBody)+L"/helmet/"+std::to_wstring(previewLastHelmetKit);auto cached=modelCache.find(cacheKey);if(cached!=modelCache.end())return cached->second;
 SourcePose salute;auto clip=poseResources.find("e32b270524630569"),rig=poseResources.find("4d1c334d294dfa97");if(clip!=poseResources.end()&&rig!=poseResources.end()){auto&c=clip->second;auto&r=rig->second;salute=decodeSourcePose(reader->read(c.archive,c.at,size_t(c.size)),reader->read(r.archive,r.at,size_t(r.size)));}
 auto decode=[&](const std::string& id,const std::map<std::string,std::string>& overrides,const std::wstring& label){
  auto ui=previewUnits.find(id);need(ui!=previewUnits.end(),"Required source outfit unit is unavailable");auto ur=ui->second;auto key=stamp+L"/source-unit-v12-salute-upper-body/"+wide(id)+(label.rfind(L"[Armor]",0)==0||label.rfind(L"[Helmet]",0)==0?L"/salute":L"/bind");ItemPreviewModel model;
  if(!readPreviewCache(key,model)){auto unit=reader->read(ur.archive,ur.at,size_t(ur.size));need(unit.size()>=116,"Short current unit resource");Bytes geo;auto gpu=ur;if(u64(unit,16)){auto gi=previewGeometry.find(hex(u64(unit,16)));need(gi!=previewGeometry.end(),"Required geometry group is unavailable");gpu=gi->second;geo=reader->read(gpu.archive,gpu.at,size_t(gpu.size));}need(gpu.gpuSize>0,"Selected item has no GPU geometry buffer");
   model=decodeItemPreview(unit,geo,std::stoull(id,nullptr,16),[&](uint64_t at,size_t n){need(at<=gpu.gpuSize&&n<=gpu.gpuSize-at,"Mesh read outside source GPU resource");return reader->read(gpu.archive+L".gpu_resources",gpu.gpuAt+at,n);},{},label.rfind(L"[Armor]",0)==0||label.rfind(L"[Helmet]",0)==0?salute:SourcePose{});writePreviewCache(key,model);
  }
  bindPreviewMaterials(*reader,model,id,stamp,overrides);
  for(auto& part:model.parts){part.name=label+L" / "+(previewSourceName(part.materialKey).empty()?L"Material "+wide(hex(part.materialKey)):wide(previewSourceName(part.materialKey)));auto it=previewPartLuts[id].find(part.materialKey);if(it!=previewPartLuts[id].end())part.lutResourceId=it->second;
   for(auto channel:{"material_lut","cape_lut","pattern_lut"}){auto t=part.textures.find(channel);if(t!=part.textures.end()){if(part.lutResourceId.empty()||std::string(channel)=="material_lut")part.lutResourceId=t->second->resourceId;if(t->second->resourceId==lut)part.lutResourceId=lut;}}
  }
  for(auto& t:model.triangles)t.affected=model.parts[t.part].lutResourceId==lut;return model;
 };
 ItemPreviewModel result;
 auto append=[&](ItemPreviewModel part){need(result.vertices.size()+part.vertices.size()<10000000&&result.triangles.size()+part.triangles.size()<10000000,"Outfit exceeds preview geometry budget");auto vertexOffset=uint32_t(result.vertices.size()),partOffset=uint32_t(result.parts.size());result.vertices.insert(result.vertices.end(),part.vertices.begin(),part.vertices.end());for(auto t:part.triangles){t.a+=vertexOffset;t.b+=vertexOffset;t.c+=vertexOffset;t.part+=partOffset;result.triangles.push_back(t);}result.parts.insert(result.parts.end(),part.parts.begin(),part.parts.end());result.sourceMeshes+=part.sourceMeshes;};

 size_t pieces=0;bool inheritedCape=false;std::vector<std::wstring> missing;
 auto assemble=[&](const GearPiece& kit,uint32_t bodyType){
  auto body=bodyType;std::set<std::pair<std::string,uint32_t>> seen;
  for(const auto& piece:previewGear)if(piece.kit==kit.kit&&piece.kind==0&&!piece.contextOnly&&(piece.body==3||piece.body==body)&&seen.emplace(piece.unit,piece.type).second){try{append(decode(piece.unit,piece.overrides,piece.name));++pieces;}catch(const std::exception&e){missing.push_back(piece.name+L": "+wide(e.what()));}}
  // Helmet match is by the current customization set ID, never a name guess.
  auto helmet=std::find_if(previewGear.begin(),previewGear.end(),[&](const GearPiece&p){return p.kind==1&&(previewLastHelmetKit?p.kit==previewLastHelmetKit:(kit.set&&p.set==kit.set))&&(p.body==3||p.body==body);});
  if(helmet!=previewGear.end())try{append(decode(helmet->unit,helmet->overrides,helmet->name));++pieces;}catch(const std::exception&e){missing.push_back(L"Helmet: "+wide(e.what()));}
 };
 if(selected!=previewGear.end()&&selected->kind==0&&!singlePiece){assemble(*selected,previewLastArmorBody);
 }else if(selected!=previewGear.end()&&selected->kind==2){
  if(previewLastArmorKit&&!singlePiece){auto armour=std::find_if(previewGear.begin(),previewGear.end(),[&](const GearPiece&p){return p.kit==previewLastArmorKit&&p.kind==0&&!p.contextOnly;});if(armour!=previewGear.end())assemble(*armour,previewLastArmorBody);}
  auto id=selected->unit;if(id=="0000000000000000"){
   auto cape=std::find_if(previewGear.begin(),previewGear.end(),[&](const GearPiece&p){return p.contextOnly&&p.unit!="0000000000000000"&&previewUnits.count(p.unit);});need(cape!=previewGear.end(),"No inherited cape mesh exists in the installed customization kits");id=cape->unit;inheritedCape=true;
  }append(decode(id,selected->overrides,selected->name));++pieces;
 }else{append(decode(owner.id,owner.overrides,owner.name));++pieces;}
 need(!result.vertices.empty(),"No displayable geometry in the selected outfit");result.name=owner.name.substr(0,owner.name.find(L" / "));result.identity=previewHash(cacheKey.data(),cacheKey.size()*sizeof(wchar_t));
 result.notice=std::to_wstring(pieces)+L" source pieces assembled with the player salute on arms/hands and upper back; legs stay in source pose. Preview lighting differs from game.";
 if(selected!=previewGear.end()&&selected->kind==2)result.notice=inheritedCape?L"Standard cape source mesh with selected kit textures. Simulated material; static cloth.":L"Source cape mesh and selected kit textures. Simulated material; static cloth.";
 if(!missing.empty()){result.notice+=L" Unavailable pieces: "+std::to_wstring(missing.size())+L" (see Logs/outfit-preview.log).";wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);auto logs=fs::path(exe).parent_path()/L"Logs";fs::create_directories(logs);std::wofstream out(logs/L"outfit-preview.log",std::ios::app);for(auto& message:missing)out<<message<<L"\n";}
 reader->verify();need(stamp==signature(folder),"Game data changed while loading preview");if(modelCache.size()>=8)modelCache.erase(modelCache.begin());modelCache.emplace(cacheKey,result);return result;
}

} // namespace omega
