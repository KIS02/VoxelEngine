#include "scene/CameraController.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>



namespace ve
{
    void CameraController::setMoveSpeed( float unitsPerSecond ) {
        if (!std::isfinite(unitsPerSecond) || unitsPerSecond <= 0.0f) {
            throw std::invalid_argument("Camera movement speed must be positive and finite");
        }

        moveSpeed_ = unitsPerSecond;
    }



    float CameraController::moveSpeed() const {
        return moveSpeed_;
    }



    void CameraController::setMouseSensitivity( float degreesPerPixel ) {
        if (!std::isfinite(degreesPerPixel) || degreesPerPixel <= 0.0f) {
            throw std::invalid_argument("Mouse sensitivity must be positive and finite");
        }

        mouseSensitivity_ = degreesPerPixel;
    }



    float CameraController::mouseSensitivity() const {
        return mouseSensitivity_;
    }



    void CameraController::update( Camera& camera, const CameraMovementInput& input, float deltaTime ) const {
        if (!std::isfinite(deltaTime) || deltaTime < 0.0f) {
            throw std::invalid_argument("Camera deltaTime must be nonnegative and finite");
        }

        if (deltaTime == 0.0f) {
            return;
        }



        // Convert Key States to Movement Axes

        const float forwardAxis = static_cast<float>(input.forward) - static_cast<float>(input.backward);
        const float rightAxis = static_cast<float>(input.right) - static_cast<float>(input.left);
        const float upAxis = static_cast<float>(input.up) - static_cast<float>(input.down);



        if (forwardAxis == 0.0f && rightAxis == 0.0f && upAxis == 0.0f) {
            return;
        }



        // Project Camera Forward onto the Horizontal Plane

        const Vec3 facing = camera.forward();

        const float flatLength = std::sqrt(facing.x * facing.x + facing.z * facing.z);

        if (flatLength <= 0.0f || !std::isfinite(flatLength)) {
            throw std::runtime_error("Camera horizontal forward direction is invalid");
        }



        const Vec3 forward{
            facing.x / flatLength,
            0.0f,
            facing.z / flatLength
        };

        const Vec3 right{ -forward.z, 0.0f, forward.x };



        // Combine Movement Directions

        const Vec3 movement{
            forward.x * forwardAxis + right.x * rightAxis,
            upAxis,
            forward.z * forwardAxis + right.z * rightAxis
        };



        // Normalize Diagonal Movement

        const float length = std::sqrt(
            movement.x * movement.x +
            movement.y * movement.y +
            movement.z * movement.z
        );

        const float distance = moveSpeed_ * deltaTime / length;



        const Vec3 displacement{
            movement.x * distance,
            movement.y * distance,
            movement.z * distance
        };

        camera.setPosition(camera.position() + displacement);
    }



    void CameraController::updateLook( Camera& camera, const MouseDelta& mouseDelta ) const {
        if (!std::isfinite(mouseDelta.x) || !std::isfinite(mouseDelta.y)) {
            throw std::invalid_argument("Mouse delta must be finite");
        }



        // No Mouse Movement

        if (mouseDelta.x == 0.0 && mouseDelta.y == 0.0) {
            return;
        }



        // Calculate Rotation in Double Precision

        const double nextYaw =
            static_cast<double>(camera.yawDegrees()) +
            mouseDelta.x * static_cast<double>(mouseSensitivity_);

        const double nextPitch =
            static_cast<double>(camera.pitchDegrees()) -
            mouseDelta.y * static_cast<double>(mouseSensitivity_);



        if (!std::isfinite(nextYaw) || !std::isfinite(nextPitch)) {
            throw std::invalid_argument("Mouse rotation exceeds the valid range");
        }



        // Keep Yaw in a Small Angle Range

        const double wrappedYaw = std::remainder(nextYaw, 360.0);



        // Prevent Looking Exactly Straight Up or Down

        constexpr double maximumPitch = 89.0;

        const double clampedPitch = std::clamp(
            nextPitch,
            -maximumPitch,
            maximumPitch
        );



        camera.setYawPitch(
            static_cast<float>(wrappedYaw),
            static_cast<float>(clampedPitch)
        );
    }
}