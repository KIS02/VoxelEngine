
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkTypes.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testStopwatchInitialState() {
        ve::BenchmarkCpuStopwatch timer;

        require(!timer.isRunning(), "Stopwatch must be stopped initially");
        require(timer.elapsedMs() == 0.0, "Initial elapsed time must be zero");

        std::cout << "[PASS] Stopwatch initial state\n";
    }


    void testStartStopAndAccumulation() {
        ve::BenchmarkCpuStopwatch timer;

        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        require(timer.isRunning(), "Stopwatch must be running");
        require(timer.elapsedMs() > 0.0, "Running timer must report elapsed time");
        timer.stop();

        const double first = timer.elapsedMs();
        require(first > 0.0, "Stopped timer must preserve elapsed time");

        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        timer.stop();

        require(timer.elapsedMs() > first, "Multiple intervals must accumulate");
        require(!timer.isRunning(), "Stopwatch must stop after stop()");

        std::cout << "[PASS] Stopwatch start, stop, and accumulation\n";
    }


    void testReset() {
        ve::BenchmarkCpuStopwatch timer;

        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        timer.reset();

        require(!timer.isRunning(), "Reset must stop the stopwatch");
        require(timer.elapsedMs() == 0.0, "Reset must clear elapsed time");

        std::cout << "[PASS] Stopwatch reset\n";
    }


    void testInvalidTransitions() {
        ve::BenchmarkCpuStopwatch timer;
        bool stopRejected = false;
        bool startRejected = false;

        try {
            timer.stop();
        } catch (const std::logic_error&) {
            stopRejected = true;
        }

        timer.start();

        try {
            timer.start();
        } catch (const std::logic_error&) {
            startRejected = true;
        }

        timer.stop();

        require(stopRejected, "Stop without start must be rejected");
        require(startRejected, "Repeated start must be rejected");

        std::cout << "[PASS] Stopwatch invalid transitions\n";
    }


    void testScopeTimerAccumulation() {
        double elapsedMs = 0.0;

        {
            ve::BenchmarkCpuScope scope(elapsedMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        const double first = elapsedMs;
        require(first > 0.0, "Scope timer must record time on exit");

        {
            ve::BenchmarkCpuScope scope(elapsedMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        require(elapsedMs > first, "Repeated scopes must accumulate");

        std::cout << "[PASS] Scoped timer accumulation\n";
    }


    void testNestedScopes() {
        double frameWallMs = 0.0;
        double updateMs = 0.0;

        {
            ve::BenchmarkCpuScope frameScope(frameWallMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));

            {
                ve::BenchmarkCpuScope updateScope(updateMs);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        require(updateMs > 0.0, "Nested update timing must be recorded");
        require(frameWallMs >= updateMs, "Frame wall time must include nested update time");

        std::cout << "[PASS] Nested CPU timing scopes\n";
    }


    void testScopeExceptionSafety() {
        double elapsedMs = 0.0;
        bool caught = false;

        try {
            ve::BenchmarkCpuScope scope(elapsedMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            throw std::runtime_error("Expected test exception");
        } catch (const std::runtime_error&) {
            caught = true;
        }

        require(caught, "Expected exception was not caught");
        require(elapsedMs > 0.0, "Scope must record time during stack unwinding");

        std::cout << "[PASS] Scoped timer exception safety\n";
    }


    void testBenchmarkContractIntegration() {
        ve::BenchmarkFrameSample sample{};
        sample.frameId = 1;

        {
            ve::BenchmarkCpuScope frameScope(sample.cpu.frameWallMs);

            {
                ve::BenchmarkCpuScope updateScope(sample.cpu.updateMs);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }

        ve::BenchmarkRunConfiguration configuration{};
        configuration.backend = ve::BenchmarkBackend::Software;
        configuration.scenarioName = "cpu_timer_unit_test";
        configuration.buildConfiguration = "Debug";
        configuration.framebufferWidth = 800;
        configuration.framebufferHeight = 600;

        require(sample.cpu.updateMs > 0.0, "Update time must be recorded");
        require(sample.cpu.frameWallMs >= sample.cpu.updateMs, "Frame time must include update time");
        require(sample.isValid(configuration), "Timed sample must satisfy Phase 7.1 contract");

        std::cout << "[PASS] Phase 7.1 BenchmarkFrameSample integration\n";
    }
}


int main() {
    try {
        testStopwatchInitialState();
        testStartStopAndAccumulation();
        testReset();
        testInvalidTransitions();
        testScopeTimerAccumulation();
        testNestedScopes();
        testScopeExceptionSafety();
        testBenchmarkContractIntegration();

        std::cout << "\nAll Benchmark CPU Timer tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "[FAIL] " << exception.what() << '\n';
        return 1;
    }
}
