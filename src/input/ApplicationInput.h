#pragma once

#include "scene/camera/CameraController.h"


namespace ve
{
    struct InputSnapshot
    {
        bool escape = false;
        bool debugOverlay = false;
        bool toggleProjection = false;
        bool toggleShadingModel = false;
        bool cycleTextureSet = false;
        bool leftMouse = false;
        bool rightMouse = false;
        CameraMovementInput movement{};
    };


    struct InputActions
    {
        bool releaseCursor = false;
        bool captureCursor = false;
        bool requestExit = false;
        bool toggleDebugOverlay = false;
        bool toggleProjection = false;
        bool toggleShadingModel = false;
        bool cycleTextureSet = false;
        bool breakBlock = false;
        bool placeBlock = false;
        CameraMovementInput movement{};
    };


    class ApplicationInput {
        private:
            bool previousEscape_ = false;
            bool previousDebugOverlay_ = false;
            bool previousProjection_ = false;
            bool previousShadingModel_ = false;
            bool previousTextureSet_ = false;
            bool previousLeftMouse_ = false;
            bool previousRightMouse_ = false;


        public:
            InputActions process( const InputSnapshot& snapshot, bool cursorCaptured );

            void reset();
    };
}
