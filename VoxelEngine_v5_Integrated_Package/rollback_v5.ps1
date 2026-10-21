param(
    [Parameter(Mandatory = $true)][string]$BackupDirectory,
    [string]$ProjectRoot = '.',
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$project = [System.IO.Path]::GetFullPath($ProjectRoot).TrimEnd([char[]]@('/', '\'))
$backupRoot = [System.IO.Path]::GetFullPath($BackupDirectory).TrimEnd([char[]]@('/', '\'))
$transaction = Join-Path $backupRoot 'transaction.json'
if (-not (Test-Path -LiteralPath $transaction -PathType Leaf)) {
    throw "Missing v5 backup transaction: $transaction"
}
$manifest = Get-Content -LiteralPath $transaction -Raw | ConvertFrom-Json
if ($manifest.schema -ne 1) { throw 'Unsupported backup schema' }

function RelativePath( [string]$Root, [string]$Value ) {
    if ([System.IO.Path]::IsPathRooted($Value) -or $Value -match '(^|[\\/])\.\.([\\/]|$)') {
        throw "Unsafe backup path: $Value"
    }
    $full = [System.IO.Path]::GetFullPath((Join-Path $Root ($Value.Replace('/', [System.IO.Path]::DirectorySeparatorChar))))
    $prefix = $Root.TrimEnd([char[]]@('/', '\')) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Backup path escapes root: $Value"
    }
    return $full
}

function HashFile( [string]$Path ) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

# Check that backups are complete and post-v5 files have not been edited.
foreach ($entry in $manifest.operations) {
    $target = RelativePath $project $entry.path
    if ($entry.operation -ne 'add') {
        $backup = RelativePath $backupRoot $entry.path
        if (-not (Test-Path -LiteralPath $backup -PathType Leaf) -or
            (HashFile $backup) -ne $entry.before_sha256) {
            throw "Backup file is missing or corrupt: $($entry.path)"
        }
    }
    if (-not $Force) {
        if ($entry.operation -eq 'delete') {
            if (Test-Path -LiteralPath $target) { throw "Target unexpectedly recreated: $($entry.path)" }
        } elseif (-not (Test-Path -LiteralPath $target -PathType Leaf) -or
                  (HashFile $target) -ne $entry.after_sha256) {
            throw "Target changed since v5 install: $($entry.path); inspect or use -Force intentionally"
        }
    }
}

foreach ($entry in $manifest.operations) {
    $target = RelativePath $project $entry.path
    if ($entry.operation -eq 'add') {
        if (Test-Path -LiteralPath $target -PathType Leaf) {
            Remove-Item -LiteralPath $target -Force
        }
    } else {
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath (RelativePath $backupRoot $entry.path) -Destination $target -Force
        if ((HashFile $target) -ne $entry.before_sha256) {
            throw "Restore integrity failed: $($entry.path)"
        }
    }
    Write-Host "[V5][RESTORED] $($entry.path)"
}
Write-Host '[V5][PASS] R6 sources restored; backup directory was retained.'
