#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "math/Color.h"
#include "render/backends/opengl/pipeline/postprocess/OpenGLHdrPass.h"
#include "render/color/ToneMapping.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void runTest() {
        require(glfwInit() == GLFW_TRUE, "GLFW initialization failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        GLFWwindow* window = glfwCreateWindow(64, 64, "HDR GPU Parity", nullptr, nullptr);
        require(window != nullptr, "OpenGL 3.3 test window creation failed");
        glfwMakeContextCurrent(window);
        require(gladLoadGL(glfwGetProcAddress) != 0, "GLAD initialization failed");

        {
            ve::OpenGLHdrPass hdr;
            const ve::ToneMappingSettings settings{ true, 1.0f, ve::ToneMappingOperator::Reinhard };
            hdr.beginScene(64, 64);
            const GLfloat clearColor[4]{ 4.0f, 1.0f, 0.0f, 73.0f / 255.0f };
            glDisable(GL_SCISSOR_TEST);
            glClearBufferfv(GL_COLOR, 0, clearColor);
            glClear(GL_DEPTH_BUFFER_BIT);
            hdr.present(settings);

            std::array<std::uint8_t, 4> pixel{};
            glReadBuffer(GL_BACK);
            glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
            require(glGetError() == GL_NO_ERROR, "GPU HDR postprocess raised an OpenGL error");

            const ve::Color32 reference = ve::toneMapToSrgb(
                ve::LinearRGB{ 4.0f, 1.0f, 0.0f }, 73, settings);
            require(std::abs(static_cast<int>(pixel[0]) - static_cast<int>(ve::getRed(reference))) <= 3,
                    "HDR red channel CPU/GPU mismatch");
            require(std::abs(static_cast<int>(pixel[1]) - static_cast<int>(ve::getGreen(reference))) <= 3,
                    "HDR green channel CPU/GPU mismatch");
            require(std::abs(static_cast<int>(pixel[2]) - static_cast<int>(ve::getBlue(reference))) <= 3,
                    "HDR blue channel CPU/GPU mismatch");
            std::cout << "[PASS] OpenGL RGBA16F HDR -> Reinhard -> SDR RGB parity\n";
        }

        glfwDestroyWindow(window);
        glfwTerminate();
    }
}


int main() {
    try {
        runTest();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
