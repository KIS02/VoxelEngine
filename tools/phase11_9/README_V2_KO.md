# Phase 11.9 Windows PowerShell 검증 스크립트 v2 수정 안내

이번 수정은 **엔진의 C++ 소스를 변경하지 않습니다.** 최초 패키지의 `tools/phase11_9/verify_phase11_9.ps1`만 교체합니다.

## 오류 원인

기존 `Invoke-Checked`는 `$ErrorActionPreference = 'Stop'` 환경에서 외부 프로그램의 `stderr`를 `2>&1`로 받았습니다. Windows PowerShell 5.1은 CMake의 경고를 `ErrorRecord`로 변환할 수 있기 때문에, CMake가 성공 종료했어도 `catch`에서 FAIL로 기록되었습니다.

Visual Studio Generator는 다중 구성 빌드를 사용하므로, Debug/Release는 `--config`를 이용해야 합니다. 기존 `-DCMAKE_BUILD_TYPE=...`은 Multi-Config에서 적용되지 않아 CMake의 `unused-cli` 경고가 나타날 수 있습니다.

## v2에서 변경한 부분

1. CMake/CTest/게임 실행의 성공과 실패를 실제 프로세스 종료 코드인 `$LASTEXITCODE`로 판정합니다. Warning 출력은 로그에 남깁니다.
2. `Invoke-Configure`가 CMake Cache를 확인합니다. Visual Studio 같은 Multi-Config는 `--config Debug/Release`, 단일 구성 Make/Ninja는 `CMAKE_BUILD_TYPE=Debug/Release`를 사용합니다.
3. `ctest -N`으로 실제 등록된 테스트 수를 검사합니다. 등록된 테스트가 없거나 소스 수보다 적으면 전체 회귀 PASS로 잘못 처리하지 않습니다. 별도 입력 파일을 요구하는 `BenchmarkCsvRegressionTests`는 정상적으로 제외합니다.
4. 기존 빌드 폴더 삭제, C++ 코드 수정, 기능 비활성화, Shader/Shadow 코드 변경은 없습니다.

## 적용

1. 현재 프로젝트의 스크립트 백업:

```powershell
Copy-Item .\tools\phase11_9\verify_phase11_9.ps1 .\tools\phase11_9\verify_phase11_9.ps1.bak
```

2. 이 패키지의 `tools/phase11_9/verify_phase11_9.ps1`을 프로젝트의 동일한 상대 경로로 덮어씁니다. ZIP은 프로젝트 루트에 압축을 풀어 병합하면 됩니다.

3. 전체 실행:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\phase11_9\verify_phase11_9.ps1
if ($LASTEXITCODE -ne 0) { throw "Phase 11.9 verification incomplete" }
```

## 해석 주의

- 이전 로그의 `-- Configuring done`, `-- Generating done`은 CMake Configure 성공을 가리킵니다. 기존 스크립트의 FAIL은 경고 처리 오탐입니다.
- 이번 변경으로 테스트 실행 자체가 보장되지는 않습니다. 실제 컴파일 및 테스트 중 새 오류가 드러나면 단계별 로그의 종료 코드와 원문을 분석해야 합니다.
- 스크립트를 새로 실행하면 `phase11_9_results` 아래에 새 타임스탬프 폴더를 만듭니다.
- 이 환경에서는 Windows PowerShell/Visual Studio 전체 실행을 할 수 없어, Windows 통과 여부는 사용자의 실행 결과로 확인해야 합니다.
