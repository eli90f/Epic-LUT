from pathlib import Path
p=Path(r'D:\GPT\OMEGA-EPIC-LUT-EDITOR\native\main.cpp');s=p.read_text(encoding='utf-8-sig')
s=s.replace('SetWindowText(GetDlgItem(win,19),advanced?', 'SetWindowText(GetDlgItem(win,3),advanced?L"Load preset (.olut / Epic)…":L"Load preset…");SetWindowText(GetDlgItem(win,15),advanced?L"Choose RGB…":L"Change color…");SetWindowText(GetDlgItem(win,2),busy?L"Cancel item scan":L"Refresh items");SetWindowText(GetDlgItem(win,19),advanced?')
s=s.replace('busy=true;cancelScan=false;say(', 'busy=true;cancelScan=false;SetWindowText(GetDlgItem(win,2),L"Cancel item scan");say(')
s=s.replace('records=std::move(*p);std::stable_sort', 'records=std::move(*p);SetWindowText(GetDlgItem(win,2),L"Refresh items");std::stable_sort')
s=s.replace('case WM_APP+3:{std::unique_ptr', 'case WM_APP+3:{SetWindowText(GetDlgItem(win,2),L"Refresh items");std::unique_ptr')
p.write_text(s,encoding='utf-8-sig')
