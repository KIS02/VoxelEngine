#include "world/ChunkCoordinates.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

#include "world/Chunk.h"



namespace ve
{
    namespace
    {
        int floorDivide( int value ) {
            const int quotient = value / Chunk::Size;
            const int remainder = value % Chunk::Size;

            return remainder < 0 ? quotient - 1 : quotient;
        }



        int positiveModulo( int value ) {
            const int remainder = value % Chunk::Size;

            return remainder < 0 ? remainder + Chunk::Size : remainder;
        }



        int checkedWorldComponent( int chunkCoordinate, int localCoordinate ) {
            const std::int64_t value =
                static_cast<std::int64_t>(chunkCoordinate) * Chunk::Size +
                static_cast<std::int64_t>(localCoordinate);


            if (value < std::numeric_limits<int>::min() ||
                value > std::numeric_limits<int>::max()) {
                throw std::out_of_range("World block coordinate exceeds integer range");
            }


            return static_cast<int>(value);
        }
    }



    ChunkBlockLocation worldToChunkLocation( const WorldBlockPosition& world ) {
        ChunkBlockLocation location{};


        location.chunk = ChunkPosition{
            floorDivide(world.x),
            floorDivide(world.y),
            floorDivide(world.z)
        };


        location.local = LocalBlockPosition{
            positiveModulo(world.x),
            positiveModulo(world.y),
            positiveModulo(world.z)
        };


        return location;
    }



    WorldBlockPosition chunkToWorldPosition( const ChunkPosition& chunk, const LocalBlockPosition& local ) {
        if (!Chunk::isInside(local.x, local.y, local.z)) {
            throw std::out_of_range("Local block coordinates are out of range");
        }


        return WorldBlockPosition{
            checkedWorldComponent(chunk.x, local.x),
            checkedWorldComponent(chunk.y, local.y),
            checkedWorldComponent(chunk.z, local.z)
        };
    }
}