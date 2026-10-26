from pathlib import Path
import sys,struct,re,json
sys.path.insert(0,r'../tools')
from game_catalog import Archives
exec(Path('build/generate_asset_names.py').read_text().split('assert h')[0])
a=Archives(Path(r'D:\SteamLibrary\steamapps\common\Helldivers 2\data'));typ=f'{h("state_machine"):016x}';targets=['Action_Group/Salute_Loop','Action_Group/Attention_Salute_Loop','Action_Group/Salute_Enter_1'];needles={x:struct.pack('<Q',h(x)) for x in targets};found=[];done=set();total=0
for ar in a.names:
 if not re.fullmatch('[a-f0-9]{16}',ar):continue
 for (rid,t),rec in a.toc(ar).items():
  if t!=typ or (rid,rec['main'][1]) in done:continue
  done.add((rid,rec['main'][1]));b=a.part(ar,rec,'main',0,rec['main'][1]);total+=1
  hits={n:b.find(k) for n,k in needles.items() if b.find(k)>=0}
  if hits:
   print('MATCH',ar,rid,len(b),hits,flush=True);found.append({'archive':ar,'id':rid,'hits':hits});Path('build/armor-preview/salute-research/'+rid+'.state_machine').write_bytes(b)
print('scanned',total,'matching',len(found));Path('build/armor-preview/salute-research/state-machine-matches.json').write_text(json.dumps(found,indent=2))
