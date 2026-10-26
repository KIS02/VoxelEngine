#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "math/Mat4.h"
#include "render/common/DefaultTextureShaders.h"
#include "render/lighting/DirectionalLight.h"
#include "render/lighting/NormalMatrix3.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool nearlyEqual( float a, float b, float epsilon = 0.0001f ) {
        return std::fabs(a - b) <= epsilon;
    }


    ve::Vec3 transformedNormal( const ve::NormalMatrix3& matrix, const ve::Vec3& normal ) {
        return ve::Vec3{
            matrix.m[0][0] * normal.x + matrix.m[0][1] * normal.y + matrix.m[0][2] * normal.z,
            matrix.m[1][0] * normal.x + matrix.m[1][1] * normal.y + matrix.m[1][2] * normal.z,
            matrix.m[2][0] * normal.x + matrix.m[2][1] * normal.y + matrix.m[2][2] * normal.z
        };
    }



    void testIdentityAndTranslation() {
        const ve::NormalMatrix3 identity = ve::calculateNormalMatrix(ve::Mat4::identity());
        const ve::NormalMatrix3 moved = ve::calculateNormalMatrix(
            ve::Mat4::translation(ve::Vec3{ 10.0f, -20.0f, 30.0f })
        );

        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                const float expected = row == column ? 1.0f : 0.0f;
                require(nearlyEqual(identity.m[row][column], expected), "Identity Normal Matrix is incorrect");
                require(nearlyEqual(moved.m[row][column], expected), "Translation must not affect Normal Matrix");
            }
        }

        std::cout << "[PASS] Identity and translation Normal Matrix\n";
    }


    void testNonUniformScaleAndRotation() {
        const ve::NormalMatrix3 scaled = ve::calculateNormalMatrix(
            ve::Mat4::scale(ve::Vec3{ 2.0f, 1.0f, 4.0f })
        );
        const ve::Vec3 transformed = transformedNormal(scaled, ve::Vec3{ 1.0f, 1.0f, 0.0f });

        require(nearlyEqual(transformed.x, 0.5f) && nearlyEqual(transformed.y, 1.0f),
                "Normals must use inverse-transpose, not Model scaling");

        const float halfPi = 3.14159265358979323846f * 0.5f;
        const ve::NormalMatrix3 rotated = ve::calculateNormalMatrix(ve::Mat4::rotationY(halfPi));
        const ve::Vec3 pointingX = transformedNormal(rotated, ve::Vec3{ 1.0f, 0.0f, 0.0f });

        require(nearlyEqual(pointingX.x, 0.0f) && nearlyEqual(pointingX.z, -1.0f),
                "Normal Matrix must preserve rotation direction");

        std::cout << "[PASS] Nonuniform scale and rotation\n";
    }


    void testShearedNormalsRemainPerpendicular() {
        ve::Mat4 model = ve::Mat4::identity();
        model(0, 0) = 2.0f;
        model(0, 1) = 1.0f;
        model(1, 1) = 3.0f;
        model(1, 2) = 2.0f;
        model(2, 2) = 4.0f;

        const ve::NormalMatrix3 normalMatrix = ve::calculateNormalMatrix(model);
        const ve::Vec3 normal = transformedNormal(normalMatrix, ve::Vec3{ 0.0f, 1.0f, 0.0f });
        const ve::Vec3 tangentX{ model(0, 0), model(1, 0), model(2, 0) };
        const ve::Vec3 tangentZ{ model(0, 2), model(1, 2), model(2, 2) };

        require(nearlyEqual(ve::dot(normal, tangentX), 0.0f) &&
                nearlyEqual(ve::dot(normal, tangentZ), 0.0f),
                "Normal must remain perpendicular to transformed tangent plane");

        std::cout << "[PASS] Sheared normal remains perpendicular to surface\n";
    }


    void testSingularTransformRejected() {
        bool rejected = false;

        try {
            (void)ve::calculateNormalMatrix(ve::Mat4::scale(ve::Vec3{ 1.0f, 0.0f, 1.0f }));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Singular Model scaling must be rejected");
        std::cout << "[PASS] Singular Model transform rejected\n";
    }


    void testShaderContract() {
        const std::string_view vertex = ve::DefaultTextureVertexShader;
        const std::string_view fragment = ve::DefaultTextureFragmentShader;

        require(vertex.find("#version 330 core") != std::string_view::npos,
                "Vertex Shader must target GLSL 330");
        require(vertex.find("layout(location = 3) in vec3 aNormal") != std::string_view::npos,
                "Vertex Shader must read Normal Attribute 3");
        require(vertex.find("uNormalMatrix * aNormal") != std::string_view::npos,
                "Vertex Shader must transform Normal into World space");
        require(vertex.find("uProjection * uView * uModel") != std::string_view::npos,
                "Vertex Shader must preserve MVP order");
        require(fragment.find("texture(uTexture, vUV) * uTint") != std::string_view::npos,
                "Fragment Shader must preserve texture and tint");
        require(fragment.find("dot(normal, lightDirection)") != std::string_view::npos,
                "Fragment Shader must compute Lambert diffuse lighting");
        require(fragment.find("baseColor.a") != std::string_view::npos,
                "Lighting must not modify alpha");

        std::cout << "[PASS] OpenGL directional-light shader source contract\n";
    }


    void testDirectionalLightingMath() {
        const ve::DirectionalLight light{ ve::Vec3{ 0.0f, 1.0f, 0.0f }, 0.25f, 0.75f };

        require(nearlyEqual(ve::lightingFactor(ve::Vec3{ 0.0f, 1.0f, 0.0f }, light), 1.0f),
                "Front face should receive full light");
        require(nearlyEqual(ve::lightingFactor(ve::Vec3{ 0.0f, -1.0f, 0.0f }, light), 0.25f),
                "Back face should receive ambient light only");

        std::cout << "[PASS] Directional-light CPU reference calculations\n";
    }
}


int main() {
    try {
        testIdentityAndTranslation();
        testNonUniformScaleAndRotation();
        testShearedNormalsRemainPerpendicular();
        testSingularTransformRejected();
        testShaderContract();
        testDirectionalLightingMath();

        std::cout << "All OpenGLDirectionalLighting unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
