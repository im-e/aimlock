
- **Textures.** You'll need UVs in `Vertex` and the input layout, a descriptor heap with SRVs (shader resource views), a sampler, and a descriptor table in the root signature. Your `uploadResource` already has the `texFootprint` branch for this. Check whether `GEMLoader.h`'s vertex structs carry UVs before you change `Vertex`.
- **Skeletal animation.** The TRex goes through your `verticesAnimated` path, but you only read position and normal from it. Animating it needs bone indices and weights per vertex, plus a bone matrix array in a constant buffer.
- **Instancing.** If your game has lots of trees or rocks (the acacia model, for example), drawing many copies in one call with per-instance data saves a lot of draw calls.
- **Skybox.** 
- **Tesselation Shader**
- **Window resize** (resize the swapchain on `WM_SIZE`). It's a nice-to-have, and fine to leave until near the end.


