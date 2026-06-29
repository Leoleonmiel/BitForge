# BitForge

EDIT : New repository for the BitForge project, because the original was deleted by a miss manipulation of the GitHub interface, this is the new repository for the project, all the files are here

📊 [**Project presentation (Google Slides)**](https://docs.google.com/presentation/d/1SG5pgmxKgExa6IxlRyvqw_RflE-PIy-k95eL4XF4HJk/edit?usp=sharing)

BitForge is a real-time **Direct3D 12** rendering engine that grew from raw **MASM x86 assembly** into a full **deferred, GPU-driven** renderer. It loads glTF scenes (Sponza by default) and renders them with a complete physically-based lighting and post-process pipeline - shadows, SSAO, screen-space reflections, volumetric fog, and temporal anti-aliasing - wrapped in a multi-language architecture spanning Assembler, C++20 modules, and HLSL.

---

## Table of contents

1. [What's BitForge?](#1-introduction--whats-bitforge)
2. [General pipeline flow](#2-architecture--general-pipeline-flow)
3. [Rendering pipeline](#3-rendering--the-rendering-pipeline)
4. [Optimization](#4-optimization--make-code-great-again)
5. [Demo overview](#5-live-demo)
6. [Project structure](#6-project-structure)
7. [Building & controls](#7-building--controls)
8. [Roadmap - What's next?](#8-roadmap--whats-next)

---

## 1.What's BitForge?

BitForge is a graduation capstone exploring how far a renderer can be pushed when built **from the metal up** - starting in assembly and layering modern graphics techniques on top. It is a deliberately **multi-language** project: low-level windowing in **MASM x86**, engine and application logic in **C++20 (with modules)**, and shading in **HLSL**.

### Timeline

| Period | Milestone |
|---|---|
| **October** | Kickoff - start in MASM x86 assembly |
| **December** | Milestone 1 |
| **January** | DirectX 12 integration - Milestone 2 |
| **February** | Rendering pipeline implementation |
| **March** | (Laptop repair) |
| **April** | Optimization |
| **May-June** | Milestone 3 |

### Key features

- **Assembler implementation** - the Win32 window is created and pumped from hand-written x86 assembly.
- **DirectX 12 integration** - a deferred, GPU-driven renderer built on bindless descriptors and indirect drawing.
- **Multi-language approach** - Assembler + C++20 modules + HLSL working together in one build.

Resources referenced span various graphics authors from **2014-2025** (plus a healthy amount of Google).

---

## 2.General pipeline flow

### Overview & code flow

The application boots through a small, layered stack - assembly at the bottom, C++ modules on top:

```
HelperWindow.asm  (MASM)   window-procedure helpers
RenderWindow.asm  (MASM)   Win32 window class + message loop
       |
Window.ixx        (C++20 module: bf.Window)   thin C++ wrapper over the asm window
       |
main.cpp          (C++20)  composition root: create window, init renderer, run frame loop
```

`main.cpp` is intentionally tiny - it centers and creates the window, initializes the `Dx12Renderer`, loads the scene, and runs the frame loop. Everything else lives behind clean module/class boundaries (the renderer, the preset system, the asset pipeline).

### Pros & cons of the design

| Pros | Cons / trade-offs |
|---|---|
| Full control from the assembly window up | Windows / x64-only |
| Bindless + GPU-driven keeps CPU draw cost minimal | Deferred rendering can't use hardware MSAA (solved with TAA) |
| Modular render graph - passes are reorderable callbacks | Higher G-buffer memory/bandwidth cost |
| C++20 modules give clean compilation boundaries | Module + assembly toolchain needs careful build setup |

---

## 3. Rendering pipeline

BitForge is a **deferred** engine: geometry is rasterized **once** into a set of textures (the G-buffer), then every later stage is a full-screen pass that reads those textures. Each frame the **render graph** executes an ordered list of passes:

![BitForge render pipeline](docs/pipeline.png)

```
Shadow -> GBuffer -> SSAO -> SSAOBlur -> Lighting -> SSR -> DoF -> Fog -> TAA -> ToneMap -> Gizmo -> UI -> Present
```

Only **Shadow** and **GBuffer** draw geometry; after that, **everything is screen-space** and stays in **linear HDR** until the final tone-map.

```
[ Shadow ]  [ GBuffer ]          <- producers (draw geometry)
      |           |
      +-----+-----+
            v
        Lighting                  <- reads G-buffer + shadow map + AO
            v
   SSR -> DoF -> Fog -> TAA       <- screen-space HDR effects
            v
        ToneMap -> Gizmo -> UI -> Present
```

### Deferred rendering - the Render Graph

Passes are `std::function` callbacks that share a `RenderContext` (command list, view/projection, camera). This makes the pipeline **modular**: passes can be reordered, toggled, or added without touching the others. The graph is the backbone everything else plugs into.

### Shadow mapping

A single **directional shadow map** (2048×2048, `D32` via an `R32_TYPELESS` resource with separate depth + sampling views). The Shadow pass renders scene depth **from the sun's point of view**; lighting and fog then sample it. Per-pixel shadowing uses **3×3 PCF** with a depth bias to remove self-shadow acne. The light's orthographic matrix is derived from the scene bounds and shared with the lighting shader.

### Geometry Buffer (G-buffer)

Four render targets in `R16G16B16A16_FLOAT` plus a `D32` depth buffer:

| Target | Contents |
|---|---|
| 0 | World **position** (`.w` = "geometry present" flag) |
| 1 | World **normal** |
| 2 | **Albedo** |
| 3 | **Material** (metallic / roughness / AO) |

The geometry pass writes these and does *no* lighting. Later passes **sample** them to recover the visible surface - so lighting runs once per visible pixel, and the buffers are reused across the whole post chain (SSAO, SSR, fog, DoF, TAA all read from them).

### Screen-Space Ambient Occlusion (& Blur)

Two passes: a **generate** pass samples a 64-point **hemisphere kernel** (randomly rotated by a tiled 4×4 noise texture) against G-buffer depth/normal to estimate contact occlusion, then a **depth-aware blur** removes the noise without bleeding across edges. The result darkens **only the ambient term** of lighting - direct light and shadows stay crisp.

### Lighting & Image-Based Lighting (IBL)

A full-screen pass runs **Cook-Torrance PBR** (GGX + Smith + Schlick), with a metallic/roughness workflow and up to 128 directional/point/spot lights from a structured buffer. Directional lights sample the shadow map.

**IBL is analytic - there is no cubemap.** The "environment" is computed by formula: a procedural sky (`EnvSky`) stands in for the reflection map, a hemisphere irradiance (`EnvIrradiance`) for the diffuse map, and an analytic BRDF approximation replaces the lookup texture. Reflections blur with roughness, ambient is scaled by AO + SSAO, and everything tints with time of day - at zero texture cost.

### Screen-Space Raycast reflections (SSR)

A reflection ray is **marched forward in small steps**; at each step it projects to screen space and checks the G-buffer depth to detect when it crosses behind a surface. On a hit, the already-lit color is pulled back, weighted by Fresnel and smoothness. Cheap (reuses the G-buffer, no ray-tracing acceleration structure) with the inherent limit that only on-screen geometry can be reflected.

### Volumetric Fog

Ray-marched participating media with **Henyey-Greenstein anisotropic scattering** and **shadow-map sampling** for god-rays / light shafts. Density, height falloff, anisotropy, step count, and color are all adjustable.

### Temporal Anti-Aliasing (TAA)

The engine's anti-aliasing solution (deferred can't use MSAA cleanly): **Halton sub-pixel jitter** each frame, **motion-vector reprojection** of the previous frame (history ping-pongs between two HDR targets), and **neighborhood clamping** to reject ghosting - stabilizing the entire image.

### Tone mapping & HDR output

Everything after lighting works in **linear HDR float**; the final pass compresses to the 8-bit backbuffer with a selectable operator (**None / Reinhard / ACES / AgX**) plus camera exposure.

---

## 4. Optimization

### SIMD implementation

Hot math is vectorized with **DirectXMath** compiled for **AVX2 + FMA** (`/arch:AVX2`, fast floating point). Batched vector/matrix operations and scene-bounds (AABB) accumulation live in **`Core/MathSimd.h`** and are used throughout **`Dx12Renderer.cpp`** and the camera - one instruction processing 4-8 floats at a time.

### GPU-driven vs CPU-driven processing

| CPU-driven (classic) | GPU-driven (BitForge default) |
|---|---|
| CPU loops over objects, one draw call each | CPU issues **one** `ExecuteIndirect` |
| Per-object buffer binds | **Unified** vertex/index buffers bound once |
| Draw cost scales with object count | GPU walks a command buffer and draws everything |

At load time, every mesh is merged into one **unified vertex buffer** + **unified index buffer**, with per-object data in an **instance buffer** and per-draw parameters in an **indirect command buffer**. A single `ExecuteIndirect` (driven by a command signature) renders the whole scene - minimal CPU involvement.

```
CPU: "draw everything" (1 call) --> GPU reads command buffer --> draws all meshes --> G-buffer
```

Other optimization angles: a multithreaded **async asset pipeline** (worker threads decode textures / parse glTF off the main thread) and **bindless** descriptors so the lighting pass binds the G-buffer + shadow + light buffers in a single table.

---

## 5.Demo overview

The default scene is **Sponza** with PBR materials, dynamic sun + point lights, and the full effect chain enabled. At runtime you can:

- Fly the cinematic camera (physical FOV, exposure triangle, DoF, shake).
- Toggle every effect and tweak it live via the Dear ImGui overlay (**F1**).
- Inspect the pipeline through the **G-Buffer View** dropdown (position, normal, albedo, metallic, roughness, IBL irradiance, SSAO, SSR ray-hits/reflections, **shadow map**).
- Select and edit lights by clicking their 3D gizmos (ray-sphere picking).
- Save / load / cycle engine configurations with the **preset system** (JSON, F5/F6/F7).

---

## 6. Project structure

```
BitForge/                  (solution root)
|- BitForge/               (the project)
|  |- Main.cpp             <- entry point only (window/scene setup + loop)
|  |- Renderer/            <- Dx12Renderer + Init/Pipeline/Resources/Deferred
|  |- Rendering/           <- render passes & systems
|  |  |- Deferred/  Shadows/  Lighting/  GpuDriven/  Materials/
|  |  |- PostProcess/  SSAO/  TAA/  VolumetricFog/  Graph/  Debug/
|  |- Camera/              <- cinematic camera
|  |- Assets/              <- async asset manager, loaders, streaming, cache
|  |- Config/Presets/      <- preset module system (bf.Preset.*)
|  |- Scene/               <- time of day, scene lighting setup
|  |- Core/                <- timer, SIMD math (MathSimd.h), Vertex
|  |- Input/  UI/          <- input + ImGui layer
|- WorkDirectory/          <- source assets + MASM sources
|  |- Assets/              <- Shaders/, Models/, Sponza/, Textures/
|  +- *.asm                <- RenderWindow.asm, ASMWindow.asm, ASMWindowHelper.asm, ASMColor.asm
|- lib/Window/             <- bf.Window C++20 module (IXX/ + CPP/)
+- x64/<Config>/           <- build output (exe + copied Assets/)
```

`WorkDirectory/Assets` is the **single source of truth** for shaders and models; a post-build step copies it next to the executable. Shaders are compiled from `.hlsl` at runtime via `D3DCompileFromFile`.

---

## 7. Building & controls

**Requirements:** Windows 10/11 with a D3D12 GPU · Visual Studio 2022/2025 (v143+ toolset) with the **Desktop C++** workload + **MASM** · Windows 10 SDK.

**Build:**

1. Open `BitForge.slnx`, select **x64** + **Debug** or **Release**.
2. **Build** (Ctrl+Shift+B) - the post-build step copies `WorkDirectory/Assets` next to the exe.
3. Run.

```powershell
msbuild BitForge\BitForge.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=<repo>\
```

> Release uses Whole Program Optimization + C++20 modules - prefer **Rebuild** for correctness. After editing a shader, do a **Build** so the runtime copy refreshes.

**Controls:**

| Input | Action |
|---|---|
| **W A S D** (Z/Q too) | Move camera |
| **Right-mouse drag** | Look around |
| **Esc** | Quit |
| **F1** | Toggle debug UI |
| **F5 / F6** | Quick-save / quick-load preset |
| **F7** | Cycle saved presets |
| Click a light gizmo | Select & edit that light |

---

## 8. Roadmap - What's next?

**Engine & architecture**
- Cleaner DirectX implementation and architecture refactoring
- Deeper raycast-based system integration
- Continued SIMD optimization
- Model/texture integration scaling toward **1,000+ objects**
- Full scene completion with PBR rendering

**Visual & simulation features**
- Improved visual fidelity
- Collision systems
- Skeletal animation
- Physics integration

**Editor / tooling**
- **Qt viewport** integration
- Viewport + hierarchy tools
- Profiler assistance
- Gizmo systems
---

<sub>Capstone Graduation Project - Leonnel Hammel.</sub>
