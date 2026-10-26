#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <memory>
#include <cwctype>
#include <commdlg.h>
#include <dwmapi.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include "patch_install.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include "lut_io.h"
#include "game_assets.h"
#include "steam_location.h"
#include "material_preview.h"
#include "item_preview.h"
#include "ui_viewport.h"
#include "fullscreen_bars.h"
#include "inline_color_picker.h"
using namespace omega;
namespace fs=std::filesystem;
HWND win, edits[4], rowBox,colBox, list, searchBox, statusBox, categoryBox, presetBox, basicBox, ownerBox, previewItemBox; HFONT font; HBRUSH bg=CreateSolidBrush(RGB(21,23,29)), field=CreateSolidBrush(RGB(33,36,44));
UiViewport vp; LutDocument doc; std::map<std::string,LutDocument> staged; std::vector<LutDocument> imported; int activeImported=-1; size_t row=0,col=0; bool original=false,checker=true,syncing=false,advanced=false,firstScan=true; float rotation=.4f; std::vector<std::vector<Pixel>> undoStack,redoStack; std::vector<TextureRecord> records; std::vector<size_t> filtered; std::atomic<bool> cancelScan=false,busy=false; std::thread worker; fs::path base; std::wstring status=L"Ready. Open DDS or scan installed game archives.";
ItemPreviewModel itemPreview;
std::vector<std::wstring> previewOwners;
size_t previewOwner=0;
std::wstring gameDataPath;
bool individualPiecePreview=false;
struct PreviewJob{std::string resource;size_t owner=0;uint64_t generation=0;std::wstring preferredOwner;int64_t restorePart=-1;std::wstring dataFolder;bool singlePiece=false;};
struct PreviewResult{PreviewJob job;ItemPreviewModel model;std::vector<std::wstring> owners;size_t actualOwner=0;std::string error;};
std::thread previewWorker;std::mutex previewMutex;std::condition_variable previewWake;
PreviewJob previewPending;bool previewHasPending=false,previewStopping=false;uint64_t previewGeneration=0;
float previewPitch=.12f,previewZoom=1.f;
bool previewFullscreen=false,previewDragging=false,previewShading=true,previewMoved=false;
int selectedPart=-1;
InlinePickerColor inlinePicker;int inlinePickerDrag=0;bool inlinePickerUndo=false,inlinePickerFocused=false;
void endInlineColorGesture();
POINT previewLastMouse{},previewPressMouse{};
WINDOWPLACEMENT previewSavedPlacement{sizeof(WINDOWPLACEMENT)};
LONG_PTR previewSavedStyle=0;
std::array<FullscreenBar,6> fullscreenBars=defaultFullscreenBars();int draggingBar=-1;POINT barPress{};int barStartX=0,barStartY=0;
void loadToolbarPositions(){
 std::ifstream in(base/L"lut-editor-cache"/L"toolbar-layout.ini");std::string tag;int version;if(!(in>>tag>>version)||tag!="OMEGA_TOOLBARS"||version!=1)return;auto restored=defaultFullscreenBars();bool seen[6]{};for(size_t n=0;n<restored.size();++n){int id,x,y;if(!(in>>id>>x>>y)||id<0||id>=int(restored.size())||seen[id])return;seen[id]=true;moveFullscreenBar(restored[id],x,y);}fullscreenBars=restored;
}
void saveToolbarPositions(){try{auto dir=base/L"lut-editor-cache";fs::create_directories(dir);auto target=dir/L"toolbar-layout.ini",temporary=dir/L"toolbar-layout.ini.tmp";{std::ofstream out(temporary,std::ios::trunc);out<<"OMEGA_TOOLBARS 1\n";for(size_t i=0;i<fullscreenBars.size();++i)out<<i<<" "<<fullscreenBars[i].x<<" "<<fullscreenBars[i].y<<"\n";out.flush();if(!out)throw std::runtime_error("Cannot write toolbar positions");}if(!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot save toolbar positions");}catch(const std::exception&e){try{fs::create_directories(base/L"Logs");std::ofstream out(base/L"Logs/errors.log",std::ios::app);out<<"Toolbar layout: "<<e.what()<<"\n";}catch(...){}}}
bool overFullscreenPanel(POINT p){if(!previewFullscreen)return false;double x=vp.logicalX(p.x),y=vp.logicalY(p.y);for(auto&b:fullscreenBars)if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return true;return false;}
RECT previewBounds(){RECT r{};GetClientRect(win,&r);if(previewFullscreen)return RECT{vp.x(16),vp.y(136),vp.x(1784),vp.y(720)};return RECT{vp.x(970),vp.y(175),vp.x(1778),vp.y(695)};}
void logPreviewFailure(const std::exception& e){try{fs::create_directories(base/L"Logs");std::ofstream log(base/L"Logs/errors.log",std::ios::app);log<<"Item preview "<<doc.resourceId<<": "<<e.what()<<"\n";}catch(...){}}
void queueItemPreview(size_t owner,const std::wstring& preferredOwner={},int64_t restorePart=-1){
 if(doc.resourceId.empty()||busy)return;
 PreviewJob job{doc.resourceId,owner,++previewGeneration,preferredOwner,restorePart};job.dataFolder=gameDataPath;job.singlePiece=individualPiecePreview;
 itemPreview={};selectedPart=-1;itemPreview.notice=L"Loading item model…";InvalidateRect(win,0,FALSE);
 {std::lock_guard<std::mutex> lock(previewMutex);previewPending=job;previewHasPending=true;}
 if(!previewWorker.joinable())previewWorker=std::thread([]{
  for(;;){PreviewJob job;{std::unique_lock<std::mutex> lock(previewMutex);previewWake.wait(lock,[]{return previewStopping||previewHasPending;});if(previewStopping)return;job=previewPending;previewHasPending=false;}
   auto result=std::make_unique<PreviewResult>();result->job=job;
   try{result->owners=gameItemPreviewOwners(job.resource);result->actualOwner=job.owner;if(!job.preferredOwner.empty()){auto found=std::find(result->owners.begin(),result->owners.end(),job.preferredOwner);if(found!=result->owners.end())result->actualOwner=(size_t)(found-result->owners.begin());}result->model=loadGameItemPreview(job.dataFolder,job.resource,result->actualOwner,job.singlePiece);}
   catch(const std::exception&e){result->error=e.what();}
   {std::lock_guard<std::mutex> lock(previewMutex);if(previewStopping)return;}
   if(PostMessage(win,WM_APP+5,0,(LPARAM)result.get()))result.release();
  }
 });
 previewWake.notify_one();
}
void refreshItemPreview(const std::wstring& preferredOwner={},int64_t restorePart=-1){
 ++previewGeneration;itemPreview={};selectedPart=-1;previewOwners.clear();previewOwner=0;SendMessage(ownerBox,CB_RESETCONTENT,0,0);EnableWindow(ownerBox,FALSE);
 if(doc.resourceId.empty()){itemPreview.notice=L"No linked game item. Select an item from the game list.";InvalidateRect(win,0,FALSE);return;}
 if(busy){itemPreview.notice=L"Waiting for the game asset scan…";InvalidateRect(win,0,FALSE);return;}
 queueItemPreview(0,preferredOwner,restorePart);
}
bool selectedPartEditable(){return selectedPart<0||(size_t(selectedPart)<itemPreview.parts.size()&&!itemPreview.parts[selectedPart].lutResourceId.empty()&&itemPreview.parts[selectedPart].lutResourceId==doc.resourceId);}
void updatePartControls(){bool enabled=selectedPartEditable()&&!doc.pixels.empty();for(int id:{14,15,16,30,31,32,33})EnableWindow(GetDlgItem(win,id),enabled);}
void layout();
void fitFullscreenMonitor(HWND h){MONITORINFO mi{sizeof(mi)};if(GetMonitorInfo(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&mi))SetWindowPos(h,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOOWNERZORDER);}
void togglePreviewFullscreen(){
 if(draggingBar>=0)saveToolbarPositions();draggingBar=-1;previewDragging=false;ReleaseCapture();
 if(!previewFullscreen){previewSavedPlacement.length=sizeof(previewSavedPlacement);GetWindowPlacement(win,&previewSavedPlacement);previewSavedStyle=GetWindowLongPtr(win,GWL_STYLE);previewFullscreen=true;MONITORINFO mi{sizeof(mi)};GetMonitorInfo(MonitorFromWindow(win,MONITOR_DEFAULTTONEAREST),&mi);SetWindowLongPtr(win,GWL_STYLE,previewSavedStyle&~(WS_THICKFRAME|WS_MAXIMIZEBOX|WS_MINIMIZEBOX));SetWindowPos(win,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOOWNERZORDER);}
 else{previewFullscreen=false;SetWindowLongPtr(win,GWL_STYLE,previewSavedStyle);SetWindowPlacement(win,&previewSavedPlacement);SetWindowPos(win,0,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);}
 layout();SetFocus(win);
}
struct PaintBuffer {
 HDC dc=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ previousBitmap=nullptr;int width=0,height=0;
 void clear(){if(dc&&previousBitmap)SelectObject(dc,previousBitmap);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);dc=nullptr;bitmap=nullptr;previousBitmap=nullptr;width=height=0;}
 HDC acquire(HDC target,int w,int h){w=std::max(1,w);h=std::max(1,h);if(dc&&w==width&&h==height)return dc;clear();dc=CreateCompatibleDC(target);if(!dc)return target;bitmap=CreateCompatibleBitmap(target,w,h);if(!bitmap){clear();return target;}previousBitmap=SelectObject(dc,bitmap);width=w;height=h;return dc;}
 ~PaintBuffer(){clear();}
} paintBuffer;
void paintCheckerBackground(HDC dc,const RECT&area){
 int saved=SaveDC(dc);IntersectClipRect(dc,area.left,area.top,area.right,area.bottom);const int tile=std::max(8,vp.length(24));HBRUSH colors[2]={CreateSolidBrush(RGB(31,34,44)),CreateSolidBrush(RGB(40,44,55))};int left=area.left-area.left%tile,top=area.top-area.top%tile;for(int y=top;y<area.bottom;y+=tile)for(int x=left;x<area.right;x+=tile){RECT square{x,y,x+tile,y+tile};FillRect(dc,&square,colors[((x/tile)+(y/tile))&1]);}DeleteObject(colors[0]);DeleteObject(colors[1]);RestoreDC(dc,saved);
}
void paintCanvasBackground(HDC dc,const RECT&client){if(previewFullscreen&&checker)paintCheckerBackground(dc,client);else FillRect(dc,&client,bg);}

void scan();
bool ensureGameLocation();
void changeGameLocation();
struct Control{HWND hwnd; int x,y,w,h;int placedX=std::numeric_limits<int>::min(),placedY=std::numeric_limits<int>::min(),placedW=0,placedH=0;};std::vector<Control> controls;
std::wstring widen(const std::string&s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),0,0);std::wstring v(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),v.data(),n);return v;}
void say(std::wstring s){status=std::move(s);SetWindowText(statusBox,status.c_str());InvalidateRect(win,nullptr,FALSE);}
void error(const std::exception&e){try{fs::create_directories(base/L"Logs");std::ofstream f(base/L"Logs/errors.log",std::ios::app);SYSTEMTIME now;GetLocalTime(&now);f<<now.wYear<<"-"<<now.wMonth<<"-"<<now.wDay<<" "<<now.wHour<<":"<<now.wMinute<<":"<<now.wSecond<<" "<<e.what()<<"\n";}catch(...){}say(L"Failed: "+widen(e.what()));MessageBox(win,widen(e.what()).c_str(),L"OMEGA LUT — error",MB_ICONERROR);}
const wchar_t* buttonTip(int id){switch(id){case 4:return L"Save OMEGA preset";case 7:return L"Reset this item to its loaded original values";case 8:return L"Undo";case 9:return L"Redo";case 10:return L"Compare original and edited appearance";case 14:return L"Apply RGBA values";case 15:return L"Use the inline color square and hue strip. Alpha remains unchanged.";case 16:return L"Import replacement DDS into selected LUT";case 17:return L"Selected asset details and shared owners";case 18:return L"Select Helldivers 2 folder";case 47:return L"Full-screen preview. Esc returns.";case 49:return L"Toggle preview shading";default:return nullptr;}}
HWND tooltips=nullptr;
bool drawActionIcon(DRAWITEMSTRUCT*d){if(!previewFullscreen||!buttonTip(d->CtlID))return false;RECT r=d->rcItem;int x=(r.left+r.right)/2,y=(r.top+r.bottom)/2,q=std::max(5,int(std::min(r.right-r.left,r.bottom-r.top)*.26f));auto pen=CreatePen(PS_SOLID,std::max(2,q/6),(d->itemState&ODS_DISABLED)?RGB(101,109,125):RGB(230,234,244));auto old=SelectObject(d->hDC,pen),brush=SelectObject(d->hDC,GetStockObject(NULL_BRUSH));auto line=[&](int a,int b,int c,int e){MoveToEx(d->hDC,a,b,nullptr);LineTo(d->hDC,c,e);};switch(d->CtlID){
case 8:case 9:{int dir=d->CtlID==8?-1:1;Arc(d->hDC,x-q,y-q,x+q,y+q,x+dir*q,y,x,y+q);line(x+dir*q,y,x+dir*q,y-q);line(x+dir*q,y,x,y);break;}
case 4:Rectangle(d->hDC,x-q,y-q,x+q,y+q);Rectangle(d->hDC,x-q/2,y-q,x+q/2,y);line(x-q/2,y+q,x-q/2,y+q/3);line(x-q/2,y+q/3,x+q/2,y+q/3);break;
case 7:Arc(d->hDC,x-q,y-q,x+q,y+q,x,y-q,x-q,y);line(x,y-q,x+q,y-q);line(x+q,y-q,x+q,y);break;
case 14:line(x-q,y,x-q/4,y+q);line(x-q/4,y+q,x+q,y-q);break;
case 15:Ellipse(d->hDC,x-q,y-q,x+q,y+q);line(x,y-q,x,y+q);line(x-q,y,x+q,y);break;
case 16:case 18:Rectangle(d->hDC,x-q,y-q/2,x+q,y+q);line(x-q,y-q/2,x-q/3,y-q);line(x-q/3,y-q,x+q/2,y-q/2);line(x,y+q/2,x,y-q/3);break;
case 17:Ellipse(d->hDC,x-q,y-q,x+q,y+q);line(x,y,x,y+q/2);line(x,y-q/2,x,y-q/2+2);break;
case 10:Rectangle(d->hDC,x-q,y-q,x+q,y+q);line(x,y-q,x,y+q);break;
case 49:Ellipse(d->hDC,x-q,y-q,x+q,y+q);for(int i=-q+2;i<0;i+=3)line(x+i,y-q/2,x+i,y+q/2);break;
case 47:for(int dx:{-1,1})for(int dy:{-1,1}){line(x+dx*q,y+dy*q,x+dx*(q/3),y+dy*q);line(x+dx*q,y+dy*q,x+dx*q,y+dy*(q/3));}break;
}SelectObject(d->hDC,brush);SelectObject(d->hDC,old);DeleteObject(pen);return true;}
HWND control(const wchar_t*cls,const wchar_t*text,int id,int x,int y,int w,int h,DWORD extra=0){HWND c=CreateWindowEx(0,cls,text,WS_CHILD|WS_VISIBLE|extra,x,y,w,h,win,(HMENU)(INT_PTR)id,GetModuleHandle(0),0);controls.push_back({c,x,y,w,h});if(auto tip=buttonTip(id)){if(!tooltips){tooltips=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,win,0,GetModuleHandle(0),0);SendMessage(tooltips,TTM_SETMAXTIPWIDTH,0,480);SendMessage(tooltips,TTM_SETDELAYTIME,TTDT_INITIAL,350);}TOOLINFO ti{sizeof(ti)};ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS;ti.hwnd=win;ti.uId=(UINT_PTR)c;ti.lpszText=const_cast<wchar_t*>(tip);SendMessage(tooltips,TTM_ADDTOOL,0,(LPARAM)&ti);}return c;}
void layout(){
 RECT r;GetClientRect(win,&r);vp.resize(r.right,r.bottom,1800,1000);
 static int fontHeight=0;const int wantedFontHeight=std::max(12,vp.length(16));const bool fontChanged=!font||fontHeight!=wantedFontHeight;HFONT previous=fontChanged?font:nullptr;if(fontChanged){fontHeight=wantedFontHeight;font=CreateFont(-fontHeight,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");}
 HDWP moves=BeginDeferWindowPos((int)controls.size());struct Move{HWND h;int x,y,w,height;};std::vector<Move> moved;
 auto place=[&](Control& c,int x,int y,int w,int h){if(c.placedX==x&&c.placedY==y&&c.placedW==w&&c.placedH==h)return;c.placedX=x;c.placedY=y;c.placedW=w;c.placedH=h;moved.push_back({c.hwnd,x,y,w,h});if(moves)moves=DeferWindowPos(moves,c.hwnd,nullptr,x,y,w,h,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOREDRAW);};
 for(auto&c:controls){int id=GetDlgCtrlID(c.hwnd),x=c.x,y=c.y,w=c.w,h=c.h;bool visible=true;
  if(id==90)x=1640;if(id==91)x=1686;if(id==92)x=1732;
  if(id==50){x=22;y=963;w=1750;h=28;}
  if(id==42){x=270;y=advanced?755:94;w=680;h=220;}
  if(id==40){x=270;y=advanced?798:138;w=680;h=32;}
  if(id==41){x=270;y=advanced?842:184;w=680;h=advanced?94:752;}
  if(id==20){x=970;y=advanced?94:94;w=300;h=400;}
  if(id==21){x=270;y=94;w=680;h=500;visible=advanced;}
  if(id==43){x=22;y=906;w=208;h=300;visible=!imported.empty();}
  if(id==44){x=970;y=766;w=500;h=300;visible=!advanced;}
  if(id==45){x=970;y=898;w=394;h=40;}
  if(id==46){x=1384;y=898;w=394;h=40;}
  if(id==47){x=1728;y=184;w=40;h=34;}
  if(id==48){x=1290;y=94;w=488;h=400;visible=advanced;}
  if(id==49){x=1548;y=766;w=230;h=36;}
  if(id==14){x=270;y=621;w=208;h=38;}
  if(id==15){x=advanced?496:970;y=advanced?621:814;w=advanced?208:500;h=38;}
  if(id>=30&&id<=33){x=270+(id-30)*173;y=574;w=161;h=34;}
  if(!advanced){switch(id){case 1:case 5:case 6:case 11:case 14:case 16:case 17:case 30:case 31:case 32:case 33:visible=false;break;
   case 2:y=94;break;case 3:y=142;break;case 4:y=190;break;case 7:y=310;break;case 8:y=358;break;case 9:y=406;break;case 10:y=478;break;case 12:y=526;break;case 19:y=598;break;case 13:y=646;break;}}
  else{if(id==19)y=676;if(id==16)y=750;if(id==17)y=800;}
  if(previewFullscreen){int bar=-1,px=0,py=0,pw=0,ph=0;switch(id){
case 42:bar=0;px=16;py=34;pw=180;ph=320;break;
case 60:bar=0;px=212;py=34;pw=932;ph=500;break;
case 48:if(!advanced)break;bar=0;px=16;py=76;pw=340;ph=400;break;
case 40:bar=0;px=advanced?372:16;py=76;pw=advanced?772:1128;ph=32;break;
case 10:bar=1;px=16;py=34;pw=42;ph=36;break;
case 49:bar=1;px=74;py=34;pw=42;ph=36;break;
case 47:bar=1;px=132;py=34;pw=42;ph=36;break;
case 20:bar=2;px=16;py=50;pw=210;ph=420;break;
case 44:bar=2;px=244;py=50;pw=330;ph=420;break;
case 21:bar=2;px=590;py=50;pw=514;ph=500;break;
case 14:bar=3;px=640;py=58;pw=42;ph=36;break;
case 15:bar=3;px=702;py=58;pw=42;ph=36;break;
case 8:bar=4;px=16;py=48;pw=42;ph=36;break;
case 9:bar=4;px=78;py=48;pw=42;ph=36;break;
case 7:bar=4;px=140;py=48;pw=42;ph=36;break;
case 16:bar=5;px=16;py=48;pw=42;ph=36;break;
case 17:bar=5;px=78;py=48;pw=42;ph=36;break;
case 18:bar=5;px=202;py=48;pw=42;ph=36;break;
case 4:bar=5;px=140;py=48;pw=42;ph=36;break;
case 30:bar=3;px=16;py=58;pw=134;ph=36;break;
case 31:bar=3;px=166;py=58;pw=134;ph=36;break;
case 32:bar=3;px=316;py=58;pw=134;ph=36;break;
case 33:bar=3;px=466;py=58;pw=134;ph=36;break;
default:break;}visible=bar>=0;if(visible){auto&b=fullscreenBars[bar];place(c,vp.x(b.x+px),vp.y(b.y+py),vp.length(pw),vp.length(ph));}}
  else{if(id==60)visible=false;place(c,vp.x(x),vp.y(y),vp.length(w),vp.length(h));}
  if(((GetWindowLongPtr(c.hwnd,GWL_STYLE)&WS_VISIBLE)!=0)!=visible)ShowWindow(c.hwnd,visible?SW_SHOW:SW_HIDE);if(fontChanged)SendMessage(c.hwnd,WM_SETFONT,(WPARAM)font,FALSE);
 }
 bool positioned=moves&&EndDeferWindowPos(moves);if(!positioned)for(const auto& m:moved)SetWindowPos(m.h,nullptr,m.x,m.y,m.w,m.height,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOREDRAW);
 if(previous)DeleteObject(previous);
 SetWindowText(GetDlgItem(win,3),advanced?L"Load preset (.olut / Epic)…":L"Load preset…");SetWindowText(GetDlgItem(win,7),advanced?L"Reset LUT values":L"Reset this item");SetWindowText(GetDlgItem(win,15),advanced?L"Choose RGB…":L"Change color…");SetWindowText(GetDlgItem(win,2),busy?L"Cancel item scan":L"Refresh items");SetWindowText(GetDlgItem(win,19),advanced?L"< Back to simple editor":L"Advanced >");SetWindowText(GetDlgItem(win,47),previewFullscreen?L"↙":L"⛶");SetWindowText(GetDlgItem(win,49),previewShading?L"Shading: on":L"Shading: off");updatePartControls();RedrawWindow(win,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_NOERASE);
}
const wchar_t* columnName(size_t c){if(doc.width==16)return L"Cape LUT channel";static const wchar_t* pattern[]={L"Pattern color",L"Pattern material / opacity",L"Unconfirmed pattern effects"};return doc.width==3&&c<3?pattern[c]:lutColumnName(c);}const wchar_t* columnHelp(size_t c){if(doc.width==16)return L"Source cape LUT RGBA. Cape shader channel semantics are not verified; values are preserved exactly.";if(doc.width!=3)return lutColumnHelp(c);return c==0?L"Pattern color RGB. Preserve alpha; its complete engine behavior is not verified.":c==1?L"Pattern material RGB / opacity A. The sample uses an approximation; exact source values are preserved.":L"Unconfirmed pattern effects. Raw RGBA is editable; no invented game behavior is claimed.";}void sync(){syncing=true;SendMessage(rowBox,CB_RESETCONTENT,0,0);for(uint32_t y=0;y<doc.height;y++){auto s=L"Material row "+std::to_wstring(y+1);SendMessage(rowBox,CB_ADDSTRING,0,(LPARAM)s.c_str());}SendMessage(rowBox,CB_SETCURSEL,row,0);SendMessage(colBox,CB_RESETCONTENT,0,0);for(uint32_t x=0;x<doc.width;x++){auto s=std::to_wstring(x+1)+L" · "+columnName(x);SendMessage(colBox,CB_ADDSTRING,0,(LPARAM)s.c_str());}SendMessage(colBox,CB_SETCURSEL,col,0);if(!doc.pixels.empty())for(int k=0;k<4;k++){std::wostringstream s;s<<std::setprecision(9)<<doc.at(col,row)[k];SetWindowText(edits[k],s.str().c_str());}SendMessage(basicBox,CB_RESETCONTENT,0,0);const size_t choices[]={0,2,5,6,16,17,18,19};for(size_t i=0;i<8;i++){if(choices[i]>=doc.width||(doc.width==3&&i>0))continue;SendMessage(basicBox,CB_ADDSTRING,0,(LPARAM)columnName(choices[i]));}int basicIndex=0;for(int i=0;i<8;i++)if(choices[i]==col)basicIndex=i;SendMessage(basicBox,CB_SETCURSEL,basicIndex,0);InvalidateRect(win,0,FALSE);syncing=false;}
void stashCurrent(){if(!doc.resourceId.empty()&&!doc.archiveId.empty()){if(doc.changed())staged[doc.resourceId]=doc;else staged.erase(doc.resourceId);}}
std::vector<std::wstring> filteredOwners;
std::vector<bool> filteredSinglePieces;
void setDoc(LutDocument d,bool restoreStaged=false,const std::wstring& previewKeepOwner={},int64_t previewKeepPart=-1){endInlineColorGesture();stashCurrent();auto saved=staged.find(d.resourceId);if(restoreStaged&&saved!=staged.end())d=saved->second;if(activeImported>=0&&size_t(activeImported)<imported.size())imported[activeImported]=doc;activeImported=-1;doc=std::move(d);row=col=0;undoStack.clear();redoStack.clear();sync();refreshItemPreview(previewKeepOwner,previewKeepPart);SendMessage(previewItemBox,CB_SETCURSEL,-1,0);for(size_t i=0;i<filtered.size();++i)if(records[filtered[i]].resourceId==doc.resourceId&&(previewKeepOwner.empty()||filteredOwners[i]==previewKeepOwner)&&filteredSinglePieces[i]==individualPiecePreview){SendMessage(previewItemBox,CB_SETCURSEL,i,0);SendMessage(list,LB_SETCURSEL,i,0);break;}say(L"Loaded "+doc.label+L" · "+std::to_wstring(doc.width)+L" × "+std::to_wstring(doc.height));}
void selectImported(size_t n){if(n>=imported.size()||int(n)==activeImported)return;auto next=imported[n];setDoc(std::move(next));activeImported=(int)n;SendMessage(presetBox,CB_SETCURSEL,n,0);}void importEpic(const std::wstring& path){if(!ensureGameLocation())return;auto preset=loadEpicPreset(path);auto current=scanGameLuts(gameDataPath);size_t rebound=0;for(auto& d:preset.documents){auto it=std::find_if(current.begin(),current.end(),[&](const TextureRecord&r){return r.resourceId==d.resourceId&&r.width==d.width&&r.height==d.height;});if(it!=current.end()){auto now=loadGameLut(*it);now.pixels=d.pixels;d=std::move(now);rebound++;}}activeImported=-1;imported=std::move(preset.documents);SendMessage(presetBox,CB_RESETCONTENT,0,0);for(size_t i=0;i<imported.size();i++){auto label=L"Epic LUT "+std::to_wstring(i+1)+L" · "+widen(imported[i].resourceId);SendMessage(presetBox,CB_ADDSTRING,0,(LPARAM)label.c_str());}if(!imported.empty())selectImported(0);for(const auto& d:imported)if(!d.archiveId.empty()&&d.changed())staged[d.resourceId]=d;layout();auto report=L"Loaded "+std::to_wstring(imported.size())+L" LUTs. "+std::to_wstring(rebound)+L" matched current game targets.\n\nUse the Epic LUT selector to switch tables. Save/export operates on the selected LUT.\n\nUnmatched targets remain detached and cannot export a game patch. Live equipment/body targeting and startup enable flags do not apply offline; only serialized LUT values are applied.\n\nSource target: "+widen(preset.targetKind)+L" / "+widen(preset.targetId);MessageBox(win,report.c_str(),L"Epic preset import report",MB_OK);}void remember(){undoStack.push_back(doc.pixels);if(undoStack.size()>100)undoStack.erase(undoStack.begin());redoStack.clear();}
bool inlineColorEnabled(){return selectedPartEditable()&&col<doc.width&&row<doc.height&&row*doc.width+col<doc.pixels.size();}
void inlineColorRects(RECT& sv,RECT& hue){
 if(previewFullscreen){auto&b=fullscreenBars[3];sv={vp.x(b.x+786),vp.y(b.y+34),vp.x(b.x+956),vp.y(b.y+90)};hue={sv.left,vp.y(b.y+98),sv.right,vp.y(b.y+114)};}
 else{sv={vp.x(1505),vp.y(810),vp.x(1778),vp.y(866)};hue={sv.left,vp.y(874),sv.right,vp.y(890)};}
}
void readInlineColor(){if(inlineColorEnabled()){auto&p=doc.at(col,row);pickerReadRgb(inlinePicker,p[0],p[1],p[2]);}}
void endInlineColorGesture(){if(inlinePickerDrag){inlinePickerDrag=0;inlinePickerUndo=false;if(GetCapture()==win)ReleaseCapture();}}
void applyInlineColor(POINT p){
 if(!inlinePickerDrag||!inlineColorEnabled())return;RECT sv,hue;inlineColorRects(sv,hue);
 if(inlinePickerDrag==1){inlinePicker.saturation=pickerUnit(float(p.x-sv.left)/std::max(1L,sv.right-sv.left-1));inlinePicker.value=1-pickerUnit(float(p.y-sv.top)/std::max(1L,sv.bottom-sv.top-1));}
 else inlinePicker.hue=pickerUnit(float(p.x-hue.left)/std::max(1L,hue.right-hue.left-1));
 const auto rgb=pickerRgb(inlinePicker);auto& value=doc.at(col,row);
 if(value[0]!=rgb[0]||value[1]!=rgb[1]||value[2]!=rgb[2]){
  if(!inlinePickerUndo){remember();inlinePickerUndo=true;}
  value[0]=rgb[0];value[1]=rgb[1];value[2]=rgb[2]; // Keep alpha/mode bit-for-bit.
  syncing=true;for(int k=0;k<4;++k){std::wostringstream number;number<<std::setprecision(9)<<value[k];SetWindowText(edits[k],number.str().c_str());}syncing=false;
 }
 InvalidateRect(win,nullptr,FALSE);
}
bool beginInlineColor(POINT p){
 RECT sv,hue;inlineColorRects(sv,hue);const bool inSquare=PtInRect(&sv,p)!=FALSE,inHue=PtInRect(&hue,p)!=FALSE;if(!inSquare&&!inHue)return false;
 if(previewFullscreen){const double x=vp.logicalX(p.x),y=vp.logicalY(p.y);for(int i=int(fullscreenBars.size())-1;i>=0;--i){auto&b=fullscreenBars[i];if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h){if(i!=3)return false;break;}}}
 if(!inlineColorEnabled())return true;readInlineColor();inlinePickerUndo=false;inlinePickerDrag=inSquare?1:2;inlinePickerFocused=true;SetFocus(win);SetCapture(win);applyInlineColor(p);return true;
}
void paintInlineColor(HDC dc){if(!inlinePickerDrag)readInlineColor();RECT sv,hue;inlineColorRects(sv,hue);drawInlineColorPicker(dc,sv,hue,inlinePicker,inlineColorEnabled(),inlinePickerFocused);}
std::wstring dialog(bool save,const wchar_t*filter,const wchar_t*ext){wchar_t p[32768]={};OPENFILENAME o={sizeof(o)};o.hwndOwner=win;o.lpstrFile=p;o.nMaxFile=32768;o.lpstrFilter=filter;o.lpstrDefExt=ext;o.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);return (save?GetSaveFileName(&o):GetOpenFileName(&o))?p:L"";}
std::wstring itemName(const TextureRecord&r){auto name=r.name;auto stop=name.find(L" / LUT ");if(stop!=std::wstring::npos)name.resize(stop);auto unit=name.find(L" / Unit ");if(unit!=std::wstring::npos)name.resize(unit);auto shared=name.find(L" | ");bool more=shared!=std::wstring::npos;if(more)name.resize(shared);if(name.rfind(L"content/",0)==0){auto slash=name.find_last_of(L'/');if(slash!=std::wstring::npos)name=name.substr(slash+1);bool cap=true;for(auto&c:name){if(c==L'_'){c=L' ';cap=true;}else if(cap){c=towupper(c);cap=false;}}}if(r.category==L"Unknown")name=L"Unknown item · "+widen(r.resourceId);if(more)name+=L" (shared texture)";return name;}
void filter(){wchar_t b[512];GetWindowText(searchBox,b,512);std::wstring q=b;for(auto&c:q)c=towlower(c);SendMessage(list,LB_RESETCONTENT,0,0);SendMessage(previewItemBox,CB_RESETCONTENT,0,0);filtered.clear();filteredOwners.clear();filteredSinglePieces.clear();int category=categoryBox?(int)SendMessage(categoryBox,CB_GETCURSEL,0,0):0;const wchar_t*cats[]={L"",L"Armor",L"Weapons",L"Vehicles",L"Unknown"};for(size_t i=0;i<records.size();i++){if(category>0&&category<5&&records[i].category.find(cats[category])==std::wstring::npos)continue;auto options=gameItemPreviewOwners(records[i].resourceId);options.erase(std::remove_if(options.begin(),options.end(),[](const std::wstring& n){return n.rfind(L"[Armor]",0)!=0&&n.rfind(L"[Cape]",0)!=0&&n.rfind(L"[Helmet]",0)!=0;}),options.end());if(options.empty())options.push_back(L"");std::sort(options.begin(),options.end());auto individualOptions=options;options.erase(std::unique(options.begin(),options.end(),[](const auto&a,const auto&b){return a.substr(0,a.find(L" / "))==b.substr(0,b.find(L" / "));}),options.end());const size_t assembledCount=options.size();if(advanced)for(const auto& option:individualOptions)if(option.rfind(L"[Armor]",0)==0)options.push_back(option);for(size_t optionIndex=0;optionIndex<options.size();++optionIndex){const auto& owner=options[optionIndex];bool singlePiece=optionIndex>=assembledCount;auto s=records[i].category+L" · "+(owner.empty()?itemName(records[i]):singlePiece?L"Individual piece · "+owner:owner.substr(0,owner.find(L" / ")))+(records[i].width==16?L" · Cape LUT":records[i].width==3?L" · Pattern":L" · Material");if(advanced)s+=L" · "+widen(records[i].resourceId);auto t=s+(owner.empty()?records[i].name:L"")+widen(records[i].resourceId);for(auto&c:t)c=towlower(c);if(t.find(q)==std::wstring::npos)continue;filtered.push_back(i);filteredOwners.push_back(owner);filteredSinglePieces.push_back(singlePiece);SendMessage(list,LB_ADDSTRING,0,(LPARAM)s.c_str());SendMessage(previewItemBox,CB_ADDSTRING,0,(LPARAM)s.c_str());if(records[i].resourceId==doc.resourceId)SendMessage(previewItemBox,CB_SETCURSEL,filtered.size()-1,0);}}if(filtered.empty())SendMessage(list,LB_ADDSTRING,0,(LPARAM)(busy?L"Loading items from the installed game…":L"No matching items. Clear search or choose All assets."));SendMessage(list,LB_SETHORIZONTALEXTENT,vp.length(6000),0);EnableWindow(previewItemBox,!filtered.empty()&&!busy);}
void saveGameLocation(){auto dir=base/L"lut-editor-cache";fs::create_directories(dir);std::ofstream out(dir/L"game-location.txt",std::ios::binary|std::ios::trunc);out<<"OMEGA_HD2_LOCATION 1\n"<<fs::path(gameDataPath).u8string()<<"\n";if(!out)throw std::runtime_error("Cannot save selected HD2 folder beside the EXE");}
void initializeGameLocation(){try{std::ifstream saved(base/L"lut-editor-cache"/L"game-location.txt",std::ios::binary);std::string header,path;if(std::getline(saved,header)&&header=="OMEGA_HD2_LOCATION 1"&&std::getline(saved,path))gameDataPath=validateHd2Folder(fs::u8path(path).wstring());}catch(...){gameDataPath.clear();}if(gameDataPath.empty())gameDataPath=findInstalledHd2().dataFolder;}

