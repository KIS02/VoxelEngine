#pragma once

#include <array>
#include <cstddef>

#include <glad/gl.h>

#include "render/shadow/ShadowMapping.h"


namespace ve
{
    struct RenderFrame;
    struct BenchmarkRenderStatistics;
    class OpenGLResourceCache;


    class OpenGLShadowPass {
        private:
            static constexpr int LayersPerCaster = 6;
            static constexpr int MaxCasters = 4;
            static constexpr int MaxLayers = MaxCasters * LayersPerCaster;

            GLuint framebuffer_ = 0;
            GLuint depthArray_ = 0;
            GLuint program_ = 0;
            int resolution_ = 0;

            ShadowFrame prepared_{};
            std::array<GLint, 16> baseLayers_{};

            void ensureResources( int resolution );
            void restoreAndRelease();

        public:
            OpenGLShadowPass() = default;
            ~OpenGLShadowPass();

            OpenGLShadowPass( const OpenGLShadowPass& ) = delete;
            OpenGLShadowPass& operator=( const OpenGLShadowPass& ) = delete;
            OpenGLShadowPass( OpenGLShadowPass&& ) = delete;
            OpenGLShadowPass& operator=( OpenGLShadowPass&& ) = delete;

            void render( const RenderFrame& frame, OpenGLResourceCache& resources,
                         BenchmarkRenderStatistics* statistics );
            void bindForLighting( GLuint programId ) const;

            bool active() const;
    };
}
