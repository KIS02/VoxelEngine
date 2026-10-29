#include "gameplay/mining/MiningController.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>


namespace ve
{
    namespace
    {
        bool sameBlock( const WorldBlockPosition& a, const WorldBlockPosition& b ) {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }
    }

    void MiningController::reset() {
        target_.reset();
        elapsed_ = 0.0f;
        progress_ = MiningProgress{};
    }

    const MiningProgress& MiningController::progress() const {
        return progress_;
    }

    MiningProgress MiningController::advance( float seconds, bool held, const Vec3& eye,
                                              const Vec3& forward, float reach, VoxelStorage& world,
                                              const GameplayBlockCatalog& blocks, Inventory& inventory,
                                              WorldItems& drops ) {
        if (!std::isfinite(seconds) || seconds < 0.0f)
            throw std::invalid_argument("Invalid mining tick duration");
        if (!held) {
            reset();
            return progress_;
        }

        const auto hit = VoxelRaycaster::cast(world, eye, forward, reach);
        if (!hit || hit->type == BlockId::Air) {
            reset();
            return progress_;
        }
        if (!target_ || !sameBlock(*target_, hit->block)) {
            target_ = hit->block;
            elapsed_ = 0.0f;
        }
        const GameplayBlockDefinition& definition = blocks.definition(hit->type);
        const auto heldTool = inventory.at(inventory.selectedHotbar());
        const float speed = ToolRules::miningSpeed(hit->type, heldTool);
        const float required = std::max(0.05f, definition.hardness * 1.5f / speed);
        elapsed_ += seconds;
        progress_ = MiningProgress{};
        progress_.block = hit->block;
        progress_.normalized = std::min(1.0f, elapsed_ / required);
        progress_.crackStage = std::clamp(static_cast<int>(progress_.normalized * 10.0f), 0, 9);
        if (elapsed_ + 1.0e-5f < required)
            return progress_;

        // Prepare a drop before mutating the block, and roll it back on mutation failure.
        std::optional<std::uint64_t> spawned{};
        if (definition.drop != ItemId::None) {
            try {
                spawned = drops.spawn(ItemStack{ definition.drop, 1 }, Vec3{
                    static_cast<float>(hit->block.x) + 0.5f,
                    static_cast<float>(hit->block.y) + 0.5f,
                    static_cast<float>(hit->block.z) + 0.5f
                });
            } catch (const std::length_error&) {
                // Keep the block intact if its drop cannot be persisted.
                reset();
                return progress_;
            }
        }
        try {
            world.setBlock(hit->block, BlockId::Air);
        } catch (...) {
            if (spawned)
                drops.remove(*spawned);
            throw;
        }
        const bool brokenTool = ToolRules::consumeUse(inventory, inventory.selectedHotbar());
        MiningProgress completed = progress_;
        completed.completed = true;
        completed.brokenType = hit->type;
        completed.toolBroke = brokenTool;
        reset();
        return completed;
    }
}
