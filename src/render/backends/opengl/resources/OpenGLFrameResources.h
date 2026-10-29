#pragma once

#include <cstdint>
#include <stdexcept>
#include <unordered_set>


namespace ve
{
    class OpenGLFrameResources {
        private:
            std::unordered_set<std::uint64_t> meshIds_;
            std::unordered_set<std::uint64_t> textureIds_;


        public:
            void track( std::uint64_t meshId, std::uint64_t textureId ) {
                if (meshId == 0 || textureId == 0)
                    throw std::invalid_argument("GPU resource IDs must be nonzero");

                meshIds_.insert(meshId);
                textureIds_.insert(textureId);
            }

            void trackTexture( std::uint64_t textureId ) {
                if (textureId == 0)
                    throw std::invalid_argument("GPU Texture ID must be nonzero");
                textureIds_.insert(textureId);
            }

            const std::unordered_set<std::uint64_t>& meshIds() const {
                return meshIds_;
            }

            const std::unordered_set<std::uint64_t>& textureIds() const {
                return textureIds_;
            }
    };
}
