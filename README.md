# Aimlock

An FPS aim trainer in the style of KovaaK's, built in C++ on a custom DirectX 12 engine and renderer for a university computer graphics module.

Aimlock has three timed scenarios, one for each core aiming skill. Every run lasts 60 seconds and ends with a score, your accuracy and your personal best.

## Scenarios

| Scenario | Skill | Inspired by | How it plays |
|---|---|---|---|
| **B180** | Clicking | B180 Voltaic | Four small spheres bounce in predictable arcs across a 180° arena. One click kills each one, and it respawns somewhere else. |
| **Controlsphere** | Tracking | Controlsphere | A single small sphere drifts all around you at varying speed and distance, with gentle jumps. Hold fire and keep your crosshair on it. |
| **voxTargetSwitch** | Target switching | voxTargetSwitch | Several strafing targets spread across a wide arc. Track one until it dies, then switch to the next. |

### Scoring

- **B180:** kills, with accuracy (hits / shots) shown alongside
- **Controlsphere:** time on target × 100 (6000 is perfect). Accuracy = time on target / time firing
- **voxTargetSwitch:** kills, with tracking accuracy shown alongside

Personal bests are saved locally for each scenario.

## Features

### Gameplay
- [ ] Main menu, scenario select, pause and results screens
- [ ] HUD: timer, score, live accuracy, crosshair
- [ ] 3-2-1 countdown before each run
- [ ] Sensitivity matching (CS/Apex/Valorant yaw), DPI and cm/360 display
- [ ] Field of view in horizontal degrees
- [ ] Raw mouse input (no acceleration)
- [ ] Hit sounds and visual hit feedback

### Graphics
- [x] DirectX 12 renderer: swapchain, command lists, fences, root signature, PSO manager
- [x] Shader reflection with automatic constant buffers (ring-buffered per draw)
- [x] Blinn-Phong lighting with a directional light
- [x] gamma-correct sRGB output
- [x] .gem model loading
- [ ] Textures with mipmaps and anisotropic filtering
- [ ] Normal mapping
- [ ] Shadow mapping with PCF soft shadows
- [ ] HDR rendering, ACES tonemapping and bloom
- [ ] 4× MSAA
- [ ] Skybox
- [ ] Instanced, additively blended hit particles
- [ ] Stretch: physically based shading, per-pass GPU timings, debug views

## Controls

| Input | Action |
|---|---|
| Mouse | Aim |
| Left click | Shoot |
| Esc | Quit (pause menu planned) |
| F1 | Toggle mouse lock and the debug UI |

## Building

Requirements:
- Windows 10/11 with a DirectX 12 capable GPU
- Visual Studio 2022 or later with the **Desktop development with C++** workload (the Windows SDK provides `d3d12`, `dxgi` and `d3dcompiler`)

Steps:
1. Open `aimlock.slnx`.
2. Select **x64** and **Debug** or **Release**.
3. Build and run. The working directory must be the project root, so that the `resources/` paths resolve.

Shaders are compiled at startup and cached next to the source as `.cso` files.

## Project structure

```
include/Engine/   engine headers: Graphics, Maths, Platform, ImGUI, External
src/Engine/       engine implementation
include/Game/     game code: camera, game objects (scenarios to come)
src/main.cpp      entry point and main loop
resources/        models (.gem), textures, shaders
```

## Third-party code

- [Dear ImGui](https://github.com/ocornut/imgui): menus, HUD and debug UI
- GEMLoader: .gem model loader supplied with the module
