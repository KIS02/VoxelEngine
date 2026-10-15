
#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <ostream>
#include <stdexcept>
#include <string>

#include "benchmark/BenchmarkFrameCollector.h"


namespace ve
{
    class BenchmarkCsvWriter {
        private:
            static const char* backendName( BenchmarkBackend backend ) {
                switch (backend) {
                    case BenchmarkBackend::Software: return "Software";
                    case BenchmarkBackend::OpenGL: return "OpenGL";
                }

                throw std::invalid_argument("Unknown benchmark backend");
            }

            static const char* gpuStateName( BenchmarkGpuTimingState state ) {
                switch (state) {
                    case BenchmarkGpuTimingState::NotApplicable: return "NotApplicable";
                    case BenchmarkGpuTimingState::Pending: return "Pending";
                    case BenchmarkGpuTimingState::Available: return "Available";
                    case BenchmarkGpuTimingState::Unsupported: return "Unsupported";
                }

                throw std::invalid_argument("Unknown GPU timing state");
            }

            static void writeCsvText( std::ostream& output, const std::string& value ) {
                output << '"';

                for (char character : value) {
                    if (character == '"')
                        output << '"';

                    output << character;
                }

                output << '"';
            }


        public:
            static void write( std::ostream& output, const BenchmarkFrameCollector& collector ) {
                if (!collector.allCpuFramesRecorded())
                    throw std::logic_error("Cannot export incomplete benchmark samples");

                const BenchmarkRunConfiguration& configuration = collector.configuration();

                output.imbue(std::locale::classic());
                output << std::fixed << std::setprecision(9);

                output << "schema_version,backend,scenario,build,framebuffer_width,framebuffer_height,"
                       << "vsync_requested,crosshair_enabled,selection_outline_enabled,"
                       << "camera_x,camera_y,camera_z,forward_x,forward_y,forward_z,"
                       << "warmup_frames,measurement_frames,frame_id,"
                       << "frame_wall_ms,event_poll_ms,input_ms,update_ms,mesh_rebuild_ms,"
                       << "frame_assembly_ms,cpu_renderer_work_ms,present_ms,"
                       << "gpu_state,gpu_render_ms,resident_chunks,draw_calls,"
                       << "vertices_referenced,indices_submitted,triangles_submitted,"
                       << "mesh_rebuild_count,gpu_mesh_upload_count,gpu_mesh_release_count\n";

                for (const BenchmarkFrameSample& sample : collector.samples()) {
                    if (!sample.isValid(configuration))
                        throw std::invalid_argument("Cannot export invalid benchmark frame sample");

                    output << BenchmarkSchemaVersion << ',';
                    writeCsvText(output, backendName(configuration.backend));
                    output << ',';
                    writeCsvText(output, configuration.scenarioName);
                    output << ',';
                    writeCsvText(output, configuration.buildConfiguration);

                    output << ',' << configuration.framebufferWidth
                           << ',' << configuration.framebufferHeight << ',';

                    if (configuration.backend == BenchmarkBackend::OpenGL)
                        output << (configuration.vsyncEnabled ? 1 : 0);

                    output << ',' << (configuration.crosshairEnabled ? 1 : 0)
                           << ',' << (configuration.selectionOutlineEnabled ? 1 : 0)
                           << ',' << configuration.cameraPosition[0]
                           << ',' << configuration.cameraPosition[1]
                           << ',' << configuration.cameraPosition[2]
                           << ',' << configuration.cameraForward[0]
                           << ',' << configuration.cameraForward[1]
                           << ',' << configuration.cameraForward[2]
                           << ',' << configuration.warmupFrameCount
                           << ',' << configuration.measurementFrameCount
                           << ',' << sample.frameId
                           << ',' << sample.cpu.frameWallMs
                           << ',' << sample.cpu.eventPollMs
                           << ',' << sample.cpu.inputMs
                           << ',' << sample.cpu.updateMs
                           << ',' << sample.cpu.meshRebuildMs
                           << ',' << sample.cpu.frameAssemblyMs
                           << ',' << sample.cpu.rendererWorkMs
                           << ',' << sample.cpu.presentMs << ',';

                    writeCsvText(output, gpuStateName(sample.gpu.state));
                    output << ',';

                    if (sample.gpu.state == BenchmarkGpuTimingState::Available)
                        output << sample.gpu.elapsedMs;

                    output << ',' << sample.render.residentChunkCount
                           << ',' << sample.render.meshDrawsSubmitted
                           << ',' << sample.render.meshVerticesReferenced
                           << ',' << sample.render.meshIndicesSubmitted
                           << ',' << sample.render.trianglesSubmitted
                           << ',' << sample.render.meshRebuildCount
                           << ',' << sample.render.gpuMeshUploadCount
                           << ',' << sample.render.gpuMeshReleaseCount << '\n';
                }

                if (!output)
                    throw std::runtime_error("Failed to write benchmark CSV data");
            }


            static std::filesystem::path writeToDirectory( const BenchmarkFrameCollector& collector,
                                                           const std::filesystem::path& directory ) {
                if (!collector.allCpuFramesRecorded())
                    throw std::logic_error("Cannot export incomplete benchmark samples");

                std::filesystem::create_directories(directory);

                const char* backend = collector.configuration().backend == BenchmarkBackend::OpenGL
                    ? "opengl" : "software";

                const auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();

                const std::filesystem::path filename = std::string(backend) + "_" +
                    std::to_string(timestamp) + ".csv";

                const std::filesystem::path path = directory / filename;
                std::ofstream file(path);

                if (!file)
                    throw std::runtime_error("Failed to open benchmark CSV output file");

                write(file, collector);
                file.flush();

                if (!file)
                    throw std::runtime_error("Failed to save benchmark CSV output file");

                return path;
            }
    };
}
