#include "render/backends/opengl/OpenGLMaterial.h"

#include <stdexcept>

#include <GLFW/glfw3.h>



namespace ve
{
    void OpenGLMaterial::initialize( const char* vertexShaderSource, const char* fragmentShaderSource,
                                     int textureWidth, int textureHeight, const std::uint8_t* rgbaPixels ) {

        if (shader_.id() != 0 || texture_.id() != 0) {
            throw std::runtime_error("OpenGLMaterial is already initialized");
        }

        if (glfwGetCurrentContext() == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }

        if (vertexShaderSource == nullptr || fragmentShaderSource == nullptr) {
            throw std::invalid_argument("Invalid Material Shader source");
        }

        if (textureWidth <= 0 || textureHeight <= 0 || rgbaPixels == nullptr) {
            throw std::invalid_argument("Invalid Material Texture data");
        }



        // Initialize Shader Program

        shader_.initialize(vertexShaderSource, fragmentShaderSource);
        shader_.use();



        // Find Material Uniforms

        textureLocation_ = glGetUniformLocation(shader_.id(), "uTexture");
        tintLocation_ = glGetUniformLocation(shader_.id(), "uTint");


        if (textureLocation_ == -1 || tintLocation_ == -1) {
            glUseProgram(0);
            throw std::runtime_error("Failed to find Material Uniform locations");
        }



        // Sampler uses Texture Unit 0

        glUniform1i(textureLocation_, 0);

        glUseProgram(0);



        // Upload Texture

        texture_.initialize(textureWidth, textureHeight, rgbaPixels);
    }



    void OpenGLMaterial::bind() {
        if (shader_.id() == 0 || texture_.id() == 0) {
            throw std::runtime_error("OpenGLMaterial is not initialized");
        }



        // Activate Shader and Texture

        shader_.use();
        texture_.bind(0);



        // Normalize Color32 Channels

        const float red = static_cast<float>(getRed(tint_)) / 255.0f;
        const float green = static_cast<float>(getGreen(tint_)) / 255.0f;
        const float blue = static_cast<float>(getBlue(tint_)) / 255.0f;
        const float alpha = static_cast<float>(getAlpha(tint_)) / 255.0f;



        // Upload Tint Color

        glUniform4f(tintLocation_, red, green, blue, alpha);
    }



    void OpenGLMaterial::unbind() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }



    void OpenGLMaterial::setTint( Color32 tint ) {
        tint_ = tint;
    }



    Color32 OpenGLMaterial::tint() const {
        return tint_;
    }



    GLuint OpenGLMaterial::programId() const {
        return shader_.id();
    }
}