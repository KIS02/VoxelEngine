#include "render/backends/software/pipeline/geometry/SoftwareGeometryPass.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/Vertex.h"
#include "assets/texture/Texture2D.h"

#include "render/backends/software/ClipSpaceClipper.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/RasterVertex.h"
#include "render/backends/software/VertexProcessor.h"
#include "render/backends/software/Viewport.h"
#include "render/frame/RenderFrame.h"
#include "render/lighting/Lighting.h"
#include "render/lighting/NormalMatrix3.h"


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
                                     const Viewport& viewport ) {
        std::vector<ClipVertex> clipVertices;
        std::array<ClipVertex, 16> clippedPolygon{};
        std::array<RasterVertex, 16> rasterPolygon{};


        // The light is supplied by the FrameBuilder, not by the Backend.
        const DirectionalLight& light = frame.lighting.directional;


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
                        &frame.cameraPosition
                    );
                }
            }
        }
    }
}
