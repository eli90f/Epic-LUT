# Armour-list development preview

- Reads armour, helmet and cape kit references from the installed game's customization table.
- Keeps per-piece LUT overrides and checks texture resources against the archive index.
- Adds 16-column cape LUT loading, preset saving and patch export.
- Lists named kit choices separately when multiple kits share a texture; edits remain shared by texture ID.
- Uses existing SDK archive names. Unmapped names remain explicit Kit IDs.

## Validation

Current installation: all body-kit labels covered (128 distinct labels from 138 table entries); all 123 cape kits represented. 116 cape LUT resources use the 16-column format.
Body and cape preset/patch round trips passed. Existing I/O validation passed, including malformed-input checks. Tests wrote only development fixtures; no game files were patched.

## Preview limits

Body armour is previewed by source piece, not as an assembled outfit. Most cape kits inherit their mesh from the equipped body; these textures remain editable but do not have a standalone mesh preview. Cape shader channel semantics are not claimed as recovered. Approximate preview lighting remains different from the game.

## Dependencies and provenance

`native/gear_catalog.h`: current-file reader, using Filediver's armor_sets.go serialized layout and SHODAN's authenticated sealed-box format. The bundled libsodium reader is extracted only beneath lut-editor-cache/runtime beside the executable. See LICENSE-libsodium.txt.
`native/gear_archive_names.inc`: existing Epic R5.9.3 sdk_catalog.lua archive labels; no new-name research or guessed aliases.

Executable: dist/OMEGA Epic LUT Editor Armour Test.exe. This is a test preview, not a published release.
