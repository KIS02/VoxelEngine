#include "scene/Scene.h"


namespace ve
{
    SceneObjectRegistry& Scene::objects() {
        return objects_;
    }

    const SceneObjectRegistry& Scene::objects() const {
        return objects_;
    }

    EntityRegistry& Scene::entities() {
        return entities_;
    }

    const EntityRegistry& Scene::entities() const {
        return entities_;
    }

    std::uint64_t Scene::voxelMaterialId() const {
        return 0;
    }

    VoxelWorld* Scene::voxelWorld() {
        return nullptr;
    }

    const VoxelWorld* Scene::voxelWorld() const {
        return nullptr;
    }

    std::optional<VoxelRaycastHit> Scene::selectedBlock() const {
        return std::nullopt;
    }

    bool Scene::removeObject( SceneObjectId id ) {
        if (!objects_.remove(id))
            return false;

        entities_.detachVisualObject(id);
        return true;
    }
}
