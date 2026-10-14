#pragma once



namespace ve
{
    struct WorldBlockPosition
    {
        int x = 0;
        int y = 0;
        int z = 0;
    };



    struct ChunkPosition
    {
        int x = 0;
        int y = 0;
        int z = 0;
    };



    struct LocalBlockPosition
    {
        int x = 0;
        int y = 0;
        int z = 0;
    };



    struct ChunkBlockLocation
    {
        ChunkPosition chunk{};
        LocalBlockPosition local{};
    };



    ChunkBlockLocation worldToChunkLocation( const WorldBlockPosition& world );

    WorldBlockPosition chunkToWorldPosition( const ChunkPosition& chunk, const LocalBlockPosition& local );
}