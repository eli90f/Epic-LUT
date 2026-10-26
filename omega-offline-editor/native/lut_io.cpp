// DDS and Stingray patch layout faithfully ported from Epic LUT R5.9.3 by its
// original authors (src/core/dds.lua, src/presets/patch_export.lua), with native
// Windows atomic publication. No game process, loader or live memory access.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "lut_io.h"
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <map>
#include <utility>

namespace omega { namespace {
constexpr size_t MaxBytes=1024*1024;
constexpr char BaseArchive[]="9ba626afa44a3aa3";
constexpr char TextureType[]="cd4238c6a0c69e32";
using Bytes=std::vector<uint8_t>;
[[noreturn]] void fail(const char* text){throw std::runtime_error(text);}
void require(bool value,const char* text){if(!value)fail(text);}
uint32_t u32(const Bytes& b,size_t at){require(at<=b.size()&&b.size()-at>=4,"Truncated binary header");return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
void word(Bytes& b,uint32_t n){for(unsigned i=0;i<4;++i)b.push_back(uint8_t(n>>(8*i)));}
void qword(Bytes& b,uint64_t n){for(unsigned i=0;i<8;++i)b.push_back(uint8_t(n>>(8*i)));}
void setWord(Bytes& b,size_t at,uint32_t n){require(at<=b.size()&&b.size()-at>=4,"Header write outside buffer");for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(n>>(8*i));}
void append(Bytes& b,const Bytes& part){b.insert(b.end(),part.begin(),part.end());}
bool magic(const Bytes& b,size_t at,const char* text,size_t n){return at<=b.size()&&n<=b.size()-at&&std::memcmp(b.data()+at,text,n)==0;}
bool validId(const std::string& id){return id.size()==16&&std::all_of(id.begin(),id.end(),[](unsigned char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');});}
void hash(Bytes& b,const std::string& id){require(validId(id),"Missing exact 16-digit resource/archive ID");for(int i=14;i>=0;i-=2){auto nib=[](char c){return c<='9'?c-'0':(c>='a'?c-'a':c-'A')+10;};b.push_back(uint8_t(nib(id[size_t(i)])*16+nib(id[size_t(i+1)])));}}
void dimensions(uint32_t w,uint32_t h){require(((w==23||w==16)&&h>=1&&h<=64)||(w==3&&h==1),"LUT must be a 23-column material, 16-column cape (1-64 rows), or a 3x1 pattern");}
void pixels(const std::vector<Pixel>& data,size_t expected){require(data.size()==expected,"LUT pixel count differs from dimensions");for(const auto& p:data)for(float v:p)require(std::isfinite(v),"Nonfinite LUT value");}
void validate(const LutDocument& d){dimensions(d.width,d.height);pixels(d.pixels,size_t(d.width)*d.height);}
float readFloat(const Bytes& b,size_t at){uint32_t bits=u32(b,at);float value;std::memcpy(&value,&bits,4);return value;}
void writeFloat(Bytes& b,float value){uint32_t bits;std::memcpy(&bits,&value,4);word(b,bits);}
float half(uint16_t bits){unsigned e=(bits>>10)&31,m=bits&1023;require(e!=31,"Nonfinite DDS value");float value=e==0?std::ldexp(float(m),-24):std::ldexp(1.0f+float(m)/1024.0f,int(e)-15);return bits&0x8000?-value:value;}
Bytes readFile(const std::wstring& path,size_t maximum=MaxBytes){
 HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 require(h!=INVALID_HANDLE_VALUE,"Cannot open input file");LARGE_INTEGER length{};
 if(!GetFileSizeEx(h,&length)||length.QuadPart<0||length.QuadPart>int64_t(maximum)){CloseHandle(h);fail("Input file exceeds its size bound");}
 Bytes out(size_t(length.QuadPart));DWORD done=0;bool ok=out.empty()||ReadFile(h,out.data(),DWORD(out.size()),&done,nullptr);CloseHandle(h);require(ok&&done==out.size(),"Cannot read complete input file");return out;
}
std::atomic<unsigned long> serial{0};
std::wstring temporary(const std::wstring& path){return path+L".omega-pending-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++serial);}
void writeNew(const std::wstring& path,const Bytes& bytes){
 HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);require(h!=INVALID_HANDLE_VALUE,"Cannot create output staging file");
 DWORD done=0;bool ok=bytes.empty()||WriteFile(h,bytes.data(),DWORD(bytes.size()),&done,nullptr);ok=ok&&done==bytes.size()&&FlushFileBuffers(h);CloseHandle(h);
 if(!ok){DeleteFileW(path.c_str());fail("Cannot write complete output file");}
 try{require(readFile(path)==bytes,"Output read-back differs");}catch(...){DeleteFileW(path.c_str());throw;}
}
void atomicFile(const std::wstring& path,const Bytes& bytes){
 require(!path.empty()&&path.find(L'\0')==std::wstring::npos,"Choose an output path");auto temp=temporary(path);writeNew(temp,bytes);
 if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temp.c_str());fail("Cannot replace output file; original was preserved");}
}
std::string utf8(const std::wstring& w){
 if(w.empty())return {};require(w.find(L'\0')==std::wstring::npos&&w.size()<=32768,"Invalid preset string");int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),int(w.size()),nullptr,0,nullptr,nullptr);require(n>0,"Invalid Unicode preset metadata");std::string out(size_t(n),'\0');require(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),int(w.size()),out.data(),n,nullptr,nullptr)==n,"Cannot encode preset metadata");return out;
}
std::wstring wide(const std::string& s){
 if(s.empty())return {};require(s.find('\0')==std::string::npos&&s.size()<=131072,"Invalid preset string");int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);require(n>0,"Invalid UTF-8 preset metadata");std::wstring out(size_t(n),L'\0');require(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),out.data(),n)==n,"Cannot decode preset metadata");return out;
}
void string(Bytes& out,const std::string& s){require(s.size()<=131072,"Preset string too large");word(out,uint32_t(s.size()));out.insert(out.end(),s.begin(),s.end());}
uint32_t crc32(const Bytes& data,size_t begin){uint32_t crc=~0u;for(size_t i=begin;i<data.size();++i){crc^=data[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(crc&1?0xedb88320u:0u);}return ~crc;}
struct Reader{
 const Bytes& b;size_t at=0;
 uint32_t number(){auto value=u32(b,at);at+=4;return value;}
 Bytes take(size_t n){require(at<=b.size()&&n<=b.size()-at,"Truncated preset");Bytes out(b.begin()+at,b.begin()+at+n);at+=n;return out;}
 std::string text(){auto bytes=take(number());return std::string(bytes.begin(),bytes.end());}
 std::vector<Pixel> data(){auto count=number();require(count<=23*64,"Too many preset pixels");std::vector<Pixel> out(count);for(auto& p:out)for(float& v:p){v=readFloat(b,at);at+=4;require(std::isfinite(v),"Nonfinite preset value");}return out;}
};
} // private

