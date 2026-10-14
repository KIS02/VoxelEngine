
#include <cstddef>
#include <exception>
#include <iostream>
#include <set>
#include <stdexcept>

#include "math/Vec3.h"

#include "render/MeshData.h"
#include "render/mesh/ChunkMeshFactory.h"

#include "world/BlockInteractor.h"
#include "world/BlockType.h"
#include "world/Chunk.h"
#include "world/World.h"



namespace
{
    using DirtyChunks = std::set<ve::ChunkPosition, ve::ChunkPositionLess>;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool samePosition( const ve::WorldBlockPosition& a, const ve::WorldBlockPosition& b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }



    void setAxis( ve::WorldBlockPosition& position, int axis, int value ) {
        if (axis == 0) position.x = value;
        else if (axis == 1) position.y = value;
        else position.z = value;
    }



    void setAxis( ve::ChunkPosition& position, int axis, int value ) {
        if (axis == 0) position.x = value;
        else if (axis == 1) position.y = value;
        else position.z = value;
    }



    void setAxis( ve::Vec3& vector, int axis, float value ) {
        if (axis == 0) vector.x = value;
        else if (axis == 1) vector.y = value;
        else vector.z = value;
    }



    const char* axisName( int axis ) {
        if (axis == 0) return "X";
        if (axis == 1) return "Y";
        return "Z";
    }



