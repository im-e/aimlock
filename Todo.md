
### 3. Coming in later lectures

- **Textures.** You'll need UVs in `Vertex` and the input layout, a descriptor heap with SRVs (shader resource views), a sampler, and a descriptor table in the root signature. Your `uploadResource` already has the `texFootprint` branch for this. Check whether `GEMLoader.h`'s vertex structs carry UVs before you change `Vertex`.
- **Skeletal animation.** The TRex goes through your `verticesAnimated` path, but you only read position and normal from it. Animating it needs bone indices and weights per vertex, plus a bone matrix array in a constant buffer.
- **Instancing.** If your game has lots of trees or rocks (the acacia model, for example), drawing many copies in one call with per-instance data saves a lot of draw calls.

### 4. Quality-of-life, to save time later

- **Dear ImGui.** A debug UI for tweaking light direction, camera speed and object transforms live is extremely useful in a game project. Check that external libraries are allowed in your brief first.
- **RenderDoc.** Capture a frame now while things are simple, so you know what "correct" looks like when something breaks later.
- **Window resize** (resize the swapchain on `WM_SIZE`). It's a nice-to-have, and fine to leave until near the end.

