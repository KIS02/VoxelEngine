#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "math/Vec4.h"

#include "scene/camera/Camera.h"



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



    void requireVec3( const ve::Vec3& actual, const ve::Vec3& expected, const char* message ) {
        require(
            nearlyEqual(actual.x, expected.x) &&
            nearlyEqual(actual.y, expected.y) &&
            nearlyEqual(actual.z, expected.z),
            message
        );
    }



    void testDefaultCamera() {
        const ve::Camera camera{};


        requireVec3(
            camera.position(),
            ve::Vec3{ 0.0f, 0.0f, 0.0f },
            "Default Camera position is incorrect"
        );


        require(nearlyEqual(camera.yawDegrees(), -90.0f), "Default Yaw is incorrect");
        require(nearlyEqual(camera.pitchDegrees(), 0.0f), "Default Pitch is incorrect");


        requireVec3(
            camera.forward(),
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            "Default Camera forward direction is incorrect"
        );


        std::cout << "[PASS] Default Camera state\n";
    }



    void testYawRotation() {
        ve::Camera camera{};


        camera.setYawPitch(0.0f, 0.0f);

        requireVec3(
            camera.forward(),
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            "Yaw 0 must face positive X"
        );


        camera.setYawPitch(90.0f, 0.0f);

        requireVec3(
            camera.forward(),
            ve::Vec3{ 0.0f, 0.0f, 1.0f },
            "Yaw 90 must face positive Z"
        );


        camera.setYawPitch(-90.0f, 0.0f);

        requireVec3(
            camera.forward(),
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            "Yaw -90 must face negative Z"
        );


        std::cout << "[PASS] Camera Yaw rotation\n";
    }



    void testPitchRotation() {
        ve::Camera camera{};


        camera.setYawPitch(-90.0f, 45.0f);


        const float expected = std::sqrt(0.5f);

        requireVec3(
            camera.forward(),
            ve::Vec3{ 0.0f, expected, -expected },
            "Pitch 45 forward direction is incorrect"
        );


        camera.setYawPitch(-90.0f, 100.0f);

        require(
            nearlyEqual(camera.pitchDegrees(), 89.0f),
            "Positive Pitch must be clamped"
        );


        camera.setYawPitch(-90.0f, -100.0f);

        require(
            nearlyEqual(camera.pitchDegrees(), -89.0f),
            "Negative Pitch must be clamped"
        );


        std::cout << "[PASS] Camera Pitch rotation and clamping\n";
    }



    void testForwardNormalization() {
        ve::Camera camera{};


        camera.setYawPitch(35.0f, 25.0f);


        const ve::Vec3 forward = camera.forward();

        const float lengthSquared = ve::dot(forward, forward);


        require(
            nearlyEqual(lengthSquared, 1.0f),
            "Camera forward vector must have unit length"
        );


        std::cout << "[PASS] Camera forward vector normalization\n";
    }



    void testLookAtTarget() {
        ve::Camera camera{};


        camera.setPosition(ve::Vec3{ 0.0f, 0.0f, 5.0f });
        camera.lookAt(ve::Vec3{ 0.0f, 0.0f, 0.0f });



        requireVec3(
            camera.forward(),
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            "Camera must look toward the origin"
        );



        const ve::Mat4 view = camera.viewMatrix();

        const ve::Vec4 worldOrigin{ 0.0f, 0.0f, 0.0f, 1.0f };

        const ve::Vec4 cameraSpace = view * worldOrigin;



        require(nearlyEqual(cameraSpace.x, 0.0f), "View Space X is incorrect");
        require(nearlyEqual(cameraSpace.y, 0.0f), "View Space Y is incorrect");
        require(nearlyEqual(cameraSpace.z, -5.0f), "View Space Z must be -5");
        require(nearlyEqual(cameraSpace.w, 1.0f), "View Space W is incorrect");


        std::cout << "[PASS] Camera LookAt and View Matrix\n";
    }



    void testCameraPositionTransform() {
        ve::Camera camera{};


        camera.setPosition(ve::Vec3{ 2.0f, 3.0f, 4.0f });
        camera.lookAt(ve::Vec3{ 2.0f, 3.0f, 3.0f });



        const ve::Mat4 view = camera.viewMatrix();

        const ve::Vec4 cameraWorldPosition{ 2.0f, 3.0f, 4.0f, 1.0f };

        const ve::Vec4 transformed = view * cameraWorldPosition;



        require(nearlyEqual(transformed.x, 0.0f), "Camera position must map to View X=0");
        require(nearlyEqual(transformed.y, 0.0f), "Camera position must map to View Y=0");
        require(nearlyEqual(transformed.z, 0.0f), "Camera position must map to View Z=0");
        require(nearlyEqual(transformed.w, 1.0f), "Camera position must preserve W=1");


        std::cout << "[PASS] Camera VoxelStorage-to-View transformation\n";
    }



    void testInvalidCameraInputs() {
        ve::Camera camera{};

        bool invalidTargetCaught = false;
        bool invalidAngleCaught = false;



        try {
            camera.lookAt(camera.position());
        }
        catch (const std::invalid_argument&) {
            invalidTargetCaught = true;
        }



        try {
            camera.setYawPitch(std::numeric_limits<float>::infinity(), 0.0f);
        }
        catch (const std::invalid_argument&) {
            invalidAngleCaught = true;
        }



        require(invalidTargetCaught, "Camera must reject an identical LookAt target");
        require(invalidAngleCaught, "Camera must reject infinite angles");


        std::cout << "[PASS] Invalid Camera input validation\n";
    }
}



int main() {
    try {
        testDefaultCamera();
        testYawRotation();
        testPitchRotation();
        testForwardNormalization();
        testLookAtTarget();
        testCameraPositionTransform();
        testInvalidCameraInputs();



        std::cout << "\nAll Camera tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}