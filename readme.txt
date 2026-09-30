< 사용법 >

○ Visual Code로 폴더 Open한 뒤, F5 눌러서 실행

○ Compile Error 발생 시 프로젝트의 build 폴더 삭제 후 실행


< 프로젝트 구조 ver1 >

VoxelEngine
│
├─ Application
│   ├─ 프로그램 lifetime 관리
│   ├─ Main Loop
│   ├─ Update
│   └─ 전체 시스템 연결
│
├─ Window
│   ├─ 창 생성
│   ├─ OS Event
│   └─ Input 접근
│
├─ Timer
│   └─ Delta Time
│
└─ Renderer
    └─ 아직 없음

< 프로젝트 제작계획 >
VoxelEngine
│
├─ Phase 0 — Engine / Platform Foundation
│  │
│  ├─ Portable Development Environment
│  │  ├─ 프로젝트 내부 MinGW-w64 사용
│  │  ├─ 프로젝트 내부 GLFW 사용
│  │  ├─ 외부 절대경로 의존성 제거
│  │  ├─ VS Code 프로젝트 단위 Build
│  │  ├─ build.bat 기반 Build
│  │  └─ Windows 10/11 x64 Portable 환경
│  │
│  ├─ Application
│  │  ├─ 프로그램 Lifetime 관리
│  │  ├─ Main Loop 관리
│  │  ├─ Input 처리 흐름 관리
│  │  ├─ Update 호출
│  │  ├─ Render 호출
│  │  └─ 전체 Engine Subsystem 연결
│  │
│  ├─ Window
│  │  ├─ GLFW 기반 Window 생성
│  │  ├─ Window 크기 관리
│  │  ├─ Window Close 상태 관리
│  │  ├─ OS Event Polling
│  │  ├─ Keyboard / Mouse Input 접근
│  │  ├─ Native Window Handle 제공
│  │  └─ Renderer와 분리
│  │
│  ├─ GLFW
│  │  ├─ Window 생성
│  │  ├─ Input
│  │  ├─ Event
│  │  └─ GLFW_NO_API 사용
│  │
│  ├─ Graphics API 상태
│  │  ├─ OpenGL Context 없음
│  │  ├─ glfwMakeContextCurrent() 사용 안 함
│  │  ├─ glfwSwapInterval() 사용 안 함
│  │  ├─ glfwSwapBuffers() 사용 안 함
│  │  └─ Window와 Renderer 완전 분리
│  │
│  ├─ Timer
│  │  ├─ std::chrono 기반 시간 측정
│  │  ├─ Frame 간 시간 측정
│  │  └─ Delta Time 계산
│  │
│  └─ Main Loop
│     ├─ Poll Events
│     ├─ Delta Time 계산
│     ├─ Process Input
│     ├─ Update
│     ├─ Render
│     └─ 다음 Frame 반복
│
│
├─ Phase 1 — Math Foundation
│  │
│  ├─ Vec3
│  │  ├─ x / y / z
│  │  ├─ Vector Addition
│  │  ├─ Vector Subtraction
│  │  ├─ Scalar Multiplication
│  │  ├─ Scalar Division
│  │  ├─ Unary Minus
│  │  ├─ += / -= / *= / /=
│  │  └─ 3D Position / Direction 표현
│  │
│  ├─ Vector Operations
│  │  ├─ Length Squared
│  │  ├─ Length
│  │  ├─ Normalize
│  │  ├─ Dot Product
│  │  └─ Cross Product
│  │
│  ├─ Vec4
│  │  ├─ x / y / z / w
│  │  ├─ Vec3 → Vec4 변환
│  │  ├─ Vec4 → Vec3 변환
│  │  ├─ Vector Arithmetic
│  │  └─ Homogeneous Coordinate 표현
│  │
│  ├─ Homogeneous Coordinate
│  │  ├─ Position
│  │  │  └─ w = 1
│  │  ├─ Direction
│  │  │  └─ w = 0
│  │  ├─ Translation의 Matrix 표현 가능
│  │  └─ Perspective Divide 기반 제공
│  │
│  ├─ Mat4
│  │  ├─ 4 × 4 Matrix
│  │  ├─ m[row][column]
│  │  ├─ Zero Matrix
│  │  ├─ Identity Matrix
│  │  └─ Matrix Element 접근
│  │
│  ├─ Matrix × Vector
│  │  ├─ Mat4 × Vec4
│  │  ├─ Row × Vector 계산
│  │  └─ Coordinate Transformation 기반
│  │
│  ├─ Matrix × Matrix
│  │  ├─ Mat4 × Mat4
│  │  ├─ Row × Column
│  │  ├─ 여러 Transform 결합
│  │  └─ Matrix Multiplication Order 이해
│  │
│  ├─ Translation Matrix
│  │  ├─ Position 이동
│  │  ├─ w = 1에 Translation 적용
│  │  └─ w = 0 Direction에는 Translation 미적용
│  │
│  ├─ Scale Matrix
│  │  ├─ X Scale
│  │  ├─ Y Scale
│  │  ├─ Z Scale
│  │  └─ Non-uniform Scale
│  │
│  ├─ Rotation Matrix
│  │  ├─ Degree ↔ Radian
│  │  ├─ sin / cos 기반 2D Rotation
│  │  ├─ Rotation X
│  │  ├─ Rotation Y
│  │  ├─ Rotation Z
│  │  ├─ Right-Hand Rule
│  │  └─ Rotation Order
│  │
│  ├─ MathUtil
│  │  ├─ Pi
│  │  ├─ toRadians()
│  │  └─ toDegrees()
│  │
│  ├─ Transform
│  │  ├─ Position
│  │  ├─ Rotation
│  │  ├─ Scale
│  │  └─ Model Matrix 생성
│  │
│  ├─ Model Matrix
│  │  ├─ Local Space
│  │  ├─ Scale
│  │  ├─ Rotation
│  │  ├─ Translation
│  │  └─ Local Space → World Space
│  │
│  ├─ Camera / View Matrix
│  │  ├─ Eye Position
│  │  ├─ Target
│  │  ├─ Up Direction
│  │  ├─ Forward Vector
│  │  ├─ Right Vector
│  │  ├─ Camera Up Vector
│  │  ├─ LookAt Matrix
│  │  ├─ Camera Transform의 Inverse 개념
│  │  └─ World Space → View Space
│  │
│  ├─ Perspective Projection
│  │  ├─ Field Of View
│  │  ├─ Aspect Ratio
│  │  ├─ Near Plane
│  │  ├─ Far Plane
│  │  ├─ Viewing Frustum
│  │  ├─ Projection Matrix
│  │  └─ View Space → Clip Space
│  │
│  ├─ Clip Space
│  │  ├─ x / y / z / w
│  │  └─ w = -ViewSpace.z
│  │
│  ├─ Perspective Divide
│  │  ├─ x / w
│  │  ├─ y / w
│  │  └─ z / w
│  │
│  ├─ NDC
│  │  ├─ Normalized Device Coordinate
│  │  ├─ X = -1 ~ +1
│  │  ├─ Y = -1 ~ +1
│  │  └─ Z = -1 ~ +1
│  │
│  └─ MVP Pipeline
│     ├─ Local Space
│     ├─ Model Matrix
│     ├─ World Space
│     ├─ View Matrix
│     ├─ View Space
│     ├─ Projection Matrix
│     ├─ Clip Space
│     ├─ Perspective Divide
│     └─ NDC
│
│
├─ Phase 2 — CPU Software Renderer
│  │
│  ├─ Color
│  │  ├─ Color32
│  │  ├─ R / G / B / A
│  │  ├─ 8 bit × 4
│  │  ├─ 32-bit Pixel
│  │  └─ Bit Shift / Packing
│  │
│  ├─ FrameBuffer
│  │  ├─ 800 × 600
│  │  ├─ CPU Pixel Memory
│  │  ├─ std::vector<Color32>
│  │  ├─ 2D → 1D Index
│  │  │  └─ index = y × width + x
│  │  ├─ clear()
│  │  ├─ setPixel()
│  │  ├─ pixel()
│  │  └─ data()
│  │
│  ├─ FrameBuffer Coordinate
│  │  ├─ Origin = Top Left
│  │  ├─ +X = Right
│  │  └─ +Y = Down
│  │
│  ├─ Win32 Frame Presentation
│  │  ├─ Window Native Handle
│  │  ├─ HWND
│  │  ├─ HDC
│  │  ├─ BITMAPINFO
│  │  ├─ Top-Down Bitmap
│  │  ├─ StretchDIBits()
│  │  ├─ FrameBuffer → Window
│  │  └─ Win32 GDI Presentation
│  │
│  ├─ Rasterizer
│  │  ├─ FrameBuffer Reference
│  │  ├─ Primitive → Pixel 변환
│  │  └─ Software Rasterization 담당
│  │
│  ├─ Pixel Rasterization
│  │  └─ setPixel()
│  │
│  ├─ Line Rasterization
│  │  ├─ 두 Screen Point 입력
│  │  ├─ Continuous Line → Discrete Pixel
│  │  ├─ Bresenham Line Algorithm
│  │  ├─ dx / dy
│  │  ├─ Step X / Step Y
│  │  ├─ Error 누적
│  │  ├─ Horizontal Line
│  │  ├─ Vertical Line
│  │  ├─ Diagonal Line
│  │  └─ 모든 방향 Line 처리
│  │
│  ├─ Triangle Wireframe
│  │  ├─ Vertex A
│  │  ├─ Vertex B
│  │  ├─ Vertex C
│  │  ├─ A → B Line
│  │  ├─ B → C Line
│  │  └─ C → A Line
│  │
│  ├─ Filled Triangle
│  │  ├─ Triangle Bounding Box
│  │  ├─ Pixel 내부/외부 판정
│  │  └─ Triangle Interior 채우기
│  │
│  ├─ Edge Function
│  │  ├─ Triangle Edge 방향 판정
│  │  ├─ Point가 Edge 어느 쪽인지 판정
│  │  └─ Triangle Inside Test
│  │
│  ├─ Barycentric Coordinates
│  │  ├─ α / β / γ
│  │  ├─ Triangle 내부 위치 표현
│  │  ├─ α + β + γ = 1
│  │  └─ Attribute Interpolation 기반
│  │
│  ├─ Attribute Interpolation
│  │  ├─ Color Interpolation
│  │  ├─ Depth Interpolation
│  │  ├─ UV Interpolation
│  │  └─ 기타 Vertex Attribute 보간
│  │
│  ├─ Depth Buffer
│  │  ├─ Pixel별 Depth 저장
│  │  ├─ Depth Clear
│  │  ├─ Depth Test
│  │  ├─ 앞 Triangle 표시
│  │  └─ 뒤 Triangle 제거
│  │
│  ├─ Viewport Transform
│  │  ├─ NDC → Screen Coordinate
│  │  ├─ X -1 ~ +1 → 0 ~ Width
│  │  ├─ Y +1 ~ -1 → 0 ~ Height
│  │  └─ NDC Y축 → FrameBuffer Y축 반전
│  │
│  ├─ MVP 연결
│  │  ├─ Local Vertex
│  │  ├─ Model
│  │  ├─ View
│  │  ├─ Projection
│  │  ├─ Clip
│  │  ├─ Perspective Divide
│  │  ├─ NDC
│  │  └─ Screen Coordinate
│  │
│  ├─ 3D Triangle
│  │  ├─ 3D Vertex 입력
│  │  ├─ MVP Transform
│  │  ├─ Perspective Projection
│  │  ├─ Screen Triangle 생성
│  │  └─ Rasterization
│  │
│  ├─ 3D Cube
│  │  ├─ Cube Vertex
│  │  ├─ Cube Index
│  │  ├─ 12 Triangles
│  │  ├─ Model Transform
│  │  ├─ Camera
│  │  ├─ Perspective
│  │  └─ CPU Rendering
│  │
│  ├─ Back-face Culling
│  │  ├─ Triangle 방향 판정
│  │  ├─ Front Face
│  │  ├─ Back Face
│  │  └─ 보이지 않는 Triangle 제거
│  │
│  └─ Basic Texture Mapping
│     ├─ UV Coordinate
│     ├─ Texture Pixel Sampling
│     ├─ UV Interpolation
│     ├─ Perspective 문제 확인
│     └─ Perspective-Correct Interpolation
│
│
├─ Phase 3 — GPU Renderer Foundation
│  │
│  ├─ Graphics API Backend 선택
│  │  ├─ OpenGL
│  │  └─ 이후 Vulkan 확장 가능
│  │
│  ├─ Graphics Context / Device
│  │  ├─ GPU와 연결
│  │  ├─ Rendering Context
│  │  └─ API Initialization
│  │
│  ├─ GPU Memory
│  │  ├─ CPU Memory
│  │  ├─ GPU Memory
│  │  └─ CPU → GPU Upload
│  │
│  ├─ Vertex Buffer
│  │  ├─ Vertex Data
│  │  └─ GPU Buffer Upload
│  │
│  ├─ Index Buffer
│  │  ├─ Triangle Index
│  │  └─ Indexed Rendering
│  │
│  ├─ Vertex Layout
│  │  ├─ Position
│  │  ├─ Normal
│  │  ├─ UV
│  │  └─ Vertex Attribute Layout
│  │
│  ├─ Shader
│  │  ├─ Vertex Shader
│  │  ├─ Fragment Shader
│  │  ├─ Compile
│  │  ├─ Link
│  │  └─ Uniform / Resource 전달
│  │
│  ├─ Texture
│  │  ├─ GPU Texture 생성
│  │  ├─ Texture Upload
│  │  ├─ Sampling
│  │  └─ Filtering
│  │
│  ├─ Draw Call
│  │  ├─ Bind Resources
│  │  ├─ Bind Pipeline State
│  │  └─ Draw Indexed
│  │
│  └─ CPU Renderer와 GPU 비교
│     ├─ CPU Matrix Transform ↔ Vertex Shader
│     ├─ CPU Rasterizer ↔ Hardware Rasterizer
│     ├─ Pixel 계산 ↔ Fragment Shader
│     ├─ DepthBuffer ↔ GPU Depth Test
│     └─ FrameBuffer ↔ GPU Render Target
│
│
├─ Phase 4 — Renderer Architecture
│  │
│  ├─ Renderer
│  │  ├─ Rendering Entry Point
│  │  ├─ Draw Submission
│  │  └─ Graphics Backend 호출
│  │
│  ├─ Mesh
│  │  ├─ Vertex Data
│  │  ├─ Index Data
│  │  ├─ Vertex Buffer
│  │  └─ Index Buffer
│  │
│  ├─ Material
│  │  ├─ Shader
│  │  ├─ Texture
│  │  └─ Rendering Parameters
│  │
│  ├─ Camera
│  │  ├─ Position
│  │  ├─ Orientation
│  │  ├─ View Matrix
│  │  └─ Projection Matrix
│  │
│  ├─ Render Submission
│  │  ├─ Mesh
│  │  ├─ Material
│  │  └─ Transform
│  │
│  ├─ GPU Resource
│  │  ├─ Buffer
│  │  ├─ Texture
│  │  ├─ Shader
│  │  └─ Resource Lifetime
│  │
│  └─ CPU / GPU Data 분리
│     ├─ CPU Mesh Data
│     ├─ GPU Mesh Resource
│     └─ Upload / Destroy 관리
│
│
├─ Phase 5 — Voxel Foundation
│  │
│  ├─ BlockType
│  │  ├─ Air
│  │  ├─ Dirt
│  │  ├─ Grass
│  │  ├─ Stone
│  │  └─ 기타 Block ID
│  │
│  ├─ Block Data
│  │  ├─ 최소 데이터 표현
│  │  ├─ BlockType 저장
│  │  └─ 불필요한 Object Allocation 방지
│  │
│  ├─ Chunk
│  │  ├─ Fixed Chunk Size
│  │  ├─ Block Array
│  │  ├─ CPU Memory Layout
│  │  └─ Cache-friendly Data
│  │
│  ├─ 3D → 1D Index
│  │  ├─ X
│  │  ├─ Y
│  │  ├─ Z
│  │  └─ Linear Index 계산
│  │
│  ├─ Chunk Coordinate
│  │  ├─ World Coordinate
│  │  ├─ Chunk Coordinate
│  │  └─ Local Block Coordinate
│  │
│  └─ Coordinate Conversion
│     ├─ World → Chunk
│     ├─ World → Local
│     └─ Chunk + Local → World
│
│
├─ Phase 6 — Voxel Meshing
│  │
│  ├─ Naive Meshing
│  │  ├─ Block 하나당 Cube
│  │  ├─ Cube당 6 Faces
│  │  └─ Cube당 12 Triangles
│  │
│  ├─ Face Generation
│  │  ├─ Left
│  │  ├─ Right
│  │  ├─ Top
│  │  ├─ Bottom
│  │  ├─ Front
│  │  └─ Back
│  │
│  ├─ Neighbor Check
│  │  ├─ 인접 Block 검사
│  │  └─ Air 여부 확인
│  │
│  ├─ Hidden Face Culling
│  │  ├─ 내부 Face 제거
│  │  └─ 보이는 Face만 생성
│  │
│  ├─ Chunk Mesh
│  │  ├─ Vertex Array
│  │  ├─ Index Array
│  │  └─ Chunk 단위 Mesh
│  │
│  ├─ Chunk Dirty State
│  │  ├─ Block 변경 감지
│  │  ├─ Dirty Flag
│  │  └─ Mesh Rebuild
│  │
│  └─ Chunk Boundary
│     ├─ Neighbor Chunk 검사
│     └─ Chunk 경계 Face 처리
│
│
├─ Phase 7 — Voxel World
│  │
│  ├─ World
│  │  ├─ 여러 Chunk 소유
│  │  └─ Chunk Lifetime 관리
│  │
│  ├─ Chunk Storage
│  │  ├─ Chunk Coordinate
│  │  └─ Chunk Lookup
│  │
│  ├─ Block Access
│  │  ├─ getBlock()
│  │  ├─ setBlock()
│  │  └─ World Coordinate 기반 접근
│  │
│  ├─ Neighbor Chunk
│  │  ├─ Chunk 경계 Block 조회
│  │  └─ Meshing 연결
│  │
│  ├─ Procedural Generation
│  │  ├─ Height Generation
│  │  ├─ Terrain Layer
│  │  └─ 기본 World Generation
│  │
│  └─ Chunk Lifetime
│     ├─ Chunk 생성
│     ├─ Chunk 유지
│     └─ Chunk 제거
│
│
├─ Phase 8 — Minecraft-like Interaction
│  │
│  ├─ FPS Camera
│  │  ├─ Position
│  │  ├─ Yaw
│  │  ├─ Pitch
│  │  └─ View Direction
│  │
│  ├─ Mouse Look
│  │  ├─ Mouse Delta
│  │  ├─ Yaw Update
│  │  └─ Pitch Update
│  │
│  ├─ Player Movement
│  │  ├─ W / A / S / D
│  │  ├─ Forward
│  │  ├─ Right
│  │  ├─ Speed
│  │  └─ Delta Time
│  │
│  ├─ Gravity
│  │  ├─ Vertical Velocity
│  │  └─ Ground 처리
│  │
│  ├─ Collision
│  │  ├─ Player Bounding Box
│  │  ├─ Block Collision
│  │  └─ Movement Resolution
│  │
│  ├─ Voxel Raycast
│  │  ├─ Camera Ray
│  │  ├─ DDA
│  │  ├─ Grid Traversal
│  │  └─ Hit Block
│  │
│  ├─ Block Breaking
│  │  ├─ Target Block
│  │  ├─ Block 제거
│  │  └─ Chunk Dirty
│  │
│  └─ Block Placement
│     ├─ Hit Face
│     ├─ Neighbor 위치
│     ├─ Block 추가
│     └─ Chunk Dirty
│
│
├─ Phase 9 — Rendering Features
│  │
│  ├─ Texture Atlas
│  │  ├─ 여러 Block Texture 통합
│  │  ├─ UV Region
│  │  └─ BlockType → UV
│  │
│  ├─ Texture Filtering
│  │  ├─ Nearest
│  │  ├─ Linear
│  │  └─ Pixel Art 처리
│  │
│  ├─ Mipmap
│  │  ├─ Distance Texture Sampling
│  │  └─ Aliasing 감소
│  │
│  ├─ Directional Lighting
│  │  ├─ Surface Normal
│  │  ├─ Light Direction
│  │  └─ Dot Product
│  │
│  ├─ Ambient Occlusion
│  │  ├─ Voxel Neighbor 검사
│  │  ├─ Vertex AO
│  │  └─ Face 음영
│  │
│  ├─ Fog
│  │  ├─ Camera Distance
│  │  └─ Far Chunk 자연스럽게 숨기기
│  │
│  ├─ Sky
│  │  ├─ Background
│  │  └─ Sky Color
│  │
│  └─ Shadow
│     ├─ Directional Light
│     ├─ Shadow Map 기초
│     └─ Depth Rendering
│
│
├─ Phase 10 — Optimization
│  │
│  ├─ Profiling
│  │  ├─ CPU Frame Time
│  │  ├─ Render Time
│  │  ├─ Meshing Time
│  │  └─ Bottleneck 측정
│  │
│  ├─ Frustum Culling
│  │  ├─ Camera Frustum
│  │  ├─ Chunk Bounding Volume
│  │  └─ 보이지 않는 Chunk Draw 제거
│  │
│  ├─ Greedy Meshing
│  │  ├─ 인접 Face 병합
│  │  ├─ Vertex 감소
│  │  ├─ Triangle 감소
│  │  └─ Draw Cost 감소
│  │
│  ├─ Chunk Streaming
│  │  ├─ Player 중심 Chunk 관리
│  │  ├─ Load Radius
│  │  ├─ Unload Radius
│  │  └─ 동적 World 관리
│  │
│  ├─ Multithreaded Meshing
│  │  ├─ Main Thread
│  │  ├─ Worker Thread
│  │  ├─ Mesh Job
│  │  └─ Result Queue
│  │
│  ├─ Thread Pool / Job System
│  │  ├─ Worker Threads
│  │  ├─ Job Queue
│  │  ├─ Synchronization
│  │  └─ Chunk Generation / Meshing Job
│  │
│  ├─ GPU Upload Queue
│  │  ├─ Worker에서 CPU Mesh 생성
│  │  ├─ Main/Render Thread로 전달
│  │  └─ GPU Resource Upload
│  │
│  ├─ Memory Optimization
│  │  ├─ Allocation 감소
│  │  ├─ Memory Pool
│  │  ├─ Arena
│  │  └─ Data Locality
│  │
│  └─ Performance Comparison
│     ├─ Naive Meshing
│     ├─ Hidden Face Culling
│     ├─ Greedy Meshing
│     ├─ Triangle Count
│     ├─ Memory Usage
│     └─ Frame Time / FPS
│
│
├─ Phase 11 — Advanced Graphics Architecture
│  │
│  ├─ Graphics Abstraction
│  │  ├─ GraphicsDevice
│  │  ├─ Buffer
│  │  ├─ Texture
│  │  ├─ Pipeline
│  │  └─ Render Target
│  │
│  ├─ Graphics Backend
│  │  ├─ OpenGL Backend
│  │  └─ Vulkan Backend 확장 가능
│  │
│  ├─ Render Pass
│  │  ├─ Color Attachment
│  │  ├─ Depth Attachment
│  │  └─ Pass 단위 Rendering
│  │
│  ├─ Pipeline State
│  │  ├─ Shader
│  │  ├─ Raster State
│  │  ├─ Depth State
│  │  └─ Blend State
│  │
│  ├─ Resource Binding
│  │  ├─ Buffer
│  │  ├─ Texture
│  │  ├─ Uniform
│  │  └─ Descriptor 개념
│  │
│  ├─ Synchronization
│  │  ├─ CPU / GPU
│  │  ├─ Command Submission
│  │  ├─ Fence
│  │  └─ Semaphore 개념
│  │
│  └─ Presentation
│     ├─ Surface
│     ├─ Swapchain
│     ├─ Back Buffer
│     └─ Present
│
│
└─ Phase 12 — Portfolio / Documentation
   │
   ├─ README
   │  ├─ 프로젝트 목표
   │  ├─ Architecture
   │  ├─ Build 방법
   │  ├─ 주요 기능
   │  └─ 실행 화면
   │
   ├─ Architecture Documentation
   │  ├─ Application Architecture
   │  ├─ Math Pipeline
   │  ├─ Software Renderer
   │  ├─ GPU Renderer
   │  ├─ Voxel Architecture
   │  └─ Threading Architecture
   │
   ├─ Rendering Pipeline Diagram
   │  ├─ Local Space
   │  ├─ World Space
   │  ├─ View Space
   │  ├─ Clip Space
   │  ├─ NDC
   │  ├─ Rasterization
   │  └─ FrameBuffer
   │
   ├─ Performance Documentation
   │  ├─ Before / After
   │  ├─ Triangle Count
   │  ├─ Chunk Count
   │  ├─ Frame Time
   │  ├─ Memory
   │  └─ Optimization Result
   │
   ├─ Technical Decision 기록
   │  ├─ 왜 Software Renderer부터 만들었는가
   │  ├─ 왜 Window / Renderer를 분리했는가
   │  ├─ 왜 Chunk를 사용하는가
   │  ├─ 왜 Hidden Face Culling을 하는가
   │  ├─ 왜 Greedy Meshing을 사용하는가
   │  └─ 어떤 Trade-off가 있었는가
   │
   └─ Portfolio Output
      ├─ Screenshots
      ├─ Demo Video
      ├─ Architecture Diagram
      ├─ Benchmark
      └─ Source Code / Git History