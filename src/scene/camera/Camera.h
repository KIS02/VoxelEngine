#pragma once

#include "math/Mat4.h"
#include "math/Vec3.h"


namespace ve
{
    class Camera {
        public:
            enum class ProjectionMode {
                Perspective,
                Orthographic
            };

        private:
            Vec3 position_{};
            float yawDegrees_ = -90.0f;
            float pitchDegrees_ = 0.0f;

            ProjectionMode projectionMode_ = ProjectionMode::Perspective;
            float fieldOfViewDegrees_ = 60.0f;
            float orthographicHeight_ = 10.0f;
            float nearPlane_ = 0.1f;
            float farPlane_ = 100.0f;


        public:
            Camera() = default;

            void setPosition( const Vec3& position );
            void setYawPitch( float yawDegrees, float pitchDegrees );
            void lookAt( const Vec3& target );

            const Vec3& position() const;
            float yawDegrees() const;
            float pitchDegrees() const;
            Vec3 forward() const;

            void setProjectionMode( ProjectionMode mode );
            ProjectionMode projectionMode() const;
            void setFieldOfViewDegrees( float degrees );
            void setOrthographicHeight( float height );
            void setClippingPlanes( float nearPlane, float farPlane );
            float fieldOfViewDegrees() const;
            float orthographicHeight() const;
            float nearPlane() const;
            float farPlane() const;

            Mat4 viewMatrix() const;
            Mat4 projectionMatrix( float aspectRatio ) const;
    };
}
