from pathlib import Path
import sys,struct,json,re
sys.path.insert(0,r'../tools')
from game_catalog import Archives
exec(Path('build/generate_asset_names.py').read_text().split('assert h')[0])
a=Archives(Path(r'D:\SteamLibrary\steamapps\common\Helldivers 2\data'));dest=Path('build/armor-preview/salute-research');labels={h(x):x for x in Path(r'research/filediver\hashes\hashes.txt').read_text().splitlines()};hits=[]
for row in json.loads((dest/'state-machine-matches.json').read_text()):
 b=(dest/(row['id']+'.state_machine')).read_bytes();q=lambda at:struct.unpack_from('<Q',b,at)[0];u=lambda at:struct.unpack_from('<I',b,at)[0]
 for name,at in row['hits'].items():
  ids=[f'{q(at+u(at+16)+k*8):016x}' for k in range(u(at+12))];hits.append({'machine':row['id'],'machineName':labels.get(int(row['id'],16),'unknown'),'name':name,'clips':ids})
print(json.dumps(hits,indent=2));refs={x for row in hits for x in row['clips']};done=set();clips=[]
for ar in a.names:
 if not re.fullmatch('[a-f0-9]{16}',ar):continue
 for (rid,t),rec in a.toc(ar).items():
  if rid not in refs or t!=f'{h("animation"):016x}' or rid in done:continue
  done.add(rid);b=a.part(ar,rec,'main',0,rec['main'][1]);(dest/(rid+'.animation')).write_bytes(b);meta=struct.unpack_from('<IIfIII',b);clips.append({'id':rid,'archive':ar,'bytes':len(b),'header':meta});print('CLIP',rid,meta)
print('clips',len(clips),'missing',refs-done);(dest/'salute-clips.json').write_text(json.dumps(clips,indent=2));(dest/'salute-state-refs.json').write_text(json.dumps(hits,indent=2))
