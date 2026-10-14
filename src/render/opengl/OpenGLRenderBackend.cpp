#include "render/opengl/OpenGLRenderBackend.h"

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/Window.h"

#include "demo/OpenGLCubeDemo.h"

#include "render/Color.h"
#include "render/MeshData.h"
#include "render/RenderFrame.h"
#include "render/Texture2D.h"

#include "render/overlay/CrosshairGeometry.h"
#include "render/overlay/SelectionOutlineGeometry.h"

#include "render/opengl/OpenGLRenderCommand.h"
#include "render/opengl/OpenGLRenderer.h"

#include <glad/gl.h>

#define GLFW_INCLUDE_NONE
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



        void drawCrosshair( const CrosshairSettings& settings, int width, int height ) {
            if (!settings.enabled) {
                return;
            }

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
                if (rect.width <= 0 || rect.height <= 0) {
                    continue;
                }

                glScissor(rect.x, height - rect.y - rect.height, rect.width, rect.height);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            glClearColor(previousClearColor[0], previousClearColor[1],
                         previousClearColor[2], previousClearColor[3]);
            glColorMask(previousColorMask[0], previousColorMask[1],
                        previousColorMask[2], previousColorMask[3]);
            glScissor(previousScissorBox[0], previousScissorBox[1],
                      previousScissorBox[2], previousScissorBox[3]);

            if (previousScissorEnabled == GL_FALSE) {
                glDisable(GL_SCISSOR_TEST);
            }
        }
    }



    OpenGLRenderBackend::OpenGLRenderBackend( Window& window, OpenGLRenderer& renderer,
                                             OpenGLCubeDemo& demo, const MeshData& sourceMesh,
                                             const Texture2D& sourceTexture )
        : window_(window), renderer_(renderer), demo_(demo),
          sourceMesh_(sourceMesh), sourceTexture_(sourceTexture) { }



    OpenGLRenderBackend::~OpenGLRenderBackend() {
        if (glfwGetCurrentContext() == nullptr) {
            return;
        }

        if (outlineVbo_ != 0) glDeleteBuffers(1, &outlineVbo_);
        if (outlineVao_ != 0) glDeleteVertexArrays(1, &outlineVao_);
        if (outlineProgram_ != 0) glDeleteProgram(outlineProgram_);
    }



    void OpenGLRenderBackend::initializeOutlineResources() {
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
        }
        catch (...) {
            glDeleteShader(vertexShader);

            if (fragmentShader != 0) {
                glDeleteShader(fragmentShader);
            }

            if (outlineProgram_ != 0) {
                glDeleteProgram(outlineProgram_);
                outlineProgram_ = 0;
            }

            throw;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint previousVao = 0;
        GLint previousBuffer = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);

        glGenVertexArrays(1, &outlineVao_);
        glGenBuffers(1, &outlineVbo_);

        glBindVertexArray(outlineVao_);
        glBindBuffer(GL_ARRAY_BUFFER, outlineVbo_);
        glBufferData(GL_ARRAY_BUFFER, 12 * 2 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

        glBindVertexArray(static_cast<unsigned int>(previousVao));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
    }



    void OpenGLRenderBackend::drawSelectionOutline( const RenderFrame& frame ) {
        const SelectionOutlineSettings& settings = frame.selectionOutline;

        if (!settings.enabled) {
            return;
        }

        const ProjectedSelectionOutline outline = SelectionOutlineGeometry::create(
            settings.blockMin, settings.expansion, frame.view, frame.projection
        );

        if (outline.count == 0) {
            return;
        }

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

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glLineWidth(1.0f);

        glUseProgram(outlineProgram_);

        constexpr float scale = 1.0f / 255.0f;
        const GLint colorLocation = glGetUniformLocation(outlineProgram_, "uColor");
        glUniform4f(colorLocation, getRed(settings.color) * scale,
                    getGreen(settings.color) * scale, getBlue(settings.color) * scale,
                    getAlpha(settings.color) * scale);

        glBindVertexArray(outlineVao_);
        glBindBuffer(GL_ARRAY_BUFFER, outlineVbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, outline.count * 6 * sizeof(float), vertices.data());
        glDrawArrays(GL_LINES, 0, static_cast<int>(outline.count * 2));

        glBindVertexArray(static_cast<unsigned int>(previousVao));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
        glUseProgram(static_cast<unsigned int>(previousProgram));
        glDepthFunc(static_cast<unsigned int>(previousDepthFunc));
        glDepthMask(previousDepthMask);
        glLineWidth(previousLineWidth);

        if (previousDepthTest == GL_TRUE) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }



    void OpenGLRenderBackend::initialize() {
        if (initialized_) {
            throw std::runtime_error("OpenGLRenderBackend is already initialized");
        }

        renderer_.initialize();
        demo_.initialize(sourceMesh_, sourceTexture_);
        initializeOutlineResources();

        initialized_ = true;

        std::cout << "[OpenGLBackend] Initialized with Mesh ID Cache and Selection Outline" << std::endl;
    }



    RenderSize OpenGLRenderBackend::framebufferSize() const {
        GLFWwindow* context = glfwGetCurrentContext();

        if (context == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(context, &width, &height);

        return RenderSize{ width, height };
    }



    const OpenGLMesh& OpenGLRenderBackend::resolveMesh( const MeshData& mesh,
                                                        std::uint64_t meshId, std::uint64_t revision ) {
        if (&mesh == &sourceMesh_) {
            return demo_.mesh();
        }

        if (meshId == 0) {
            throw std::invalid_argument("Cached OpenGL Mesh requires a nonzero Mesh ID");
        }

        const auto found = uploadedMeshes_.find(meshId);

        if (found != uploadedMeshes_.end() && found->second.revision == revision) {
            return *found->second.mesh;
        }

        auto newGpuMesh = std::make_unique<OpenGLMesh>();
        newGpuMesh->initialize(mesh);

        if (found == uploadedMeshes_.end()) {
            CachedMesh entry{};
            entry.mesh = std::move(newGpuMesh);
            entry.revision = revision;

            auto [iterator, inserted] = uploadedMeshes_.emplace(meshId, std::move(entry));
            (void)inserted;

            std::cout << "[OpenGLBackend] Mesh uploaded"
                      << " | ID " << meshId << " | Revision " << revision
                      << " | " << mesh.vertexCount() << " vertices" << std::endl;

            return *iterator->second.mesh;
        }

        found->second.mesh = std::move(newGpuMesh);
        found->second.revision = revision;

        std::cout << "[OpenGLBackend] Mesh reuploaded"
                  << " | ID " << meshId << " | Revision " << revision
                  << " | " << mesh.vertexCount() << " vertices" << std::endl;

        return *found->second.mesh;
    }



    void OpenGLRenderBackend::releaseUnusedMeshes( const std::unordered_set<std::uint64_t>& activeIds ) {
        for (auto iterator = uploadedMeshes_.begin(); iterator != uploadedMeshes_.end();) {
            if (activeIds.find(iterator->first) != activeIds.end()) {
                ++iterator;
                continue;
            }

            const std::uint64_t releasedId = iterator->first;
            iterator = uploadedMeshes_.erase(iterator);

            std::cout << "[OpenGLBackend] Mesh released" << " | ID " << releasedId << std::endl;
        }
    }



    void OpenGLRenderBackend::render( const RenderFrame& frame ) {
        if (!initialized_) {
            throw std::runtime_error("OpenGLRenderBackend is not initialized");
        }

        const RenderSize size = framebufferSize();

        if (size.width <= 0 || size.height <= 0) {
            return;
        }

        std::unordered_set<std::uint64_t> activeIds;
        std::unordered_map<std::uint64_t, std::pair<const MeshData*, std::uint64_t>> activeMeshes;

        for (const RenderDraw& draw : frame.draws) {
            if (draw.mesh == nullptr) {
                throw std::invalid_argument("OpenGL Draw requires MeshData");
            }

            if (draw.mesh == &sourceMesh_) {
                continue;
            }

            if (draw.meshId == 0) {
                throw std::invalid_argument("Chunk Draw requires a nonzero Mesh ID");
            }

            if (draw.mesh->indexCount() == 0) {
                continue;
            }

            const auto [iterator, inserted] = activeMeshes.emplace(
                draw.meshId, std::make_pair(draw.mesh, draw.meshRevision)
            );

            if (!inserted && (iterator->second.first != draw.mesh ||
                              iterator->second.second != draw.meshRevision)) {
                throw std::invalid_argument("Conflicting Mesh ID in RenderFrame");
            }

            activeIds.insert(draw.meshId);
        }

        releaseUnusedMeshes(activeIds);

        constexpr float colorScale = 1.0f / 255.0f;
        OpenGLClearCommand clearCommand{};
        clearCommand.red = static_cast<float>(getRed(frame.clearColor)) * colorScale;
        clearCommand.green = static_cast<float>(getGreen(frame.clearColor)) * colorScale;
        clearCommand.blue = static_cast<float>(getBlue(frame.clearColor)) * colorScale;
        clearCommand.alpha = static_cast<float>(getAlpha(frame.clearColor)) * colorScale;
        clearCommand.clearDepth = true;
        renderer_.submit(clearCommand);

        for (const RenderDraw& draw : frame.draws) {
            if (draw.mesh->indexCount() == 0) {
                continue;
            }

            const OpenGLMesh& gpuMesh = resolveMesh(*draw.mesh, draw.meshId, draw.meshRevision);

            OpenGLDrawCommand command{};
            command.mesh = &gpuMesh;
            command.material = &demo_.material();
            command.model = draw.model;

            renderer_.submit(command, frame.view, frame.projection);
        }

        // The selection is a depth-tested 3D overlay; the crosshair is a 2D overlay.
        drawSelectionOutline(frame);
        drawCrosshair(frame.crosshair, size.width, size.height);

        window_.swapBuffers();
    }
}
