#pragma once


namespace ve
{
    inline constexpr const char* DefaultTextureVertexShader = R"(#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec2 vUV;
out vec3 vNormalWorld;
out vec3 vWorldPosition;

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    gl_Position = uProjection * uView * worldPosition;
    vUV = aUV;
    vNormalWorld = uNormalMatrix * aNormal;
    vWorldPosition = worldPosition.xyz;
}
)";


    inline constexpr const char* DefaultTextureFragmentShader = R"(#version 330 core

in vec2 vUV;
in vec3 vNormalWorld;
in vec3 vWorldPosition;

uniform sampler2D uTexture;
uniform vec4 uTint;

uniform vec3 uDirectionToLight;
uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform vec3 uLightColor;

uniform vec3 uCameraPosition;
uniform float uSpecularStrength;
uniform float uShininess;
uniform vec3 uDiffuseTint;
uniform vec3 uSpecularColor;
uniform int uLightingDebugMode;

layout(location = 0) out vec4 FragColor;

float srgbToLinearChannel(float value) {
    if (value <= 0.04045)
        return value / 12.92;

    return pow((value + 0.055) / 1.055, 2.4);
}

vec3 decodeSrgb(vec3 rgb) {
    return vec3(
        srgbToLinearChannel(rgb.r),
        srgbToLinearChannel(rgb.g),
        srgbToLinearChannel(rgb.b)
    );
}

float linearToSrgbChannel(float value) {
    float clamped = clamp(value, 0.0, 1.0);

    if (clamped <= 0.0031308)
        return clamped * 12.92;

    return 1.055 * pow(clamped, 1.0 / 2.4) - 0.055;
}

vec3 encodeSrgb(vec3 rgb) {
    return vec3(
        linearToSrgbChannel(rgb.r),
        linearToSrgbChannel(rgb.g),
        linearToSrgbChannel(rgb.b)
    );
}

void main() {
    // GL_SRGB8_ALPHA8 automatically decodes texture RGB to Linear RGB.
    // Material uTint remains normalized sRGB and must be decoded exactly once.
    vec4 sampled = texture(uTexture, vUV);
    vec3 albedo = sampled.rgb * decodeSrgb(uTint.rgb) * uDiffuseTint;
    float alpha = sampled.a * uTint.a;

    float normalLengthSquared = dot(vNormalWorld, vNormalWorld);
    vec3 normal = normalLengthSquared > 1.0e-12
        ? vNormalWorld * inversesqrt(normalLengthSquared)
        : vec3(0.0);

    vec3 lightDirection = normalize(uDirectionToLight);
    float lightFacing = dot(normal, lightDirection);
    float diffuseFactor = max(lightFacing, 0.0);

    vec3 ambient = albedo * uAmbientStrength;
    vec3 diffuse = albedo * uLightColor * (uDiffuseStrength * diffuseFactor);

    vec3 toCamera = uCameraPosition - vWorldPosition;
    float cameraDistanceSquared = dot(toCamera, toCamera);
    vec3 viewDirection = cameraDistanceSquared > 1.0e-12
        ? toCamera * inversesqrt(cameraDistanceSquared)
        : vec3(0.0);

    float alignment = 0.0;
    float specular = 0.0;

    if (lightFacing > 0.0 && uSpecularStrength > 0.0 &&
        cameraDistanceSquared > 1.0e-12) {
        vec3 halfway = lightDirection + viewDirection;
        float halfwayLengthSquared = dot(halfway, halfway);

        if (halfwayLengthSquared > 1.0e-12) {
            vec3 h = halfway * inversesqrt(halfwayLengthSquared);
            alignment = max(dot(normal, h), 0.0);
            specular = uSpecularStrength * pow(alignment, uShininess);
        }
    }

    if (uLightingDebugMode == 1) {
        FragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    if (uLightingDebugMode == 2) {
        FragColor = vec4(vec3(alignment), 1.0);
        return;
    }

    if (uLightingDebugMode == 3) {
        FragColor = vec4(vec3(specular), 1.0);
        return;
    }

    if (uLightingDebugMode == 4) {
        FragColor = vec4(vec3(uSpecularStrength), 1.0);
        return;
    }

    if (uLightingDebugMode == 5) {
        FragColor = vec4(viewDirection * 0.5 + 0.5, 1.0);
        return;
    }

    // Output target: GL_RGBA8 with GL_FRAMEBUFFER_SRGB disabled.
    // For an sRGB framebuffer, disable this manual encoding or disable hardware encoding.
    vec3 specularColor = uSpecularColor * uLightColor * specular;
    vec3 linearResult = ambient + diffuse + specularColor;
    FragColor = vec4(encodeSrgb(linearResult), alpha);
}
)";
}