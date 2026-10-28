#pragma once

#include <glad/gl.h>

#include "render/backends/opengl/Shader.h"

#include "render/color/ToneMapping.h"


namespace ve
{
    class OpenGLHdrPass {
        private:
            GLuint framebuffer_ = 0;
            GLuint colorTexture_ = 0;
            GLuint depthBuffer_ = 0;
            Shader shader_{};
            GLuint fullscreenVao_ = 0;
            int width_ = 0;
            int height_ = 0;

            void ensureResources( int width, int height );
            void ensureProgram();

        public:
            OpenGLHdrPass() = default;
            ~OpenGLHdrPass();

            OpenGLHdrPass( const OpenGLHdrPass& ) = delete;
            OpenGLHdrPass& operator=( const OpenGLHdrPass& ) = delete;
            OpenGLHdrPass( OpenGLHdrPass&& ) = delete;
            OpenGLHdrPass& operator=( OpenGLHdrPass&& ) = delete;

            void beginScene( int width, int height );
            void present( const ToneMappingSettings& settings );
    };
}
