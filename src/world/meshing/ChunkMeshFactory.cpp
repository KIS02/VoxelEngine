#include "world/meshing/ChunkMeshFactory.h"

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "math/Vec2.h"
#include "math/Vec3.h"

#include "render/Color.h"
#include "assets/mesh/Vertex.h"

#include "world/BlockType.h"
#include "world/Chunk.h"
#include "world/ChunkCoordinates.h"
#include "world/World.h"



namespace ve
{
    namespace
    {
        Color32 blockColor( BlockType type ) {
            switch (type) {
                case BlockType::Grass:
                    return makeColor(80, 180, 70);

                case BlockType::Dirt:
                    return makeColor(135, 90, 50);

                case BlockType::Stone:
                    return makeColor(150, 150, 150);

                case BlockType::Air:
                    return makeColor(255, 255, 255);
            }

            return makeColor(255, 0, 255);
        }



        bool isFaceExposed( const Chunk& chunk, const World* world,
                            const ChunkPosition& chunkPosition, int x, int y, int z ) {

            // Neighbor Is Inside the Current Chunk

            if (Chunk::isInside(x, y, z)) {
                return !isSolid(chunk.blockAt(x, y, z));
            }



            // Legacy Single-Chunk Mode

            if (world == nullptr) {
                return true;
            }



            // Find the Neighbor Chunk Position

            std::int64_t chunkX = chunkPosition.x;
            std::int64_t chunkY = chunkPosition.y;
            std::int64_t chunkZ = chunkPosition.z;



            // Convert Out-of-Bounds Local Coordinates

            if (x < 0) {
                --chunkX;
                x += Chunk::Size;
            }
            else if (x >= Chunk::Size) {
                ++chunkX;
                x -= Chunk::Size;
            }

            if (y < 0) {
                --chunkY;
                y += Chunk::Size;
            }
            else if (y >= Chunk::Size) {
                ++chunkY;
                y -= Chunk::Size;
            }

            if (z < 0) {
                --chunkZ;
                z += Chunk::Size;
            }
            else if (z >= Chunk::Size) {
                ++chunkZ;
                z -= Chunk::Size;
            }



            // Coordinates Beyond ChunkPosition's Range Cannot Be Loaded

            constexpr std::int64_t minimum = std::numeric_limits<int>::min();
            constexpr std::int64_t maximum = std::numeric_limits<int>::max();

            if (chunkX < minimum || chunkX > maximum ||
                chunkY < minimum || chunkY > maximum ||
                chunkZ < minimum || chunkZ > maximum) {
                return true;
            }



            const ChunkPosition neighborPosition{
                static_cast<int>(chunkX),
                static_cast<int>(chunkY),
                static_cast<int>(chunkZ)
            };


            const Chunk* neighbor = world->findChunk(neighborPosition);



            // Missing Chunks Are Temporarily Treated as Air

            if (neighbor == nullptr) {
                return true;
            }


            return !isSolid(neighbor->blockAt(x, y, z));
        }



        void appendFace( std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices,
                         const std::array<Vec3, 4>& positions, Color32 color ) {

            const std::uint32_t baseIndex = static_cast<std::uint32_t>(vertices.size());



            // Four Vertices with a Shared UV Convention

            vertices.push_back(Vertex{ positions[0], color, Vec2{ 0.0f, 0.0f } });
            vertices.push_back(Vertex{ positions[1], color, Vec2{ 1.0f, 0.0f } });
            vertices.push_back(Vertex{ positions[2], color, Vec2{ 1.0f, 1.0f } });
            vertices.push_back(Vertex{ positions[3], color, Vec2{ 0.0f, 1.0f } });



            // Two Counter-Clockwise Triangles

            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);

            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }



