
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

#include "render/common/DefaultTextureShaders.h"
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
                  const ve::DirectionalLight& light, const ve::Vec3& cameraPosition,
                  float specularStrength, float shininess ) {
        ve::MaterialData material{};
        material.specularStrength = specularStrength;
        material.shininess = shininess;

        ve::validateMaterialLighting(material);


        // CPU Reference Pixel

        const ve::Color32 expected = ve::shadeColorBlinnPhong(
            ve::makeColor(100, 120, 150, 73),
            ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{ 0.0f, 0.0f, 0.0f },
            cameraPosition, light, material
        );


        // Prepare Offscreen OpenGL State

        target.bind();

        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDisable(GL_DITHER);
        glDisable(GL_MULTISAMPLE);

        glDepthMask(GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Submit Through the Actual OpenGLRenderer

        ve::OpenGLDrawCommand command{};

        command.mesh = &mesh;
        command.material = &gpuMaterial;
        command.model = ve::Mat4::identity();

        command.directionalLight = light;
        command.specularStrength = specularStrength;
        command.shininess = shininess;
        command.lightingDebugMode = 0;

        renderer.submit(
            command,
            ve::Mat4::identity(),
            ve::Mat4::identity(),
            cameraPosition
        );


        // Read GPU Result

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


        // Shared Test Light

        const ve::DirectionalLight frontLight{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        const ve::DirectionalLight backLight{
            ve::Vec3{ 0.0f, 0.0f, -1.0f }, 0.2f, 0.8f
        };


        // Test Cases

        runCase(
            "Aligned Highlight", renderer, gpuMesh, gpuMaterial, target,
            frontLight, ve::Vec3{ 0.0f, 0.0f, 5.0f }, 0.4f, 16.0f
        );

        runCase(
            "Matte Material", renderer, gpuMesh, gpuMaterial, target,
            frontLight, ve::Vec3{ 0.0f, 0.0f, 5.0f }, 0.0f, 16.0f
        );

        runCase(
            "Side Camera", renderer, gpuMesh, gpuMaterial, target,
            frontLight, ve::Vec3{ 5.0f, 0.0f, 0.0f }, 0.8f, 4.0f
        );

        runCase(
            "Back Light", renderer, gpuMesh, gpuMaterial, target,
            backLight, ve::Vec3{ 0.0f, 0.0f, 5.0f }, 0.8f, 4.0f
        );

        std::cout << "All Lighting GPU parity tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
