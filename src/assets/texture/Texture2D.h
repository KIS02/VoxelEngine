#pragma once

#include <cstdint>
#include <vector>

#include "math/Vec2.h"
#include "render/Color.h"


namespace ve
{
    class Texture2D {
        private:
            int width_ = 0;
            int height_ = 0;

            std::vector<Color32> pixels_;
            std::uint64_t id_ = 0;
            std::uint64_t revision_ = 1;


            static std::uint64_t allocateId();
            void advanceRevision();


        public:
            Texture2D( int width, int height );
            Texture2D( const Texture2D& other );
            Texture2D( Texture2D&& other );

            Texture2D& operator=( const Texture2D& other );
            Texture2D& operator=( Texture2D&& other );


            int width() const;
            int height() const;
            std::uint64_t id() const;
            std::uint64_t revision() const;

            void setPixel( int x, int y, Color32 color );
            Color32 pixel( int x, int y ) const;
            Color32 sampleNearest( const Vec2& uv ) const;

            static Texture2D checkerboard( int width, int height, int cellSize,
                                           Color32 colorA, Color32 colorB );
    };
}
