#include "scene/entity/EntityRegistry.h"

#include <stdexcept>
#include <utility>


namespace ve
{
    EntityId EntityRegistry::create( Entity entity ) {
        if (nextId_ == 0)
            throw std::overflow_error("Entity ID space exhausted");

        const EntityId id{ nextId_++ };
        entity.id = id;
        entities_.emplace(id.value, std::move(entity));
        return id;
    }

    bool EntityRegistry::remove( EntityId id ) {
        return entities_.erase(id.value) != 0;
    }

    Entity* EntityRegistry::find( EntityId id ) {
        const auto it = entities_.find(id.value);
        return it == entities_.end() ? nullptr : &it->second;
    }

    const Entity* EntityRegistry::find( EntityId id ) const {
        const auto it = entities_.find(id.value);
        return it == entities_.end() ? nullptr : &it->second;
    }

    std::size_t EntityRegistry::size() const {
        return entities_.size();
    }

    void EntityRegistry::detachVisualObject( SceneObjectId visualId ) {
        if (!visualId.valid())
            return;

        for (auto& [id, entity] : entities_) {
            if (entity.visualObject == visualId)
                entity.visualObject = SceneObjectId{};
        }
    }
}
