#include "scene/SceneManager.h"

#include <stdexcept>
#include <utility>


namespace ve
{
    bool SceneManager::add( const std::string& name, std::unique_ptr<Scene> scene ) {
        if (name.empty() || !scene || scenes_.find(name) != scenes_.end())
            return false;

        return scenes_.emplace(name, std::move(scene)).second;
    }

    bool SceneManager::requestActivation( const std::string& name ) {
        if (scenes_.find(name) == scenes_.end())
            return false;

        pendingName_ = name;
        return true;
    }

    bool SceneManager::applyPendingActivation() {
        if (!pendingName_)
            return false;

        activeName_ = *pendingName_;
        pendingName_.reset();
        return true;
    }

    bool SceneManager::remove( const std::string& name ) {
        if (name == activeName_ || (pendingName_ && name == *pendingName_))
            return false;

        return scenes_.erase(name) == 1;
    }

    Scene* SceneManager::active() {
        const auto it = scenes_.find(activeName_);
        return it == scenes_.end() ? nullptr : it->second.get();
    }

    const Scene* SceneManager::active() const {
        const auto it = scenes_.find(activeName_);
        return it == scenes_.end() ? nullptr : it->second.get();
    }

    const std::string& SceneManager::activeName() const {
        return activeName_;
    }

    std::size_t SceneManager::size() const {
        return scenes_.size();
    }
}
