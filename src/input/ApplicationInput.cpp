#include "input/ApplicationInput.h"


namespace ve
{
    InputActions ApplicationInput::process( const InputSnapshot& snapshot, bool cursorCaptured ) {
        InputActions actions{};
        actions.movement = snapshot.movement;

        actions.toggleDebugOverlay = snapshot.debugOverlay && !previousDebugOverlay_;
        previousDebugOverlay_ = snapshot.debugOverlay;
        actions.toggleProjection = snapshot.toggleProjection && !previousProjection_;
        previousProjection_ = snapshot.toggleProjection;
        actions.toggleShadingModel = snapshot.toggleShadingModel && !previousShadingModel_;
        previousShadingModel_ = snapshot.toggleShadingModel;
        actions.cycleTextureSet = snapshot.cycleTextureSet && !previousTextureSet_;
        previousTextureSet_ = snapshot.cycleTextureSet;

        if (snapshot.escape && !previousEscape_) {
            actions.releaseCursor = cursorCaptured;
            actions.requestExit = !cursorCaptured;
        }
        previousEscape_ = snapshot.escape;

        const bool leftJustPressed = snapshot.leftMouse && !previousLeftMouse_;
        const bool rightJustPressed = snapshot.rightMouse && !previousRightMouse_;

        if (!snapshot.escape && leftJustPressed) {
            if (!cursorCaptured)
                actions.captureCursor = true;
            else
                actions.breakBlock = true;
        }

        if (!snapshot.escape && !leftJustPressed && rightJustPressed && cursorCaptured)
            actions.placeBlock = true;

        previousLeftMouse_ = snapshot.leftMouse;
        previousRightMouse_ = snapshot.rightMouse;
        return actions;
    }

    void ApplicationInput::reset() {
        previousEscape_ = false;
        previousDebugOverlay_ = false;
        previousProjection_ = false;
        previousShadingModel_ = false;
        previousTextureSet_ = false;
        previousLeftMouse_ = false;
        previousRightMouse_ = false;
    }
}
