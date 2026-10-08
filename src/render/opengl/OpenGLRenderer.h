#pragma once

#include <glad/gl.h>

#include "render/opengl/OpenGLTexture2D.h"
#include "render/opengl/Shader.h"



namespace ve
{
    struct Mat4;


    class OpenGLRenderer {
        private:
            Shader shader_;
            OpenGLTexture2D texture_;

            GLuint vao_ = 0;
            GLuint vbo_ = 0;
            GLuint ebo_ = 0;

            GLint modelLocation_ = -1;
            GLint viewLocation_ = -1;
            GLint projectionLocation_ = -1;
            GLint textureLocation_ = -1;


        public:
            ~OpenGLRenderer();


            void initialize();

            void clear( float red, float green, float blue, float alpha );

            void drawCube( const Mat4& model, const Mat4& view, const Mat4& projection );
    };
}