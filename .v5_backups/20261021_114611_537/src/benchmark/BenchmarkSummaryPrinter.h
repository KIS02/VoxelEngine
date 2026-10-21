
#pragma once

#include <cstddef>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "benchmark/BenchmarkFrameCollector.h"
#include "benchmark/BenchmarkStatistics.h"


namespace ve
{
    class BenchmarkSummaryPrinter {
        private:
            static BenchmarkTimeStatistics printMetric( std::ostream& output, const char* label,
                                                        const std::vector<double>& milliseconds ) {
                const BenchmarkTimeStatistics stats = BenchmarkStatisticsCalculator::calculate(milliseconds);

                output << std::left << std::setw(25) << label << std::right
                       << std::setw(11) << stats.meanMs
                       << std::setw(11) << stats.medianMs
                       << std::setw(11) << stats.p95Ms
                       << std::setw(11) << stats.p99Ms
                       << std::setw(11) << stats.minMs
                       << std::setw(11) << stats.maxMs << '\n';

                return stats;
            }


            static BenchmarkTimeStatistics printCpuMetric( std::ostream& output, const char* label,
                                                           const std::vector<BenchmarkFrameSample>& samples,
                                                           double BenchmarkCpuTimings::* member ) {
                std::vector<double> milliseconds;
                milliseconds.reserve(samples.size());

                for (const BenchmarkFrameSample& sample : samples)
                    milliseconds.push_back(sample.cpu.*member);

                return printMetric(output, label, milliseconds);
            }


        public:
            static void print( std::ostream& output, const BenchmarkFrameCollector& collector ) {
                if (!collector.allCpuFramesRecorded())
                    throw std::logic_error("Cannot summarize incomplete benchmark CPU samples");


                const BenchmarkRunConfiguration& configuration = collector.configuration();
                const std::vector<BenchmarkFrameSample>& samples = collector.samples();

                std::vector<double> gpuMilliseconds;
                gpuMilliseconds.reserve(samples.size());

                std::size_t pendingGpuCount = 0;
                std::size_t unsupportedGpuCount = 0;


                // Collect Only Valid GPU Measurements

                for (const BenchmarkFrameSample& sample : samples) {
                    if (!sample.isValid(configuration))
                        throw std::invalid_argument("Invalid benchmark sample in summary");

                    if (sample.gpu.state == BenchmarkGpuTimingState::Available)
                        gpuMilliseconds.push_back(sample.gpu.elapsedMs);
                    else if (sample.gpu.state == BenchmarkGpuTimingState::Pending)
                        ++pendingGpuCount;
                    else if (sample.gpu.state == BenchmarkGpuTimingState::Unsupported)
                        ++unsupportedGpuCount;
                }


                // Build Summary Outside the Measurement Interval

                std::ostringstream report;
                report << std::fixed << std::setprecision(4);


                // Configuration Summary

                report << "\n[Benchmark][Statistics]"
                       << " | Backend " << (configuration.backend == BenchmarkBackend::OpenGL ? "OpenGL" : "Software")
                       << " | Samples " << samples.size()
                       << " | Scene " << configuration.scenarioName
                       << '\n';

                report << "[Benchmark][Statistics]"
                       << " | Build " << configuration.buildConfiguration
                       << " | Resolution " << configuration.framebufferWidth
                       << 'x' << configuration.framebufferHeight
                       << '\n';


                // Statistics Table Header

                report << std::left << std::setw(25) << "Metric (ms)" << std::right
                       << std::setw(11) << "Mean"
                       << std::setw(11) << "Median"
                       << std::setw(11) << "P95"
                       << std::setw(11) << "P99"
                       << std::setw(11) << "Min"
                       << std::setw(11) << "Max"
                       << '\n';


                // CPU Timing Statistics

                const BenchmarkTimeStatistics frameStats = printCpuMetric(
                    report, "CPU Frame Wall", samples, &BenchmarkCpuTimings::frameWallMs
                );

                printCpuMetric(report, "Event Poll", samples, &BenchmarkCpuTimings::eventPollMs);
                printCpuMetric(report, "Input", samples, &BenchmarkCpuTimings::inputMs);
                printCpuMetric(report, "Update", samples, &BenchmarkCpuTimings::updateMs);
                printCpuMetric(report, "Mesh Rebuild", samples, &BenchmarkCpuTimings::meshRebuildMs);
                printCpuMetric(report, "Frame Assembly", samples, &BenchmarkCpuTimings::frameAssemblyMs);
                printCpuMetric(report, "CPU Renderer Work", samples, &BenchmarkCpuTimings::rendererWorkMs);
                printCpuMetric(report, "Present", samples, &BenchmarkCpuTimings::presentMs);


                // GPU Timing Statistics

                if (configuration.backend == BenchmarkBackend::OpenGL) {
                    if (!gpuMilliseconds.empty())
                        printMetric(report, "GPU Render", gpuMilliseconds);
                    else
                        report << "[Benchmark][Statistics] GPU Render: No available samples\n";

                    report << "[Benchmark][Statistics]"
                           << " | GPU Available " << gpuMilliseconds.size()
                           << " | Pending " << pendingGpuCount
                           << " | Unsupported " << unsupportedGpuCount
                           << '\n';
                } else {
                    report << "[Benchmark][Statistics] GPU Render: NotApplicable (Software)\n";
                }


                // FPS Estimate from Mean Frame Wall Time

                const std::optional<double> fps =
                    BenchmarkStatisticsCalculator::estimatedFramesPerSecond(frameStats);

                if (fps.has_value()) {
                    report << "[Benchmark][Statistics]"
                           << " Estimated FPS (from mean frame wall): " << *fps
                           << '\n';
                } else {
                    report << "[Benchmark][Statistics] Estimated FPS: Unavailable\n";
                }


                // Measurement Interpretation

                report << "[Benchmark][Statistics]"
                       << " Input and Update are disabled in fixed Benchmark Mode"
                       << '\n';

                report << "[Benchmark][Statistics]"
                       << " GPU Render is GPU query time, not CPU frame wall time"
                       << '\n';


                output << report.str();
            }
    };
}
