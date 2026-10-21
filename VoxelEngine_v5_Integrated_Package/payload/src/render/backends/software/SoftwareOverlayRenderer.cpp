#include "render/backends/software/SoftwareOverlayRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/RasterVertex.h"
#include "render/backends/software/Viewport.h"

#include "render/frame/RenderFrame.h"
#include "render/overlay/CrosshairGeometry.h"
#include "render/overlay/SelectionOutlineGeometry.h"


namespace ve
{
    namespace
    {
        void drawDepthTestedLine( FrameBuffer& frameBuffer, const DepthBuffer& depthBuffer,
                                  const RasterVertex& a, const RasterVertex& b, Color32 color ) {
            const float dx = b.position.x - a.position.x;
            const float dy = b.position.y - a.position.y;
            const float length = std::max(std::fabs(dx), std::fabs(dy));

            const int steps = static_cast<int>(std::ceil(length));
            constexpr float depthTolerance = 0.0002f;

            for (int i = 0; i <= steps; ++i) {
                const float t = steps == 0 ? 0.0f : static_cast<float>(i) / static_cast<float>(steps);
                const float xFloat = a.position.x + dx * t;
                const float yFloat = a.position.y + dy * t;
                const float depth = a.depth + (b.depth - a.depth) * t;

                const int x = static_cast<int>(std::floor(xFloat));
                const int y = static_cast<int>(std::floor(yFloat));

                if (x < 0 || y < 0 || x >= frameBuffer.width() || y >= frameBuffer.height()) {
                    continue;
                }

                if (depth < 0.0f || depth > 1.0f) {
                    continue;
                }

                if (depth <= depthBuffer.depth(x, y) + depthTolerance) {
                    frameBuffer.setPixel(x, y, color);
                }
            }
        }
    }


    void SoftwareOverlayRenderer::drawCrosshair( FrameBuffer& frameBuffer, const CrosshairSettings& settings ) {
            if (!settings.enabled) {
                return;
            }

            const auto rectangles = CrosshairGeometry::create(
                frameBuffer.width(), frameBuffer.height(), settings.armLength, settings.thickness
            );

            for (const ScreenRect& rectangle : rectangles) {
                for (int y = rectangle.y; y < rectangle.y + rectangle.height; ++y) {
                    for (int x = rectangle.x; x < rectangle.x + rectangle.width; ++x) {
                        frameBuffer.setPixel(x, y, settings.color);
                    }
                }
            }
        }


    void SoftwareOverlayRenderer::drawSelectionOutline( FrameBuffer& frameBuffer, const DepthBuffer& depthBuffer,
                                        const Viewport& viewport, const RenderFrame& frame ) {
            const SelectionOutlineSettings& settings = frame.selectionOutline;

            if (!settings.enabled) {
                return;
            }

            const ProjectedSelectionOutline outline = SelectionOutlineGeometry::create(
                settings.blockMin, settings.expansion, frame.view, frame.projection
            );

            for (std::size_t i = 0; i < outline.count; ++i) {
                const ProjectedLine& line = outline.lines[i];

                const RasterVertex a = viewport.transform(line.start, settings.color);
                const RasterVertex b = viewport.transform(line.end, settings.color);

                drawDepthTestedLine(frameBuffer, depthBuffer, a, b, settings.color);
            }
        }
}
