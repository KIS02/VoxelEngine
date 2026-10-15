
#pragma once

#include <cstddef>
#include <optional>
#include <utility>

#include "render/pipeline/PipelineTrianglePicker.h"


namespace ve
{
    enum class PipelineInspectorStage {
        Local,
        World,
        View,
        Clip,
        Clipping,
        Ndc,
        Screen,
        Rasterization,
        Fragment
    };


    class PipelineInspectorController {
        private:
            static constexpr std::size_t StageCount = 9;

            bool enabled_ = false;

            PipelineInspectorStage stage_ = PipelineInspectorStage::Local;
            std::optional<PipelineTrace> selectedTrace_{};


        public:
            bool isEnabled() const {
                return enabled_;
            }

            bool hasSelection() const {
                return selectedTrace_.has_value();
            }

            PipelineInspectorStage stage() const {
                return stage_;
            }

            std::size_t stageIndex() const {
                return static_cast<std::size_t>(stage_);
            }

            static constexpr std::size_t stageCount() {
                return StageCount;
            }

            const std::optional<PipelineTrace>& selectedTrace() const {
                return selectedTrace_;
            }


            void open() {
                enabled_ = true;
            }

            void close() {
                enabled_ = false;
            }

            void toggle() {
                enabled_ = !enabled_;
            }

            void clearSelection() {
                selectedTrace_.reset();
                stage_ = PipelineInspectorStage::Local;
            }


            // Pick a Triangle Using the Current Framebuffer Pixel

            bool selectAtPixel( const RenderFrame& frame, int framebufferWidth,
                                int framebufferHeight, int pixelX, int pixelY ) {
                if (!enabled_)
                    return false;

                std::optional<PipelineTrace> candidate = PipelineTrianglePicker::pick(
                    frame, framebufferWidth, framebufferHeight, pixelX, pixelY
                );

                if (!candidate.has_value())
                    return false;


                // Commit the New Frozen Trace Only After Picking Succeeds

                selectedTrace_ = std::move(candidate);
                stage_ = PipelineInspectorStage::Local;

                return true;
            }


            void nextStage() {
                if (!enabled_ || !selectedTrace_.has_value())
                    return;

                const std::size_t index = stageIndex();

                if (index + 1 >= StageCount)
                    return;

                stage_ = static_cast<PipelineInspectorStage>(index + 1);
            }

            void previousStage() {
                if (!enabled_ || !selectedTrace_.has_value())
                    return;

                const std::size_t index = stageIndex();

                if (index == 0)
                    return;

                stage_ = static_cast<PipelineInspectorStage>(index - 1);
            }


            static const char* stageName( PipelineInspectorStage stage ) {
                switch (stage) {
                    case PipelineInspectorStage::Local:
                        return "LOCAL SPACE";

                    case PipelineInspectorStage::World:
                        return "WORLD SPACE";

                    case PipelineInspectorStage::View:
                        return "VIEW SPACE";

                    case PipelineInspectorStage::Clip:
                        return "CLIP SPACE";

                    case PipelineInspectorStage::Clipping:
                        return "CLIPPING";

                    case PipelineInspectorStage::Ndc:
                        return "NDC";

                    case PipelineInspectorStage::Screen:
                        return "SCREEN SPACE";

                    case PipelineInspectorStage::Rasterization:
                        return "RASTERIZATION";

                    case PipelineInspectorStage::Fragment:
                        return "FRAGMENT";
                }

                return "UNKNOWN";
            }
    };
}
