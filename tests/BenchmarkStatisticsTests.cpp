
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "benchmark/BenchmarkStatistics.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void requireNear( double actual, double expected, double tolerance, const char* message ) {
        if (std::fabs(actual - expected) > tolerance)
            throw std::runtime_error(message);
    }


    void testBasicStatistics() {
        const std::vector<double> values{ 4.0, 1.0, 3.0, 2.0 };
        const ve::BenchmarkTimeStatistics result = ve::BenchmarkStatisticsCalculator::calculate(values);

        require(result.sampleCount == 4, "Incorrect sample count");
        requireNear(result.minMs, 1.0, 1e-9, "Incorrect minimum");
        requireNear(result.maxMs, 4.0, 1e-9, "Incorrect maximum");
        requireNear(result.meanMs, 2.5, 1e-9, "Incorrect mean");
        requireNear(result.medianMs, 2.5, 1e-9, "Incorrect median");
        requireNear(result.p95Ms, 3.85, 1e-9, "Incorrect P95");
        requireNear(result.p99Ms, 3.97, 1e-9, "Incorrect P99");
        require(values[0] == 4.0 && values[1] == 1.0, "Input samples must not be modified");

        std::cout << "[PASS] Mean, Median, P95, P99, Min and Max\n";
    }


    void testLargePercentileSequence() {
        std::vector<double> values;

        for (int value = 100; value >= 1; --value)
            values.push_back(static_cast<double>(value));

        const ve::BenchmarkTimeStatistics result = ve::BenchmarkStatisticsCalculator::calculate(values);

        requireNear(result.meanMs, 50.5, 1e-9, "Incorrect 100-sample mean");
        requireNear(result.medianMs, 50.5, 1e-9, "Incorrect 100-sample median");
        requireNear(result.p95Ms, 95.05, 1e-9, "Incorrect 100-sample P95");
        requireNear(result.p99Ms, 99.01, 1e-9, "Incorrect 100-sample P99");

        std::cout << "[PASS] Percentile interpolation for 100 samples\n";
    }


    void testEstimatedFrameRate() {
        const ve::BenchmarkTimeStatistics result =
            ve::BenchmarkStatisticsCalculator::calculate({ 2.0, 10.0 });

        const std::optional<double> fps =
            ve::BenchmarkStatisticsCalculator::estimatedFramesPerSecond(result);

        require(fps.has_value(), "Valid frame time must produce estimated FPS");
        requireNear(*fps, 1000.0 / 6.0, 1e-9, "FPS must use mean frame time");

        std::cout << "[PASS] FPS uses reciprocal of mean frame wall time\n";
    }


    void testZeroAndSingleSample() {
        const ve::BenchmarkTimeStatistics single =
            ve::BenchmarkStatisticsCalculator::calculate({ 7.0 });

        const ve::BenchmarkTimeStatistics zeros =
            ve::BenchmarkStatisticsCalculator::calculate({ 0.0, 0.0 });

        requireNear(single.medianMs, 7.0, 1e-9, "Single-sample median mismatch");
        requireNear(single.p95Ms, 7.0, 1e-9, "Single-sample P95 mismatch");
        requireNear(single.p99Ms, 7.0, 1e-9, "Single-sample P99 mismatch");
        requireNear(zeros.meanMs, 0.0, 1e-9, "Zero samples mean mismatch");

        require(!ve::BenchmarkStatisticsCalculator::estimatedFramesPerSecond(zeros).has_value(),
                "Zero mean frame time must not produce FPS");

        std::cout << "[PASS] Single-sample and zero-duration edge cases\n";
    }


    void testInvalidSamplesRejected() {
        const std::vector<std::vector<double>> invalidCases{
            {},
            { 1.0, -0.1 },
            { 1.0, std::numeric_limits<double>::quiet_NaN() },
            { std::numeric_limits<double>::infinity() }
        };

        for (const std::vector<double>& values : invalidCases) {
            bool rejected = false;

            try {
                (void)ve::BenchmarkStatisticsCalculator::calculate(values);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }

            require(rejected, "Invalid timing samples must be rejected");
        }

        std::cout << "[PASS] Empty, negative, NaN and infinite samples rejected\n";
    }
}


int main() {
    try {
        testBasicStatistics();
        testLargePercentileSequence();
        testEstimatedFrameRate();
        testZeroAndSingleSample();
        testInvalidSamplesRejected();

        std::cout << "\nAll Benchmark Statistics tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
