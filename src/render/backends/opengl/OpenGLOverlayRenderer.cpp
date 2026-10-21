#include "render/backends/opengl/OpenGLOverlayRenderer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "math/Color.h"
#include "render/frame/RenderFrame.h"
#include "render/overlay/CrosshairGeometry.h"
#include "render/overlay/SelectionOutlineGeometry.h"

#include <glad/gl.h>
#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>


namespace ve
{
    namespace
    {
        unsigned int compileOutlineShader( unsigned int type, const char* source ) {
            const unsigned int shader = glCreateShader(type);

            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

            if (compiled != GL_TRUE) {
                GLchar log[1024]{};
                glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
                glDeleteShader(shader);

                throw std::runtime_error(std::string("Selection Outline shader compilation failed: ") + log);
            }

            return shader;
        }
    }


    OpenGLOverlayRenderer::~OpenGLOverlayRenderer() {
        if (glfwGetCurrentContext() == nullptr)
            return;

        if (outlineVbo_ != 0)
            glDeleteBuffers(1, &outlineVbo_);

        if (outlineVao_ != 0)
            glDeleteVertexArrays(1, &outlineVao_);

        if (outlineProgram_ != 0)
            glDeleteProgram(outlineProgram_);
    }


    void OpenGLOverlayRenderer::initialize() {
        static constexpr char vertexSource[] = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
void main() {
    gl_Position = vec4(aPosition, 1.0);
}
)";

        static constexpr char fragmentSource[] = R"(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main() {
    FragColor = uColor;
}
)";

        const unsigned int vertexShader = compileOutlineShader(GL_VERTEX_SHADER, vertexSource);
        unsigned int fragmentShader = 0;

        try {
            fragmentShader = compileOutlineShader(GL_FRAGMENT_SHADER, fragmentSource);

            outlineProgram_ = glCreateProgram();
            glAttachShader(outlineProgram_, vertexShader);
            glAttachShader(outlineProgram_, fragmentShader);
            glLinkProgram(outlineProgram_);

            GLint linked = GL_FALSE;
            glGetProgramiv(outlineProgram_, GL_LINK_STATUS, &linked);

            if (linked != GL_TRUE) {
                GLchar log[1024]{};
                glGetProgramInfoLog(outlineProgram_, sizeof(log), nullptr, log);

                throw std::runtime_error(std::string("Selection Outline program link failed: ") + log);
            }
        } catch (...) {
            glDeleteShader(vertexShader);

            if (fragmentShader != 0)
                glDeleteShader(fragmentShader);

            if (outlineProgram_ != 0) {
                glDeleteProgram(outlineProgram_);
                outlineProgram_ = 0;
            }

            throw;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);


        // Preserve Existing OpenGL Bindings

        GLint previousVao = 0;
        GLint previousBuffer = 0;

        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);


        // Create Selection Outline VAO and VBO

        glGenVertexArrays(1, &outlineVao_);
        glGenBuffers(1, &outlineVbo_);

        glBindVertexArray(outlineVao_);
        glBindBuffer(GL_ARRAY_BUFFER, outlineVbo_);

        glBufferData(
            GL_ARRAY_BUFFER,
            12 * 2 * 3 * sizeof(float),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);


        // Restore Existing Bindings

        glBindVertexArray(static_cast<unsigned int>(previousVao));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
    }


    void OpenGLOverlayRenderer::drawSelectionOutline( const RenderFrame& frame ) {
        const SelectionOutlineSettings& settings = frame.selectionOutline;

        if (!settings.enabled)
            return;

        const ProjectedSelectionOutline outline = SelectionOutlineGeometry::create(
            settings.blockMin, settings.expansion, frame.view, frame.projection
        );

        if (outline.count == 0)
            return;


        // Build Depth-tested Outline Geometry

        std::array<float, 12 * 2 * 3> vertices{};

        for (std::size_t i = 0; i < outline.count; ++i) {
            const Vec3& a = outline.lines[i].start;
            const Vec3& b = outline.lines[i].end;
            const std::size_t offset = i * 6;

            vertices[offset + 0] = a.x;
            vertices[offset + 1] = a.y;
            vertices[offset + 2] = a.z;

            vertices[offset + 3] = b.x;
            vertices[offset + 4] = b.y;
            vertices[offset + 5] = b.z;
        }


        // Preserve OpenGL State

        GLint previousProgram = 0;
        GLint previousVao = 0;
        GLint previousBuffer = 0;
        GLint previousDepthFunc = 0;

        GLfloat previousLineWidth = 1.0f;
        GLboolean previousDepthMask = GL_TRUE;

        const GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);

        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
        glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

        glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);


        // Configure Depth-tested Line Rendering

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glLineWidth(1.0f);

        glUseProgram(outlineProgram_);

        constexpr float scale = 1.0f / 255.0f;

        const GLint colorLocation = glGetUniformLocation(outlineProgram_, "uColor");

        glUniform4f(
            colorLocation,
            getRed(settings.color) * scale,
            getGreen(settings.color) * scale,
            getBlue(settings.color) * scale,
            getAlpha(settings.color) * scale
        );

        glBindVertexArray(outlineVao_);
        glBindBuffer(GL_ARRAY_BUFFER, outlineVbo_);

        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            outline.count * 6 * sizeof(float),
            vertices.data()
        );

        glDrawArrays(GL_LINES, 0, static_cast<int>(outline.count * 2));


        // Restore OpenGL State

        glBindVertexArray(static_cast<unsigned int>(previousVao));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
        glUseProgram(static_cast<unsigned int>(previousProgram));

        glDepthFunc(static_cast<unsigned int>(previousDepthFunc));
        glDepthMask(previousDepthMask);
        glLineWidth(previousLineWidth);

        if (previousDepthTest == GL_TRUE)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }


    void OpenGLOverlayRenderer::drawCrosshair( const CrosshairSettings& settings, int width, int height ) {
            if (!settings.enabled)
                return;

            const auto rectangles = CrosshairGeometry::create(
                width, height, settings.armLength, settings.thickness
            );

            const GLboolean previousScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);

            GLint previousScissorBox[4]{};
            GLfloat previousClearColor[4]{};
            GLboolean previousColorMask[4]{};

            glGetIntegerv(GL_SCISSOR_BOX, previousScissorBox);
            glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);
            glGetBooleanv(GL_COLOR_WRITEMASK, previousColorMask);

            constexpr float scale = 1.0f / 255.0f;

            glEnable(GL_SCISSOR_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

            glClearColor(
                getRed(settings.color) * scale,
                getGreen(settings.color) * scale,
                getBlue(settings.color) * scale,
                getAlpha(settings.color) * scale
            );

            for (const ScreenRect& rect : rectangles) {
                if (rect.width <= 0 || rect.height <= 0)
                    continue;

                glScissor(rect.x, height - rect.y - rect.height, rect.width, rect.height);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            glClearColor(previousClearColor[0], previousClearColor[1],
                         previousClearColor[2], previousClearColor[3]);

            glColorMask(previousColorMask[0], previousColorMask[1],
                        previousColorMask[2], previousColorMask[3]);

            glScissor(previousScissorBox[0], previousScissorBox[1],
                      previousScissorBox[2], previousScissorBox[3]);

            if (previousScissorEnabled == GL_FALSE)
                glDisable(GL_SCISSOR_TEST);
        }
}
