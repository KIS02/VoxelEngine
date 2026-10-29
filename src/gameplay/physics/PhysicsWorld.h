#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>
#include <utility>

#include "gameplay/physics/CollisionAabb.h"
#include "voxel/chunk/VoxelStorage.h"


namespace ve
{
    struct DynamicCollider
    {
        std::uint64_t id = 0;
        CollisionAabb bounds{};
        Vec3 velocity{};
        bool solid = true;
    };


    struct PhysicsObstacle
    {
        CollisionAabb bounds{};
        Vec3 velocity{};
        std::uint64_t id = 0;
    };


    class PhysicsWorld {
        private:
            const VoxelStorage* voxels_ = nullptr;
            std::vector<DynamicCollider> dynamic_{};

        public:
            explicit PhysicsWorld( const VoxelStorage& voxels ) : voxels_(&voxels) { }

            void setDynamicColliders( std::vector<DynamicCollider> colliders ) {
                for (std::size_t i = 0; i < colliders.size(); ++i) {
                    colliders[i].bounds.validate();
                    if (!std::isfinite(colliders[i].velocity.x) ||
                        !std::isfinite(colliders[i].velocity.y) ||
                        !std::isfinite(colliders[i].velocity.z))
                        throw std::invalid_argument("Invalid dynamic collider velocity");
                    for (std::size_t j = 0; j < i; ++j) {
                        if (colliders[i].id == colliders[j].id)
                            throw std::invalid_argument("Duplicate dynamic collider ID");
                    }
                }
                dynamic_ = std::move(colliders);
            }

            const std::vector<DynamicCollider>& dynamicColliders() const {
                return dynamic_;
            }

            bool intersects( const CollisionAabb& box ) const {
                box.validate();
                for (const DynamicCollider& collider : dynamic_) {
                    if (collider.solid && box.overlaps(collider.bounds))
                        return true;
                }
                bool occupied = false;
                visitVoxels(box, [&]( const CollisionAabb& block ) {
                    if (box.overlaps(block))
                        occupied = true;
                });
                return occupied;
            }

            bool canPlaceBlock( const WorldBlockPosition& position, const CollisionAabb& player ) const {
                const CollisionAabb block{
                    Vec3{ static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z) },
                    Vec3{ static_cast<float>(position.x) + 1.0f, static_cast<float>(position.y) + 1.0f,
                          static_cast<float>(position.z) + 1.0f }
                };
                return !block.overlaps(player);
            }

            std::vector<PhysicsObstacle> collect( const CollisionAabb& box ) const {
                box.validate();
                std::vector<PhysicsObstacle> obstacles;
                for (const DynamicCollider& collider : dynamic_) {
                    if (collider.solid)
                        obstacles.push_back(PhysicsObstacle{ collider.bounds, collider.velocity, collider.id });
                }
                visitVoxels(box, [&]( const CollisionAabb& block ) {
                    obstacles.push_back(PhysicsObstacle{ block, Vec3{}, 0 });
                });
                return obstacles;
            }

            std::optional<Vec3> findSafeEyePosition( const Vec3& preferred, float radius,
                                                      float height, float eyeHeight ) const {
                if (!std::isfinite(preferred.x) || !std::isfinite(preferred.y) ||
                    !std::isfinite(preferred.z))
                    throw std::invalid_argument("Invalid spawn search position");

                // Search the local columns in distance order, from higher surfaces to lower surfaces.
                for (int ring = 0; ring <= 4; ++ring) {
                    for (int dx = -ring; dx <= ring; ++dx) {
                        for (int dz = -ring; dz <= ring; ++dz) {
                            if (ring > 0 && std::max(std::abs(dx), std::abs(dz)) != ring)
                                continue;
                            const float x = std::floor(preferred.x) + 0.5f + static_cast<float>(dx);
                            const float z = std::floor(preferred.z) + 0.5f + static_cast<float>(dz);
                            for (int dy = 8; dy >= -64; --dy) {
                                const float bottom = std::floor(preferred.y) + static_cast<float>(dy);
                                const CollisionAabb body{
                                    Vec3{ x - radius, bottom + 0.001f, z - radius },
                                    Vec3{ x + radius, bottom + height + 0.001f, z + radius }
                                };
                                if (intersects(body))
                                    continue;
                                const CollisionAabb support{
                                    Vec3{ x - radius, bottom - 0.05f, z - radius },
                                    Vec3{ x + radius, bottom + 0.001f, z + radius }
                                };
                                if (!intersects(support))
                                    continue;
                                return Vec3{ x, bottom + eyeHeight + 0.001f, z };
                            }
                        }
                    }
                }
                return std::nullopt;
            }

        private:
            template <typename Visitor>
            void visitVoxels( const CollisionAabb& area, Visitor&& visit ) const {
                constexpr double minInt = static_cast<double>(std::numeric_limits<int>::min()) + 1.0;
                constexpr double maxInt = static_cast<double>(std::numeric_limits<int>::max()) - 1.0;
                if (area.min.x < minInt || area.min.y < minInt || area.min.z < minInt ||
                    area.max.x > maxInt || area.max.y > maxInt || area.max.z > maxInt)
                    throw std::out_of_range("Physics voxel coordinate out of range");
                const int x0 = static_cast<int>(std::floor(area.min.x)) - 1;
                const int y0 = static_cast<int>(std::floor(area.min.y)) - 1;
                const int z0 = static_cast<int>(std::floor(area.min.z)) - 1;
                const int x1 = static_cast<int>(std::floor(area.max.x)) + 1;
                const int y1 = static_cast<int>(std::floor(area.max.y)) + 1;
                const int z1 = static_cast<int>(std::floor(area.max.z)) + 1;
                const std::int64_t nx = static_cast<std::int64_t>(x1) - x0 + 1;
                const std::int64_t ny = static_cast<std::int64_t>(y1) - y0 + 1;
                const std::int64_t nz = static_cast<std::int64_t>(z1) - z0 + 1;
                if (nx > 64 || ny > 64 || nz > 64 || nx * ny * nz > 16384)
                    throw std::length_error("Physics voxel query exceeds broadphase budget");

                for (int z = z0; z <= z1; ++z) {
                    for (int y = y0; y <= y1; ++y) {
                        for (int x = x0; x <= x1; ++x) {
                            if (!isSolid(voxels_->blockAt(WorldBlockPosition{ x, y, z })))
                                continue;
                            visit(CollisionAabb{
                                Vec3{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) },
                                Vec3{ static_cast<float>(x) + 1.0f, static_cast<float>(y) + 1.0f,
                                      static_cast<float>(z) + 1.0f }
                            });
                        }
                    }
                }
            }
    };
}