bool ensureGameLocation(){if(!gameDataPath.empty())return true;auto found=findInstalledHd2();gameDataPath=found.dataFolder;if(gameDataPath.empty()){say(L"HD2 was not found in Steam. Select the Helldivers 2 folder, or cancel and use Select HD2 folder later.");gameDataPath=chooseHd2Folder(win);if(gameDataPath.empty())return false;saveGameLocation();}return true;}
void changeGameLocation(){
 if(doc.changed()||!staged.empty())throw std::runtime_error("Save your presets and reset staged changes before switching game installations.");auto folder=chooseHd2Folder(win,gameDataPath.empty()?L"":fs::path(gameDataPath).parent_path().wstring());if(folder.empty()||folder==gameDataPath)return;
 cancelScan=true;if(worker.joinable())worker.join();busy=false;{std::lock_guard<std::mutex> lock(previewMutex);previewStopping=true;previewHasPending=false;}previewWake.notify_all();if(previewWorker.joinable())previewWorker.join();previewStopping=false;++previewGeneration;MSG pending;while(PeekMessage(&pending,win,WM_APP+1,WM_APP+1,PM_REMOVE))delete (std::vector<TextureRecord>*)pending.lParam;while(PeekMessage(&pending,win,WM_APP+3,WM_APP+3,PM_REMOVE))delete (std::string*)pending.lParam;while(PeekMessage(&pending,win,WM_APP+5,WM_APP+5,PM_REMOVE))delete (PreviewResult*)pending.lParam;
 clearGameAssetCache();gameDataPath=folder;saveGameLocation();records.clear();filtered.clear();itemPreview={};selectedPart=-1;previewOwners.clear();doc.resourceId.clear();doc.archiveId.clear();firstScan=true;SendMessage(list,LB_RESETCONTENT,0,0);SendMessage(previewItemBox,CB_RESETCONTENT,0,0);say(L"Using HD2 data: "+gameDataPath);scan();
}
void scan(){if(!ensureGameLocation())return;if(busy){cancelScan=true;say(L"Cancelling scan…");return;}if(worker.joinable())worker.join();busy=true;cancelScan=false;++previewGeneration;{std::lock_guard<std::mutex> lock(previewMutex);previewHasPending=false;}EnableWindow(GetDlgItem(win,45),FALSE);EnableWindow(GetDlgItem(win,46),FALSE);SetWindowText(GetDlgItem(win,2),L"Cancel item scan");say(L"Scanning installed game files… (Scan again to cancel)");worker=std::thread([folder=gameDataPath]{try{auto result=new std::vector<TextureRecord>(scanGameLuts(folder,[](size_t n,size_t total,const std::wstring&){if(n%20==0)PostMessage(win,WM_APP+2,(WPARAM)n,(LPARAM)total);return !cancelScan.load();}));PostMessage(win,WM_APP+1,0,(LPARAM)result);}catch(const std::exception&e){auto s=new std::string(e.what());PostMessage(win,WM_APP+3,0,(LPARAM)s);}busy=false;});}
void text(HDC dc,int x,int y,int w,int h,const std::wstring&s,COLORREF color=RGB(221,225,233)){SetTextColor(dc,color);RECT r={vp.x(x),vp.y(y),vp.x(x+w),vp.y(y+h)};DrawText(dc,s.c_str(),-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){try{switch(m){case WM_CREATE:{win=h;control(L"BUTTON",L"—",90,1240,8,42,32,BS_OWNERDRAW);control(L"BUTTON",L"□",91,1286,8,42,32,BS_OWNERDRAW);control(L"BUTTON",L"×",92,1332,8,48,32,BS_OWNERDRAW);
int y=90;auto button=[&](const wchar_t*s,int id){control(L"BUTTON",s,id,22,y,208,36,BS_OWNERDRAW);y+=44;};button(L"Open DDS…",1);button(L"Scan game / Cancel",2);button(L"Load preset (.olut / Epic)…",3);button(L"Save OMEGA preset…",4);button(L"Export DDS…",5);button(L"Export game patch…",6);y+=12;button(L"Reset all values",7);button(L"Undo",8);button(L"Redo",9);button(L"Original / Edited",10);button(L"Checker background",11);button(L"Reset preview view",12);button(L"About / source credits",13);
categoryBox=control(WC_COMBOBOX,L"",42,22,700,208,220,CBS_DROPDOWNLIST|WS_VSCROLL);for(auto c:{L"All assets",L"Armor",L"Weapons",L"Vehicles",L"Unknown owners"})SendMessage(categoryBox,CB_ADDSTRING,0,(LPARAM)c);SendMessage(categoryBox,CB_SETCURSEL,0,0);control(L"BUTTON",L"Replace selected LUT DDS…",16,22,750,208,40,BS_OWNERDRAW);control(L"BUTTON",L"Selected asset details",17,22,800,208,36,BS_OWNERDRAW);control(L"BUTTON",L"Advanced >",19,22,676,208,36,BS_OWNERDRAW);control(L"BUTTON",L"Install into game",45,975,748,385,38,BS_OWNERDRAW);control(L"BUTTON",L"Restore game files",46,975,794,385,38,BS_OWNERDRAW);basicBox=control(WC_COMBOBOX,L"",44,975,560,385,300,CBS_DROPDOWNLIST|WS_VSCROLL);rowBox=control(WC_COMBOBOX,L"",20,270,94,220,400,CBS_DROPDOWNLIST|WS_VSCROLL);colBox=control(WC_COMBOBOX,L"",21,505,94,440,500,CBS_DROPDOWNLIST|WS_VSCROLL);presetBox=control(WC_COMBOBOX,L"",43,975,94,385,400,CBS_DROPDOWNLIST|WS_VSCROLL);SendMessage(presetBox,CB_ADDSTRING,0,(LPARAM)L"Imported Epic LUT selector");SendMessage(presetBox,CB_SETCURSEL,0,0);for(int i=0;i<4;i++)edits[i]=control(L"EDIT",L"",30+i,310+i*158,575,143,32,ES_AUTOHSCROLL|WS_BORDER);control(L"BUTTON",L"Apply RGBA",14,970,575,170,34,BS_OWNERDRAW);control(L"BUTTON",L"Choose RGB…",15,1155,575,205,34,BS_OWNERDRAW);
searchBox=control(L"EDIT",L"",40,270,690,1090,30,ES_AUTOHSCROLL|WS_BORDER);SendMessage(searchBox,EM_SETCUEBANNER,0,(LPARAM)L"Search items by name…");list=control(L"LISTBOX",L"",41,270,728,1090,106,LBS_NOTIFY|WS_VSCROLL|WS_HSCROLL|WS_BORDER);control(L"BUTTON",L"Select HD2 folder...",18,22,850,208,36,BS_OWNERDRAW);statusBox=control(L"STATIC",status.c_str(),50,22,963,1750,30);
control(L"BUTTON",L"⛶",47,1728,184,40,34,BS_OWNERDRAW);ownerBox=control(WC_COMBOBOX,L"",48,1290,94,488,400,CBS_DROPDOWNLIST|WS_VSCROLL);control(L"BUTTON",L"Shading: on",49,1548,766,230,36,BS_OWNERDRAW);previewItemBox=control(WC_COMBOBOX,L"",60,206,14,500,420,CBS_DROPDOWNLIST|WS_VSCROLL);return 0;}
case WM_DPICHANGED:{if(previewFullscreen)fitFullscreenMonitor(h);else{auto rect=(RECT*)l;SetWindowPos(h,nullptr,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER|SWP_NOACTIVATE);}layout();return 0;}
case WM_DISPLAYCHANGE:if(previewFullscreen)fitFullscreenMonitor(h);layout();return 0;
case WM_ERASEBKGND:return 1;
case WM_SIZE:layout();return 0;
case WM_GETMINMAXINFO:((MINMAXINFO*)l)->ptMinTrackSize={1200,700};return 0;
case WM_NCHITTEST:{if(previewFullscreen)return HTCLIENT;auto hit=DefWindowProc(h,m,w,l);POINT p={GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(h,&p);RECT r;GetClientRect(h,&r);int b=7;if(p.y<b)return p.x<b?HTTOPLEFT:p.x>r.right-b?HTTOPRIGHT:HTTOP;if(p.y>r.bottom-b)return p.x<b?HTBOTTOMLEFT:p.x>r.right-b?HTBOTTOMRIGHT:HTBOTTOM;if(p.x<b)return HTLEFT;if(p.x>r.right-b)return HTRIGHT;if(vp.logicalY(p.y)<50&&vp.logicalX(p.x)<1635)return HTCAPTION;return hit;}
case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:SetTextColor((HDC)w,RGB(225,228,236));SetBkColor((HDC)w,m==WM_CTLCOLORSTATIC?RGB(21,23,29):RGB(33,36,44));return (LRESULT)(m==WM_CTLCOLORSTATIC?bg:field);
case WM_DRAWITEM:{auto*d=(DRAWITEMSTRUCT*)l;FillRect(d->hDC,&d->rcItem,field);auto pen=CreatePen(PS_SOLID,1,RGB(112,38,50));auto old=SelectObject(d->hDC,pen);auto ob=SelectObject(d->hDC,GetStockObject(NULL_BRUSH));RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,8,8);SelectObject(d->hDC,ob);SelectObject(d->hDC,old);DeleteObject(pen);if(drawActionIcon(d))return TRUE;wchar_t s[128];GetWindowText(d->hwndItem,s,128);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,d->CtlID==92?RGB(255,90,102):RGB(235,237,241));DrawText(d->hDC,s,-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);return TRUE;}
case WM_APP+1:{if(worker.joinable())worker.join();busy=false;std::unique_ptr<std::vector<TextureRecord>> p((std::vector<TextureRecord>*)l);if(!cancelScan)records=std::move(*p);EnableWindow(GetDlgItem(win,45),TRUE);EnableWindow(GetDlgItem(win,46),TRUE);SetWindowText(GetDlgItem(win,2),L"Refresh items");std::stable_sort(records.begin(),records.end(),[](const TextureRecord&a,const TextureRecord&b){if((a.category==L"Unknown")!=(b.category==L"Unknown"))return a.category!=L"Unknown";return a.name<b.name;});filter();if(firstScan&&!cancelScan&&!filtered.empty()){SendMessage(list,LB_SETCURSEL,0,0);individualPiecePreview=filteredSinglePieces[0];setDoc(loadGameLut(records[filtered[0]]),true,filteredOwners[0]);}if(!firstScan)refreshItemPreview();firstScan=false;say(cancelScan?L"Scan cancelled.":L"Scan complete: "+std::to_wstring(records.size())+L" editable LUT resources. Select an item to load its preview.");return 0;}
case WM_APP+5:{std::unique_ptr<PreviewResult> result((PreviewResult*)l);if(result->job.generation!=previewGeneration||result->job.resource!=doc.resourceId||result->job.owner!=previewOwner)return 0;
 previewOwner=result->actualOwner;previewOwners=std::move(result->owners);SendMessage(ownerBox,CB_RESETCONTENT,0,0);for(const auto& name:previewOwners)SendMessage(ownerBox,CB_ADDSTRING,0,(LPARAM)name.c_str());SendMessage(ownerBox,CB_SETCURSEL,previewOwner,0);EnableWindow(ownerBox,previewOwners.size()>1);
 if(result->error.empty()){itemPreview=std::move(result->model);selectedPart=-1;if(result->job.restorePart>=0)for(size_t i=0;i<itemPreview.parts.size();++i)if(itemPreview.parts[i].materialKey==uint32_t(result->job.restorePart)){selectedPart=int(i);break;}}else{itemPreview={};itemPreview.notice=L"Item preview unavailable: "+widen(result->error);logPreviewFailure(std::runtime_error(result->error));}updatePartControls();InvalidateRect(h,0,FALSE);return 0;}
case WM_APP+4:scan();filter();return 0;
case WM_APP+2:say(L"Scanning archive "+std::to_wstring(w)+L" / "+std::to_wstring(l));return 0;
case WM_APP+3:{if(worker.joinable())worker.join();busy=false;EnableWindow(GetDlgItem(win,45),TRUE);EnableWindow(GetDlgItem(win,46),TRUE);SetWindowText(GetDlgItem(win,2),L"Refresh items");std::unique_ptr<std::string> s((std::string*)l);throw std::runtime_error(*s);}
case WM_COMMAND:{int id=LOWORD(w),notification=HIWORD(w);if(id==18){changeGameLocation();return 0;}
if(id==60&&notification==CBN_SELCHANGE&&!busy){auto index=SendMessage(previewItemBox,CB_GETCURSEL,0,0);if(index>=0&&size_t(index)<filtered.size()){SendMessage(list,LB_SETCURSEL,index,0);individualPiecePreview=filteredSinglePieces[index];setDoc(loadGameLut(records[filtered[index]]),true,filteredOwners[index]);}return 0;}
if(id==47){togglePreviewFullscreen();return 0;}
if(id==49){previewShading=!previewShading;SetWindowText(GetDlgItem(win,49),previewShading?L"Shading: on":L"Shading: off");InvalidateRect(h,0,FALSE);return 0;}
if(id==48&&notification==CBN_SELCHANGE){int index=(int)SendMessage(ownerBox,CB_GETCURSEL,0,0);if(index>=0&&size_t(index)<previewOwners.size()&&!busy){previewOwner=(size_t)index;queueItemPreview(previewOwner);}return 0;}
if(id==92){SendMessage(h,WM_CLOSE,0,0);return 0;}if(id==90){ShowWindow(h,SW_MINIMIZE);return 0;}if(id==91){ShowWindow(h,IsZoomed(h)?SW_RESTORE:SW_MAXIMIZE);return 0;}if(id==19){advanced=!advanced;if(!advanced){if(individualPiecePreview){individualPiecePreview=false;refreshItemPreview();}if(doc.width==3)col=0;const size_t choices[]={0,2,5,6,16,17,18,19};if(std::find(std::begin(choices),std::end(choices),col)==std::end(choices))col=0;sync();}layout();filter();return 0;}if(id==44&&notification==CBN_SELCHANGE){const size_t choices[]={0,2,5,6,16,17,18,19};int index=(int)SendMessage(basicBox,CB_GETCURSEL,0,0);if(index>=0&&index<8&&choices[index]<doc.width){col=choices[index];sync();}return 0;}if(id==43&&notification==CBN_SELCHANGE){selectImported(SendMessage(presetBox,CB_GETCURSEL,0,0));return 0;}if(id==42&&notification==CBN_SELCHANGE){filter();return 0;}if(id==40&&notification==EN_CHANGE){filter();return 0;}if(id==20&&notification==CBN_SELCHANGE){row=SendMessage(rowBox,CB_GETCURSEL,0,0);sync();return 0;}if(id==21&&notification==CBN_SELCHANGE){col=SendMessage(colBox,CB_GETCURSEL,0,0);sync();return 0;}if(id==41&&notification==LBN_SELCHANGE&&!busy){auto i=SendMessage(list,LB_GETCURSEL,0,0);if(i>=0&&size_t(i)<filtered.size()){SendMessage(previewItemBox,CB_SETCURSEL,i,0);individualPiecePreview=filteredSinglePieces[i];setDoc(loadGameLut(records[filtered[i]]),true,filteredOwners[i]);}return 0;}
if(id>=30&&id<=33&&!syncing&&notification==EN_SETFOCUS){remember();return 0;}if(id>=30&&id<=33&&!syncing&&notification==EN_CHANGE){Pixel v;bool valid=true;for(int k=0;k<4;k++){wchar_t b[128],*end;GetWindowText(edits[k],b,128);v[k]=wcstof(b,&end);while(iswspace(*end))end++;if(end==b||*end||!std::isfinite(v[k]))valid=false;}if(valid&&!doc.pixels.empty()){doc.at(col,row)=v;InvalidateRect(h,0,FALSE);}return 0;}std::wstring p;switch(id){case 45:{if(!ensureGameLocation())break;stashCurrent();std::vector<LutDocument> batch;for(const auto& entry:staged)batch.push_back(entry.second);if(batch.empty())throw std::runtime_error("No appearance changes are staged. Choose an item and change a color first.");say(L"Installing "+std::to_wstring(batch.size())+L" edited textures…");auto result=installLutPatches(batch,gameDataPath,base.wstring());say(result);MessageBox(h,result.c_str(),L"Installation finished",MB_ICONINFORMATION);break;}
case 46:{if(!ensureGameLocation())break;auto result=restoreLutPatch(gameDataPath,base.wstring());staged.clear();activeImported=-1;doc.resourceId.clear();firstScan=true;scan();say(result);MessageBox(h,result.c_str(),L"Restore finished",MB_ICONINFORMATION);break;}
case 1:p=dialog(false,L"DDS texture\0*.dds\0\0",L"dds");if(!p.empty())setDoc(loadDDS(p));break;case 2:firstScan=false;scan();break;case 3:p=dialog(false,L"OMEGA / Epic LUT presets\0*.olut;*.omega-lut;*.dbflut\0All files\0*.*\0\0",L"olut");if(!p.empty()){auto ext=fs::path(p).extension().wstring();for(auto&c:ext)c=towlower(c);if(ext==L".dbflut")importEpic(p);else setDoc(loadPreset(p));}break;case 4:p=dialog(true,L"OMEGA LUT preset\0*.olut\0\0",L"olut");if(!p.empty()){savePreset(doc,p);say(L"Preset saved.");}break;case 5:p=dialog(true,L"DDS texture\0*.dds\0\0",L"dds");if(!p.empty()){saveDDS(doc,p);say(L"DDS exported successfully.");}break;case 6:{if(!ensureGameLocation())break;if(doc.resourceId.empty())throw std::runtime_error("Load a current game LUT first. A standalone DDS has no verified game target.");auto current=scanGameLuts(gameDataPath);auto match=std::find_if(current.begin(),current.end(),[](const TextureRecord&r){return r.resourceId==doc.resourceId;});if(match==current.end())throw std::runtime_error("Target resource is unavailable in the current game. Rescan and select a current target.");auto fresh=loadGameLut(*match);if(fresh.nativeHeader!=doc.nativeHeader||fresh.pixels!=doc.original||fresh.width!=doc.width||fresh.height!=doc.height)throw std::runtime_error("Source changed since this preset was made. Load the current game target and replace its DDS explicitly.");auto out=base/L"Exports"/(L"LUT-"+std::to_wstring(GetTickCount64()));fs::create_directories(out.parent_path());exportPatch(doc,out.wstring());say(L"Patch exported: "+out.wstring());MessageBox(h,(L"Patch export finished.\n\n"+out.wstring()+L"\n\nGame files were not changed.").c_str(),L"Export successful",MB_ICONINFORMATION);break;}case 7:remember();doc.pixels=doc.original;sync();say(L"All LUT values restored to loaded originals.");break;case 8:if(!undoStack.empty()){redoStack.push_back(doc.pixels);doc.pixels=undoStack.back();undoStack.pop_back();sync();}break;case 9:if(!redoStack.empty()){undoStack.push_back(doc.pixels);doc.pixels=redoStack.back();redoStack.pop_back();sync();}break;case 10:original=!original;InvalidateRect(h,0,FALSE);break;case 11:checker=!checker;InvalidateRect(h,0,FALSE);break;case 12:rotation=.4f;previewPitch=.12f;previewZoom=1.f;InvalidateRect(h,0,FALSE);break;case 13:MessageBox(h,L"OMEGA Epic LUT Editor R5.9.3.6\n\nBased on Epic LUT R5.9.3 by HWG90 (0BSD).\nMatchYourColors: CowboyBingus.\nDebug LUT: Plain Furniture. OMEGA branding: original OMEGA project.\n\nFile-only DDS/preset/patch editing. No runtime, injection or process-memory access.\nPreview uses recovered item geometry where available. Material shading is approximate; exact in-game shader and lighting parity is not verified.",L"OMEGA — credits",MB_OK);break;
case 17:{auto info=doc.label+L"\n\nResource: "+widen(doc.resourceId)+L"\nArchive: "+widen(doc.archiveId)+L"\nSource: "+doc.sourcePath+L"\nDimensions: "+std::to_wstring(doc.width)+L" × "+std::to_wstring(doc.height)+L"\n\nA texture may be shared by several items. Export replaces this resource for every user of it.\n\nItem labels are recovered source paths with current serialized links. Unknown owners are not guessed.";MessageBox(h,info.c_str(),L"Current LUT / shared owners",MB_OK);break;}case 16:{p=dialog(false,L"DDS texture\0*.dds\0\0",L"dds");if(!p.empty()){auto replacement=loadDDS(p);if(replacement.width!=doc.width||replacement.height!=doc.height)throw std::runtime_error("Replacement DDS dimensions must exactly match the selected LUT target.");remember();doc.pixels=std::move(replacement.pixels);sync();say(L"DDS values imported into selected target. Current resource identity preserved.");}break;}case 14:{Pixel v;for(int k=0;k<4;k++){wchar_t b[128],*end;GetWindowText(edits[k],b,128);v[k]=wcstof(b,&end);while(iswspace(*end))end++;if(end==b||*end||!std::isfinite(v[k]))throw std::runtime_error("Enter four finite float32 numbers. HDR and negative values are allowed.");}remember();doc.at(col,row)=v;sync();say(L"RGBA applied. Preview updated.");break;}case 15:{inlinePickerFocused=true;readInlineColor();SetFocus(win);InvalidateRect(win,0,FALSE);say(L"Drag the inline color square for saturation/brightness and the hue strip for hue. Alpha stays unchanged.");break;}}return 0;}
case WM_KEYDOWN:if(w==VK_ESCAPE&&previewFullscreen){togglePreviewFullscreen();return 0;}break;
case WM_LBUTTONDOWN:{POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};if(previewFullscreen){for(int i=(int)fullscreenBars.size()-1;i>=0;--i){auto&b=fullscreenBars[i];double x=vp.logicalX(p.x),y=vp.logicalY(p.y);if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+28){draggingBar=i;barPress=p;barStartX=b.x;barStartY=b.y;SetCapture(h);return 0;}}}if(beginInlineColor(p))return 0;if(overFullscreenPanel(p))return 0;RECT bounds=previewBounds();if(PtInRect(&bounds,p)){previewDragging=true;previewMoved=false;previewLastMouse=previewPressMouse=p;SetCapture(h);SetFocus(h);return 0;}auto x=vp.logicalX(p.x),y=vp.logicalY(p.y);if(!previewFullscreen&&advanced&&x>=270&&x<950&&y>=165&&y<535&&!doc.pixels.empty()){col=std::min<size_t>(doc.width-1,size_t((x-270)*doc.width/680));row=std::min<size_t>(doc.height-1,size_t((y-165)*doc.height/370));sync();}return 0;}
case WM_MOUSEMOVE:if(inlinePickerDrag){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};applyInlineColor(p);return 0;}if(draggingBar>=0){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};auto&b=fullscreenBars[draggingBar];moveFullscreenBar(b,barStartX+int((p.x-barPress.x)/vp.scale),barStartY+int((p.y-barPress.y)/vp.scale));layout();return 0;}if(previewDragging){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};auto dx=p.x-previewPressMouse.x,dy=p.y-previewPressMouse.y;if(dx*dx+dy*dy>=16)previewMoved=true;if(previewMoved){rotation+=float(p.x-previewLastMouse.x)*.008f;previewPitch=std::clamp(previewPitch+float(p.y-previewLastMouse.y)*.008f,-1.5f,1.5f);InvalidateRect(h,0,FALSE);}previewLastMouse=p;}return 0;
case WM_LBUTTONUP:if(inlinePickerDrag){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};applyInlineColor(p);endInlineColorGesture();return 0;}if(draggingBar>=0){saveToolbarPositions();draggingBar=-1;ReleaseCapture();return 0;}if(previewDragging){bool select=!previewMoved;previewDragging=false;ReleaseCapture();if(select&&!itemPreview.vertices.empty()){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};RECT bounds=previewBounds();if(PtInRect(&bounds,p)){int pickedRow=pickItemPreviewRow(p.x,p.y);int part=pickItemPreview(p.x,p.y);if(part>=0&&size_t(part)<itemPreview.parts.size()){auto chosen=itemPreview.parts[part];selectedPart=part;if(!chosen.lutResourceId.empty()&&chosen.lutResourceId!=doc.resourceId){auto target=std::find_if(records.begin(),records.end(),[&](const TextureRecord& r){return r.resourceId==chosen.lutResourceId;});if(target!=records.end()&&!busy){auto ownerName=previewOwner<previewOwners.size()?previewOwners[previewOwner]:std::wstring{};auto next=loadGameLut(*target);setDoc(std::move(next),true,ownerName,chosen.materialKey);if(pickedRow>=0&&uint32_t(pickedRow)<doc.height&&doc.width==23){row=pickedRow;col=0;sync();}say(L"Selected material: "+chosen.name+L". Choose its material row, then change color.");}else say(L"Selected material: "+chosen.name+L". Its editable LUT is unavailable.");}else {if(pickedRow>=0&&uint32_t(pickedRow)<doc.height&&doc.width==23){row=pickedRow;col=0;sync();}say(L"Selected material: "+chosen.name+(chosen.lutResourceId.empty()?L". No verified editable LUT for this part.":L". Color region selected from source mask."));}}else selectedPart=-1;updatePartControls();InvalidateRect(h,0,FALSE);}}}return 0;
case WM_CAPTURECHANGED:inlinePickerDrag=0;inlinePickerUndo=false;draggingBar=-1;previewDragging=false;return 0;
case WM_CANCELMODE:endInlineColorGesture();draggingBar=-1;previewDragging=false;ReleaseCapture();return 0;
case WM_ACTIVATEAPP:if(!w)endInlineColorGesture();if(!w&&previewDragging){previewDragging=false;ReleaseCapture();}break;
case WM_MOUSEWHEEL:{POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(h,&p);if(overFullscreenPanel(p))return 0;RECT bounds=previewBounds();if(PtInRect(&bounds,p)){previewZoom=std::clamp(previewZoom*std::pow(1.12f,float(GET_WHEEL_DELTA_WPARAM(w))/WHEEL_DELTA),.25f,5.f);InvalidateRect(h,0,FALSE);return 0;}break;}
case WM_RBUTTONUP:if(previewFullscreen){togglePreviewFullscreen();return 0;}break;
case WM_PAINT:{PAINTSTRUCT ps;HDC front=BeginPaint(h,&ps);RECT client;GetClientRect(h,&client);HDC dc=paintBuffer.acquire(front,client.right,client.bottom);paintCanvasBackground(dc,client);auto oldPaintFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);
 if(!previewFullscreen){text(dc,22,16,1500,35,L"Ω  OMEGA  /  EPIC LUT EDITOR     R5.9.3.6",RGB(249,91,108));text(dc,22,53,1600,30,advanced?L"ADVANCED  /  Exact LUT values and DDS tools":L"Choose an item  >  Change its appearance  >  Install",RGB(145,154,171));text(dc,970,137,800,30,L"ITEM PREVIEW  ·  Click a part  ·  Hold M1 to rotate  ·  Wheel to zoom");
  if(advanced){text(dc,270,135,700,28,L"LUT GRID  ·  Click a cell to edit its exact RGBA values");if(!doc.pixels.empty())for(size_t y=0;y<doc.height;y++)for(size_t x=0;x<doc.width;x++){auto&p=doc.at(x,y);auto cv=[](float v){v=std::max(0.f,v);return(BYTE)(255*v/(1+v));};RECT r={vp.x(270+680.*x/doc.width),vp.y(165+370.*y/doc.height),vp.x(270+680.*(x+1)/doc.width),vp.y(165+370.*(y+1)/doc.height)};auto b=CreateSolidBrush(RGB(cv(p[0]),cv(p[1]),cv(p[2])));FillRect(dc,&r,b);DeleteObject(b);FrameRect(dc,&r,(HBRUSH)GetStockObject(BLACK_BRUSH));if(x==col&&y==row){InflateRect(&r,-2,-2);FrameRect(dc,&r,(HBRUSH)GetStockObject(WHITE_BRUSH));}}
  text(dc,270,540,700,25,L"SELECTED CELL  ·  R                         G                         B                         A");text(dc,270,668,680,70,columnHelp(col),RGB(164,174,191));text(dc,270,730,680,24,L"CURRENT GAME ITEMS  ·  Select to preview",RGB(249,91,108));}
  else text(dc,970,856,515,36,columnHelp(col),RGB(164,174,191));
 }
 RECT pr=previewBounds();if(checker)paintCheckerBackground(dc,pr);else FillRect(dc,&pr,field);
 if(!doc.pixels.empty()&&!itemPreview.vertices.empty())renderItemPreview(dc,pr,doc,row,itemPreview,rotation,previewPitch,previewZoom,checker,original,previewShading,selectedPart,col);
 else{auto message=itemPreview.notice.empty()?L"Select an item to load its model.":itemPreview.notice;RECT messageRect=pr;InflateRect(&messageRect,-24,-24);SetTextColor(dc,RGB(181,190,207));DrawText(dc,message.c_str(),-1,&messageRect,DT_CENTER|DT_WORDBREAK|DT_NOPREFIX);}
 std::wstring caption=(original?L"ORIGINAL":L"EDITED");if(selectedPart>=0&&size_t(selectedPart)<itemPreview.parts.size())caption+=L" · Material: "+itemPreview.parts[selectedPart].name;caption+=itemPreview.vertices.empty()?L" · Model unavailable":L" · "+itemPreviewRenderStatus();
 if(previewFullscreen){
for(size_t i=0;i<fullscreenBars.size();++i){auto&b=fullscreenBars[i];RECT card{vp.x(b.x),vp.y(b.y),vp.x(b.x+b.w),vp.y(b.y+b.h)};auto brush=CreateSolidBrush(RGB(25,28,37));auto pen=CreatePen(PS_SOLID,std::max(1,vp.length(1)),RGB(58,65,81));auto ob=SelectObject(dc,brush),op=SelectObject(dc,pen);RoundRect(dc,card.left,card.top,card.right,card.bottom,vp.length(12),vp.length(12));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(brush);DeleteObject(pen);text(dc,b.x+16,b.y+4,b.w-100,24,b.title,RGB(242,104,128));text(dc,b.x+b.w-80,b.y+4,66,24,L"DRAG",RGB(113,123,144));
if(i==2){text(dc,b.x+16,b.y+28,210,20,L"Material row",RGB(153,165,185));text(dc,b.x+244,b.y+28,330,20,L"Color region",RGB(153,165,185));text(dc,b.x+590,b.y+28,514,20,L"Advanced channel",RGB(153,165,185));}
if(i==3){for(int k=0;k<4;k++){const wchar_t*labels[]={L"RED",L"GREEN",L"BLUE",L"ALPHA / MODE"};text(dc,b.x+16+k*150,b.y+30,134,22,labels[k],RGB(153,165,185));}text(dc,b.x+16,b.y+100,728,22,L"Inline RGB picker: 0–1  ·  Alpha / mode is preserved",RGB(116,128,149));paintInlineColor(dc);}
if(i==1)text(dc,b.x+16,b.y+78,b.w-32,22,L"Compare / Shading / Exit full screen",RGB(116,128,149));
if(i==4)text(dc,b.x+16,b.y+96,b.w-32,22,L"Undo / Redo / Reset",RGB(116,128,149));
if(i==5)text(dc,b.x+16,b.y+96,b.w-32,22,L"DDS / Info / Save / HD2 folder",RGB(116,128,149));}
text(dc,32,698,1720,20,caption+L"  /  M1 rotate / Wheel zoom / Esc return",RGB(151,163,184));
}
 else{text(dc,970,704,808,26,caption,RGB(249,110,123));text(dc,970,734,808,28,itemPreview.notice,RGB(164,174,191));size_t pending=staged.size();bool listed=staged.count(doc.resourceId)!=0;if(!doc.resourceId.empty()&&!doc.archiveId.empty()){if(doc.changed()&&!listed)pending++;if(!doc.changed()&&listed)pending--;}text(dc,270,938,1508,23,std::to_wstring(pending)+L" edited textures · edits stay when switching items",RGB(164,174,191));}
 if(!previewFullscreen)paintInlineColor(dc);SelectObject(dc,oldPaintFont);if(dc!=front)BitBlt(front,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,dc,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);EndPaint(h,&ps);return 0;}
