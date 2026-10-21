#pragma once

#include "input/MouseTracker.h"
#include "scene/Camera.h"



namespace ve
{
    struct CameraMovementInput
    {
        bool forward = false;
        bool backward = false;

        bool left = false;
        bool right = false;

        bool up = false;
        bool down = false;
    };



    class CameraController {
        private:
            float moveSpeed_ = 8.0f;
            float mouseSensitivity_ = 0.1f;


        public:
            void setMoveSpeed( float unitsPerSecond );
            float moveSpeed() const;

            void setMouseSensitivity( float degreesPerPixel );
            float mouseSensitivity() const;

            void update( Camera& camera, const CameraMovementInput& input, float deltaTime ) const;

            void updateLook( Camera& camera, const MouseDelta& mouseDelta ) const;
    };
}