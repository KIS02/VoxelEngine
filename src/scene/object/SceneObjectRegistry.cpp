#include "scene/object/SceneObjectRegistry.h"

#include <limits>
#include <utility>


namespace ve
{
    SceneObjectId SceneObjectRegistry::create( SceneObject object ) {
        if (nextId_ == 0)
            throw std::overflow_error("SceneObject ID space exhausted");

        const SceneObjectId id{ nextId_++ };
        object.id = id;
        objects_.emplace(id.value, std::move(object));
        return id;
    }

    bool SceneObjectRegistry::remove( SceneObjectId id ) {
        return objects_.erase(id.value) != 0;
    }

    SceneObject* SceneObjectRegistry::find( SceneObjectId id ) {
        const auto it = objects_.find(id.value);
        return it == objects_.end() ? nullptr : &it->second;
    }

    const SceneObject* SceneObjectRegistry::find( SceneObjectId id ) const {
        const auto it = objects_.find(id.value);
        return it == objects_.end() ? nullptr : &it->second;
    }

    std::size_t SceneObjectRegistry::size() const {
        return objects_.size();
    }

    void SceneObjectRegistry::clear() {
        objects_.clear();
    }
}
