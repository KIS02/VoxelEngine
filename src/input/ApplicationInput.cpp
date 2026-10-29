#include "input/ApplicationInput.h"


namespace ve
{
    InputActions ApplicationInput::process( const InputSnapshot& snapshot, bool cursorCaptured,
                                            bool inventoryOpen ) {
        InputActions actions{};
        actions.movement = snapshot.movement;
        actions.breakHeld = snapshot.leftMouse && cursorCaptured && !inventoryOpen && !snapshot.escape;
        actions.toggleInventory = snapshot.toggleInventory && !previousInventory_;
        previousInventory_ = snapshot.toggleInventory;
        actions.dropSelected = snapshot.dropSelected && !previousDrop_ && !inventoryOpen;
        previousDrop_ = snapshot.dropSelected;
        actions.consumeFood = snapshot.consumeFood && !previousFood_ && !inventoryOpen;
        previousFood_ = snapshot.consumeFood;
        actions.hotbarNumber = snapshot.hotbarNumber;
        actions.hotbarScroll = snapshot.hotbarScroll;
        actions.cursorX = snapshot.cursorX;
        actions.cursorY = snapshot.cursorY;
        actions.shiftHeld = snapshot.shiftHeld;

        actions.toggleDebugOverlay = snapshot.debugOverlay && !previousDebugOverlay_;
        previousDebugOverlay_ = snapshot.debugOverlay;
        actions.toggleProjection = snapshot.toggleProjection && !previousProjection_;
        previousProjection_ = snapshot.toggleProjection;
        actions.toggleGameMode = snapshot.toggleGameMode && !previousGameMode_;
        previousGameMode_ = snapshot.toggleGameMode;
        actions.respawn = snapshot.respawn && !previousRespawn_;
        previousRespawn_ = snapshot.respawn;
        actions.toggleShadingModel = snapshot.toggleShadingModel && !previousShadingModel_;
        previousShadingModel_ = snapshot.toggleShadingModel;
        actions.cycleTextureSet = snapshot.cycleTextureSet && !previousTextureSet_;
        previousTextureSet_ = snapshot.cycleTextureSet;

        if (snapshot.escape && !previousEscape_) {
            actions.closeInventory = inventoryOpen;
            actions.releaseCursor = cursorCaptured && !inventoryOpen;
            actions.requestExit = !cursorCaptured && !inventoryOpen;
        }
        previousEscape_ = snapshot.escape;

        const bool leftJustPressed = snapshot.leftMouse && !previousLeftMouse_;
        const bool rightJustPressed = snapshot.rightMouse && !previousRightMouse_;

        if (inventoryOpen) {
            actions.inventoryLeft = leftJustPressed;
            actions.inventoryRight = rightJustPressed;
            actions.movement = CameraMovementInput{};
            actions.breakHeld = false;
            actions.toggleProjection = false;
            actions.toggleGameMode = false;
            actions.respawn = false;
            actions.consumeFood = false;
        } else if (!snapshot.escape && leftJustPressed) {
            if (!cursorCaptured)
                actions.captureCursor = true;
            else
                actions.breakBlock = true;
        }

        if (!inventoryOpen && !snapshot.escape && !leftJustPressed && rightJustPressed && cursorCaptured)
            actions.placeBlock = true;

        previousLeftMouse_ = snapshot.leftMouse;
        previousRightMouse_ = snapshot.rightMouse;
        return actions;
    }

    void ApplicationInput::reset() {
        previousEscape_ = false;
        previousDebugOverlay_ = false;
        previousProjection_ = false;
        previousGameMode_ = false;
        previousRespawn_ = false;
        previousShadingModel_ = false;
        previousTextureSet_ = false;
        previousInventory_ = false;
        previousDrop_ = false;
        previousFood_ = false;
        previousLeftMouse_ = false;
        previousRightMouse_ = false;
    }
}
