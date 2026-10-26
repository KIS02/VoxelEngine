#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/texture/Texture2D.h"

#include "math/Color.h"
#include "math/Mat4.h"

#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLMesh.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/OpenGLRenderer.h"

#include "render/backends/software/ClipSpaceClipper.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/VertexProcessor.h"
#include "render/backends/software/Viewport.h"

#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
#include "render/frame/RenderFrame.h"
#include "render/lighting/Lighting.h"
#include "assets/texture/ImageLoader.h"


namespace
{
    constexpr int Width = 33;
    constexpr int Height = 33;
    constexpr int Center = 16;
    constexpr int Tolerance = 2;


    class HiddenContext {
        private:
            GLFWwindow* window_ = nullptr;


        public:
            HiddenContext() {
                if (glfwInit() != GLFW_TRUE)
                    throw std::runtime_error("GLFW initialization failed");

                glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_DEPTH_BITS, 24);
                glfwWindowHint(GLFW_SAMPLES, 0);

                window_ = glfwCreateWindow(Width, Height, "RenderFrame Parity", nullptr, nullptr);

                if (window_ == nullptr) {
                    glfwTerminate();
                    throw std::runtime_error("Hidden OpenGL context creation failed");
                }

                glfwMakeContextCurrent(window_);
            }


            ~HiddenContext() {
                if (window_ != nullptr)
                    glfwDestroyWindow(window_);

                glfwTerminate();
            }


