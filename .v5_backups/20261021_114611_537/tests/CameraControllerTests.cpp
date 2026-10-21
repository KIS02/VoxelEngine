#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "scene/CameraController.h"



namespace
{
    constexpr float epsilon = 0.0001f;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool nearlyEqual( float a, float b ) {
        return std::fabs(a - b) < epsilon;
    }



    void requirePosition( const ve::Camera& camera, float x, float y, float z ) {
        const ve::Vec3& p = camera.position();

        require(
            nearlyEqual(p.x, x) &&
            nearlyEqual(p.y, y) &&
            nearlyEqual(p.z, z),
            "Camera position mismatch"
        );
    }



    void testIdleAndCancelledMovement() {
        ve::Camera camera{};
        ve::CameraController controller{};

        controller.update(camera, ve::CameraMovementInput{}, 1.0f);
        requirePosition(camera, 0.0f, 0.0f, 0.0f);



        ve::CameraMovementInput input{};

        input.forward = input.backward = true;
        input.left = input.right = true;
        input.up = input.down = true;

        controller.update(camera, input, 1.0f);

        requirePosition(camera, 0.0f, 0.0f, 0.0f);


        std::cout << "[PASS] Idle and opposing inputs cancel\n";
    }



    void testForwardBackward() {
        ve::Camera camera{};
        ve::CameraController controller{};

        ve::CameraMovementInput input{};

        input.forward = true;

        controller.update(camera, input, 0.5f);
        requirePosition(camera, 0.0f, 0.0f, -4.0f);



        input.forward = false;
        input.backward = true;

        controller.update(camera, input, 0.5f);
        requirePosition(camera, 0.0f, 0.0f, 0.0f);


        std::cout << "[PASS] Forward/backward movement\n";
    }



    void testStrafing() {
        ve::Camera camera{};
        ve::CameraController controller{};

        ve::CameraMovementInput input{};

        input.right = true;

        controller.update(camera, input, 0.5f);
        requirePosition(camera, 4.0f, 0.0f, 0.0f);



        input.right = false;
        input.left = true;

        controller.update(camera, input, 0.5f);
        requirePosition(camera, 0.0f, 0.0f, 0.0f);


        std::cout << "[PASS] Camera-relative strafing\n";
    }



    void testVertical() {
        ve::Camera camera{};
        ve::CameraController controller{};

        ve::CameraMovementInput input{};

        input.up = true;

        controller.update(camera, input, 0.25f);
        requirePosition(camera, 0.0f, 2.0f, 0.0f);



        input.up = false;
        input.down = true;

        controller.update(camera, input, 0.25f);
        requirePosition(camera, 0.0f, 0.0f, 0.0f);


        std::cout << "[PASS] Vertical movement\n";
    }



    void testDiagonalSpeed() {
        ve::Camera camera{};
        ve::CameraController controller{};

        ve::CameraMovementInput input{};

        input.forward = true;
        input.right = true;

        controller.update(camera, input, 1.0f);



        const float axis = 8.0f / std::sqrt(2.0f);

        requirePosition(camera, axis, 0.0f, -axis);


        std::cout << "[PASS] Diagonal movement normalization\n";
    }



    void testYawRelativeDirection() {
        ve::Camera camera{};
        ve::CameraController controller{};

        camera.setYawPitch(0.0f, 0.0f);



        ve::CameraMovementInput input{};

        input.forward = true;

        controller.update(camera, input, 1.0f);
        requirePosition(camera, 8.0f, 0.0f, 0.0f);



        input.forward = false;
        input.right = true;

        controller.update(camera, input, 0.5f);
        requirePosition(camera, 8.0f, 0.0f, 4.0f);


        std::cout << "[PASS] Movement follows camera Yaw\n";
    }



    void testPitchDoesNotAffectHorizontalMovement() {
        ve::Camera camera{};
        ve::CameraController controller{};

        camera.setYawPitch(-90.0f, 75.0f);



        ve::CameraMovementInput input{};
        input.forward = true;

        controller.update(camera, input, 1.0f);

        requirePosition(camera, 0.0f, 0.0f, -8.0f);


        std::cout << "[PASS] Forward motion stays horizontal\n";
    }



    void testFrameRateIndependence() {
        ve::Camera once{};
        ve::Camera many{};

        ve::CameraController controller{};

        ve::CameraMovementInput input{};
        input.forward = true;



        // One Update with a Full Second

        controller.update(once, input, 1.0f);



        // Sixty Updates with 1/60 Second Each

        for (int i = 0; i < 60; ++i) {
            controller.update(many, input, 1.0f / 60.0f);
        }



        requirePosition(
            many,
            once.position().x,
            once.position().y,
            once.position().z
        );


        std::cout << "[PASS] DeltaTime frame-rate independence\n";
    }



    void testSpeedAndInvalidInputs() {
        ve::Camera camera{};
        ve::CameraController controller{};

        controller.setMoveSpeed(12.0f);

        require(nearlyEqual(controller.moveSpeed(), 12.0f), "Movement speed setter failed");



        ve::CameraMovementInput input{};
        input.up = true;

        controller.update(camera, input, 0.5f);

        requirePosition(camera, 0.0f, 6.0f, 0.0f);



        bool badSpeed = false;
        bool badTime = false;



        try {
            controller.setMoveSpeed(0.0f);
        }
        catch (const std::invalid_argument&) {
            badSpeed = true;
        }



        try {
            controller.update(camera, input, -1.0f);
        }
        catch (const std::invalid_argument&) {
            badTime = true;
        }



        require(badSpeed && badTime, "Invalid movement inputs must be rejected");

        requirePosition(camera, 0.0f, 6.0f, 0.0f);


        std::cout << "[PASS] Movement speed and invalid input validation\n";
    }
}



int main() {
    try {
        testIdleAndCancelledMovement();
        testForwardBackward();
        testStrafing();
        testVertical();
        testDiagonalSpeed();
        testYawRelativeDirection();
        testPitchDoesNotAffectHorizontalMovement();
        testFrameRateIndependence();
        testSpeedAndInvalidInputs();



        std::cout << "\nAll Camera Controller tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}