#pragma once

#include "math/Mat4.h"
#include "math/Vec3.h"



namespace ve
{
    class Camera {
        private:
            Vec3 position_{};

            float yawDegrees_ = -90.0f;
            float pitchDegrees_ = 0.0f;


        public:
            Camera() = default;


            void setPosition( const Vec3& position );
            void setYawPitch( float yawDegrees, float pitchDegrees );

            void lookAt( const Vec3& target );


            const Vec3& position() const;

            float yawDegrees() const;
            float pitchDegrees() const;


            Vec3 forward() const;

            Mat4 viewMatrix() const;
    };
}