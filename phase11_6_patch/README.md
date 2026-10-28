# Phase 11.6 전체 소스 분석 기반 수정 패치

이 패치는 사용자가 제공한 **VoxelEngine_Phase11_6_FullSource.zip**에 들어 있는 실제 소스를 기준으로 작성했다. 기존 Texture·Multi-Material·Light 수집·HDR 경로를 변경하지 않는다.

## 검증된 원인과 수정

1. `src/render/backends/opengl/OpenGLRenderBackend.cpp`: `shadowPass_.render()`가 같은 프레임에 **2번** 호출되고 있었다. 임시 `[ShadowState]` 코드와 중복 호출을 제거하고 본래의 호출 하나를 유지했다.
2. `src/render/lighting/Lighting.cpp`: Software 픽셀마다 `validateMaterialLighting` 및 `validateSceneLights`를 다시 검사했다. `shadeLinearMultiLightValidated()`는 사전 검증된 Software 경로에서만 쓰며, 기존 공개 `shadeLinearMultiLight()`는 매번 유효성 검증을 유지한다.
3. `src/render/backends/software/Rasterizer.cpp`: `lightingValidated=true`가 전달될 때만 검증이 없는 빠른 조명 경로를 사용한다. 다른 기존 호출은 기존 방식 그대로 남아 있다.
4. `src/render/backends/software/pipeline/geometry/SoftwareGeometryPass.cpp`: 전체 Light 검증은 프레임당 한 번, Material 검증은 Draw마다 한 번 수행한다.
5. `src/render/shadow/ShadowMapping.cpp`: Software Shadow Acne를 완화하기 위한 **수신 면 법선 방향 오프셋**을 추가했다. 기본값은 0으로 유지해 다른 사용자와 테스트의 동작을 변경하지 않으며, Software에서는 기본 0.05 블록 단위를 적용한다. 실제 화면에서 최종 확인이 필요하다.
6. `OpenGLRenderBackend.cpp`와 `SoftwareGeometryPass.cpp`: 렌더링 단계별 CPU 벽시계 진단 로그를 `VOXELENGINE_PROFILE_RENDER=1`로 선택적으로 출력한다.
7. `src/platform/window/Window.cpp`: `VOXELENGINE_DISABLE_VSYNC=1`일 때 OpenGL Swap Interval을 0으로 설정하여 VSync 대기 시간과 렌더링 비용을 구분할 수 있다. 기본값은 기존 VSync ON 그대로다.
8. 신규 회귀 테스트: `SoftwareLightingFastPathTests`, `ShadowReceiverOffsetTests`.

**이 패치는 모든 FPS 문제 해결을 보장하지 않는다.** OpenGL이 HDR ON/OFF 모두 7 FPS인 원인은 현재 로그 없이 단정할 수 없다. 본 패치는 이미 존재하던 Shadow Map 캐시를 다시 작성하지 않는다. 이동하는 카메라에서는 재생성이 발생한다.

## 적용

```powershell
Set-Location C:\Users\rladl\Desktop\VoxelEngine
Expand-Archive -Path .\Phase11_6_FullSource_Audit_Fix.zip -DestinationPath .\phase11_6_patch -Force
& .\phase11_6_patch\apply.ps1 -ProjectRoot .
```

백업 경로는 스크립트의 `[DONE] Backup:` 출력에 표시된다. **현재 패치 ZIP에는 CMake/외부 의존성/Texture 파일이 들어 있지 않다.** 파일들을 기존 프로젝트 위에 덮어쓰는 방식이다.

## 테스트: 우선 Debug 빌드

```powershell
& .\tools\cmake\bin\cmake.exe -S . -B .\build\dev
& .\tools\cmake\bin\cmake.exe --build .\build\dev --config Debug --target VoxelEngine ShadowMappingTests ShadowLightingIntegrationTests MultiLightMathTests SoftwareLightingFastPathTests ShadowReceiverOffsetTests --parallel 4
& .\tools\cmake\bin\ctest.exe --test-dir .\build\dev -C Debug -R "^(ShadowMappingTests|ShadowLightingIntegrationTests|MultiLightMathTests|SoftwareLightingFastPathTests|ShadowReceiverOffsetTests)$" --output-on-failure
```

## 성능 비교: 별도 Release 빌드 권장

```powershell
& .\tools\cmake\bin\cmake.exe -S . -B .\build\perf
& .\tools\cmake\bin\cmake.exe --build .\build\perf --config Release --target VoxelEngine --parallel 4

$env:VOXELENGINE_PROFILE_RENDER = "1"
$env:VOXELENGINE_DISABLE_VSYNC = "1" # GPU 성능 진단용, 기본 설정은 VSync ON
$env:VOXELENGINE_DEMO_MULTI_LIGHT = "1"
$env:VOXELENGINE_DEMO_SHADOW = "1"
$env:VOXELENGINE_DEMO_HDR = "1"
Remove-Item Env:VOXELENGINE_SHADOW_BIAS_PROBE -ErrorAction SilentlyContinue

& .\build\perf\Release\VoxelEngine.exe --opengl --play --seed 1337
& .\build\perf\Release\VoxelEngine.exe --software --play --seed 1337
```

`[Profile][OpenGL CPU]`는 Shadow / Geometry / Post+Present 호출의 **CPU 측 경과 시간**을 나타낸다. GPU 실행 시간은 기존 `[Benchmark][GPU]` 로그를 따로 참고한다. `[Profile][Software]`는 Shadow 생성(+캐시 조회) 및 Geometry 비용을 나타낸다. FPS는 같은 해상도·동일한 장면·정지/이동 상태에서 비교한다.

Software Shadow Acne가 남으면 새로운 소스 수정 없이 아래 값을 0, 0.05, 0.10으로 비교할 수 있다. 유효 범위는 0~0.25 블록이다. 값이 커질수록 그림자가 물체와 분리되어 보일 수 있다.

```powershell
$env:VOXELENGINE_SOFTWARE_SHADOW_OFFSET = "0.05"
& .\build\perf\Release\VoxelEngine.exe --software --play --seed 1337
```

## 원복

```powershell
& .\phase11_6_patch\restore.ps1 -ProjectRoot . -BackupRoot ".\.phase11_6_backups\<실제 출력된 폴더명>"
```

## 수행한 오프라인 테스트

- Linux `g++ -std=c++20` 헤더/Software 조명/Shadow 관련 파일의 컴파일 검사 통과.
- `ShadowMappingTests`, `MultiLightMathTests`, `ShadowLightingIntegrationTests` 통과.
- 신규 `SoftwareLightingFastPathTests`, `ShadowReceiverOffsetTests` 통과.
- OpenGL 파일은 프로젝트 ZIP에 `external/glfw` 및 `external/glad`가 없어 여기서 완전 빌드 검증 불가. 실제 MSVC/AMD GPU 환경에서 검증 필요.
