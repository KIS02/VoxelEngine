#pragma once

#include "scene/camera/CameraController.h"


namespace ve
{
    struct InputSnapshot
    {
        bool escape = false;
        bool debugOverlay = false;
        bool toggleProjection = false;
        bool toggleGameMode = false;
        bool respawn = false;
        bool toggleShadingModel = false;
        bool cycleTextureSet = false;
        bool leftMouse = false;
        bool breakHeld = false;
        bool toggleInventory = false;
        bool dropSelected = false;
        bool consumeFood = false;
        int hotbarNumber = -1;
        int hotbarScroll = 0;
        int cursorX = -1;
        int cursorY = -1;
        bool shiftHeld = false;
        bool rightMouse = false;
        CameraMovementInput movement{};
    };


    struct InputActions
    {
        bool releaseCursor = false;
        bool captureCursor = false;
        bool requestExit = false;
        bool toggleInventory = false;
        bool closeInventory = false;
        bool dropSelected = false;
        bool consumeFood = false;
        int hotbarNumber = -1;
        int hotbarScroll = 0;
        int cursorX = -1;
        int cursorY = -1;
        bool shiftHeld = false;
        bool inventoryLeft = false;
        bool inventoryRight = false;
        bool toggleDebugOverlay = false;
        bool toggleProjection = false;
        bool toggleGameMode = false;
        bool respawn = false;
        bool toggleShadingModel = false;
        bool cycleTextureSet = false;
        bool breakBlock = false;
        bool breakHeld = false;
        bool placeBlock = false;
        CameraMovementInput movement{};
    };


    class ApplicationInput {
        private:
            bool previousEscape_ = false;
            bool previousDebugOverlay_ = false;
            bool previousProjection_ = false;
            bool previousGameMode_ = false;
            bool previousRespawn_ = false;
            bool previousShadingModel_ = false;
            bool previousTextureSet_ = false;
            bool previousInventory_ = false;
            bool previousDrop_ = false;
            bool previousFood_ = false;
            bool previousLeftMouse_ = false;
            bool previousRightMouse_ = false;


        public:
            InputActions process( const InputSnapshot& snapshot, bool cursorCaptured,
                                  bool inventoryOpen = false );

            void reset();
    };
}
