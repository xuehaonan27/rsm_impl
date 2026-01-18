#version 410 core

in vec3 vFragPosViewSpace;
in vec3 vNormalViewSpace;
in vec2 vTexCoord;

layout(location = 0) out vec3 gFlux;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gPosition;

uniform sampler2D uDiffuseTexture;
uniform vec3 uDiffuseColor;
uniform bool uHasTexture;
uniform vec3 uLightColor;
uniform vec3 uLightDirViewSpace;

void main() {
    vec3 texelColor;
    if (uHasTexture) {
        texelColor = texture(uDiffuseTexture, vTexCoord).rgb;
    } else {
        texelColor = uDiffuseColor;
    }
    
    // Flux = Light color * surface reflectance
    // This represents the reflected radiant flux from this VPL
    float NdotL=max(0.0,dot(normalize(vNormalViewSpace),-uLightDirViewSpace));
    vec3 vplFlux = uLightColor * texelColor*NdotL;
    
    gFlux = vplFlux;
    gNormal = normalize(vNormalViewSpace);
    gPosition = vFragPosViewSpace;
}