        MeshData buildMesh( const Chunk& chunk, const World* world,
                            const ChunkPosition& position ) {

            std::vector<Vertex> vertices;
            std::vector<std::uint32_t> indices;



            // Visit Every Block

            for (int y = 0; y < Chunk::Size; ++y) {
                for (int z = 0; z < Chunk::Size; ++z) {
                    for (int x = 0; x < Chunk::Size; ++x) {
                        const BlockType type = chunk.blockAt(x, y, z);


                        if (!isSolid(type)) {
                            continue;
                        }


                        const Color32 color = blockColor(type);



                        // Local Block Boundaries

                        const float x0 = static_cast<float>(x);
                        const float y0 = static_cast<float>(y);
                        const float z0 = static_cast<float>(z);

                        const float x1 = x0 + 1.0f;
                        const float y1 = y0 + 1.0f;
                        const float z1 = z0 + 1.0f;



                        // Front (+Z)

                        if (isFaceExposed(chunk, world, position, x, y, z + 1)) {
                            appendFace(
                                vertices, indices,
                                std::array<Vec3, 4>{
                                    Vec3{ x0, y0, z1 },
                                    Vec3{ x1, y0, z1 },
                                    Vec3{ x1, y1, z1 },
                                    Vec3{ x0, y1, z1 }
                                },
                                color
                            );
                        }



                        // Back (-Z)

                        if (isFaceExposed(chunk, world, position, x, y, z - 1)) {
                            appendFace(
                                vertices, indices,
                                std::array<Vec3, 4>{
                                    Vec3{ x0, y0, z0 },
                                    Vec3{ x0, y1, z0 },
                                    Vec3{ x1, y1, z0 },
                                    Vec3{ x1, y0, z0 }
                                },
                                color
                            );
                        }



                        // Left (-X)

                        if (isFaceExposed(chunk, world, position, x - 1, y, z)) {
                            appendFace(
                                vertices, indices,
                                std::array<Vec3, 4>{
                                    Vec3{ x0, y0, z0 },
                                    Vec3{ x0, y0, z1 },
                                    Vec3{ x0, y1, z1 },
                                    Vec3{ x0, y1, z0 }
                                },
                                color
                            );
                        }



                        // Right (+X)

                        if (isFaceExposed(chunk, world, position, x + 1, y, z)) {
                            appendFace(
                                vertices, indices,
                                std::array<Vec3, 4>{
                                    Vec3{ x1, y0, z1 },
                                    Vec3{ x1, y0, z0 },
                                    Vec3{ x1, y1, z0 },
                                    Vec3{ x1, y1, z1 }
                                },
                                color
                            );
                        }



                        // Top (+Y)

                        if (isFaceExposed(chunk, world, position, x, y + 1, z)) {
                            appendFace(
                                vertices, indices,
                                std::array<Vec3, 4>{
                                    Vec3{ x0, y1, z1 },
                                    Vec3{ x1, y1, z1 },
                                    Vec3{ x1, y1, z0 },
                                    Vec3{ x0, y1, z0 }
                                },
                                color
                            );
                        }



                        // Bottom (-Y)

                        if (isFaceExposed(chunk, world, position, x, y - 1, z)) {
                            appendFace(
                                vertices, indices,
                                std::array<Vec3, 4>{
                                    Vec3{ x0, y0, z0 },
                                    Vec3{ x1, y0, z0 },
                                    Vec3{ x1, y0, z1 },
                                    Vec3{ x0, y0, z1 }
                                },
                                color
                            );
                        }
                    }
                }
            }



            return MeshData{ std::move(vertices), std::move(indices) };
        }
    }



    MeshData ChunkMeshFactory::create( const Chunk& chunk ) {
        // Preserve the Existing Single-Chunk Behavior

        return buildMesh(chunk, nullptr, ChunkPosition{});
    }



    MeshData ChunkMeshFactory::create( const World& world, const ChunkPosition& position ) {
        const Chunk* chunk = world.findChunk(position);

        if (chunk == nullptr) {
            throw std::invalid_argument("Cannot create Mesh for a missing Chunk");
        }


        return buildMesh(*chunk, &world, position);
    }
}