#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
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



    void requireBlock( const ve::WorldBlockPosition& actual,
                       int x, int y, int z, const char* message ) {
        require(actual.x == x && actual.y == y && actual.z == z, message);
    }



    void testStraightRayHitsFirstBlock() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ 0, 2, 2 }, ve::BlockType::Stone);
        world.setBlock(ve::WorldBlockPosition{ 0, 2, 0 }, ve::BlockType::Dirt);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 2.5f, 5.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            10.0f
        );



        require(hit.has_value(), "Ray must hit Stone");
        requireBlock(hit->block, 0, 2, 2, "Ray must select the nearest block");
        requireBlock(hit->faceNormal, 0, 0, 1, "Hit face normal is incorrect");

        require(hit->type == ve::BlockType::Stone, "Hit BlockType is incorrect");
        require(nearlyEqual(hit->distance, 2.5f), "Hit distance must be 2.5");


        std::cout << "[PASS] Ray hits nearest solid block\n";
    }



    void testMaximumDistance() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ 0, 0, 2 }, ve::BlockType::Stone);



        const ve::Vec3 origin{ 0.5f, 0.5f, 5.5f };
        const ve::Vec3 direction{ 0.0f, 0.0f, -1.0f };



        const auto tooShort = ve::VoxelRaycaster::cast(
            world, origin, direction, 2.49f
        );

        const auto exactReach = ve::VoxelRaycaster::cast(
            world, origin, direction, 2.5f
        );



        require(!tooShort.has_value(), "Ray must not hit beyond maximum distance");
        require(exactReach.has_value(), "Ray must hit at exact maximum distance");


        std::cout << "[PASS] Ray maximum distance limitation\n";
    }



    void testNegativeCoordinates() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ -17, 0, 0 }, ve::BlockType::Stone);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ -14.5f, 0.5f, 0.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            6.0f
        );



        require(hit.has_value(), "Negative-coordinate block must be found");

        requireBlock(hit->block, -17, 0, 0, "Negative block coordinates are incorrect");
        requireBlock(hit->faceNormal, 1, 0, 0, "Negative X face normal is incorrect");

        require(nearlyEqual(hit->distance, 1.5f), "Negative X distance is incorrect");


        std::cout << "[PASS] Negative World and Chunk coordinates\n";
    }



    void testChunkBoundary() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ 16, 0, 0 }, ve::BlockType::Grass);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 15.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            2.0f
        );



        require(hit.has_value(), "Ray must cross Chunk boundary");

        requireBlock(hit->block, 16, 0, 0, "Chunk boundary hit is incorrect");
        requireBlock(hit->faceNormal, -1, 0, 0, "Positive X face normal is incorrect");

        require(hit->type == ve::BlockType::Grass, "Hit block must be Grass");
        require(nearlyEqual(hit->distance, 0.5f), "Chunk boundary distance is incorrect");


        std::cout << "[PASS] Ray traversal across Chunk boundary\n";
    }



    void testStartsInsideSolidBlock() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ 0, 0, 0 }, ve::BlockType::Stone);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f
        );



        require(hit.has_value(), "Ray starting inside Stone must hit");

        requireBlock(hit->block, 0, 0, 0, "Inside hit block is incorrect");
        requireBlock(hit->faceNormal, 0, 0, 0, "Inside hit must have no entry face");

        require(nearlyEqual(hit->distance, 0.0f), "Inside hit distance must be zero");


        std::cout << "[PASS] Ray starts inside solid block\n";
    }



    void testExactNegativeBoundary() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ -1, 0, 0 }, ve::BlockType::Stone);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.0f, 0.5f, 0.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            1.0f
        );



        require(hit.has_value(), "Negative boundary must hit adjacent block");

        requireBlock(hit->block, -1, 0, 0, "Negative boundary block is incorrect");

        require(nearlyEqual(hit->distance, 0.0f), "Boundary hit distance must be zero");


        std::cout << "[PASS] Exact negative-direction boundary\n";
    }



    void testDiagonalRay() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ 2, 0, 2 }, ve::BlockType::Stone);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 1.0f },
            6.0f
        );



        require(hit.has_value(), "Diagonal Ray must hit target block");

        requireBlock(hit->block, 2, 0, 2, "Diagonal hit block is incorrect");

        const float expectedDistance = 1.5f * std::sqrt(2.0f);

        require(nearlyEqual(hit->distance, expectedDistance), "Diagonal distance is incorrect");


        std::cout << "[PASS] Diagonal Voxel traversal\n";
    }



    void testVerticalRay() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ 1, 4, -2 }, ve::BlockType::Stone);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 1.5f, 1.5f, -1.5f },
            ve::Vec3{ 0.0f, 1.0f, 0.0f },
            6.0f
        );



        require(hit.has_value(), "Vertical Ray must hit target");

        requireBlock(hit->block, 1, 4, -2, "Vertical hit block is incorrect");
        requireBlock(hit->faceNormal, 0, -1, 0, "Vertical face normal is incorrect");

        require(nearlyEqual(hit->distance, 2.5f), "Vertical hit distance is incorrect");


        std::cout << "[PASS] Vertical Y-axis Ray traversal\n";
    }



    void testEmptyWorldAndRayMiss() {
        ve::World world{};



        const auto emptyWorldHit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 0.5f, 3.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            6.0f
        );



        require(!emptyWorldHit.has_value(), "Empty World must not produce a hit");



        world.setBlock(ve::WorldBlockPosition{ 0, 0, 0 }, ve::BlockType::Stone);



        // The Ray Passes One Block Above the Stone

        const auto missedRay = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 1.5f, 3.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            6.0f
        );



        require(!missedRay.has_value(), "Ray must not hit an off-axis block");


        std::cout << "[PASS] Empty World and off-axis Ray miss\n";
    }



    void testDirectionNormalization() {
        ve::World world{};

        world.setBlock(ve::WorldBlockPosition{ 0, 0, 2 }, ve::BlockType::Stone);



        const auto hit = ve::VoxelRaycaster::cast(
            world,
            ve::Vec3{ 0.5f, 0.5f, 5.5f },
            ve::Vec3{ 0.0f, 0.0f, -10.0f },
            6.0f
        );



        require(hit.has_value(), "Unnormalized direction must still work");

        requireBlock(hit->block, 0, 0, 2, "Unnormalized Ray hit is incorrect");

        require(nearlyEqual(hit->distance, 2.5f), "Distance must use normalized direction");


        std::cout << "[PASS] Ray direction normalization\n";
    }



    void testInvalidInputs() {
        ve::World world{};

        const ve::Vec3 origin{ 0.0f, 0.0f, 0.0f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };



        auto expectInvalid = [&]( const ve::Vec3& rayOrigin, const ve::Vec3& rayDirection,
                                  float maxDistance ) {
            bool caught = false;


            try {
                (void)ve::VoxelRaycaster::cast(
                    world, rayOrigin, rayDirection, maxDistance
                );
            }
            catch (const std::invalid_argument&) {
                caught = true;
            }


            require(caught, "Invalid Ray input must be rejected");
        };



        expectInvalid(origin, ve::Vec3{ 0.0f, 0.0f, 0.0f }, 6.0f);

        expectInvalid(origin, direction, -1.0f);

        expectInvalid(
            origin,
            direction,
            std::numeric_limits<float>::infinity()
        );

        expectInvalid(
            ve::Vec3{ std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f },
            direction,
            6.0f
        );


        std::cout << "[PASS] Invalid Voxel Ray input validation\n";
    }
}



int main() {
    try {
        testStraightRayHitsFirstBlock();
        testMaximumDistance();
        testNegativeCoordinates();
        testChunkBoundary();
        testStartsInsideSolidBlock();
        testExactNegativeBoundary();
        testDiagonalRay();
        testVerticalRay();
        testEmptyWorldAndRayMiss();
        testDirectionNormalization();
        testInvalidInputs();



        std::cout << "\nAll Voxel Raycaster tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}