case WM_CLOSE:saveToolbarPositions();cancelScan=true;{std::lock_guard<std::mutex> lock(previewMutex);previewStopping=true;previewHasPending=false;}previewWake.notify_one();if(worker.joinable())worker.join();if(previewWorker.joinable())previewWorker.join();{MSG pending;while(PeekMessage(&pending,h,WM_APP+5,WM_APP+5,PM_REMOVE))delete (PreviewResult*)pending.lParam;}DestroyWindow(h);return 0;case WM_DESTROY:PostQuitMessage(0);return 0;}}catch(const std::exception&e){error(e);}return DefWindowProc(h,m,w,l);}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,LPWSTR,int show){wchar_t path[32768];GetModuleFileName(0,path,32768);base=fs::path(path).parent_path();initializeGameLocation();loadToolbarPositions();INITCOMMONCONTROLSEX cc={sizeof(cc),ICC_STANDARD_CLASSES};InitCommonControlsEx(&cc);WNDCLASS wc={};wc.lpfnWndProc=proc;wc.hInstance=inst;wc.hCursor=LoadCursor(0,IDC_ARROW);wc.hIcon=LoadIcon(inst,MAKEINTRESOURCE(101));wc.lpszClassName=L"OmegaEpicLutOffline";RegisterClass(&wc);win=CreateWindowEx(WS_EX_APPWINDOW,wc.lpszClassName,L"OMEGA Epic LUT Editor R5.9.3.6",WS_POPUP|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN,12,12,std::min(2100,GetSystemMetrics(SM_CXSCREEN)-24),std::min(1250,GetSystemMetrics(SM_CYSCREEN)-60),0,0,inst,0);BOOL dark=TRUE;DwmSetWindowAttribute(win,20,&dark,sizeof(dark));MARGINS margins={1,1,1,1};DwmExtendFrameIntoClientArea(win,&margins);try{auto res=FindResource(inst,MAKEINTRESOURCE(102),RT_RCDATA);auto data=LoadResource(inst,res);auto ptr=(uint8_t*)LockResource(data);setDoc(decodeDDS(std::vector<uint8_t>(ptr,ptr+SizeofResource(inst,res)),L"Epic supplied debug LUT (Plain Furniture)"));}catch(const std::exception&e){error(e);}layout();ShowWindow(win,show);UpdateWindow(win);try{if(!gameDataPath.empty())for(auto& d:loadInstalledLutPatches(gameDataPath,base.wstring()))staged[d.resourceId]=std::move(d);}catch(const std::exception&e){error(e);}PostMessage(win,WM_APP+4,0,0);MSG msg;while(GetMessage(&msg,0,0,0)>0){if(msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE&&previewFullscreen){togglePreviewFullscreen();continue;}TranslateMessage(&msg);DispatchMessage(&msg);}return 0;}









