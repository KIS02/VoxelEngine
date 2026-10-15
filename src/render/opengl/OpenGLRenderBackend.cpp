
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

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkTypes.h"

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


        void drawCrosshair( const CrosshairSettings& settings, int width, int height ) {
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


    OpenGLRenderBackend::OpenGLRenderBackend( Window& window, OpenGLRenderer& renderer,
                                             OpenGLCubeDemo& demo, const MeshData& sourceMesh,
                                             const Texture2D& sourceTexture )
        : window_(window), renderer_(renderer), demo_(demo),
          sourceMesh_(sourceMesh), sourceTexture_(sourceTexture) { }


    OpenGLRenderBackend::~OpenGLRenderBackend() {
        if (glfwGetCurrentContext() == nullptr)
            return;

        if (outlineVbo_ != 0)
            glDeleteBuffers(1, &outlineVbo_);

        if (outlineVao_ != 0)
            glDeleteVertexArrays(1, &outlineVao_);

        if (outlineProgram_ != 0)
            glDeleteProgram(outlineProgram_);
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


    void OpenGLRenderBackend::drawSelectionOutline( const RenderFrame& frame ) {
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




    void OpenGLRenderBackend::drawInspectorOutline( const RenderFrame& frame, int width, int height ) {
        const std::vector<Vec2>& polygon = frame.pipelineInspector.outline;

        if (!frame.pipelineInspector.panel.enabled || polygon.size() < 3)
            return;

        if (width <= 0 || height <= 0 || polygon.size() > 24)
            return;


        // Convert Screen-space Points Into OpenGL NDC

        std::array<float, 24 * 3> vertices{};

        for (std::size_t i = 0; i < polygon.size(); ++i) {
            const Vec2& point = polygon[i];

            vertices[i * 3 + 0] = point.x / static_cast<float>(width) * 2.0f - 1.0f;
            vertices[i * 3 + 1] = 1.0f - point.y / static_cast<float>(height) * 2.0f;
            vertices[i * 3 + 2] = 0.0f;
        }


        // Preserve Existing OpenGL State

        GLint previousProgram = 0;
        GLint previousVao = 0;
        GLint previousBuffer = 0;

        GLfloat previousLineWidth = 1.0f;
        GLboolean previousDepthMask = GL_TRUE;

        const GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean previousBlend = glIsEnabled(GL_BLEND);
        const GLboolean previousCullFace = glIsEnabled(GL_CULL_FACE);
        const GLboolean previousScissor = glIsEnabled(GL_SCISSOR_TEST);
        const GLboolean previousStencil = glIsEnabled(GL_STENCIL_TEST);

        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);

        glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);


        // Draw Highlight Without Changing the Depth Buffer

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_STENCIL_TEST);

        glDepthMask(GL_FALSE);
        glLineWidth(1.0f);

        glUseProgram(outlineProgram_);

        const GLint colorLocation = glGetUniformLocation(outlineProgram_, "uColor");

        glUniform4f(colorLocation, 1.0f, 230.0f / 255.0f, 70.0f / 255.0f, 1.0f);

        glBindVertexArray(outlineVao_);
        glBindBuffer(GL_ARRAY_BUFFER, outlineVbo_);

        glBufferSubData(
            GL_ARRAY_BUFFER, 0,
            polygon.size() * 3 * sizeof(float),
            vertices.data()
        );

        glDrawArrays(GL_LINE_LOOP, 0, static_cast<int>(polygon.size()));


        // Restore Previous OpenGL State

        glBindVertexArray(static_cast<unsigned int>(previousVao));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
        glUseProgram(static_cast<unsigned int>(previousProgram));

        glLineWidth(previousLineWidth);
        glDepthMask(previousDepthMask);

        if (previousDepthTest == GL_TRUE)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        if (previousBlend == GL_TRUE)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        if (previousCullFace == GL_TRUE)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);

        if (previousScissor == GL_TRUE)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);

        if (previousStencil == GL_TRUE)
            glEnable(GL_STENCIL_TEST);
        else
            glDisable(GL_STENCIL_TEST);
    }


    

    void OpenGLRenderBackend::initialize() {
        if (initialized_)
            throw std::runtime_error("OpenGLRenderBackend is already initialized");


        // Initialize OpenGL Renderer

        renderer_.initialize();

        demo_.initialize(sourceMesh_, sourceTexture_);

        initializeOutlineResources();


        // Initialize GPU Timer Query Pool

        gpuTimer_.initialize();

        initialized_ = true;


        // Diagnostics

        std::cout << "[OpenGLBackend] Initialized with Mesh ID Cache and Selection Outline"
                  << std::endl;

        std::cout << "[Benchmark][GPU] OpenGL Timer Query initialized"
                  << std::endl;
    }


    RenderSize OpenGLRenderBackend::framebufferSize() const {
        GLFWwindow* context = glfwGetCurrentContext();

        if (context == nullptr)
            throw std::runtime_error("No current OpenGL context");

        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(context, &width, &height);

        return RenderSize{ width, height };
    }


    const OpenGLMesh& OpenGLRenderBackend::resolveMesh( const MeshData& mesh,
                                                        std::uint64_t meshId, std::uint64_t revision ) {
        if (&mesh == &sourceMesh_)
            return demo_.mesh();

        if (meshId == 0)
            throw std::invalid_argument("Cached OpenGL Mesh requires a nonzero Mesh ID");


        const auto found = uploadedMeshes_.find(meshId);

        if (found != uploadedMeshes_.end() && found->second.revision == revision)
            return *found->second.mesh;


        // Create Updated GPU Mesh

        auto newGpuMesh = std::make_unique<OpenGLMesh>();
        newGpuMesh->initialize(mesh);


        // First Upload

        if (found == uploadedMeshes_.end()) {
            CachedMesh entry{};

            entry.mesh = std::move(newGpuMesh);
            entry.revision = revision;

            auto [iterator, inserted] = uploadedMeshes_.emplace(meshId, std::move(entry));
            (void)inserted;


            // GPU Resource Statistics

            if (BenchmarkRenderStatistics* statistics = measuredRenderStatistics())
                ++statistics->gpuMeshUploadCount;


            std::cout << "[OpenGLBackend] Mesh uploaded"
                      << " | ID " << meshId
                      << " | Revision " << revision
                      << " | " << mesh.vertexCount() << " vertices"
                      << std::endl;

            return *iterator->second.mesh;
        }


        // Mesh Revision Changed

        found->second.mesh = std::move(newGpuMesh);
        found->second.revision = revision;


        // GPU Resource Statistics

        if (BenchmarkRenderStatistics* statistics = measuredRenderStatistics())
            ++statistics->gpuMeshUploadCount;


        std::cout << "[OpenGLBackend] Mesh reuploaded"
                  << " | ID " << meshId
                  << " | Revision " << revision
                  << " | " << mesh.vertexCount() << " vertices"
                  << std::endl;

        return *found->second.mesh;
    }


    void OpenGLRenderBackend::releaseUnusedMeshes( const std::unordered_set<std::uint64_t>& activeIds ) {
        for (auto iterator = uploadedMeshes_.begin(); iterator != uploadedMeshes_.end();) {
            if (activeIds.find(iterator->first) != activeIds.end()) {
                ++iterator;
                continue;
            }


            // Release Missing Mesh

            const std::uint64_t releasedId = iterator->first;

            iterator = uploadedMeshes_.erase(iterator);


            // GPU Resource Statistics

            if (BenchmarkRenderStatistics* statistics = measuredRenderStatistics())
                ++statistics->gpuMeshReleaseCount;


            std::cout << "[OpenGLBackend] Mesh released"
                      << " | ID " << releasedId
                      << std::endl;
        }
    }


    void OpenGLRenderBackend::render( const RenderFrame& frame ) {
        BenchmarkCpuTimings unusedTimings{};

        BenchmarkCpuTimings& timings = measuredCpuTimings() != nullptr
            ? *measuredCpuTimings() : unusedTimings;


        // CPU-side OpenGL Rendering Work

        {
            BenchmarkCpuScope renderScope(timings.rendererWorkMs);

            if (!initialized_)
                throw std::runtime_error("OpenGLRenderBackend is not initialized");


            // Check Framebuffer Size

            const RenderSize size = framebufferSize();

            if (size.width <= 0 || size.height <= 0)
                return;


            // Validate Active Meshes

            std::unordered_set<std::uint64_t> activeIds;
            std::unordered_map<std::uint64_t, std::pair<const MeshData*, std::uint64_t>> activeMeshes;

            for (const RenderDraw& draw : frame.draws) {
                if (draw.mesh == nullptr)
                    throw std::invalid_argument("OpenGL Draw requires MeshData");

                if (draw.mesh == &sourceMesh_)
                    continue;

                if (draw.meshId == 0)
                    throw std::invalid_argument("Chunk Draw requires a nonzero Mesh ID");

                if (draw.mesh->indexCount() == 0)
                    continue;

                const auto [iterator, inserted] = activeMeshes.emplace(
                    draw.meshId, std::make_pair(draw.mesh, draw.meshRevision)
                );

                if (!inserted && (iterator->second.first != draw.mesh ||
                                  iterator->second.second != draw.meshRevision))
                    throw std::invalid_argument("Conflicting Mesh ID in RenderFrame");

                activeIds.insert(draw.meshId);
            }


            // Release Unused GPU Meshes

            releaseUnusedMeshes(activeIds);


            // Determine Original Frame ID

            const std::uint64_t frameId = measuredFrameId() != 0
                ? measuredFrameId() : ++directRenderFrameId_;


            // Start Non-blocking GPU Timer Query

            const bool gpuQueryStarted = gpuTimer_.begin(frameId);

            if (!gpuQueryStarted && (gpuTimer_.skippedCount() == 1 ||
                                     gpuTimer_.skippedCount() % 120 == 0)) {

                std::cout << "[Benchmark][GPU] Query pool full"
                          << " | Skipped samples " << gpuTimer_.skippedCount()
                          << std::endl;
            }


            try {
                // Clear Color and Depth

                constexpr float colorScale = 1.0f / 255.0f;

                OpenGLClearCommand clearCommand{};

                clearCommand.red = static_cast<float>(getRed(frame.clearColor)) * colorScale;
                clearCommand.green = static_cast<float>(getGreen(frame.clearColor)) * colorScale;
                clearCommand.blue = static_cast<float>(getBlue(frame.clearColor)) * colorScale;
                clearCommand.alpha = static_cast<float>(getAlpha(frame.clearColor)) * colorScale;
                clearCommand.clearDepth = true;

                renderer_.submit(clearCommand);


                // Draw World and Reference Cube

                for (const RenderDraw& draw : frame.draws) {
                    if (draw.mesh->indexCount() == 0)
                        continue;

                    const OpenGLMesh& gpuMesh = resolveMesh(
                        *draw.mesh, draw.meshId, draw.meshRevision
                    );

                    OpenGLDrawCommand command{};

                    command.mesh = &gpuMesh;
                    command.material = &demo_.material();
                    command.model = draw.model;

                    renderer_.submit(command, frame.view, frame.projection);
                }


                // Depth-tested 3D Selection Outline

                drawSelectionOutline(frame);


                // Screen-space 2D Crosshair

                drawCrosshair(frame.crosshair, size.width, size.height);



                // Screen-space F3 Debug HUD

                debugOverlay_.draw(frame.debugOverlay, size.width, size.height);


                // Highlight the Selected Pipeline Polygon

                drawInspectorOutline(frame, size.width, size.height);


                // Right-hand F4 Pipeline Inspector

                debugOverlay_.draw(frame.pipelineInspector.panel, size.width, size.height, true);

            } catch (...) {
                if (gpuQueryStarted)
                    gpuTimer_.end();

                throw;
            }


            // End GPU Timing Before Buffer Presentation

            if (gpuQueryStarted)
                gpuTimer_.end();
        }


        // OpenGL Buffer Presentation

        {
            BenchmarkCpuScope presentScope(timings.presentMs);
            window_.swapBuffers();
        }
    }


    bool OpenGLRenderBackend::tryPopGpuTiming( std::uint64_t& frameId, BenchmarkGpuTiming& timing ) {
        return gpuTimer_.tryCollect(frameId, timing);
    }
}
