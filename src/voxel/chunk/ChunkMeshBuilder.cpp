#include "voxel/chunk/ChunkMeshBuilder.h"

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Color.h"

#include "assets/mesh/Vertex.h"
#include "assets/block/BlockRegistry.h"

#include "voxel/block/BlockId.h"
#include "voxel/chunk/Chunk.h"
#include "voxel/chunk/ChunkCoordinates.h"
#include "voxel/chunk/VoxelStorage.h"


namespace ve
{
    namespace
    {
        bool isFaceExposed( const Chunk& chunk, const VoxelStorage* world,
                            const ChunkPosition& chunkPosition, int x, int y, int z ) {
            if (Chunk::isInside(x, y, z))
                return !isSolid(chunk.blockAt(x, y, z));

            if (world == nullptr)
                return true;

            std::int64_t chunkX = chunkPosition.x;
            std::int64_t chunkY = chunkPosition.y;
            std::int64_t chunkZ = chunkPosition.z;

            if (x < 0) {
                --chunkX;
                x += Chunk::Size;
            } else if (x >= Chunk::Size) {
                ++chunkX;
                x -= Chunk::Size;
            }

            if (y < 0) {
                --chunkY;
                y += Chunk::Size;
            } else if (y >= Chunk::Size) {
                ++chunkY;
                y -= Chunk::Size;
            }

            if (z < 0) {
                --chunkZ;
                z += Chunk::Size;
            } else if (z >= Chunk::Size) {
                ++chunkZ;
                z -= Chunk::Size;
            }

            constexpr std::int64_t minimum = std::numeric_limits<int>::min();
            constexpr std::int64_t maximum = std::numeric_limits<int>::max();

            if (chunkX < minimum || chunkX > maximum ||
                chunkY < minimum || chunkY > maximum ||
                chunkZ < minimum || chunkZ > maximum)
                return true;

            const ChunkPosition neighborPosition{
                static_cast<int>(chunkX),
                static_cast<int>(chunkY),
                static_cast<int>(chunkZ)
            };

            const Chunk* neighbor = world->findChunk(neighborPosition);

            if (neighbor == nullptr)
                return true;

            return !isSolid(neighbor->blockAt(x, y, z));
        }


        struct FaceBuffers
        {
            std::vector<Vertex> vertices;
            std::vector<std::uint32_t> indices;
        };


        void appendFace( FaceBuffers& buffers, const std::array<Vec3, 4>& positions, Color32 color ) {
            const std::uint32_t baseIndex = static_cast<std::uint32_t>(buffers.vertices.size());

            const Vec3 normal = cross(
                positions[1] - positions[0],
                positions[2] - positions[0]
            ).normalized();

            buffers.vertices.push_back(Vertex{ positions[0], color, Vec2{ 0.0f, 0.0f }, normal });
            buffers.vertices.push_back(Vertex{ positions[1], color, Vec2{ 1.0f, 0.0f }, normal });
            buffers.vertices.push_back(Vertex{ positions[2], color, Vec2{ 1.0f, 1.0f }, normal });
            buffers.vertices.push_back(Vertex{ positions[3], color, Vec2{ 0.0f, 1.0f }, normal });

            buffers.indices.push_back(baseIndex + 0);
            buffers.indices.push_back(baseIndex + 1);
            buffers.indices.push_back(baseIndex + 2);

            buffers.indices.push_back(baseIndex + 0);
            buffers.indices.push_back(baseIndex + 2);
            buffers.indices.push_back(baseIndex + 3);
        }


