#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "assets/mesh/MeshIdGenerator.h"
#include "assets/mesh/Vertex.h"
#include "math/Color.h"
#include "voxel/chunk/ChunkMeshCache.h"

#if __has_include("render/Vertex.h")
#error Duplicate render/Vertex.h must not be reintroduced
#endif

#if __has_include("render/Color.h")
#error Shared colors must not depend on the rendering layer
#endif

#if __has_include("render/MeshIDGenerator.h")
#error VoxelStorage mesh IDs must not depend on the rendering layer
#endif


int main() {
    static_assert(std::is_standard_layout_v<ve::Vertex>);
    static_assert(std::is_same_v<ve::Color32, std::uint32_t>);

    ve::MeshIdGenerator generator{};
    ve::ChunkMeshCache meshes(generator);
    unsigned visits = 0;
    meshes.forEachMesh([&visits]( const ve::ChunkPosition&, const ve::MeshData&,
                                  std::uint64_t, std::uint64_t ) {
        ++visits;
    });

    if (visits != 0 || generator.generate() == 0)
        throw std::runtime_error("R6 dependency boundary contract failed");
    return 0;
}
