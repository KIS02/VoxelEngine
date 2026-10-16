#pragma once


namespace ve
{
    inline constexpr const char* DefaultTextureVertexShader = R"(#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vUV;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    vUV = aUV;
}
)";

    inline constexpr const char* DefaultTextureFragmentShader = R"(#version 330 core

in vec2 vUV;
uniform sampler2D uTexture;
uniform vec4 uTint;
layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = texture(uTexture, vUV) * uTint;
}
)";
}
