#pragma once

#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <string_view>
#include <vector>


namespace ve
{
    namespace content
    {
        inline std::string trim( const std::string& value ) {
            const std::size_t first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                return {};

            const std::size_t last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        inline std::vector<std::string> split( const std::string& line, char delimiter = '|' ) {
            std::vector<std::string> fields;
            std::size_t start = 0;
            while (start <= line.size()) {
                const std::size_t end = line.find(delimiter, start);
                fields.push_back(trim(line.substr(start, end == std::string::npos ? end : end - start)));
                if (end == std::string::npos)
                    break;
                start = end + 1;
            }
            return fields;
        }

        inline int parseInt( const std::string& text ) {
            int value = 0;
            const char* begin = text.data();
            const char* end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end)
                throw std::invalid_argument("Invalid content integer: " + text);
            return value;
        }

        inline float parseFloat( const std::string& text ) {
            float value = 0.0f;
            const char* begin = text.data();
            const char* end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value))
                throw std::invalid_argument("Invalid content float: " + text);
            return value;
        }

        template <typename Callback>
        bool readLines( const std::filesystem::path& path, Callback&& callback ) {
            std::error_code statusError;
            const bool exists = std::filesystem::exists(path, statusError);
            if (statusError)
                throw std::runtime_error("Cannot inspect content path: " + path.string());
            if (!exists)
                return false;

            std::ifstream stream(path, std::ios::binary);
            if (!stream)
                throw std::runtime_error("Cannot open content file: " + path.string());

            std::string line;
            int number = 0;
            while (std::getline(stream, line)) {
                ++number;
                const std::string cleaned = trim(line);
                if (cleaned.empty() || cleaned[0] == '#')
                    continue;
                try {
                    callback(split(cleaned));
                } catch (const std::exception& error) {
                    throw std::runtime_error(path.string() + ":" + std::to_string(number) + ": " + error.what());
                }
            }
            if (stream.bad())
                throw std::runtime_error("Error reading content file: " + path.string());
            return true;
        }
    }
}
