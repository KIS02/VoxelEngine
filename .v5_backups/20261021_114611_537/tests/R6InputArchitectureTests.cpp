#include <stdexcept>

#include "input/ApplicationInput.h"


namespace
{
    void require( bool condition ) {
        if (!condition)
            throw std::runtime_error("R6 input architecture contract failed");
    }
}


int main() {
    ve::ApplicationInput input{};
    ve::InputSnapshot state{};

    // Press edges generate one action, holding and release generate none.
    state.leftMouse = true;
    require(input.process(state, true).breakBlock);
    require(!input.process(state, true).breakBlock);
    state.leftMouse = false;
    require(!input.process(state, true).breakBlock);

    // The first click with a free cursor captures it without editing the world.
    state.leftMouse = true;
    const ve::InputActions capture = input.process(state, false);
    require(capture.captureCursor && !capture.breakBlock && !capture.placeBlock);
    state.leftMouse = false;
    input.process(state, true);

    // Right click places a block only in captured mode.
    state.rightMouse = true;
    require(input.process(state, true).placeBlock);
    require(!input.process(state, true).placeBlock);
    state.rightMouse = false;
    input.process(state, true);
    state.rightMouse = true;
    require(!input.process(state, false).placeBlock);

    // Left press has priority when both buttons are pressed together.
    input.reset();
    state = ve::InputSnapshot{};
    state.leftMouse = true;
    state.rightMouse = true;
    const ve::InputActions simultaneous = input.process(state, true);
    require(simultaneous.breakBlock && !simultaneous.placeBlock);

    // Escape suppresses editing, first releases cursor, next press requests exit.
    input.reset();
    state = ve::InputSnapshot{};
    state.escape = true;
    state.leftMouse = true;
    require(input.process(state, true).releaseCursor);
    require(!input.process(state, true).releaseCursor);
    state.escape = false;
    state.leftMouse = false;
    input.process(state, false);
    state.escape = true;
    const ve::InputActions exit = input.process(state, false);
    require(exit.requestExit && !exit.breakBlock && !exit.placeBlock);

    // F3 toggles on the press edge, movement snapshot is preserved verbatim.
    input.reset();
    state = ve::InputSnapshot{};
    state.debugOverlay = true;
    state.movement.forward = true;
    state.movement.up = true;
    const ve::InputActions firstToggle = input.process(state, true);
    require(firstToggle.toggleDebugOverlay && firstToggle.movement.forward && firstToggle.movement.up);
    require(!input.process(state, true).toggleDebugOverlay);
    state.debugOverlay = false;
    input.process(state, true);
    state.debugOverlay = true;
    require(input.process(state, true).toggleDebugOverlay);
    return 0;
}
