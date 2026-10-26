# OMEGA Epic LUT Editor working rules

- Explain each step and substep briefly when starting it; report material findings or failures promptly.
- Show the full remaining-substeps list only when the user asks for it. Do not repeat it in routine updates.
- Deliver a development preview for user testing before making the release build/package.
- Store this editor's cache in `lut-editor-cache` beside its executable. Do not modify the game's caches.
- Preserve recovered source data and identify missing rendering features. Never describe approximate shading as exact in-game rendering.
- Reuse names from verified existing asset catalogs only when their identifiers/source links match. Unknown names remain explicit.
- Do not search for new entity names in this task. Check only whether existing verified existing asset catalogs names have verified matching identifiers; defer unmatched names to a future user request.
