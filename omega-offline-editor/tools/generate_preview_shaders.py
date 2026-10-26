"""Embed recovered Filediver material shaders, preserving original material logic.
The only shader-body changes add preview-only unlit/selection controls.
Source is BSD-3-Clause, Copyright 2026 Filediver contributors; retain LICENSE-Filediver.txt.
"""
from pathlib import Path
import hashlib
root=Path(__file__).resolve().parent.parent
src=Path(r'research/filediver\cmd\filediver-gui\widgets\previews\materials\shaders')
out=['// Recovered Filediver shaders. BSD-3-Clause, Copyright 2026 Filediver contributors.','#pragma once','namespace omega::recovered {']
for fn,symbol in [('lut.frag','lutFragment'),('object.frag','fallbackFragment'),('standard.frag','standardFragment'),('integrate_ibl.frag','iblFragment')]:
 s=(src/fn).read_text(encoding='utf-8').replace('#include "lighting.frag"',(src/'lighting.frag').read_text(encoding='utf-8'))
 out.append('// '+fn+' SHA256 '+hashlib.sha256((src/fn).read_bytes()).hexdigest())
 if fn!='integrate_ibl.frag':
  s=s.replace('out vec4 fragColor;','out vec4 fragColor;\nuniform bool omega_selected;\nuniform bool omega_unlit;\nuniform int omega_selected_row;')
  last=s.rfind('}')
  unlit='vec4(base_color.rgb,1.0)' if fn=='lut.frag' else ('vec4(color.rgb,1.0)' if fn=='standard.frag' else 'vec4(albedo,1.0)')
  selected='omega_selected && material_lut_row==uint(omega_selected_row)' if fn=='lut.frag' else 'omega_selected'
  s=s[:last]+f'\n if(omega_unlit)fragColor={unlit};\n if({selected})fragColor.rgb=mix(fragColor.rgb,vec3(1,.18,.3),.10);\n'+s[last:]
 out.append('static const char* '+symbol+'=')
 for i in range(0,len(s),6000):out.append('R"OMGL('+s[i:i+6000]+')OMGL"')
 out[-1]+=';'
out.append('}')
(root/'native/generated_preview_shaders.h').write_text('\n'.join(out),encoding='utf-8')
