
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <vector>


namespace ve
{
    struct BenchmarkTimeStatistics
    {
        std::size_t sampleCount = 0;

        double minMs = 0.0;
        double maxMs = 0.0;
        double meanMs = 0.0;
        double medianMs = 0.0;
        double p95Ms = 0.0;
        double p99Ms = 0.0;
    };


    class BenchmarkStatisticsCalculator {
        private:
            static double percentile( const std::vector<double>& sortedValues, double fraction ) {
                const double position = static_cast<double>(sortedValues.size() - 1) * fraction;
                const std::size_t lower = static_cast<std::size_t>(position);
                const std::size_t upper = std::min(lower + 1, sortedValues.size() - 1);
                const double weight = position - static_cast<double>(lower);

                return sortedValues[lower] + (sortedValues[upper] - sortedValues[lower]) * weight;
            }


        public:
            static BenchmarkTimeStatistics calculate( const std::vector<double>& milliseconds ) {
                if (milliseconds.empty())
                    throw std::invalid_argument("Benchmark statistics require at least one sample");

                for (double value : milliseconds) {
                    if (!std::isfinite(value) || value < 0.0)
                        throw std::invalid_argument("Benchmark timing samples must be finite and nonnegative");
                }

                std::vector<double> sortedValues = milliseconds;
                std::sort(sortedValues.begin(), sortedValues.end());

                const long double sum = std::accumulate(
                    sortedValues.begin(), sortedValues.end(), 0.0L
                );

                BenchmarkTimeStatistics result{};
                result.sampleCount = sortedValues.size();
                result.minMs = sortedValues.front();
                result.maxMs = sortedValues.back();
                result.meanMs = static_cast<double>(sum / sortedValues.size());
                result.medianMs = percentile(sortedValues, 0.50);
                result.p95Ms = percentile(sortedValues, 0.95);
                result.p99Ms = percentile(sortedValues, 0.99);

                return result;
            }


            static std::optional<double> estimatedFramesPerSecond(
                const BenchmarkTimeStatistics& frameWallStatistics ) {

                if (frameWallStatistics.sampleCount == 0 || frameWallStatistics.meanMs <= 0.0)
                    return std::nullopt;

                const double framesPerSecond = 1000.0 / frameWallStatistics.meanMs;

                if (!std::isfinite(framesPerSecond))
                    return std::nullopt;

                return framesPerSecond;
            }
    };
}
