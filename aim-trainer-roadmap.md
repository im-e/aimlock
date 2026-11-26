# DX12 Aim Trainer: Roadmap

Three scenarios (B180, Controlsphere, voxTargetSwitch), each with a 60 s timer, a score and a results screen, built on your own DX12 engine. The plan assumes hand-in roughly two weeks from 26 Nov (around 10 Dec). If your actual deadline is different, shift the dates.

The order is chosen so you always have **something you could hand in**, and each graphics feature builds on the one before it.

---

## Where the engine is now

| Have | Missing (and what needs it) |
|---|---|
| Device, swapchain, double buffering, fences | **Descriptor tables + samplers** in the root signature. Textures, shadows, skybox and post-processing all depend on this |
| Root CBVs (b0 VS, b1 PS) with reflection + ring buffer | **One shared shader-visible heap**. ImGui currently owns its own, and only one can be bound at a time |
| PSO manager (fixed state: cull back, depth on, no blend) | **Configurable PSO state**: depth func/write, cull, blend, RT format, sample count, depth bias |
| sRGB backbuffer (gamma-correct) | **Off-screen render targets**: you only draw to the backbuffer right now |
| GEM loading, submeshes, Blinn-Phong | **UV + tangent in `Vertex`**. GEM already provides `tangent` (Vec4) and `u`, `v` |
| ImGui, raw mouse input | **Click counter** (fast clicks get lost), sens/FOV conversion |
| | `Present(1, 0)` = vsync on. Aim trainers want low latency, so add a toggle |

Skeletal animation and tessellation are on your Todo, but an aim trainer has no use for them. **Drop them.** Instancing and the skybox stay.

---

## Schedule

| Days | Phase | Done means |
|---|---|---|
| 1–2 | **0. Engine foundation** | A textured cube renders through an off-screen target and a fullscreen pass |
| 3–4 | **1. Game skeleton + B180** | Menu → B180 → 60 s → results, with HUD and saved best. *A complete game you could submit.* |
| 5 | **2. Controlsphere + voxTS** | All three scenarios playable |
| 6 | **3a. Room: textures, normal maps, mips, anisotropic filtering** | Grid room that doesn't shimmer |
| 7–8 | **3b. Shadow mapping (PCF)** | Targets cast soft shadows on the floor |
| 9 | **3c. HDR, tonemapping, bloom** | Targets glow |
| 10 | **3d. MSAA + skybox** | Clean edges on small, distant targets; 360° environment |
| 11 | **3e. Hit particles (instancing + additive blend)** | Burst on every kill |
| 12 | **3f. Stretch: PBR / GPU timings / debug views** | Whatever fits |
| 13–14 | **Freeze** | No new features. Bugs, tuning, video, write-up, screenshots |

**If you fall behind, cut in this order:** PBR → particles → skybox → MSAA. Don't cut shadows or bloom: they make the biggest visual difference and give you the most to write about.

---

## Phase 0: Engine foundation (days 1–2)

### 0.1 One shared descriptor heap

You can only bind **one** shader-visible CBV/SRV/UAV heap at a time, and `DebugUI` currently creates and binds its own. If you add a second heap for your textures, whichever was bound last wins and the other's descriptors become invalid.

