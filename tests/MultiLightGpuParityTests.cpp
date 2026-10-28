
#include <array>
#include <cstdint>
#include <cmath>
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
#include "assets/mesh/Vertex.h"

#include "math/Color.h"
#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"

#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLMesh.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/OpenGLRenderer.h"

#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
#include "render/lighting/Lighting.h"


namespace
{
    constexpr int TestWidth = 33;
    constexpr int TestHeight = 33;
    constexpr int ChannelTolerance = 2;


    class GlfwSession {
        private:
            GLFWwindow* window_ = nullptr;


        public:
            GlfwSession() {
                if (glfwInit() != GLFW_TRUE)
                    throw std::runtime_error("GLFW initialization failed");

                glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_DEPTH_BITS, 24);
                glfwWindowHint(GLFW_SAMPLES, 0);

                window_ = glfwCreateWindow(TestWidth, TestHeight, "Lighting Parity", nullptr, nullptr);

                if (window_ == nullptr) {
                    glfwTerminate();
                    throw std::runtime_error("Failed to create hidden OpenGL window");
                }

                glfwMakeContextCurrent(window_);
            }


            ~GlfwSession() {
                if (window_ != nullptr)
                    glfwDestroyWindow(window_);

                glfwTerminate();
            }


            GlfwSession( const GlfwSession& ) = delete;
            GlfwSession& operator=( const GlfwSession& ) = delete;
    };


    class OffscreenTarget {
        private:
            GLuint framebuffer_ = 0;
            GLuint colorTexture_ = 0;
            GLuint depthBuffer_ = 0;


            void release() {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (depthBuffer_ != 0)
                    glDeleteRenderbuffers(1, &depthBuffer_);

                if (colorTexture_ != 0)
                    glDeleteTextures(1, &colorTexture_);

                if (framebuffer_ != 0)
                    glDeleteFramebuffers(1, &framebuffer_);

                framebuffer_ = 0;
                colorTexture_ = 0;
                depthBuffer_ = 0;
            }


        public:
            OffscreenTarget() {
                glGenFramebuffers(1, &framebuffer_);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

                glGenTextures(1, &colorTexture_);
                glBindTexture(GL_TEXTURE_2D, colorTexture_);

                glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_RGBA8, TestWidth, TestHeight,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
                );

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D, colorTexture_, 0
                );

                glGenRenderbuffers(1, &depthBuffer_);
                glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer_);

                glRenderbufferStorage(
                    GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, TestWidth, TestHeight
                );

                glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    GL_RENDERBUFFER, depthBuffer_
                );

                const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

                glBindTexture(GL_TEXTURE_2D, 0);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    release();
                    throw std::runtime_error("Offscreen framebuffer is incomplete");
                }
            }


            ~OffscreenTarget() {
                release();
            }


            OffscreenTarget( const OffscreenTarget& ) = delete;
            OffscreenTarget& operator=( const OffscreenTarget& ) = delete;


            void bind() const {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
                glViewport(0, 0, TestWidth, TestHeight);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
            }


            std::array<std::uint8_t, 4> readCenterPixel() const {
                bind();

                std::array<std::uint8_t, 4> rgba{};

                glReadBuffer(GL_COLOR_ATTACHMENT0);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);

                glReadPixels(
                    TestWidth / 2, TestHeight / 2, 1, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, rgba.data()
                );

                return rgba;
            }
    };


    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool channelsClose( const std::array<std::uint8_t, 4>& actual, ve::Color32 expected ) {
        const std::array<std::uint8_t, 4> reference{
            ve::getRed(expected), ve::getGreen(expected),
            ve::getBlue(expected), ve::getAlpha(expected)
        };

        for (std::size_t i = 0; i < actual.size(); ++i) {
            const int difference = std::abs(
                static_cast<int>(actual[i]) - static_cast<int>(reference[i])
            );

            if (difference > ChannelTolerance)
                return false;
        }

        return true;
    }


    void printPixel( const char* label, const std::array<std::uint8_t, 4>& rgba ) {
        std::cout << label << " ("
                  << static_cast<int>(rgba[0]) << ", "
                  << static_cast<int>(rgba[1]) << ", "
                  << static_cast<int>(rgba[2]) << ", "
                  << static_cast<int>(rgba[3]) << ")";
    }


    void runCase( const char* name, ve::OpenGLRenderer& renderer, ve::OpenGLMesh& mesh,
                  ve::OpenGLMaterial& gpuMaterial, const OffscreenTarget& target,
                  const ve::LightingSettings& lighting ) {
        ve::MaterialData material{};
        material.specularStrength = 0.25f;
        material.shininess = 16.0f;
        ve::validateSceneLights(lighting);

        const ve::Vec3 cameraPosition{ 0.0f, 0.0f, 5.0f };
        const ve::Color32 expected = ve::shadeColorMultiLight(
            ve::makeColor(100, 120, 150, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{}, cameraPosition, lighting, material
        );

        target.bind();
        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDisable(GL_DITHER);
        glDisable(GL_MULTISAMPLE);
        glDepthMask(GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ve::OpenGLDrawCommand command{};
        command.mesh = &mesh;
        command.material = &gpuMaterial;
        command.model = ve::Mat4::identity();
        command.directionalLight = lighting.directional;
        command.sceneLighting = &lighting;
        command.specularStrength = material.specularStrength;
        command.shininess = material.shininess;
        renderer.submit(command, ve::Mat4::identity(), ve::Mat4::identity(), cameraPosition);

        const std::array<std::uint8_t, 4> actual = target.readCenterPixel();
        require(glGetError() == GL_NO_ERROR, "OpenGL reported an error");

        const std::array<std::uint8_t, 4> cpu{
            ve::getRed(expected), ve::getGreen(expected),
            ve::getBlue(expected), ve::getAlpha(expected)
        };

        std::cout << "[" << name << "] ";
        printPixel("CPU", cpu);
        std::cout << " | ";
        printPixel("GPU", actual);
        std::cout << '\n';

        if (!channelsClose(actual, expected))
            throw std::runtime_error(std::string(name) + ": CPU/GPU RGBA mismatch");

        std::cout << "[PASS] " << name << '\n';
    }

}


int main() {
    try {
        GlfwSession session{};

        ve::OpenGLRenderer renderer{};
        renderer.initialize();

        OffscreenTarget target{};


        // Use a Constant 1x1 Texture

        const std::uint8_t texel[4] = { 100, 120, 150, 73 };

        ve::OpenGLMaterial gpuMaterial{};

        gpuMaterial.initialize(
            ve::DefaultTextureVertexShader,
            ve::DefaultTextureFragmentShader,
            1, 1, texel
        );


        // Create a Triangle Covering the Center Pixel

        std::vector<ve::Vertex> vertices(3);

        vertices[0].position = ve::Vec3{ -0.9f, -0.9f, 0.0f };
        vertices[1].position = ve::Vec3{ 0.9f, -0.9f, 0.0f };
        vertices[2].position = ve::Vec3{ 0.0f, 0.9f, 0.0f };

        for (ve::Vertex& vertex : vertices) {
            vertex.normal = ve::Vec3{ 0.0f, 0.0f, 1.0f };
            vertex.uv = ve::Vec2{ 0.5f, 0.5f };
        }

        ve::MeshData meshData(
            std::move(vertices),
            std::vector<std::uint32_t>{ 0, 1, 2 }
        );

        ve::OpenGLMesh gpuMesh{};
        gpuMesh.initialize(meshData);


        ve::SceneLightData directional{};
        directional.type = ve::LightType::Directional;
        directional.directionToLight = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        directional.emissionDirection = ve::Vec3{ 0.0f, 0.0f, -1.0f };
        directional.ambient = 0.10f;
        directional.diffuse = 0.25f;

        ve::SceneLightData point{};
        point.type = ve::LightType::Point;
        point.position = ve::Vec3{ 0.0f, 0.0f, 2.0f };
        point.range = 6.0f;
        point.ambient = 0.02f;
        point.diffuse = 0.8f;
        point.color = ve::LinearRGB{ 0.3f, 0.6f, 1.0f };

        ve::SceneLightData spot{};
        spot.type = ve::LightType::Spot;
        spot.position = ve::Vec3{ 0.0f, 0.0f, 2.0f };
        spot.emissionDirection = ve::Vec3{ 0.0f, 0.0f, -1.0f };
        spot.range = 6.0f;
        spot.innerConeDegrees = 15.0f;
        spot.outerConeDegrees = 25.0f;
        spot.ambient = 0.0f;
        spot.diffuse = 0.7f;
        spot.color = ve::LinearRGB{ 1.0f, 0.3f, 0.1f };

        ve::LightingSettings lighting{};
        lighting.sceneLights = { directional };
        runCase("Directional Snapshot", renderer, gpuMesh, gpuMaterial, target, lighting);

        lighting.sceneLights = { point };
        runCase("Point Attenuation", renderer, gpuMesh, gpuMaterial, target, lighting);

        lighting.sceneLights = { spot };
        runCase("Spot Cone", renderer, gpuMesh, gpuMaterial, target, lighting);

        lighting.sceneLights = { directional, point, spot };
        runCase("Three-Lights Accumulation", renderer, gpuMesh, gpuMaterial, target, lighting);

        spot.emissionDirection = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        lighting.sceneLights = { spot };
        runCase("Spot Outside Cone", renderer, gpuMesh, gpuMaterial, target, lighting);

        lighting.sceneLights.clear();
        lighting.directional.ambient = 0.0f;
        lighting.directional.diffuse = 0.0f;
        runCase("No Lights - No Residual Specular", renderer, gpuMesh, gpuMaterial, target, lighting);

        std::cout << "All MultiLight GPU parity tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
