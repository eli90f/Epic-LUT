# OMEGA Epic LUT Editor R5.9.3.5 — User manual

## Edit and install
1. Run `OMEGA Epic LUT Editor R5.9.3.5.exe`. The app finds HD2 through Steam’s registered libraries and app manifest, on any drive. If it is not found, select its game folder or `data` folder. **Select HD2 folder** remains available to choose another installation; the Files toolbar contains the same control in fullscreen.
2. Choose **Armor**, **Weapons**, **Vehicles** or **All assets** above the list. Search by name and select an item. Identified items appear before unknown resources.
3. Choose a material row above the preview. Select the color layer below it, then use the inline color picker. Alpha and unrelated fields are preserved. Several rows or textures can belong to the same item; a shared texture can affect several items.
4. Switch items to make more edits. The app keeps edits by texture ID during the session. The edited-texture count appears below the list.
5. Close Helldivers 2 normally. Click **Install into game**. It installs all staged appearance edits and displays a success or failure message. Start the game to test the result. Editing files does not change a running game's already loaded textures.
6. Click **Restore game files** to remove the patches installed by this editor. The app then clears staged edits and reloads the current game files.

**Reset this item** resets the selected LUT to its loaded original baseline. **Restore game files** removes all patches owned by this editor. Other mods and original base archives are preserved.

**Save OMEGA preset** saves the selected LUT, including exact RGBA values and baseline metadata. Unsaved session edits are not automatically saved on exit. Successfully installed values are retained in the install State folder and loaded on later app starts. Keep the `State` folder beside the EXE so ownership and Restore continue to work. Errors are logged in `Logs\errors.log` beside the EXE.

## Advanced panel
Click **Advanced >** for the exact LUT grid, all raw RGBA cells, column descriptions, DDS import/export, patch-only export, source details and checker background. **< Back to simple editor** hides those tools while preserving the values.

Typed finite float32 values update the preview immediately. Negative and HDR values are preserved. The standard RGB color picker is limited to 0–1; use Advanced for values outside that range. Unknown channels remain editable without an invented effect description. Undo/Redo apply to the currently selected LUT.

**Open DDS** opens a detached texture. **Replace selected LUT DDS** copies matching dimensions into the selected game target while retaining its resource identity. A detached DDS cannot install or export a targeted patch without a current target. **Export game patch** exports the selected texture only; Install applies the whole staged set. Export folders are created under `Exports` beside the EXE.

## Original Epic presets
Load preset accepts OMEGA `.olut`/`.omega-lut` and original **EPIC-LUT v2 `.dbflut`** files. The Epic selector switches imported LUTs. Supported imported LUTs that match exact current resource IDs and dimensions are staged for install; unmatched targets stay detached and are counted in the import report.

All supported raw RGBA cells are parsed. Original equipment/body/enabled/effects/row flags are read as metadata; their original live activation and targeting behavior is not implemented offline. Source preset files remain unchanged. RGB-only v1, cape-specific 16-column LUTs and outfit ZIP manifests are explicitly unsupported. Saving an OMEGA preset saves the selected LUT, not an entire multi-LUT outfit.

## Preview and item limits
The preview loads source meshes, UV layers, textures, masks and recovered material shaders. Click a region to select its mask-defined LUT row. Preview lighting/seed, static pose and scene context can differ from the game; check final appearance in-game. Exact file values are preserved independently of display rendering.

This edits supported material properties, not gun fire rate, vehicle movement, damage, models or animation. The current game scan found 2,509 supported LUTs. Of these, 455 have verified owner names: 309 weapon/equipment links, 42 vehicle links and 107 armor/helmet links, with shared categories overlapping. These are texture counts, not item counts. The other 2,054 remain explicitly unidentified. Full shared owner paths and IDs are available under Advanced → Selected asset details.

The installer uses vacant numeric patch slots and an ownership ledger with SHA256 proofs. If installed files have been changed externally, it refuses to overwrite or delete them. Current targets are revalidated before install. Close other tools holding those patch files if a file operation fails. File installation/restore is tested in fixture folders; actual game rendering still needs your test.

## Credits
Epic LUT R5.9.3: HWG90 (Zero-Clause BSD). Archive/format research and MatchYourColors: CowboyBingus (Zero-Clause BSD). Resource-path dictionary: Filediver contributors (BSD-3-Clause). Supplied debug LUT: Plain Furniture. OMEGA branding/artwork: the user's original OMEGA project. Full licenses accompany the package.
