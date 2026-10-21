
#include "render/backends/opengl/OpenGLRenderBackend.h"

#include <array>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/Window.h"

#include "render/common/DefaultTextureShaders.h"
#include "render/common/MaterialCachePolicy.h"

#include "math/Color.h"
#include "assets/mesh/MeshData.h"
#include "render/api/RenderFrame.h"
#include "assets/material/MaterialData.h"
#include "assets/texture/Texture2D.h"

#include "render/backends/opengl/OpenGLOverlayRenderer.h"

#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/OpenGLRenderer.h"

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
        std::vector<std::uint8_t> toRGBA8( const Texture2D& texture ) {
            if (texture.width() <= 0 || texture.height() <= 0)
                throw std::invalid_argument("OpenGL Material Texture dimensions are invalid");

            const std::size_t count = static_cast<std::size_t>(texture.width()) *
                                      static_cast<std::size_t>(texture.height());
            std::vector<std::uint8_t> bytes(count * 4);

            for (int y = 0; y < texture.height(); ++y) {
                for (int x = 0; x < texture.width(); ++x) {
                    const Color32 color = texture.pixel(x, y);
                    const std::size_t offset =
                        (static_cast<std::size_t>(y) * static_cast<std::size_t>(texture.width()) +
                         static_cast<std::size_t>(x)) * 4;
                    bytes[offset + 0] = getRed(color);
                    bytes[offset + 1] = getGreen(color);
                    bytes[offset + 2] = getBlue(color);
                    bytes[offset + 3] = getAlpha(color);
                }
            }
            return bytes;
        }





    }


    OpenGLRenderBackend::OpenGLRenderBackend( Window& window, OpenGLRenderer& renderer,
                                             const Texture2D& sourceTexture )
        : window_(window), renderer_(renderer), sourceTexture_(sourceTexture) { }











    OpenGLRenderBackend::~OpenGLRenderBackend() = default;


    void OpenGLRenderBackend::initialize() {
        if (initialized_)
            throw std::runtime_error("OpenGLRenderBackend is already initialized");


        // Initialize OpenGL Renderer

        renderer_.initialize();


        overlayRenderer_.initialize();


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


    OpenGLMaterial& OpenGLRenderBackend::resolveMaterial( const Texture2D& texture ) {
        const auto found = materials_.find(texture.id());
        const MaterialCacheAction action = MaterialCachePolicy::decide(
            found != materials_.end(), found != materials_.end() ? found->second.revision : 0,
            texture.revision()
        );

        if (action == MaterialCacheAction::Reuse)
            return *found->second.material;

        std::vector<std::uint8_t> pixels = toRGBA8(texture);
        auto replacement = std::make_unique<OpenGLMaterial>();
        replacement->initialize(DefaultTextureVertexShader, DefaultTextureFragmentShader,
                                texture.width(), texture.height(), pixels.data());

        const std::uint64_t revision = texture.revision();
        if (found != materials_.end()) {
            found->second.material = std::move(replacement);
            found->second.revision = revision;
            return *found->second.material;
        }

        CachedMaterial entry{};
        entry.material = std::move(replacement);
        entry.revision = revision;
        return *materials_.emplace(texture.id(), std::move(entry)).first->second.material;
    }


    const OpenGLMesh& OpenGLRenderBackend::resolveMesh( const MeshData& mesh,
                                                        std::uint64_t meshId, std::uint64_t revision ) {
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


    void OpenGLRenderBackend::releaseUnusedMaterials( const std::unordered_set<std::uint64_t>& activeIds ) {
        for (auto iterator = materials_.begin(); iterator != materials_.end();) {
            if (!MaterialCachePolicy::shouldRelease(iterator->first, activeIds)) {
                ++iterator;
                continue;
            }

            iterator = materials_.erase(iterator);
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

                if (draw.meshId == 0)
                    throw std::invalid_argument("OpenGL Draw requires a nonzero Mesh ID");

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


            // Collect Active Texture IDs Before Releasing Cached GPU Materials

            std::unordered_set<std::uint64_t> activeTextureIds;

            for (const RenderDraw& draw : frame.draws) {
                if (draw.mesh->indexCount() == 0)
                    continue;

                const Texture2D* texture = &sourceTexture_;
                if (draw.material != nullptr) {
                    if (draw.material->texture == nullptr)
                        throw std::invalid_argument("OpenGL Draw Material requires a Texture");

                    texture = draw.material->texture;
                }

                activeTextureIds.insert(texture->id());
            }


            // Release Unused GPU Meshes and GPU Materials

            releaseUnusedMeshes(activeIds);
            releaseUnusedMaterials(activeTextureIds);


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
                    const Texture2D* texture = &sourceTexture_;
                    if (draw.material != nullptr) {
                        if (draw.material->texture == nullptr)
                            throw std::invalid_argument("OpenGL Draw Material requires a Texture");
                        texture = draw.material->texture;
                    }
                    command.material = &resolveMaterial(*texture);
                    command.model = draw.model;

                    renderer_.submit(command, frame.view, frame.projection);
                }


                // Depth-tested 3D Selection Outline

                overlayRenderer_.drawSelectionOutline(frame);


                // Screen-space 2D Crosshair

                overlayRenderer_.drawCrosshair(frame.crosshair, size.width, size.height);


                // Screen-space F3 Debug HUD

                debugOverlay_.draw(frame.debugOverlay, size.width, size.height);
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
