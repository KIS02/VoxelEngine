param(
    [Parameter(Mandatory = $true)][string]$BackupDirectory,
    [string]$ProjectRoot = "."
)

$ErrorActionPreference = "Stop"
$project = (Resolve-Path -LiteralPath $ProjectRoot).Path
$backup = (Resolve-Path -LiteralPath $BackupDirectory).Path
$manifest = Get-Content -LiteralPath (Join-Path $backup "manifest.json") -Raw | ConvertFrom-Json

foreach ($item in $manifest) {
    $destination = Join-Path $project $item.Path
    if ($item.Existed) {
        Copy-Item -LiteralPath (Join-Path $backup $item.Path) -Destination $destination -Force
        Write-Host "[RESTORED] $($item.Path)"
    } else {
        Remove-Item -LiteralPath $destination -Force -ErrorAction SilentlyContinue
        Write-Host "[REMOVED NEW FILE] $($item.Path)"
    }
}
