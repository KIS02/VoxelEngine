# Fix 2 – R6 ZIP vs v5 manifest SHA-256 audit

The reuploaded R6 source archive was inspected byte-for-byte. This audit is based on the 2026-10-21 reuploaded archive, and only on files touched by the v5 installer.

- All 189 patch operations pass the content preflight.
- 36 existing files differ only in CRLF/LF line endings; after replacing each CRLF with LF, their full SHA-256 is exactly the manifest’s original SHA-256.
- 89 existing files have an exact SHA-256 match.
- 64 newly created files are absent in the R6 source as required.
- There are zero content differences, missing source files, or preexisting target additions.
- Existing source bytes (including CRLF) will be backed up unchanged, and rollback will restore them byte for byte.
- The payload and manifest are identical to the initial v5 integration package; only the installer/README/audit were updated.

## CRLF-only paths

- `scripts/verify_r6.ps1`
- `src/benchmark/BenchmarkSession.cpp`
- `src/core/Application.cpp`
- `src/demo/OpenGLCubeDemo.cpp`
- `src/geometry/CubeMeshFactory.cpp`
- `src/input/ApplicationInput.cpp`
- `src/input/ApplicationInput.h`
- `src/render/RenderHost.cpp`
- `src/render/RenderHost.h`
- `src/render/api/RenderFrame.h`
- `src/render/backends/opengl/OpenGLOverlayRenderer.cpp`
- `src/render/backends/opengl/OpenGLRenderBackend.cpp`
- `src/render/backends/opengl/OpenGLRenderBackend.h`
- `src/render/backends/software/SoftwareOverlayRenderer.cpp`
- `src/render/backends/software/SoftwareRenderBackend.cpp`
- `src/render/overlay/SoftwareDebugOverlay.h`
- `src/render/texture/UVTestTexture.cpp`
- `src/scene/DemoFrameBuilder.cpp`
- `src/scene/DemoFrameBuilder.h`
- `src/scene/DemoScene.cpp`
- `src/scene/DemoScene.h`
- `src/world/BlockTypeName.h`
- `src/world/meshing/ChunkMeshFactory.cpp`
- `src/world/meshing/ChunkMeshManager.cpp`
- `src/world/meshing/ChunkMeshManager.h`
- `tests/BlockInteractionRegressionTests.cpp`
- `tests/CameraInteractionIntegrationTests.cpp`
- `tests/ChunkMeshFactoryTests.cpp`
- `tests/MaterialBindingContractTests.cpp`
- `tests/MeshIdTests.cpp`
- `tests/R6DependencyBoundaryTests.cpp`
- `tests/R6InputArchitectureTests.cpp`
- `tests/R6SceneArchitectureTests.cpp`
- `tests/RenderArchitectureTests.cpp`
- `tests/TextureConsistencyTests.cpp`
- `tests/WorldStabilityStressTests.cpp`
