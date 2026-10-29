#pragma once

#include <optional>

#include "gameplay/block/GameplayBlockCatalog.h"
#include "gameplay/items/WorldItems.h"
#include "gameplay/tools/ToolRules.h"
#include "voxel/ray/VoxelRaycaster.h"
#include "voxel/chunk/VoxelStorage.h"


namespace ve
{
    struct MiningProgress
    {
        std::optional<WorldBlockPosition> block{};
        float normalized = 0.0f;
        int crackStage = 0;
        bool completed = false;
        BlockId brokenType = BlockId::Air;
        bool toolBroke = false;
    };


    class MiningController {
        private:
            std::optional<WorldBlockPosition> target_{};
            float elapsed_ = 0.0f;
            MiningProgress progress_{};

        public:
            void reset();
            const MiningProgress& progress() const;
            MiningProgress advance( float seconds, bool held, const Vec3& eye,
                                    const Vec3& forward, float reach, VoxelStorage& world,
                                    const GameplayBlockCatalog& blocks, Inventory& inventory,
                                    WorldItems& drops );
    };
}
