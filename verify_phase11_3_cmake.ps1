param(
    [switch]$RunGpuParity
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Get-Location).Path
$cmake = Join-Path $projectRoot 'tools\cmake\bin\cmake.exe'
$ctest = Join-Path $projectRoot 'tools\cmake\bin\ctest.exe'
$build = Join-Path $projectRoot 'build\dev'

if (-not (Test-Path -LiteralPath $cmake -PathType Leaf)) {
    throw "Bundled CMake was not found: $cmake"
}

if (-not (Test-Path -LiteralPath $ctest -PathType Leaf)) {
    throw "Bundled CTest was not found: $ctest"
}

$tests = @(
    'SceneLightComponentTests',
    'GrassSideOrientationTests',
    'VoxelMaterialSubmeshTests',
    'ImageFileLoadingTests',
    'R6SceneArchitectureTests',
    'Phase9SceneModesTests',
    'V5SceneIntegrationTests'
)

foreach ($test in $tests) {
    $path = Join-Path $projectRoot "tests\$test.cpp"
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing test source: $path"
    }
}

if ($RunGpuParity) {
    $tests += 'RenderFrameGpuParityTests'
}

Write-Host '[RUN] Configure optimized VoxelEngine CMake'
& $cmake -S $projectRoot -B $build
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }

Write-Host '[RUN] Build targeted tests and VoxelEngine'
$targets = @('VoxelEngine') + $tests
$buildArguments = @('--build', $build, '--config', 'Debug', '--target') + $targets + @('--parallel', '4')
& $cmake @buildArguments
if ($LASTEXITCODE -ne 0) { throw 'CMake build failed' }

Write-Host '[RUN] Relevant CTest cases'
$pattern = '^(' + ($tests -join '|') + ')$'
& $ctest --test-dir $build -C Debug --output-on-failure -R $pattern
if ($LASTEXITCODE -ne 0) { throw 'Phase 11.3 relevant tests failed' }

Write-Host '[PASS] Phase 11.3 selected tests and VoxelEngine build'
