#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "scene/Scene.h"


namespace ve
{
    class SceneManager {
        private:
            std::map<std::string, std::unique_ptr<Scene>> scenes_;
            std::string activeName_;
            std::optional<std::string> pendingName_;


        public:
            bool add( const std::string& name, std::unique_ptr<Scene> scene );
            bool requestActivation( const std::string& name );
            bool applyPendingActivation();
            bool remove( const std::string& name );
            Scene* active();
            const Scene* active() const;
            const std::string& activeName() const;
            std::size_t size() const;
    };
}
