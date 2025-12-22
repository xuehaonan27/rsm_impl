#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aTangent;
layout(location = 3) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vFragPosViewSpace;
out vec3 vNormalViewSpace;
out vec2 vTexCoord;

void main() {
    mat4 modelView = uView * uModel;
    vec4 fragPosViewSpace = modelView * vec4(aPosition, 1.0);
    
    gl_Position = uProjection * fragPosViewSpace;
    
    vFragPosViewSpace = fragPosViewSpace.xyz;
    vNormalViewSpace = normalize(mat3(transpose(inverse(modelView))) * aNormal);
    vTexCoord = aTexCoord;
}
