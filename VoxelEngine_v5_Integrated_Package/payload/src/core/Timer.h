#pragma once

#include <chrono>


namespace ve
{

class Timer
{
public:

    Timer()
        :
        lastTime_(Clock::now())
    {
    }


    float tick()
    {
        const auto currentTime = Clock::now();


        const std::chrono::duration<float> delta =
            currentTime - lastTime_;


        lastTime_ = currentTime;


        return delta.count();
    }


private:

    using Clock = std::chrono::steady_clock;


    Clock::time_point lastTime_;
};

}