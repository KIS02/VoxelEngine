$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

Push-Location $root

try {
    $compiler = Join-Path $root "tools/mingw/bin/g++.exe"
    $buildDir = Join-Path $root "build"

    if (-not (Test-Path $compiler)) {
        throw "MinGW compiler not found: $compiler"
    }

    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null


    function Run-Test {
        param(
            [string]$Name,
            [string[]]$Sources
        )

        $testFile = "tests/$Name.cpp"
        $executable = Join-Path $buildDir "$Name.exe"

        if (-not (Test-Path $testFile)) {
            throw "Test source not found: $testFile"
        }

        Write-Host ""
        Write-Host "===================================="
        Write-Host " Building: $Name"
        Write-Host "===================================="

        $arguments = @(
            "-std=c++20"
            "-Wall"
            "-Wextra"
            "-Isrc"
        ) + $Sources + @(
            $testFile
            "-o"
            $executable
        )

        & $compiler @arguments

        if ($LASTEXITCODE -ne 0) {
            throw "Compilation failed: $Name"
        }

        Write-Host ""
        Write-Host " Running: $Name"

        & $executable

        if ($LASTEXITCODE -ne 0) {
            throw "Test failed: $Name"
        }

        Write-Host "[SUITE PASS] $Name"
    }


    $chunkSources = @(
        "src/world/Chunk.cpp"
    )

    $coordinateSources = @(
        "src/world/Chunk.cpp"
        "src/world/ChunkCoordinates.cpp"
    )

    $worldSources = @(
        "src/world/Chunk.cpp"
        "src/world/ChunkCoordinates.cpp"
        "src/world/World.cpp"
        "src/render/MeshData.cpp"
        "src/render/mesh/ChunkMeshFactory.cpp"
    )

    $demoSources = $worldSources + @(
        "src/demo/ChunkDemoFactory.cpp"
    )

    $architectureSources = @(
        "src/render/MeshData.cpp"
        "src/render/mesh/CubeMeshFactory.cpp"
    )

    $textureSources = @(
        "src/render/Texture2D.cpp"
        "src/render/texture/UVTestTexture.cpp"
    )


    Write-Host ""
    Write-Host "VoxelEngine Phase 5 Regression Tests"
    Write-Host "===================================="


    Run-Test -Name "ChunkTests" -Sources $chunkSources

    Run-Test -Name "ChunkCoordinatesTests" -Sources $coordinateSources

    Run-Test -Name "ChunkMeshFactoryTests" -Sources $worldSources

    Run-Test -Name "ChunkDemoTests" -Sources $demoSources

    Run-Test -Name "WorldTests" -Sources $demoSources

    Run-Test -Name "WorldDirtyTests" -Sources $worldSources

    Run-Test -Name "WorldDirtyRecoveryTests" -Sources $worldSources

    Run-Test -Name "WorldRemovalTests" -Sources $worldSources

    Run-Test -Name "MeshIdTests" -Sources @()

    Run-Test -Name "RenderArchitectureTests" -Sources $architectureSources

    Run-Test -Name "TextureConsistencyTests" -Sources $textureSources


    Write-Host ""
    Write-Host "===================================="
    Write-Host " ALL REGRESSION TEST SUITES PASSED"
    Write-Host "===================================="
}
finally {
    Pop-Location
}