#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "math/Vec3.h"

#include "world/BlockType.h"
#include "world/VoxelRaycaster.h"
#include "world/World.h"



namespace
{
    constexpr float epsilon = 0.0001f;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool nearlyEqual( float a, float b ) {
        return std::fabs(a - b) < epsilon;
    }



    void requirePosition( const ve::WorldBlockPosition& position, int x, int y, int z ) {
        require(
            position.x == x && position.y == y && position.z == z,
            "Unexpected World Block position"
        );
    }



    void requireNormal( const ve::WorldBlockPosition& normal, int x, int y, int z ) {
        require(
            normal.x == x && normal.y == y && normal.z == z,
            "Unexpected Hit Face Normal"
        );
    }



    void testPositiveChunkBoundary() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ 16, 1, 1 },
            ve::BlockType::Stone
        );

        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 15.5f, 1.5f, 1.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            3.0f
        );

        require(hit.has_value(), "Positive Chunk boundary hit missing");

        requirePosition(hit->block, 16, 1, 1);
        requireNormal(hit->faceNormal, -1, 0, 0);
        require(nearlyEqual(hit->distance, 0.5f), "Incorrect positive boundary distance");

        std::cout << "[PASS] Positive Chunk boundary\n";
    }



    void testNegativeChunkBoundary() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ -17, 1, 1 },
            ve::BlockType::Dirt
        );

        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ -15.5f, 1.5f, 1.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            3.0f
        );

        require(hit.has_value(), "Negative Chunk boundary hit missing");

        requirePosition(hit->block, -17, 1, 1);
        requireNormal(hit->faceNormal, 1, 0, 0);
        require(nearlyEqual(hit->distance, 0.5f), "Incorrect negative boundary distance");

        std::cout << "[PASS] Negative Chunk boundary\n";
    }



    void checkFace( const ve::WorldBlockPosition& block, const ve::Vec3& direction,
                    const ve::WorldBlockPosition& expectedNormal ) {
        ve::World world{};

        world.setBlock(block, ve::BlockType::Stone);

        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            direction,
            2.0f
        );

        require(hit.has_value(), "Expected face hit missing");

        requirePosition(hit->block, block.x, block.y, block.z);

        requireNormal(
            hit->faceNormal,
            expectedNormal.x,
            expectedNormal.y,
            expectedNormal.z
        );

        require(nearlyEqual(hit->distance, 0.5f), "Incorrect face hit distance");
    }



    void testSixFaceNormals() {
        checkFace(
            ve::WorldBlockPosition{ 1, 0, 0 },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            ve::WorldBlockPosition{ -1, 0, 0 }
        );

        checkFace(
            ve::WorldBlockPosition{ -1, 0, 0 },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            ve::WorldBlockPosition{ 1, 0, 0 }
        );

        checkFace(
            ve::WorldBlockPosition{ 0, 1, 0 },
            ve::Vec3{ 0.0f, 1.0f, 0.0f },
            ve::WorldBlockPosition{ 0, -1, 0 }
        );

        checkFace(
            ve::WorldBlockPosition{ 0, -1, 0 },
            ve::Vec3{ 0.0f, -1.0f, 0.0f },
            ve::WorldBlockPosition{ 0, 1, 0 }
        );

        checkFace(
            ve::WorldBlockPosition{ 0, 0, 1 },
            ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::WorldBlockPosition{ 0, 0, -1 }
        );

        checkFace(
            ve::WorldBlockPosition{ 0, 0, -1 },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            ve::WorldBlockPosition{ 0, 0, 1 }
        );

        std::cout << "[PASS] All six Hit Face Normals\n";
    }



    void testExactNegativeBoundary() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ -1, 0, 0 },
            ve::BlockType::Stone
        );

        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.0f, 0.5f, 0.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            0.0f
        );

        require(hit.has_value(), "Exact negative boundary hit missing");

        requirePosition(hit->block, -1, 0, 0);
        requireNormal(hit->faceNormal, 0, 0, 0);
        require(nearlyEqual(hit->distance, 0.0f), "Boundary distance must be zero");

        std::cout << "[PASS] Exact negative-direction starting boundary\n";
    }



    void testExactMaximumReach() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ 3, 0, 0 },
            ve::BlockType::Stone
        );

        const ve::Vec3 origin{ 0.5f, 0.5f, 0.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        const auto tooShort = ve::VoxelRaycaster::cast(
            world, origin, direction, 2.49f
        );

        const auto exact = ve::VoxelRaycaster::cast(
            world, origin, direction, 2.5f
        );

        require(!tooShort.has_value(), "Ray must not exceed maximum reach");
        require(exact.has_value(), "Ray must hit at exact maximum reach");

        requirePosition(exact->block, 3, 0, 0);
        require(nearlyEqual(exact->distance, 2.5f), "Exact reach distance mismatch");

        std::cout << "[PASS] Exact maximum Ray reach\n";
    }



    void testZeroReach() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ 2, 3, 4 },
            ve::BlockType::Stone
        );

        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 2.5f, 3.5f, 4.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            0.0f
        );

        require(hit.has_value(), "Ray inside a solid block must hit at zero reach");

        requirePosition(hit->block, 2, 3, 4);
        requireNormal(hit->faceNormal, 0, 0, 0);
        require(nearlyEqual(hit->distance, 0.0f), "Inside-block distance must be zero");

        const auto miss = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 1.5f, 3.5f, 4.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            0.0f
        );

        require(!miss.has_value(), "Zero reach must not enter another block");

        std::cout << "[PASS] Zero-distance Raycasting\n";
    }



    void testWorldModification() {
        ve::World world{};

        const ve::WorldBlockPosition first{ 2, 1, 1 };
        const ve::WorldBlockPosition second{ 3, 1, 1 };

        world.setBlock(first, ve::BlockType::Dirt);
        world.setBlock(second, ve::BlockType::Stone);

        const ve::Vec3 origin{ 0.5f, 1.5f, 1.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        const auto firstHit = ve::VoxelRaycaster::cast(
            world, origin, direction, 6.0f
        );

        require(firstHit.has_value(), "First block must be selected");
        requirePosition(firstHit->block, 2, 1, 1);

        world.setBlock(first, ve::BlockType::Air);

        const auto secondHit = ve::VoxelRaycaster::cast(
            world, origin, direction, 6.0f
        );

        require(secondHit.has_value(), "Second block must be selected after removal");
        requirePosition(secondHit->block, 3, 1, 1);

        require(secondHit->type == ve::BlockType::Stone, "Second block type mismatch");

        std::cout << "[PASS] Raycast updates after World modification\n";
    }



    void testDiagonalBoundaryTie() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ 1, 1, 0 },
            ve::BlockType::Stone
        );

        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 1.0f, 0.0f },
            3.0f
        );

        require(hit.has_value(), "Diagonal tie must reach target block");

        requirePosition(hit->block, 1, 1, 0);

        // Current DDA Tie-Breaking: X Before Y

        requireNormal(hit->faceNormal, 0, -1, 0);

        const float expectedDistance = 0.5f * std::sqrt(2.0f);

        require(nearlyEqual(hit->distance, expectedDistance), "Diagonal distance mismatch");

        std::cout << "[PASS] Deterministic diagonal boundary tie\n";
    }



    void testOppositeDirectionMiss() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ -1, 0, 0 },
            ve::BlockType::Stone
        );

        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f
        );

        require(!hit.has_value(), "Ray must not select a block behind its origin");

        std::cout << "[PASS] Ray ignores blocks in opposite direction\n";
    }
}



int main() {
    try {
        testPositiveChunkBoundary();
        testNegativeChunkBoundary();
        testSixFaceNormals();
        testExactNegativeBoundary();
        testExactMaximumReach();
        testZeroReach();
        testWorldModification();
        testDiagonalBoundaryTie();
        testOppositeDirectionMiss();

        std::cout << "\nAll Voxel Raycaster Boundary tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}