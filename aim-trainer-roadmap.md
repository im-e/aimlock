# DX12 Aim Trainer: Roadmap

Three scenarios (B180, Controlsphere, voxTargetSwitch), each with a 60 s timer, a score and a results screen, built on my own DX12 engine. Hand-in is roughly two weeks from 26 Nov (around 10 Dec).

The order is chosen so there's always **something I could hand in**, and each graphics feature builds on the one before it.

---

## Progress log

**27 Nov**
- ImGui removed completely: `DebugUI`, the ImGui sources, the WndProc hook, the vcxproj entries and `imgui.ini`
- Camera cut down to what an aim trainer needs: fixed position, mouse look only. Walking, sprinting and flying are gone
- Sensitivity now uses CS/Apex units (0.022°/count), FOV is horizontal degrees (default 103) converted to vertical
- Test models (bunny, T-Rex, trees) removed from the scene. Placeholder room is a floor and back wall from the cube
- Procedural unit sphere (`Mesh::createSphere`) for targets
- Per-vertex colour dropped from `Vertex` (the shader never read it)
- Input: `keysPressed` (edge, ignores key repeat), `leftClicks` counter, `focused` flag
- **Game state machine started** (`Game`, `GameState`): Main menu → Countdown → Playing → Results, plus Paused. One placeholder target with hitscan (`MathUtils::raySphere`), kills/accuracy, in-memory best score. Until the UI renderer exists the window title shows the current screen
- Comments and code tidied

**27 Nov (afternoon)**
- `Scenario` base class (`start`, `update(dt, AimInput)`, `getDrawList`, `score`, `accuracy`, `statsText`). `Game` owns the timer, countdown, pausing and saving, scenarios only own their targets and scoring
- **Controlsphere** done: one sphere circling 360° around the player, random speed/direction/distance changes eased in, gravity hops. Score = time on target × 100 (6000 perfect, checked with a perfect-tracking test), accuracy = on target / time firing. The sphere lights up while tracked. Hits are tested against last frame's position, since that's what the player saw
- New states: **ScenarioSelect** between MainMenu and Countdown, **Settings** (from the main menu or pause), **Leaderboard** (from scenario select or results)
- **DPI + cm/360**: yaw presets (CS2/Apex 0.022, Valorant 0.07, Overwatch 2 0.0066), sens, DPI and FOV, saved to `settings.txt`. DPI only feeds the cm/360 readout, the turning itself still works in raw counts
- **Scores**: every run appended to `scores.txt` (scenario, score, accuracy, cm/360, date). Leaderboard = top 10 per scenario from that history; results show your rank and "new best". Full board printed to the Output window until the UI can draw it
- The old placeholder click target is gone, B180 replaces it

---

## Where the engine is now

| Have | Missing (and what needs it) |
|---|---|
| Device, swapchain, double buffering, fences | **Descriptor tables + samplers** in the root signature. Textures, the font atlas, shadows, skybox and post-processing all depend on this |
| Root CBVs (b0 VS, b1 PS) with reflection + ring buffer | **One shared shader-visible heap**, owned by Core |
| PSO manager (fixed state: cull back, depth on, no blend) | **Configurable PSO state**: depth func/write, cull, blend, RT format, sample count, depth bias |
| sRGB backbuffer (gamma-correct) | **Off-screen render targets**: only drawing to the backbuffer right now |
| GEM loading, submeshes, procedural sphere, Blinn-Phong | **UV + tangent in `Vertex`**. GEM already provides `tangent` (Vec4) and `u`, `v` |
| Raw mouse input, click counter, key edges, cursor pixels | **Own UI renderer** (quads + font atlas + widgets) |
| State machine (menu, select, settings, leaderboard, countdown, play, pause, results) | B180 and voxTargetSwitch |
| Scenario base class, Controlsphere, scores.txt leaderboard, settings.txt, cm/360 | Leaderboard/settings drawn with the UI instead of the title bar |
| | `Present(1, 0)` = vsync on. Aim trainers want low latency, so add a toggle |

Skeletal animation and tessellation were on the Todo, but an aim trainer has no use for them. **Dropped.** Instancing and the skybox stay.

---

## Schedule

