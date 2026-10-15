
#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "render/pipeline/PipelineTrace.h"


namespace ve
{
    class PipelineTransformTracer {
        private:
            static bool isFinite( const Vec3& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
            }

            static bool isFinite( const Vec4& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y) &&
                       std::isfinite(value.z) && std::isfinite(value.w);
            }

            static bool isFinite( const Mat4& matrix ) {
                for (int row = 0; row < 4; ++row) {
                    for (int column = 0; column < 4; ++column) {
                        if (!std::isfinite(matrix(row, column)))
                            return false;
                    }
                }

                return true;
            }


        public:
            static bool trace( PipelineTrace& result ) {
                if (!result.triangle.has_value())
                    return false;

                const PipelineTriangleSnapshot& snapshot = *result.triangle;

                if (snapshot.framebufferWidth <= 0 || snapshot.framebufferHeight <= 0)
                    return false;

                if (!isFinite(snapshot.model) || !isFinite(snapshot.view) ||
                    !isFinite(snapshot.projection))
                    return false;


                // Calculate All Three Vertices Before Committing

                std::array<Vec4, 3> world{};
                std::array<Vec4, 3> view{};
                std::array<Vec4, 3> clip{};

                for (std::size_t i = 0; i < 3; ++i) {
                    const Vec3& position = snapshot.vertices[i].position;

                    if (!isFinite(position))
                        return false;


                    // Local Space -> World Space

                    const Vec4 local{ position.x, position.y, position.z, 1.0f };

                    world[i] = snapshot.model * local;

                    if (!isFinite(world[i]))
                        return false;


                    // World Space -> View Space

                    view[i] = snapshot.view * world[i];

                    if (!isFinite(view[i]))
                        return false;


                    // View Space -> Clip Space

                    clip[i] = snapshot.projection * view[i];

                    if (!isFinite(clip[i]))
                        return false;
                }


                // Commit the Completed Transform Results

                result.worldPositions = world;
                result.viewPositions = view;
                result.clipPositions = clip;


                // Invalidate Later Pipeline Stages

                result.clippedPolygon.reset();
                result.ndcPolygon.reset();
                result.screenPolygon.reset();
                result.pixelSample.reset();


                // These Values Are CPU Reference Calculations

                result.origin = PipelineTraceOrigin::CpuReference;

                return true;
            }
    };
}
