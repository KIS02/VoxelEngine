#pragma once

#include <cstdint>

#include <glad/gl.h>

#include "math/Color.h"
#include "render/backends/opengl/OpenGLTexture2D.h"
#include "render/backends/opengl/Shader.h"



namespace ve
{
    class OpenGLMaterial {
        private:
            Shader shader_;
            OpenGLTexture2D texture_;

            Color32 tint_ = makeColor(255, 255, 255, 255);

            GLint textureLocation_ = -1;
            GLint tintLocation_ = -1;


        public:
            OpenGLMaterial() = default;


            OpenGLMaterial( const OpenGLMaterial& ) = delete;
            OpenGLMaterial& operator=( const OpenGLMaterial& ) = delete;

            OpenGLMaterial( OpenGLMaterial&& ) = delete;
            OpenGLMaterial& operator=( OpenGLMaterial&& ) = delete;


            void initialize( const char* vertexShaderSource, const char* fragmentShaderSource,
                             int textureWidth, int textureHeight, const std::uint8_t* rgbaPixels );

            void bind();
            void unbind();


            void setTint( Color32 tint );

            Color32 tint() const;

            GLuint programId() const;
    };
}