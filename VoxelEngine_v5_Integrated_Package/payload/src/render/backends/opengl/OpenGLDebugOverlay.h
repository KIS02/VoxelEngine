
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include "render/frame/RenderFrame.h"
#include "render/overlay/SoftwareDebugOverlay.h"


namespace ve
{
    class OpenGLDebugOverlay {
        private:
            struct Vertex
            {
                float x = 0.0f;
                float y = 0.0f;
                float u = 0.0f;
                float v = 0.0f;
                float r = 1.0f;
                float g = 1.0f;
                float b = 1.0f;
                float a = 1.0f;
            };


            static constexpr int GlyphWidth = 5;
            static constexpr int GlyphHeight = 7;
            static constexpr int GlyphPixelWidth = 8;
            static constexpr int GlyphPixelHeight = 11;

            static constexpr int CharacterAdvance = 10;
            static constexpr int LineAdvance = 15;
            static constexpr int Padding = 8;
            static constexpr int OriginX = 12;
            static constexpr int OriginY = 12;

            static constexpr int AtlasColumns = 16;
            static constexpr int AtlasRows = 8;
            static constexpr int AtlasWidth = AtlasColumns * GlyphWidth;
            static constexpr int AtlasHeight = AtlasRows * GlyphHeight;


            unsigned int program_ = 0;
            unsigned int vao_ = 0;
            unsigned int vbo_ = 0;
            unsigned int fontTexture_ = 0;

            int viewportLocation_ = -1;
            int textureLocation_ = -1;
            int textModeLocation_ = -1;

            std::vector<Vertex> textVertices_;


            static unsigned int compileShader( unsigned int type, const char* source ) {
                const unsigned int shader = glCreateShader(type);

                glShaderSource(shader, 1, &source, nullptr);
                glCompileShader(shader);

                GLint compiled = GL_FALSE;
                glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

                if (compiled != GL_TRUE) {
                    GLchar log[1024]{};
                    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
                    glDeleteShader(shader);

                    throw std::runtime_error(std::string("Debug Overlay shader failed: ") + log);
                }

                return shader;
            }


            static void appendQuad( std::vector<Vertex>& vertices, float x0, float y0,
                                    float x1, float y1, float u0, float v0,
                                    float u1, float v1, const std::array<float, 4>& color ) {
                const Vertex topLeft{ x0, y0, u0, v0, color[0], color[1], color[2], color[3] };
                const Vertex topRight{ x1, y0, u1, v0, color[0], color[1], color[2], color[3] };
                const Vertex bottomRight{ x1, y1, u1, v1, color[0], color[1], color[2], color[3] };
                const Vertex bottomLeft{ x0, y1, u0, v1, color[0], color[1], color[2], color[3] };

                vertices.push_back(topLeft);
                vertices.push_back(topRight);
                vertices.push_back(bottomRight);

                vertices.push_back(topLeft);
                vertices.push_back(bottomRight);
                vertices.push_back(bottomLeft);
            }


            static void restoreEnabled( unsigned int capability, unsigned char enabled ) {
                if (enabled == GL_TRUE)
                    glEnable(capability);
                else
                    glDisable(capability);
            }


            void release() {
                if (fontTexture_ != 0) {
                    glDeleteTextures(1, &fontTexture_);
                    fontTexture_ = 0;
                }

                if (vbo_ != 0) {
                    glDeleteBuffers(1, &vbo_);
                    vbo_ = 0;
                }

                if (vao_ != 0) {
                    glDeleteVertexArrays(1, &vao_);
                    vao_ = 0;
                }

                if (program_ != 0) {
                    glDeleteProgram(program_);
                    program_ = 0;
                }
            }


            void initialize() {
                if (program_ != 0)
                    return;

                static constexpr char vertexSource[] = R"(
#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

uniform vec2 uViewportSize;

out vec2 vUV;
out vec4 vColor;

void main() {
    vec2 ndc = vec2(
        aPosition.x / uViewportSize.x * 2.0 - 1.0,
        1.0 - aPosition.y / uViewportSize.y * 2.0
    );

    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)";

                static constexpr char fragmentSource[] = R"(
#version 330 core
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uGlyphAtlas;
uniform int uTextMode;

out vec4 FragColor;

void main() {
    float coverage = uTextMode != 0
        ? texture(uGlyphAtlas, vUV).r
        : 1.0;

    FragColor = vec4(vColor.rgb, vColor.a * coverage);
}
)";

                const unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
                unsigned int fragmentShader = 0;

