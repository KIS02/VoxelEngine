#include <iostream>
#include <stdexcept>

#include "input/ApplicationInput.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::ApplicationInput input{};
        ve::InputSnapshot snapshot{};
        snapshot.toggleShadingModel = true;
        require(input.process(snapshot, true).toggleShadingModel,
                "M must toggle on the first press");
        require(!input.process(snapshot, true).toggleShadingModel,
                "Holding M must not toggle repeatedly");
        require(!input.process(snapshot, false).toggleShadingModel,
                "Cursor release while holding M must not generate a new press");

        snapshot.toggleShadingModel = false;
        require(!input.process(snapshot, true).toggleShadingModel,
                "Releasing M must not toggle");
        snapshot.toggleShadingModel = true;
        require(input.process(snapshot, true).toggleShadingModel,
                "Second M press must toggle again");

        input.reset();
        snapshot = ve::InputSnapshot{};
        snapshot.toggleProjection = true;
        snapshot.toggleShadingModel = true;
        const ve::InputActions both = input.process(snapshot, true);
        require(both.toggleProjection && both.toggleShadingModel,
                "M and P must generate independent actions");
        require(!input.process(snapshot, true).toggleProjection &&
                !input.process(snapshot, true).toggleShadingModel,
                "Holding M+P must not repeat either toggle");

        snapshot.toggleProjection = false;
        snapshot.toggleShadingModel = false;
        input.process(snapshot, true);
        snapshot.toggleProjection = true;
        require(input.process(snapshot, true).toggleProjection,
                "Projection must work without a Shading toggle");
        require(!input.process(snapshot, true).toggleShadingModel,
                "P must never generate a Shading toggle");

        input.reset();
        snapshot = ve::InputSnapshot{};
        snapshot.toggleShadingModel = true;
        require(input.process(snapshot, false).toggleShadingModel,
                "M must use the same edge contract with cursor released");

        std::cout << "[PASS] M press edge, hold, release, reset and independent P key\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
