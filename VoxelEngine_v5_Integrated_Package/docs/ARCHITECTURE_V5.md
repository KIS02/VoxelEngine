# VoxelEngine Architecture v5 — 실제 적용 구조

## 계층과 책임

```text
Application (Main Loop, Benchmark session, activation boundaries)
├── AssetManager (shared MeshData, Texture2D, MaterialData; global Mesh IDs)
├── SceneManager (owns Scene instances; requested activation is deferred)
│   └── DemoScene : Scene (concrete demo creation and input-driven gameplay)
│       ├── SceneObjectRegistry (SceneObjectId / Transform / Mesh and Material IDs)
│       ├── EntityRegistry (EntityId / EntityKind / optional GridPlacement / visualObject ID)
│       ├── Camera + CameraController
│       └── scene/voxel/VoxelWorld (per-scene VoxelStorage + ChunkMeshCache)
├── UIManager (F3 state, telemetry, crosshair and debug frame data)
├── FrameBuilder (reads Scene, Assets, Camera and UI, assembles RenderFrame, submits)
└── RenderHost (only one backend instantiated)
    └── Software or OpenGL backend (consumes RenderFrame)
```

## 변경된 파일 경로

- `core/Window.*` → `platform/window/Window.*`; Win32 presentation remains in `platform/windows/`.
- `scene/Camera.*`, `scene/CameraController.*` → `scene/camera/*`; `Camera::ProjectionMode` nested enum and `projectionMatrix(aspectRatio)`.
- `scene/DemoScene.*` → `scene/demo/DemoScene.*`, derives `Scene`.
- `scene/DemoFrameBuilder.*` → `render/frame/FrameBuilder.*`; `RenderFrame.h` moved from `render/api/`; `FrameSubmit.cpp` handles the submission side for testability.
- `render/RenderHost.*` → `render/host/RenderHost.*`. Active backend owns only necessary data.
- `world/World.*` → `voxel/chunk/VoxelStorage.*` (Block data and Dirty lifecycle), wrapped by `scene/voxel/VoxelWorld.*` with mesh cache.
- `world/Chunk.*`, `ChunkCoordinates.*`, `world/meshing/*` → `voxel/chunk/*`; `ChunkMeshBuilder` and `ChunkMeshCache` are new names.
- `world/BlockType.h` → `voxel/block/BlockId.h`; shared definitions go to `assets/block/BlockRegistry.h` and `BlockDefinition.h`.
- `world/VoxelRaycaster.*` → `voxel/ray/*`; `world/BlockInteractor.h` → `gameplay/interaction/*`.
- `geometry/CubeMeshFactory.*` → `assets/geometry/primitives/CubeMeshGenerator.*`; added `CylinderMeshGenerator.*`.
- `render/texture/UVTestTexture.*` → `assets/texture/*`.
- `demo/ChunkDemoFactory.*` → `scene/demo/DemoChunkGenerator.*`; removed the unused `demo/OpenGLCubeDemo.*` coupling.
- UI presentation settings (`CrosshairSettings`, `DebugOverlaySettings`) are grouped in `ui/UIFrameData.h`. `RenderFrame` contains `ui` plus world-space `selectionOutline`.
- Scripts: `scripts/verify_r6.ps1` replaced by `scripts/verify_v5.ps1` with v5 clean-build output directory and existing CSV command contract.

## Ownership and lifetime

`Application::Impl` constructs Window first, AssetManager second, SceneManager third, and RenderHost afterwards. Backends are destructed **before** Scenes, and Scenes are destructed before AssetManager and Window. Scene and RenderFrame only reference shared Asset objects, which have stable map-held addresses. Asset Manager uses a single MeshIdGenerator across multiple Scenes, eliminating accidental GPU mesh ID collisions.

The per-scene `VoxelWorld` holds a `VoxelStorage` (block/chunk data) and `ChunkMeshCache`; the mesh cache references the global AssetManager's MeshIdGenerator but does not own it. The cache is destroyed before the AssetManager.

`SceneManager::requestActivation()` queues the target Scene. `applyPendingActivation()` runs at the caller's safe frame boundary. The manager does not remove active or pending Scenes. `Scene::removeObject()` removes a SceneObject and detaches dependent Entity visuals. Call `Scene::removeObject` rather than mutating the Registry directly when an Entity may have a visual reference.

A `RenderFrame` holds borrowed MeshData and MaterialData pointers; Scene changes and asset resource deletion must not occur while it is being rendered. Current RenderHost submission is synchronous at the CPU API level; OpenGL manages its own GPU resource lifetimes. The application currently activates only DemoScene by default. SceneManager supports multiple scenes; switching them from gameplay UI is a future feature, tested through the manager API.

## User features

- `P`: Projection toggle (perspective / orthographic) with proper input press-edge debounce, original View transform unaffected.
- `F3`: Debug HUD stored in UIManager; Selection Outline remains a world-space overlay.
- Default R6 fixed four-Chunk DemoScene and reference Cube are retained, but the cube is now a SceneObject with Asset references.
- Scene Objects and Voxel chunks both produce `RenderDraw` objects in one shared RenderFrame.
- Backend construction no longer creates unused software buffers for OpenGL or an unused OpenGLRenderer for software.
- Backends require a Material with a Texture for each actual draw instead of keeping a Scene-owned default texture reference.

## Known scope limits

- Gameplay `PlayerController`, animated entities, full generic Entity System, scene loading from disk, async ResourceHandles, streaming, physics and full terrain noise generation are future functionality; no empty placeholder classes were created.
- The current Block Registry stores block ID → solid flag / color tint. Per-face texture atlases, UV variants and interactive Entity spawn rules can build on this when needed.
- A Cylinder generator is provided and tested; the reference cube remains the only extra Demo SceneObject rendered by default, preserving the benchmark scene contract.
- A renamed `VoxelStorage` is the generalized raw-voxel container, while Scene-specific aggregation (`VoxelWorld`) owns cache. No `BlockEntity` type exists.
