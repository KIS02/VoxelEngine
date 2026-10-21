#pragma once

#include <cstddef>
#include <cstdint>
#include <map>

#include "scene/entity/Entity.h"


namespace ve
{
    class EntityRegistry {
        private:
            std::map<std::uint64_t, Entity> entities_;
            std::uint64_t nextId_ = 1;


        public:
            EntityId create( Entity entity = {} );
            bool remove( EntityId id );
            Entity* find( EntityId id );
            const Entity* find( EntityId id ) const;
            std::size_t size() const;

            // Called when a linked visual object is removed from the Scene.
            void detachVisualObject( SceneObjectId visualId );
    };
}
