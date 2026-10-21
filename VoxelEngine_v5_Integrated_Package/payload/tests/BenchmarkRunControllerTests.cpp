
#include <iostream>
#include <stdexcept>
#include <string>

#include "benchmark/BenchmarkRunController.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    ve::BenchmarkRunConfiguration testConfiguration() {
        ve::BenchmarkRunConfiguration configuration{};
        configuration.backend = ve::BenchmarkBackend::Software;
        configuration.scenarioName = "fixed_voxel_scene";
        configuration.buildConfiguration = "Release";
        configuration.framebufferWidth = 800;
        configuration.framebufferHeight = 600;
        configuration.warmupFrameCount = 3;
        configuration.measurementFrameCount = 4;
        return configuration;
    }

    void testInitialStateAndValidation() {
        ve::BenchmarkRunController controller;

        require(controller.phase() == ve::BenchmarkRunPhase::Idle, "Initial phase must be Idle");
        require(!controller.isRunning(), "Idle benchmark must not be running");
        require(!controller.shouldRecordCurrentFrame(), "Idle benchmark must not record samples");

        bool invalidRejected = false;

        try {
            controller.begin(ve::BenchmarkRunConfiguration{});
        } catch (const std::invalid_argument&) {
            invalidRejected = true;
        }

        require(invalidRejected, "Invalid configuration must be rejected");
        require(controller.phase() == ve::BenchmarkRunPhase::Idle, "Invalid start must preserve Idle state");

        std::cout << "[PASS] Initial state and invalid configuration\n";
    }

    void testWarmupBoundary() {
        ve::BenchmarkRunController controller;
        controller.begin(testConfiguration());

        for (int i = 0; i < 3; ++i) {
            require(controller.phase() == ve::BenchmarkRunPhase::Warmup, "Expected Warmup before commit");
            require(!controller.shouldRecordCurrentFrame(), "Warmup frame must not be recorded");
            controller.commitFrame();
        }

        require(controller.warmupFramesCompleted() == 3, "Expected exactly three warmup frames");
        require(controller.measurementFramesCompleted() == 0, "Warmup must not count measurements");
        require(controller.phase() == ve::BenchmarkRunPhase::Measurement, "Expected Measurement after warmup");

        std::cout << "[PASS] Exact warmup boundary\n";
    }

    void testMeasurementBoundary() {
        ve::BenchmarkRunController controller;
        controller.begin(testConfiguration());

        for (int i = 0; i < 3; ++i)
            controller.commitFrame();

        for (int i = 0; i < 4; ++i) {
            require(controller.shouldRecordCurrentFrame(), "Measurement frame must be recorded");
            controller.commitFrame();
        }

        require(controller.isCompleted(), "Expected Completed after final sample");
        require(!controller.isRunning(), "Completed benchmark must not run");
        require(!controller.shouldRecordCurrentFrame(), "Completed benchmark must not record more samples");
        require(controller.measurementFramesCompleted() == 4, "Expected exactly four measurement frames");
        require(controller.totalFramesCompleted() == 7, "Expected exactly seven committed frames");

        std::cout << "[PASS] Exact measurement boundary and completion\n";
    }

    void testInvalidTransitions() {
        ve::BenchmarkRunController controller;
        bool idleCommitRejected = false;
        bool nestedBeginRejected = false;
        bool completedCommitRejected = false;

        try {
            controller.commitFrame();
        } catch (const std::logic_error&) {
            idleCommitRejected = true;
        }

        controller.begin(testConfiguration());

        try {
            controller.begin(testConfiguration());
        } catch (const std::logic_error&) {
            nestedBeginRejected = true;
        }

        for (int i = 0; i < 7; ++i)
            controller.commitFrame();

        try {
            controller.commitFrame();
        } catch (const std::logic_error&) {
            completedCommitRejected = true;
        }

        require(idleCommitRejected, "Commit before begin must be rejected");
        require(nestedBeginRejected, "Starting a second active run must be rejected");
        require(completedCommitRejected, "Commit after completion must be rejected");

        std::cout << "[PASS] Illegal state transitions\n";
    }

    void testCommitOnlyAfterSuccessfulFrame() {
        ve::BenchmarkRunController controller;
        controller.begin(testConfiguration());

        require(controller.totalFramesCompleted() == 0, "No frames should be committed yet");
        require(controller.phase() == ve::BenchmarkRunPhase::Warmup, "Must remain Warmup without commit");

        controller.commitFrame();

        require(controller.totalFramesCompleted() == 1, "One frame should be committed");
        require(controller.phase() == ve::BenchmarkRunPhase::Warmup, "One commit should not finish warmup");

        std::cout << "[PASS] Frames advance only when committed\n";
    }

    void testConfigurationIsCopied() {
        ve::BenchmarkRunConfiguration configuration = testConfiguration();
        ve::BenchmarkRunController controller;
        controller.begin(configuration);

        configuration.scenarioName = "changed_after_begin";
        configuration.warmupFrameCount = 99;

        require(controller.configuration().scenarioName == "fixed_voxel_scene", "Run must own its configuration");
        require(controller.configuration().warmupFrameCount == 3, "Run must retain original warmup count");

        std::cout << "[PASS] Immutable run configuration snapshot\n";
    }

    void testResetAndRestart() {
        ve::BenchmarkRunController controller;
        controller.begin(testConfiguration());
        controller.commitFrame();
        controller.reset();

        require(controller.phase() == ve::BenchmarkRunPhase::Idle, "Reset must return to Idle");
        require(controller.totalFramesCompleted() == 0, "Reset must clear counters");

        ve::BenchmarkRunConfiguration configuration = testConfiguration();
        configuration.warmupFrameCount = 1;
        configuration.measurementFrameCount = 1;
        controller.begin(configuration);

        require(!controller.shouldRecordCurrentFrame(), "First frame must warm up");
        controller.commitFrame();
        require(controller.shouldRecordCurrentFrame(), "Second frame must be measured");
        controller.commitFrame();
        require(controller.isCompleted(), "Second frame must complete run");

        std::cout << "[PASS] Reset and short-run restart\n";
    }
}


int main() {
    try {
        testInitialStateAndValidation();
        testWarmupBoundary();
        testMeasurementBoundary();
        testInvalidTransitions();
        testCommitOnlyAfterSuccessfulFrame();
        testConfigurationIsCopied();
        testResetAndRestart();

        std::cout << "\nAll Benchmark Run Controller tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
