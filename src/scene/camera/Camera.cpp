#include "scene/camera/Camera.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>


namespace ve
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;
        constexpr float DegreesToRadians = Pi / 180.0f;
        constexpr float RadiansToDegrees = 180.0f / Pi;
        constexpr float MaximumPitch = 89.0f;
    }


    void Camera::setPosition( const Vec3& position ) {
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
            throw std::invalid_argument("Camera position must be finite");

        position_ = position;
    }

    void Camera::setYawPitch( float yawDegrees, float pitchDegrees ) {
        if (!std::isfinite(yawDegrees) || !std::isfinite(pitchDegrees))
            throw std::invalid_argument("Camera angles must be finite");

        yawDegrees_ = yawDegrees;
        pitchDegrees_ = std::clamp(pitchDegrees, -MaximumPitch, MaximumPitch);
    }

    void Camera::lookAt( const Vec3& target ) {
        const Vec3 direction = target - position_;
        const float lengthSquared = dot(direction, direction);
        if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f)
            throw std::invalid_argument("Camera target must differ from position");

        const Vec3 normalized = direction * (1.0f / std::sqrt(lengthSquared));
        const float yaw = std::atan2(normalized.z, normalized.x) * RadiansToDegrees;
        const float pitch = std::asin(std::clamp(normalized.y, -1.0f, 1.0f)) * RadiansToDegrees;
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
        const float yaw = yawDegrees_ * DegreesToRadians;
        const float pitch = pitchDegrees_ * DegreesToRadians;
        const float cosPitch = std::cos(pitch);
        return Vec3{ cosPitch * std::cos(yaw), std::sin(pitch), cosPitch * std::sin(yaw) };
    }

    void Camera::setProjectionMode( ProjectionMode mode ) {
        projectionMode_ = mode;
    }

    Camera::ProjectionMode Camera::projectionMode() const {
        return projectionMode_;
    }

    void Camera::setFieldOfViewDegrees( float degrees ) {
        if (!std::isfinite(degrees) || degrees <= 0.0f || degrees >= 180.0f)
            throw std::invalid_argument("Perspective FOV must be between 0 and 180 degrees");

        fieldOfViewDegrees_ = degrees;
    }

    void Camera::setOrthographicHeight( float height ) {
        if (!std::isfinite(height) || height <= 0.0f)
            throw std::invalid_argument("Orthographic height must be positive and finite");

        orthographicHeight_ = height;
    }

    void Camera::setClippingPlanes( float nearPlane, float farPlane ) {
        if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) || nearPlane <= 0.0f || farPlane <= nearPlane)
            throw std::invalid_argument("Clipping planes require 0 < near < far");

        nearPlane_ = nearPlane;
        farPlane_ = farPlane;
    }

    float Camera::fieldOfViewDegrees() const {
        return fieldOfViewDegrees_;
    }

    float Camera::orthographicHeight() const {
        return orthographicHeight_;
    }

    float Camera::nearPlane() const {
        return nearPlane_;
    }

    float Camera::farPlane() const {
        return farPlane_;
    }

    Mat4 Camera::viewMatrix() const {
        return Mat4::lookAt(position_, position_ + forward(), Vec3{ 0.0f, 1.0f, 0.0f });
    }

    Mat4 Camera::projectionMatrix( float aspectRatio ) const {
        if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0f)
            throw std::invalid_argument("Camera aspect ratio must be positive and finite");

        if (projectionMode_ == ProjectionMode::Perspective) {
            return Mat4::perspective(
                fieldOfViewDegrees_ * DegreesToRadians, aspectRatio, nearPlane_, farPlane_
            );
        }

        // OpenGL NDC: x/y/z in [-1, 1], right-handed camera space, looking down -Z.
        const float width = orthographicHeight_ * aspectRatio;
        const float depth = farPlane_ - nearPlane_;
        Mat4 result = Mat4::identity();
        result(0, 0) = 2.0f / width;
        result(1, 1) = 2.0f / orthographicHeight_;
        result(2, 2) = -2.0f / depth;
        result(2, 3) = -(farPlane_ + nearPlane_) / depth;
        return result;
    }
}
