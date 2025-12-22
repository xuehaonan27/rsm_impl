#pragma once

#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"

// 简化的材质类，用于实现基于法向贴图的光照
class CubeMaterial : public IMaterial {
public:
  // 纹理
  Texture2D *base_color_tex;  // 颜色纹理
  Texture2D *normal_tex;       // 法向纹理

  // 材质参数
  glm::vec4 base_color_factor = glm::vec4(1.0f);
  float normal_scale = 1.0f;
  int debug_mode = 0;  // 0=正常, 1=基础颜色, 2=法向贴图, 3=UV, 4=几何法向, 5=最终法向

  // 光照参数
  glm::vec3 light_dir_vs;      // 视图空间中的光源方向
  glm::vec3 light_radiance;    // 光源辐射度 (颜色 * 强度)
  glm::vec3 ambient_radiance;  // 环境光辐射度

  CubeMaterial();
  void use() override;

private:
  std::unique_ptr<Program> _program;
  
  // Uniform locations
  GLint _base_color_tex_location;
  GLint _normal_tex_location;

  // Uniform buffer objects
  std::unique_ptr<Buffer> _transform_buffer;
  std::unique_ptr<Buffer> _params_buffer;
};
