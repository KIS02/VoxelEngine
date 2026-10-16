< 사용법 >

○ Visual Code로 폴더 Open한 뒤, F5 눌러서 실행

○ Compile Error 발생 시 프로젝트의 build 폴더 삭제 후 실행

○ 통합 테스트 명령어 
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_regression_tests.ps1


VoxelEngine : Project Architecture

src/
├── assets/
│   ├── mesh/
│   │   ├── MeshData.h
│   │   └── Vertex.h
│   └── texture/
│       └── Texture2D.h
│
├── geometry/
│   └── CubeMeshFactory.h
│
├── world/
│   └── meshing/
│       └── ChunkMeshFactory.h
│
└── render/
    ├── api/
    ├── common/
    └── backends/
        ├── software/
        │   ├── Rasterizer.h
        │   ├── FrameBuffer.h
        │   └── DepthBuffer.h
        └── opengl/



○ Assets: Define Mesh and Texture

○ Geometry: Generate Mesh

○ World/Meshing: Convert Voxel World into Mesh

○ Render: Rendering completed data

○ Backend: perform actual rendering using CPU or GPU






< Main Loop >

Window::pollEvents()
        ↓
Timer::tick() → Delta Time
        ↓
Application::processInput()
        ↓
CameraController::update / updateLook
        ↓
VoxelRaycaster(Camera.position, Camera.forward)
        ↓
BlockInteractor::breakBlock / placeBlock (클릭 시)
        ↓
World::setBlock → Dirty Chunks
        ↓
Application::rebuildDirtyMeshes()
        ↓
ChunkMeshFactory::create() → MeshData / Revision 갱신
        ↓
Application::createDemoFrame() → RenderFrame
        ↓
IRenderBackend::render(RenderFrame)
        ├── Software Backend → CPU Pixel → Win32FramePresenter
        └── OpenGL Backend → GPU Mesh Cache → GL Draw → SwapBuffers

