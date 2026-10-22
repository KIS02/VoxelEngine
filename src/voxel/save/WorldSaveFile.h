#pragma once

#include <filesystem>
#include <map>

#include "voxel/chunk/VoxelStorage.h"
#include "voxel/generation/TerrainGenerationConfig.h"
#include "voxel/streaming/ChunkDelta.h"


namespace ve
{
    using WorldDeltaMap = std::map<ChunkPosition, ChunkDelta, ChunkPositionLess>;


    struct WorldSaveData
    {
        TerrainGenerationConfig terrain{};
        WorldDeltaMap deltas{};
    };


    void writeWorldSaveFile( const std::filesystem::path& path, const WorldSaveData& data );

    WorldSaveData readWorldSaveFile( const std::filesystem::path& path );
}