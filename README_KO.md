# VoxelEngine Phase 11.9 통합 검증 / Phase 12 시작 패키지

**기준:** Phase 11.6의 실제 Windows 그림자 정상 출력 보고 + Phase 11.7 PBR + Phase 11.8 M/N 전환 참조 소스. **사용자 최신 로컬 소스가 최종 기준이며, 본 패키지는 기존 엔진 파일을 덮어쓰지 않는다.**

## 1. 이번 패키지가 하는 일

- **11.9.1** `ShadowSettings` ABI/ODR 회귀 방지용 신규 C++ 테스트 추가. PBR / HDR / Multi-Light / Shadow / RenderFrame GPU Parity 및 CPU 테스트는 기존 테스트 전부 실행.
- **11.9.2** `TextureSetInputTests`, `TextureSetSceneTests`, `ShadingModeInputTests`, `ShadingModeSceneTests` 등 자동 테스트 포함. 두 실제 PBR 이미지 세트는 실제 화면으로 추가 확인.
- **11.9.3** `tests/*Tests.cpp` 기반 전체 테스트, 독립 Debug 빌드에서 CTest 실행. `BenchmarkCsvRegressionTests`는 실행 인자가 필요해 별도로 실행.
- **11.9.4** 독립 Release 빌드: Blinn–Phong/PBR × OpenGL/Software = **4회** 800×600, Warmup 120 + Measurement 600프레임. 생성된 CSV로 기존 `BenchmarkCsvRegressionTests` 계약 확인.
- **11.9.5** 실행 결과(`phase11_9_results/<timestamp>/results.json`, 단계별 log)를 보존. 실패/미검증이 있으면 자동으로 완료라고 표시하지 않음.
- **Phase 12.0** 기존 Software 렌더링 Profile 옵션을 사용해 병목 측정을 시작. 소스 리팩터링이나 FPS 개선 전 정확성 기준을 고정.

## 2. 안전한 설치

1. 프로젝트 루트의 현재 상태를 백업하거나 Git commit 생성. **이 패키지는 `ShadowMapping.h/.cpp`, Renderer, CMakeLists.txt, World Save를 덮어쓰지 않음.**
2. `Phase11_9_to_Phase12_Kit.zip`의 내부 `tests/`, `tools/` 디렉터리를 **VoxelEngine 프로젝트 루트에 병합**. 생성 파일은 다음 세 개뿐임.
   - `tests/ShadowSettingsLayoutContractTests.cpp` (신규)
   - `tools/phase11_9/verify_phase11_9.ps1` (신규)
   - `tools/phase12/profile_software.ps1` (신규)
3. 최신 `src/render/shadow/ShadowMapping.h`에 `normalOffset`와 `pcfRadius`가 **둘 다 선언**되어 있어야 함. 기존 헤더가 구버전이면 코드를 임의 수정하지 말고 Phase 11.6 최종 소스의 헤더/CPP 정합성을 복원해야 함.
4. `assets/textures/voxel/sets/set_1`, `set_2` 경로 및 파일명은 현재 `VoxelTextureSetConfig.h`에 맞춰 배치. 기존 세트 자동 연결 및 N키 기능은 변경하지 않음.

## 3. 한 번에 검증 실행 (Windows PowerShell, 프로젝트 루트)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\phase11_9\verify_phase11_9.ps1
if ($LASTEXITCODE -ne 0) { throw "Phase 11.9 not fully verified: $LASTEXITCODE" }
```

스크립트는 Debug 전체 소스/테스트 빌드, 전체 CTest, Release 빌드, 4회 Benchmark, 2회 CSV Parity를 순서대로 실행. GPU 테스트가 가능한 Windows 환경이 필요하며 **Software Benchmark는 실행 시간이 길 수 있음.** 기존 `build/dev`나 `build/perf`의 빌드 산출물을 삭제하지 않으며 매 실행마다 시각이 포함된 새 빌드 디렉터리를 생성함. 결과를 `phase11_9_results/`에 기록. 결과 폴더는 Git ignore 대상으로 지정하는 것이 좋음.

진단 목적으로 CPU 테스트만 수행하려면:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\phase11_9\verify_phase11_9.ps1 -SkipGpuTests -SkipBenchmark
```

이 명령은 **전체 Phase 통과 판정 용도가 아니며** 일부 단계를 생략함.

## 4. Shadow 오류 대응

최신 Phase 11.6 기준 구조체 멤버 배치는 `resolution`(0), `nearPlane`(4), `directionalHalfExtent`(8), `directionalDistance`(12), `bias`(16), `slopeBias`(20), `normalOffset`(24), `pcfRadius`(28)로 일반 4바이트 int/float 구성에서 총 32바이트.

