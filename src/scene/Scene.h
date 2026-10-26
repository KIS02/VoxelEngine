#pragma once

#include <optional>
#include <cstdint>

#include "assets/block/BlockDefinition.h"
#include "input/ApplicationInput.h"
#include "input/MouseTracker.h"
#include "scene/entity/EntityRegistry.h"
#include "scene/object/SceneObjectRegistry.h"
#include "voxel/ray/VoxelRaycaster.h"


namespace ve
{
    class Camera;
    class VoxelWorld;

    class Scene {
        private:
            SceneObjectRegistry objects_;
            EntityRegistry entities_;


        public:
            virtual ~Scene() = default;

            Scene( const Scene& ) = delete;
            Scene& operator=( const Scene& ) = delete;

            Scene( Scene&& ) = delete;
            Scene& operator=( Scene&& ) = delete;


            SceneObjectRegistry& objects();
            const SceneObjectRegistry& objects() const;

            EntityRegistry& entities();
            const EntityRegistry& entities() const;

            bool removeObject( SceneObjectId id );


            virtual Camera& camera() = 0;
            virtual const Camera& camera() const = 0;

            virtual std::uint64_t voxelMaterialId() const;

            virtual std::uint64_t voxelMaterialId( BlockMaterialKey ) const {
                return voxelMaterialId();
            }

            virtual VoxelWorld* voxelWorld();
            virtual const VoxelWorld* voxelWorld() const;

            virtual std::optional<VoxelRaycastHit> selectedBlock() const;

            virtual void update( float deltaTime, bool cursorCaptured,
                                 const MouseDelta& delta, const InputActions& actions ) = 0;


        protected:
            Scene() = default;
    };
}