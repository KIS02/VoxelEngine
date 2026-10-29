param(
    [string]$ProjectRoot = "",
    [switch]$SkipBenchmark,
    [switch]$SkipGpuTests
)

# No existing source/build directory is removed or overwritten by this script.
# The full suite uses newly named build directories to avoid mixed object files.
# v2: Windows PowerShell 5.1 native stderr is not a process failure; always check exit codes.
# v2: Configure Debug/Release correctly for Visual Studio multi-config generators.
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}
$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
Set-Location -LiteralPath $ProjectRoot

$cmake = Join-Path $ProjectRoot 'tools\cmake\bin\cmake.exe'
$ctest = Join-Path $ProjectRoot 'tools\cmake\bin\ctest.exe'
if (!(Test-Path -LiteralPath $cmake)) { throw "Missing bundled CMake: $cmake" }
if (!(Test-Path -LiteralPath $ctest)) { throw "Missing bundled CTest: $ctest" }
if (!(Test-Path -LiteralPath (Join-Path $ProjectRoot 'CMakeLists.txt'))) {
    throw 'Run this script from the VoxelEngine project root'
}

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$resultDir = Join-Path $ProjectRoot "phase11_9_results\$stamp"
New-Item -ItemType Directory -Path $resultDir -Force | Out-Null
$debugBuild = Join-Path $ProjectRoot ("build\phase11_9_${stamp}_debug")
$releaseBuild = Join-Path $ProjectRoot ("build\phase11_9_${stamp}_release")

$results = New-Object 'System.Collections.Generic.List[object]'
$csvResults = @{}

function Record-Result {
    param([string]$Name, [string]$Status, [string]$Detail)
    $results.Add([pscustomobject]@{ Stage = $Name; Status = $Status; Detail = $Detail })
    Write-Host "[$Status] $Name $Detail"
}

function Invoke-Checked {
    param([string]$Stage, [string]$Executable, [string[]]$Arguments)

    $safe = $Stage -replace '[^a-zA-Z0-9_-]', '_'
    $logFile = Join-Path $resultDir "$safe.log"
    Write-Host "`n[RUN] $Stage"
    Write-Host "$Executable $($Arguments -join ' ')"

    try {
        # In Windows PowerShell 5.1, stderr redirected with 2>&1 may become an
        # ErrorRecord. Do not make such diagnostic output a terminating error.
        # A native command succeeds/fails according to its process exit code.
        $ErrorActionPreference = 'Continue'

        & $Executable @Arguments 2>&1 |
            ForEach-Object {
                if ($_ -is [System.Management.Automation.ErrorRecord]) {
                    $_.ToString()
                } else {
                    $_
                }
            } |
            Tee-Object -FilePath $logFile -ErrorAction Stop | Out-Host

        $exitCode = $LASTEXITCODE
        if ($null -eq $exitCode) { $exitCode = 1 }
        if ($exitCode -ne 0) {
            Record-Result $Stage 'FAIL' "ExitCode=$exitCode; Log=$logFile"
            return $false
        }
        if (!(Test-Path -LiteralPath $logFile)) {
            Record-Result $Stage 'FAIL' "Log file was not created: $logFile"
            return $false
        }

        Record-Result $Stage 'PASS' "Log=$logFile"
        return $true
    } catch {
        $_ | Out-String | Add-Content -LiteralPath $logFile
        Record-Result $Stage 'FAIL' "Exception=$($_.Exception.Message); Log=$logFile"
        return $false
    }
}

function Invoke-Configure {
    param([string]$Stage, [string]$BuildDirectory, [string]$Configuration)

    # Multi-config generators (Visual Studio, Ninja Multi-Config, Xcode) use
    # --config Debug/Release at build/test time, not CMAKE_BUILD_TYPE.
    $configured = Invoke-Checked $Stage $cmake @(
        '-S', $ProjectRoot, '-B', $BuildDirectory, '-DBUILD_TESTING=ON'
    )
    if (!$configured) { return $false }

    $cacheFile = Join-Path $BuildDirectory 'CMakeCache.txt'
    if (!(Test-Path -LiteralPath $cacheFile)) {
        Record-Result "$Stage generator inspection" 'FAIL' 'CMakeCache.txt was not created'
        return $false
    }

    $multiConfig = @(Select-String -LiteralPath $cacheFile -Pattern '^CMAKE_CONFIGURATION_TYPES:[^=]*=\S+' | Select-Object -First 1).Count -gt 0

    if ($multiConfig) {
        Write-Host "[INFO] Multi-config CMake generator: --config $Configuration"
        return $true
    }

    # Single-config Make/Ninja generators require a separate configuration
    # with the build type set, especially for a meaningful Release benchmark.
    Write-Host "[INFO] Single-config CMake generator: CMAKE_BUILD_TYPE=$Configuration"
    $reconfigured = Invoke-Checked "$Stage build type" $cmake @(
        '-S', $ProjectRoot, '-B', $BuildDirectory,
        '-DBUILD_TESTING=ON', "-DCMAKE_BUILD_TYPE=$Configuration"
    )
    return $reconfigured
}

