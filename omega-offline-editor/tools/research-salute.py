"""Read-only leanrea probe: current avatar state-machine salute references."""
from pathlib import Path
import sys,struct,json,re,hashlib
sys.path.insert(0,r'../tools')
from game_catalog import Archives
exec(Path('build/generate_asset_names.py').read_text().split('assert h')[0])
a=Archives(Path(r'D:\SteamLibrary\steamapps\common\Helldivers 2\data'));ar='18235e0c9ec0e636';uid='4d1c334d294dfa97';toc=a.toc(ar)
rec=toc[(uid,f'{h("state_machine"):016x}')];b=a.part(ar,rec,'main',0,rec['main'][1]);dest=Path('build/armor-preview/salute-research');dest.mkdir(exist_ok=True);(dest/'avatar.state_machine').write_bytes(b)
u=lambda at:struct.unpack_from('<I',b,at)[0];q=lambda at:struct.unpack_from('<Q',b,at)[0]
labels={h(x):x for x in Path(r'research/filediver\hashes\hashes.txt').read_text().splitlines()};group=u(8);n=u(group);print('groups',n,'size',len(b));states=[]
for i in range(n):
 layer=group+u(group+4+i*4);count=u(layer+8);assert count<100000
 for j in range(count):
  at=layer+u(layer+12+j*4);name=labels.get(q(at),f'{q(at):016x}');nt=u(at+12);off=u(at+16);assert nt<10000
  ids=[f'{q(at+off+k*8):016x}' for k in range(nt)] if off else []
  states.append({'name':name,'hash':f'{q(at):016x}','offset':at,'type':u(at+8),'loop':bool(u(at+32)),'animations':ids})
hits=[s for s in states if 'salute' in s['name'].lower()];print('salute states',hits)
(dest/'states.json').write_text(json.dumps(states,indent=2));(dest/'salute-states.json').write_text(json.dumps(hits,indent=2))
refs={v for hit in hits for v in hit['animations']};found=[]
for ar in a.names:
 if not re.fullmatch('[a-f0-9]{16}',ar):continue
 for (rid,typ),rec in a.toc(ar).items():
  if rid in refs and typ==f'{h("animation"):016x}':
   data=a.part(ar,rec,'main',0,rec['main'][1]);out=dest/(rid+'.animation');out.write_bytes(data);found.append({'id':rid,'archive':ar,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()});refs.remove(rid)
 if not refs:break
print('animation files',found);(dest/'animations.json').write_text(json.dumps(found,indent=2))
