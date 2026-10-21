
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "debug/DebugOverlayTelemetry.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    bool containsLine( const std::vector<std::string>& lines, const std::string& expected ) {
        for (const std::string& line : lines) {
            if (line == expected)
                return true;
        }

        return false;
    }

    void testEmptyTelemetry() {
        ve::DebugOverlayTelemetry telemetry;

        require(telemetry.frameSampleCount() == 0, "Initial history must be empty");
        require(telemetry.estimatedFps() == 0.0, "No history must yield zero FPS");

        std::cout << "[PASS] Empty telemetry is safe\n";
    }

    void testRollingFrameAverage() {
        ve::DebugOverlayTelemetry telemetry;
        ve::BenchmarkCpuTimings cpu{};
        ve::BenchmarkRenderStatistics render{};

        for (int i = 0; i < 60; ++i) {
            cpu.frameWallMs = 10.0;
            telemetry.recordFrame(cpu, render);
        }

        require(telemetry.frameSampleCount() == 60, "Expected 60-frame history");
        require(std::fabs(telemetry.estimatedFps() - 100.0) < 1e-8, "100 FPS expected");

        cpu.frameWallMs = 20.0;
        telemetry.recordFrame(cpu, render);

        require(telemetry.frameSampleCount() == 60, "History must stay at 60 samples");
        require(std::fabs(telemetry.averageFrameMs() - 610.0 / 60.0) < 1e-8,
                "Oldest sample must be replaced");

        std::cout << "[PASS] Rolling 60-frame time average and FPS\n";
    }

    void testGpuAndSoftwareFormatting() {
        ve::DebugOverlayTelemetry telemetry;
        ve::BenchmarkCpuTimings cpu{};
        ve::BenchmarkRenderStatistics render{};

        cpu.frameWallMs = 16.0;
        cpu.rendererWorkMs = 12.0;
        cpu.presentMs = 0.5;

        render.residentChunkCount = 4;
        render.meshDrawsSubmitted = 5;
        render.meshIndicesSubmitted = 17508;
        render.trianglesSubmitted = 5836;

        telemetry.recordFrame(cpu, render);

        ve::DebugOverlayScene scene{};
        scene.backend = ve::BenchmarkBackend::OpenGL;
        scene.cameraPosition = { 8.5f, 6.5f, 10.5f };
        scene.chunkPosition = { 0, 0, 0 };
        scene.targetBlock = "Grass (8, 3, 9)";

        auto lines = telemetry.makeLines(scene);

        require(lines.size() == 12, "Expected 12 overlay lines");
        require(containsLine(lines, "GPU RENDER: PENDING"), "GPU must initially be pending");
        require(containsLine(lines, "TARGET: Grass (8, 3, 9)"), "Missing block target");
        require(containsLine(lines, "XYZ: 8.50 / 6.50 / 10.50"), "Incorrect coordinates");
        require(containsLine(lines, "CHUNKS: 4  DRAWS: 5"), "Incorrect geometry summary");
        require(containsLine(lines, "TRIANGLES: 5836"), "Incorrect triangle count");

        ve::BenchmarkGpuTiming gpu{};
        gpu.state = ve::BenchmarkGpuTimingState::Available;
        gpu.elapsedMs = 0.125;

        telemetry.observeGpuTiming(gpu);

        lines = telemetry.makeLines(scene);

        require(containsLine(lines, "GPU RENDER: 0.12 MS") ||
                containsLine(lines, "GPU RENDER: 0.13 MS"), "GPU result must appear");

        scene.backend = ve::BenchmarkBackend::Software;
        lines = telemetry.makeLines(scene);

        require(containsLine(lines, "GPU RENDER: N/A"), "Software GPU timing must be N/A");

        std::cout << "[PASS] OpenGL and Software HUD data formatting\n";
    }

    void testInvalidTimingRejected() {
        ve::DebugOverlayTelemetry telemetry;
        ve::BenchmarkCpuTimings cpu{};
        ve::BenchmarkRenderStatistics render{};

        cpu.frameWallMs = -1.0;

        bool rejected = false;

        try {
            telemetry.recordFrame(cpu, render);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Negative frame timing must be rejected");
        require(telemetry.frameSampleCount() == 0, "Invalid frame must not enter history");

        std::cout << "[PASS] Invalid frame samples rejected\n";
    }
}


int main() {
    try {
        testEmptyTelemetry();
        testRollingFrameAverage();
        testGpuAndSoftwareFormatting();
        testInvalidTimingRejected();

        std::cout << "\nAll Debug Overlay Telemetry tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
