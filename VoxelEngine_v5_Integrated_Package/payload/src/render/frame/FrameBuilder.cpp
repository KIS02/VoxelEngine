#include "render/frame/FrameBuilder.h"

#include <cmath>
#include <stdexcept>

#include "assets/AssetManager.h"
#include "scene/Scene.h"
#include "scene/camera/Camera.h"
#include "scene/voxel/VoxelWorld.h"
#include "ui/UIManager.h"
#include "voxel/chunk/Chunk.h"


namespace ve
{
    RenderFrame FrameBuilder::build( const Scene& scene, const AssetManager& assets,
                                     const UIManager& ui, int width, int height,
                                     bool cursorCaptured, RenderBackend backend ) {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument("Invalid framebuffer dimensions");

        RenderFrame frame{};
        frame.clearColor = makeColor(20, 31, 77);
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        frame.view = scene.camera().viewMatrix();
        frame.projection = scene.camera().projectionMatrix(aspectRatio);

        if (const VoxelWorld* voxel = scene.voxelWorld()) {
            const std::uint64_t materialId = scene.voxelMaterialId();
            if (materialId == 0)
                throw std::logic_error("Voxel scene requires a shared material");

            const MaterialData& material = assets.material(materialId);
            voxel->meshes().forEachMesh([&frame, &material]( const ChunkPosition& position,
                                                              const MeshData& mesh, std::uint64_t meshId,
                                                              std::uint64_t revision ) {
                const Vec3 origin{
                    static_cast<float>(position.x * Chunk::Size),
                    static_cast<float>(position.y * Chunk::Size),
                    static_cast<float>(position.z * Chunk::Size)
                };
                frame.draws.push_back(RenderDraw{
                    &mesh, Mat4::translation(origin), revision, meshId, &material
                });
            });
        }

        scene.objects().forEach([&frame, &assets]( const SceneObject& object ) {
            if (object.meshAssetId == 0 || object.materialAssetId == 0)
                return;

            constexpr float Radians = 3.14159265358979323846f / 180.0f;
            const Vec3& r = object.rotationDegrees;
            const Mat4 model = Mat4::translation(object.position) *
                               Mat4::rotationZ(r.z * Radians) *
                               Mat4::rotationY(r.y * Radians) *
                               Mat4::rotationX(r.x * Radians) * Mat4::scale(object.scale);
            frame.draws.push_back(RenderDraw{
                &assets.mesh(object.meshAssetId), model, 1,
                object.meshAssetId, &assets.material(object.materialAssetId)
            });
        });

        ui.appendToFrame(frame, scene.camera(), scene.selectedBlock(), cursorCaptured, backend);
        return frame;
    }

}