LutDocument decodeDDS(const Bytes& b,const std::wstring& label){
 require(b.size()>=128&&b.size()<=MaxBytes&&magic(b,0,"DDS ",4),"Invalid DDS");require(u32(b,4)==124&&u32(b,76)==32,"Invalid DDS header sizes");
 LutDocument d;d.width=u32(b,16);d.height=u32(b,12);dimensions(d.width,d.height);
 require(u32(b,24)<=1&&(u32(b,8)&0x800000)==0&&u32(b,112)==0,"Only single-layer 2D LUT textures are supported");
 uint32_t format=u32(b,84);size_t offset=128;
 if(format==0x30315844){require(b.size()>=148&&u32(b,132)==3&&u32(b,136)==0&&u32(b,140)==1,"Unsupported DDS texture kind");format=u32(b,128);offset=148;}
 else if(format==113)format=10;else if(format==116)format=2;else fail("DDS requires RGBA16F or RGBA32F");
 require(format==2||format==10,"DDS requires RGBA16F or RGBA32F");
 uint32_t levels=std::max(1u,u32(b,28)),maxLevels=1,extent=std::max(d.width,d.height);while(extent>1){++maxLevels;extent/=2;}require(levels<=maxLevels,"DDS mip count exceeds dimensions");
 size_t total=0;for(uint32_t i=0;i<levels;++i)total+=size_t(std::max(1u,d.width>>i))*std::max(1u,d.height>>i)*4*(format==2?4:2);
 require(b.size()==offset+total,"DDS payload size mismatch");d.pixels.resize(size_t(d.width)*d.height);
 const size_t channelBytes=format==2?4:2;const size_t channels=total/channelBytes;
 for(size_t i=0;i<channels;++i){size_t at=offset+i*channelBytes;float value=format==2?readFloat(b,at):half(uint16_t(b[at])|uint16_t(b[at+1])<<8);require(std::isfinite(value),"Nonfinite DDS value");if(i<d.pixels.size()*4)d.pixels[i/4][i%4]=value;}
 d.original=d.pixels;d.label=label;return d;
}
Bytes encodeDDS(const LutDocument& d){
 validate(d);Bytes out;out.reserve(148+d.pixels.size()*16);out.insert(out.end(),{'D','D','S',' '});
 for(uint32_t v:{124u,0x100fu,d.height,d.width,d.width*16,0u,1u})word(out,v);for(int i=0;i<11;++i)word(out,0);
 for(uint32_t v:{32u,4u,0x30315844u,0u,0u,0u,0u,0u,0x1000u,0u,0u,0u,0u,2u,3u,0u,1u,0u})word(out,v);
 require(out.size()==148,"Internal DDS header size mismatch");for(const auto& p:d.pixels)for(float v:p)writeFloat(out,v);return out;
}
LutDocument loadDDS(const std::wstring& path){auto d=decodeDDS(readFile(path),path.substr(path.find_last_of(L"/\\")+1));d.sourcePath=path;return d;}
void saveDDS(const LutDocument& d,const std::wstring& path){atomicFile(path,encodeDDS(d));}
void savePreset(const LutDocument& d,const std::wstring& path){
 validate(d);require(d.original.empty()||(d.width==3?d.original.size()==3:(d.original.size()%d.width==0&&d.original.size()<=d.width*64)),"Invalid original LUT baseline");for(const auto& p:d.original)for(float v:p)require(std::isfinite(v),"Nonfinite original LUT value");
 require(d.resourceId.empty()||validId(d.resourceId),"Invalid resource ID");require(d.archiveId.empty()||validId(d.archiveId),"Invalid archive ID");require(d.nativeHeader.empty()||d.nativeHeader.size()==340,"Invalid native source header size");
 const char marker[]="OMEGA-LUT-PRESET\n";Bytes out(marker,marker+sizeof(marker)-1);word(out,1);word(out,d.width);word(out,d.height);
 string(out,d.resourceId);string(out,d.archiveId);string(out,utf8(d.label));string(out,utf8(d.sourcePath));word(out,uint32_t(d.nativeHeader.size()));append(out,d.nativeHeader);
 for(const auto* list:{&d.pixels,&d.original}){word(out,uint32_t(list->size()));for(const auto& p:*list)for(float v:p)writeFloat(out,v);}
 word(out,crc32(out,0));atomicFile(path,out);
}
LutDocument loadPreset(const std::wstring& path){
 auto bytes=readFile(path);const char marker[]="OMEGA-LUT-PRESET\n";constexpr size_t markerSize=sizeof(marker)-1;require(bytes.size()>=markerSize+16&&magic(bytes,0,marker,markerSize),"Invalid OMEGA LUT preset");auto checksum=u32(bytes,bytes.size()-4);bytes.resize(bytes.size()-4);require(crc32(bytes,0)==checksum,"Preset checksum mismatch");Reader r{bytes,markerSize};require(r.number()==1,"Unsupported OMEGA LUT preset version");LutDocument d;d.width=r.number();d.height=r.number();dimensions(d.width,d.height);d.resourceId=r.text();d.archiveId=r.text();d.label=wide(r.text());d.sourcePath=wide(r.text());auto headerSize=r.number();require(headerSize==0||headerSize==340,"Invalid native source header size");d.nativeHeader=r.take(headerSize);d.pixels=r.data();d.original=r.data();require(r.at==bytes.size(),"Trailing preset data");validate(d);require(d.original.empty()||(d.width==3?d.original.size()==3:(d.original.size()%d.width==0&&d.original.size()<=d.width*64)),"Invalid original LUT baseline");require(d.resourceId.empty()||validId(d.resourceId),"Invalid resource ID");require(d.archiveId.empty()||validId(d.archiveId),"Invalid archive ID");return d;
}
void exportPatch(const LutDocument& d,const std::wstring& folder){
 validate(d);require(validId(d.resourceId)&&validId(d.archiveId),"Original resource/archive provenance is required for patch export");const auto& h=d.nativeHeader;
 require(h.size()==340&&magic(h,192,"DDS ",4)&&magic(h,276,"DX10",4),"Original native texture metadata is missing or invalid");require(u32(h,320)==2||u32(h,320)==10,"Original texture is not a supported float LUT");require(u32(h,324)==3&&u32(h,328)==0&&u32(h,332)==1,"Original texture is not a single-layer 2D LUT");
 const auto oldWidth=u32(h,208),oldHeight=u32(h,204);dimensions(oldWidth,oldHeight);require(d.width==oldWidth,"LUT columns differ from selected destination");
 auto dds=encodeDDS(d);Bytes body(h.begin(),h.begin()+4);word(body,0);word(body,0xffffffffu);body.resize(192,0);body.insert(body.end(),dds.begin(),dds.begin()+148);Bytes gpu(dds.begin()+148,dds.end());
 Bytes main;word(main,0xf0000011);word(main,1);word(main,1);word(main,0);main.resize(72,0);qword(main,0);hash(main,TextureType);qword(main,1);word(main,16);word(main,64);
 hash(main,d.resourceId);hash(main,TextureType);qword(main,184);qword(main,0);qword(main,0);qword(main,0);qword(main,0);word(main,uint32_t(body.size()));word(main,0);word(main,uint32_t(gpu.size()));word(main,16);word(main,64);word(main,1);require(main.size()==184,"Internal patch entry size mismatch");append(main,body);
 require(!folder.empty()&&folder.find(L'\0')==std::wstring::npos,"Choose a patch destination folder");require(GetFileAttributesW(folder.c_str())==INVALID_FILE_ATTRIBUTES,"Patch destination already exists; choose a new folder");auto staging=temporary(folder);require(CreateDirectoryW(staging.c_str(),nullptr)!=0,"Cannot create patch staging folder");
 const std::wstring stem=staging+L"\\9ba626afa44a3aa3.patch_0";std::vector<std::wstring> written;
 try{written.push_back(stem);writeNew(stem,main);written.push_back(stem+L".gpu_resources");writeNew(written.back(),gpu);written.push_back(stem+L".stream");writeNew(written.back(),{});require(MoveFileExW(staging.c_str(),folder.c_str(),MOVEFILE_WRITE_THROUGH)!=0,"Cannot publish complete patch triplet; destination was preserved");}
 catch(...){for(const auto& file:written)DeleteFileW(file.c_str());RemoveDirectoryW(staging.c_str());throw;}
}
EpicPreset loadEpicPreset(const std::wstring& path){
 // Exact record schema: original src/legacy/presets.lua P.encode/P.decode.
 auto bytes=readFile(path,4*1024*1024);require(!bytes.empty()&&bytes.back()=='\n',"Epic preset must end in a newline");
 for(uint8_t c:bytes)require(c==9||c==10||c==13||c>=32,"Invalid control character in Epic preset");
 std::string text(bytes.begin(),bytes.end());EpicPreset result;
 struct State{LutDocument doc;std::vector<bool> rows,cells;};std::vector<State> states;std::map<std::string,size_t> byId;
 auto index=[](const std::string& s){uint32_t n=0;auto parsed=std::from_chars(s.data(),s.data()+s.size(),n);require(!s.empty()&&parsed.ec==std::errc()&&parsed.ptr==s.data()+s.size(),"Invalid Epic preset integer");return n;};
 auto flag=[](const std::string& s){require(s=="0"||s=="1","Invalid Epic preset flag");return s=="1";};
 auto color=[](const std::string& s){return s.size()==7&&s[0]=='#'&&std::all_of(s.begin()+1,s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');});};
 auto number=[](const std::string& s){double value=0;auto parsed=std::from_chars(s.data(),s.data()+s.size(),value,std::chars_format::general);require(!s.empty()&&parsed.ec==std::errc()&&parsed.ptr==s.data()+s.size()&&std::isfinite(value),"Invalid Epic preset float");float f=float(value);require(std::isfinite(f),"Epic preset float32 overflow");return f;};
 size_t begin=0;unsigned lineNumber=0;
 while(begin<text.size()){
  auto end=text.find('\n',begin);require(end!=std::string::npos,"Truncated Epic preset line");auto line=text.substr(begin,end-begin);if(!line.empty()&&line.back()=='\r')line.pop_back();begin=end+1;++lineNumber;
  std::vector<std::string> f;size_t at=0;for(;;){auto stop=line.find('\t',at);f.push_back(line.substr(at,stop==std::string::npos?stop:stop-at));if(stop==std::string::npos)break;at=stop+1;}
  if(lineNumber==1){require(line!="DBF-ARMOR-LUT\t1","RGB-only Epic v1 requires the original game baseline; its missing channels cannot be reconstructed from this file");require(line=="EPIC-LUT\t2","Unsupported Epic preset format; choose an EPIC-LUT v2 .dbflut file");}
  else if(lineNumber==2){require(f.size()==3&&f[0]=="target"&&(f[1]=="armor"||f[1]=="helmet"||f[1]=="cape")&&f[2].size()==8,"Invalid Epic target identity");require(std::all_of(f[2].begin(),f[2].end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');}),"Invalid Epic target ID");result.targetKind=f[1];result.targetId=f[2];}
  else if(lineNumber==3){require(f.size()==2&&f[0]=="body"&&!f[1].empty()&&f[1].size()<=128,"Invalid Epic body identity");result.body=f[1];}
  else if(lineNumber==4){require(f.size()==2&&f[0]=="enabled","Missing Epic enabled flag");result.enabled=flag(f[1]);}
  else if(lineNumber==5){require(f.size()==2&&f[0]=="effects","Missing Epic effects flag");result.preserveEffects=flag(f[1]);}
  else if(f[0]=="lut"){
   require(f.size()==4&&validId(f[1])&&!byId.count(f[1]),"Unknown or duplicate Epic LUT identity");uint32_t w=index(f[2]),h=index(f[3]);
   if(!(((w==23||w==16)&&h>=1&&h<=64)||(w==3&&h==1)))throw std::runtime_error("Epic LUT "+f[1]+" has unsupported dimensions "+std::to_string(w)+"x"+std::to_string(h)+"; supported:23x1..64,16x1..64 or3x1");
   require(states.size()<4096,"Too many Epic LUTs");State s;s.doc.width=w;s.doc.height=h;s.doc.resourceId=f[1];s.doc.sourcePath=path;s.doc.label=wide(result.targetKind+" "+result.targetId+" / "+f[1]);s.doc.pixels.resize(size_t(w)*h);s.rows.resize(h);s.cells.resize(size_t(w)*h);byId[f[1]]=states.size();states.push_back(std::move(s));
  }else if(f[0]=="row"){
   require(f.size()==5&&byId.count(f[1]),"Invalid Epic row identity");auto& s=states[byId[f[1]]];uint32_t row=index(f[2]);require(row>=1&&row<=s.doc.height&&f[2]==std::to_string(row)&&!s.rows[row-1]&&color(f[4]),"Invalid or duplicate Epic row");s.rows[row-1]=true;result.rowSettings.push_back({f[1],row,flag(f[3]),f[4]});
  }else if(f[0]=="float"){
   require(f.size()==8&&byId.count(f[1]),"Invalid Epic float identity");auto& s=states[byId[f[1]]];uint32_t row=index(f[2]),col=index(f[3]);require(row>=1&&row<=s.doc.height&&col>=1&&col<=s.doc.width,"Epic float cell outside LUT dimensions");size_t cell=size_t(row-1)*s.doc.width+col-1;require(!s.cells[cell],"Duplicate Epic float cell");s.cells[cell]=true;for(size_t channel=0;channel<4;++channel)s.doc.pixels[cell][channel]=number(f[4+channel]);
  }else fail("Unknown Epic preset record");
 }
 require(lineNumber>=5&&!states.empty(),"Incomplete Epic preset");
 for(auto& s:states){require(std::all_of(s.rows.begin(),s.rows.end(),[](bool v){return v;})&&std::all_of(s.cells.begin(),s.cells.end(),[](bool v){return v;}),"Epic preset is missing row settings or RGBA cells");s.doc.original=s.doc.pixels;result.documents.push_back(std::move(s.doc));}
 return result;
}
} // omega
