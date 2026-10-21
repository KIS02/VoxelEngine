# VoxelEngine v5 — 통합 아키텍처 리팩터링 적용 패키지

이 패키지는 **사용자가 2026-10-21 전달한 `VoxelEngine_R6_Final_Source.zip`의 원본 바이트**에 맞추어 제작한 변경 패키지입니다. R6 적용 전 소스, GitHub `main` 또는 다른 버전에 적용할 수 없습니다. **프로젝트 폴더 전체를 덮어씌우는 ZIP이 아니며** 파일별 SHA-256을 확인하는 PowerShell 스크립트로 적용해야 합니다.

## 적용기 Fix 2 — Windows 줄바꿈(LF/CRLF) 자동 판별

로컬 `VoxelEngine_R6_Final_Source.zip`과 v5 원본 매니페스트를 전체 비교한 결과, **불일치한 36개 파일 모두 오직 CRLF(Windows) ↔ LF 줄바꿈 차이**였습니다. C++ 및 PowerShell 코드 내용 변경은 발견되지 않았습니다.

이번 적용기는 실제 원본 SHA-256이 달라도 **CRLF를 LF로 변환한 SHA-256이 정확히 매니페스트 원본과 일치하는 경우에만** 허용합니다. 원본 소스는 정규화하거나 덮어쓰지 않으며, 원본 그대로 백업하고 실제 원본 해시를 원복 트랜잭션에 기록합니다. 코드나 공백 등 그 밖의 수정이 있는 파일은 이전과 같이 즉시 거부합니다.

추가 예외 옵션 없이 아래 명령을 실행하면 됩니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\VoxelEngine_v5_Integrated_Package\apply_v5.ps1 -ProjectRoot .
powershell -NoProfile -ExecutionPolicy Bypass -File .\VoxelEngine_v5_Integrated_Package\apply_v5.ps1 -ProjectRoot . -Apply
```

기존의 `-AllowModifiedLegacyVerifier`는 이제 필요하지 않습니다(호환을 위해 파라미터만 남겨둠). **Fix 1 및 원본 패키지의 C++ 소스, 테스트, payload, manifest는 변경하지 않았습니다.** 파일 전체를 동일한 폴더에 덮어 압축 해제하거나 이전 폴더를 교체하세요. 적용 전 별도의 Git 커밋이나 폴더 백업을 권장합니다.

## 설치 전 확인

- 최신 프로젝트 전체를 Git 커밋 또는 별도 백업으로 보관
- `src`, `tests`, `scripts`, `CMakeLists.txt`가 프로젝트 루트에 존재할 것
- Windows 프로젝트 루트의 `external/glfw`, `external/glad`, `tools/mingw`, `tools/cmake`는 그대로 유지
- 새 아키텍처 적용 후 컴파일과 화면 확인까지 수행해야 최종 승인 가능

## 설치 (프로젝트 루트 PowerShell)

이 ZIP을 프로젝트 루트의 `VoxelEngine_v5_Integrated_Package` 폴더로 압축 해제합니다.

**1. Dry Run — 무변경:**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\VoxelEngine_v5_Integrated_Package\apply_v5.ps1 -ProjectRoot .
```

**2. 백업 후 일괄 적용:**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\VoxelEngine_v5_Integrated_Package\apply_v5.ps1 -ProjectRoot . -Apply
```

원본이 ZIP과 다르면 적용 전에 중단합니다. 정상이면 `.v5_backups\<UTC timestamp>\`에 삭제/교체 파일의 원본을 모두 복사한 뒤 적용합니다. 도중 오류가 발생하면 자동 원복을 시도합니다.

**3. Clean Build와 C++ 전체 테스트:**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_v5.ps1 -CleanBuild
```

**4. Software + OpenGL 고정 Benchmark 및 CSV 계약 검사:**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_v5.ps1 -CleanBuild -RunBenchmarks
```

**5. 실제 화면 확인:**

```powershell
.\build\v5_clean\VoxelEngine.exe --software --play
.\build\v5_clean\VoxelEngine.exe --opengl --play
```

`P`: Perspective / Orthographic 전환 (입력 에지 방식), `F3`: Debug Overlay. WASD / Space / Shift / Mouse / ESC, 블록 선택·파괴·설치와 800×600 Benchmark 동작도 반드시 확인해야 합니다.

## 원복

적용 로그의 백업 경로를 **정확히** 사용하세요. 예를 들어:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\VoxelEngine_v5_Integrated_Package\rollback_v5.ps1 -ProjectRoot . -BackupDirectory .\.v5_backups\<실제_백업폴더명>
```

사용자가 적용 후 수정한 소스가 있다면 `rollback_v5.ps1`은 기본적으로 중단합니다. 그런 수정을 버리고 원복해야 하는 상황에만 `-Force`를 명시하세요. 적용 도중 자동 원복이 실패한 경우에는 `.v5_backups/`의 파일을 수동으로 복구하거나 작업 전 Git 커밋으로 되돌려야 합니다.

## 중요한 검증 범위

Linux C++20 환경에서 기존 39종과 신규 v5 4종, 총 43개 C++ 실행 테스트를 모두 통과했습니다. Windows 전용 `Window.cpp`, Win32 Presenter, GLFW/GLAD/OpenGL 구현 전체의 컴파일·링크, PowerShell 스크립트 실행, GPU/CPU 실제 화면 비교는 여기서 수행할 수 없습니다. 이 단계의 Windows 검증은 **사용자 컴퓨터에서 필수**입니다. `BenchmarkCsvRegressionTests`는 실제 CSV 두 개가 만들어진 경우에만 실행됩니다.

## 정책

- 공유 Asset은 `AssetManager`가 소유하며, Scene이 직접 GPU 리소스를 소유하지 않습니다.
- `BlockEntity`라는 별도 분류는 도입하지 않습니다. 특수 블록은 일반 `Entity` + 선택적 `GridPlacement`로 취급할 수 있습니다.
- `FrameBuilder`가 `RenderFrame`을 만들고 `RenderHost`에 제출합니다. 별도 `FrameRenderer`는 존재하지 않습니다.
- Backend는 Draw마다 `Material`과 `Texture`를 명시적으로 제공받습니다. 장면 Texture를 Backend 생성자에 전달하지 않습니다.
- v5 패치는 R6 원본 파일 해시 검증 및 백업으로 R6 코드를 보호합니다.

세부 구현과 테스트 한계는 `docs/ARCHITECTURE_V5.md` 및 `docs/VALIDATION.md`를 참고하세요.
