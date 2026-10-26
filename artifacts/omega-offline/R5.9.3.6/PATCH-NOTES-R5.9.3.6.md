[Original Epic LUT by HWG90](https://github.com/HWG90/Epic-LUT)

# OMEGA Epic LUT Editor R5.9.3.6

- Armour browsing now includes body armour and cape kits from the installed customization table, with verified source texture references.
- Added 16-column cape LUT loading, editing, preset saving and patch export.
- Normal mode previews complete armour sets. Individual pieces are available in Advanced mode, including fullscreen opened from Advanced.
- Helmet selections carry into armour previews. Cape previews retain the last selected helmet and armour. Armour editing hides the cape.
- Recovered the player salute hold through its current state-machine event links and animation resource. The preview applies its still pose to arms/hands and upper back, keeping hips and legs in their source pose.
- Corrected source vertex-index handling that previously omitted an arm.
- Current kit labels take precedence over duplicate legacy unit labels.
- Fullscreen uses the same outfit/individual-piece selection mode as the editor it was opened from.

## Preview limitations

Armour uses source geometry, textures and the existing recovered material shader with preview lighting. Cape shading is simulated from source textures/LUTs and is not exact in-game rendering. Inherited capes use the standard source cape mesh. Cape physics, animated draping and the full cape shader are not implemented. Unverified cape LUT channel meanings remain explicit.

## Validation

Verified current kit coverage, body/cape preset and exported-patch read-back, a full armour/helmet render, helmet-to-armour-to-cape selection retention, Advanced individual-piece mode and GPU rendering. Tests wrote development fixtures only; no game files were patched during verification.

Published as the OMEGA R5.9.3.6 standalone release. The binary is the previously checked local build; this publication refresh updates repository presentation and removes private development references.
