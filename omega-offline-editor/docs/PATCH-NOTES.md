# Patch notes — R5.9.3.5

- Remove the fixed developer game path from every application operation.
- Detect Steam through Windows registration, enumerate all libraryfolders.vdf locations, and use appmanifest_553850.acf's actual installation directory.
- Support current and legacy library metadata, spaces and Unicode paths, and moved installations.
- Add Select HD2 folder as an always-available control, including the fullscreen Files toolbar, even when detection succeeds.
- Accept the HD2 installation root or its data folder, validate the target, and remember an explicit user selection locally beside the EXE.
- Route scanning, previews, preset rebinding, patch export, Install and Restore through the same selected location.
- Packages include no developer-specific saved game location. Game files were not modified by validation.

# Patch notes — R5.9.3.4

- Native-resolution textured item preview using source meshes, UV layers, tangent frames, textures and recovered material shaders.
- Inline HSV color picker; continuous RGB edits preserve alpha and create one undo step per gesture.
- Click source-mask regions to select the matching material/LUT row and show a selection overlay.
- Fullscreen at the active Windows monitor resolution, with per-monitor DPI/display-change handling.
- Categorized draggable panels, compact action icons and hover tooltips.
- Double-buffered painting and batched control movement to remove panel-drag trails.
- Save and restore toolbar positions in lut-editor-cache/toolbar-layout.ini beside the EXE.
- Checkerboard behind the edited item and fullscreen workspace.
- Source-aware geometry/texture caches beside the EXE; no game-cache changes.
- Reuse verified existing asset aliases for 199 already-indexed units.
- Retain original credits, file-only install/restore and error logs.

Preview lighting/seed, static pose and scene context can differ from the game; exact engine parity remains unverified. See PREVIEW-LIMITS.md.

# Patch notes — R5.9.3.3

- Automatically populate the item list at startup and select an identified item.
- Put identified items first and show readable names in the simple view.
- Expand the normal item list and keep categories/search beside it.
- Add a simple appearance editor with raw grid, RGBA fields, DDS tools and source details behind Advanced.
- Fix the advanced category dropdown overlapping the Back button.
- Preserve edited textures while switching items; show an edited-texture count.
- Explicitly loaded presets now take precedence over older staged values.
- Add Install into game for all staged edits and Restore game files for editor-owned patches.
- Validate current source baselines and track owned patch files with SHA256 proofs; use new vacant patch slots and roll back handled transaction failures.
- Reload current game files after Restore. Release cached archive handles before installing/restoring.
- Keep full original float data, native single-EXE operation, error logging, original credits and the approximate in-app preview.

The original R5.9.3.1 backend checks found 2,509 supported LUTs, with 455 verified owner names. Actual named gun and vehicle patch exports reloaded with exact edited values. The in-game shader and final rendering are not reproduced by the app preview.
