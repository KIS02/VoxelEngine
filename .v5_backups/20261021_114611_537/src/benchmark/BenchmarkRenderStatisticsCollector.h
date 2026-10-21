
#pragma once

#include <cstdint>
#include <stdexcept>

#include "benchmark/BenchmarkTypes.h"

#include "assets/mesh/MeshData.h"
#include "render/api/RenderFrame.h"


namespace ve
{
    class BenchmarkRenderStatisticsCollector {
        public:
            static BenchmarkRenderStatistics collect( const RenderFrame& frame,
                                                      std::uint64_t residentChunkCount ) {
                BenchmarkRenderStatistics statistics{};

                statistics.residentChunkCount = residentChunkCount;


                for (const RenderDraw& draw : frame.draws) {
                    if (draw.mesh == nullptr)
                        throw std::invalid_argument("Benchmark Draw requires MeshData");

                    const std::size_t indexCount = draw.mesh->indexCount();

                    if (indexCount % 3 != 0)
                        throw std::invalid_argument("Benchmark Mesh requires complete triangles");

                    if (indexCount == 0)
                        continue;


                    ++statistics.meshDrawsSubmitted;

                    statistics.meshVerticesReferenced +=
                        static_cast<std::uint64_t>(draw.mesh->vertexCount());

                    statistics.meshIndicesSubmitted +=
                        static_cast<std::uint64_t>(indexCount);
                }


                statistics.trianglesSubmitted = statistics.meshIndicesSubmitted / 3;

                return statistics;
            }
    };
}
