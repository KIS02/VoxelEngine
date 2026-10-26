#pragma once

#include <filesystem>

#include "assets/texture/ImageData.h"


namespace ve
{
    class ImageLoader {
        public:
            static ImageData load( const std::filesystem::path& path,
                                   TextureColorSpace colorSpace = TextureColorSpace::SRGB );
    };
}