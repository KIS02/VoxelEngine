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
        snapshot.cycleTextureSet = true;

        require(input.process(snapshot, true).cycleTextureSet, "First N press was not detected");
        require(!input.process(snapshot, true).cycleTextureSet, "Holding N repeated a press");
        require(!input.process(snapshot, false).cycleTextureSet, "Cursor state retriggered N");

        snapshot.cycleTextureSet = false;
        require(!input.process(snapshot, true).cycleTextureSet, "N release generated a press");
        snapshot.cycleTextureSet = true;
        require(input.process(snapshot, true).cycleTextureSet, "Second N press was not detected");

        input.reset();
        snapshot.toggleProjection = true;
        snapshot.toggleShadingModel = true;
        const ve::InputActions all = input.process(snapshot, true);
        require(all.cycleTextureSet && all.toggleProjection && all.toggleShadingModel,
                "N, P and M must be independent");
        require(!input.process(snapshot, true).cycleTextureSet,
                "Holding N with M+P should not repeat");

        input.reset();
        snapshot = ve::InputSnapshot{};
        snapshot.cycleTextureSet = true;
        require(input.process(snapshot, false).cycleTextureSet,
                "N must work when cursor is released");

        std::cout << "[PASS] N press edge, hold, release, reset, M/P independence\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