| Days | Phase | Done means |
|---|---|---|
| 1–2 | **0. Engine foundation** | A textured cube renders through an off-screen target and a fullscreen pass. ~~ImGui removed~~ ✅ |
| 3 | **1. UI system** | Text, panels and working buttons/sliders drawn with my own renderer |
| 4–5 | **2. Game skeleton + B180** | Menu → B180 → 60 s → results, with HUD and saved best. *A complete game I could submit.* (state machine started early, see log) |
| 6 | **3. Controlsphere + voxTS** | All three scenarios playable |
| 7 | **4a. Room: textures, normal maps, mips, anisotropic filtering** | Grid room that doesn't shimmer |
| 8–9 | **4b. Shadow mapping (PCF)** | Targets cast soft shadows on the floor |
| 10 | **4c. HDR, tonemapping, bloom** | Targets glow |
| 11 | **4d. MSAA + skybox** | Clean edges on small, distant targets; 360° environment |
| 12 | **4e. Hit particles (instancing + additive blend)** | Burst on every kill |
| — | **4f. Stretch: kerned text / batched UI / PBR / GPU timings / debug views** | Only if ahead |
| 13–14 | **Freeze** | No new features. Bugs, tuning, video, write-up, screenshots |

**If I fall behind, cut in this order:** PBR → particles → skybox → MSAA. Don't cut shadows, bloom or the UI: shadows and bloom make the biggest visual difference, and the UI is now explicitly something the lecturer marks.

---

## Phase 0: Engine foundation (days 1–2)

### 0.1 One shared descriptor heap ✅ ImGui removed

You can only bind **one** shader-visible CBV/SRV/UAV heap at a time. With ImGui gone, Core owns it.

1. Create the heap in `Core` (e.g. 1024 slots) with a free-list allocator: `allocate(count)` returns a **contiguous** block, plus `free`. (The old `DebugUI.cpp` had a free list that's a decent starting point — it's in git history.)
2. ~~Remove ImGui~~ Done 27 Nov. Values that were on ImGui sliders (light direction, ambient) are hard-coded in `main.cpp`'s `Lighting` struct until the UI system gives sliders back.
3. In `beginFrame`, call `SetDescriptorHeaps` **once**, before any draw.

Each **material** allocates a block of 4 slots (albedo, normal, rmax, spare) and writes its SRVs straight into those slots with `CreateShaderResourceView`. Its textures are then contiguous, so one table binding per draw covers all of them. The font atlas is just another texture with its own slot.

### 0.2 Root signature with tables and static samplers

```
param 0  root CBV   b0          VS   (unchanged; also the per-quad UI data)
param 1  root CBV   b1          PS   (unchanged)
param 2  table      t0–t3       PS   material: albedo, normal, rmax, spare (UI: t0 = atlas/image)
param 3  table      t4–t5       PS   per-frame: shadow map, sky cubemap
param 4  root SRV   t6          VS   particle/instance StructuredBuffer (phase 4e; batched UI in 4f)

static samplers
s0  anisotropic x16, wrap      textures
s1  linear, clamp              post-processing
s2  comparison LESS_EQUAL, border = white    shadow map (PCF)
s3  point, clamp               UI / font atlas (crisp pixels)
```

