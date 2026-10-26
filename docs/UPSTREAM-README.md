# Epic LUT R5.9.3
Single DDS and patch filename options, Name only exports, and the Choose LUT Patches fix. [Release notes](docs/RELEASE-R5.9.3.md).

**STYLISH FREEDOM. Defend Freedom in Style.**

Paint your Helldiver in-game. Match your helmet to your armor, build a color scheme from scratch, or get into the full material table. R5.9.3 reduces background work with event-driven recovery and cached, cooperative Shared LUT publication. It includes input handoff support for updated Photo Mode builds, and retains expanded **Shared LUT** appearances for compatible squadmates and adds checked-region camo, one-click gear mapping, an embedded Pattern editor and patch exports that preserve verified assignments. Player Preview keeps its default idle pose. Saved presets support up to 4,096 bindings for Transmog setups. [What's new](docs/RELEASE-R5.9.3.md).

[Download R5.9.3](https://github.com/HWG90/Epic-LUT/releases/tag/R5.9.3) | [All three packages on GitHub](https://github.com/HWG90/Epic-LUT/releases/tag/R5.9.3) | [Report bugs](https://github.com/HWG90/Epic-LUT/issues)

## Install

Choose one package:

- **BSL (Nexus Main File):** import `Epic-LUT-R5.9.3-BSL.zip` through Arsenal/HD2MM and enable the startup option with **Bingus Shared Loader v15+**.
- **LLL (GitHub only):** extract the complete `armor_lut_editor` folder from `Epic-LUT-R5.9.3-LLL.zip` into your loader's Mods directory. LLL R30+ uses `%LOCALAPPDATA%/LLL/Helldivers2/Mods`. Enable Epic LUT. [Modular installation details](docs/LLL-MODULAR.md).
- **MDL (Nexus Optional File):** use `Epic-LUT-R5.9.3-MDL.zip` with its bundled API-2 `mod.lua` entrypoint. Place the complete `armor_lut_editor` folder in MDL's configured Mods directory. [MDL installation details](docs/MDL-LLL.md).

The modular LLL package uses `main.lua` and requires LLL R30+. LLL can also load the MDL-compatible bundled package. MDL needs the bundled package; it cannot use the modular LLL package.

Enable one Epic LUT entrypoint. Disable the separate **epic_player_preview** test addon: preview is included.

No Python or MCM installation required. Windows PowerShell/.NET handles file picking and archive reads. **RAR requires installed 7-Zip.** Turn **Match Your Colors matching Off** before editing the same gear.

## Shared LUT — your look, visible to your squad

**Shared Lobby LUT** shares your currently applied Armor, Helmet and associated Pattern edits with compatible Epic LUT squadmates. Apply a saved Armory preset to share that look. Supported tables retain their exact RGBA, material, camo and emissive channels. Improved lossless compression and table/value deduplication fit larger appearances, with support for up to 4,096 bindings within the lobby budget.

Both players need R5.9.3, with **Configuration > Share full LUT appearance with Epic LUT users** enabled. Its current default is On; the same toggle controls sending and receiving. Turning it Off removes your advertisement and restores received appearances still owned by Epic LUT. Updates wait for a stable squad lobby and settled edits, then apply only to the sender's matching body, Armor and Helmet kits. Your own outfit can be different from theirs.

Cape material LUTs are excluded from sharing in this build. Unmodified games and Match Your Colors-only clients do not interpret the packets. Oversized appearances stay local with a visible status while receiving remains available. **Shared Lobby LUT remains in testing; two-player visual confirmation is still required for this build.** [Sharing details](docs/UPSTREAM-MYC-INTEGRATION.md).

## Pattern LUTs and row tools

**LUT Editor** includes three **Quick Editor: Pattern LUT** swatches directly beneath the material grid, labeled **Color**, **Mat/Opac** and **Roughness** above their selectors. **Load Current Gear** prepares their current values; click a swatch to edit it. The third column's **Alpha controls roughness**; its ordinary picker edits Alpha while retaining the unmapped RGB values. Choose **Pattern LUT Editor** to replace the table with the full Pattern workspace, including raw channels, undo/redo, import, DDS export and Pattern Patch ZIP export. Armor or Helmet returns to the material table without discarding Pattern edits. **Show Alpha** is shared. [Pattern controls](docs/PATTERN-LUTS.md).

The **Rows** tools preview the selected saved row preset beside their controls. Narrow windows place the preview below the controls with scrolling. Previewing leaves the editor unchanged; use **Apply preset to row** when ready.

## Roomier LUT Editor

The Pixel Grid uses the full height above a compact action bar. **Hide Values** gives it the inspector's width; **Show Values** restores the numeric editor. The setting is remembered.

Docked **Tools** starts open on **Import**; saved undocked Tools starts closed. Its **Import**, **Rows**, **Export**, and **Options** tabs appear once inside the docked or floating panel. Controls stay compact as the window widens. **Scratch** opens a separate brush palette with color and alpha controls; it can stay open alongside the toolbox. Export keeps the shared format selector and timestamped filenames.

**Tools > Options > Load Debug LUT** loads the bundled 23x8 Debug table by **Plain Furniture** into the editor. Each load starts with a fresh copy; use the Apply controls to send it to gear.

For one-click DDS export of every custom LUT currently applied to your gear, choose **Tools > Export > All Custom LUTs (DDS)**. Pick **LUT# + HEX**, **LUT# + Decimal**, **LUT#**, **HEX**, or **Decimal** naming, then press **Export**. The export name prefixes every file, for example `HONK LUT3-d3ce605892d5331b.dds`. Each batch gets a new timestamped folder under the export location. Pattern LUTs are included as `PatternLUT#`; unknown IDs use the LUT number. The Cape and Pattern export options control companion inclusion.

To export a saved Armory collection without equipping it, select the preset and choose **Raw DDS (entire preset)**. **Save Current Gear Preset** keeps the checked Armor, Helmet and Cape LUTs with their associated Patterns in the app for later reuse.

Closing or popping out Player Preview returns its reserved grid width. Tools and Scratch can stay open while the Pattern workspace replaces the material table.

The active tools shortcut stays highlighted. Clicking it again keeps its window open; use that window's close button to close it. The LUT dropdown sits indented below Helmet/Armor, with Pattern Editor beside those gear buttons. Settings and the footer share one UI scale. Windows and popouts have visible borders.

## Larger tables and custom rows

Material LUTs with up to **64 rows** remain editable. Scroll over the Pixel Grid or use its row-page buttons; choosing a row brings it into view. Basic's region list and the Value Editor scroll separately. Region labels flash the actual selected row on the model when that material uses it.

Continuous material edits coalesce to 15 live uploads per second and apply the final value immediately on release. Retained texture data stays bounded to 64 MiB and 16,384 allocations. Reloading Epic LUT reclaims eligible textures after its preview and live bindings finish cleanup. Restore alone preserves Undo/Redo; older allocations or uncertain ownership remain retained until Helldivers restarts. A limit refusal keeps the local draft available.

Tables above eight rows retain their added rows when Preserve Original Emissives is enabled. Original values are preserved for the rows present in the game snapshot; extra rows keep the imported values. DDS and patch export retain the custom row count. The model must reference those rows for them to be visible.

## Start with Basic

Press **F9**, then **Load Current Gear** to load Armor, Helmet and Cape. Choose the LUT above either column, then click a region color to edit it. Colors apply live to that table; alpha and material values remain intact. The layout prioritizes all eight regions; larger tables scroll. Gear selectors stay gray until current or imported LUTs are ready. Gold Load buttons pulse until their first use.

- Click a **Region** label to flash it magenta on your character. **Stop Highlight** ends it immediately.
- **Copy Helmet / Copy Armor** transfers the selected table to the opposite gear. **Copy Helmet to All** transfers it to every worn Armor LUT.
- After importing, **Apply Imported Texture** appears directly below each destination LUT selector. Use the button's arrow to choose the source texture, then press **Apply [filename]** to apply it to the LUT selected above. Choosing a texture updates both buttons without applying it.
- A **Camo** checkbox sits beside each region, initially unchecked. Check the regions to edit, then choose **Off / Pattern 0–5**, a generated **Jungle, Desert, Urban or Arctic** colorway, or one of the four camo colors. These controls affect only checked regions as one undoable edit; unchecked rows retain every value. The color swatches above the selectors show the first checked region. Pattern selection initializes dormant camo; colorways supply a complete setup using the Match Your Colors camo recipe and keep an existing active mask. Editing a camo tint switches its colorway label to **Current / custom**. Checking Helmet Region 8 explicitly includes it. Base colors, shader mode and emission remain intact. Neutral thumbnails use the actual game masks; the selected material's shader determines how camo appears.
- **Advanced Mode** in the header opens the full editor; **Basic Mode** brings you back.
- Hover for guidance. Extra import controls appear only after loading a file.

## Import and apply

1. Choose **Import DDS / ZIP / RAR**. Expand archive folders and choose a gold variant folder containing LUTs. Import loads colors without installing archive content.
2. Choose the imported table: `lut001.dds` is LUT 1. Its swatch strip previews the primary colors.
3. In Basic, choose the destination LUT above either gear column, then use **Apply Imported Texture** immediately below it. The main button shows the source filename; its arrow changes the source.
4. Use **Map LUTs to Gear** in Basic or Import / Apply. Choose an imported table, then click a slot in the Armor, Helmet or Cape column to apply it. Check the gear columns that **Auto-map** should affect. It orders numbered imports numerically: `lut000` goes to slot 1, `lut001` to slot 2, and imports repeat from the beginning when there are more destination slots. Each checked gear column starts from the first import. Pattern tables stay outside this material cycle; verified accompanying Pattern data is applied with its matching material. Each mapping action supports Undo.
5. **Apply Matching LUTs** maps patch tables by resource ID to your worn gear. Unmatched, unidentified and duplicate-ID entries stay untouched.

Press **F10**, then **Import / Apply** for full table previews. Every loaded Armor and Helmet LUT appears in its own scrollable column. Click a table's title or texture hash to select it; selecting a title leaves its colors unchanged. **Send to LUT Editor** copies the selected imported table into the editor.

Matching uses texture IDs, not armor names or table order. Archives can contain gear you are not wearing; use manual targeting when no IDs match.

Applied Armor, Helmet and Cape material LUTs, plus Pattern LUTs, are retained during loading gaps and automatically restored to matching worn gear when the game recreates its materials. Recovery reuses the applied texture without changing its values. Different gear and bindings owned by another writer are left untouched; Restore clears the retained appearance.

## Pick, paint, copy

On Import / Apply swatch previews:

- **Left-click:** select.
- **Double-click:** edit color.
- **Right-click:** paint the held Quick Scratch color into that cell.
- **Middle-click:** copy into Quick Scratch.

Painting targets the clicked gear table. **Undo Last Action / Redo Last Action** includes imports and gear applications. The LUT Editor also provides pixel-edit undo/redo and copy/paste.

**Quick Scratch** opens a movable color panel with ten session swatch slots. Pick a color once and paint it where you need it. Scratch Copy/Paste uses system clipboard hex colors; the color picker HEX field also supports Ctrl+C/V.

## LUT Editor

Choose **Load Current Gear**, then **Armor LUT** or **Helmet LUT** and a table. Loading reads every current worn LUT, including Cape and stock-game values, and initializes the quick Pattern swatches. The Import toolbox offers a selected-table Apply action and **Map LUTs to Gear** for explicit imported-table assignment or Auto-map.

The labeled grid covers all **23 columns**, with a collapsible row value editor. Edit RGB, type floats, use material/camo selectors, or unlock advanced channels. Bump-map choices show R0-R25 alongside their material names, in actual column-2 R-value order. Drag in Select mode to select a rectangle; Shift-click also extends it. Ctrl+C/V copy and paste the grid selection, Ctrl+Z undoes an edit, and Ctrl+Shift+Z (or Ctrl+Y) redoes it. Click or drag row labels in Select mode to select whole rows; Shift-click extends the row range. Copy, select the destination rows, and paste from their top-left row. Full-row paste includes every RGBA channel and requires Advanced editing. Selected swatches keep their colors inside an outline. Grid tools also support drawing and moving pixels. Double-click color cells opens the picker; middle-click also copies the full pixel.

Choose **Armor LUT**, then select a **Cape LUT** entry from its dropdown to edit the worn cape's 23-column material table with the regular grid, value editor, tools and shortcuts. Turn off **Include Capes in Armor Exports** in Configuration or Export tools to leave capes out of Armor collection exports; exporting a selected Cape LUT still works.

Click the texture ID beside the LUT dropdown to give it a custom name. Names are saved locally by the exact texture ID. Hover the name to see its original hash; save an empty name to show the hash again.

Use **Tools → Rows → Import Paydex JSON...** to choose a Paydex row-presets JSON file. Imported rows appear immediately in the preset selector. Choose a destination row and press **Apply preset to row** separately. The importer preserves full RGBA/material values, reuses identical existing presets, and gives conflicting names a suffix instead of overwriting them.

Tools remembers your last **Dock / Undock** choice across reloads. It starts open on **Import** when docked; saved undocked Tools start closed. Manual visibility is retained while the editor stays loaded. Drag the horizontal divider between the table and Tools up or down to adjust their space; the split is retained when resizing or redocking. The docked Tools body scrolls independently when space is limited.

Press **Delete** in the material grid to reset the selected cells or rows to the loaded original values as one undoable edit. With no range selected, it resets the focused cell. Delete inside a text field still edits its text.

Select multiple rows through their row labels, or select their first-column cells, then **Apply preset to selected rows** to fill every selected row with the preset as one undoable edit. Use the **X** beside a saved row, palette or Armory preset to remove it from the library. Removed files remain recoverable; the applied appearance is preserved.

The Pattern LUT Editor occupies the table workspace while Tools and Scratch remain available. Its controls scroll independently when space is limited.

Select a Value Editor channel, then use Copy Value/Paste Value to transfer its raw number. Clipboard paste respects the advanced editing lock. Typed numbers commit when clicking away, before another row or column becomes active; invalid values stay in the field for correction, and Escape cancels. Text fields support click/drag selection, Shift+arrow selection, Home/End, and Ctrl+A/C/X/V while typing. Held letters, Backspace/Delete and navigation keys repeat after a short delay.

Typed values can exceed recommended slider ranges. Swatches clamp RGB for viewing; stored floats remain intact. Export edited DDS files to `%LOCALAPPDATA%/Epic LUT/files/exports` to share them.

## Player Preview

Use **Player Preview** or **F6** to see your worn character while editing. It docks beside the LUT Editor and can pop out on other pages. Armor and Helmet edits update the copied model live.

- Left-drag pans; right-drag rotates; mouse wheel zooms.
- Drag the title to move a floating panel; drag its corner to resize.
- **Pop Out / Dock** changes placement.

A footer pose selector provides **Idle**, **Salute**, **Wave**, **Taunt** and **High Five**. Idle remains the default. Emotes hold a real game clip frame, loaded locally on first selection; changing poses retains your camera and current LUT edits. Continuous preview animation remains a separately named development build. [Pose details](docs/PREVIEW-ANIMATION.md).

Menu and Player Preview native rendering pause when the game loses focus or its display dimensions are invalid, and resume when foreground rendering is ready. Close the preview or editor before switching game screens.

SDK names come from a compact, pinned Community Edition catalog. Unknown resources and shader parents remain unknown. [SDK integration details](docs/SDK-INTEGRATION.md).

Preview uses the game's lighter **Game Default UI pipeline**, with an independently owned model and portrait target. [Full Render vs Game Default](docs/PREVIEW-RENDER-PATH.md).

## The Armory

**Save to Armory / Save Current Gear Preset** provides independent **Armor**, **Helmet** and **Cape** checkboxes, initially all checked. Choose the parts to include, press **Save**, then name the preset.

Select a preset to preview it. Name-selector swatches give a quick glance. Compact Armor, Helmet, Cape and Pattern lanes each have independent wheel and scrollbar controls. Identical tables within the same gear family share one preview with a count; every saved binding remains available for application and export. Choose which parts to apply to mix and match. Type labels show included gear. **Rename Preset** changes its name; **Delete Preset** asks before removing it from the library.

**The Armory** provides independent **Armor**, **Helmet** and **Cape** checkboxes above one **Apply Preset** button. Parts available in the selected preset start checked; unavailable parts are disabled. Apply changes only the checked parts as one undoable action. Older presets with Armor-labelled Cape entries remain usable when their destination keys match the current Cape exactly.

When naming a saved preset, start typing an existing name and press **Tab** to complete its suggested match. Completion changes the draft; **Save Preset** or Enter saves it.

Basic mode uses one **Apply Preset...** button to open a two-column LUT chooser. Select individual Armor, Cape and Helmet LUTs or use **Select All**. Matching Pattern LUTs are included with the selected material LUTs.

If the current piece has an extra LOD mesh, an unmatched destination can use the saved piece/material palette only when every saved mesh in that family has identical palette bytes and an exact live match confirms the current texture resource and dimensions. Conflicting palettes and unproved destinations stay unchanged. Application logs distinguish exact and adapted mesh matches after binding readback succeeds.

Presets live in `%LOCALAPPDATA%/Epic LUT/presets`. Library changes leave worn colors unchanged.

To share a saved look, select it in **The Armory**, enter an export name and choose a format:

- **Shareable Preset ZIP:** includes every saved Armor, Helmet, Cape and Pattern LUT. Use **Import Shared Preset** in The Armory to add it to another library.
- **Selected LUT Patch ZIP:** packages the selected saved table for a mod manager.
- **Entire Preset Patch ZIP:** packages all tables in that preset together.
- **Raw DDS:** exports the selected saved table for another editor.

Exports use the saved preset and its verified assignments. Different material-bound palettes sharing one texture ID are retained through exact material and mesh-slot replacements. Gear overrides keep their required original texture IDs; conflicting fixed assignments or unproved destinations report an error before publishing files. For an older preset's binding-aware patch export, load the same gear and apply that preset first so its destinations can be verified. Shareable Preset ZIP and Raw DDS remain available without that proof. Filenames receive a timestamp and export folders open through **Open Export Location**. [Patch export details](docs/INTERNAL-PATCH-EXPORT.md).

## Restore and configure

**Restore Arrowhead LUT (Original)** restores original bindings and refreshes displayed colors. **Restore Imported** resets editor values to the imported file.

**Preserve Original Emissives** defaults Off. It preserves original emissive RGBA and primary shader mode, including zeros. Missing snapshots or insufficient source rows produce an error instead of guessed values.

**Configuration** lets you rebind Basic, Advanced and Player Preview shortcuts; adjust UI scale, fonts and window size; and display LUT IDs in hex or decimal. Keys have readable names such as **Insert** and **F9**.

Settings are grouped in two scrollable columns. **Always populate all LUT slots from current gear** loads Armor and Helmet tables after equipment changes. **Turn off Player Preview** remembers your choice and disables its shortcut and automatic dock until you enable it again.

When DBF-MCM is available, **Epic LUT Settings** appears there too. Both menus use the same saved values; its editor shortcuts open Basic or the LUT Editor directly.

**Check for Updates** checks the public GitHub release. Automatic checks are opt-in, once per launch, and never install anything.

## Compatibility

Supports 23-column 2D RGBA16F/RGBA32F DDS, valid mip chains, and ZIP/RAR containing DDS or supported patch resources with GPU/stream sidecars. Mesh-only archives contain no palettes to import.

Native adapters target Steam build **25480438**; game updates may require changes. Packages contain no user settings, presets, logs, snapshots or game textures.

## Credits

### A huge thank you to Scarpheon

A huge thank you to **Scarpheon** for all the time he has dedicated to helping me build a better tool for people.

His testing, bug reports, and continued assistance were vital to getting **Export to Patch** implemented correctly and working reliably. He kept trying builds, checking the results in-game, and helping me work through the problems until we got it right.

And a **massive additional thank you to Scarpheon for mapping every bump map**. He worked through all 26 entries, matched their actual column-2 R values, and gave us useful names instead of a list of mystery numbers. Flat grime, suede, denim, wool, leather, metal, circuitboard: those readable choices are here because he put in the time to identify them.

That mapping makes material editing easier for everyone using this tool. Alongside his repeated testing and Export to Patch work, it is a substantial contribution to what Epic LUT has become. **Scarpheon, thank you for the patience, the detail, and the continued help. You have made this a much better tool for people.**

That work deserves more than a name in a credits list. Epic LUT is a better tool because of the time and care he has put into it. **Thank you, Scarpheon. Your help made this possible.**

An additional thank you to **Scarpheon** for his continued assistance with **Shared LUT testing**. Your help with this update has made a real difference.

**Arkturus425** — thank you for helping test the **Shared LUT Update**.

**Plain Furniture** created the bundled **Debug LUT**.

Made by **Goose**. Thanks to everyone helping make Epic LUT better.

Native adapters and foundational LUT research: [CowboyBingus / Match Your Colors](https://github.com/CowboyBingus/MatchYourColors), under the included Zero-Clause BSD license.

[Paydex LUT Editor](https://github.com/paytonrog/paydex-lut-editor) inspired the semantic controls and layout. Epic LUT's implementation is independently authored.

Thank you to **Shikami** for the **Region Indicator** idea, and [**Lytatroan**](https://www.nexusmods.com/profile/Lytatroan) for the human-made thumbnail.

R6 is in development with weapon LUT editing for primary, secondary and support weapons, weapon presets and weapon import/export. Weapon features are separate from R5. The existing R6 preview is experimental, with no support offered.

## OMEGA offline editor

The standalone C++ OMEGA Epic LUT Editor is available in [omega-offline-editor](omega-offline-editor/README.md), with its R5.9.3.6 application package, matching source archive and patch notes. The original live editor source remains here unchanged.
