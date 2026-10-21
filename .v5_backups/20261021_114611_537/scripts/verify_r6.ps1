param(
    [switch]$CleanBuild,
    [switch]$RunBenchmarks,
    [switch]$SkipRegressionTests
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cmake = Join-Path $root 'tools\cmake\bin\cmake.exe'
$gcc = Join-Path $root 'tools\mingw\bin\gcc.exe'
$gxx = Join-Path $root 'tools\mingw\bin\g++.exe'
$make = Join-Path $root 'tools\mingw\bin\mingw32-make.exe'
$buildDir = Join-Path $root 'build\r6_clean'
$env:PATH = (Join-Path $root 'tools\mingw\bin') + ';' + $env:PATH

foreach ($path in @($cmake, $gcc, $gxx, $make)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "R6 verification prerequisite is missing: $path"
    }
}
if (-not (Test-Path -LiteralPath (Join-Path $root 'external\glfw') -PathType Container)) {
    throw 'Missing external\glfw in project root'
}
if (-not (Test-Path -LiteralPath (Join-Path $root 'external\glad\src\gl.c') -PathType Leaf)) {
    throw 'Missing external\glad in project root'
}

if ($CleanBuild) {
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
    Write-Host '[R6] Configuring clean Release build'
    & $cmake -S $root -B $buildDir -G 'MinGW Makefiles' "-DCMAKE_C_COMPILER=$gcc" "-DCMAKE_CXX_COMPILER=$gxx" "-DCMAKE_MAKE_PROGRAM=$make" '-DCMAKE_BUILD_TYPE=Release'
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
    & $cmake --build $buildDir --parallel 4
    if ($LASTEXITCODE -ne 0) { throw 'CMake build failed' }
    Write-Host '[R6] Clean Release build passed'
}

if (-not $SkipRegressionTests) {
    Write-Host '[R6] Running complete source regression suites'
    $powershellExe = (Get-Process -Id $PID).Path
    & $powershellExe '-NoProfile' '-ExecutionPolicy' 'Bypass' '-File' (Join-Path $root 'scripts\run_regression_tests.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'C++ regression suites failed' }
}

if ($RunBenchmarks) {
    $exe = Join-Path $buildDir 'VoxelEngine.exe'
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
        throw 'Run -CleanBuild together with -RunBenchmarks to create the R6 executable'
    }
    $csvDir = Join-Path $root 'benchmark_results'
    New-Item -ItemType Directory -Path $csvDir -Force | Out-Null

    function New-BenchmarkCsv( [string]$BackendName, [string]$CommandFlag ) {
        $oldFiles = @(Get-ChildItem -LiteralPath $csvDir -Filter "$BackendName`_*.csv" -File | ForEach-Object { $_.Name })
        Write-Host "[R6] Running $BackendName benchmark"
        & $exe $CommandFlag '--benchmark' | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "$BackendName benchmark failed" }
        $newFiles = @(Get-ChildItem -LiteralPath $csvDir -Filter "$BackendName`_*.csv" -File |
            Where-Object { $_.Name -notin $oldFiles } | Sort-Object LastWriteTimeUtc -Descending)
        if ($newFiles.Count -ne 1) {
            throw "$BackendName benchmark did not produce exactly one new CSV"
        }
        return $newFiles[0].FullName
    }

    Push-Location $root
    try {
        $softwareCsv = New-BenchmarkCsv 'software' '--software'
        $openGLCsv = New-BenchmarkCsv 'opengl' '--opengl'
        Write-Host '[R6] Comparing Software/OpenGL CSV contracts'
        $powershellExe = (Get-Process -Id $PID).Path
        & $powershellExe '-NoProfile' '-ExecutionPolicy' 'Bypass' '-File' (Join-Path $root 'scripts\run_regression_tests.ps1') '-SoftwareCsv' $softwareCsv '-OpenGLCsv' $openGLCsv
        if ($LASTEXITCODE -ne 0) { throw 'Software/OpenGL benchmark CSV verification failed' }
        Write-Host "[R6][PASS] Software benchmark CSV: $softwareCsv"
        Write-Host "[R6][PASS] OpenGL benchmark CSV: $openGLCsv"
    } finally {
        Pop-Location
    }
}
Write-Host '[R6] Requested verification phases finished successfully'
