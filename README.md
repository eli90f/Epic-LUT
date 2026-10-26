[Original Epic LUT by HWG90 →](https://github.com/HWG90/Epic-LUT)

# OMEGA Epic LUT Editor

**Standalone offline appearance editor by Ilya — R5.9.3.6**

Edit supported Helldivers 2 material LUTs with a C++ desktop interface, item previews and reversible patch installation. Preserve original-creator credits and licenses.

## Download

[**Download the latest OMEGA release**](https://github.com/eli90f/Epic-LUT/releases/tag/OMEGA-R5.9.3.6)

The release includes the application ZIP, matching editable source ZIP, patch notes and SHA-256 checksums.

## Features

- Browse helmets, body armour, capes, weapons and vehicles.
- Edit supported LUT colours and material values with simple and advanced controls.
- Preview complete armour sets, retain the selected helmet and show the selected cape with the outfit.
- Use fullscreen previews, mouse rotation and movable toolbars.
- Stage appearance edits, install them with **Install into game**, and remove owned patches with **Restore game files**.
- Detect the Steam game installation or choose another HD2 folder manually.

## Getting started

1. Download and extract the application ZIP from the release.
2. Run `OMEGA Epic LUT Editor R5.9.3.6.exe`.
3. Confirm the detected HD2 folder or select it manually.
4. Select an item and adjust its supported material values.
5. Close HD2, then use **Install into game** to apply staged edits.

Keep the app's `State` folder so it can track and restore its installed patches. Cache files are stored in `lut-editor-cache` beside the executable.

## Preview accuracy

Previews use recovered source geometry, textures and material information with editor lighting. They are **not guaranteed to reproduce the complete in-game renderer**. Cape shading is simulated; cape cloth physics and animated draping are not implemented. Check the final appearance in-game.

## Documentation and source

- [Current patch notes](omega-offline-editor/docs/PATCH-NOTES-R5.9.3.6.md)
- [User manual](omega-offline-editor/docs/MANUAL.md)
- [Technical details](omega-offline-editor/docs/TECHNICAL.md)
- [Preview limits](omega-offline-editor/docs/PREVIEW-LIMITS.md)
- [Editable C++ source](omega-offline-editor)
- [Report an issue](https://github.com/eli90f/Epic-LUT/issues)

The OMEGA standalone editor is under `omega-offline-editor/`. The original upstream project is retained at the repository root for history and attribution; its original instructions are in [Upstream README](docs/UPSTREAM-README.md).

## Credits

- **Original Epic LUT:** [HWG90](https://github.com/HWG90/Epic-LUT).
- **OMEGA standalone editor and design:** Ilya.
- **Archive/format research and MatchYourColors:** CowboyBingus.
- **Resource-path dictionary and format/shader references:** Filediver contributors.
- **Supplied debug LUT:** Plain Furniture.

See the preserved upstream license and the licenses bundled with the OMEGA source and application.