- A table is a `D3D12_DESCRIPTOR_RANGE` (type SRV, NumDescriptors, BaseShaderRegister) wrapped in a `D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE` parameter.
- Static samplers go in `desc.NumStaticSamplers` / `desc.pStaticSamplers`. They don't use heap slots and don't need binding.
- Bind with `SetGraphicsRootDescriptorTable(2, material.gpuHandle)`. **Do this after `psos.bind()`**, because setting the root signature clears every root argument (there's a comment about this in `drawScene` in `main.cpp`).
- Update the comments and constants in `Shader.h`, since they describe the root layout.

### 0.3 Configurable PSO state

```cpp
enum class BlendMode { Opaque, Alpha, Additive };
struct PSOState {
    D3D12_CULL_MODE cull = D3D12_CULL_MODE_BACK;
    bool depthTest = true, depthWrite = true;
    D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
    BlendMode blend = BlendMode::Opaque;
    DXGI_FORMAT rtFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;  // UNKNOWN + numRTs = 0 for depth-only
    UINT numRTs = 1, samples = 1;
    int depthBias = 0; float slopeBias = 0.0f;
};
```
`createPSO(core, name, shader, layout, state)`. Every PSO needed is a different combination of these:

| PSO | Differences from default |
|---|---|
| Scene (opaque) | default, `samples = 4` once MSAA is in |
| Shadow | `numRTs = 0`, `rtFormat = UNKNOWN`, no PS, depth bias |
| Skybox | `depthFunc = LESS_EQUAL`, `depthWrite = false`, `cull = FRONT` (you're inside the cube) |
| Particles | `blend = Additive`, `depthWrite = false` |
| Post-process | `depthTest = false`, `rtFormat` = whatever it writes to |
| Final to backbuffer | `rtFormat = R8G8B8A8_UNORM_SRGB` |
| **UI** | `blend = Alpha`, `depthTest = false`, `cull = NONE`, `rtFormat = R8G8B8A8_UNORM_SRGB`, empty input layout |

**The most common debug-layer error:** the PSO's RT format, sample count or DSV format doesn't match what's currently bound. When a draw silently does nothing, check that first. (The UI PSO draws to the backbuffer with no depth buffer bound, so set its DSV format to `UNKNOWN`.)

### 0.4 Vertex layout: UV + tangent

```cpp
struct Vertex { Vec3 position; Vec3 normal; Vec4 tangent; float u, v; };  // 48 bytes
```
Fill these from `GEMStaticVertex` in `loadGEM`, add `TANGENT` (R32G32B32A32_FLOAT) and `TEXCOORD` (R32G32_FLOAT) to `inputLayout()`. The per-vertex colour is already gone. The sphere generator and room generator fill them too:
- sphere: `u = s/segments`, `v = r/rings`, `tangent = (-sinφ, 0, cosφ, 1)` (the sphere already duplicates the seam column for this)
- room: see 4a

### 0.5 Textures

1. Add `stb_image.h` (github.com/nothings/stb). Use `stbi_load(path, &w, &h, &n, 4)` to always get RGBA8.
2. **Mipmaps on the CPU:** keep halving with a 2×2 box filter until 1×1. Without mips, a grid texture on a far wall shimmers badly, and in an aim trainer that's distracting. Worth a sentence in the report. (The font atlas gets **no** mips: it's always drawn at 1:1.)
3. Create a `DEFAULT`-heap `TEXTURE2D` resource with `MipLevels = mipCount`.
   - **Format matters:** albedo = `R8G8B8A8_UNORM_SRGB`. Normal maps and rmax = `R8G8B8A8_UNORM` (linear data). Font atlas = `R8_UNORM` (one coverage channel). Loading a normal map as sRGB makes the lighting subtly wrong.
4. `device->GetCopyableFootprints(&desc, 0, mipCount, 0, footprints, rows, rowSizes, &totalSize)` gives one footprint per mip. Rows in the upload buffer are padded to **256-byte** pitch, so `memcpy` row by row. Then `CopyTextureRegion` once per mip. `uploadResource` already has a single-footprint branch; extend it to loop over subresources. Make the loader take a format and bytes-per-pixel so the same path uploads the R8 atlas.
5. Transition to `PIXEL_SHADER_RESOURCE` and create the SRV (`Texture2D.MipLevels = mipCount`).

The T-Rex textures (17–24 MB PNGs) aren't used by the game any more and can be deleted from `resources/`.

### 0.6 RenderTarget class + fullscreen pass

```cpp
class RenderTarget {
    ID3D12Resource* texture;      // colour
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;   // from an RTV heap (make Core's bigger)
    UINT srvSlot;                 // in the shared heap, so later passes can read it
    D3D12_RESOURCE_STATES state;  // track it so transition(newState) only adds a barrier when needed
    void create(Core*, UINT w, UINT h, DXGI_FORMAT, UINT samples = 1);
};
```
Add `DepthTarget` the same way (`R32_TYPELESS` resource, DSV as `D32_FLOAT`, SRV as `R32_FLOAT`, so the shadow map can be read). Split `Core::beginFrame` so it no longer binds the backbuffer automatically. Add `core.setTarget(rt, depth)` and `core.setBackbuffer()` (with `depth = nullptr` allowed, for the UI).

**Fullscreen triangle**, with no vertex buffer and no input layout:
```hlsl
struct FSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD; };
FSOut FullscreenVS(uint id : SV_VertexID) {
    FSOut o;
    o.uv  = float2((id << 1) & 2, id & 2);
    o.pos = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return o;
}
```
`IASetPrimitiveTopology(TRIANGLELIST); DrawInstanced(3, 1, 0, 0);`. Every post-processing pass reuses this, and the UI quad in Phase 1 is the same idea extended to an arbitrary rectangle.

**Phase 0 is done when:** the scene renders into an off-screen `R16G16B16A16_FLOAT` target, a fullscreen pass copies it to the backbuffer, and one textured cube appears.

### 0.7 Input and presentation

- ✅ `leftClicks` counter in `WM_LBUTTONDOWN`, reset in `processMessages`
- ✅ `keysPressed[]` edges (key repeat filtered with lParam bit 30)
- ✅ Cursor position in client pixels (`mousex/mousey`). Still to add for the UI: `leftPressed` / `leftReleased` per frame
- ✅ Cursor mode driven by the game state: free in menus, hidden + clipped while Countdown/Playing (`Game::changeState`)
- ✅ `sensitivity * 0.022°` per count (CS/Apex yaw); horizontal FOV converted to vertical in `Camera::getProjection`
- ☐ vsync toggle: `Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING)`. Tearing also needs `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` on the swapchain; check support with `CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING)`.

---

## Phase 1: Custom UI system (day 3)

Everything on screen that isn't the 3D scene is a **screen-space quad**: panels, buttons, slider bars, the crosshair, each letter of text, and debug images like the shadow map. One shader, one PSO, one draw per quad to begin with.

### 1.1 The UI quad

Extend the fullscreen triangle into a quad positioned in **pixels**. Per quad, write a small constant buffer through the existing root-CBV ring buffer (param 0, b0) and draw 4 vertices as a triangle strip.

```hlsl
cbuffer UIQuad : register(b0) {
    float4 rect;        // x, y, w, h in pixels, top-left origin
    float4 uvRect;      // u0, v0, u1, v1 in the atlas
    float4 colour;      // LINEAR rgba (see the sRGB note below)
    float2 screenSize;
    float  mode;        // 0 = solid colour, 1 = font (atlas .r = alpha), 2 = image (rgba)
    float  pad;
};
Texture2D tex : register(t0);
SamplerState pointClamp : register(s3);

struct UIOut {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 colour : COLOR;
    nointerpolation float mode : TEXCOORD1;
};

UIOut UIVS(uint id : SV_VertexID) {
    float2 corner = float2(id & 1, id >> 1);            // strip: (0,0) (1,0) (0,1) (1,1)
    float2 px = rect.xy + corner * rect.zw;
    UIOut o;
    o.pos = float4(px / screenSize * float2(2, -2) + float2(-1, 1), 0, 1);  // pixels -> NDC, Y down
    o.uv = lerp(uvRect.xy, uvRect.zw, corner);
    o.colour = colour;
    o.mode = mode;
    return o;
}

float4 UIPS(UIOut i) : SV_TARGET {
    if (i.mode < 0.5) return i.colour;
    float4 t = tex.Sample(pointClamp, i.uv);
    if (i.mode < 1.5) return float4(i.colour.rgb, i.colour.a * t.r);   // font coverage
    return t * i.colour;                                               // image
}
```

```cpp
cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
cmd->DrawInstanced(4, 1, 0, 0);
```

- **Why the VS passes the colour through:** param 0 (b0) is VS-visible only, so the PS gets the colour and mode from the VS output instead of needing a new root parameter.
- **Before textures work**, mode 0 (solid colour) still gives panels, buttons and the crosshair, so the menus can be laid out early — and the state machine is already there to hang them on.
- Bind the atlas (or any SRV) with `SetGraphicsRootDescriptorTable(2, ...)`. Only rebind when the texture changes, so all the text quads share one binding.

### 1.2 sRGB colours (an easy mistake, and a good report point)

The backbuffer is `_SRGB`, so the hardware **encodes** whatever the shader outputs from linear to sRGB. Colours picked from a colour picker or hex code are already sRGB, so outputting them directly makes them come out washed out. Convert on the CPU before writing the CB:

```cpp
float toLinear(float c) { return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f); }
```

Alpha stays as it is. Blending also happens in linear space on an sRGB target, which makes anti-aliased text edges look slightly thinner than in an image editor. Worth mentioning in the write-up.

### 1.3 Text renderer

Basic version first, upgrade only if ahead (stretch 4f).

**Basic: monospace grid atlas.** A PNG with the 95 printable ASCII characters in a 16×16 grid (char `c` is cell `c % 16, c / 16`). Bake one from any TTF with a bitmap font generator, or draw it by hand. Then:
```cpp
float UI::text(float x, float y, const char* s, float scale, Colour col) {
    float cw = cellW * scale, ch = cellH * scale, startX = x;
    for (; *s; s++) {
        if (*s == '\n') { x = startX; y += ch; continue; }
        int c = (unsigned char)*s, cx = c % 16, cy = c / 16;
        float4 uv = { cx / 16.f, cy / 16.f, (cx + 1) / 16.f, (cy + 1) / 16.f };
        quad({ x, y, cw, ch }, uv, col, MODE_FONT);
        x += cw;                                       // fixed advance
    }
    return x - startX;                                 // width, for centring/alignment
}
```
Snap `x`/`y` to whole pixels so the point sampler doesn't smear glyphs. Integer `scale` for large HUD numbers (2×, 3×) keeps them crisp.

**Proper text (stretch): baked TTF with advances and kerning.** Use `stb_truetype.h` (same repo as stb_image):
- `stbtt_BakeFontBitmap(ttf, 0, pixelHeight, bitmap, 512, 512, 32, 96, charData)` packs the glyphs into an R8 bitmap. Upload as `R8_UNORM`.
- `stbtt_GetBakedQuad(charData, 512, 512, c - 32, &x, &y, &q, 1)` gives each glyph's screen rect and UVs, and advances `x` by that glyph's real width (proportional spacing).
- Kerning: `stbtt_GetCodepointKernAdvance(&info, prev, c) * stbtt_ScaleForPixelHeight(&info, pixelHeight)` added between each pair ("AV", "To" sit closer together).
- Bake two sizes (body ~20 px, HUD ~64 px) rather than scaling one, so both stay sharp.

Either way, add `measure(str, scale)` (the same loop without drawing) to centre text in buttons and on screen.

### 1.4 Widgets (immediate mode)

Immediate mode means each widget is a function call every frame that both draws and returns whether it was used. No widget tree to manage, which keeps it small.

```cpp
struct UIInput { float mx, my; bool down, pressed, released; };

class UI {
public:
    void begin(Core*, const UIInput&, float screenW, float screenH);
    void rect(float x, float y, float w, float h, Colour);
    void image(float x, float y, float w, float h, UINT srvSlot);   // shadow map / bloom debug views
    float text(float x, float y, const char*, float scale, Colour);
    float measure(const char*, float scale);
    bool button(int id, float x, float y, float w, float h, const char* label);
    bool slider(int id, float x, float y, float w, const char* label, float& v, float lo, float hi);
    bool checkbox(int id, float x, float y, const char* label, bool& v);
    void end();
private:
    int hot = 0, active = 0;   // hovered widget, widget the mouse went down on
};
```

- **Button:** *hot* if the mouse is inside the rect. Becomes *active* when pressed while hot. **Clicked** when the mouse is released while it's both hot and active (so pressing on one button and releasing on another does nothing). Three shades: normal, hot, active.
- **Slider:** while active, `v = lo + (hi - lo) * saturate((mx - x) / w)`, even if the mouse leaves the bar. Draw the track, the filled part, and the value as text (`snprintf("%.2f")`).
- **Checkbox:** a button that flips `v`.
- **Resolution independence:** lay out in a 1920×1080 reference space and multiply by `s = screenH / 1080.0f`, with helpers for anchoring (centre, top-right) so the HUD sits in the same place at any resolution.
- Each widget gets a unique `id` (a constant, or a hash of the label).

### 1.5 Where the UI pass goes

After the tonemap/composite pass, straight onto the backbuffer, so the UI is never bloomed, tonemapped or multisampled. Set the backbuffer with no depth buffer, bind the UI PSO, then draw every queued quad.

**Hooking it into the state machine:** each `Game::updateX()` gets a matching `drawX(UI&)`. Once buttons exist, swap the keyboard shortcuts (Enter/Esc/Q) for buttons, and replace `Game::getStatusText()` + the window title with the real HUD.

**Phase 1 is done when:** a test screen shows a panel, a line of text, a button that logs a click, and a slider that changes a value.

---

## Phase 2–3: The game (days 4–6)

**Done (27 Nov afternoon):** `Scenario` base class, `ControlsphereScenario`, ScenarioSelect / Settings / Leaderboard states, `scores.txt` with a top 10 leaderboard, DPI and cm/360.

**Still to do:**
- `B180Scenario` and `VoxTSScenario`: derive from `Scenario`, add a `push_back` in `Game::init`, and they show up in scenario select and get their own leaderboard
- `closestHit` across several targets (B180 has four, voxTS several), using `AimInput::clicks` so every click this frame is tested
- Draw the Settings and Leaderboard screens with the UI (rows already exist as `SettingRow` and `ScoreBoard::top`)

**Screens, all built with my UI:**

| Screen | Contents |
|---|---|
| Main menu | Title, Play, Settings, Quit buttons |
| Scenario select | One button per scenario with its best score underneath, and a Leaderboard button |
| Leaderboard | Top 10 table: rank, score, accuracy, cm/360, date. Arrows to switch scenario |
| Settings | Game (yaw preset), sensitivity, DPI, live cm/360, FOV |
| Countdown | Big centred 3-2-1 |
| HUD | Timer (top centre), score, kills, accuracy %, **crosshair** |
| Pause (Esc) | Resume, sensitivity slider, FOV slider, vsync checkbox, crosshair settings, Quit to menu |
| Results | Score, accuracy, kills, "New best!" if beaten, Retry / Menu buttons |
| Debug panel (F1) | Tuning sliders for every scenario parameter, graphics toggles, FPS / frame time |

**The crosshair is UI too:** four rects plus an optional centre dot, with gap, length, thickness, colour and outline as settings. Custom crosshairs are a standard aim-trainer feature, so this is cheap polish that fits the game.

Build B180 all the way to the results screen first. The other two reuse the same structure.

---

## Phase 4: Graphics (days 7–12)

### 4a. The room: textures, normal mapping, anisotropic filtering (day 7)

**Procedural room mesh** (replaces the stretched-cube floor/wall in `Game::buildRoom`). Build 6 inward-facing quads (normals point into the room, winding reversed so the inside faces survive back-face culling). Scale UVs by wall size, so `u = x * tilesPerMetre`. The grid then stays square on every wall, whatever its dimensions. Tangent = the direction U increases along that wall.

**Normal mapping in the pixel shader:**
```hlsl
float3 N = normalize(input.normal);
float3 T = normalize(input.tangent.xyz - dot(input.tangent.xyz, N) * N);  // Gram-Schmidt
float3 B = cross(N, T) * input.tangent.w;                                 // w = handedness
float3 n = normalMap.Sample(s0, input.uv).xyz * 2.0 - 1.0;
N = normalize(mul(n, float3x3(T, B, N)));
```
Transform the tangent with `World` in the VS, the same way as the normal.

**Grid look without a texture**, for the KovaaK's style. Anti-aliased lines with `fwidth`:
```hlsl
float2 g = abs(frac(uv - 0.5) - 0.5) / fwidth(uv);
float line = 1.0 - saturate(min(g.x, g.y));
albedo = lerp(albedo, gridColour, line);
```
`fwidth` keeps the lines one pixel wide at any distance, which is a nice contrast with the mipmapped texture approach for the write-up.

**Free assets:** ambientCG and Poly Haven have CC0 tile and concrete textures with normal maps.

### 4b. Shadow mapping with PCF (days 8–9)

Useful for gameplay as well as looks: a shadow under a B180 bot tells the player where it will land.

1. **Shadow map:** a 2048² `DepthTarget` (from 0.6). Needs a second DSV slot, so make Core's DSV heap bigger.
2. **Light matrices.** Add `Matrix::orthographic(w, h, n, f)`, matching the `perspective` convention:
   `p[0] = 2/w; p[5] = 2/h; p[10] = 1/(f-n); p[11] = -n/(f-n); p[15] = 1;`
   `lightView = lookAt(roomCentre - lightDir * 30, roomCentre, up)`. Use `up = (0,0,1)` if the light points straight down. Size the ortho box to just fit the room, since a tighter box means sharper shadows.
3. **Shadow pass**, before the main pass:
   - transition shadow map → `DEPTH_WRITE`, clear to 1, viewport 2048²
   - bind the shadow PSO (no RT, no PS, `slopeBias ≈ 2`, `depthBias` tuned by eye)
   - `shadow.hlsl` VS: `WVP = lightVP * world`. The `Shader` class handles this unchanged.
   - draw targets (and the room for self-shadowing)
   - transition → `PIXEL_SHADER_RESOURCE`
4. **Main pass:** pass `lightVP` to the VS and output `lightPos = mul(lightVP, worldPos)`. In the PS:
```hlsl
float3 ndc = input.lightPos.xyz / input.lightPos.w;
float2 suv = float2(ndc.x * 0.5 + 0.5, -ndc.y * 0.5 + 0.5);    // flip Y
float shadow = 0;
[unroll] for (int y = -1; y <= 1; y++)
[unroll] for (int x = -1; x <= 1; x++)
    shadow += shadowMap.SampleCmpLevelZero(s2, suv + float2(x, y) * texel, ndc.z);
shadow /= 9.0;
colour = albedo * (ambient + diffuse * shadow * lightColour) + spec * shadow;
```
   The comparison sampler does a 2×2 bilinear compare per tap, so 9 taps blend 36 comparisons and give soft edges cheaply.
5. **Likely problems:** shadow acne (raise the bias), peter-panning (bias too high, so shadows detach from the object), a shadow cut off at the edges (ortho box too small). Add a debug-panel checkbox that draws the shadow map in a corner with `ui.image(..., shadowMap.srvSlot)`. The depth SRV is single-channel, so it shows up red; add a mode that outputs `t.rrr`. A great debugging tool, a good screenshot for the report, and it shows off the UI renderer drawing an arbitrary SRV.

### 4c. HDR, tonemapping, bloom (day 10)

Already rendering into `R16G16B16A16_FLOAT` from Phase 0, so values above 1.0 survive.

1. **Emissive targets:** in the PS, add `emissive * objectColour` with `emissive ≈ 3–6` on targets. Brighter for the `flash` on hit.
2. **Bright pass** (fullscreen) into a half-res target: `max(colour - threshold, 0)`.
3. **Blur.** Either works:
   - *Simple:* ping-pong a separable 9-tap Gaussian (horizontal, then vertical) at half res, 2 iterations.
   - *Extra marks:* the downsample/upsample chain (the Call of Duty: Advanced Warfare method). Downsample with a 13-tap filter through 5 mip levels, then upsample with a 3×3 tent filter, adding back at each level. Wide, stable bloom that doesn't flicker on small moving targets.
4. **Composite + tonemap** to the backbuffer:
```hlsl
float3 c = hdr.Sample(s1, uv).rgb + bloom.Sample(s1, uv).rgb * bloomStrength;
c *= exposure;
c = saturate((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14));  // ACES fit (Narkowicz)
return float4(c, 1);   // the SRGB backbuffer applies gamma
```
5. The UI pass draws **after** this, so the UI isn't bloomed or tonemapped.

Exposure, threshold and strength go on debug-panel sliders, with before/after screenshots.

### 4d. MSAA + skybox (day 11)

**MSAA 4×.** Small, distant spheres crawl and shimmer without it, and in an aim trainer that hurts readability, so it's worth arguing for in the report.
- Check support: `CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS)` for `R16G16B16A16_FLOAT` and 4 samples.
- Create the scene colour and depth targets with `SampleDesc.Count = 4`, and scene/skybox/particle PSOs with `samples = 4`. Shadow pass, post passes and UI stay at 1.
- After the scene: colour → `RESOLVE_SOURCE`, resolve target → `RESOLVE_DEST`, then `ResolveSubresource(dst, 0, src, 0, R16G16B16A16_FLOAT)`. Post-processing reads the resolved target.
- Discussion point: resolving *before* tonemapping lets very bright edges stay aliased. A tonemap-aware resolve fixes it.

**Skybox.** Controlsphere is 360°, so the player sees everything around them.
- Cubemap from 6 face images: texture with `DepthOrArraySize = 6`, SRV as `TEXTURECUBE`. Upload each face like a normal texture (subresource = face × mipCount + mip). Poly Haven HDRIs can be converted to faces with free online tools.
- Draw a cube *after* the opaque geometry, using the view matrix with its translation removed. In the VS: `output.position = mul(VP, float4(pos, 1)).xyww;` sets depth to exactly 1. `LESS_EQUAL` depth, no depth write, front-face culling. Sample with the direction `pos`.
- No-asset fallback: a procedural gradient sky in the PS (`lerp(horizon, zenith, dir.y)`, plus a sun disc from `pow(dot(dir, -lightDir), 500)`).
- With PBR, the skybox can be sampled for ambient and reflections too.

### 4e. Hit particles: instancing + additive blending (day 12)

Ticks off **instancing** from the Todo list.
- On kill, spawn 30–40 particles at the target's position (CPU array: position, velocity, life, size, colour). Each frame apply gravity and drag, fade them, remove dead ones.
- Upload the live particles into a `StructuredBuffer` in an **upload heap, one per frame in flight**. Overwriting the buffer the GPU is still reading from the previous frame causes flicker. Bind with `SetGraphicsRootShaderResourceView(4, buffer->GetGPUVirtualAddress())` (root param 4, no heap slot).
- `DrawInstanced(6, particleCount, 0, 0)`. The VS builds a camera-facing quad from `SV_VertexID` and the camera's right/up vectors, and reads `particles[SV_InstanceID]`.
- PS: a soft circle with `saturate(1 - length(uv * 2 - 1))`, emissive > 1 so it blooms. Additive PSO, depth test on, depth write off.

### 4f. Stretch goals (only if ahead)

- **Proper text renderer:** the stb_truetype baked atlas with proportional advances and kerning (see 1.3). The lecturer specifically mentioned this, so it's probably the best value stretch goal for the UI marks.
- **Batched UI:** instead of one CB write + draw per quad, push every quad into a per-frame `StructuredBuffer` (same pattern and root param 4 as the particles) and issue `DrawInstanced(4, quadCount)` once per texture. The VS reads `quads[SV_InstanceID]`. Measure draw count and CPU time before/after for the report.
- **PBR (Cook-Torrance):** GGX normal distribution, Smith-Schlick geometry, Schlick Fresnel, using the albedo, normal and rmax textures. Check the GEM texture packing in the lecture notes before assuming which rmax channel is roughness, metal or AO.
- **GPU timings per pass:** a timestamp query heap, `EndQuery` around each pass, `ResolveQueryData` into a readback buffer, divided by `GetTimestampFrequency()`. Show ms per pass as text in the debug panel. About two hours, and it gives the report a real performance section.
- **Debug view toggles:** normals, albedo only, shadow factor, bloom buffer. Cheap, and strong report figures.
- **Fresnel rim on targets:** `pow(1 - saturate(dot(N, V)), 3) * rimColour`, so targets stand out against any background.

---

## Report and video checklist

- A pipeline diagram of the frame: shadow pass → scene (MSAA) → resolve → bright pass → blur → composite/tonemap → **UI pass** → present
- A section on the UI system: the screen-space quad, pixel-to-NDC conversion, the font atlas, immediate-mode widgets (hot/active), and the sRGB colour conversion
- A diagram of the game state machine and why pausing doesn't touch the run timer
- Before/after screenshots for each feature (from the debug-panel toggles)
- Shadow map and bloom buffers shown in debug views
- GPU timings table
- Problems hit and how they were fixed (shadow acne, sRGB vs linear normal maps and UI colours, root arguments cleared by `psos.bind()`, fast clicks lost before the click counter). Markers usually reward this.
- A video showing all three scenarios, the menus, and the results screen after a run
- A frame captured in **PIX** or **RenderDoc** to show the passes. Both free, and the fastest way to debug a black screen.
