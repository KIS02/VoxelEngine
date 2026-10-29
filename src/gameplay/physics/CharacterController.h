#pragma once

#include <vector>

#include "gameplay/physics/PhysicsWorld.h"
#include "gameplay/physics/PlayerCollider.h"
#include "gameplay/player/PlayerMovementCommand.h"
#include "gameplay/player/PlayerState.h"


namespace ve
{
    struct CharacterPhysicsSettings
    {
        float gravity = 24.0f;
        float terminalSpeed = 40.0f;
        float groundAcceleration = 28.0f;
        float airAcceleration = 9.0f;
        float groundFriction = 18.0f;
        float airDrag = 1.0f;
        float fallDamageThreshold = 12.0f;
        float fallDamagePerSpeed = 1.0f;
    };


    struct CharacterPhysicsResult
    {
        bool jumped = false;
        bool landed = false;
        bool hitWall = false;
        bool hitCeiling = false;
        float landingSpeed = 0.0f;
        float pendingFallDamage = 0.0f;
    };


    class CharacterController {
        private:
            PlayerCollider collider_{};
            CharacterPhysicsSettings settings_{};
            bool jumpHeld_ = false;

        public:
            CharacterController() = default;

            const PlayerCollider& collider() const {
                return collider_;
            }

            const CharacterPhysicsSettings& settings() const {
                return settings_;
            }

            void reset() {
                jumpHeld_ = false;
            }

            CharacterPhysicsResult step( PlayerState& player, const PlayerMovementCommand& command,
                                          const PhysicsWorld& world, float deltaTime );

        private:
            struct MotionResult
            {
                Vec3 position{};
                Vec3 velocity{};
                bool grounded = false;
                bool wall = false;
                bool ceiling = false;
            };

            MotionResult move( const Vec3& start, const Vec3& velocity, float deltaTime,
                                const std::vector<PhysicsObstacle>& obstacles ) const;
    };
}
