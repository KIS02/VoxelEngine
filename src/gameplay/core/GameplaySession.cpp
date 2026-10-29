#include "gameplay/core/GameplaySession.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <utility>


namespace ve
{
    void GameplaySession::loadCatalogs( const std::filesystem::path& blockFile,
                                        const std::filesystem::path& itemFile ) {
        ItemRegistry newItems = items_;
        GameplayBlockCatalog newBlocks = blocks_;
        newItems.loadFile(itemFile);
        newBlocks.loadFile(blockFile);

        for (const BlockId id : { BlockId::Grass, BlockId::Dirt, BlockId::Stone, BlockId::CraftingTable }) {
            const ItemId drop = newBlocks.definition(id).drop;
            if (drop != ItemId::None && !newItems.contains(drop))
                throw std::invalid_argument("Block references an unknown ItemId");
        }

        items_ = std::move(newItems);
        blocks_ = std::move(newBlocks);
    }

    void GameplaySession::configurePhysics( const VoxelStorage& voxels ) {
        physics_.emplace(voxels);
    }

    void GameplaySession::setDynamicColliders( std::vector<DynamicCollider> colliders ) {
        if (!physics_)
            throw std::logic_error("Physics must be configured before registering colliders");
        physics_->setDynamicColliders(std::move(colliders));
    }

    void GameplaySession::setMovementCommand( const PlayerMovementCommand& movement ) {
        if (movement.jump && !previousFrameJump_)
            pendingJump_ = true;
        previousFrameJump_ = movement.jump;
        movement_ = movement;
    }

    const PlayerCollider& GameplaySession::playerCollider() const {
        return character_.collider();
    }

    CollisionAabb GameplaySession::playerBounds() const {
        return character_.collider().boundsAt(player_.position);
    }

    float GameplaySession::totalFallDamage() const {
        return totalFallDamage_;
    }

    void GameplaySession::start( const Vec3& position ) {
        PlayerSpawn::spawn(player_, position);
        lastObservedPosition_ = player_.position;
        clock_.reset();
        worldClock_.reset();
        mining_.reset();
        miningHeld_ = false;
        cooldowns_.clear();
        character_.reset();
        movement_ = PlayerMovementCommand{};
        pendingJump_ = false;
        previousFrameJump_ = false;
        totalFallDamage_ = 0.0f;
        needs_.reset();
        events_.take();
        events_.publish(GameplayEvent{ GameplayEventType::PlayerSpawned, player_.id, player_.position, player_.mode });
    }

    void GameplaySession::respawn() {
        if (physics_ && player_.mode == GameMode::Survival) {
            const Vec3& wanted = player_.spawnPosition;
            const auto safe = physics_->findSafeEyePosition(wanted, character_.collider().radius,
                character_.collider().height, character_.collider().eyeHeight);
            if (!safe.has_value())
                throw std::runtime_error("No valid Survival respawn location");
            PlayerSpawn::setSpawnPoint(player_, *safe);
        }
        PlayerSpawn::respawn(player_);
        character_.reset();
        mining_.reset();
        pendingJump_ = false;
        previousFrameJump_ = false;
        lastObservedPosition_ = player_.position;
        cooldowns_.clear();
        needs_.reset();
        events_.publish(GameplayEvent{ GameplayEventType::PlayerRespawned, player_.id, player_.position, player_.mode });
    }

    void GameplaySession::restorePlayer( const PlayerState& saved ) {
        if (!player_.spawned)
            throw std::logic_error("Player must spawn before restoring state");
        PlayerState candidate = saved;
        candidate.stats.validate();
        if (!PlayerState::finitePosition(candidate.position) ||
            !PlayerState::finitePosition(candidate.spawnPosition))
            throw std::invalid_argument("Invalid saved player position");
        if (candidate.mode == GameMode::Survival && physics_ &&
            physics_->intersects(character_.collider().boundsAt(candidate.position))) {
            const auto safe = physics_->findSafeEyePosition(candidate.position,
                character_.collider().radius, character_.collider().height,
                character_.collider().eyeHeight);
            if (!safe)
                throw std::runtime_error("Cannot restore player inside a solid voxel");
            candidate.position = *safe;
        }
        candidate.previousPosition = candidate.position;
        candidate.velocity = Vec3{};
        candidate.grounded = false;
        player_ = candidate;
        lastObservedPosition_ = player_.position;
        character_.reset();
        movement_ = PlayerMovementCommand{};
        pendingJump_ = false;
        previousFrameJump_ = false;
        mining_.reset();
        needs_.reset();
        cooldowns_.clear();
        events_.take();
    }

    bool GameplaySession::consumeSelectedFood() {
        return needs_.eat(player_, inventory_, events_);
    }

    void GameplaySession::toggleMode() {
        setMode(player_.mode == GameMode::Creative ? GameMode::Survival : GameMode::Creative);
    }

    void GameplaySession::setMode( GameMode mode ) {
        if (!player_.spawned)
            throw std::logic_error("Player must spawn before changing game modes");
        if (player_.mode == mode)
            return;

        if (mode == GameMode::Survival && physics_) {
            const auto safe = physics_->findSafeEyePosition(player_.position,
                character_.collider().radius, character_.collider().height,
                character_.collider().eyeHeight);
            if (!safe.has_value())
                throw std::runtime_error("No valid Survival spawn location within search radius");
            player_.setPosition(*safe);
            PlayerSpawn::setSpawnPoint(player_, *safe);
        }
        player_.setMode(mode);
        character_.reset();
        mining_.reset();
        pendingJump_ = false;
        previousFrameJump_ = false;
        movement_ = PlayerMovementCommand{};
        events_.publish(GameplayEvent{ GameplayEventType::GameModeChanged, player_.id, player_.position, mode });
    }

