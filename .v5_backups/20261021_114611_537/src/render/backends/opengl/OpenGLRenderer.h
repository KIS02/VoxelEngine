#pragma once

#include "render/backends/opengl/OpenGLRenderCommand.h"



namespace ve
{
    class OpenGLRenderer {
        public:
            void initialize();


            void submit( const OpenGLClearCommand& command );
            void submit( const OpenGLRenderStateCommand& command );
            void submit( const OpenGLDrawCommand& command, const Mat4& view, const Mat4& projection );


            void clear( float red, float green, float blue, float alpha );
    };
}