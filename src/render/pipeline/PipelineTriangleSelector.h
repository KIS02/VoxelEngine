
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "render/MeshData.h"
#include "render/RenderFrame.h"
#include "render/pipeline/PipelineTrace.h"


namespace ve
{
    class PipelineTriangleSelector {
        private:
            std::optional<PipelineTriangleSnapshot> snapshot_{};


        public:
            bool select( const RenderFrame& frame, int framebufferWidth, int framebufferHeight,
                         std::size_t drawIndex, std::size_t triangleIndex ) {
                if (framebufferWidth <= 0 || framebufferHeight <= 0)
                    return false;

                if (drawIndex >= frame.draws.size())
                    return false;


                // Find the Requested RenderDraw

                const RenderDraw& draw = frame.draws[drawIndex];

                if (draw.mesh == nullptr)
                    return false;


                // Read the Original Mesh Data

                const std::vector<Vertex>& vertices = draw.mesh->vertices();
                const std::vector<std::uint32_t>& indices = draw.mesh->indices();

                if (indices.size() % 3 != 0)
                    return false;

                if (triangleIndex >= indices.size() / 3)
                    return false;


                // Resolve Triangle Indices

                const std::size_t firstIndex = triangleIndex * 3;

                const std::array<std::uint32_t, 3> vertexIndices{
                    indices[firstIndex],
                    indices[firstIndex + 1],
                    indices[firstIndex + 2]
                };

                for (std::uint32_t index : vertexIndices) {
                    if (static_cast<std::size_t>(index) >= vertices.size())
                        return false;
                }


                // Create an Independent Triangle Snapshot

                PipelineTriangleSnapshot captured{};

                captured.identity.drawIndex = drawIndex;
                captured.identity.triangleIndex = triangleIndex;
                captured.identity.meshId = draw.meshId;
                captured.identity.meshRevision = draw.meshRevision;
                captured.identity.vertexIndices = vertexIndices;


                // Copy Position, UV and Color of Each Vertex

                for (std::size_t i = 0; i < 3; ++i) {
                    const Vertex& vertex = vertices[vertexIndices[i]];

                    captured.vertices[i].position = vertex.position;
                    captured.vertices[i].uv = vertex.uv;
                    captured.vertices[i].color = vertex.color;
                }


                // Copy Transformation Matrices

                captured.model = draw.model;
                captured.view = frame.view;
                captured.projection = frame.projection;


                // Record the Original Viewport Dimensions

                captured.framebufferWidth = framebufferWidth;
                captured.framebufferHeight = framebufferHeight;


                // Commit Only a Fully Valid Selection

                snapshot_ = captured;

                return true;
            }


            const std::optional<PipelineTriangleSnapshot>& snapshot() const {
                return snapshot_;
            }


            bool hasSelection() const {
                return snapshot_.has_value();
            }


            void clear() {
                snapshot_.reset();
            }
    };
}
