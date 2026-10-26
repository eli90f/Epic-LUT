# Textured development preview

The development preview loads verified item geometry, UV0/UV1/UV2, tangent frames, original material parameters, and source texture arrays from installed game files. Compressed textures retain native formats and mip chains. The active edited LUT is uploaded separately; source values, alpha modes, HDR values and negative values are preserved.

The GPU path directly reuses Filediver's recovered LUT fragment shader, normal/detail/camo/mask processing and IBL integration. Non-LUT parts use its source texture fallback. Clicking uses the original ID-mask array and its recovered threshold to select a material and LUT row. Highlighting follows the selected LUT region.

This is not a guarantee of identical game rendering. Lighting, preview seed, static pose and scene context differ from the game. Full native engine shader parity, animations and complete runtime visibility/attachment variants are not verified. Missing texture bindings are recorded in Logs/material-bindings.log beside the EXE. GPU failure is shown by the rendering-status caption rather than presented as a working textured path.

The application-owned geometry and texture caches are in lut-editor-cache beside the EXE. Entries include source/decoder identity, checksum and bounds validation. Game shader-cache files are not modified. Full-screen rendering uses the monitor's current Windows resolution, with no 1000-pixel preview cap.

The color picker is inline in Color / Values. It changes RGB only, preserves alpha, and records one undo step per drag gesture. Floating panels are categorized and draggable; painting is buffered and child-control movement is batched.

Provenance: native/generated_preview_shaders.h contains source hashes and the Filediver BSD-3-Clause notice. tools/generate_preview_shaders.py regenerates the retained shader bundle. Original Epic LUT credits and notices remain alongside the test EXE.
