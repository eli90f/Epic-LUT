# Full outfit preview — development test

## Implemented

- Assemble current kit body pieces in their source coordinate system and body variant.
- Use a current set-ID helmet match until the user selects a helmet.
- Remember the selected helmet and armour while switching to cape editing, including an asset refresh in the same game folder.
- Hide cape geometry during armour editing. Add the selected cape over the remembered armour/helmet when editing the cape.
- Show assembled outfits in the normal editor and its fullscreen view.
- Make explicit individual-piece entries available in Advanced mode and fullscreen entered from Advanced.
- Show inherited capes on the standard source cape mesh resolved from current armour kits.
- Correct group index ranges which previously dropped a source arm.

## Source salute recovered with leanrea

Read-only inventory: 448 distinct state-machine resource versions inspected. Initial named matches were NPC salutes and were not used as the player pose.

The current player avatar state machine links action_salute_01 from Action_Empty to clip 070b422612518deb, and action_salute_immediate to looping clip e32b270524630569. The loop has 90 initial-transform channels, matching the 90 hashes in the avatar bones resource. Evidence: build/armor-preview/salute-research/player-salute-links.json and the local clip/rig probe files.

The preview reads the current clip and bone list from the selected installation. It decodes the initial hold frame, resolves channel hashes to unit joints, follows source skin-palette remaps and blends source skin weights. User-requested mask: arms/hands plus spine/chest/clavicles; hips and legs retain their bind pose. This is a still frame, not an animation player.

## Validation

Current-game render test passed for a complete 15-piece armour/helmet combination and the same combination with a cape. The test asserts that a separately chosen helmet remains present, the cape adds geometry, and Advanced single-piece mode contains fewer vertices. GPU rendering passed. Test writes stayed under the development folder; the game was not patched.

## Material limits

Armour uses the existing recovered material shader and source textures, with preview lighting. Cape material shading is explicitly simulated using source textures/LUTs; it is not the full recovered game cape shader. Cape LUT channel meanings remain unverified, and the selected cape channel is shown as a material sample. No cloth physics, animated cape draping or exact in-game lighting is claimed.

Executable: dist/OMEGA Epic LUT Editor Outfit Test.exe. No release publication.
