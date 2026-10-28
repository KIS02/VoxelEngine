# Phase 11.6 Shadow Quality + Performance Integrated Patch

This patch is based on the source files uploaded in the conversation. It has **not been compiled with the local Windows/MSVC build**. Keep the automatic backup and verify the regression tests before committing.

## Included full source files

- `src/render/shadow/ShadowFrameFingerprint.h` (new): fingerprints camera, shadow settings, scene lights, draw mesh identities/revisions, and model matrices.
- `src/render/shadow/ShadowMapping.cpp`: fixed-capacity CPU triangle clipping (no per-triangle vector allocations), slope-based depth-writer offset analogous to OpenGL polygon offset.
- `src/render/backends/software/pipeline/geometry/SoftwareGeometryPass.cpp`: thread-local shadow-map cache; 384px depth map and single-tap sampling for performance; modest receiver bias instead of large diagnostic values.
- `src/render/backends/opengl/pipeline/shadow/OpenGLShadowPass.cpp`: caches the GPU shadow depth map on unchanged inputs and restores correct GL state; defaults to 1024px (the previously working quality), optional medium=512px or low=256px; uses GL_LINEAR depth comparison.
- `src/render/backends/opengl/pipeline/shading/DefaultTextureShaders.h`: replaces nine explicit shader shadow fetches with one hardware 2×2 PCF lookup; preserves the existing uniforms, lighting, texture and HDR paths.

## Important behavior and trade-offs

- Shadow maps are recalculated whenever camera position, caster/light position/direction/range, mesh revision/identity, or object transform changes. This is correct, but moving the camera continuously still incurs a Shadow Pass every frame.
- GPU shadow filtering is a single hardware-filtered 2×2 comparison. This improves performance but has softer/shorter filtering than the original nine-tap PCF. Shadows may look different.
- Software defaults to one hard shadow compare per pixel rather than 3×3 PCF. This is a deliberate performance vs. softness trade-off. CPU and GPU parity need visual verification.
- GPU defaults to **1024×1024** because this resolution was visually validated with normal bias. For speed, choose `VOXELENGINE_SHADOW_QUALITY=medium` (512×512) or `low` (256×256), but inspect the terrain for shadow acne at lower resolutions. Moving the camera can reduce FPS because the depth map is regenerated.
- Since the existing application runs a software color rasterizer with HDR and multi-lighting at full resolution, **1 FPS may not be eliminated solely by shadow optimizations**. Frame timings must be measured again. Achieving interactive Software FPS while moving may require renderer-internal render-scale/tiling/SIMD work that is outside the supplied sources.
- Existing `VOXELENGINE_SHADOW_BIAS_PROBE` is intentionally ignored by the new files. Remove it from the environment to avoid confusion.
- Static cache is per-thread in Software and per-OpenGLShadowPass instance in OpenGL (ownership is cleaned up with the pass). The GPU path expects use from the current owning GL context/thread as in the current engine.

## Install (PowerShell)

From the VoxelEngine project root, assuming the ZIP is located there:

```powershell
Set-Location C:\Users\rladl\Desktop\VoxelEngine
Expand-Archive -Path .\Phase11_6_Shadow_Optimization_Patch.zip -DestinationPath .\shadow_patch -Force
& .\shadow_patch\apply.ps1 -ProjectRoot .

Remove-Item Env:VOXELENGINE_SHADOW_BIAS_PROBE -ErrorAction SilentlyContinue
& .\tools\cmake\bin\cmake.exe --build .\build\dev --config Debug --target VoxelEngine --parallel 4
if ($LASTEXITCODE -ne 0) { throw "VoxelEngine build failed" }
```

Do not modify other files while investigating. The install script prints its timestamped backup folder. To restore it:

```powershell
& .\shadow_patch\restore.ps1 -ProjectRoot . -BackupDirectory .\backups\phase11_6_shadow_optimization_YYYYMMDD_HHMMSS
```

## Compare OpenGL / Software and HDR / SDR

```powershell
$env:VOXELENGINE_DEMO_MULTI_LIGHT = "1"
$env:VOXELENGINE_DEMO_SHADOW = "1"
$env:VOXELENGINE_DEMO_HDR = "1"
$env:VOXELENGINE_SHADOW_QUALITY = "medium"
& .\build\dev\Debug\VoxelEngine.exe --opengl --play --seed 1337
& .\build\dev\Debug\VoxelEngine.exe --software --play --seed 1337
$env:VOXELENGINE_DEMO_HDR = "0"
& .\build\dev\Debug\VoxelEngine.exe --opengl --play --seed 1337
```

Omitting the quality variable uses the proven 1024px default; `medium` enables 512px and `low` enables 256px. Run only one engine instance at a time, keep the view comparable, and record the printed `[Benchmark][CPU] Wall`, `[Benchmark][Render] Work`, and `[Benchmark][GPU] GPU Render` values, plus actual FPS. Test both stationary and moving camera states: the caching improvement is expected to be larger when stationary.

## Validation after build

Run the project regression suite and the Phase 11.3-11.6 tests. In particular verify:

1. MaterialKey 0-3 textures are still assigned to distinct materials, all scene objects still draw.
2. Shadow OFF shows no shadow artifacts. Shadow ON shows terrain and freshly placed block shadows, checking both the default 1024px and the optional `medium` 512px quality.
3. OpenGL + HDR ON, Software + HDR ON, OpenGL + HDR OFF are each rendered correctly.
4. Moving the camera, modifying/removing a chunk, and changing a light transform must invalidate and rebuild the shadow cache correctly.
5. Watch for existing shader contract tests that depend on nine explicit PCF taps. This patch intentionally changes the filtering algorithm but retains all shader uniforms.

This is a source-level performance patch, not a guarantee of any target FPS. If the performance counters stay near 7/1/7 FPS, the next target is the profiling of the rasterizer/fragment shader and render resolution, not additional blind bias adjustments.
