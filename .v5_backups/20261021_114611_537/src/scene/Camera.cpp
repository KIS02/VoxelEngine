#include "scene/Camera.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>



namespace ve
{
    namespace
    {
        constexpr float pi = 3.14159265358979323846f;

        constexpr float degreesToRadians = pi / 180.0f;
        constexpr float radiansToDegrees = 180.0f / pi;

        constexpr float maximumPitch = 89.0f;
    }



    void Camera::setPosition( const Vec3& position ) {
        if (!std::isfinite(position.x) ||
            !std::isfinite(position.y) ||
            !std::isfinite(position.z)) {
            throw std::invalid_argument("Camera position must be finite");
        }


        position_ = position;
    }



    void Camera::setYawPitch( float yawDegrees, float pitchDegrees ) {
        if (!std::isfinite(yawDegrees) || !std::isfinite(pitchDegrees)) {
            throw std::invalid_argument("Camera angles must be finite");
        }


        yawDegrees_ = yawDegrees;
        pitchDegrees_ = std::clamp(pitchDegrees, -maximumPitch, maximumPitch);
    }



    void Camera::lookAt( const Vec3& target ) {
        const Vec3 direction = target - position_;

        const float lengthSquared = ve::dot(direction, direction);



        // The Camera Cannot Look at Its Own Position

        if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f) {
            throw std::invalid_argument("Camera target must differ from its position");
        }



        // Normalize the Direction Vector

        const float inverseLength = 1.0f / std::sqrt(lengthSquared);

        const Vec3 normalizedDirection{
            direction.x * inverseLength,
            direction.y * inverseLength,
            direction.z * inverseLength
        };



        // Recover Yaw and Pitch from the Target Direction

        const float yaw = std::atan2(
            normalizedDirection.z,
            normalizedDirection.x
        ) * radiansToDegrees;

        const float pitch = std::asin(
            std::clamp(normalizedDirection.y, -1.0f, 1.0f)
        ) * radiansToDegrees;



        setYawPitch(yaw, pitch);
    }



    const Vec3& Camera::position() const {
        return position_;
    }



    float Camera::yawDegrees() const {
        return yawDegrees_;
    }



    float Camera::pitchDegrees() const {
        return pitchDegrees_;
    }



    Vec3 Camera::forward() const {
        const float yaw = yawDegrees_ * degreesToRadians;
        const float pitch = pitchDegrees_ * degreesToRadians;

        const float cosPitch = std::cos(pitch);



        return Vec3{
            cosPitch * std::cos(yaw),
            std::sin(pitch),
            cosPitch * std::sin(yaw)
        };
    }



    Mat4 Camera::viewMatrix() const {
        const Vec3 target = position_ + forward();

        const Vec3 worldUp{ 0.0f, 1.0f, 0.0f };


        return Mat4::lookAt(position_, target, worldUp);
    }
}