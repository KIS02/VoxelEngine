param(
    [string]$ProjectRoot = '.',
    [switch]$Apply,
    [switch]$AllowModifiedLegacyVerifier
)

$ErrorActionPreference = 'Stop'
$project = [System.IO.Path]::GetFullPath($ProjectRoot).TrimEnd([char[]]@('/', '\'))
$payloadRoot = Join-Path $PSScriptRoot 'payload'
$manifestPath = Join-Path $PSScriptRoot 'manifest.json'

if (-not (Test-Path -LiteralPath (Join-Path $project 'CMakeLists.txt') -PathType Leaf)) {
    throw "Invalid VoxelEngine project root: $project"
}
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Missing manifest: $manifestPath"
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.schema -ne 1) {
    throw 'Unsupported v5 manifest schema'
}

function RelativePath( [string]$Root, [string]$Value ) {
    if ([System.IO.Path]::IsPathRooted($Value) -or $Value -match '(^|[\\/])\.\.([\\/]|$)') {
        throw "Unsafe manifest path: $Value"
    }
    $full = [System.IO.Path]::GetFullPath((Join-Path $Root ($Value.Replace('/', [System.IO.Path]::DirectorySeparatorChar))))
    $prefix = $Root.TrimEnd([char[]]@('/', '\')) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Manifest path escapes root: $Value"
    }
    return $full
}

function HashFile( [string]$Path ) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

# SHA-256 of the same UTF-8 bytes after replacing only CRLF (0D 0A) with LF (0A).
# Never normalize BOM, whitespace, other carriage returns, or code content.
# Used strictly for source preflight; the original local bytes are NEVER rewritten.
function HashWithLfLineEndings( [string]$Path ) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $utf8 = [System.Text.UTF8Encoding]::new($false, $true)
    $text = $utf8.GetString($bytes)
    $lfBytes = $utf8.GetBytes($text.Replace("`r`n", "`n"))
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        return [System.BitConverter]::ToString($sha256.ComputeHash($lfBytes)).Replace('-', '')
    } finally {
        $sha256.Dispose()
    }
}

# Complete validation precedes ALL source changes.
$actions = @()
foreach ($entry in $manifest.operations) {
    $target = RelativePath $project $entry.path
    $payload = RelativePath $payloadRoot $entry.path

    if ($entry.operation -eq 'add') {
        if (Test-Path -LiteralPath $target) {
            throw "New v5 file already exists: $($entry.path)"
        }
    } elseif ($entry.operation -eq 'modify' -or $entry.operation -eq 'delete') {
        if (-not (Test-Path -LiteralPath $target -PathType Leaf)) {
            throw "R6 original file missing: $($entry.path)"
        }
        $observedHash = HashFile $target
        if ($observedHash -ne $entry.before_sha256) {
            $normalizedHash = $null
            try {
                $normalizedHash = HashWithLfLineEndings $target
            } catch {
                throw "R6 original hash mismatch (not valid UTF-8 text): $($entry.path)"
            }
            if ($normalizedHash -ne $entry.before_sha256) {
                throw "R6 original hash mismatch (not a CRLF-only difference): $($entry.path). Source content changed; do not override."
            }
            # Source content is identical; only CRLF versus LF differs.
            # Use the ACTUAL local SHA-256 in the transaction, backup and rollback.
            $entry.before_sha256 = $observedHash
            Write-Host "[V5][CRLF-ONLY] $($entry.path) - original local bytes will be backed up unchanged"
        }
    } else {
        throw "Unknown v5 operation: $($entry.operation)"
    }

    if ($entry.operation -ne 'delete') {
        if (-not (Test-Path -LiteralPath $payload -PathType Leaf)) {
            throw "Missing v5 payload: $($entry.path)"
        }
        if ((HashFile $payload) -ne $entry.after_sha256) {
            throw "v5 payload hash mismatch: $($entry.path)"
        }
    }
    $actions += [pscustomobject]@{ Entry = $entry; Target = $target; Payload = $payload }
    Write-Host "[V5][READY] $($entry.operation) $($entry.path)"
}

Write-Host "[V5] Validated $($actions.Count) file operations."
if (-not $Apply) {
    Write-Host '[V5] Dry Run successful. No project files changed.'
    exit 0
}

$timestamp = (Get-Date).ToUniversalTime().ToString('yyyyMMdd_HHmmss_fff')
$backupRoot = Join-Path $project ('.v5_backups\' + $timestamp)
New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null

# Back up every source file BEFORE applying the first change.
foreach ($action in $actions) {
    if ($action.Entry.operation -eq 'add') { continue }
    $backup = RelativePath $backupRoot $action.Entry.path
    New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
    Copy-Item -LiteralPath $action.Target -Destination $backup -ErrorAction Stop
    if ((HashFile $backup) -ne $action.Entry.before_sha256) {
        throw "Failed to verify backup of $($action.Entry.path)"
    }
}
$manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $backupRoot 'transaction.json') -Encoding UTF8

try {
    foreach ($action in $actions) {
        if ($action.Entry.operation -eq 'delete') {
            Remove-Item -LiteralPath $action.Target -Force -ErrorAction Stop
        } else {
            New-Item -ItemType Directory -Path (Split-Path -Parent $action.Target) -Force | Out-Null
            Copy-Item -LiteralPath $action.Payload -Destination $action.Target -Force -ErrorAction Stop
            if ((HashFile $action.Target) -ne $action.Entry.after_sha256) {
                throw "Installed file hash mismatch: $($action.Entry.path)"
            }
        }
        Write-Host "[V5][APPLIED] $($action.Entry.path)"
    }
} catch {
    $reason = $_
    Write-Warning 'v5 installation failed. Restoring R6 files from the backup...'
    foreach ($action in $actions) {
        if ($action.Entry.operation -eq 'add') {
            if (Test-Path -LiteralPath $action.Target -PathType Leaf) {
                Remove-Item -LiteralPath $action.Target -Force
            }
        } else {
            $backup = RelativePath $backupRoot $action.Entry.path
            New-Item -ItemType Directory -Path (Split-Path -Parent $action.Target) -Force | Out-Null
            Copy-Item -LiteralPath $backup -Destination $action.Target -Force
        }
    }
    throw "v5 application failed; attempted automatic rollback: $reason"
}

Write-Host '[V5][PASS] Integrated Architecture v5 was applied.'
Write-Host "[V5] Backup directory: $backupRoot"
Write-Host '[V5] Next run scripts\verify_v5.ps1 -CleanBuild'
