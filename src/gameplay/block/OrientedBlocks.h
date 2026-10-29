#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <tuple>

#include "math/Vec3.h"
#include "voxel/ray/VoxelRaycaster.h"


namespace ve
{
    struct OrientedBlockKey
    {
        int x = 0;
        int y = 0;
        int z = 0;

        bool operator<( const OrientedBlockKey& other ) const {
            return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
        }
    };


    class OrientedBlocks {
        private:
            std::map<OrientedBlockKey, std::uint8_t> orientations_{};

        public:
            static std::uint8_t facing( const Vec3& forward ) {
                if (!std::isfinite(forward.x) || !std::isfinite(forward.z))
                    throw std::invalid_argument("Invalid placement facing");
                if (std::fabs(forward.x) > std::fabs(forward.z))
                    return forward.x > 0.0f ? 1 : 3;
                return forward.z > 0.0f ? 2 : 0;
            }

            void set( const WorldBlockPosition& position, std::uint8_t facing ) {
                if (facing > 3)
                    throw std::invalid_argument("Invalid block facing");
                orientations_[OrientedBlockKey{ position.x, position.y, position.z }] = facing;
            }

            void erase( const WorldBlockPosition& position ) {
                orientations_.erase(OrientedBlockKey{ position.x, position.y, position.z });
            }

            std::uint8_t get( const WorldBlockPosition& position ) const {
                const auto it = orientations_.find(OrientedBlockKey{ position.x, position.y, position.z });
                return it == orientations_.end() ? 0 : it->second;
            }

            std::size_t size() const { return orientations_.size(); }
            bool load( const std::filesystem::path& path );
            void save( const std::filesystem::path& path ) const;
    };
}
