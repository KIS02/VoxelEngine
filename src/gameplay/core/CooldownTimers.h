#pragma once

#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>


namespace ve
{
    class CooldownTimers {
        private:
            std::map<std::string, float> remaining_{};


        public:
            void start( const std::string& id, float seconds ) {
                if (id.empty() || !std::isfinite(seconds) || seconds <= 0.0f)
                    throw std::invalid_argument("Invalid cooldown");

                remaining_[id] = seconds;
            }

            bool ready( const std::string& id ) const {
                return remaining_.find(id) == remaining_.end();
            }

            float remaining( const std::string& id ) const {
                const auto it = remaining_.find(id);
                return it == remaining_.end() ? 0.0f : it->second;
            }

            std::vector<std::string> advance( float deltaTime ) {
                if (!std::isfinite(deltaTime) || deltaTime < 0.0f)
                    throw std::invalid_argument("Invalid cooldown time step");

                std::vector<std::string> expired;
                for (auto it = remaining_.begin(); it != remaining_.end();) {
                    it->second -= deltaTime;
                    if (it->second <= 0.0f) {
                        expired.push_back(it->first);
                        it = remaining_.erase(it);
                    } else {
                        ++it;
                    }
                }
                return expired;
            }

            void clear() {
                remaining_.clear();
            }
    };
}
