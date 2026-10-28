param(
    [string]$ProjectRoot = "."
)

$ErrorActionPreference = "Stop"
$project = (Resolve-Path -LiteralPath $ProjectRoot).Path
$patch = Join-Path $PSScriptRoot "patch"

if (!(Test-Path -LiteralPath (Join-Path $project "CMakeLists.txt"))) {
    throw "CMakeLists.txt not found. Set -ProjectRoot to the VoxelEngine project root."
}

$files = @(
    "src/render/shadow/ShadowFrameFingerprint.h",
    "src/render/shadow/ShadowMapping.cpp",
    "src/render/backends/software/pipeline/geometry/SoftwareGeometryPass.cpp",
    "src/render/backends/opengl/pipeline/shadow/OpenGLShadowPass.cpp",
    "src/render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
)

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$backup = Join-Path $project "backups/phase11_6_shadow_optimization_$stamp"
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$manifest = @()

foreach ($relative in $files) {
    $source = Join-Path $patch $relative
    $destination = Join-Path $project $relative
    $backupFile = Join-Path $backup $relative

    if (!(Test-Path -LiteralPath $source)) {
        throw "Missing patch file: $source"
    }

    $existed = Test-Path -LiteralPath $destination
    $manifest += [pscustomobject]@{ Path = $relative; Existed = $existed }
    if ($existed) {
        New-Item -ItemType Directory -Path (Split-Path $backupFile -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $destination -Destination $backupFile -Force
    }
}

$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup "manifest.json") -Encoding UTF8

foreach ($relative in $files) {
    $source = Join-Path $patch $relative
    $destination = Join-Path $project $relative
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
    Write-Host "[APPLIED] $relative"
}

Write-Host "[BACKUP] $backup"
Write-Host "[NEXT] Build the VoxelEngine target and run the three benchmark scenarios."
