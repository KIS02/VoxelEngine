#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "gameplay/block/GameplayBlockCatalog.h"
#include "gameplay/crafting/CraftingGrid.h"
#include "gameplay/mining/MiningController.h"
#include "gameplay/core/CooldownTimers.h"
#include "gameplay/core/FixedGameplayClock.h"
#include "gameplay/core/GameplayEvents.h"
#include "gameplay/world/WorldClock.h"
#include "gameplay/items/ItemRegistry.h"
#include "gameplay/items/Inventory.h"
#include "gameplay/items/WorldItems.h"
#include "gameplay/player/PlayerSpawn.h"
#include "gameplay/survival/SurvivalNeeds.h"
#include "gameplay/physics/CharacterController.h"


namespace ve
{
    class GameplaySession {
        private:
            PlayerState player_{};
            Vec3 lastObservedPosition_{};
            FixedGameplayClock clock_{};
            WorldClock worldClock_{};
            GameplayEvents events_{};
            CooldownTimers cooldowns_{};
            ItemRegistry items_{};
            GameplayBlockCatalog blocks_{};
            Inventory inventory_{ items_ };
            WorldItems worldItems_{ items_ };
            CraftingGrid crafting_{ items_ };
            MiningController mining_{};
            SurvivalNeeds needs_{};
            VoxelStorage* miningVoxels_ = nullptr;
            bool miningHeld_ = false;
            Vec3 miningForward_{ 0.0f, 0.0f, -1.0f };
            float miningReach_ = 6.0f;
            std::optional<PhysicsWorld> physics_{};
            CharacterController character_{};
            PlayerMovementCommand movement_{};
            bool pendingJump_ = false;
            bool previousFrameJump_ = false;
            float totalFallDamage_ = 0.0f;


        public:
            GameplaySession() = default;

            // Optional external definitions are loaded atomically as a pair.
            void loadCatalogs( const std::filesystem::path& blockFile,
                               const std::filesystem::path& itemFile );

            void start( const Vec3& position );
            void setMiningInput( bool held, const Vec3& forward, float reach );
            const MiningProgress& miningProgress() const { return mining_.progress(); }
            CraftingGrid& crafting() { return crafting_; }
            const CraftingGrid& crafting() const { return crafting_; }
            bool craftSelectedRecipe();
            void configurePhysics( const VoxelStorage& voxels );
            void configureMining( VoxelStorage& voxels ) { miningVoxels_ = &voxels; }
            void setDynamicColliders( std::vector<DynamicCollider> colliders );
            void setMovementCommand( const PlayerMovementCommand& movement );
            const PlayerCollider& playerCollider() const;
            CollisionAabb playerBounds() const;
            float totalFallDamage() const;
            void respawn();
            void restorePlayer( const PlayerState& saved );
            bool consumeSelectedFood();
            void toggleMode();
            void setMode( GameMode mode );
            void synchronizeCreativePosition( const Vec3& position );
            GameplayClockResult advance( float frameSeconds );

            const PlayerState& player() const {
                return player_;
            }

            PlayerState& player() {
                return player_;
            }

            const FixedGameplayClock& clock() const {
                return clock_;
            }

            WorldClock& worldClock() { return worldClock_; }
            const WorldClock& worldClock() const { return worldClock_; }

            GameplayEvents& events() {
                return events_;
            }

            CooldownTimers& cooldowns() {
                return cooldowns_;
            }

            Inventory& inventory() {
                return inventory_;
            }
            const Inventory& inventory() const {
                return inventory_;
            }
            WorldItems& worldItems() {
                return worldItems_;
            }
            const WorldItems& worldItems() const {
                return worldItems_;
            }

            bool dropSelected( const Vec3& forward );

            const ItemRegistry& items() const {
                return items_;
            }

            const GameplayBlockCatalog& blocks() const {
                return blocks_;
            }

        private:
            void fixedUpdate( float deltaTime );
    };
}
