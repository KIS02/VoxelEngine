#include "render/opengl/OpenGLRenderer.h"

#include <iostream>
#include <stdexcept>

#include "math/Mat4.h"

#include "render/opengl/OpenGLMaterial.h"
#include "render/opengl/OpenGLMesh.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>



namespace ve
{
    void OpenGLRenderer::initialize() {
        if (glfwGetCurrentContext() == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }



        // Initialize GLAD

        const int version = gladLoadGL(glfwGetProcAddress);

        if (version == 0) {
            throw std::runtime_error("Failed to initialize GLAD");
        }


        const int major = GLAD_VERSION_MAJOR(version);
        const int minor = GLAD_VERSION_MINOR(version);

        if (major < 3 || (major == 3 && minor < 3)) {
            throw std::runtime_error("OpenGL 3.3 or higher is required");
        }


        std::cout << "[GLAD] OpenGL "
                  << major << "." << minor
                  << " loaded"
                  << std::endl;



        // OpenGL Information

        const GLubyte* versionString = glGetString(GL_VERSION);
        const GLubyte* rendererString = glGetString(GL_RENDERER);

        if (versionString != nullptr) {
            std::cout << "[OpenGL] Version: "
                      << reinterpret_cast<const char*>(versionString)
                      << std::endl;
        }

        if (rendererString != nullptr) {
            std::cout << "[OpenGL] Renderer: "
                      << reinterpret_cast<const char*>(rendererString)
                      << std::endl;
        }



        // Check Default Framebuffer Depth Buffer

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLint depthBits = 0;

        glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER,
            GL_DEPTH,
            GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
            &depthBits
        );


        if (depthBits <= 0) {
            throw std::runtime_error("OpenGL framebuffer has no depth buffer");
        }


        std::cout << "[OpenGL] Depth Buffer: "
                  << depthBits << " bits"
                  << std::endl;



        // Initialize Default Render State

        submit(OpenGLRenderStateCommand{ true, true });


        std::cout << "[OpenGLRenderer] Initialized successfully" << std::endl;
    }



    void OpenGLRenderer::submit( const OpenGLClearCommand& command ) {
        GLFWwindow* context = glfwGetCurrentContext();

        if (context == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }


        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(context, &width, &height);

        if (width <= 0 || height <= 0) {
            return;
        }



        // Configure Viewport

        glViewport(0, 0, width, height);



        // Configure Clear Color

        glClearColor(command.red, command.green, command.blue, command.alpha);



        // Select Buffers to Clear

        GLbitfield clearMask = GL_COLOR_BUFFER_BIT;

        if (command.clearDepth) {
            clearMask |= GL_DEPTH_BUFFER_BIT;
        }



        // Execute Clear Command

        glClear(clearMask);
    }



    void OpenGLRenderer::submit( const OpenGLRenderStateCommand& command ) {
        if (glfwGetCurrentContext() == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }



        // Depth Testing

        if (command.depthTest) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glClearDepth(1.0);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }



        // Back-Face Culling

        if (command.backFaceCulling) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
        }
        else {
            glDisable(GL_CULL_FACE);
        }
    }



    void OpenGLRenderer::submit( const OpenGLDrawCommand& command, const Mat4& view, const Mat4& projection ) {
        if (glfwGetCurrentContext() == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }

        if (command.mesh == nullptr || command.material == nullptr) {
            throw std::invalid_argument("Draw Command requires Mesh and Material");
        }

        if (command.mesh->indexCount() == 0 || command.material->programId() == 0) {
            throw std::runtime_error("Draw Command contains uninitialized resources");
        }



        // Select Shader Program

        const GLuint programId = command.material->programId();



        // Find Program-Specific MVP Uniform Locations

        const GLint modelLocation = glGetUniformLocation(programId, "uModel");
        const GLint viewLocation = glGetUniformLocation(programId, "uView");
        const GLint projectionLocation = glGetUniformLocation(programId, "uProjection");


        if (modelLocation == -1 || viewLocation == -1 || projectionLocation == -1) {
            throw std::runtime_error("Draw Command Shader is missing MVP Uniforms");
        }



        // Bind Material

        command.material->bind();



        // Upload Model Matrix

        glUniformMatrix4fv(
            modelLocation,
            1,
            GL_TRUE,
            &command.model.m[0][0]
        );



        // Upload View Matrix

        glUniformMatrix4fv(
            viewLocation,
            1,
            GL_TRUE,
            &view.m[0][0]
        );



        // Upload Projection Matrix

        glUniformMatrix4fv(
            projectionLocation,
            1,
            GL_TRUE,
            &projection.m[0][0]
        );



        // Draw Mesh

        command.mesh->draw();



        // Unbind Material

        command.material->unbind();
    }



    void OpenGLRenderer::clear( float red, float green, float blue, float alpha ) {
        submit(OpenGLClearCommand{ red, green, blue, alpha, true });
    }
}