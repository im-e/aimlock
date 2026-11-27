The full plan is in `aim-trainer-roadmap.md`, this is just the short list.

- **Shared descriptor heap + root signature tables/samplers.** Needed before textures and the UI font atlas.
- **Textures.** UVs and tangents in `Vertex` and the input layout, SRVs in the shared heap, mipmaps. `uploadResource` already has the `texFootprint` branch to build on.
- **UI renderer.** Screen-space quad shader + PSO, font atlas text, then buttons/sliders/checkboxes. Hook a draw function onto each game state.
- **Scenarios.** `Scenario` base class, then B180, Controlsphere and voxTargetSwitch. Move the placeholder target out of `Game`.
- **Save best scores** to `scores.txt`.
- **Vsync toggle** with tearing support, aim trainers want low latency.
- **Instancing.** For the hit particles.
- **Skybox.**
- **Window resize** (resize the swapchain on `WM_SIZE`). Nice to have, fine to leave until near the end.
