param(
    [string]$ProjectRoot = '.',
    [string]$OutputFile = 'VoxelEngine_R6_Final_Source.zip'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path
$projectFile = Join-Path $root 'CMakeLists.txt'
if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
    throw "Not a VoxelEngine project root: $root"
}

$paths = @('src', 'tests', 'scripts', 'CMakeLists.txt', 'cmake', 'VoxelEngine_R4C_ChunkMeshTests', 'VoxelEngine_R3C4_OpenGL_Lifecycle')
$present = @($paths | Where-Object { Test-Path -LiteralPath (Join-Path $root $_) })
$target = [System.IO.Path]::GetFullPath((Join-Path $root $OutputFile))
if ($target -eq $projectFile) {
    throw 'Invalid archive output path'
}

Push-Location $root
try {
    $sourceFiles = @($present | ForEach-Object { Get-Item -LiteralPath $_ })
    Compress-Archive -Path $sourceFiles.FullName -DestinationPath $target -CompressionLevel Optimal -Force
    if (-not (Test-Path -LiteralPath $target -PathType Leaf)) {
        throw 'Source archive was not created'
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($target)
    try {
        $entries = @($zip.Entries | Where-Object { $_.Name -ne '' })
        if ($entries.Count -lt 30) {
            throw "Unexpected source archive size: $($entries.Count) files"
        }
        $hasApp = @($entries | Where-Object { $_.FullName -replace '\\','/' -match '(^|/)src/core/Application\.cpp$' }).Count -gt 0
        if (-not $hasApp) {
            # Compress-Archive flattens the selected src folder into a src-prefixed entry on supported PowerShell versions.
            # A missing Application.cpp indicates an incomplete or unexpected package layout.
            throw 'Application.cpp was not found in exported ZIP'
        }
        Write-Host "[SOURCE][OK] Files: $($entries.Count)"
    } finally {
        $zip.Dispose()
    }
} finally {
    Pop-Location
}

$checksum = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
Write-Host "[SOURCE][OK] Archive: $target"
Write-Host "[SOURCE][OK] SHA256: $checksum"
Write-Host '[SOURCE] This archive contains sources only. It excludes build, tools, external and benchmark CSV.'
