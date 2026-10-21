#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "scene/camera/CameraController.h"



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



    void requireAngles( const ve::Camera& camera, float yaw, float pitch ) {
        require(
            nearlyEqual(camera.yawDegrees(), yaw) &&
            nearlyEqual(camera.pitchDegrees(), pitch),
            "Camera Yaw/Pitch mismatch"
        );
    }



    void testDefaultSensitivity() {
        const ve::CameraController controller{};

        require(
            nearlyEqual(controller.mouseSensitivity(), 0.1f),
            "Default mouse sensitivity must be 0.1"
        );


        std::cout << "[PASS] Default Mouse sensitivity\n";
    }



    void testHorizontalRotation() {
        ve::Camera camera{};
        ve::CameraController controller{};



        // Move Mouse 50 Pixels Right

        controller.updateLook(camera, ve::MouseDelta{ 50.0, 0.0 });

        requireAngles(camera, -85.0f, 0.0f);



        // Move Mouse 50 Pixels Left

        controller.updateLook(camera, ve::MouseDelta{ -50.0, 0.0 });

        requireAngles(camera, -90.0f, 0.0f);


        std::cout << "[PASS] Mouse horizontal Yaw rotation\n";
    }



    void testVerticalRotation() {
        ve::Camera camera{};
        ve::CameraController controller{};



        // Mouse Up: Negative GLFW Y Delta

        controller.updateLook(camera, ve::MouseDelta{ 0.0, -20.0 });

        requireAngles(camera, -90.0f, 2.0f);



        // Mouse Down: Positive GLFW Y Delta

        controller.updateLook(camera, ve::MouseDelta{ 0.0, 20.0 });

        requireAngles(camera, -90.0f, 0.0f);


        std::cout << "[PASS] Mouse vertical Pitch rotation\n";
    }



    void testCustomSensitivity() {
        ve::Camera camera{};
        ve::CameraController controller{};

        controller.setMouseSensitivity(0.25f);



        require(
            nearlyEqual(controller.mouseSensitivity(), 0.25f),
            "Custom sensitivity was not stored"
        );


        controller.updateLook(camera, ve::MouseDelta{ 40.0, -8.0 });

        requireAngles(camera, -80.0f, 2.0f);


        std::cout << "[PASS] Custom Mouse sensitivity\n";
    }



    void testPitchClamping() {
        ve::Camera camera{};
        ve::CameraController controller{};



        // Very Large Upward Movement

        controller.updateLook(camera, ve::MouseDelta{ 0.0, -2000.0 });

        requireAngles(camera, -90.0f, 89.0f);



        // Very Large Downward Movement

        controller.updateLook(camera, ve::MouseDelta{ 0.0, 4000.0 });

        requireAngles(camera, -90.0f, -89.0f);


        std::cout << "[PASS] Mouse Pitch clamping\n";
    }



    void testYawWrapping() {
        ve::Camera camera{};
        ve::CameraController controller{};

        camera.setYawPitch(179.0f, 0.0f);



        // 179 + 4 = 183 Degrees, Equivalent to -177 Degrees

        controller.updateLook(camera, ve::MouseDelta{ 40.0, 0.0 });

        requireAngles(camera, -177.0f, 0.0f);


        std::cout << "[PASS] Mouse Yaw angle wrapping\n";
    }



    void testForwardVectorAfterRotation() {
        ve::Camera camera{};
        ve::CameraController controller{};



        // Rotate from -Z to +X

        controller.updateLook(camera, ve::MouseDelta{ 900.0, 0.0 });

        requireAngles(camera, 0.0f, 0.0f);



        const ve::Vec3 forward = camera.forward();

        require(nearlyEqual(forward.x, 1.0f), "Forward X must be positive");
        require(nearlyEqual(forward.y, 0.0f), "Forward Y must be zero");
        require(nearlyEqual(forward.z, 0.0f), "Forward Z must be zero");


        std::cout << "[PASS] Camera Forward updates after Mouse rotation\n";
    }



    void testMovementUsesUpdatedDirection() {
        ve::Camera camera{};
        ve::CameraController controller{};



        // Rotate the Camera Toward +X

        controller.updateLook(camera, ve::MouseDelta{ 900.0, 0.0 });



        // Move Forward for Half a Second

        ve::CameraMovementInput input{};
        input.forward = true;

        controller.update(camera, input, 0.5f);



        const ve::Vec3& position = camera.position();

        require(nearlyEqual(position.x, 4.0f), "Forward movement must follow rotated Camera");
        require(nearlyEqual(position.y, 0.0f), "Horizontal movement must preserve height");
        require(nearlyEqual(position.z, 0.0f), "Forward movement must not continue toward -Z");


        std::cout << "[PASS] WASD movement follows Mouse Look\n";
    }



    void testZeroMouseDelta() {
        ve::Camera camera{};
        ve::CameraController controller{};

        camera.setPosition(ve::Vec3{ 2.0f, 3.0f, 4.0f });
        camera.setYawPitch(25.0f, 15.0f);



        controller.updateLook(camera, ve::MouseDelta{});

        requireAngles(camera, 25.0f, 15.0f);



        const ve::Vec3& position = camera.position();

        require(nearlyEqual(position.x, 2.0f), "Mouse Look must preserve Camera X");
        require(nearlyEqual(position.y, 3.0f), "Mouse Look must preserve Camera Y");
        require(nearlyEqual(position.z, 4.0f), "Mouse Look must preserve Camera Z");


        std::cout << "[PASS] Zero Mouse Delta preserves Camera state\n";
    }



    void testInvalidInputs() {
        ve::Camera camera{};
        ve::CameraController controller{};

        bool badSensitivity = false;
        bool badX = false;
        bool badY = false;



        try {
            controller.setMouseSensitivity(0.0f);
        }
        catch (const std::invalid_argument&) {
            badSensitivity = true;
        }



        try {
            controller.updateLook(
                camera,
                ve::MouseDelta{ std::numeric_limits<double>::infinity(), 0.0 }
            );
        }
        catch (const std::invalid_argument&) {
            badX = true;
        }



        try {
            controller.updateLook(
                camera,
                ve::MouseDelta{ 0.0, std::numeric_limits<double>::quiet_NaN() }
            );
        }
        catch (const std::invalid_argument&) {
            badY = true;
        }



        require(badSensitivity, "Zero sensitivity must be rejected");
        require(badX, "Infinite Mouse Delta X must be rejected");
        require(badY, "NaN Mouse Delta Y must be rejected");

        requireAngles(camera, -90.0f, 0.0f);


        std::cout << "[PASS] Invalid Mouse Look input validation\n";
    }
}



int main() {
    try {
        testDefaultSensitivity();
        testHorizontalRotation();
        testVerticalRotation();
        testCustomSensitivity();
        testPitchClamping();
        testYawWrapping();
        testForwardVectorAfterRotation();
        testMovementUsesUpdatedDirection();
        testZeroMouseDelta();
        testInvalidInputs();



        std::cout << "\nAll Mouse Look tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}