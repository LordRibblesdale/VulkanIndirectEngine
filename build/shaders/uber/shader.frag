#version 450

layout (std140) uniform FragmentData {
    sampler2D diffuseTexture;
    sampler2D shadowMapTexture;

};

out vec4 fragColor;

void main() {

}