`pcfRadius=-858993460`은 16진수 `0xCCCCCCCC`에 해당하며 MSVC Debug 미초기화 메모리 패턴과 일치. 이는 ABI 불일치 또는 미초기화 등으로 발생할 수 있으며, 로그만으로 원인을 단정할 수 없음. 헤더 변경 전후 번역 단위가 섞였을 가능성을 확인하기 위해, 새 빌드 디렉터리에서 모든 `.cpp`를 다시 컴파일하고 새 ABI 계약 테스트를 실행함. **검증 삭제, PCF 값 강제 보정, Shadow Bias 변경은 하지 않음.**

## 5. 실제 화면에서 수동으로 필요한 검증

CTest는 키의 상승 엣지와 Scene 상태, RenderFrame의 일반적 계약을 검증하지만 실제 Windows 화면의 시각적 변화까지 검증하지 못함. `--play --seed 1337`에서 두 Backend를 각각 실행한 뒤:

- F3 HUD: M으로 Blinn–Phong ↔ PBR, N으로 DEFAULT → SET 1 → SET 2 → DEFAULT, P 투영 변경이 서로 독립인지 확인.
- M/N 반복 시 Mesh ID/Revision 및 Chunk 수, Shadow 품질, Material 연결, HDR 화면이 유지되는지 확인.
- 세트 1과 2의 **실제 Normal/Roughness/Metallic/AO 이미지**가 지정한 경로에서 로드되어 차이를 만들어내는지 확인. 맵이 없으면 Fallback이 적용되므로 화면만으로 로딩 성공을 추측하지 말 것.
- CPU/GPU의 화면 비교는 **같은 Seed, 카메라, 해상도, 그림자 품질, Texture Set, ShadingModel**로 수행. 완전히 동일한 픽셀을 기대하기보다 Parity Test 허용 오차와 차이 원인을 확인.

## 6. Benchmark 주의사항

기존 `--benchmark` 모드는 800×600 고정 4 Chunk 장면을 사용하며 World Save가 적용되는 `--play` 모드와 조건이 다름. 이 결과를 이전 사용자 관측치(OpenGL 1000+ FPS / Software 약 5 FPS)와 직접 비교하지 말 것. 벤치마크는 N키 전환(실시간 입력)을 실행하지 않으므로 **기본 텍스처의 Blinn–Phong/PBR 비교**를 제공함. Texture Set 1/2 비교는 별도의 반복 가능한 자동 장면이 확보된 다음 확장.

4개 CSV가 생성되고 CSV Parity가 통과해야 11.9.4가 완료됨. GPU Timing 지원 여부는 실제 드라이버/OpenGL 환경에 따라 달라질 수 있음. 실패 시 측정값을 지어내지 말고 원인을 로그로 남김.

## 7. Phase 11 종료 조건

- 전체 Debug CTest 0 실패(지원되지 않는 GPU 테스트는 명시적으로 미검증).
- ShadowSettings ABI 테스트 + ShadowMappingTests 통과.
- Release OpenGL/Software 두 Shading 모드의 Benchmark CSV 생성 및 Schema/Geometry 계약 통과.
- P/M/N과 두 PBR Texture Set 실제 화면 검증.
- Debug/Release 빌드 성공; 회귀와 성능 비교 기록.

**이 패키지 자체의 Linux 테스트 성공을 사용자 Windows 전체 통과로 간주하지 않는다.** Windows 결과가 모두 PASS일 때 Phase 11을 완료 처리한다.

## 8. Phase 12 첫 실행: Software 프로파일

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\phase12\profile_software.ps1 -Shading BlinnPhong -BuildDirectory "build\phase11_9_YYYYMMDD_HHMMSS_release"
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\phase12\profile_software.ps1 -Shading PBR -BuildDirectory "build\phase11_9_YYYYMMDD_HHMMSS_release"
```

창을 닫으면 로그가 `phase12_results`에 남음. 이전 정상 동작을 보존한 채 **Shadow / Geometry / Cache HIT·MISS** 로그를 모은 후 실제 병목에 따라 세부 최적화 순서를 결정. 어떤 구현 변경도 프로파일 없이 무작정 진행하지 않음.


위 명령의 `YYYYMMDD_HHMMSS`는 검증 스크립트 출력에 기록된 Release 빌드 폴더 이름으로 바꾸어 실행한다. `-BuildDirectory`를 생략하면 과거의 기본 경로를 사용하므로 **생성된 경로를 명시하는 것을 권장**한다.
