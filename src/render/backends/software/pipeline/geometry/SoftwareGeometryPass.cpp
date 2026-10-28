#include "render/backends/software/pipeline/geometry/SoftwareGeometryPass.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/Vertex.h"
#include "assets/texture/Texture2D.h"

#include "render/backends/software/ClipSpaceClipper.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/resources/HdrColorBuffer.h"
#include "render/backends/software/RasterVertex.h"
#include "render/backends/software/VertexProcessor.h"
#include "render/backends/software/Viewport.h"
#include "render/frame/RenderFrame.h"
#include "render/lighting/Lighting.h"
#include "render/lighting/NormalMatrix3.h"
#include "render/shadow/ShadowFrameAdapter.h"
#include "render/shadow/ShadowFrameFingerprint.h"


namespace ve
{
    namespace
    {
        bool isFinite( const Vec4& p ) {
            return std::isfinite(p.x) && std::isfinite(p.y) &&
                   std::isfinite(p.z) && std::isfinite(p.w);
        }
    }


    void SoftwareGeometryPass::draw( const RenderFrame& frame, Rasterizer& rasterizer,
                                     const Viewport& viewport, HdrColorBuffer* hdr ) {
        std::vector<ClipVertex> clipVertices;
        std::array<ClipVertex, 16> clippedPolygon{};
        std::array<RasterVertex, 16> rasterPolygon{};
        using Clock = std::chrono::steady_clock;
        const auto startTime = Clock::now();
        bool shadowCacheHit = false;
        validateSceneLights(frame.lighting);


        // Keep a per-render-thread shadow map while geometry, lights, and camera stay unchanged.
        struct CachedShadowFrame
        {
            ShadowFrame frame;
            std::uint64_t fingerprint = 0;
            bool valid = false;
        };

        static thread_local CachedShadowFrame shadowCache{};
        const ShadowVisibility* shadows = nullptr;

        for (const SceneLightData& source : frame.lighting.sceneLights) {
            if (!source.castsShadows)
                continue;

            ShadowSettings settings{};
            settings.resolution = 384;
            settings.pcfRadius = 0;
            // A small world-space receiver offset combats self-shadowing without
            // removing block-scale cast shadows as a large depth bias would.
            settings.normalOffset = 0.05f;
            if (const char* requested = std::getenv("VOXELENGINE_SOFTWARE_SHADOW_OFFSET")) {
                char* end = nullptr;
                const float value = std::strtof(requested, &end);
                if (end == requested || *end != '\0' || !std::isfinite(value) ||
                    value < 0.0f || value > 0.25f)
                    throw std::invalid_argument("Invalid Software Shadow receiver offset");
                settings.normalOffset = value;
            }

            const std::uint64_t fingerprint = ShadowFrameFingerprint::calculate(frame, settings);
            if (!shadowCache.valid || shadowCache.fingerprint != fingerprint) {
                ShadowFrame rebuilt = ShadowFrameAdapter::build(frame, settings);
                shadowCache.frame = std::move(rebuilt);
                shadowCache.fingerprint = fingerprint;
                shadowCache.valid = true;
            } else {
                shadowCacheHit = true;
            }

            if (!shadowCache.frame.lights.empty())
                shadows = &shadowCache.frame;
            break;
        }

        const auto shadowEndTime = Clock::now();
        const DirectionalLight& light = frame.lighting.directional;


        // Render Every Draw

        for (const RenderDraw& draw : frame.draws) {
            if (draw.mesh == nullptr)
                throw std::invalid_argument("Software Draw requires MeshData");

            if (draw.material == nullptr || draw.material->texture == nullptr)
                throw std::invalid_argument("Software Draw requires a Material with Texture");

            validateMaterialLighting(*draw.material);
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
                        vertex, viewport
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
                    rasterizer.drawTriangleTextured(
                        rasterPolygon[0],
                        rasterPolygon[j],
                        rasterPolygon[j + 1],
                        *sampledTexture,
                        &light,
                        draw.material,
                        &frame.cameraPosition,
                        &frame.lighting,
                        shadows,
                        hdr,
                        true
                    );
                }
            }
        }

        static thread_local std::uint64_t profileFrame = 0;
        if (std::getenv("VOXELENGINE_PROFILE_RENDER") != nullptr && (++profileFrame % 10 == 0)) {
            const auto endTime = Clock::now();
            const double shadowMs = std::chrono::duration<double, std::milli>(shadowEndTime - startTime).count();
            const double rasterMs = std::chrono::duration<double, std::milli>(endTime - shadowEndTime).count();
            std::cout << "[Profile][Software] Shadow=" << shadowMs << " ms | Geometry="
                      << rasterMs << " ms | ShadowCache=" << (shadowCacheHit ? "HIT" : "MISS") << '\n';
        }
    }
}
