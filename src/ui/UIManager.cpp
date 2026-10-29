#include "ui/UIManager.h"

#include <cmath>
#include <sstream>

#include "assets/block/BlockName.h"
#include "scene/camera/Camera.h"
#include "voxel/chunk/Chunk.h"


namespace ve
{
    void UIManager::toggleDebug() {
        debugEnabled_ = !debugEnabled_;
    }

    void UIManager::setDebugEnabled( bool enabled ) {
        debugEnabled_ = enabled;
    }

    bool UIManager::debugEnabled() const {
        return debugEnabled_;
    }

    void UIManager::recordFrame( const BenchmarkCpuTimings& timings,
                                 const BenchmarkRenderStatistics& statistics ) {
        telemetry_.recordFrame(timings, statistics);
    }

    void UIManager::observeGpuTiming( const BenchmarkGpuTiming& timing ) {
        telemetry_.observeGpuTiming(timing);
    }

    void UIManager::appendToFrame( RenderFrame& frame, const Camera& camera,
                                   std::optional<VoxelRaycastHit> selected,
                                   bool cursorCaptured, RenderBackend backend ) const {
        frame.ui.crosshair.enabled = cursorCaptured;
        frame.ui.crosshair.color = makeColor(255, 255, 255);
        frame.ui.crosshair.armLength = 7;
        frame.ui.crosshair.thickness = 3;

        if (cursorCaptured && selected.has_value()) {
            const WorldBlockPosition& block = selected->block;
            frame.selectionOutline.enabled = true;
            frame.selectionOutline.blockMin = Vec3{
                static_cast<float>(block.x), static_cast<float>(block.y), static_cast<float>(block.z)
            };
            frame.selectionOutline.color = makeColor(255, 0, 255);
            frame.selectionOutline.expansion = 0.03f;
        }

        if (!debugEnabled_)
            return;

        DebugOverlayScene overlay{};
        overlay.backend = backend == RenderBackend::OpenGL
            ? BenchmarkBackend::OpenGL : BenchmarkBackend::Software;
        const Vec3 position = camera.position();
        overlay.cameraPosition = { position.x, position.y, position.z };
        const float chunkSize = static_cast<float>(Chunk::Size);
        overlay.chunkPosition = {
            static_cast<int>(std::floor(position.x / chunkSize)),
            static_cast<int>(std::floor(position.y / chunkSize)),
            static_cast<int>(std::floor(position.z / chunkSize))
        };
        if (selected.has_value() && cursorCaptured) {
            const WorldBlockPosition& block = selected->block;
            std::ostringstream target;
            target << blockTypeName(selected->type)
                   << " (" << block.x << ", " << block.y << ", " << block.z << ")";
            overlay.targetBlock = target.str();
        }
        frame.ui.debugOverlay.enabled = true;
        frame.ui.debugOverlay.lines = telemetry_.makeLines(overlay);
        frame.ui.debugOverlay.lines.push_back(frame.shadingModel == ShadingModel::Pbr
            ? "SHADING: PBR" : "SHADING: BLINN-PHONG");
        frame.ui.debugOverlay.lines.push_back(
            frame.voxelTextureSetIndex == 0 ? "VOXEL TEXTURES: DEFAULT" :
            frame.voxelTextureSetIndex == 1 ? "VOXEL TEXTURES: SET 1" : "VOXEL TEXTURES: SET 2"
        );
    }
}
