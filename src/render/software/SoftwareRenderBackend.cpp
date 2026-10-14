#include "render/software/SoftwareRenderBackend.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "platform/windows/Win32FramePresenter.h"

#include "render/ClipSpaceClipper.h"
#include "render/Color.h"
#include "render/DepthBuffer.h"
#include "render/FrameBuffer.h"
#include "render/MeshData.h"
#include "render/Rasterizer.h"
#include "render/RasterVertex.h"
#include "render/RenderFrame.h"
#include "render/Texture2D.h"
#include "render/Vertex.h"
#include "render/VertexProcessor.h"
#include "render/Viewport.h"

#include "render/overlay/CrosshairGeometry.h"
#include "render/overlay/SelectionOutlineGeometry.h"



namespace ve
{
    namespace
    {
        bool isFinite( const Vec4& p ) {
            return std::isfinite(p.x) && std::isfinite(p.y) &&
                   std::isfinite(p.z) && std::isfinite(p.w);
        }



        void drawCrosshair( FrameBuffer& frameBuffer, const CrosshairSettings& settings ) {
            if (!settings.enabled) {
                return;
            }

            const auto rectangles = CrosshairGeometry::create(
                frameBuffer.width(), frameBuffer.height(), settings.armLength, settings.thickness
            );

            for (const ScreenRect& rectangle : rectangles) {
                for (int y = rectangle.y; y < rectangle.y + rectangle.height; ++y) {
                    for (int x = rectangle.x; x < rectangle.x + rectangle.width; ++x) {
                        frameBuffer.setPixel(x, y, settings.color);
                    }
                }
            }
        }



        void drawDepthTestedLine( FrameBuffer& frameBuffer, const DepthBuffer& depthBuffer,
                                  const RasterVertex& a, const RasterVertex& b, Color32 color ) {
            const float dx = b.position.x - a.position.x;
            const float dy = b.position.y - a.position.y;
            const float length = std::max(std::fabs(dx), std::fabs(dy));

            const int steps = static_cast<int>(std::ceil(length));
            constexpr float depthTolerance = 0.0002f;

            for (int i = 0; i <= steps; ++i) {
                const float t = steps == 0 ? 0.0f : static_cast<float>(i) / static_cast<float>(steps);
                const float xFloat = a.position.x + dx * t;
                const float yFloat = a.position.y + dy * t;
                const float depth = a.depth + (b.depth - a.depth) * t;

                const int x = static_cast<int>(std::floor(xFloat));
                const int y = static_cast<int>(std::floor(yFloat));

                if (x < 0 || y < 0 || x >= frameBuffer.width() || y >= frameBuffer.height()) {
                    continue;
                }

                if (depth < 0.0f || depth > 1.0f) {
                    continue;
                }

                if (depth <= depthBuffer.depth(x, y) + depthTolerance) {
                    frameBuffer.setPixel(x, y, color);
                }
            }
        }



        void drawSelectionOutlineSoftware( FrameBuffer& frameBuffer, const DepthBuffer& depthBuffer,
                                        const Viewport& viewport, const RenderFrame& frame ) {
            const SelectionOutlineSettings& settings = frame.selectionOutline;

            if (!settings.enabled) {
                return;
            }

            const ProjectedSelectionOutline outline = SelectionOutlineGeometry::create(
                settings.blockMin, settings.expansion, frame.view, frame.projection
            );

            for (std::size_t i = 0; i < outline.count; ++i) {
                const ProjectedLine& line = outline.lines[i];

                const RasterVertex a = viewport.transform(line.start, settings.color);
                const RasterVertex b = viewport.transform(line.end, settings.color);

                drawDepthTestedLine(frameBuffer, depthBuffer, a, b, settings.color);
            }
        }
    }



