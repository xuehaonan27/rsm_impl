#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aTangent;
layout(location = 3) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;           // Camera view matrix
uniform mat4 uLightVP;        // Light view-projection matrix

out vec3 vFragPosViewSpace;   // Position in camera view space
out vec3 vNormalViewSpace;    // Normal in camera view space
out vec2 vTexCoord;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    
    // Transform to light clip space for rasterization
    gl_Position = uLightVP * worldPos;
    
    // But store position and normal in CAMERA view space
    // This simplifies the shading pass calculations
    vFragPosViewSpace = (uView * worldPos).xyz;
    vNormalViewSpace = normalize(mat3(transpose(inverse(uView * uModel))) * aNormal);
    vTexCoord = aTexCoord;
}
