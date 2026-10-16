#pragma once

#include <glad/gl.h>



namespace ve
{
    class Shader {
        private:
            GLuint program_ = 0;


        public:
            Shader() = default;
            ~Shader();

            Shader( const Shader& ) = delete;
            Shader& operator=( const Shader& ) = delete;

            Shader( Shader&& ) = delete;
            Shader& operator=( Shader&& ) = delete;


            void initialize( const char* vertexSource, const char* fragmentSource );

            void use() const;

            GLuint id() const;
    };
}