function Resolve-Executable {
    param([string]$Directory, [string]$Name, [string]$Configuration)

    $withConfig = Join-Path $Directory "$Configuration\$Name.exe"
    if (Test-Path -LiteralPath $withConfig) { return $withConfig }
    $singleConfig = Join-Path $Directory "$Name.exe"
    if (Test-Path -LiteralPath $singleConfig) { return $singleConfig }
    throw "Executable not found: $Name in $Directory"
}

function Find-NewCsv {
    param([string[]]$Before)

    $csvDir = Join-Path $ProjectRoot 'benchmark_results'
    if (!(Test-Path -LiteralPath $csvDir)) { return $null }
    $newFiles = @(Get-ChildItem -LiteralPath $csvDir -Filter '*.csv' -File |
        Where-Object { $_.FullName -notin $Before } |
        Sort-Object LastWriteTime -Descending)
    if ($newFiles.Count -eq 0) { return $null }
    return $newFiles[0].FullName
}

try {
    $layoutTest = Join-Path $ProjectRoot 'tests\ShadowSettingsLayoutContractTests.cpp'
    if (!(Test-Path -LiteralPath $layoutTest)) {
        throw "Missing test: $layoutTest. Copy the kit's tests/ file to your project first."
    }

    $header = Get-Content -LiteralPath (Join-Path $ProjectRoot 'src\render\shadow\ShadowMapping.h') -Raw
    if ($header -notmatch 'normalOffset' -or $header -notmatch 'pcfRadius') {
        Record-Result 'ShadowSettings header contract' 'FAIL' 'Header lacks normalOffset/pcfRadius: do not force-apply an old header'
        throw 'ShadowSettings header is incompatible with the Phase 11.6 receiver-offset path'
    }
    Record-Result 'ShadowSettings header contract' 'PASS' 'Both fields are declared'

    $testSources = @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'tests') -Filter '*Tests.cpp' -File)
    $testNames = @($testSources | ForEach-Object { $_.BaseName } | Sort-Object -Unique)
    if ($testNames.Count -eq 0) { throw 'No standalone tests were discovered' }
    Write-Host "Discovered $($testNames.Count) test source files"

    $testConfigure = Invoke-Configure 'Debug configure' $debugBuild 'Debug'
    $testBuilt = $false
    if ($testConfigure) {
        $testBuildArgs = @('--build', $debugBuild, '--config', 'Debug', '--target', 'VoxelEngine') +
            $testNames + @('--parallel', '4')
        $testBuilt = Invoke-Checked 'Debug engine and test build' $cmake $testBuildArgs
    }

    if ($testBuilt) {
        # CTest exits successfully even when zero tests are registered.
        # Verify registration before claiming that the full suite passed.
        $ctestDiscovered = Invoke-Checked 'CTest discovery' $ctest @(
            '--test-dir', $debugBuild, '-C', 'Debug', '-N'
        )
        if ($ctestDiscovered) {
            $listingLog = Join-Path $resultDir 'CTest_discovery.log'
            $totalLine = @(Select-String -LiteralPath $listingLog -Pattern 'Total Tests:\s*([0-9]+)' | Select-Object -Last 1)
            $registeredCount = -1
            if ($totalLine.Count -gt 0) {
                $registeredCount = [int]$totalLine[0].Matches[0].Groups[1].Value
            }
            $expectedCount = @($testNames | Where-Object {
                $_ -ne 'BenchmarkCsvRegressionTests'
            }).Count

            if ($registeredCount -lt $expectedCount) {
                Record-Result 'Full CTest regression' 'FAIL' (
                    "Only $registeredCount of $expectedCount expected tests registered; " +
                    "review $listingLog and CMakeLists.txt"
                )
            } else {
                $ctestArgs = @('--test-dir', $debugBuild, '-C', 'Debug', '--output-on-failure',
                    '--timeout', '300', '-j', '1', '-E', '^BenchmarkCsvRegressionTests$')
                if ($SkipGpuTests) {
                    $ctestArgs += @('-LE', '^gpu$')
                }
                [void](Invoke-Checked 'Full CTest regression' $ctest $ctestArgs)
            }
        } else {
            Record-Result 'Full CTest regression' 'SKIP' 'CTest discovery failed'
        }
    } else {
        Record-Result 'Full CTest regression' 'SKIP' 'Debug configure/build did not succeed'
    }

    if ($SkipBenchmark) {
        Record-Result 'Release benchmark' 'SKIP' 'Requested -SkipBenchmark'
    } else {
        $releaseConfigured = Invoke-Configure 'Release configure' $releaseBuild 'Release'
        $releaseBuilt = $false
        if ($releaseConfigured) {
            $releaseBuilt = Invoke-Checked 'Release build' $cmake @(
                '--build', $releaseBuild, '--config', 'Release', '--target',
                'VoxelEngine', 'BenchmarkCsvRegressionTests', '--parallel', '4'
            )
        }

        if ($releaseBuilt) {
            $savedEnv = @{}
            $envNames = @('VOXELENGINE_DEMO_PBR', 'VOXELENGINE_DEMO_HDR',
                'VOXELENGINE_DEMO_SHADOW', 'VOXELENGINE_DEMO_MULTI_LIGHT',
                'VOXELENGINE_DISABLE_VSYNC', 'VOXELENGINE_SHADOW_BIAS_PROBE')
            foreach ($name in $envNames) {
                $savedEnv[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
            }

            try {
                [Environment]::SetEnvironmentVariable('VOXELENGINE_DEMO_HDR', '1', 'Process')
                [Environment]::SetEnvironmentVariable('VOXELENGINE_DEMO_SHADOW', '1', 'Process')
                [Environment]::SetEnvironmentVariable('VOXELENGINE_DEMO_MULTI_LIGHT', '1', 'Process')
                [Environment]::SetEnvironmentVariable('VOXELENGINE_DISABLE_VSYNC', '1', 'Process')
                [Environment]::SetEnvironmentVariable('VOXELENGINE_SHADOW_BIAS_PROBE', $null, 'Process')
                $engine = Resolve-Executable $releaseBuild 'VoxelEngine' 'Release'
                $csvExe = Resolve-Executable $releaseBuild 'BenchmarkCsvRegressionTests' 'Release'

                foreach ($mode in @('BlinnPhong', 'PBR')) {
                    [Environment]::SetEnvironmentVariable(
                        'VOXELENGINE_DEMO_PBR', $(if ($mode -eq 'PBR') { '1' } else { '0' }), 'Process'
                    )
                    foreach ($backend in @('opengl', 'software')) {
                        $prior = @()
                        $csvDir = Join-Path $ProjectRoot 'benchmark_results'
                        if (Test-Path -LiteralPath $csvDir) {
                            $prior = @(Get-ChildItem -LiteralPath $csvDir -Filter '*.csv' -File |
                                ForEach-Object { $_.FullName })
                        }

                        $label = "$mode-$backend"
                        $ok = Invoke-Checked "Benchmark $label" $engine @(
                            "--$backend", '--benchmark', '--seed', '1337'
                        )
                        if (!$ok) { continue }

                        $csv = Find-NewCsv $prior
                        if ($null -eq $csv) {
                            Record-Result "Benchmark CSV $label" 'FAIL' 'No new CSV was generated'
                            continue
                        }
                        $csvResults[$label] = $csv
                        Record-Result "Benchmark CSV $label" 'PASS' $csv
                    }

                    $glName = "$mode-opengl"
                    $swName = "$mode-software"
                    if ($csvResults.ContainsKey($glName) -and $csvResults.ContainsKey($swName)) {
                        [void](Invoke-Checked "Benchmark CSV parity $mode" $csvExe @(
                            $csvResults[$glName], $csvResults[$swName]
                        ))
                    } else {
                        Record-Result "Benchmark CSV parity $mode" 'SKIP' 'GPU/CPU CSV pair missing'
                    }
                }
            } finally {
                foreach ($name in $envNames) {
                    [Environment]::SetEnvironmentVariable($name, $savedEnv[$name], 'Process')
                }
            }
        } else {
            Record-Result 'Release benchmark' 'SKIP' 'Release configure/build did not succeed'
        }
    }
} catch {
    Record-Result 'Orchestrator' 'FAIL' $_.Exception.Message
} finally {
    $resultPath = Join-Path $resultDir 'results.json'
    @($results.ToArray()) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $resultPath -Encoding UTF8
    $results | Format-Table -AutoSize | Out-Host
    $failCount = @($results | Where-Object { $_.Status -eq 'FAIL' }).Count
    $skipCount = @($results | Where-Object { $_.Status -eq 'SKIP' }).Count
    Write-Host "[Phase11.9] Failures=$failCount Skipped=$skipCount Results=$resultDir"
    if ($failCount -gt 0) { exit 1 }
    if ($skipCount -gt 0) { exit 2 }
    exit 0
}
