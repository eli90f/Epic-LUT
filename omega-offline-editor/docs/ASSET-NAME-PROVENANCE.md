# Asset owner labels and current disk validation

The editor resolves named gun, vehicle and gear LUTs through the current disk Unit -> Material -> Texture graph. No game process, injected module or old live address is used.

The compiled input contains 2,001 exact Helldiver paths from Filediver's retained hash dictionary and 127 factual gear unit IDs/names from original Epic LUT R5.9.3 src/gear/sdk_catalog.lua (SDK revision 5a886256e54db52b5d228335eae20ce796f87fc0). Paths are hashed with Stingray Murmur64A seed zero. Names are accepted only when the exact ID identifies a current Unit TOC entry. Original archive names are never substituted as texture owners.

For each named Unit, current serialized material offsets, counts and bounds are checked inside the resource. Each referenced material must resolve to a current Material TOC entry in its owning or supported shared archive. Its bounded texture-slot table must refer to a current Texture resource already validated as a supported native float LUT. Only explicit MaterialLut and PatternLut slot links are accepted. This follows original tools/game_catalog.py serialization, with current-file range, type and destination-metadata checks.

Measured current-game results: 2,509 supported LUTs; 455 have validated named owners. Categories contain 309 Weapons/equipment LUTs, 42 Vehicle LUTs, and 107 Armor/helmet/cape LUTs. Counts overlap where a texture has multiple categories. There are 1,189 valid named unit instances and 1,893 supported material-to-LUT links. All proven shared owner paths/names remain in each label; unresolved ownership remains Unknown with its exact IDs. Weapons includes attachments, throwables and equipment, not just guns. These links do not claim each item is currently available in gameplay.

Examples:
- 03a96d02ec5d6790 -> content/fac_helldivers/equipment/primary_weapons/marksman_rifle_vigilance/marksman_rifle_vigilance
- 030b54da2ded92e0 -> content/fac_helldivers/vehicles/combat_walker_missile/combat_walker_missile
- 006fc7ad6ba70e89 retains both laser-rifle and laser-battery owners.

A real named gun LUT and vehicle LUT were each loaded, edited, exported into a new project-local patch folder, and reloaded with exact edited pixels and resource IDs. No installed-game file was written. The source native header is 340 bytes. Current TOC magic 0xf0000011 and the shared patch destination archive 9ba626afa44a3aa3 were verified. Current large raw DSAR chunks are read through bounded seeks rather than loaded in full. Compressed reads remain bounded and validated.

Validation artifacts: build/current-game-assets-proof.json, build/current-owner-graph-proof.json, build/current-lut-owner-graph.tsv. Unsupported formats, other dimensions and unresolved materials are counted explicitly. The graph only covers recovered source names and is not a complete game ownership inventory. Gameplay, multiplayer effects, unit retargeting and live swapping remain unverified or outside scope.

Provenance: native/asset_unit_names.inc records the source hashes.txt SHA256 c9a18597eb2342558aeb548946e1e9de752d912f27f6fac10940ccaa9ae5c0f1. Filediver LICENSE is BSD-3-Clause, Copyright 2026 Filediver contributors. Original Epic vendor decoders carry CowboyBingus Zero-Clause BSD (2026). Preserve the full notices in the release.
