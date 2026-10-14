#pragma once

#include <glad/gl.h>



namespace ve
{
    class MeshData;


    class OpenGLMesh {
        private:
            GLuint vao_ = 0;
            GLuint vbo_ = 0;
            GLuint ebo_ = 0;

            GLsizei indexCount_ = 0;


        public:
            OpenGLMesh() = default;
            ~OpenGLMesh();


            OpenGLMesh( const OpenGLMesh& ) = delete;
            OpenGLMesh& operator=( const OpenGLMesh& ) = delete;

            OpenGLMesh( OpenGLMesh&& ) = delete;
            OpenGLMesh& operator=( OpenGLMesh&& ) = delete;


            void initialize( const MeshData& meshData );

            void draw() const;


            GLsizei indexCount() const;
    };
}