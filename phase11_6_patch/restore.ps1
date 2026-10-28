param(
    [string]$ProjectRoot = ".",
    [Parameter(Mandatory = $true)]
    [string]$BackupRoot
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path $ProjectRoot).Path
$backup = (Resolve-Path $BackupRoot).Path
if (!(Test-Path (Join-Path $backup "new_files.txt"))) {
    throw "Not a valid patch backup: $backup"
}

Get-Content (Join-Path $backup "new_files.txt") | ForEach-Object {
    if ([string]::IsNullOrWhiteSpace($_)) { return }
    $path = Join-Path $root $_
    if (Test-Path $path) {
        Remove-Item -LiteralPath $path -Force
        Write-Host "[REMOVE] $_"
    }
}

Get-ChildItem $backup -Recurse -File | Where-Object { $_.Name -ne "new_files.txt" } | ForEach-Object {
    $relative = $_.FullName.Substring($backup.Length).TrimStart([char[]]@('\', '/'))
    $path = Join-Path $root $relative
    New-Item -ItemType Directory -Path (Split-Path $path) -Force | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $path -Force
    Write-Host "[RESTORE] $relative"
}
Write-Host "[DONE] Backup restored."
