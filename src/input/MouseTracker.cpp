#include "input/MouseTracker.h"

#include <cmath>
#include <stdexcept>



namespace ve
{
    void MouseTracker::sample( double x, double y ) {
        if (!std::isfinite(x) || !std::isfinite(y)) {
            throw std::invalid_argument("Mouse coordinates must be finite");
        }



        // Ignore the First Position Because There Is No Previous Sample

        if (!initialized_) {
            previousX_ = x;
            previousY_ = y;

            delta_ = MouseDelta{};
            initialized_ = true;

            return;
        }



        // Calculate Movement Since the Previous Sample

        const double deltaX = x - previousX_;
        const double deltaY = y - previousY_;

        if (!std::isfinite(deltaX) || !std::isfinite(deltaY)) {
            throw std::invalid_argument("Mouse delta must be finite");
        }



        delta_.x = deltaX;
        delta_.y = deltaY;



        // Store the Current Position for the Next Frame

        previousX_ = x;
        previousY_ = y;
    }



    void MouseTracker::reset() {
        previousX_ = 0.0;
        previousY_ = 0.0;

        delta_ = MouseDelta{};

        initialized_ = false;
    }



    MouseDelta MouseTracker::delta() const {
        return delta_;
    }
}