                try {
                    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

                    program_ = glCreateProgram();
                    glAttachShader(program_, vertexShader);
                    glAttachShader(program_, fragmentShader);
                    glLinkProgram(program_);

                    GLint linked = GL_FALSE;
                    glGetProgramiv(program_, GL_LINK_STATUS, &linked);

                    if (linked != GL_TRUE) {
                        GLchar log[1024]{};
                        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);

                        throw std::runtime_error(std::string("Debug Overlay link failed: ") + log);
                    }
                } catch (...) {
                    glDeleteShader(vertexShader);

                    if (fragmentShader != 0)
                        glDeleteShader(fragmentShader);

                    release();
                    throw;
                }

                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);

                viewportLocation_ = glGetUniformLocation(program_, "uViewportSize");
                textureLocation_ = glGetUniformLocation(program_, "uGlyphAtlas");
                textModeLocation_ = glGetUniformLocation(program_, "uTextMode");


                // Preserve Existing OpenGL Bindings

                GLint previousVao = 0;
                GLint previousBuffer = 0;
                GLint previousActiveTexture = 0;
                GLint previousTexture = 0;
                GLint previousUnpackAlignment = 4;

                glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
                glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
                glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

                glActiveTexture(GL_TEXTURE0);
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);


                // Vertex Array and Dynamic Vertex Buffer

                glGenVertexArrays(1, &vao_);
                glGenBuffers(1, &vbo_);

                glBindVertexArray(vao_);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_);
                glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<const void*>(offsetof(Vertex, x)));

                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<const void*>(offsetof(Vertex, u)));

                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<const void*>(offsetof(Vertex, r)));


                // Generate 5x7 Bitmap Font Atlas

                std::vector<std::uint8_t> pixels(AtlasWidth * AtlasHeight, 0);

                for (int character = 0; character < 128; ++character) {
                    const auto glyph = SoftwareDebugOverlay::glyphRows(static_cast<char>(character));

                    const int atlasX = (character % AtlasColumns) * GlyphWidth;
                    const int atlasY = (character / AtlasColumns) * GlyphHeight;

                    for (int y = 0; y < GlyphHeight; ++y) {
                        for (int x = 0; x < GlyphWidth; ++x) {
                            const bool visible = (glyph[y] & (1u << (GlyphWidth - x - 1))) != 0;

                            const std::size_t index =
                                static_cast<std::size_t>(atlasY + y) * AtlasWidth + atlasX + x;

                            pixels[index] = visible ? 255 : 0;
                        }
                    }
                }


                // OpenGL 3.3 Single-channel Glyph Texture

                glGenTextures(1, &fontTexture_);
                glBindTexture(GL_TEXTURE_2D, fontTexture_);

                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AtlasWidth, AtlasHeight,
                             0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


                // Restore Existing Bindings

                glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
                glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(previousTexture));
                glActiveTexture(static_cast<unsigned int>(previousActiveTexture));

                glBindVertexArray(static_cast<unsigned int>(previousVao));
                glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
            }


        public:
            OpenGLDebugOverlay() = default;

            OpenGLDebugOverlay( const OpenGLDebugOverlay& ) = delete;
            OpenGLDebugOverlay& operator=( const OpenGLDebugOverlay& ) = delete;

            ~OpenGLDebugOverlay() {
                if (glfwGetCurrentContext() != nullptr)
                    release();
            }


            void draw( const DebugOverlaySettings& settings, int width, int height ) {
                if (!settings.enabled || settings.lines.empty() || width <= 0 || height <= 0)
                    return;

                const int availableWidth = width - OriginX - Padding * 2;
                const int availableHeight = height - OriginY - Padding * 2;

                if (availableWidth < CharacterAdvance || availableHeight < LineAdvance)
                    return;

                const int maxCharacters = availableWidth / CharacterAdvance;
                const int maxLines = availableHeight / LineAdvance;
                const std::size_t visibleLines =
                    std::min(settings.lines.size(), static_cast<std::size_t>(maxLines));

                std::size_t widestLine = 0;

                for (std::size_t i = 0; i < visibleLines; ++i) {
                    widestLine = std::max(
                        widestLine,
                        std::min(settings.lines[i].size(), static_cast<std::size_t>(maxCharacters))
                    );
                }

                if (widestLine == 0)
                    return;


                // Build Screen-space Glyph Geometry

                textVertices_.clear();
                textVertices_.reserve(visibleLines * static_cast<std::size_t>(maxCharacters) * 6);

                const std::array<float, 4> headerColor{ 115.0f / 255.0f, 220.0f / 255.0f, 1.0f, 1.0f };
                const std::array<float, 4> bodyColor{ 1.0f, 1.0f, 1.0f, 1.0f };

                for (std::size_t line = 0; line < visibleLines; ++line) {
                    const std::string& text = settings.lines[line];
                    const std::size_t count =
                        std::min(text.size(), static_cast<std::size_t>(maxCharacters));

                    const std::array<float, 4>& color = line == 0 ? headerColor : bodyColor;

                    for (std::size_t i = 0; i < count; ++i) {
                        unsigned char character = static_cast<unsigned char>(text[i]);

                        if (character >= 128)
                            character = '?';

                        const int atlasColumn = character % AtlasColumns;
                        const int atlasRow = character / AtlasColumns;

                        const float u0 = static_cast<float>(atlasColumn * GlyphWidth) / AtlasWidth;
                        const float v0 = static_cast<float>(atlasRow * GlyphHeight) / AtlasHeight;
                        const float u1 = static_cast<float>((atlasColumn + 1) * GlyphWidth) / AtlasWidth;
                        const float v1 = static_cast<float>((atlasRow + 1) * GlyphHeight) / AtlasHeight;

                        const float x = static_cast<float>(
                            OriginX + Padding + static_cast<int>(i) * CharacterAdvance
                        );

                        const float y = static_cast<float>(
                            OriginY + Padding + static_cast<int>(line) * LineAdvance
                        );

                        appendQuad(textVertices_, x, y, x + GlyphPixelWidth, y + GlyphPixelHeight,
                                   u0, v0, u1, v1, color);
                    }
                }


                // Background Matches the Software HUD Layout

                const float panelWidth = static_cast<float>(
                    Padding * 2 + static_cast<int>(widestLine) * CharacterAdvance
                );

                const float panelHeight = static_cast<float>(
                    Padding * 2 + static_cast<int>(visibleLines - 1) * LineAdvance + GlyphPixelHeight
                );

                std::vector<Vertex> background;
                background.reserve(6);

                appendQuad(background, static_cast<float>(OriginX), static_cast<float>(OriginY),
                           OriginX + panelWidth, OriginY + panelHeight,
                           0.0f, 0.0f, 0.0f, 0.0f, { 0.0f, 0.0f, 0.0f, 0.6f });


                // Save the Existing OpenGL State

                GLint previousProgram = 0;
                GLint previousVao = 0;
                GLint previousBuffer = 0;
                GLint previousActiveTexture = 0;
                GLint previousTexture = 0;

                GLint previousBlendSrcRgb = 0;
                GLint previousBlendDstRgb = 0;
                GLint previousBlendSrcAlpha = 0;
                GLint previousBlendDstAlpha = 0;
                GLint previousBlendEquationRgb = 0;
                GLint previousBlendEquationAlpha = 0;

                GLboolean previousDepthMask = GL_TRUE;
                GLboolean previousColorMask[4]{};

                const GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
                const GLboolean previousBlend = glIsEnabled(GL_BLEND);
                const GLboolean previousCullFace = glIsEnabled(GL_CULL_FACE);
                const GLboolean previousScissor = glIsEnabled(GL_SCISSOR_TEST);
                const GLboolean previousStencil = glIsEnabled(GL_STENCIL_TEST);

                glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
                glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
                glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);

                glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrcRgb);
                glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDstRgb);
                glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrcAlpha);
                glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDstAlpha);
                glGetIntegerv(GL_BLEND_EQUATION_RGB, &previousBlendEquationRgb);
                glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &previousBlendEquationAlpha);

                glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
                glGetBooleanv(GL_COLOR_WRITEMASK, previousColorMask);

                glActiveTexture(GL_TEXTURE0);
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);


                // Apply 2D Overlay State

                initialize();

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
                glDisable(GL_SCISSOR_TEST);
                glDisable(GL_STENCIL_TEST);

                glDepthMask(GL_FALSE);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                glEnable(GL_BLEND);
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                glUseProgram(program_);
                glUniform2f(viewportLocation_, static_cast<float>(width), static_cast<float>(height));
                glUniform1i(textureLocation_, 0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fontTexture_);

                glBindVertexArray(vao_);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_);


                // Draw Translucent Background

                glUniform1i(textModeLocation_, 0);
                glBufferData(GL_ARRAY_BUFFER, background.size() * sizeof(Vertex),
                             background.data(), GL_STREAM_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(background.size()));


                // Draw Glyphs Using Bitmap Atlas

                if (!textVertices_.empty()) {
                    glUniform1i(textModeLocation_, 1);
                    glBufferData(GL_ARRAY_BUFFER, textVertices_.size() * sizeof(Vertex),
                                 textVertices_.data(), GL_STREAM_DRAW);
                    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(textVertices_.size()));
                }


                // Restore OpenGL State

                glBindVertexArray(static_cast<unsigned int>(previousVao));
                glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
                glUseProgram(static_cast<unsigned int>(previousProgram));

                glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(previousTexture));
                glActiveTexture(static_cast<unsigned int>(previousActiveTexture));

                glBlendFuncSeparate(previousBlendSrcRgb, previousBlendDstRgb,
                                    previousBlendSrcAlpha, previousBlendDstAlpha);

                glBlendEquationSeparate(previousBlendEquationRgb, previousBlendEquationAlpha);

                glDepthMask(previousDepthMask);
                glColorMask(previousColorMask[0], previousColorMask[1],
                            previousColorMask[2], previousColorMask[3]);

                restoreEnabled(GL_DEPTH_TEST, previousDepthTest);
                restoreEnabled(GL_BLEND, previousBlend);
                restoreEnabled(GL_CULL_FACE, previousCullFace);
                restoreEnabled(GL_SCISSOR_TEST, previousScissor);
                restoreEnabled(GL_STENCIL_TEST, previousStencil);
            }
    };
}
