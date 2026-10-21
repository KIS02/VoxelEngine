#pragma once

#include <optional>

#include "debug/DebugOverlayTelemetry.h"
#include "render/api/RenderBackend.h"
#include "render/frame/RenderFrame.h"
#include "voxel/ray/VoxelRaycaster.h"


namespace ve
{
    class Camera;

    // Stores presentation state, not scene/gameplay state.
    class UIManager {
        private:
            DebugOverlayTelemetry telemetry_;
            bool debugEnabled_ = false;

        public:
            void toggleDebug();
            void setDebugEnabled( bool enabled );
            bool debugEnabled() const;
            void recordFrame( const BenchmarkCpuTimings& timings,
                              const BenchmarkRenderStatistics& statistics );
            void observeGpuTiming( const BenchmarkGpuTiming& timing );
            void appendToFrame( RenderFrame& frame, const Camera& camera,
                                std::optional<VoxelRaycastHit> selected,
                                bool cursorCaptured, RenderBackend backend ) const;
    };
}