    void GameplaySession::synchronizeCreativePosition( const Vec3& position ) {
        if (!player_.spawned)
            throw std::logic_error("Player has not spawned");
        if (player_.mode != GameMode::Creative)
            return;

        player_.setPosition(position);
    }

    GameplayClockResult GameplaySession::advance( float frameSeconds ) {
        if (!player_.spawned)
            throw std::logic_error("Player has not spawned");

        return clock_.advance(frameSeconds, [&]( float fixedDelta ) {
            fixedUpdate(fixedDelta);
        });
    }

    void GameplaySession::setMiningInput( bool held, const Vec3& forward, float reach ) {
        if (!PlayerState::finitePosition(forward) || !std::isfinite(reach) || reach <= 0.0f)
            throw std::invalid_argument("Invalid mining input");
        miningHeld_ = held;
        miningForward_ = forward;
        miningReach_ = reach;
    }

    bool GameplaySession::craftSelectedRecipe() {
        if (!crafting_.craft(inventory_))
            return false;
        events_.publish(GameplayEvent{ GameplayEventType::ItemCrafted, player_.id,
            player_.position, player_.mode });
        return true;
    }

    bool GameplaySession::dropSelected( const Vec3& forward ) {
        if (!player_.spawned || !player_.alive)
            return false;
        const std::size_t selected = inventory_.selectedHotbar();
        const auto before = inventory_.at(selected);
        const auto item = inventory_.remove(selected, 1);
        if (!item)
            return false;
        try {
            worldItems_.spawn(*item, player_.position + forward * 1.3f + Vec3{ 0.0f, -0.5f, 0.0f });
            events_.publish(GameplayEvent{ GameplayEventType::ItemDropped, player_.id,
                player_.position, player_.mode, std::to_string(static_cast<int>(item->id)) });
        } catch (const std::length_error&) {
            inventory_.set(selected, before);
            return false;
        } catch (...) {
            inventory_.set(selected, before);
            throw;
        }
        return true;
    }

    void GameplaySession::fixedUpdate( float deltaTime ) {
        const std::uint64_t previousDay = worldClock_.day();
        worldClock_.advance(deltaTime);
        if (worldClock_.day() != previousDay)
            events_.publish(GameplayEvent{ GameplayEventType::WorldDayChanged, player_.id,
                player_.position, player_.mode, std::to_string(worldClock_.day()) });

        const auto collected = worldItems_.advance(deltaTime, player_.position, inventory_, player_.alive);
        for (const ItemStack& item : collected)
            events_.publish(GameplayEvent{ GameplayEventType::ItemPickedUp, player_.id,
                player_.position, player_.mode, std::to_string(static_cast<int>(item.id)) });
        const std::vector<std::string> expired = cooldowns_.advance(deltaTime);
        for (const std::string& name : expired)
            events_.publish(GameplayEvent{ GameplayEventType::CooldownExpired,
                                           player_.id, player_.position, player_.mode, name });

        if (physics_ && player_.mode == GameMode::Survival && player_.alive) {
            PlayerMovementCommand command = movement_;
            command.jump = command.jump || pendingJump_;
            command.sprint = command.sprint && player_.stamina > 0.0f && player_.hunger > 0.0f;
            pendingJump_ = false;
            const CharacterPhysicsResult result = character_.step(player_, command, *physics_, deltaTime);
            if (result.jumped)
                events_.publish(GameplayEvent{ GameplayEventType::PlayerJumped,
                    player_.id, player_.position, player_.mode });
            if (result.landed)
                events_.publish(GameplayEvent{ GameplayEventType::PlayerLanded,
                    player_.id, player_.position, player_.mode });
            if (result.pendingFallDamage > 0.0f) {
                const DamageResult damage = DamageSystem::apply(player_, result.pendingFallDamage,
                    DamageType::Fall, events_);
                totalFallDamage_ += damage.applied;
            }
            needs_.advance(player_, command.sprint && player_.grounded &&
                (command.forward != 0.0f || command.strafe != 0.0f), deltaTime, events_);
        }

        if (miningVoxels_ && player_.alive && player_.mode == GameMode::Survival) {
            const MiningProgress result = mining_.advance(deltaTime, miningHeld_, player_.position,
                miningForward_, miningReach_, *miningVoxels_, blocks_, inventory_, worldItems_);
            if (result.completed) {
                events_.publish(GameplayEvent{ GameplayEventType::BlockBroken, player_.id,
                    Vec3{ static_cast<float>(result.block->x), static_cast<float>(result.block->y),
                        static_cast<float>(result.block->z) }, player_.mode });
                if (result.toolBroke)
                    events_.publish(GameplayEvent{ GameplayEventType::ToolBroken, player_.id,
                        player_.position, player_.mode });
            }
        } else {
            mining_.reset();
        }

        const Vec3& position = player_.position;
        if (position.x != lastObservedPosition_.x || position.y != lastObservedPosition_.y ||
            position.z != lastObservedPosition_.z) {
            events_.publish(GameplayEvent{ GameplayEventType::PlayerMoved,
                player_.id, position, player_.mode });
            lastObservedPosition_ = position;
        }
    }
}
