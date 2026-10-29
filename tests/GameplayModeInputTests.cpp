#include <iostream>
#include <stdexcept>

#include "input/ApplicationInput.h"


namespace
{
    void require( bool value, const char* message ) {
        if (!value)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::ApplicationInput input;
        ve::InputSnapshot snapshot{};
        snapshot.toggleGameMode = true;
        snapshot.toggleProjection = true;
        ve::InputActions action = input.process(snapshot, true);
        require(action.toggleGameMode && action.toggleProjection, "G/P simultaneous input failed");
        action = input.process(snapshot, true);
        require(!action.toggleGameMode && !action.toggleProjection, "G/P repeat triggered while held");
        snapshot.toggleGameMode = false;
        snapshot.toggleProjection = false;
        input.process(snapshot, true);
        snapshot.toggleGameMode = true;
        action = input.process(snapshot, false);
        require(action.toggleGameMode, "G key failed when cursor released");
        input.reset();
        require(input.process(snapshot, true).toggleGameMode, "Input reset retained stale G key state");
        std::cout << "[PASS] G gameplay mode toggle rising edge, independence from P and reset\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
