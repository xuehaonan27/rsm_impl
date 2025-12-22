#version 330 core

// 从顶点着色器输入
in vec3 position_vs;    // 视图空间位置
in vec3 normal_vs;      // 视图空间法线
in vec3 tangent_vs;     // 视图空间切线
in vec3 bitangent_vs;   // 视图空间副切线
in vec2 uv0_vs;         // 纹理坐标

// 参数 uniform block
// std140 布局规则：vec3 对齐到 16 字节，占用 16 字节空间
layout(std140) uniform Params {
  vec4 base_color_factor;    // offset 0, size 16
  vec4 normal_scale_padding; // offset 16, size 16 (x = normal_scale, y = debug_mode)
  vec4 light_dir_vs;         // offset 32, size 16 (xyz = light_dir, w = padding)
  vec4 light_radiance;       // offset 48, size 16 (xyz = radiance, w = padding)
  vec4 ambient_radiance;     // offset 64, size 16 (xyz = radiance, w = padding)
};

// 纹理采样器
uniform sampler2D base_color_tex;  // 颜色纹理
uniform sampler2D normal_tex;       // 法向纹理

// 输出颜色
layout(location = 0) out vec4 frag_color_out;

// sRGB 到线性空间转换
vec3 srgb_to_linear(vec3 srgb) {
  return pow(srgb, vec3(2.2));
}

// 线性到 sRGB 空间转换（gamma 校正）
vec3 linear_to_srgb(vec3 linear_color) {
  return pow(linear_color, vec3(1.0 / 2.2));
}

// 获取基础颜色
vec4 get_base_color() {
  vec4 raw = texture(base_color_tex, uv0_vs);
  return vec4(srgb_to_linear(raw.rgb), raw.a) * base_color_factor;
}

// 安全归一化
vec3 safe_normalize(vec3 v) {
  return dot(v, v) == 0 ? v : normalize(v);
}

// 从法向贴图解码切线空间法向
vec3 decode_normal_ts() {
  float normal_scale = normal_scale_padding.x;
  vec3 normal = texture(normal_tex, uv0_vs).xyz * 2.0 - 1.0;
  return safe_normalize(normal * vec3(normal_scale, normal_scale, 1.0));
}

// 将切线空间法向转换到视图空间
vec3 get_normal_vs() {
  vec3 normal_ts = decode_normal_ts();
  // 使用 TBN 矩阵将切线空间法向转换到视图空间
  return normal_ts.x * safe_normalize(tangent_vs) +
         normal_ts.y * safe_normalize(bitangent_vs) +
         normal_ts.z * safe_normalize(normal_vs);
}

const float PI = 3.14159265359;

void main() {
  // 调试模式：0=正常, 1=仅基础颜色, 2=仅法向贴图, 3=UV坐标, 4=几何法向
  int debug_mode = int(normal_scale_padding.y);
  
  // 获取视图空间法向（使用法向贴图）
  vec3 n_vs = safe_normalize(get_normal_vs());
  
  // 光源方向（已在视图空间）
  vec3 l_vs = safe_normalize(light_dir_vs.xyz);
  
  // 视线方向（从片段指向相机，相机在视图空间原点）
  vec3 v_vs = -safe_normalize(position_vs);
  
  // 确保法向朝向相机
  n_vs = faceforward(n_vs, -v_vs, n_vs);

  // 获取基础颜色
  vec4 base_color = get_base_color();

  // 调试输出
  if (debug_mode == 1) {
    // 仅显示基础颜色纹理（带 gamma 校正）
    vec4 raw = texture(base_color_tex, uv0_vs);
    frag_color_out = raw;
    return;
  } else if (debug_mode == 2) {
    // 仅显示法向贴图（原始值）
    vec3 raw_normal = texture(normal_tex, uv0_vs).xyz;
    frag_color_out = vec4(raw_normal, 1.0);
    return;
  } else if (debug_mode == 3) {
    // 显示 UV 坐标
    frag_color_out = vec4(uv0_vs, 0.0, 1.0);
    return;
  } else if (debug_mode == 4) {
    // 显示几何法向（视图空间，映射到 0-1）
    frag_color_out = vec4(normal_vs * 0.5 + 0.5, 1.0);
    return;
  } else if (debug_mode == 5) {
    // 显示最终法向（带法向贴图，映射到 0-1）
    frag_color_out = vec4(n_vs * 0.5 + 0.5, 1.0);
    return;
  }

  // 计算漫反射光照（Lambert）
  float NoL = max(dot(n_vs, l_vs), 0.0);
  vec3 diffuse = base_color.rgb / PI;

  // 计算 Blinn-Phong 镜面反射
  vec3 h_vs = safe_normalize(l_vs + v_vs);  // 半向量
  float NoH = max(dot(n_vs, h_vs), 0.0);
  float shininess = 32.0;  // 光泽度
  float specular_strength = 0.5;
  vec3 specular = vec3(specular_strength * pow(NoH, shininess));

  // 直接光照
  vec3 direct_lighting = (diffuse + specular) * light_radiance.xyz * NoL;

  // 环境光照（简单的常量环境光）
  vec3 ambient_lighting = diffuse * ambient_radiance.xyz;

  // 最终颜色
  vec3 final_color = direct_lighting + ambient_lighting;

  // Gamma 校正
  final_color = linear_to_srgb(final_color);

  frag_color_out = vec4(final_color, base_color.a);
}
