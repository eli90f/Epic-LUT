from pathlib import Path
p=Path(r'D:\GPT\OMEGA-EPIC-LUT-EDITOR\native\main.cpp')
s=p.read_text(encoding='utf-8-sig')
s=s.replace('categoryBox, presetBox;', 'categoryBox, presetBox, basicBox;')
s=s.replace('syncing=false;', 'syncing=false,advanced=false,firstScan=true;',1)
a=s.index('void layout(){');b=s.index('const wchar_t* columnName',a)
s=s[:a]+'''void layout(){RECT r;GetClientRect(win,&r);vp.resize(r.right,r.bottom);if(font)DeleteObject(font);font=CreateFont(-vp.length(16),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");for(auto&c:controls){int id=GetDlgCtrlID(c.hwnd),x=c.x,y=c.y,w=c.w,h=c.h;bool visible=true;
if(!advanced){switch(id){case 1:case 5:case 11:case 14:case 16:case 17:case 21:case 30:case 31:case 32:case 33:visible=false;break;
case 2:y=94;break;case 3:y=142;break;case 4:y=190;break;case 6:y=238;break;case 7:y=310;break;case 8:y=358;break;case 9:y=406;break;case 10:y=478;break;case 12:y=526;break;case 19:y=598;break;case 13:y=646;break;
case 20:x=975;y=94;w=385;break;case 42:x=270;y=94;w=680;break;case 40:x=270;y=135;w=680;break;case 41:x=270;y=180;w=680;h=650;break;case 15:x=975;y=610;w=385;h=40;break;case 44:x=975;y=560;w=385;break;case 43:x=22;y=740;w=208;visible=!imported.empty();break;}}
else if(id==44)visible=false;
ShowWindow(c.hwnd,visible?SW_SHOW:SW_HIDE);MoveWindow(c.hwnd,vp.x(x),vp.y(y),vp.length(w),vp.length(h),TRUE);SendMessage(c.hwnd,WM_SETFONT,(WPARAM)font,TRUE);}SetWindowText(GetDlgItem(win,19),advanced?L"< Back to simple editor":L"Advanced >");InvalidateRect(win,0,TRUE);}
''' +s[b:]
s=s.replace('InvalidateRect(win,0,FALSE);syncing=false;}', '''SendMessage(basicBox,CB_RESETCONTENT,0,0);const size_t choices[]={0,2,5,6,16,17,18,19};for(size_t i=0;i<8;i++){if(choices[i]>=doc.width)continue;SendMessage(basicBox,CB_ADDSTRING,0,(LPARAM)columnName(choices[i]));}int basicIndex=0;for(int i=0;i<8;i++)if(choices[i]==col)basicIndex=i;SendMessage(basicBox,CB_SETCURSEL,basicIndex,0);InvalidateRect(win,0,FALSE);syncing=false;}''')
s=s.replace('if(n>=imported.size())return;', 'if(n>=imported.size()||int(n)==activeImported)return;')
s=s.replace('if(!imported.empty())selectImported(0);auto report=', 'if(!imported.empty())selectImported(0);layout();auto report=')
s=s.replace('void filter(){', '''std::wstring itemName(const TextureRecord&r){auto name=r.name;auto stop=name.find(L" / LUT ");if(stop!=std::wstring::npos)name.resize(stop);auto shared=name.find(L" | ");bool more=shared!=std::wstring::npos;if(more)name.resize(shared);if(name.rfind(L"content/",0)==0){auto slash=name.find_last_of(L'/');if(slash!=std::wstring::npos)name=name.substr(slash+1);bool cap=true;for(auto&c:name){if(c==L'_'){c=L' ';cap=true;}else if(cap){c=towupper(c);cap=false;}}}if(r.category==L"Unknown")name=L"Unknown item · "+widen(r.resourceId);if(more)name+=L" (shared texture)";return name;}
void filter(){''')
s=s.replace('auto s=records[i].category+L" · "+widen(records[i].resourceId)+L" · "+std::to_wstring(records[i].width)+L"×"+std::to_wstring(records[i].height)+L" · "+records[i].name;auto t=s;', 'auto s=records[i].category+L" · "+itemName(records[i]);if(advanced)s+=L" · "+widen(records[i].resourceId);auto t=s+records[i].name+widen(records[i].resourceId);')
s=s.replace('}SendMessage(list,LB_SETHORIZONTALEXTENT', '}if(filtered.empty())SendMessage(list,LB_ADDSTRING,0,(LPARAM)(busy?L"Loading items from the installed game…":L"No matching items. Clear search or choose All assets."));SendMessage(list,LB_SETHORIZONTALEXTENT')
s=s.replace('control(L"BUTTON",L"Selected asset details",17,22,800,208,36,BS_OWNERDRAW);', 'control(L"BUTTON",L"Selected asset details",17,22,800,208,36,BS_OWNERDRAW);control(L"BUTTON",L"Advanced >",19,22,676,208,36,BS_OWNERDRAW);basicBox=control(WC_COMBOBOX,L"",44,975,560,385,300,CBS_DROPDOWNLIST|WS_VSCROLL);')
s=s.replace('L"Search current game LUT resource IDs"', 'L"Search items by name…"')
s=s.replace('records=std::move(*p);filter();say(', '''records=std::move(*p);std::stable_sort(records.begin(),records.end(),[](const TextureRecord&a,const TextureRecord&b){if((a.category==L"Unknown")!=(b.category==L"Unknown"))return a.category!=L"Unknown";return a.name<b.name;});filter();if(firstScan&&!cancelScan&&!filtered.empty()){SendMessage(list,LB_SETCURSEL,0,0);setDoc(loadGameLut(records[filtered[0]]));}firstScan=false;say(''')
s=s.replace('case WM_APP+2:', 'case WM_APP+4:scan();filter();return 0;\ncase WM_APP+2:')
s=s.replace('if(id==43&&notification==CBN_SELCHANGE)', '''if(id==19){advanced=!advanced;layout();filter();return 0;}if(id==44&&notification==CBN_SELCHANGE){const size_t choices[]={0,2,5,6,16,17,18,19};int index=(int)SendMessage(basicBox,CB_GETCURSEL,0,0);if(index>=0&&index<8&&choices[index]<doc.width){col=choices[index];sync();}return 0;}if(id==43&&notification==CBN_SELCHANGE)''')
s=s.replace('case 2:scan();break;', 'case 2:firstScan=false;scan();break;')
s=s.replace('if(x>=270&&x<950&&y>=165', 'if(advanced&&x>=270&&x<950&&y>=165')
s=s.replace('text(dc,270,135,700,28,L"LUT GRID', 'if(advanced)text(dc,270,135,700,28,L"LUT GRID')
s=s.replace('if(!doc.pixels.empty()){for(size_t y=', 'if(!doc.pixels.empty()){if(advanced){for(size_t y=')
s=s.replace('RECT r={vp.x(975),vp.y(160)', '}RECT r={vp.x(975),vp.y(160)')
s=s.replace('text(dc,270,540,700,25,L"SELECTED CELL', 'if(advanced){text(dc,270,540,700,25,L"SELECTED CELL')
s=s.replace('columnHelp(col),RGB(164,174,191));}text(dc,270,663', 'columnHelp(col),RGB(164,174,191));}else{text(dc,975,660,385,140,columnHelp(col),RGB(164,174,191));text(dc,975,800,385,36,L"Double-click an item to edit its appearance.",RGB(249,110,123));}}if(advanced)text(dc,270,663')
s=s.replace('ShowWindow(win,show);UpdateWindow(win);MSG', 'layout();ShowWindow(win,show);UpdateWindow(win);PostMessage(win,WM_APP+4,0,0);MSG')
s=s.replace('R5.9.3 · OFFLINE','R5.9.3.2 · OFFLINE').replace('R5.9.3 Offline Test 1','R5.9.3 Offline Test 2')
p.write_text(s,encoding='utf-8-sig')
p=Path(r'D:\GPT\OMEGA-EPIC-LUT-EDITOR\tools\build.cmd');s=p.read_text().replace('R5.9.3.1.exe','R5.9.3.2.exe');p.write_text(s)
p=Path(r'D:\GPT\OMEGA-EPIC-LUT-EDITOR\native\omega.rc');s=p.read_text().replace('5,9,3,1','5,9,3,2').replace('5.9.3.1','5.9.3.2').replace('Offline Test 1','Offline Test 2');p.write_text(s)
