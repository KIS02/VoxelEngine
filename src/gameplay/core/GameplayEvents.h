#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "gameplay/core/GameplayEvent.h"


namespace ve
{
    class GameplayEvents {
        private:
            std::vector<GameplayEvent> pending_{};
            static constexpr std::size_t maxPending_ = 4096;


        public:
            void publish( GameplayEvent event ) {
                if (pending_.size() >= maxPending_)
                    throw std::overflow_error("Gameplay event queue capacity exceeded");

                pending_.push_back(std::move(event));
            }

            std::vector<GameplayEvent> take() {
                std::vector<GameplayEvent> result;
                result.swap(pending_);
                return result;
            }

            std::size_t pendingCount() const {
                return pending_.size();
            }
    };
}
