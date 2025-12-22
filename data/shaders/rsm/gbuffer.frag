#version 410 core

in vec3 vFragPosViewSpace;
in vec3 vNormalViewSpace;
in vec2 vTexCoord;

layout(location = 0) out vec3 gAlbedo;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gPosition;

uniform sampler2D uDiffuseTexture;
uniform vec3 uDiffuseColor;
uniform bool uHasTexture;

void main() {
    vec3 albedo;
    if (uHasTexture) {
        albedo = texture(uDiffuseTexture, vTexCoord).rgb;
    } else {
        albedo = uDiffuseColor;
    }
    
    gAlbedo = albedo;
    gNormal = normalize(vNormalViewSpace);
    gPosition = vFragPosViewSpace;
}
