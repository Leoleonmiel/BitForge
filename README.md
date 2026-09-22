# BitForge

> New repository — the original was lost to a Git mishap.

📊 [**Project presentation (Google Slides)**](https://docs.google.com/presentation/d/1SG5pgmxKgExa6IxlRyvqw_RflE-PIy-k95eL4XF4HJk/edit?usp=sharing)

A graduation capstone renderer built **from the metal up**: Win32 windowing in hand-written **MASM x86-64**, engine and app logic in **C++20 modules**, shading in **HLSL**. On top of that sits a deferred, GPU-driven **DirectX 12** renderer with a full post-process chain.

---
## Rendering pipeline

Geometry is rasterized **once** into a G-buffer; every later stage is a full-screen pass reading those textures. Everything stays in **linear HDR** until the final tone-map.

![BitForge render pipeline](docs/pipeline.png)

- **Shadows** : one 2048² directional map (`D32`), 3×3 PCF with depth bias; ortho matrix derived from scene bounds
  
- **G-buffer** : four `R16G16B16A16_FLOAT` targets + `D32` depth: world position (`.w` = geometry flag), world normal, albedo, material (metallic/roughness/AO)
  
- **SSAO** : 64-point hemisphere kernel rotated by 4×4 noise, then a depth-aware blur. Darkens only the ambient term
  
- **Lighting** : Cook-Torrance PBR (GGX + Smith + Schlick), up to 128 lights from a structured buffer. **IBL is analytic, no cubemap**: procedural sky for reflections, hemisphere irradiance for diffuse, approximated BRDF — zero texture cost
  
- **SSR** : forward ray-march, projecting each step to screen space and testing G-buffer depth; hits pull back already-lit color weighted by Fresnel and smoothness. Only on-screen geometry reflects
  
- **Volumetric fog** : ray-marched media with Henyey-Greenstein scattering and shadow-map sampling for god-rays
  
- **TAA** : Halton jitter, motion-vector reprojection from a ping-pong history, neighborhood clamping against ghosting
  
- **Tone map** : None / Reinhard / ACES / AgX + camera exposure
  
---
## Optimization

**SIMD** : hot math via DirectXMath built for AVX2 + FMA (`/arch:AVX2`); batched vector/matrix ops and AABB accumulation in `Core/MathSimd.h`

**GPU-driven geometry** : at load time all meshes merge into one unified vertex + index buffer, per-object data into an instance buffer, per-draw params into an indirect command buffer
---
## Controls

| Input | Action |
|---|---|
| **W A S D** (Z/Q too) | Move camera |
| **Right-mouse drag** | Look around |
| **Esc** | Quit |
| **F1** | Toggle debug UI |
| **F5 / F6 / F7** | Quick-save / quick-load / cycle presets |
| Click a light gizmo | Select & edit that light |
