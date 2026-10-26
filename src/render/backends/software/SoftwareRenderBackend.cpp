#include "render/backends/software/SoftwareRenderBackend.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "platform/windows/Win32FramePresenter.h"

#include "math/Color.h"
#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/Vertex.h"
#include "assets/texture/Texture2D.h"

#include "render/backends/software/ClipSpaceClipper.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/RasterVertex.h"
#include "render/backends/software/VertexProcessor.h"
#include "render/backends/software/Viewport.h"

#include "render/frame/RenderFrame.h"
#include "render/lighting/NormalMatrix3.h"
#include "render/lighting/Lighting.h"

#include "render/backends/software/SoftwareOverlayRenderer.h"
#include "render/overlay/SoftwareDebugOverlay.h"

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    namespace
    {
        bool isFinite( const Vec4& p ) {
            return std::isfinite(p.x) && std::isfinite(p.y) &&
                   std::isfinite(p.z) && std::isfinite(p.w);
        }
    }


    SoftwareRenderBackend::SoftwareRenderBackend( FrameBuffer& frameBuffer, DepthBuffer& depthBuffer,
                                                  Rasterizer& rasterizer, Viewport& viewport,
                                                  Win32FramePresenter& presenter )
        : frameBuffer_(frameBuffer), depthBuffer_(depthBuffer), rasterizer_(rasterizer),
          viewport_(viewport), presenter_(presenter) { }


    void SoftwareRenderBackend::initialize() {
        std::cout << "[SoftwareBackend] Initialized with Clip Space triangle clipping" << std::endl;
    }


    RenderSize SoftwareRenderBackend::framebufferSize() const {
        return RenderSize{ frameBuffer_.width(), frameBuffer_.height() };
    }


    void SoftwareRenderBackend::render( const RenderFrame& frame ) {
        BenchmarkCpuTimings unusedTimings{};

        BenchmarkCpuTimings& timings = measuredCpuTimings() != nullptr
            ? *measuredCpuTimings() : unusedTimings;


        // CPU Rendering Work

        {
            BenchmarkCpuScope renderScope(timings.rendererWorkMs);

            frameBuffer_.clear(frame.clearColor);
            depthBuffer_.clear(1.0f);

            std::vector<ClipVertex> clipVertices;
            std::array<ClipVertex, 16> clippedPolygon{};
            std::array<RasterVertex, 16> rasterPolygon{};


            // Shared Directional Light

            const DirectionalLight light{
                Vec3{ -0.45f, 1.0f, 0.35f }, 0.25f, 0.75f
            };


            // Render Every Draw

            for (const RenderDraw& draw : frame.draws) {
                if (draw.mesh == nullptr)
                    throw std::invalid_argument("Software Draw requires MeshData");

                if (draw.material == nullptr || draw.material->texture == nullptr)
                    throw std::invalid_argument("Software Draw requires a Material with Texture");

                const Texture2D* sampledTexture = draw.material->texture;

                const std::vector<Vertex>& vertices = draw.mesh->vertices();
                const std::vector<std::uint32_t>& indices = draw.mesh->indices();

                if (indices.size() % 3 != 0)
                    throw std::invalid_argument("Software Mesh indices must contain complete triangles");


                // Prepare Transformation Matrices

                const Mat4 modelViewProjection = frame.projection * frame.view * draw.model;
                const NormalMatrix3 normalMatrix = calculateNormalMatrix(draw.model);

                clipVertices.resize(vertices.size());

                for (std::size_t i = 0; i < vertices.size(); ++i) {
                    Vertex vertex = vertices[i];
                    vertex.color = makeColor(255, 255, 255);

                    clipVertices[i] = VertexProcessor::toClipSpace(
                        vertex, draw.model, modelViewProjection, normalMatrix
                    );

                    if (!isFinite(clipVertices[i].position))
                        throw std::invalid_argument("Non-finite Clip Space vertex");
                }

                // Process Indexed Triangles

                for (std::size_t i = 0; i < indices.size(); i += 3) {
                    const std::uint32_t indexA = indices[i];
                    const std::uint32_t indexB = indices[i + 1];
                    const std::uint32_t indexC = indices[i + 2];

                    if (indexA >= vertices.size() || indexB >= vertices.size() ||
                        indexC >= vertices.size())
                        throw std::out_of_range("Software Mesh index exceeds vertex count");


                    // Clip Triangle Against View Frustum

                    const std::size_t count = ClipSpaceClipper::clipTriangle(
                        clipVertices[indexA],
                        clipVertices[indexB],
                        clipVertices[indexC],
                        clippedPolygon
                    );

                    if (count < 3)
                        continue;


                    // Convert Clipped Polygon to Screen Space

                    bool valid = true;

                    for (std::size_t j = 0; j < count; ++j) {
                        const ClipVertex& vertex = clippedPolygon[j];

                        if (!isFinite(vertex.position) || vertex.position.w <= 0.0f) {
                            valid = false;
                            break;
                        }

                        rasterPolygon[j] = VertexProcessor::toRasterSpace(
                            vertex, viewport_
                        );

                        if (!std::isfinite(rasterPolygon[j].position.x) ||
                            !std::isfinite(rasterPolygon[j].position.y) ||
                            !std::isfinite(rasterPolygon[j].depth)) {
                            valid = false;
                            break;
                        }
                    }

                    if (!valid)
                        continue;


                    // Rasterize Clipped Polygon as a Triangle Fan

                    for (std::size_t j = 1; j + 1 < count; ++j) {
                        rasterizer_.drawTriangleTextured(
                            rasterPolygon[0],
                            rasterPolygon[j],
                            rasterPolygon[j + 1],
                            *sampledTexture,
                            &light,
                            draw.material,
                            &frame.cameraPosition
                        );
                    }
                }
            }


            // Depth-tested 3D Selection Outline

            SoftwareOverlayRenderer::drawSelectionOutline(
                frameBuffer_, depthBuffer_, viewport_, frame
            );


            // Screen-space 2D Crosshair

            SoftwareOverlayRenderer::drawCrosshair(
                frameBuffer_, frame.ui.crosshair
            );


            // Screen-space F3 Debug Overlay

            SoftwareDebugOverlay::draw(
                frameBuffer_, frame.ui.debugOverlay
            );
        }


        // Win32 Frame Presentation

        {
            BenchmarkCpuScope presentScope(timings.presentMs);
            presenter_.present(frameBuffer_);
        }
    }
}