        template<typename EmitFace>
        void visitVisibleFaces( const Chunk& chunk, const VoxelStorage* world,
                                const ChunkPosition& position, EmitFace&& emit ) {
            for (int y = 0; y < Chunk::Size; ++y) {
                for (int z = 0; z < Chunk::Size; ++z) {
                    for (int x = 0; x < Chunk::Size; ++x) {
                        const BlockId type = chunk.blockAt(x, y, z);

                        if (!isSolid(type))
                            continue;

                        const Color32 color = BlockRegistry::definition(type).tint;

                        const float x0 = static_cast<float>(x);
                        const float y0 = static_cast<float>(y);
                        const float z0 = static_cast<float>(z);
                        const float x1 = x0 + 1.0f;
                        const float y1 = y0 + 1.0f;
                        const float z1 = z0 + 1.0f;

                        if (isFaceExposed(chunk, world, position, x, y, z + 1)) {
                            emit(type, BlockFace::Front, std::array<Vec3, 4>{
                                Vec3{ x0, y0, z1 },
                                Vec3{ x1, y0, z1 },
                                Vec3{ x1, y1, z1 },
                                Vec3{ x0, y1, z1 }
                            }, color);
                        }

                        if (isFaceExposed(chunk, world, position, x, y, z - 1)) {
                            emit(type, BlockFace::Back, std::array<Vec3, 4>{
                                Vec3{ x0, y0, z0 },
                                Vec3{ x0, y1, z0 },
                                Vec3{ x1, y1, z0 },
                                Vec3{ x1, y0, z0 }
                            }, color);
                        }

                        if (isFaceExposed(chunk, world, position, x - 1, y, z)) {
                            emit(type, BlockFace::Left, std::array<Vec3, 4>{
                                Vec3{ x0, y0, z0 },
                                Vec3{ x0, y0, z1 },
                                Vec3{ x0, y1, z1 },
                                Vec3{ x0, y1, z0 }
                            }, color);
                        }

                        if (isFaceExposed(chunk, world, position, x + 1, y, z)) {
                            emit(type, BlockFace::Right, std::array<Vec3, 4>{
                                Vec3{ x1, y0, z1 },
                                Vec3{ x1, y0, z0 },
                                Vec3{ x1, y1, z0 },
                                Vec3{ x1, y1, z1 }
                            }, color);
                        }

                        if (isFaceExposed(chunk, world, position, x, y + 1, z)) {
                            emit(type, BlockFace::Top, std::array<Vec3, 4>{
                                Vec3{ x0, y1, z1 },
                                Vec3{ x1, y1, z1 },
                                Vec3{ x1, y1, z0 },
                                Vec3{ x0, y1, z0 }
                            }, color);
                        }

                        if (isFaceExposed(chunk, world, position, x, y - 1, z)) {
                            emit(type, BlockFace::Bottom, std::array<Vec3, 4>{
                                Vec3{ x0, y0, z0 },
                                Vec3{ x1, y0, z0 },
                                Vec3{ x1, y0, z1 },
                                Vec3{ x0, y0, z1 }
                            }, color);
                        }
                    }
                }
            }
        }


        MeshData buildLegacyMesh( const Chunk& chunk, const VoxelStorage* world,
                                  const ChunkPosition& position ) {
            FaceBuffers buffers;

            visitVisibleFaces(
                chunk, world, position,
                [&buffers]( BlockId, BlockFace, const std::array<Vec3, 4>& positions, Color32 color ) {
                    appendFace(buffers, positions, color);
                }
            );

            return MeshData{ std::move(buffers.vertices), std::move(buffers.indices) };
        }


        MaterialMeshes buildMaterialMeshes( const Chunk& chunk, const VoxelStorage* world,
                                            const ChunkPosition& position ) {
            std::map<BlockMaterialKey, FaceBuffers> buffers;

            visitVisibleFaces(
                chunk, world, position,
                [&buffers]( BlockId type, BlockFace face,
                            const std::array<Vec3, 4>& positions, Color32 ) {
                    appendFace(
                        buffers[BlockRegistry::material(type, face)],
                        positions,
                        makeColor(255, 255, 255)
                    );
                }
            );

            MaterialMeshes result;

            for (auto& [key, mesh] : buffers)
                result.emplace(key, MeshData{ std::move(mesh.vertices), std::move(mesh.indices) });

            return result;
        }
    }


    MeshData ChunkMeshBuilder::create( const Chunk& chunk ) {
        return buildLegacyMesh(chunk, nullptr, ChunkPosition{});
    }


    MeshData ChunkMeshBuilder::create( const VoxelStorage& world, const ChunkPosition& position ) {
        const Chunk* chunk = world.findChunk(position);

        if (chunk == nullptr)
            throw std::invalid_argument("Cannot create Mesh for a missing Chunk");

        return buildLegacyMesh(*chunk, &world, position);
    }


    MaterialMeshes ChunkMeshBuilder::createMaterialMeshes( const Chunk& chunk ) {
        return buildMaterialMeshes(chunk, nullptr, ChunkPosition{});
    }


    MaterialMeshes ChunkMeshBuilder::createMaterialMeshes( const VoxelStorage& world,
                                                            const ChunkPosition& position ) {
        const Chunk* chunk = world.findChunk(position);

        if (chunk == nullptr)
            throw std::invalid_argument("Cannot create material meshes for a missing Chunk");

        return buildMaterialMeshes(*chunk, &world, position);
    }
}