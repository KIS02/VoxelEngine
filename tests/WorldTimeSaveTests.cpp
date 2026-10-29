#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "gameplay/save/WorldTimeSave.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto path = std::filesystem::temp_directory_path() /
            ("phase12_6_world_time_test_" + std::to_string(unique) + ".vet");
        std::filesystem::remove(path);
        ve::WorldClock initial;
        require(!ve::WorldTimeSave::read(path, initial), "Missing save must be optional");

        initial.restore(53, 270.0, 600.0);
        ve::WorldTimeSave::write(path, initial);
        ve::WorldClock recovered;
        require(ve::WorldTimeSave::read(path, recovered), "Saved world time not loaded");
        require(recovered.day() == 53 && std::fabs(recovered.hours() - 10.8) < 1.0e-8 &&
                recovered.dayLengthSeconds() == 600.0, "World time roundtrip mismatch");

        std::vector<char> bytes;
        {
            std::ifstream input(path, std::ios::binary);
            require(static_cast<bool>(input), "Cannot open saved world time for checksum test");
            bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
            require(!input.bad(), "Cannot read saved world time for checksum test");
        }
        require(bytes.size() == 36, "Unexpected world time save byte size");
        bytes[15] ^= 0x22;
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        bool rejected = false;
        try { (void)ve::WorldTimeSave::read(path, recovered); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected && recovered.day() == 53, "Damaged save must not mutate the previous time");
        {
            std::ofstream oversized(path, std::ios::binary | std::ios::trunc);
            std::vector<char> garbage(1024 * 1024, 'x');
            oversized.write(garbage.data(), static_cast<std::streamsize>(garbage.size()));
        }
        rejected = false;
        try { (void)ve::WorldTimeSave::read(path, recovered); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected && recovered.day() == 53, "Oversized save must be rejected without modifying time");

        require(std::filesystem::remove(path), "World time test file must be removed");
        std::cout << "[PASS] WorldTimeSave persistence, missing file, checksum, transactional restore\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
