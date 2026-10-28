#pragma once

#include <cstdint>
#include <vector>

#include "math/LinearRGB.h"
#include "render/color/ToneMapping.h"


namespace ve
{
    class FrameBuffer;


    struct HdrPixel
    {
        LinearRGB color{};
        std::uint8_t alpha = 255;
    };


    class HdrColorBuffer {
        private:
            int width_ = 0;
            int height_ = 0;
            std::vector<HdrPixel> pixels_;

        public:
            HdrColorBuffer() = default;
            HdrColorBuffer( int width, int height );

            void resize( int width, int height );
            void clear( const HdrPixel& color );
            void setPixel( int x, int y, const HdrPixel& value );
            HdrPixel pixel( int x, int y ) const;
            void resolve( FrameBuffer& target, const ToneMappingSettings& settings ) const;

            int width() const;
            int height() const;
    };
}
