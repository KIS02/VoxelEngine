# VoxelEngine v5 Verification

## Performed in this environment

- Read and unpacked the user-supplied `VoxelEngine_R6_Final_Source.zip`; changes are against exact file bytes.
- Built/ran C++20 unit/regression suites on Linux using GCC with include-driven `.cpp` dependency closure. 43/43 executable C++ suites passed (39 existing R6 suites plus 4 new v5 suites), covering original contracts, voxel dirty and chunk removal, geometry, input, camera, SceneManager/SceneObjectRegistry/EntityRegistry, material binding and frame assembly. `BenchmarkCsvRegressionTests` requires actual exported CSV and is intentionally excluded from Linux sample tests.
- C++20 `-fsyntax-only` passed for the platform-independent source compilation group; Win32, OpenGL, GLFW/GLAD builds are not available here.
- Verified no project-local quoted include targets are missing.
- Byte-for-byte Python simulation of all manifest operations (apply and rollback) checks exact reproducibility of source tree contents.

## Must verify on Windows

1. Dry Run SHA-256 preflight; do not use `-Apply` if any source mismatch is detected.
2. `apply_v5.ps1 -Apply` saves originals and installs the patch.
3. `scripts/verify_v5.ps1 -CleanBuild`: clean release build and **all** discovered C++ test suites; `BenchmarkCsvRegressionTests` skips until CSVs are provided.
4. `scripts/verify_v5.ps1 -CleanBuild -RunBenchmarks`: Software and OpenGL benchmark sessions and CSV cross-check. Check scenario, 120 warmup frames and 600 measured frames.
5. Manual Software/OpenGL runs: confirm world, reference cube, block break/place, selection, F3, Escape and P perspective/orthographic toggle.
6. Confirm Windows/OpenGL shader and cached GPU resource lifetimes by switching camera projection and removing/rebuilding chunks.
7. If any verification fails, use backup folder with `rollback_v5.ps1` (without `-Force` unless you intentionally discard edits).

## Limitations

The creator cannot claim the actual Windows build and OpenGL runtime passed because the compiler toolchain, windowing environment, GLFW/GLAD and GPU are not part of the supplied ZIP. PowerShell scripts were created and statically inspected; native PowerShell execution has not been performed here. R6 CSV compatibility is an intended invariance to be established by step 4.
