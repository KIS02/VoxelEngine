#include "render/backends/opengl/pipeline/postprocess/OpenGLHdrPass.h"

#include <stdexcept>
#include <string>

#include "render/backends/opengl/Shader.h"


namespace ve
{
    namespace
    {
        constexpr const char* VertexSource = R"(#version 330 core
out vec2 vUV;
void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vUV = position;
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
)";

        constexpr const char* FragmentSource = R"(#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uHdrColor;
uniform float uExposure;
uniform int uToneOperator;
float encodeChannel(float value) {
    value = clamp(value, 0.0, 1.0);
    return value <= 0.0031308 ? value * 12.92 : 1.055 * pow(value, 1.0 / 2.4) - 0.055;
}
void main() {
    vec4 hdr = texture(uHdrColor, vUV);
    vec3 exposed = max(hdr.rgb, vec3(0.0)) * uExposure;
    vec3 mapped = uToneOperator == 0 ? exposed / (vec3(1.0) + exposed) : clamp(exposed, 0.0, 1.0);
    FragColor = vec4(encodeChannel(mapped.r), encodeChannel(mapped.g), encodeChannel(mapped.b), hdr.a);
}
)";
    }


    OpenGLHdrPass::~OpenGLHdrPass() {
        if (framebuffer_ != 0)
            glDeleteFramebuffers(1, &framebuffer_);
        if (colorTexture_ != 0)
            glDeleteTextures(1, &colorTexture_);
        if (depthBuffer_ != 0)
            glDeleteRenderbuffers(1, &depthBuffer_);
        if (fullscreenVao_ != 0)
            glDeleteVertexArrays(1, &fullscreenVao_);
    }


    void OpenGLHdrPass::ensureProgram() {
        if (shader_.id() != 0)
            return;

        shader_.initialize(VertexSource, FragmentSource);
    }


    void OpenGLHdrPass::ensureResources( int width, int height ) {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument("HDR framebuffer dimensions must be positive");

        if (framebuffer_ != 0 && width_ == width && height_ == height)
            return;

        if (framebuffer_ == 0)
            glGenFramebuffers(1, &framebuffer_);
        if (colorTexture_ == 0)
            glGenTextures(1, &colorTexture_);
        if (depthBuffer_ == 0)
            glGenRenderbuffers(1, &depthBuffer_);
        if (fullscreenVao_ == 0)
            glGenVertexArrays(1, &fullscreenVao_);

        width_ = width;
        height_ = height;

        glBindTexture(GL_TEXTURE_2D, colorTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer_);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            throw std::runtime_error("HDR framebuffer is incomplete");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    void OpenGLHdrPass::beginScene( int width, int height ) {
        ensureProgram();
        ensureResources(width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glViewport(0, 0, width, height);
        glDisable(GL_FRAMEBUFFER_SRGB);
    }


    void OpenGLHdrPass::present( const ToneMappingSettings& settings ) {
        validateToneMapping(settings);
        if (framebuffer_ == 0)
            throw std::runtime_error("HDR scene has not been initialized");

        // Copy HDR depth for selection-outline depth tests on the default framebuffer.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width_, height_);
        glDisable(GL_FRAMEBUFFER_SRGB);

        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        GLint previousProgram = 0;
        GLint previousVao = 0;
        GLint previousActiveTexture = GL_TEXTURE0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTexture_);
        glUseProgram(shader_.id());
        glUniform1i(glGetUniformLocation(shader_.id(), "uHdrColor"), 0);
        glUniform1f(glGetUniformLocation(shader_.id(), "uExposure"), settings.exposure);
        glUniform1i(glGetUniformLocation(shader_.id(), "uToneOperator"),
                    settings.method == ToneMappingOperator::Reinhard ? 0 : 1);
        glBindVertexArray(fullscreenVao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(static_cast<GLuint>(previousVao));
        glUseProgram(static_cast<GLuint>(previousProgram));
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        if (depthEnabled)
            glEnable(GL_DEPTH_TEST);
        if (blendEnabled)
            glEnable(GL_BLEND);
        if (cullEnabled)
            glEnable(GL_CULL_FACE);
    }
}
