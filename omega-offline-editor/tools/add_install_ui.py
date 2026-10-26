from pathlib import Path
p=Path(r'D:\GPT\OMEGA-EPIC-LUT-EDITOR\native\main.cpp');s=p.read_text(encoding='utf-8-sig')
s=s.replace('#include <iomanip>','#include <iomanip>\n#include <map>\n#include "patch_install.h"')
s=s.replace('LutDocument doc; std::vector', 'LutDocument doc; std::map<std::string,LutDocument> staged; std::vector')
s=s.replace('case 1:case 5:case 11:', 'case 1:case 5:case 6:case 11:')
s=s.replace('case 44:x=975;y=560;w=385;break;', 'case 44:x=975;y=560;w=385;break;case 45:x=975;y=748;w=385;h=38;break;case 46:x=975;y=794;w=385;h=38;break;')
s=s.replace('else if(id==44)visible=false;', 'else {if(id==44)visible=false;if(id==42)y=715;if(id==40||id==41)w=680;if(id==45){x=975;y=728;w=385;h=40;}if(id==46){x=975;y=778;w=385;h=40;}}')
s=s.replace('void setDoc(LutDocument d){', '''void stashCurrent(){if(!doc.resourceId.empty()&&!doc.archiveId.empty()){if(doc.changed())staged[doc.resourceId]=doc;else staged.erase(doc.resourceId);}}
void setDoc(LutDocument d){stashCurrent();auto saved=staged.find(d.resourceId);if(saved!=staged.end())d=saved->second;''')
s=s.replace('control(L"BUTTON",L"Advanced >",19,22,676,208,36,BS_OWNERDRAW);', 'control(L"BUTTON",L"Advanced >",19,22,676,208,36,BS_OWNERDRAW);control(L"BUTTON",L"Install into game",45,975,748,385,38,BS_OWNERDRAW);control(L"BUTTON",L"Restore game files",46,975,794,385,38,BS_OWNERDRAW);')
s=s.replace('if(id==19){advanced=!advanced;layout();filter();return 0;}', 'if(id==19){advanced=!advanced;if(!advanced){const size_t choices[]={0,2,5,6,16,17,18,19};if(std::find(std::begin(choices),std::end(choices),col)==std::end(choices))col=0;sync();}layout();filter();return 0;}')
s=s.replace('case 1:p=dialog(', '''case 45:{stashCurrent();std::vector<LutDocument> batch;for(const auto& entry:staged)batch.push_back(entry.second);if(batch.empty())throw std::runtime_error("No appearance changes are staged. Choose an item and change a color first.");say(L"Installing "+std::to_wstring(batch.size())+L" edited textures…");auto result=installLutPatches(batch,L"D:\\\\SteamLibrary\\\\steamapps\\\\common\\\\Helldivers 2\\\\data",base.wstring());say(result);MessageBox(h,result.c_str(),L"Installation finished",MB_ICONINFORMATION);break;}
case 46:{auto result=restoreLutPatch(L"D:\\\\SteamLibrary\\\\steamapps\\\\common\\\\Helldivers 2\\\\data",base.wstring());staged.clear();activeImported=-1;doc.resourceId.clear();firstScan=true;scan();say(result);MessageBox(h,result.c_str(),L"Restore finished",MB_ICONINFORMATION);break;}
case 1:p=dialog(''')
s=s.replace('text(dc,975,660,385,140,columnHelp(col)', 'text(dc,975,660,385,75,columnHelp(col)')
s=s.replace('text(dc,975,800,385,36,L"Double-click an item to edit its appearance.",RGB(249,110,123));','')
s=s.replace('PostMessage(win,WM_APP+4,0,0);MSG', 'try{for(auto& d:loadInstalledLutPatches(L"D:\\\\SteamLibrary\\\\steamapps\\\\common\\\\Helldivers 2\\\\data",base.wstring()))staged[d.resourceId]=std::move(d);}catch(const std::exception&e){error(e);}PostMessage(win,WM_APP+4,0,0);MSG')
p.write_text(s,encoding='utf-8-sig')
