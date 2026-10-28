#include "render/frame/FrameBuilder.h"

#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>

#include "assets/AssetManager.h"
#include "assets/material/MaterialData.h"
#include "assets/texture/Texture2D.h"
#include "scene/Scene.h"
#include "scene/camera/Camera.h"
#include "scene/lighting/SceneLightCollector.h"
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
        frame.toneMapping = scene.toneMappingSettings();
        frame.clearColor = makeColor(20, 31, 77);
        SceneLightCollector::collect(scene.objects(), frame.lighting);


        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        frame.view = scene.camera().viewMatrix();
        frame.projection = scene.camera().projectionMatrix(aspectRatio);
        frame.cameraPosition = scene.camera().position();


        // Voxel Material Submesh Draws

        static bool materialProbePrinted = false;
        std::map<BlockMaterialKey, std::pair<std::uint64_t, std::uint64_t>> materialBindings;
        std::uint64_t voxelDrawCount = 0;

        if (const VoxelWorld* voxel = scene.voxelWorld()) {
            voxel->meshes().forEachMaterialMesh(
                [&frame, &scene, &assets, &materialBindings, &voxelDrawCount](
                    const ChunkPosition& position, BlockMaterialKey key, const MeshData& mesh,
                    std::uint64_t meshId, std::uint64_t revision ) {
                    const std::uint64_t materialId = scene.voxelMaterialId(key);

                    if (materialId == 0)
                        throw std::logic_error("Voxel Submesh requires a Material");

                    const MaterialData& material = assets.material(materialId);
                    if (material.texture == nullptr)
                        throw std::logic_error("Voxel Submesh Material has no Texture");

                    materialBindings.emplace(key, std::make_pair(materialId, material.texture->id()));
                    ++voxelDrawCount;

                    const Vec3 origin{
                        static_cast<float>(position.x * Chunk::Size),
                        static_cast<float>(position.y * Chunk::Size),
                        static_cast<float>(position.z * Chunk::Size)
                    };

                    frame.draws.push_back(RenderDraw{
                        &mesh, Mat4::translation(origin), revision, meshId,
                        &material
                    });
                }
            );
        }


        // Scene Object Draws

        scene.objects().forEach([&frame, &assets]( const SceneObject& object ) {
            if (object.meshAssetId == 0 || object.materialAssetId == 0)
                return;

            constexpr float Radians = 3.14159265358979323846f / 180.0f;
            const Vec3& r = object.rotationDegrees;

            const Mat4 model =
                Mat4::translation(object.position) *
                Mat4::rotationZ(r.z * Radians) *
                Mat4::rotationY(r.y * Radians) *
                Mat4::rotationX(r.x * Radians) *
                Mat4::scale(object.scale);

            frame.draws.push_back(RenderDraw{
                &assets.mesh(object.meshAssetId), model, 1,
                object.meshAssetId, &assets.material(object.materialAssetId)
            });
        });


        // UI Data

        ui.appendToFrame(
            frame, scene.camera(), scene.selectedBlock(), cursorCaptured, backend
        );

        return frame;
    }
}
