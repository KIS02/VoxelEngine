#include "render/backends/opengl/OpenGLRenderer.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include "math/Mat4.h"
#include "render/lighting/Lighting.h"
#include "render/lighting/NormalMatrix3.h"

#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLMesh.h"

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



    void OpenGLRenderer::submit( const OpenGLDrawCommand& command, const Mat4& view,
                                 const Mat4& projection, const Vec3& cameraPosition ) {
        if (glfwGetCurrentContext() == nullptr)
            throw std::runtime_error("No current OpenGL context");

        if (command.mesh == nullptr || command.material == nullptr)
            throw std::invalid_argument("Draw Command requires Mesh and Material");

        if (command.mesh->indexCount() == 0 || command.material->programId() == 0)
            throw std::runtime_error("Draw Command contains uninitialized resources");

        MaterialData material{};
        material.specularStrength = command.specularStrength;
        material.shininess = command.shininess;
        material.diffuseTint = command.diffuseTint;
        material.specularColor = command.specularColor;
        validateMaterialLighting(material);
        validateDirectionalLighting(command.directionalLight);

        if (!std::isfinite(cameraPosition.x) || !std::isfinite(cameraPosition.y) ||
            !std::isfinite(cameraPosition.z))
            throw std::invalid_argument("Invalid camera position");

        const GLuint programId = command.material->programId();

        const auto requireUniform = [programId]( const char* name ) {
            const GLint location = glGetUniformLocation(programId, name);

            if (location == -1)
                throw std::runtime_error(std::string("Missing Shader Uniform: ") + name);

            return location;
        };

        const GLint modelLocation = requireUniform("uModel");
        const GLint viewLocation = requireUniform("uView");
        const GLint projectionLocation = requireUniform("uProjection");
        const GLint normalMatrixLocation = requireUniform("uNormalMatrix");
        const GLint lightDirectionLocation = requireUniform("uDirectionToLight");
        const GLint ambientLocation = requireUniform("uAmbientStrength");
        const GLint diffuseLocation = requireUniform("uDiffuseStrength");
        const GLint lightColorLocation = requireUniform("uLightColor");
        const GLint cameraLocation = requireUniform("uCameraPosition");
        const GLint specularLocation = requireUniform("uSpecularStrength");
        const GLint shininessLocation = requireUniform("uShininess");
        const GLint diffuseTintLocation = requireUniform("uDiffuseTint");
        const GLint specularColorLocation = requireUniform("uSpecularColor");
        const GLint debugLocation = requireUniform("uLightingDebugMode");

        const NormalMatrix3 normalMatrix = calculateNormalMatrix(command.model);
        const DirectionalLight& light = command.directionalLight;
        const Vec3 direction = light.directionToLight.normalized();

        // Shader manually encodes Linear RGB to sRGB for the GL_RGBA8 target.
        glDisable(GL_FRAMEBUFFER_SRGB);
        command.material->bind();

        glUniformMatrix4fv(modelLocation, 1, GL_TRUE, &command.model.m[0][0]);
        glUniformMatrix4fv(viewLocation, 1, GL_TRUE, &view.m[0][0]);
        glUniformMatrix4fv(projectionLocation, 1, GL_TRUE, &projection.m[0][0]);
        glUniformMatrix3fv(normalMatrixLocation, 1, GL_TRUE, &normalMatrix.m[0][0]);

        glUniform3f(lightDirectionLocation, direction.x, direction.y, direction.z);
        glUniform1f(ambientLocation, light.ambient);
        glUniform1f(diffuseLocation, light.diffuse);
        glUniform3f(lightColorLocation, light.color.r, light.color.g, light.color.b);
        glUniform3f(cameraLocation, cameraPosition.x, cameraPosition.y, cameraPosition.z);
        glUniform1f(specularLocation, command.specularStrength);
        glUniform1f(shininessLocation, command.shininess);
        glUniform3f(diffuseTintLocation, command.diffuseTint.r, command.diffuseTint.g, command.diffuseTint.b);
        glUniform3f(specularColorLocation, command.specularColor.r,
                    command.specularColor.g, command.specularColor.b);
        glUniform1i(debugLocation, command.lightingDebugMode);

        command.mesh->draw();
        command.material->unbind();
    }


    void OpenGLRenderer::clear( float red, float green, float blue, float alpha ) {
        submit(OpenGLClearCommand{ red, green, blue, alpha, true });
    }
}