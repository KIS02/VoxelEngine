
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "benchmark/BenchmarkTypes.h"

#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>


namespace ve
{
    class OpenGLGpuTimer {
        private:
            static constexpr std::size_t QueryCount = 8;

            struct QuerySlot
            {
                GLuint id = 0;
                std::uint64_t frameId = 0;
            };


            std::array<QuerySlot, QueryCount> slots_{};

            std::size_t head_ = 0;
            std::size_t pendingCount_ = 0;

            std::uint64_t skippedCount_ = 0;

            bool initialized_ = false;
            bool active_ = false;


        public:
            OpenGLGpuTimer() = default;

            ~OpenGLGpuTimer() {
                if (glfwGetCurrentContext() != nullptr)
                    shutdown();
            }

            OpenGLGpuTimer( const OpenGLGpuTimer& ) = delete;
            OpenGLGpuTimer& operator=( const OpenGLGpuTimer& ) = delete;
            OpenGLGpuTimer( OpenGLGpuTimer&& ) = delete;
            OpenGLGpuTimer& operator=( OpenGLGpuTimer&& ) = delete;


            void initialize() {
                if (initialized_)
                    throw std::logic_error("OpenGLGpuTimer is already initialized");

                if (glfwGetCurrentContext() == nullptr)
                    throw std::runtime_error("OpenGLGpuTimer requires a current OpenGL context");

                std::array<GLuint, QueryCount> ids{};
                glGenQueries(static_cast<GLsizei>(QueryCount), ids.data());

                for (GLuint id : ids) {
                    if (id == 0) {
                        for (GLuint createdId : ids) {
                            if (createdId != 0)
                                glDeleteQueries(1, &createdId);
                        }

                        throw std::runtime_error("Failed to create OpenGL timer queries");
                    }
                }

                for (std::size_t i = 0; i < QueryCount; ++i)
                    slots_[i].id = ids[i];

                head_ = 0;
                pendingCount_ = 0;
                skippedCount_ = 0;

                active_ = false;
                initialized_ = true;
            }


            void shutdown() {
                if (!initialized_)
                    return;

                if (active_) {
                    glEndQuery(GL_TIME_ELAPSED);
                    active_ = false;
                }

                for (QuerySlot& slot : slots_) {
                    if (slot.id != 0)
                        glDeleteQueries(1, &slot.id);

                    slot = QuerySlot{};
                }

                head_ = 0;
                pendingCount_ = 0;
                initialized_ = false;
            }


            bool begin( std::uint64_t frameId ) {
                if (!initialized_)
                    throw std::logic_error("OpenGLGpuTimer is not initialized");

                if (active_)
                    throw std::logic_error("A GPU timing query is already active");

                if (pendingCount_ == QueryCount) {
                    ++skippedCount_;
                    return false;
                }

                const std::size_t index = (head_ + pendingCount_) % QueryCount;

                slots_[index].frameId = frameId;

                glBeginQuery(GL_TIME_ELAPSED, slots_[index].id);

                ++pendingCount_;
                active_ = true;

                return true;
            }


            void end() {
                if (!active_)
                    throw std::logic_error("No GPU timing query is active");

                glEndQuery(GL_TIME_ELAPSED);
                active_ = false;
            }


            bool tryCollect( std::uint64_t& frameId, BenchmarkGpuTiming& timing ) {
                if (!initialized_ || pendingCount_ == 0 || active_)
                    return false;

                QuerySlot& slot = slots_[head_];

                GLuint available = GL_FALSE;
                glGetQueryObjectuiv(slot.id, GL_QUERY_RESULT_AVAILABLE, &available);

                if (available != GL_TRUE)
                    return false;

                GLuint64 elapsedNanoseconds = 0;
                glGetQueryObjectui64v(slot.id, GL_QUERY_RESULT, &elapsedNanoseconds);

                frameId = slot.frameId;

                timing.state = BenchmarkGpuTimingState::Available;
                timing.elapsedMs = static_cast<double>(elapsedNanoseconds) / 1000000.0;

                slot.frameId = 0;

                head_ = (head_ + 1) % QueryCount;
                --pendingCount_;

                return true;
            }


            std::uint64_t skippedCount() const {
                return skippedCount_;
            }
    };
}
