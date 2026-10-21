# Compatibility entry point. The maintained regression runner discovers all test suites.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$powershell = (Get-Process -Id $PID).Path
& $powershell '-NoProfile' '-ExecutionPolicy' 'Bypass' '-File' (Join-Path $root 'scripts\run_regression_tests.ps1')
if ($LASTEXITCODE -ne 0) {
    throw 'Phase 5 compatibility regression run failed'
}
Write-Host '[PASS] Phase 5 compatibility runner completed the full regression suite'
