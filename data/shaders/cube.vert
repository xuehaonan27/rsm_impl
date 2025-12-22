#version 330 core

// 顶点属性输入
layout(location = 0) in vec3 position_os;  // 对象空间位置
layout(location = 1) in vec3 normal_os;    // 对象空间法线
layout(location = 2) in vec4 tangent_os;   // 对象空间切线 (xyz: 切线方向, w: 手性)
layout(location = 3) in vec2 uv0_os;       // 纹理坐标

// 输出到片段着色器
out vec3 position_vs;    // 视图空间位置
out vec3 normal_vs;      // 视图空间法线
out vec3 tangent_vs;     // 视图空间切线
out vec3 bitangent_vs;   // 视图空间副切线
out vec2 uv0_vs;         // 纹理坐标

// 变换矩阵 uniform block
layout(std140) uniform Transform {
  mat4 MV;    // Model-View 矩阵
  mat4 I_MV;  // Model-View 矩阵的逆
  mat4 P;     // 投影矩阵
};

// 使用逆转置矩阵变换法线
vec3 transform_normal(mat4 mat_inverse, vec3 n) {
  return vec3(dot(mat_inverse[0].xyz, n),
              dot(mat_inverse[1].xyz, n),
              dot(mat_inverse[2].xyz, n));
}

// 变换向量（不含平移）
vec3 transform_vector(mat4 mat, vec3 v) {
  return mat[0].xyz * v.x + mat[1].xyz * v.y + mat[2].xyz * v.z;
}

// 变换位置（含平移）
vec4 transform_position(mat4 mat, vec3 p) {
  return mat * vec4(p, 1.0);
}

// 计算副切线
vec3 calculate_bitangent(vec3 normal, vec4 tangent) {
  return cross(normal, tangent.xyz) * tangent.w;
}

// 安全归一化（避免除以零）
vec3 safe_normalize(vec3 v) {
  return dot(v, v) == 0 ? v : normalize(v);
}

void main() {
  // 变换到视图空间
  position_vs = transform_position(MV, position_os).xyz;
  normal_vs = safe_normalize(transform_normal(I_MV, normal_os));
  tangent_vs = safe_normalize(transform_vector(MV, tangent_os.xyz));
  vec3 bitangent_os = calculate_bitangent(normal_os, tangent_os);
  bitangent_vs = safe_normalize(transform_vector(MV, bitangent_os));
  uv0_vs = uv0_os;

  // 计算裁剪空间位置
  gl_Position = transform_position(P, position_vs);
}