    SoftwareRenderBackend::SoftwareRenderBackend( FrameBuffer& frameBuffer, DepthBuffer& depthBuffer,
                                                  Rasterizer& rasterizer, Viewport& viewport,
                                                  Win32FramePresenter& presenter, const Texture2D& texture )
        : frameBuffer_(frameBuffer), depthBuffer_(depthBuffer), rasterizer_(rasterizer),
          viewport_(viewport), presenter_(presenter), texture_(texture) { }



    void SoftwareRenderBackend::initialize() {
        std::cout << "[SoftwareBackend] Initialized with Clip Space triangle clipping" << std::endl;
    }



    RenderSize SoftwareRenderBackend::framebufferSize() const {
        return RenderSize{ frameBuffer_.width(), frameBuffer_.height() };
    }



    void SoftwareRenderBackend::render( const RenderFrame& frame ) {
        frameBuffer_.clear(frame.clearColor);
        depthBuffer_.clear(1.0f);

        std::vector<ClipVertex> clipVertices;
        std::array<ClipVertex, 16> clippedPolygon{};
        std::array<RasterVertex, 16> rasterPolygon{};

        for (const RenderDraw& draw : frame.draws) {
            if (draw.mesh == nullptr) {
                throw std::invalid_argument("Software Draw requires MeshData");
            }

            const std::vector<Vertex>& vertices = draw.mesh->vertices();
            const std::vector<std::uint32_t>& indices = draw.mesh->indices();

            if (indices.size() % 3 != 0) {
                throw std::invalid_argument("Software Mesh indices must contain complete triangles");
            }

            const Mat4 modelViewProjection = frame.projection * frame.view * draw.model;

            clipVertices.resize(vertices.size());

            for (std::size_t i = 0; i < vertices.size(); ++i) {
                Vertex vertex = vertices[i];
                vertex.color = makeColor(255, 255, 255);

                clipVertices[i] = VertexProcessor::toClipSpace(vertex, modelViewProjection);

                if (!isFinite(clipVertices[i].position)) {
                    throw std::invalid_argument("Non-finite Clip Space vertex");
                }
            }

            for (std::size_t i = 0; i < indices.size(); i += 3) {
                const std::uint32_t indexA = indices[i];
                const std::uint32_t indexB = indices[i + 1];
                const std::uint32_t indexC = indices[i + 2];

                if (indexA >= vertices.size() || indexB >= vertices.size() || indexC >= vertices.size()) {
                    throw std::out_of_range("Software Mesh index exceeds vertex count");
                }

                const std::size_t count = ClipSpaceClipper::clipTriangle(
                    clipVertices[indexA], clipVertices[indexB], clipVertices[indexC], clippedPolygon
                );

                if (count < 3) {
                    continue;
                }

                bool valid = true;

                for (std::size_t j = 0; j < count; ++j) {
                    const ClipVertex& vertex = clippedPolygon[j];

                    if (!isFinite(vertex.position) || vertex.position.w <= 0.0f) {
                        valid = false;
                        break;
                    }

                    rasterPolygon[j] = VertexProcessor::toRasterSpace(vertex, viewport_);

                    if (!std::isfinite(rasterPolygon[j].position.x) ||
                        !std::isfinite(rasterPolygon[j].position.y) ||
                        !std::isfinite(rasterPolygon[j].depth)) {
                        valid = false;
                        break;
                    }
                }

                if (!valid) {
                    continue;
                }

                for (std::size_t j = 1; j + 1 < count; ++j) {
                    rasterizer_.drawTriangleTextured(
                        rasterPolygon[0], rasterPolygon[j], rasterPolygon[j + 1], texture_
                    );
                }
            }
        }

        // World outline needs the 3D depth buffer but must not modify it.
        drawSelectionOutlineSoftware(frameBuffer_, depthBuffer_, viewport_, frame);

        // The 2D crosshair must remain visible over both the world and the outline.
        drawCrosshair(frameBuffer_, frame.crosshair);

        presenter_.present(frameBuffer_);
    }
}
