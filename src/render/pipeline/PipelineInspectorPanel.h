
#pragma once

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

#include "render/RenderFrame.h"
#include "render/pipeline/PipelineInspectorController.h"


namespace ve
{
    class PipelineInspectorPanel {
        private:
            static std::string number( float value ) {
                std::ostringstream stream;
                stream << std::fixed << std::setprecision(2) << value;
                return stream.str();
            }

            static void appendVec3( std::vector<std::string>& lines, std::size_t index,
                                    const Vec3& value ) {
                const std::string label = "V" + std::to_string(index);

                lines.push_back(label + " X " + number(value.x) + " Y " + number(value.y));
                lines.push_back("   Z " + number(value.z));
            }

            static void appendVec4( std::vector<std::string>& lines, std::size_t index,
                                    const Vec4& value ) {
                const std::string label = "V" + std::to_string(index);

                lines.push_back(label + " X " + number(value.x) + " Y " + number(value.y));
                lines.push_back("   Z " + number(value.z) + " W " + number(value.w));
            }

            static void appendVec2( std::vector<std::string>& lines, std::size_t index,
                                    const Vec2& value ) {
                lines.push_back(
                    "V" + std::to_string(index) + " X " + number(value.x) +
                    " Y " + number(value.y)
                );
            }


        public:
            static void populate( RenderFrame& frame, const PipelineInspectorController& controller ) {
                if (!controller.isEnabled())
                    return;

                PipelineInspectorSettings& inspector = frame.pipelineInspector;

                inspector.panel.enabled = true;

                std::vector<std::string>& lines = inspector.panel.lines;

                lines.push_back("PIPELINE INSPECTOR");
                lines.push_back("CPU REFERENCE TRACE");
                lines.push_back("F4 CLOSE  ARROWS STEP");
                lines.push_back("-----------------------");

                if (!controller.hasSelection()) {
                    lines.push_back("NO TRIANGLE SELECTED");
                    lines.push_back("CLICK THE 3D SCENE");
                    lines.push_back("CAMERA IS FROZEN");
                    return;
                }

                const PipelineTrace& trace = *controller.selectedTrace();
                const PipelineTriangleSnapshot& triangle = *trace.triangle;

                lines.push_back("MESH ID " + std::to_string(triangle.identity.meshId));
                lines.push_back("REVISION " + std::to_string(triangle.identity.meshRevision));
                lines.push_back("DRAW " + std::to_string(triangle.identity.drawIndex) +
                                " TRI " + std::to_string(triangle.identity.triangleIndex));
                lines.push_back("-----------------------");
                lines.push_back(
                    "STAGE " + std::to_string(controller.stageIndex() + 1) + "/9"
                );
                lines.push_back(PipelineInspectorController::stageName(controller.stage()));

                if (trace.screenPolygon.has_value()) {
                    for (const PipelineScreenVertex& vertex : *trace.screenPolygon)
                        inspector.outline.push_back(vertex.position);
                }

                switch (controller.stage()) {
                    case PipelineInspectorStage::Local: {
                        for (std::size_t i = 0; i < 3; ++i)
                            appendVec3(lines, i, triangle.vertices[i].position);

                        break;
                    }

                    case PipelineInspectorStage::World: {
                        if (trace.worldPositions.has_value()) {
                            for (std::size_t i = 0; i < 3; ++i)
                                appendVec4(lines, i, (*trace.worldPositions)[i]);
                        }

                        break;
                    }

                    case PipelineInspectorStage::View: {
                        if (trace.viewPositions.has_value()) {
                            for (std::size_t i = 0; i < 3; ++i)
                                appendVec4(lines, i, (*trace.viewPositions)[i]);
                        }

                        break;
                    }

                    case PipelineInspectorStage::Clip: {
                        if (trace.clipPositions.has_value()) {
                            for (std::size_t i = 0; i < 3; ++i)
                                appendVec4(lines, i, (*trace.clipPositions)[i]);
                        }

                        break;
                    }

                    case PipelineInspectorStage::Clipping: {
                        if (trace.clippedPolygon.has_value()) {
                            lines.push_back("VERTICES " + std::to_string(trace.clippedPolygon->size()));

                            if (trace.clippedPolygon->empty())
                                lines.push_back("FULLY CLIPPED");

                            for (std::size_t i = 0; i < trace.clippedPolygon->size(); ++i)
                                appendVec4(lines, i, (*trace.clippedPolygon)[i].position);
                        }

                        break;
                    }

                    case PipelineInspectorStage::Ndc: {
                        if (trace.ndcPolygon.has_value()) {
                            for (std::size_t i = 0; i < trace.ndcPolygon->size(); ++i)
                                appendVec3(lines, i, (*trace.ndcPolygon)[i]);
                        }

                        break;
                    }

                    case PipelineInspectorStage::Screen: {
                        if (trace.screenPolygon.has_value()) {
                            for (std::size_t i = 0; i < trace.screenPolygon->size(); ++i)
                                appendVec2(lines, i, (*trace.screenPolygon)[i].position);
                        }

                        break;
                    }

                    case PipelineInspectorStage::Rasterization: {
                        if (trace.pixelSample.has_value()) {
                            const PipelinePixelSample& sample = *trace.pixelSample;

                            lines.push_back("PIXEL " + std::to_string(sample.x) +
                                            "," + std::to_string(sample.y));
                            lines.push_back(sample.covered ? "COVERED YES" : "COVERED NO");

                            if (sample.covered) {
                                appendVec3(lines, 0, sample.barycentric);

                                lines.push_back("UV " + number(sample.uv.x) +
                                                "," + number(sample.uv.y));

                                lines.push_back("DEPTH " + number(sample.incomingDepth));
                            }
                        }

                        break;
                    }

                    case PipelineInspectorStage::Fragment: {
                        lines.push_back("DEPTH BEFORE DRAW");
                        lines.push_back("NOT CAPTURED");
                        lines.push_back("GPU FRAGMENT DATA N/A");
                        lines.push_back("REFERENCE TRACE ONLY");
                        break;
                    }
                }

                lines.push_back("-----------------------");
                lines.push_back("CLICK TO SELECT AGAIN");
            }
    };
}
