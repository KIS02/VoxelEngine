#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "benchmark/BenchmarkTypes.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    ve::BenchmarkRunConfiguration createConfiguration( ve::BenchmarkBackend backend ) {
        ve::BenchmarkRunConfiguration configuration{};

        configuration.backend = backend;
        configuration.scenarioName = "four_chunk_reference_world";
        configuration.buildConfiguration = "Release";
        configuration.framebufferWidth = 800;
        configuration.framebufferHeight = 600;
        configuration.vsyncEnabled = false;
        configuration.crosshairEnabled = false;
        configuration.selectionOutlineEnabled = false;
        configuration.cameraPosition = { 8.5f, 6.5f, 10.5f };
        configuration.cameraForward = { 0.0f, -0.8f, -0.6f };
        configuration.warmupFrameCount = 120;
        configuration.measurementFrameCount = 600;

        return configuration;
    }


    ve::BenchmarkFrameSample createSoftwareSample() {
        ve::BenchmarkFrameSample sample{};

        sample.frameId = 42;
        sample.cpu.frameWallMs = 16.0;
        sample.cpu.eventPollMs = 0.1;
        sample.cpu.inputMs = 0.1;
        sample.cpu.updateMs = 0.2;
        sample.cpu.meshRebuildMs = 0.0;
        sample.cpu.frameAssemblyMs = 0.3;
        sample.cpu.rendererWorkMs = 12.0;
        sample.cpu.presentMs = 2.0;

        sample.gpu.state = ve::BenchmarkGpuTimingState::NotApplicable;

        sample.render.residentChunkCount = 4;
        sample.render.meshDrawsSubmitted = 2;
        sample.render.meshVerticesReferenced = 48;
        sample.render.meshIndicesSubmitted = 60;
        sample.render.trianglesSubmitted = 20;

        return sample;
    }


    void testDefaultConfigurationIsInvalid() {
        ve::BenchmarkRunConfiguration configuration{};

        require(!configuration.isValid(), "Unconfigured benchmark run must be invalid");
        require(ve::BenchmarkSchemaVersion == 1, "Unexpected benchmark schema version");

        std::cout << "[PASS] Benchmark contract version and required configuration\n";
    }


    void testValidRunConfiguration() {
        const auto software = createConfiguration(ve::BenchmarkBackend::Software);
        const auto opengl = createConfiguration(ve::BenchmarkBackend::OpenGL);

        require(software.isValid(), "Software run configuration must be valid");
        require(opengl.isValid(), "OpenGL run configuration must be valid");
        require(software.framebufferWidth == opengl.framebufferWidth, "Resolution mismatch");
        require(software.cameraPosition == opengl.cameraPosition, "Camera position mismatch");
        require(software.cameraForward == opengl.cameraForward, "Camera direction mismatch");

        std::cout << "[PASS] Shared scene and camera run configuration\n";
    }


    void testInvalidResolutionAndDuration() {
        auto configuration = createConfiguration(ve::BenchmarkBackend::Software);

        configuration.framebufferWidth = 0;
        require(!configuration.isValid(), "Zero framebuffer width must fail");

        configuration.framebufferWidth = 800;
        configuration.framebufferHeight = -1;
        require(!configuration.isValid(), "Negative framebuffer height must fail");

        configuration.framebufferHeight = 600;
        configuration.warmupFrameCount = 0;
        require(!configuration.isValid(), "Zero warmup frames must fail");

        configuration.warmupFrameCount = 120;
        configuration.measurementFrameCount = 0;
        require(!configuration.isValid(), "Zero measured frames must fail");

        std::cout << "[PASS] Invalid resolution and run length rejected\n";
    }


    void testInvalidSceneAndCamera() {
        auto configuration = createConfiguration(ve::BenchmarkBackend::Software);

        configuration.scenarioName.clear();
        require(!configuration.isValid(), "Missing scenario name must fail");

        configuration.scenarioName = "demo";
        configuration.buildConfiguration.clear();
        require(!configuration.isValid(), "Missing build configuration must fail");

        configuration.buildConfiguration = "Release";
        configuration.cameraForward = { 0.0f, 0.0f, 0.0f };
        require(!configuration.isValid(), "Zero camera direction must fail");

        configuration.cameraForward = { 0.0f, 0.0f, -1.0f };
        configuration.cameraPosition[0] = std::numeric_limits<float>::quiet_NaN();
        require(!configuration.isValid(), "NaN camera position must fail");

        std::cout << "[PASS] Missing scene metadata and invalid camera rejected\n";
    }


    void testValidSoftwareSample() {
        const auto configuration = createConfiguration(ve::BenchmarkBackend::Software);
        const auto sample = createSoftwareSample();

        require(sample.isValid(configuration), "Valid Software frame sample rejected");
        require(sample.render.trianglesSubmitted == 20, "Submitted triangle count changed");
        require(sample.frameId == 42, "Frame ID was not retained");

        std::cout << "[PASS] Valid Software frame sample\n";
    }


    void testCpuTimingRejectsNegativeAndNonfinite() {
        auto sample = createSoftwareSample();
        const auto configuration = createConfiguration(ve::BenchmarkBackend::Software);

        sample.cpu.rendererWorkMs = -0.01;
        require(!sample.isValid(configuration), "Negative CPU time must fail");

        sample.cpu.rendererWorkMs = std::numeric_limits<double>::infinity();
        require(!sample.isValid(configuration), "Infinite CPU time must fail");

        sample.cpu.rendererWorkMs = 12.0;
        sample.cpu.presentMs = std::numeric_limits<double>::quiet_NaN();
        require(!sample.isValid(configuration), "NaN present time must fail");

        std::cout << "[PASS] Invalid CPU measurements rejected\n";
    }


    void testTriangleStatisticsConsistency() {
        auto sample = createSoftwareSample();
        const auto configuration = createConfiguration(ve::BenchmarkBackend::Software);

        sample.render.meshIndicesSubmitted = 61;
        require(!sample.isValid(configuration), "Non-triangle index count must fail");

        sample.render.meshIndicesSubmitted = 60;
        sample.render.trianglesSubmitted = 19;
        require(!sample.isValid(configuration), "Triangle count mismatch must fail");

        sample.render.trianglesSubmitted = 20;
        require(sample.isValid(configuration), "Restored triangle statistics must pass");

        std::cout << "[PASS] Submitted index and triangle counts agree\n";
    }


    void testPendingGpuTiming() {
        const auto configuration = createConfiguration(ve::BenchmarkBackend::OpenGL);
        auto sample = createSoftwareSample();

        sample.gpu.state = ve::BenchmarkGpuTimingState::Pending;
        require(sample.isValid(configuration), "Pending GPU measurement must be valid");

        sample.gpu.elapsedMs = 2.5;
        require(!sample.isValid(configuration), "Pending GPU measurement must not expose a value");

        sample.gpu.state = ve::BenchmarkGpuTimingState::Available;
        require(sample.isValid(configuration), "Available GPU measurement must be valid");

        sample.gpu.elapsedMs = -2.0;
        require(!sample.isValid(configuration), "Negative GPU duration must fail");

        std::cout << "[PASS] Pending and available GPU timings distinguished\n";
    }


    void testBackendSpecificGpuTiming() {
        auto sample = createSoftwareSample();
        const auto software = createConfiguration(ve::BenchmarkBackend::Software);
        const auto opengl = createConfiguration(ve::BenchmarkBackend::OpenGL);

        sample.gpu.state = ve::BenchmarkGpuTimingState::Available;
        sample.gpu.elapsedMs = 1.25;
        require(!sample.isValid(software), "Software backend cannot report GPU time");
        require(sample.isValid(opengl), "OpenGL backend should accept GPU time");

        sample.gpu.state = ve::BenchmarkGpuTimingState::Unsupported;
        sample.gpu.elapsedMs = 0.0;
        require(sample.isValid(opengl), "Unavailable OpenGL timing should be distinguishable");
        require(!sample.isValid(software), "Software timing state must be NotApplicable");

        std::cout << "[PASS] Software and OpenGL timing states are separated\n";
    }


    void testGpuResourceCounters() {
        const auto software = createConfiguration(ve::BenchmarkBackend::Software);
        const auto opengl = createConfiguration(ve::BenchmarkBackend::OpenGL);
        auto sample = createSoftwareSample();

        sample.render.gpuMeshUploadCount = 1;
        require(!sample.isValid(software), "Software sample cannot claim GPU uploads");

        sample.render.gpuMeshReleaseCount = 1;
        require(!sample.isValid(software), "Software sample cannot claim GPU releases");

        sample.gpu.state = ve::BenchmarkGpuTimingState::Pending;
        require(sample.isValid(opengl), "OpenGL GPU resource events must be accepted");

        require(sample.isValid(opengl), "GPU resource events must not invalidate timing data");

        std::cout << "[PASS] Backend GPU resource events are recorded consistently\n";
    }
}


int main() {
    try {
        testDefaultConfigurationIsInvalid();
        testValidRunConfiguration();
        testInvalidResolutionAndDuration();
        testInvalidSceneAndCamera();
        testValidSoftwareSample();
        testCpuTimingRejectsNegativeAndNonfinite();
        testTriangleStatisticsConsistency();
        testPendingGpuTiming();
        testBackendSpecificGpuTiming();
        testGpuResourceCounters();

        std::cout << "\nAll Benchmark Contract tests passed.\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