1. Move the heap and its free-list allocator from `DebugUI.cpp` into `Core`. Make it bigger (e.g. 1024) and add `allocate(count)`, which returns a **contiguous** block, plus `free`.
2. Pass `core->srvHeap` to ImGui in `DebugUI::init` (its alloc/free callbacks just call Core's allocator).
3. In `beginFrame`, call `SetDescriptorHeaps` **once**, before any draw. Remove the call from `DebugUI::render`.

Each **material** allocates a block of 4 slots (albedo, normal, rmax, spare) and writes its SRVs straight into those slots with `CreateShaderResourceView`. Its textures are then contiguous, so one table binding per draw covers all of them.

### 0.2 Root signature with tables and static samplers

```
param 0  root CBV   b0          VS   (unchanged)
param 1  root CBV   b1          PS   (unchanged)
param 2  table      t0–t3       PS   material: albedo, normal, rmax, spare
param 3  table      t4–t5       PS   per-frame: shadow map, sky cubemap
param 4  root SRV   t6          VS   particle/instance StructuredBuffer (phase 3e)

static samplers
s0  anisotropic x16, wrap      textures
s1  linear, clamp              post-processing
s2  comparison LESS_EQUAL, border = white    shadow map (PCF)
```

- A table is a `D3D12_DESCRIPTOR_RANGE` (type SRV, NumDescriptors, BaseShaderRegister) wrapped in a `D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE` parameter.
- Static samplers go in `desc.NumStaticSamplers` / `desc.pStaticSamplers`. They don't use heap slots and don't need binding.
- Bind with `SetGraphicsRootDescriptorTable(2, material.gpuHandle)`. **Do this after `psos.bind()`**, because setting the root signature clears every root argument (you already have a comment about this in `main.cpp`).
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
`createPSO(core, name, shader, layout, state)`. Every PSO you'll need is a different combination of these:

| PSO | Differences from default |
|---|---|
| Scene (opaque) | default, `samples = 4` once MSAA is in |
| Shadow | `numRTs = 0`, `rtFormat = UNKNOWN`, no PS, depth bias |
| Skybox | `depthFunc = LESS_EQUAL`, `depthWrite = false`, `cull = FRONT` (you're inside the cube) |
| Particles | `blend = Additive`, `depthWrite = false` |
| Post-process | `depthTest = false`, `rtFormat` = whatever it writes to |
| Final to backbuffer | `rtFormat = R8G8B8A8_UNORM_SRGB` |

**The most common debug-layer error:** the PSO's RT format, sample count or DSV format doesn't match what's currently bound. When a draw silently does nothing, check that first.

### 0.4 Vertex layout: UV + tangent

```cpp
struct Vertex { Vec3 position; Vec3 normal; Vec4 tangent; float u, v; };  // 48 bytes
```
Fill these from `GEMStaticVertex` in `loadGEM`, add `TANGENT` (R32G32B32A32_FLOAT) and `TEXCOORD` (R32G32_FLOAT) to `inputLayout()`, and drop the per-vertex `Color` (the shader uses `objectColour`). The sphere generator and room generator fill them too:
- sphere: `u = s/segments`, `v = r/rings`, `tangent = (-sinθ, 0, cosθ, 1)`
- room: see 3a

### 0.5 Textures

1. Add `stb_image.h` (github.com/nothings/stb). Use `stbi_load(path, &w, &h, &n, 4)` to always get RGBA8.
2. **Mipmaps on the CPU:** keep halving with a 2×2 box filter until 1×1. Without mips, a grid texture on a far wall shimmers badly, and in an aim trainer that's distracting. That makes it worth a sentence in your report.
3. Create a `DEFAULT`-heap `TEXTURE2D` resource with `MipLevels = mipCount`.
   - **Format matters:** albedo = `R8G8B8A8_UNORM_SRGB`. Normal maps and rmax = `R8G8B8A8_UNORM` (linear data). If you load a normal map as sRGB, the lighting comes out subtly wrong.
4. `device->GetCopyableFootprints(&desc, 0, mipCount, 0, footprints, rows, rowSizes, &totalSize)` gives you one footprint per mip. Rows in the upload buffer are padded to **256-byte** pitch, so `memcpy` row by row. Then call `CopyTextureRegion` once per mip. Your `uploadResource` already has a single-footprint branch; extend it to loop over subresources.
5. Transition to `PIXEL_SHADER_RESOURCE` and create the SRV (`Texture2D.MipLevels = mipCount`).

Avoid the T-Rex textures: they're 17–24 MB PNGs and the game doesn't use them.

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
Add `DepthTarget` the same way (`R32_TYPELESS` resource, DSV as `D32_FLOAT`, SRV as `R32_FLOAT`, so the shadow map can be read). Split `Core::beginFrame` so it no longer binds the backbuffer automatically. Add `core.setTarget(rt, depth)` and `core.setBackbuffer()`.

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
`IASetPrimitiveTopology(TRIANGLELIST); DrawInstanced(3, 1, 0, 0);`. Every post-processing pass reuses this.

**Phase 0 is done when:** the scene renders into an off-screen `R16G16B16A16_FLOAT` target, a fullscreen pass copies it to the backbuffer, ImGui draws on top, and one textured cube appears.

### 0.7 Input and presentation (about an hour)
- `leftClicks` counter in `WM_LBUTTONDOWN`, reset in `processMessages`
- `lookSensitivity = sens * 0.022 * PI / 180` (CS/Apex yaw); FOV slider in horizontal degrees, converted to vertical
- vsync toggle: `Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING)`. Tearing also needs `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` on the swapchain; check support with `CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING)`.

---

## Phase 1–2: The game (days 3–5)

See the scenario design from earlier in our chat. In short:
- `GameState` machine: Menu → Select → Countdown → Playing → Results (+ Paused)
- `Scenario` base class with `B180Scenario`, `ControlsphereScenario` and `VoxTSScenario`
- Targets in angle/distance/height around the player; `raySphere` + `closestHit`
- ImGui menus, plus HUD via `GetForegroundDrawList()` with a loaded TTF font
- Best scores in `scores.txt`
- Tuning sliders for every scenario parameter in the debug window

Build B180 all the way to the results screen first. The other two reuse the same structure.

---

## Phase 3: Graphics (days 6–12)

### 3a. The room: textures, normal mapping, anisotropic filtering (day 6)

**Procedural room mesh.** Build 6 inward-facing quads (normals point into the room, winding reversed so the inside faces survive back-face culling). Scale UVs by wall size, so `u = x * tilesPerMetre`. The grid then stays square on every wall, whatever its dimensions. Tangent = the direction U increases along that wall.

**Normal mapping in the pixel shader:**
```hlsl
float3 N = normalize(input.normal);
float3 T = normalize(input.tangent.xyz - dot(input.tangent.xyz, N) * N);  // Gram-Schmidt
float3 B = cross(N, T) * input.tangent.w;                                 // w = handedness
float3 n = normalMap.Sample(s0, input.uv).xyz * 2.0 - 1.0;
N = normalize(mul(n, float3x3(T, B, N)));
```
Transform the tangent with `World` in the VS, the same way you do the normal.

**Grid look without a texture**, if you want the KovaaK's style. Draw anti-aliased lines with `fwidth`:
```hlsl
float2 g = abs(frac(uv - 0.5) - 0.5) / fwidth(uv);
float line = 1.0 - saturate(min(g.x, g.y));
albedo = lerp(albedo, gridColour, line);
```
`fwidth` keeps the lines one pixel wide at any distance, which is a nice contrast with the mipmapped texture approach for the write-up.

**Free assets:** ambientCG and Poly Haven have CC0 tile and concrete textures with normal maps.

### 3b. Shadow mapping with PCF (days 7–8)

This one is useful for gameplay as well as looks: a shadow under a B180 bot tells the player where it will land.

1. **Shadow map:** a 2048² `DepthTarget` (from 0.6). You need a second DSV slot, so make Core's DSV heap bigger.
2. **Light matrices.** Add `Matrix::orthographic(w, h, n, f)`, which matches your `perspective` convention:
   `p[0] = 2/w; p[5] = 2/h; p[10] = 1/(f-n); p[11] = -n/(f-n); p[15] = 1;`
   `lightView = lookAt(roomCentre - lightDir * 30, roomCentre, up)`. Use `up = (0,0,1)` if the light points straight down. Size the ortho box to just fit the room, since a tighter box means sharper shadows.
3. **Shadow pass**, before the main pass:
   - transition shadow map → `DEPTH_WRITE`, clear to 1, viewport 2048²
   - bind the shadow PSO (no RT, no PS, `slopeBias ≈ 2`, `depthBias` tuned by eye)
   - `shadow.hlsl` VS: `WVP = lightVP * world`. Your `Shader` class handles this unchanged.
   - draw targets (and the room if you want self-shadowing)
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
5. **Problems you'll probably hit:** shadow acne (raise the bias), peter-panning (bias too high, so shadows detach from the object), a shadow cut off at the edges (the ortho box is too small). Add an ImGui checkbox that draws the shadow map in a corner (`ImGui::Image` with its SRV handle). It's a great debugging tool and a good screenshot for the report.

### 3c. HDR, tonemapping, bloom (day 9)

You'll already be rendering into `R16G16B16A16_FLOAT` from Phase 0, so values above 1.0 survive.

1. **Emissive targets:** in the PS, add `emissive * objectColour` with `emissive ≈ 3–6` on targets. Make it brighter for the `flash` on hit.
2. **Bright pass** (fullscreen) into a half-res target: `max(colour - threshold, 0)`.
3. **Blur.** Either of these works:
   - *Simple:* ping-pong a separable 9-tap Gaussian (horizontal, then vertical) at half res, 2 iterations.
   - *Extra marks:* the downsample/upsample chain (the Call of Duty: Advanced Warfare method). Downsample with a 13-tap filter through 5 mip levels, then upsample with a 3×3 tent filter, adding back at each level. This gives a wide, stable bloom that doesn't flicker on small moving targets, and it's well documented online.
4. **Composite + tonemap** to the backbuffer:
```hlsl
float3 c = hdr.Sample(s1, uv).rgb + bloom.Sample(s1, uv).rgb * bloomStrength;
c *= exposure;
c = saturate((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14));  // ACES fit (Narkowicz)
return float4(c, 1);   // the SRGB backbuffer applies gamma
```
5. ImGui draws **after** this pass, so the UI isn't bloomed or tonemapped.

Put exposure, threshold and strength on sliders, and take before/after screenshots.

### 3d. MSAA + skybox (day 10)

**MSAA 4×.** Small, distant spheres crawl and shimmer without it, and in an aim trainer that hurts readability, so it's worth arguing for in the report.
- Check support: `CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS)` for `R16G16B16A16_FLOAT` and 4 samples.
- Create the scene colour and depth targets with `SampleDesc.Count = 4`, and scene/skybox/particle PSOs with `samples = 4`. The shadow pass and post passes stay at 1.
- After the scene: colour → `RESOLVE_SOURCE`, resolve target → `RESOLVE_DEST`, then `ResolveSubresource(dst, 0, src, 0, R16G16B16A16_FLOAT)`. Post-processing reads the resolved target.
- Optional discussion point: resolving *before* tonemapping lets very bright edges stay aliased. A tonemap-aware resolve fixes it, and mentioning it shows you understand the problem.

**Skybox.** Controlsphere is 360°, so the player sees everything around them.
- Use a cubemap from 6 face images: texture with `DepthOrArraySize = 6`, SRV as `TEXTURECUBE`. Upload each face like a normal texture (subresource = face × mipCount + mip). Poly Haven HDRIs can be converted to faces with free online tools.
- Draw a cube *after* the opaque geometry, using the view matrix with its translation removed (the sky never gets closer). In the VS: `output.position = mul(VP, float4(pos, 1)).xyww;` sets depth to exactly 1. Use `LESS_EQUAL` depth, no depth write, front-face culling. Sample with the direction `pos`.
- No-asset fallback: a procedural gradient sky in the PS (`lerp(horizon, zenith, dir.y)`, plus a sun disc from `pow(dot(dir, -lightDir), 500)`).
- If you get to PBR, you can sample the skybox for ambient and reflections too.

### 3e. Hit particles: instancing + additive blending (day 11)

This ticks off **instancing** from your Todo list.
- On kill, spawn 30–40 particles at the target's position (CPU array: position, velocity, life, size, colour). Each frame apply gravity and drag, fade them, and remove dead ones.
- Upload the live particles into a `StructuredBuffer` in an **upload heap, one per frame in flight**. If you overwrite the buffer the GPU is still reading from the previous frame, you'll get flicker. Bind it with `SetGraphicsRootShaderResourceView(4, buffer->GetGPUVirtualAddress())`, which is root param 4 and needs no heap slot.
- `DrawInstanced(6, particleCount, 0, 0)`. The VS builds a camera-facing quad from `SV_VertexID` and the camera's right/up vectors, and reads `particles[SV_InstanceID]`.
- PS: a soft circle with `saturate(1 - length(uv * 2 - 1))`, emissive > 1 so it blooms. Use the Additive PSO with depth test on and depth write off.

### 3f. Stretch goals (day 12, pick by remaining time)

- **PBR (Cook-Torrance):** GGX normal distribution, Smith-Schlick geometry, Schlick Fresnel, using the albedo, normal and rmax textures. Check the GEM texture packing in your lecture notes before assuming which rmax channel is roughness, metal or AO. The most visible marks, but also the most time.
- **GPU timings per pass:** a timestamp query heap, `EndQuery` around each pass, `ResolveQueryData` into a readback buffer, divided by `GetTimestampFrequency()`. Show milliseconds per pass in ImGui. About two hours, and it gives your report a real performance section (shadow map resolution vs cost, MSAA cost, and so on).
- **Debug view toggles:** normals, albedo only, shadow factor, bloom buffer. Cheap, and they make strong report figures.
- **Fresnel rim on targets:** `pow(1 - saturate(dot(N, V)), 3) * rimColour`, so targets stand out against any background.

---

## Report and video checklist

- A pipeline diagram of your frame: shadow pass → scene (MSAA) → resolve → bright pass → blur → composite/tonemap → ImGui → present
- Before/after screenshots for each feature (from the ImGui toggles)
- Shadow map and bloom buffers shown in debug views
- GPU timings table
- The problems you hit and how you fixed them (shadow acne, sRGB vs linear normal maps, the descriptor heap clash). Markers usually reward this.
- A video showing all three scenarios, with the results screen after a run
- Capture a frame in **PIX** or **RenderDoc** to show the passes. Both are free, and they're also the fastest way to debug a black screen.
