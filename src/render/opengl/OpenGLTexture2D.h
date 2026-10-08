#pragma once

#include <cstdint>

#include <glad/gl.h>



namespace ve
{
    class OpenGLTexture2D {
        private:
            GLuint id_ = 0;

            int width_ = 0;
            int height_ = 0;


        public:
            OpenGLTexture2D() = default;
            ~OpenGLTexture2D();


            OpenGLTexture2D( const OpenGLTexture2D& ) = delete;
            OpenGLTexture2D& operator=( const OpenGLTexture2D& ) = delete;

            OpenGLTexture2D( OpenGLTexture2D&& ) = delete;
            OpenGLTexture2D& operator=( OpenGLTexture2D&& ) = delete;


            void initialize( int width, int height, const std::uint8_t* rgbaPixels );

            void bind( GLuint textureUnit ) const;


            GLuint id() const;

            int width() const;
            int height() const;
    };
}