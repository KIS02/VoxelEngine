#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>


namespace ve
{
    struct GameplayClockResult
    {
        int steps = 0;
        float interpolationAlpha = 0.0f;
        double discardedSeconds = 0.0;
    };


    class FixedGameplayClock {
        private:
            double fixedStep_ = 1.0 / 60.0;
            double accumulator_ = 0.0;
            std::uint64_t ticks_ = 0;
            double discardedSeconds_ = 0.0;
            int maxCatchupSteps_ = 8;


        public:
            explicit FixedGameplayClock( double fixedStep = 1.0 / 60.0, int maxCatchupSteps = 8 )
                : fixedStep_(fixedStep), maxCatchupSteps_(maxCatchupSteps) {
                if (!std::isfinite(fixedStep_) || fixedStep_ <= 0.0 ||
                    fixedStep_ > 1.0 || maxCatchupSteps_ < 1 || maxCatchupSteps_ > 128)
                    throw std::invalid_argument("Invalid FixedGameplayClock settings");
            }

            template <typename Callback>
            GameplayClockResult advance( double frameSeconds, Callback&& tick ) {
                if (!std::isfinite(frameSeconds) || frameSeconds < 0.0)
                    throw std::invalid_argument("Invalid Gameplay frame delta");

                GameplayClockResult result{};
                const double accepted = std::min(frameSeconds, 0.25);
                result.discardedSeconds = frameSeconds - accepted;
                accumulator_ += accepted;

                while (accumulator_ + 1.0e-10 >= fixedStep_ && result.steps < maxCatchupSteps_) {
                    tick(static_cast<float>(fixedStep_));
                    accumulator_ -= fixedStep_;
                    if (accumulator_ < 0.0)
                        accumulator_ = 0.0;
                    ++ticks_;
                    ++result.steps;
                }

                if (accumulator_ + 1.0e-10 >= fixedStep_) {
                    double remainder = std::fmod(accumulator_, fixedStep_);
                    if (remainder + 1.0e-10 >= fixedStep_)
                        remainder = 0.0;
                    result.discardedSeconds += accumulator_ - remainder;
                    accumulator_ = remainder;
                }

                discardedSeconds_ += result.discardedSeconds;
                result.interpolationAlpha = static_cast<float>(accumulator_ / fixedStep_);
                return result;
            }

            void reset() {
                accumulator_ = 0.0;
                ticks_ = 0;
                discardedSeconds_ = 0.0;
            }

            std::uint64_t ticks() const {
                return ticks_;
            }

            double fixedStep() const {
                return fixedStep_;
            }

            double totalDiscardedSeconds() const {
                return discardedSeconds_;
            }
    };
}
