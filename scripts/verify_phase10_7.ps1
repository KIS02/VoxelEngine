param(
    [string]$ProjectRoot = "."
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path $ProjectRoot).Path
$cmake = Join-Path $root "tools/cmake/bin/cmake.exe"
$build = Join-Path $root "build/v5_clean"

if (-not (Test-Path $cmake)) {
    throw "Cannot find bundled CMake: $cmake"
}

& $cmake -S $root -B $build
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }

$targets = @("ColoredLightingTests", "ColoredShaderContractTests", "ColoredLightingGpuParityTests", "VoxelEngine")
& $cmake --build $build --target @targets --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Phase 10.7 build failed" }

foreach ($name in @("ColoredLightingTests", "ColoredShaderContractTests", "ColoredLightingGpuParityTests")) {
    $binary = Join-Path $build "$name.exe"
    if (-not (Test-Path $binary)) {
        throw "Test executable not found: $binary"
    }

    & $binary
    if ($LASTEXITCODE -ne 0) {
        throw "Test failed: $name"
    }

    Write-Host "[PASS] $name"
}

Write-Host "[PASS] Phase 10.7 source build and dedicated tests"
Write-Host "Note: Existing 10.6 test source lists may require ColorSpace.cpp linkage and updated color-space expectations."