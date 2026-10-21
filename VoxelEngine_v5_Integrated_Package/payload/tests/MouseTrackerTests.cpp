#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "input/MouseTracker.h"



namespace
{
    constexpr double epsilon = 0.000001;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool nearlyEqual( double a, double b ) {
        return std::fabs(a - b) < epsilon;
    }



    void requireDelta( const ve::MouseDelta& delta, double x, double y ) {
        require(
            nearlyEqual(delta.x, x) && nearlyEqual(delta.y, y),
            "Mouse delta mismatch"
        );
    }



    void testDefaultState() {
        const ve::MouseTracker tracker{};

        requireDelta(tracker.delta(), 0.0, 0.0);


        std::cout << "[PASS] Default Mouse Tracker state\n";
    }



    void testFirstSampleIgnored() {
        ve::MouseTracker tracker{};

        tracker.sample(400.0, 300.0);


        requireDelta(tracker.delta(), 0.0, 0.0);


        std::cout << "[PASS] First Mouse sample ignored\n";
    }



    void testMovementDirections() {
        ve::MouseTracker tracker{};

        tracker.sample(400.0, 300.0);



        // Move Right and Up

        tracker.sample(415.0, 290.0);

        requireDelta(tracker.delta(), 15.0, -10.0);



        // Move Left and Down

        tracker.sample(395.0, 310.0);

        requireDelta(tracker.delta(), -20.0, 20.0);


        std::cout << "[PASS] Mouse movement direction signs\n";
    }



    void testStationaryMouse() {
        ve::MouseTracker tracker{};

        tracker.sample(100.0, 100.0);
        tracker.sample(120.0, 110.0);

        requireDelta(tracker.delta(), 20.0, 10.0);



        // Same Position in the Next Frame

        tracker.sample(120.0, 110.0);

        requireDelta(tracker.delta(), 0.0, 0.0);


        std::cout << "[PASS] Stationary Mouse produces zero Delta\n";
    }



    void testFrameRelativeMovement() {
        ve::MouseTracker tracker{};

        tracker.sample(10.0, 20.0);

        tracker.sample(15.0, 23.0);

        requireDelta(tracker.delta(), 5.0, 3.0);



        // Delta Must Be Relative to the Most Recent Sample

        tracker.sample(17.0, 22.0);

        requireDelta(tracker.delta(), 2.0, -1.0);


        std::cout << "[PASS] Mouse Delta is relative to previous frame\n";
    }



    void testResetAndResume() {
        ve::MouseTracker tracker{};

        tracker.sample(100.0, 100.0);
        tracker.sample(110.0, 120.0);

        requireDelta(tracker.delta(), 10.0, 20.0);



        // Simulate Losing Window Focus

        tracker.reset();

        requireDelta(tracker.delta(), 0.0, 0.0);



        // First Sample After Regaining Focus

        tracker.sample(500.0, 400.0);

        requireDelta(tracker.delta(), 0.0, 0.0);



        // Normal Movement Resumes

        tracker.sample(504.0, 397.0);

        requireDelta(tracker.delta(), 4.0, -3.0);


        std::cout << "[PASS] Mouse reset and focus recovery\n";
    }



    void testInvalidCoordinates() {
        ve::MouseTracker tracker{};

        tracker.sample(10.0, 20.0);



        bool invalidCaught = false;



        try {
            tracker.sample(
                std::numeric_limits<double>::infinity(),
                30.0
            );
        }
        catch (const std::invalid_argument&) {
            invalidCaught = true;
        }



        require(invalidCaught, "Infinite Mouse coordinate must be rejected");



        // Invalid Samples Must Not Corrupt the Previous Position

        tracker.sample(12.0, 23.0);

        requireDelta(tracker.delta(), 2.0, 3.0);


        std::cout << "[PASS] Invalid Mouse coordinate validation\n";
    }
}



int main() {
    try {
        testDefaultState();
        testFirstSampleIgnored();
        testMovementDirections();
        testStationaryMouse();
        testFrameRelativeMovement();
        testResetAndResume();
        testInvalidCoordinates();



        std::cout << "\nAll Mouse Tracker tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}