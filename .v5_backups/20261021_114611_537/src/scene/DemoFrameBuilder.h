#pragma once

#include "render/api/RenderBackend.h"
#include "render/api/RenderFrame.h"


namespace ve
{
    class DemoScene;
    class DebugOverlayTelemetry;


    class DemoFrameBuilder {
        public:
            static RenderFrame build( const DemoScene& scene, int width, int height,
                                      bool cursorCaptured, bool debugOverlayEnabled,
                                      RenderBackend backend, const DebugOverlayTelemetry& telemetry );
    };
}
