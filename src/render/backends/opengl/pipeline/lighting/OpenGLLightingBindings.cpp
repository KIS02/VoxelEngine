#include "render/backends/opengl/pipeline/lighting/OpenGLLightingBindings.h"

#include <stdexcept>
#include <string>

#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/lighting/NormalMatrix3.h"

#include <glad/gl.h>


namespace ve
{
    void OpenGLLightingBindings::apply( const OpenGLDrawCommand& command, const Mat4& view,
                                        const Mat4& projection, const Vec3& cameraPosition ) {
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
    }
}
