#include "render/opengl/Shader.h"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>



namespace ve
{
    namespace
    {
        std::string getShaderLog( GLuint shader ) {
            GLint logLength = 0;

            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength <= 0) {
                return "No shader information log available";
            }


            std::string infoLog(static_cast<std::size_t>(logLength), '\0');
            GLsizei written = 0;

            glGetShaderInfoLog(shader, logLength, &written, infoLog.data());

            infoLog.resize(static_cast<std::size_t>(written));

            return infoLog;
        }


        std::string getProgramLog( GLuint program ) {
            GLint logLength = 0;

            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength <= 0) {
                return "No program information log available";
            }


            std::string infoLog(static_cast<std::size_t>(logLength), '\0');
            GLsizei written = 0;

            glGetProgramInfoLog(program, logLength, &written, infoLog.data());

            infoLog.resize(static_cast<std::size_t>(written));

            return infoLog;
        }


        GLuint compileShader( GLenum type, const char* source, const char* name ) {
            const GLuint shader = glCreateShader(type);

            if (shader == 0) {
                throw std::runtime_error(std::string("Failed to create ") + name);
            }


            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);


            GLint compileStatus = GL_FALSE;

            glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);


            if (compileStatus != GL_TRUE) {
                const std::string infoLog = getShaderLog(shader);

                glDeleteShader(shader);

                throw std::runtime_error(
                    std::string(name) + " compilation failed:\n" + infoLog
                );
            }


            std::cout << "[Shader] " << name << " compiled successfully" << std::endl;

            return shader;
        }
    }



    Shader::~Shader() {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
    }


    void Shader::initialize( const char* vertexSource, const char* fragmentSource ) {
        if (program_ != 0) {
            throw std::runtime_error("Shader is already initialized");
        }


        GLuint vertexShader = 0;
        GLuint fragmentShader = 0;
        GLuint program = 0;


        try {
            vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, "Vertex Shader");
            fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, "Fragment Shader");


            program = glCreateProgram();

            if (program == 0) {
                throw std::runtime_error("Failed to create Shader Program");
            }


            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);

            glLinkProgram(program);


            GLint linkStatus = GL_FALSE;

            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);


            if (linkStatus != GL_TRUE) {
                throw std::runtime_error(
                    "Shader Program linking failed:\n" + getProgramLog(program)
                );
            }


            glDetachShader(program, vertexShader);
            glDetachShader(program, fragmentShader);

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            vertexShader = 0;
            fragmentShader = 0;


            program_ = program;
            program = 0;


            std::cout << "[Shader] Program linked successfully" << std::endl;
        }
        catch (...) {
            if (program != 0) {
                glDeleteProgram(program);
            }

            if (fragmentShader != 0) {
                glDeleteShader(fragmentShader);
            }

            if (vertexShader != 0) {
                glDeleteShader(vertexShader);
            }

            throw;
        }
    }


    void Shader::use() const {
        if (program_ == 0) {
            throw std::runtime_error("Shader is not initialized");
        }

        glUseProgram(program_);
    }


    GLuint Shader::id() const {
        return program_;
    }
}