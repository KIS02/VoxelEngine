#include "gameplay/block/OrientedBlocks.h"

#include <fstream>
#include <set>
#include <string>
#include <system_error>

#include "gameplay/core/ContentTextParser.h"

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif


namespace ve
{
    bool OrientedBlocks::load( const std::filesystem::path& path ) {
        std::map<OrientedBlockKey, std::uint8_t> next;
        bool headerSeen = false;
        const bool loaded = content::readLines(path, [&]( const std::vector<std::string>& fields ) {
            if (!headerSeen) {
                if (fields.size() != 1 || fields[0] != "VE-ORIENTED-BLOCKS-V1")
                    throw std::invalid_argument("Invalid orientation save version");
                headerSeen = true;
                return;
            }
            if (fields.size() != 4)
                throw std::invalid_argument("Expected x|y|z|facing");
            const OrientedBlockKey key{ content::parseInt(fields[0]), content::parseInt(fields[1]),
                                       content::parseInt(fields[2]) };
            const int facing = content::parseInt(fields[3]);
            if (facing < 0 || facing > 3 || next.size() >= 200000 ||
                !next.emplace(key, static_cast<std::uint8_t>(facing)).second)
                throw std::invalid_argument("Invalid or duplicate block orientation");
        });
        if (loaded) {
            if (!headerSeen)
                throw std::invalid_argument("Empty orientation save file");
            orientations_.swap(next);
        }
        return loaded;
    }

    void OrientedBlocks::save( const std::filesystem::path& path ) const {
        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        std::filesystem::path temporary = path;
        temporary += ".tmp";
        try {
            {
                std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
                if (!stream)
                    throw std::runtime_error("Cannot create temporary orientation save");
                stream << "VE-ORIENTED-BLOCKS-V1\n";
                for (const auto& [key, facing] : orientations_)
                    stream << key.x << '|' << key.y << '|' << key.z << '|' << static_cast<int>(facing) << '\n';
                stream.flush();
                if (!stream)
                    throw std::runtime_error("Failed writing orientation save");
            }
#if defined(_WIN32)
            if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                throw std::runtime_error("Failed replacing orientation save");
#else
            std::filesystem::rename(temporary, path);
#endif
        } catch (...) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw;
        }
    }
}