            HiddenContext( const HiddenContext& ) = delete;
            HiddenContext& operator=( const HiddenContext& ) = delete;
    };


    class GpuTarget {
        private:
            GLuint framebuffer_ = 0;
            GLuint color_ = 0;
            GLuint depth_ = 0;


        public:
            GpuTarget() {
                glGenFramebuffers(1, &framebuffer_);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

                glGenTextures(1, &color_);
                glBindTexture(GL_TEXTURE_2D, color_);

                glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
                );

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D, color_, 0
                );

                glGenRenderbuffers(1, &depth_);
                glBindRenderbuffer(GL_RENDERBUFFER, depth_);

                glRenderbufferStorage(
                    GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height
                );

                glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    GL_RENDERBUFFER, depth_
                );

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    throw std::runtime_error("GPU test framebuffer incomplete");

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }


            ~GpuTarget() {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (depth_ != 0)
                    glDeleteRenderbuffers(1, &depth_);

                if (color_ != 0)
                    glDeleteTextures(1, &color_);

                if (framebuffer_ != 0)
                    glDeleteFramebuffers(1, &framebuffer_);
            }


            GpuTarget( const GpuTarget& ) = delete;
            GpuTarget& operator=( const GpuTarget& ) = delete;


            void bind() const {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
                glViewport(0, 0, Width, Height);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                glReadBuffer(GL_COLOR_ATTACHMENT0);
            }


            std::array<std::uint8_t, 4> readColor() const {
                bind();

                std::array<std::uint8_t, 4> result{};
                glPixelStorei(GL_PACK_ALIGNMENT, 1);

                glReadPixels(
                    Center, Center, 1, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, result.data()
                );

                return result;
            }


            float readDepth() const {
                bind();

                float value = 1.0f;

                glReadPixels(
                    Center, Center, 1, 1,
                    GL_DEPTH_COMPONENT, GL_FLOAT, &value
                );

                return value;
            }
    };


    struct PixelResult
    {
        ve::Color32 color = 0;
        float depth = 1.0f;
    };


    void require( bool value, const std::string& message ) {
        if (!value)
            throw std::runtime_error(message);
    }

    ve::ClipVertex makeClipVertex( const ve::Vertex& vertex, const ve::Mat4& model,
                                const ve::Mat4& mvp, const ve::NormalMatrix3& normalMatrix ) {
        return ve::VertexProcessor::toClipSpace(
            vertex, model, mvp, normalMatrix
        );
    }

    PixelResult renderCpu( const ve::RenderFrame& frame ) {
        ve::FrameBuffer framebuffer(Width, Height);
        ve::DepthBuffer depth(Width, Height);
        ve::Rasterizer rasterizer(framebuffer, depth);
        const ve::Viewport viewport(Width, Height);

        framebuffer.clear(frame.clearColor);
        depth.clear(1.0f);

        for (const ve::RenderDraw& draw : frame.draws) {
            require(draw.mesh != nullptr && draw.material != nullptr &&
                    draw.material->texture != nullptr, "CPU Draw has missing resources");

            const ve::NormalMatrix3 normalMatrix = ve::calculateNormalMatrix(draw.model);
            const ve::Mat4 mvp = frame.projection * frame.view * draw.model;

            const auto& vertices = draw.mesh->vertices();
            const auto& indices = draw.mesh->indices();

            for (std::size_t i = 0; i < indices.size(); i += 3) {
                const ve::ClipVertex a = makeClipVertex(
                    vertices.at(indices.at(i)), draw.model, mvp, normalMatrix
                );

                const ve::ClipVertex b = makeClipVertex(
                    vertices.at(indices.at(i + 1)), draw.model, mvp, normalMatrix
                );

                const ve::ClipVertex c = makeClipVertex(
                    vertices.at(indices.at(i + 2)), draw.model, mvp, normalMatrix
                );

                std::array<ve::ClipVertex, 16> polygon{};

                const std::size_t count = ve::ClipSpaceClipper::clipTriangle(
                    a, b, c, polygon
                );

                if (count < 3)
                    continue;

                std::array<ve::RasterVertex, 16> raster{};

                for (std::size_t j = 0; j < count; ++j)
                    raster[j] = ve::VertexProcessor::toRasterSpace(polygon[j], viewport);

                for (std::size_t j = 1; j + 1 < count; ++j) {
                    rasterizer.drawTriangleTextured(
                        raster[0], raster[j], raster[j + 1], *draw.material->texture,
                        &frame.lighting.directional, draw.material, &frame.cameraPosition
                    );
                }
            }
        }

        return PixelResult{
            framebuffer.pixel(Center, Center),
            depth.depth(Center, Center)
        };
    }


    std::vector<std::uint8_t> textureBytes( const ve::Texture2D& texture ) {
        std::vector<std::uint8_t> result;

        result.reserve(
            static_cast<std::size_t>(texture.width()) *
            static_cast<std::size_t>(texture.height()) * 4
        );

        for (int y = 0; y < texture.height(); ++y) {
            for (int x = 0; x < texture.width(); ++x) {
                const ve::Color32 color = texture.pixel(x, y);

                result.push_back(ve::getRed(color));
                result.push_back(ve::getGreen(color));
                result.push_back(ve::getBlue(color));
                result.push_back(ve::getAlpha(color));
            }
        }

        return result;
    }


    PixelResult renderGpu( const ve::RenderFrame& frame, ve::OpenGLRenderer& renderer,
                           const GpuTarget& target ) {
        target.bind();

        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDisable(GL_DITHER);
        glDisable(GL_MULTISAMPLE);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);

        constexpr float Inv255 = 1.0f / 255.0f;

        glClearColor(
            ve::getRed(frame.clearColor) * Inv255,
            ve::getGreen(frame.clearColor) * Inv255,
            ve::getBlue(frame.clearColor) * Inv255,
            ve::getAlpha(frame.clearColor) * Inv255
        );

        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (const ve::RenderDraw& draw : frame.draws) {
            require(draw.mesh != nullptr && draw.material != nullptr &&
                    draw.material->texture != nullptr, "GPU Draw has missing resources");

            ve::OpenGLMesh gpuMesh{};
            gpuMesh.initialize(*draw.mesh);

            const ve::Texture2D& texture = *draw.material->texture;
            const std::vector<std::uint8_t> bytes = textureBytes(texture);

            ve::OpenGLMaterial gpuMaterial{};

            gpuMaterial.initialize(
                ve::DefaultTextureVertexShader,
                ve::DefaultTextureFragmentShader,
                texture.width(), texture.height(), bytes.data()
            );

            ve::OpenGLDrawCommand command{};

            command.mesh = &gpuMesh;
            command.material = &gpuMaterial;
            command.model = draw.model;

            command.directionalLight = frame.lighting.directional;
            command.specularStrength = draw.material->specularStrength;
            command.shininess = draw.material->shininess;
            command.lightingDebugMode = 0;

            renderer.submit(
                command, frame.view, frame.projection, frame.cameraPosition
            );
        }

        const auto rgba = target.readColor();
        const float windowDepth = target.readDepth();

        require(glGetError() == GL_NO_ERROR, "OpenGL reported an error");

        return PixelResult{
            ve::makeColor(rgba[0], rgba[1], rgba[2], rgba[3]),
            windowDepth
        };
    }


    void compareFrame( const char* name, const ve::RenderFrame& frame,
                       ve::OpenGLRenderer& renderer, const GpuTarget& target ) {
        const PixelResult cpu = renderCpu(frame);
        const PixelResult gpu = renderGpu(frame, renderer, target);

        std::cout << "[" << name << "] CPU rgba ("
                  << static_cast<int>(ve::getRed(cpu.color)) << ", "
                  << static_cast<int>(ve::getGreen(cpu.color)) << ", "
                  << static_cast<int>(ve::getBlue(cpu.color)) << ", "
                  << static_cast<int>(ve::getAlpha(cpu.color)) << ") depth=" << cpu.depth
                  << " | GPU rgba ("
                  << static_cast<int>(ve::getRed(gpu.color)) << ", "
                  << static_cast<int>(ve::getGreen(gpu.color)) << ", "
                  << static_cast<int>(ve::getBlue(gpu.color)) << ", "
                  << static_cast<int>(ve::getAlpha(gpu.color)) << ") depth=" << gpu.depth << '\n';

        const std::array<int, 4> difference{
            std::abs(static_cast<int>(ve::getRed(cpu.color)) - static_cast<int>(ve::getRed(gpu.color))),
            std::abs(static_cast<int>(ve::getGreen(cpu.color)) - static_cast<int>(ve::getGreen(gpu.color))),
            std::abs(static_cast<int>(ve::getBlue(cpu.color)) - static_cast<int>(ve::getBlue(gpu.color))),
            std::abs(static_cast<int>(ve::getAlpha(cpu.color)) - static_cast<int>(ve::getAlpha(gpu.color)))
        };

        for (int channel : difference)
            require(channel <= Tolerance, std::string(name) + ": RGBA mismatch");

        require(cpu.depth < 1.0f && gpu.depth < 1.0f,
                std::string(name) + ": center pixel was not rasterized");

        require(std::fabs(cpu.depth - gpu.depth) < 0.002f,
                std::string(name) + ": Window Depth mismatch");

        std::cout << "[PASS] " << name << '\n';
    }


    ve::MeshData triangle( const std::array<ve::Vec3, 3>& positions,
                            const ve::Vec3& normal, const ve::Vec2& uv ) {
        std::vector<ve::Vertex> vertices(3);

        for (std::size_t i = 0; i < 3; ++i) {
            vertices[i].position = positions[i];
            vertices[i].normal = normal;
            vertices[i].uv = uv;
        }

        return ve::MeshData(
            std::move(vertices),
            std::vector<std::uint32_t>{ 0, 1, 2 }
        );
    }


    void runTests( ve::OpenGLRenderer& renderer, const GpuTarget& target ) {
        ve::Texture2D solid(1, 1);
        solid.setPixel(0, 0, ve::makeColor(100, 120, 150, 73));

        ve::MaterialData glossy{ &solid, 0.6f, 8.0f };


        // Test 1: Rotation + Nonuniform Scale

        const ve::MeshData flat = triangle({
            ve::Vec3{ -0.7f, -0.6f, 0.0f },
            ve::Vec3{ 0.7f, -0.6f, 0.0f },
            ve::Vec3{ 0.0f, 0.8f, 0.0f }
        }, ve::Vec3{ 0.6f, 0.8f, 0.2f }, ve::Vec2{ 0.5f, 0.5f });

        ve::RenderFrame transformFrame{};

        const ve::Mat4 model =
            ve::Mat4::rotationZ(0.65f) *
            ve::Mat4::scale(ve::Vec3{ 1.4f, 0.5f, 1.0f });

        const ve::Vec3 normal = ve::transformNormal(
            ve::calculateNormalMatrix(model),
            ve::Vec3{ 0.6f, 0.8f, 0.2f }
        ).normalized();

        transformFrame.cameraPosition = normal * 5.0f;
        transformFrame.lighting.directional = ve::DirectionalLight{
            normal, 0.2f, 0.8f
        };

        transformFrame.draws.push_back(ve::RenderDraw{
            &flat, model, 1, 1, &glossy
        });

        compareFrame(
            "Rotation + Nonuniform Scale", transformFrame, renderer, target
        );


        // Test 2: Perspective-correct World Position

        const ve::MeshData perspective = triangle({
            ve::Vec3{ -0.8f, -0.7f, -1.6f },
            ve::Vec3{ 0.8f, -0.7f, -3.0f },
            ve::Vec3{ 0.0f, 1.1f, -4.0f }
        }, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec2{ 0.5f, 0.5f });

        ve::RenderFrame perspectiveFrame{};

        perspectiveFrame.projection = ve::Mat4::perspective(
            1.04719755f, 1.0f, 0.3f, 20.0f
        );

        perspectiveFrame.cameraPosition = ve::Vec3{ 0.5f, 0.4f, 2.0f };

        perspectiveFrame.lighting.directional = ve::DirectionalLight{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        perspectiveFrame.draws.push_back(ve::RenderDraw{
            &perspective, ve::Mat4::identity(), 1, 2, &glossy
        });

        compareFrame(
            "Perspective-correct World Position", perspectiveFrame, renderer, target
        );


        // Test 3: Nearest Texture Sampling

        ve::Texture2D palette(4, 4);

        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                palette.setPixel(
                    x, y,
                    ve::makeColor(
                        static_cast<std::uint8_t>(x * 50 + 10),
                        static_cast<std::uint8_t>(y * 50 + 20),
                        80
                    )
                );
            }
        }

        ve::MaterialData matte{ &palette, 0.0f, 8.0f };

        const ve::MeshData textured = triangle({
            ve::Vec3{ -0.7f, -0.7f, 0.0f },
            ve::Vec3{ 0.7f, -0.7f, 0.0f },
            ve::Vec3{ 0.0f, 0.8f, 0.0f }
        }, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec2{ 0.8f, 0.6f });

        ve::RenderFrame textureFrame{};

        textureFrame.cameraPosition = ve::Vec3{ 0.0f, 0.0f, 3.0f };
        textureFrame.lighting.directional = ve::DirectionalLight{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        textureFrame.draws.push_back(ve::RenderDraw{
            &textured, ve::Mat4::identity(), 1, 3, &matte
        });

        compareFrame("Nearest texture sampling", textureFrame, renderer, target);


        // Test 4: Clamp-to-Edge UV

        const ve::MeshData clamped = triangle({
            ve::Vec3{ -0.7f, -0.7f, 0.0f },
            ve::Vec3{ 0.7f, -0.7f, 0.0f },
            ve::Vec3{ 0.0f, 0.8f, 0.0f }
        }, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec2{ -0.2f, 1.2f });

        textureFrame.draws.clear();

        textureFrame.draws.push_back(ve::RenderDraw{
            &clamped, ve::Mat4::identity(), 1, 4, &matte
        });

        compareFrame("Clamp-to-edge UV", textureFrame, renderer, target);


        // Test 5: Near-plane Clipping

        const ve::MeshData crossing = triangle({
            ve::Vec3{ -0.2f, -0.2f, -0.15f },
            ve::Vec3{ 0.65f, -0.6f, -2.0f },
            ve::Vec3{ 0.0f, 0.8f, -3.0f }
        }, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec2{ 0.5f, 0.5f });

        perspectiveFrame.draws.clear();

        perspectiveFrame.draws.push_back(ve::RenderDraw{
            &crossing, ve::Mat4::identity(), 1, 5, &glossy
        });

        compareFrame("Near-plane clipping", perspectiveFrame, renderer, target);


        // Test 6: Multi-Draw and Depth

        ve::Texture2D farTexture(1, 1);
        ve::Texture2D nearTexture(1, 1);

        farTexture.setPixel(0, 0, ve::makeColor(200, 30, 30));
        nearTexture.setPixel(0, 0, ve::makeColor(30, 200, 30));

        ve::MaterialData farMaterial{ &farTexture, 0.0f, 8.0f };
        ve::MaterialData nearMaterial{ &nearTexture, 0.0f, 8.0f };

        const ve::MeshData overlapping = triangle({
            ve::Vec3{ -0.9f, -0.8f, 0.0f },
            ve::Vec3{ 0.9f, -0.8f, 0.0f },
            ve::Vec3{ 0.0f, 1.0f, 0.0f }
        }, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec2{ 0.5f, 0.5f });

        ve::RenderFrame multiDraw{};

        multiDraw.projection = ve::Mat4::perspective(
            1.04719755f, 1.0f, 0.3f, 20.0f
        );

        multiDraw.cameraPosition = ve::Vec3{ 0.0f, 0.0f, 3.0f };

        multiDraw.lighting.directional = ve::DirectionalLight{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        multiDraw.draws.push_back(ve::RenderDraw{
            &overlapping,
            ve::Mat4::translation(ve::Vec3{ 0.0f, 0.0f, -4.0f }),
            1, 6, &farMaterial
        });

        multiDraw.draws.push_back(ve::RenderDraw{
            &overlapping,
            ve::Mat4::translation(ve::Vec3{ 0.0f, 0.0f, -2.0f }),
            1, 6, &nearMaterial
        });

        compareFrame("RenderFrame Multi-Draw + Depth", multiDraw, renderer, target);

        const ve::MeshData imageTriangle = triangle({
            ve::Vec3{ -0.8f, -0.8f, 0.0f },
            ve::Vec3{ 0.8f, -0.8f, 0.0f },
            ve::Vec3{ 0.0f, 0.9f, 0.0f }
        }, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec2{ 0.1f, 0.1f });

        for (const char* filename : {
            "assets/textures/phase11_1_demo.png",
            "assets/textures/phase11_1_demo.jpg"
        }) {
            const ve::Texture2D imported(ve::ImageLoader::load(filename));
            const ve::MaterialData imageMaterial{ &imported, 0.0f, 8.0f };

            ve::RenderFrame imageFrame{};
            imageFrame.cameraPosition = ve::Vec3{ 0.0f, 0.0f, 3.0f };
            imageFrame.lighting.directional = ve::DirectionalLight{
                ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
            };

            imageFrame.draws.push_back(ve::RenderDraw{
                &imageTriangle, ve::Mat4::identity(), 1, 7, &imageMaterial
            });

            compareFrame(filename, imageFrame, renderer, target);
        }
    }
}


int main() {
    try {
        HiddenContext context{};

        ve::OpenGLRenderer renderer{};
        renderer.initialize();

        GpuTarget target{};

        runTests(renderer, target);

        std::cout << "All RenderFrame GPU parity tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}