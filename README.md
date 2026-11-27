# Aimlock

An FPS aim trainer in the style of KovaaK's, built in C++ on my own DirectX 12 engine and renderer for a university computer graphics module.

Aimlock has three timed scenarios, one for each core aiming skill. Every run lasts 60 seconds and ends with a score, your accuracy and your personal best.

All of the UI (menus, HUD, crosshair, results and the debug panel) is drawn by a custom UI renderer built from screen-space quads and a font atlas. No UI libraries.

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

Every run is saved to `scores.txt` with its score, accuracy, your cm/360 at the time and the date. Each scenario has a top 10 leaderboard built from that history.

## Current state

The game runs through a state machine: **Main menu → Scenario select → Countdown → Playing → Results**, plus **Settings**, **Leaderboard** and **Paused** (Esc, or if the window loses focus). **Controlsphere** is fully playable and its scores are saved. Until the UI renderer is in, the window title shows the current screen and stats, and the full leaderboard is printed to Visual Studio's Output window.

## Features

### Gameplay
- [x] Game state machine: main menu, scenario select, settings, leaderboard, 3-2-1 countdown, 60 s run, pause, results
- [x] `Scenario` base class, so each scenario only handles its own targets and scoring
- [x] Controlsphere
- [ ] B180
- [ ] voxTargetSwitch
- [x] Hitscan shooting with every click counted (no clicks lost between frames)
- [x] Raw mouse input (no acceleration)
- [x] Sensitivity matching: pick your game's yaw (CS2/Apex, Valorant, Overwatch 2), enter your sens and DPI, and see your cm/360
- [x] Field of view in horizontal degrees
- [x] Settings saved to `settings.txt`
- [x] Every run saved to `scores.txt`, with a top 10 leaderboard per scenario
- [x] Cursor freed in menus and locked while aiming
- [x] Target lights up while you're tracking it
- [ ] Hit sounds

### UI (custom renderer)
- [ ] Screen-space textured quads positioned in pixels
- [ ] Bitmap font atlas text renderer (stretch: baked TTF with proportional advances and kerning)
- [ ] Immediate-mode widgets: buttons, sliders, checkboxes
- [ ] HUD: timer, score, live accuracy, customisable crosshair
- [ ] Menus, pause and results screens, debug panel

### Graphics
- [x] DirectX 12 renderer: swapchain, command lists, fences, root signature, PSO manager
- [x] Shader reflection with automatic constant buffers (ring-buffered per draw)
- [x] Blinn-Phong lighting with a directional light
- [x] Gamma-correct sRGB output
- [x] .gem model loading and procedural sphere meshes
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
| Left click | Shoot (hold to track in Controlsphere) |
| Enter / Space | Main menu: play. Scenario select: start. Results: retry |
| Up / Down | Pick a scenario or a setting |
| Left / Right | Change a setting (hold Shift for bigger steps), switch leaderboard |
| S | Settings (from the main menu or the pause menu) |
| L | Leaderboard (from scenario select or results) |
| Esc | Back. Pauses or resumes during a run, quits from the main menu |
| Q | Quit the run while paused (not saved) |

These keyboard shortcuts get replaced by proper buttons once the UI renderer is done.

## Building

Requirements:
- Windows 10/11 with a DirectX 12 capable GPU
- Visual Studio 2022 or later with the **Desktop development with C++** workload (the Windows SDK provides `d3d12`, `dxgi` and `d3dcompiler`)

Steps:
1. Open `aimlock.slnx`.
2. Select **x64** and **Debug** or **Release**.
3. Build and run. The working directory needs to be the project root so the `resources/` paths resolve.

Shaders are compiled at startup and cached next to the source as `.cso` files. `settings.txt` and `scores.txt` are created in the project root the first time you change a setting or finish a run, and are git-ignored.

## Project structure

```
include/Engine/   engine headers: Graphics, Maths, Platform, External
src/Engine/       engine implementation
include/Game/     game headers: Game (state machine), GameState, Scenario, Settings, ScoreBoard, Camera, GameObject
include/Game/Scenarios/   one header per scenario
src/Game/         game implementation (src/Game/Scenarios/ for the scenarios)
src/main.cpp      entry point, main loop and scene drawing
resources/        models (.gem), textures, shaders
```

## Third-party code

- GEMLoader: .gem model loader supplied with the module
