param(
    [string]$ProjectRoot = "."
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path $ProjectRoot).Path
$source = Join-Path $PSScriptRoot "patch"
if (!(Test-Path (Join-Path $root "CMakeLists.txt"))) {
    throw "CMakeLists.txt not found in project root: $root"
}
if (!(Test-Path $source)) {
    throw "Patch source directory not found: $source"
}

$patchFiles = @(Get-ChildItem $source -Recurse -File)
foreach ($file in $patchFiles) {
    $relative = $file.FullName.Substring($source.Length).TrimStart([char[]]@('\', '/'))
    if ($relative -notlike "src\*" -and $relative -notlike "tests\*") {
        throw "Unexpected patch target: $relative"
    }
}

$stamp = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$backupRoot = Join-Path $root ".phase11_6_backups\$stamp"
New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
$newFiles = New-Object System.Collections.Generic.List[string]

foreach ($file in $patchFiles) {
    $relative = $file.FullName.Substring($source.Length).TrimStart([char[]]@('\', '/'))
    $destination = Join-Path $root $relative
    $backup = Join-Path $backupRoot $relative

    if (Test-Path $destination) {
        New-Item -ItemType Directory -Path (Split-Path $backup) -Force | Out-Null
        Copy-Item -LiteralPath $destination -Destination $backup -Force
    } else {
        $newFiles.Add($relative)
    }
}

$newFiles | Set-Content -Path (Join-Path $backupRoot "new_files.txt") -Encoding UTF8

foreach ($file in $patchFiles) {
    $relative = $file.FullName.Substring($source.Length).TrimStart([char[]]@('\', '/'))
    $destination = Join-Path $root $relative
    New-Item -ItemType Directory -Path (Split-Path $destination) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
    Write-Host "[PATCH] $relative"
}

Write-Host "[DONE] Backup: $backupRoot"
Write-Host "[INFO] Restore: & .\phase11_6_patch\restore.ps1 -ProjectRoot . -BackupRoot '$backupRoot'"