    DirtyChunks consumeDirtyChunks( ve::World& world ) {
        DirtyChunks dirty;

        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            dirty.insert(position);
        });

        return dirty;
    }



    void requireDirty( const DirtyChunks& dirty, const ve::ChunkPosition& position ) {
        require(dirty.find(position) != dirty.end(), "Required Chunk was not marked Dirty");
    }



    void requireNotDirty( const DirtyChunks& dirty, const ve::ChunkPosition& position ) {
        require(dirty.find(position) == dirty.end(), "Unrelated Chunk became Dirty");
    }



    void requireMeshIndices( const ve::World& world, const ve::ChunkPosition& position,
                             std::size_t expected, const char* message ) {
        const ve::MeshData mesh = ve::ChunkMeshFactory::create(world, position);

        require(mesh.indexCount() == expected, message);
    }



    void testPositiveBoundaryBreakAndRestore( int axis ) {
        constexpr int size = ve::Chunk::Size;
        constexpr int last = size - 1;

        ve::World world{};



        ve::WorldBlockPosition first{ 1, 1, 1 };
        ve::WorldBlockPosition second{ 1, 1, 1 };

        setAxis(first, axis, last);
        setAxis(second, axis, size);



        ve::ChunkPosition firstChunk{ 0, 0, 0 };
        ve::ChunkPosition secondChunk{ 0, 0, 0 };

        setAxis(secondChunk, axis, 1);



        ve::Vec3 origin{ 1.5f, 1.5f, 1.5f };
        ve::Vec3 direction{ 0.0f, 0.0f, 0.0f };

        setAxis(origin, axis, static_cast<float>(last) - 0.5f);
        setAxis(direction, axis, 1.0f);



        // Two Adjacent Blocks Have Five Visible Faces Each

        world.setBlock(first, ve::BlockType::Stone);
        world.setBlock(second, ve::BlockType::Dirt);

        consumeDirtyChunks(world);

        requireMeshIndices(world, firstChunk, 30, "First Chunk must have five faces");
        requireMeshIndices(world, secondChunk, 30, "Second Chunk must have five faces");



        // Break the Block at the Chunk Boundary

        const auto broken = ve::BlockInteractor::breakBlock(
            world, origin, direction, 6.0f
        );

        require(broken.has_value(), "Boundary block breaking failed");
        require(samePosition(broken->block, first), "Wrong boundary block was broken");

        require(world.blockAt(first) == ve::BlockType::Air, "First block was not removed");
        require(world.blockAt(second) == ve::BlockType::Dirt, "Second block was changed");



        const DirtyChunks dirtyAfterBreak = consumeDirtyChunks(world);

        requireDirty(dirtyAfterBreak, firstChunk);
        requireDirty(dirtyAfterBreak, secondChunk);



        // Removing the First Block Exposes the Neighbor's Sixth Face

        requireMeshIndices(world, firstChunk, 0, "Removed block left geometry behind");
        requireMeshIndices(world, secondChunk, 36, "Neighbor face was not exposed");



        // Restore the Block by Clicking the Neighbor's Visible Face

        const auto placed = ve::BlockInteractor::placeBlock(
            world, origin, direction, 6.0f, ve::BlockType::Stone
        );

        require(placed.has_value(), "Boundary block restoration failed");
        require(samePosition(placed->placedBlock, first), "Restoration used wrong position");

        require(world.blockAt(first) == ve::BlockType::Stone, "Block was not restored");
        require(world.blockAt(second) == ve::BlockType::Dirt, "Anchor block was modified");



        const DirtyChunks dirtyAfterPlace = consumeDirtyChunks(world);

        requireDirty(dirtyAfterPlace, firstChunk);
        requireDirty(dirtyAfterPlace, secondChunk);



        // Shared Faces Must Be Hidden Again

        requireMeshIndices(world, firstChunk, 30, "Restored block has incorrect faces");
        requireMeshIndices(world, secondChunk, 30, "Shared neighbor face was not hidden");

        std::cout << "[PASS] Positive " << axisName(axis)
                  << " boundary break and restoration\n";
    }



    void testNegativeBoundaryChunkCreation( int axis ) {
        constexpr int size = ve::Chunk::Size;

        ve::World world{};



        ve::WorldBlockPosition anchor{ 1, 1, 1 };
        ve::WorldBlockPosition target{ 1, 1, 1 };

        setAxis(anchor, axis, -size);
        setAxis(target, axis, -size - 1);



        ve::ChunkPosition anchorChunk{ 0, 0, 0 };
        ve::ChunkPosition targetChunk{ 0, 0, 0 };

        setAxis(anchorChunk, axis, -1);
        setAxis(targetChunk, axis, -2);



        ve::Vec3 origin{ 1.5f, 1.5f, 1.5f };
        ve::Vec3 direction{ 0.0f, 0.0f, 0.0f };

        setAxis(origin, axis, static_cast<float>(-size) - 2.5f);
        setAxis(direction, axis, 1.0f);



        // Create Only the Anchor Block

        world.setBlock(anchor, ve::BlockType::Stone);

        consumeDirtyChunks(world);

        requireMeshIndices(world, anchorChunk, 36, "Anchor must initially have six faces");



        // Place a Block Into the Previously Missing Neighbor Chunk

        const auto placed = ve::BlockInteractor::placeBlock(
            world, origin, direction, 6.0f, ve::BlockType::Dirt
        );

        require(placed.has_value(), "Negative boundary placement failed");
        require(samePosition(placed->placedBlock, target), "Wrong negative target position");

        require(world.findChunk(targetChunk) != nullptr, "New negative Chunk was not created");
        require(world.blockAt(target) == ve::BlockType::Dirt, "New block was not stored");



        const DirtyChunks dirtyAfterPlace = consumeDirtyChunks(world);

        requireDirty(dirtyAfterPlace, anchorChunk);
        requireDirty(dirtyAfterPlace, targetChunk);



        // Both Blocks Must Hide the Shared Face

        requireMeshIndices(world, anchorChunk, 30, "Anchor shared face was not hidden");
        requireMeshIndices(world, targetChunk, 30, "New Chunk has incorrect mesh");



        // Break the Newly Placed Block

        const auto broken = ve::BlockInteractor::breakBlock(
            world, origin, direction, 6.0f
        );

        require(broken.has_value(), "Negative boundary breaking failed");
        require(samePosition(broken->block, target), "Wrong negative block was removed");



        const DirtyChunks dirtyAfterBreak = consumeDirtyChunks(world);

        requireDirty(dirtyAfterBreak, anchorChunk);
        requireDirty(dirtyAfterBreak, targetChunk);



        requireMeshIndices(world, anchorChunk, 36, "Anchor face was not restored");
        requireMeshIndices(world, targetChunk, 0, "Removed block still has geometry");

        std::cout << "[PASS] Negative " << axisName(axis)
                  << " boundary creation and removal\n";
    }



    void testInteriorEditDoesNotDirtyDistantChunk() {
        ve::World world{};

        const ve::WorldBlockPosition target{ 2, 2, 2 };
        const ve::WorldBlockPosition distantBlock{ ve::Chunk::Size + 2, 2, 2 };

        const ve::ChunkPosition targetChunk{ 0, 0, 0 };
        const ve::ChunkPosition distantChunk{ 1, 0, 0 };



        world.setBlock(target, ve::BlockType::Stone);
        world.setBlock(distantBlock, ve::BlockType::Stone);

        consumeDirtyChunks(world);



        const auto broken = ve::BlockInteractor::breakBlock(
            world,
            ve::Vec3{ 2.5f, 2.5f, 4.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            6.0f
        );

        require(broken.has_value(), "Interior block breaking failed");
        require(samePosition(broken->block, target), "Wrong interior block removed");



        const DirtyChunks dirty = consumeDirtyChunks(world);

        requireDirty(dirty, targetChunk);
        requireNotDirty(dirty, distantChunk);
        require(dirty.size() == 1, "Interior edit dirtied extra Chunks");

        requireMeshIndices(world, targetChunk, 0, "Interior block was not removed");
        requireMeshIndices(world, distantChunk, 36, "Distant Chunk mesh was changed");

        std::cout << "[PASS] Interior edit leaves distant Chunk clean\n";
    }



    void testPositiveChunkCornerDirtyPropagation() {
        constexpr int last = ve::Chunk::Size - 1;

        ve::World world{};



        const ve::ChunkPosition center{ 0, 0, 0 };
        const ve::ChunkPosition positiveX{ 1, 0, 0 };
        const ve::ChunkPosition positiveY{ 0, 1, 0 };
        const ve::ChunkPosition positiveZ{ 0, 0, 1 };
        const ve::ChunkPosition diagonal{ 1, 1, 1 };



        world.getOrCreateChunk(center);
        world.getOrCreateChunk(positiveX);
        world.getOrCreateChunk(positiveY);
        world.getOrCreateChunk(positiveZ);
        world.getOrCreateChunk(diagonal);

        consumeDirtyChunks(world);



        // This Block Touches Three Positive Chunk Boundaries

        world.setBlock(
            ve::WorldBlockPosition{ last, last, last },
            ve::BlockType::Stone
        );



        const DirtyChunks dirty = consumeDirtyChunks(world);

        requireDirty(dirty, center);
        requireDirty(dirty, positiveX);
        requireDirty(dirty, positiveY);
        requireDirty(dirty, positiveZ);

        requireNotDirty(dirty, diagonal);
        require(dirty.size() == 4, "Positive corner dirtied unexpected Chunks");

        std::cout << "[PASS] Positive Chunk corner dirties three face neighbors\n";
    }



    void testNegativeChunkCornerDirtyPropagation() {
        ve::World world{};



        const ve::ChunkPosition center{ 0, 0, 0 };
        const ve::ChunkPosition negativeX{ -1, 0, 0 };
        const ve::ChunkPosition negativeY{ 0, -1, 0 };
        const ve::ChunkPosition negativeZ{ 0, 0, -1 };
        const ve::ChunkPosition diagonal{ -1, -1, -1 };



        world.getOrCreateChunk(center);
        world.getOrCreateChunk(negativeX);
        world.getOrCreateChunk(negativeY);
        world.getOrCreateChunk(negativeZ);
        world.getOrCreateChunk(diagonal);

        consumeDirtyChunks(world);



        // Local Coordinate (0, 0, 0) Touches Three Negative Boundaries

        world.setBlock(
            ve::WorldBlockPosition{ 0, 0, 0 },
            ve::BlockType::Stone
        );



        const DirtyChunks dirty = consumeDirtyChunks(world);

        requireDirty(dirty, center);
        requireDirty(dirty, negativeX);
        requireDirty(dirty, negativeY);
        requireDirty(dirty, negativeZ);

        requireNotDirty(dirty, diagonal);
        require(dirty.size() == 4, "Negative corner dirtied unexpected Chunks");

        std::cout << "[PASS] Negative Chunk corner dirties three face neighbors\n";
    }
}



int main() {
    try {
        testPositiveBoundaryBreakAndRestore(0);
        testPositiveBoundaryBreakAndRestore(1);
        testPositiveBoundaryBreakAndRestore(2);

        testNegativeBoundaryChunkCreation(0);
        testNegativeBoundaryChunkCreation(1);
        testNegativeBoundaryChunkCreation(2);

        testInteriorEditDoesNotDirtyDistantChunk();

        testPositiveChunkCornerDirtyPropagation();
        testNegativeChunkCornerDirtyPropagation();



        std::cout << "\nAll Chunk Boundary Interaction tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
