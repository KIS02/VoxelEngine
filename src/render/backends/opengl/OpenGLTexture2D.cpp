#include "render/backends/opengl/OpenGLTexture2D.h"

#include <stdexcept>

#include <GLFW/glfw3.h>


namespace ve
{
    OpenGLTexture2D::~OpenGLTexture2D() {
        if (id_ != 0)
            glDeleteTextures(1, &id_);
    }


    void OpenGLTexture2D::initialize( int width, int height, const std::uint8_t* rgbaPixels,
                                      TextureColorSpace colorSpace ) {
        if (id_ != 0)
            throw std::runtime_error("OpenGL Texture is already initialized");

        if (glfwGetCurrentContext() == nullptr)
            throw std::runtime_error("No current OpenGL context");

        if (width <= 0 || height <= 0 || rgbaPixels == nullptr)
            throw std::runtime_error("Invalid Texture dimensions or pixel data");

        glGenTextures(1, &id_);

        if (id_ == 0)
            throw std::runtime_error("Failed to create OpenGL Texture");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, id_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLint previousUnpackAlignment = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const GLint internalFormat = colorSpace == TextureColorSpace::SRGB
            ? GL_SRGB8_ALPHA8 : GL_RGBA8;

        glTexImage2D(
            GL_TEXTURE_2D, 0, internalFormat,
            width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

        GLint uploadedWidth = 0;
        GLint uploadedHeight = 0;

        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &uploadedWidth);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &uploadedHeight);

        const GLenum error = glGetError();
        glBindTexture(GL_TEXTURE_2D, 0);

        if (error != GL_NO_ERROR)
            throw std::runtime_error("OpenGL Texture upload failed");

        if (uploadedWidth != width || uploadedHeight != height)
            throw std::runtime_error("OpenGL Texture dimensions do not match");

        width_ = width;
        height_ = height;
    }


    void OpenGLTexture2D::bind( GLuint textureUnit ) const {
        if (id_ == 0)
            throw std::runtime_error("OpenGL Texture is not initialized");

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, id_);
    }


    GLuint OpenGLTexture2D::id() const {
        return id_;
    }

    int OpenGLTexture2D::width() const {
        return width_;
    }

    int OpenGLTexture2D::height() const {
        return height_;
    }
}