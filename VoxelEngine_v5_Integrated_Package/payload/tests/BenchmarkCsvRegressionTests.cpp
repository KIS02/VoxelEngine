
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace
{
    struct CsvData
    {
        std::vector<std::string> headers;
        std::unordered_map<std::string, std::size_t> columns;
        std::vector<std::vector<std::string>> rows;
    };


    void require( bool condition, const std::string& message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    std::vector<std::string> parseCsvLine( const std::string& line ) {
        std::vector<std::string> values;
        std::string value;
        bool quoted = false;

        for (std::size_t i = 0; i < line.size(); ++i) {
            const char character = line[i];

            if (character == '"') {
                if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
                    value.push_back('"');
                    ++i;
                } else {
                    quoted = !quoted;
                }
            } else if (character == ',' && !quoted) {
                values.push_back(value);
                value.clear();
            } else {
                value.push_back(character);
            }
        }

        require(!quoted, "Unclosed CSV quotation");
        values.push_back(value);
        return values;
    }


    CsvData readCsv( const std::string& path ) {
        std::ifstream file(path);
        require(file.is_open(), "Cannot open CSV: " + path);

        CsvData data{};
        std::string line;

        require(static_cast<bool>(std::getline(file, line)), "CSV has no header: " + path);

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        data.headers = parseCsvLine(line);

        for (std::size_t i = 0; i < data.headers.size(); ++i) {
            const auto [iterator, inserted] = data.columns.emplace(data.headers[i], i);
            (void)iterator;

            require(inserted, "Duplicate CSV column: " + data.headers[i]);
        }

        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            require(!line.empty(), "Unexpected blank CSV row");

            std::vector<std::string> row = parseCsvLine(line);

            require(row.size() == data.headers.size(), "CSV field count mismatch");

            data.rows.push_back(std::move(row));
        }

        require(!file.bad(), "CSV read error: " + path);

        return data;
    }


    const std::string& field( const CsvData& data, const std::vector<std::string>& row,
                              const std::string& name ) {
        const auto found = data.columns.find(name);

        require(found != data.columns.end(), "Missing CSV column: " + name);

        return row[found->second];
    }


    std::uint64_t unsignedValue( const std::string& value, const std::string& name ) {
        require(!value.empty(), "Empty unsigned field: " + name);

        require(std::all_of(value.begin(), value.end(), []( unsigned char c ) {
            return c >= '0' && c <= '9';
        }), "Invalid unsigned field: " + name);

        return std::stoull(value);
    }


    double milliseconds( const std::string& value, const std::string& name ) {
        require(!value.empty(), "Empty timing field: " + name);

        std::size_t consumed = 0;
        const double result = std::stod(value, &consumed);

        require(consumed == value.size() && std::isfinite(result) && result >= 0.0,
                "Invalid timing field: " + name);

        return result;
    }


    void validateSchema( const CsvData& data ) {
        const std::vector<std::string> required{
            "schema_version", "backend", "scenario", "build", "framebuffer_width", "framebuffer_height",
            "vsync_requested", "crosshair_enabled", "selection_outline_enabled",
            "camera_x", "camera_y", "camera_z", "forward_x", "forward_y", "forward_z",
            "warmup_frames", "measurement_frames", "frame_id", "frame_wall_ms", "event_poll_ms",
            "input_ms", "update_ms", "mesh_rebuild_ms", "frame_assembly_ms", "cpu_renderer_work_ms",
            "present_ms", "gpu_state", "gpu_render_ms", "resident_chunks", "draw_calls",
            "vertices_referenced", "indices_submitted", "triangles_submitted", "mesh_rebuild_count",
            "gpu_mesh_upload_count", "gpu_mesh_release_count"
        };

        require(data.headers.size() == required.size(), "Unexpected schema column count");

        for (const std::string& name : required)
            require(data.columns.count(name) == 1, "Missing schema field: " + name);

        require(data.rows.size() == 600, "Expected exactly 600 measurement frames");
    }


    void validateFrames( const CsvData& data, const std::string& expectedBackend ) {
        const std::vector<std::string> timingColumns{
            "frame_wall_ms", "event_poll_ms", "input_ms", "update_ms", "mesh_rebuild_ms",
            "frame_assembly_ms", "cpu_renderer_work_ms", "present_ms"
        };

        const std::vector<std::string> stableColumns{
            "schema_version", "backend", "scenario", "build", "framebuffer_width", "framebuffer_height",
            "vsync_requested", "crosshair_enabled", "selection_outline_enabled",
            "camera_x", "camera_y", "camera_z", "forward_x", "forward_y", "forward_z",
            "warmup_frames", "measurement_frames", "resident_chunks", "draw_calls",
            "vertices_referenced", "indices_submitted", "triangles_submitted"
        };

        const std::vector<std::string>& baseline = data.rows.front();

        require(field(data, baseline, "backend") == expectedBackend, "Wrong backend");
        require(field(data, baseline, "schema_version") == "1", "Wrong schema version");
        require(field(data, baseline, "scenario") == "fixed_demo_world_4_chunks", "Wrong benchmark scene");
        require(field(data, baseline, "build") == "NDEBUG build", "Benchmark must use Release build");
        require(field(data, baseline, "framebuffer_width") == "800", "Wrong framebuffer width");
        require(field(data, baseline, "framebuffer_height") == "600", "Wrong framebuffer height");
        require(field(data, baseline, "warmup_frames") == "120", "Wrong warmup frame count");
        require(field(data, baseline, "measurement_frames") == "600", "Wrong measurement frame count");
        require(field(data, baseline, "crosshair_enabled") == "0", "Crosshair must be disabled");
        require(field(data, baseline, "selection_outline_enabled") == "0", "Selection outline must be disabled");
        require(field(data, baseline, "resident_chunks") == "4", "Unexpected Chunk count");
        require(field(data, baseline, "draw_calls") == "5", "Unexpected Draw count");
        require(field(data, baseline, "vertices_referenced") == "11672", "Unexpected Vertex count");
        require(field(data, baseline, "indices_submitted") == "17508", "Unexpected Index count");
        require(field(data, baseline, "triangles_submitted") == "5836", "Unexpected Triangle count");

        if (expectedBackend == "OpenGL")
            require(field(data, baseline, "vsync_requested") == "0", "OpenGL must request Swap Interval 0");
        else
            require(field(data, baseline, "vsync_requested").empty(), "Software VSync must be not applicable");

        for (std::size_t i = 0; i < data.rows.size(); ++i) {
            const std::vector<std::string>& row = data.rows[i];

            const std::uint64_t frameId = unsignedValue(field(data, row, "frame_id"), "frame_id");

            require(frameId == 121 + i, "Missing, duplicate, or out-of-order Frame ID");

            for (const std::string& name : stableColumns)
                require(field(data, row, name) == field(data, baseline, name), "Scene changed: " + name);

            for (const std::string& name : timingColumns)
                (void)milliseconds(field(data, row, name), name);

            const std::uint64_t indices = unsignedValue(field(data, row, "indices_submitted"), "indices_submitted");
            const std::uint64_t triangles = unsignedValue(field(data, row, "triangles_submitted"), "triangles_submitted");

            require(indices == triangles * 3, "Index/Triangle mismatch");

            for (const char* name : { "mesh_rebuild_count", "gpu_mesh_upload_count", "gpu_mesh_release_count" })
                require(unsignedValue(field(data, row, name), name) == 0,
                        std::string("Resource mutation during measurement: ") + name);

            const std::string gpuState = field(data, row, "gpu_state");
            const std::string gpuTime = field(data, row, "gpu_render_ms");

            if (expectedBackend == "OpenGL") {
                require(gpuState == "Available", "Missing OpenGL GPU query result");
                (void)milliseconds(gpuTime, "gpu_render_ms");
            } else {
                require(gpuState == "NotApplicable", "Software GPU state mismatch");
                require(gpuTime.empty(), "Software GPU time must be blank");
            }
        }
    }


    void validateSameScenario( const CsvData& opengl, const CsvData& software ) {
        const std::vector<std::string> matchingColumns{
            "scenario", "build", "framebuffer_width", "framebuffer_height", "crosshair_enabled",
            "selection_outline_enabled", "camera_x", "camera_y", "camera_z",
            "forward_x", "forward_y", "forward_z", "warmup_frames", "measurement_frames",
            "resident_chunks", "draw_calls", "vertices_referenced", "indices_submitted", "triangles_submitted"
        };

        for (std::size_t i = 0; i < opengl.rows.size(); ++i) {
            for (const std::string& name : matchingColumns) {
                require(field(opengl, opengl.rows[i], name) == field(software, software.rows[i], name),
                        "Cross-backend mismatch: " + name);
            }
        }
    }
}


int main( int argc, char* argv[] ) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: BenchmarkCsvRegressionTests <opengl.csv> <software.csv>\n";
            return 1;
        }

        const CsvData opengl = readCsv(argv[1]);
        const CsvData software = readCsv(argv[2]);

        validateSchema(opengl);
        validateSchema(software);

        std::cout << "[PASS] CSV schema v1 and 36 required fields\n";

        validateFrames(opengl, "OpenGL");

        std::cout << "[PASS] OpenGL: 600 sequential frames, fixed scene, valid CPU/GPU timings\n";

        validateFrames(software, "Software");

        std::cout << "[PASS] Software: 600 sequential frames, fixed scene, valid CPU timings\n";

        validateSameScenario(opengl, software);

        std::cout << "[PASS] Cross-backend scene, camera, resolution and geometry match\n";
        std::cout << "[PASS] No Mesh rebuilds, GPU uploads or releases during measurement\n";

        std::cout << "\nAll Benchmark CSV Regression tests passed.\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
