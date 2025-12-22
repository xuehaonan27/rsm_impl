#include "material.hpp"

namespace {
// 变换矩阵块（与 cube.vert 中的 Transform 块对应）
struct TransformBlock {
  glm::mat4 MV;    // Model-View 矩阵
  glm::mat4 I_MV;  // Model-View 矩阵的逆
  glm::mat4 P;     // 投影矩阵
};

// 参数块（用于片段着色器）
// std140 布局：所有成员使用 vec4 确保 16 字节对齐
struct ParamsBlock {
  glm::vec4 base_color_factor;    // offset 0, size 16
  glm::vec4 normal_scale_padding; // offset 16, size 16 (x = normal_scale)
  glm::vec4 light_dir_vs;         // offset 32, size 16 (xyz = light_dir)
  glm::vec4 light_radiance;       // offset 48, size 16 (xyz = radiance)
  glm::vec4 ambient_radiance;     // offset 64, size 16 (xyz = radiance)
};
} // namespace

CubeMaterial::CubeMaterial() {
  // 创建着色器程序
  _program = Program::create_from_files("shaders/cube.vert", "shaders/cube.frag");

  // 获取纹理 uniform 位置
  _base_color_tex_location = glGetUniformLocation(_program->get(), "base_color_tex");
  _normal_tex_location = glGetUniformLocation(_program->get(), "normal_tex");

  // 设置 uniform block 绑定点
  GLuint transform_index = glGetUniformBlockIndex(_program->get(), "Transform");
  glUniformBlockBinding(_program->get(), transform_index, 0);
  GLuint params_index = glGetUniformBlockIndex(_program->get(), "Params");
  glUniformBlockBinding(_program->get(), params_index, 1);

  // 创建 uniform buffer
  _transform_buffer = std::make_unique<Buffer>(nullptr, sizeof(TransformBlock));
  _params_buffer = std::make_unique<Buffer>(nullptr, sizeof(ParamsBlock));
}

void CubeMaterial::use() {
  glUseProgram(_program->get());

  // 绑定纹理
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, base_color_tex != nullptr ? base_color_tex->get() : 0);
  glUniform1i(_base_color_tex_location, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, normal_tex != nullptr ? normal_tex->get() : 0);
  glUniform1i(_normal_tex_location, 1);

  // 更新变换矩阵 buffer
  TransformBlock transform_block{};
  transform_block.MV = view * model;
  transform_block.I_MV = glm::inverse(transform_block.MV);
  transform_block.P = projection;
  glBindBuffer(GL_UNIFORM_BUFFER, _transform_buffer->get());
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TransformBlock), &transform_block);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  // 更新参数 buffer
  ParamsBlock params_block{};
  params_block.base_color_factor = base_color_factor;
  params_block.normal_scale_padding = glm::vec4(normal_scale, static_cast<float>(debug_mode), 0.0f, 0.0f);
  params_block.light_dir_vs = glm::vec4(light_dir_vs, 0.0f);
  params_block.light_radiance = glm::vec4(light_radiance, 1.0f);
  params_block.ambient_radiance = glm::vec4(ambient_radiance, 1.0f);
  glBindBuffer(GL_UNIFORM_BUFFER, _params_buffer->get());
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ParamsBlock), &params_block);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  // 绑定 uniform buffer 到绑定点
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, _transform_buffer->get());
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, _params_buffer->get());

  // 启用背面剔除
  glEnable(GL_CULL_FACE);
}
