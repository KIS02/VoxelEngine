param(
    [string]$ProjectRoot = "",
    [ValidateSet('BlinnPhong', 'PBR')]
    [string]$Shading = 'BlinnPhong',
    [string]$BuildDirectory = 'build\perf'
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}
$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
Set-Location -LiteralPath $ProjectRoot

$build = Join-Path $ProjectRoot $BuildDirectory
$exe = Join-Path $build 'Release\VoxelEngine.exe'
if (!(Test-Path -LiteralPath $exe)) {
    $exe = Join-Path $build 'VoxelEngine.exe'
}
if (!(Test-Path -LiteralPath $exe)) {
    throw "Release executable not found: $exe"
}

$resultDir = Join-Path $ProjectRoot 'phase12_results'
New-Item -ItemType Directory -Path $resultDir -Force | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$log = Join-Path $resultDir "software_${Shading}_$stamp.log"

$names = @('VOXELENGINE_PROFILE_RENDER', 'VOXELENGINE_DEMO_PBR',
    'VOXELENGINE_DEMO_HDR', 'VOXELENGINE_DEMO_SHADOW',
    'VOXELENGINE_DEMO_MULTI_LIGHT', 'VOXELENGINE_SHADOW_BIAS_PROBE')
$previous = @{}
foreach ($name in $names) {
    $previous[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}

try {
    [Environment]::SetEnvironmentVariable('VOXELENGINE_PROFILE_RENDER', '1', 'Process')
    [Environment]::SetEnvironmentVariable('VOXELENGINE_DEMO_PBR',
        $(if ($Shading -eq 'PBR') { '1' } else { '0' }), 'Process')
    [Environment]::SetEnvironmentVariable('VOXELENGINE_DEMO_HDR', '1', 'Process')
    [Environment]::SetEnvironmentVariable('VOXELENGINE_DEMO_SHADOW', '1', 'Process')
    [Environment]::SetEnvironmentVariable('VOXELENGINE_DEMO_MULTI_LIGHT', '1', 'Process')
    [Environment]::SetEnvironmentVariable('VOXELENGINE_SHADOW_BIAS_PROBE', $null, 'Process')

    Write-Host "[Phase12] Rendering Software $Shading; close the game window when sampling is complete"
    Write-Host "[Phase12] Log: $log"
    & $exe --software --play --seed 1337 2>&1 | Tee-Object -FilePath $log | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "Software renderer exited with code $LASTEXITCODE" }

    $samples = @(Select-String -LiteralPath $log -Pattern '\[Profile\]\[Software\]')
    Write-Host "[Phase12] Recorded $($samples.Count) Software profile log lines"
    if ($samples.Count -eq 0) {
        Write-Warning 'No Software profiler lines found; verify the actual source has VOXELENGINE_PROFILE_RENDER support.'
    }
} finally {
    foreach ($name in $names) {
        [Environment]::SetEnvironmentVariable($name, $previous[$name], 'Process')
    }
}
