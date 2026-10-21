#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>

#include "scene/object/SceneObject.h"


namespace ve
{
    class SceneObjectRegistry {
        private:
            std::map<std::uint64_t, SceneObject> objects_;
            std::uint64_t nextId_ = 1;


        public:
            SceneObjectId create( SceneObject object = {} );
            bool remove( SceneObjectId id );
            SceneObject* find( SceneObjectId id );
            const SceneObject* find( SceneObjectId id ) const;
            std::size_t size() const;
            void clear();

            template <typename Visitor>
            void forEach( Visitor&& visitor ) const {
                for (const auto& [id, object] : objects_)
                    visitor(object);
            }
    };
}
