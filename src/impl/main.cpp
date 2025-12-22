#include "../common/application.hpp"
#include "../common/mesh.hpp"
#include "../common/texture.hpp"
#include "../common/utils.hpp"
#include "material.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <memory>

// 创建立方体网格
// 每个面有 4 个顶点，6 个面共 24 个顶点
// 每个顶点有正确的法线、切线和纹理坐标
std::unique_ptr<Mesh> create_cube_mesh() {
  // 立方体顶点数据
  // 每个面的顶点顺序：左下、右下、右上、左上（逆时针，从外部看）
  std::vector<Mesh::Vertex> vertices;
  std::vector<uint32_t> indices;

  // 定义立方体的 6 个面
  // 每个面：4个顶点位置、法线、切线、UV坐标
  
  // 前面 (z = +0.5)
  {
    glm::vec3 normal(0, 0, 1);
    glm::vec4 tangent(1, 0, 0, 1);  // 切线指向 +X
    uint32_t base = vertices.size();
    vertices.push_back({{-0.5f, -0.5f, 0.5f}, normal, tangent, {0, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{ 0.5f, -0.5f, 0.5f}, normal, tangent, {1, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{ 0.5f,  0.5f, 0.5f}, normal, tangent, {1, 1}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f,  0.5f, 0.5f}, normal, tangent, {0, 1}, {0, 0}, {1, 1, 1, 1}});
    indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
  }
  
  // 后面 (z = -0.5)
  {
    glm::vec3 normal(0, 0, -1);
    glm::vec4 tangent(-1, 0, 0, 1);  // 切线指向 -X
    uint32_t base = vertices.size();
    vertices.push_back({{ 0.5f, -0.5f, -0.5f}, normal, tangent, {0, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f, -0.5f, -0.5f}, normal, tangent, {1, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f,  0.5f, -0.5f}, normal, tangent, {1, 1}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{ 0.5f,  0.5f, -0.5f}, normal, tangent, {0, 1}, {0, 0}, {1, 1, 1, 1}});
    indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
  }
  
  // 右面 (x = +0.5)
  {
    glm::vec3 normal(1, 0, 0);
    glm::vec4 tangent(0, 0, -1, 1);  // 切线指向 -Z
    uint32_t base = vertices.size();
    vertices.push_back({{0.5f, -0.5f,  0.5f}, normal, tangent, {0, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{0.5f, -0.5f, -0.5f}, normal, tangent, {1, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{0.5f,  0.5f, -0.5f}, normal, tangent, {1, 1}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{0.5f,  0.5f,  0.5f}, normal, tangent, {0, 1}, {0, 0}, {1, 1, 1, 1}});
    indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
  }
  
  // 左面 (x = -0.5)
  {
    glm::vec3 normal(-1, 0, 0);
    glm::vec4 tangent(0, 0, 1, 1);  // 切线指向 +Z
    uint32_t base = vertices.size();
    vertices.push_back({{-0.5f, -0.5f, -0.5f}, normal, tangent, {0, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f, -0.5f,  0.5f}, normal, tangent, {1, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f,  0.5f,  0.5f}, normal, tangent, {1, 1}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f,  0.5f, -0.5f}, normal, tangent, {0, 1}, {0, 0}, {1, 1, 1, 1}});
    indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
  }
  
  // 上面 (y = +0.5)
  {
    glm::vec3 normal(0, 1, 0);
    glm::vec4 tangent(1, 0, 0, 1);  // 切线指向 +X
    uint32_t base = vertices.size();
    vertices.push_back({{-0.5f, 0.5f,  0.5f}, normal, tangent, {0, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{ 0.5f, 0.5f,  0.5f}, normal, tangent, {1, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{ 0.5f, 0.5f, -0.5f}, normal, tangent, {1, 1}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f, 0.5f, -0.5f}, normal, tangent, {0, 1}, {0, 0}, {1, 1, 1, 1}});
    indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
  }
  
  // 下面 (y = -0.5)
  {
    glm::vec3 normal(0, -1, 0);
    glm::vec4 tangent(1, 0, 0, 1);  // 切线指向 +X
    uint32_t base = vertices.size();
    vertices.push_back({{-0.5f, -0.5f, -0.5f}, normal, tangent, {0, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{ 0.5f, -0.5f, -0.5f}, normal, tangent, {1, 0}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{ 0.5f, -0.5f,  0.5f}, normal, tangent, {1, 1}, {0, 0}, {1, 1, 1, 1}});
    vertices.push_back({{-0.5f, -0.5f,  0.5f}, normal, tangent, {0, 1}, {0, 0}, {1, 1, 1, 1}});
    indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
  }

  return std::make_unique<Mesh>(
      vertices.data(),
      static_cast<uint32_t>(vertices.size()),
      indices.data(),
      static_cast<uint32_t>(indices.size()));
}

class HomeworkApp final : public Application {
public:
  HomeworkApp() : Application("Homework - Normal Mapping", 800, 600) {}

private:
  void init() override {
    // 创建相机
    _camera = std::make_unique<ModelViewerCamera>();

    // 加载纹理
    _base_color_texture = std::make_unique<Texture2D>("cube/色彩纹理图.bmp");
    _normal_texture = std::make_unique<Texture2D>("cube/法向图.bmp");

    // 创建立方体网格
    _cube_mesh = create_cube_mesh();

    // 创建材质
    _material = std::make_unique<CubeMaterial>();
    _material->base_color_tex = _base_color_texture.get();
    _material->normal_tex = _normal_texture.get();
  }

  void draw_ui() {
    // FPS 显示
    {
      float frame_time = average_frame_time();
      if (frame_time > 0.0f) {
        ImGui::Text("FPS: %.1f (%.2f ms)", 1.0f / frame_time, frame_time * 1000.0f);
      } else {
        ImGui::Text("FPS: N/A");
      }
    }

    // 截图按钮
    if (ImGui::Button("Screen Shot")) {
      request_screen_shot();
    }

    int id = 0;

    // 相机控制
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(id++);
      _camera->draw_ui();
      ImGui::PopID();
    }

    // 模型控制
    if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(id++);
      ImGui::SliderAngle("Rotation X", &_model_rotation.x);
      ImGui::SliderAngle("Rotation Y", &_model_rotation.y);
      ImGui::SliderAngle("Rotation Z", &_model_rotation.z);
      ImGui::SliderFloat("Scale", &_model_scale, 0.1f, 3.0f);
      ImGui::PopID();
    }

    // 光源控制
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(id++);
      ImGui::SliderAngle("Pitch", &_light_pitch, 0.0f, 180.0f);
      ImGui::SliderAngle("Yaw", &_light_yaw);
      ImGui::ColorEdit3("Color", (float *)&_light_color);
      ImGui::SliderFloat("Strength", &_light_strength, 0.0f, 10.0f);
      ImGui::PopID();
    }

    // 环境光控制
    if (ImGui::CollapsingHeader("Ambient Light")) {
      ImGui::PushID(id++);
      ImGui::ColorEdit3("Color", (float *)&_ambient_color);
      ImGui::SliderFloat("Strength", &_ambient_strength, 0.0f, 2.0f);
      ImGui::PopID();
    }

    // 材质参数
    if (ImGui::CollapsingHeader("Material")) {
      ImGui::PushID(id++);
      ImGui::SliderFloat("Normal Scale", &_material->normal_scale, 0.0f, 2.0f);
      ImGui::ColorEdit4("Base Color Factor", (float *)&_material->base_color_factor);
      ImGui::PopID();
    }

    // 调试选项
    if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(id++);
      const char* debug_modes[] = {
        "0: Normal (Lit)",
        "1: Base Color Texture",
        "2: Normal Map Texture",
        "3: UV Coordinates",
        "4: Geometry Normal",
        "5: Final Normal (with normal map)"
      };
      ImGui::Combo("Debug Mode", &_material->debug_mode, debug_modes, IM_ARRAYSIZE(debug_modes));
      ImGui::PopID();
    }
  }

  void draw_scene() {
    // 获取窗口尺寸
    int fb_width, fb_height;
    glfwGetFramebufferSize(_window, &fb_width, &fb_height);

    // 设置视口和清除颜色
    glm::vec3 clear_color = _ambient_color * _ambient_strength;
    glClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);
    glViewport(0, 0, fb_width, fb_height);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 计算变换矩阵
    float aspect = static_cast<float>(fb_width) / static_cast<float>(fb_height);
    glm::mat4 view = _camera->view();
    glm::mat4 projection = _camera->projection(aspect);

    // 计算模型矩阵
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(_model_scale));
    model = glm::rotate(model, _model_rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, _model_rotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, _model_rotation.z, glm::vec3(0, 0, 1));

    // 设置材质变换矩阵
    _material->model = model;
    _material->view = view;
    _material->projection = projection;

    // 计算光源方向（从球坐标转换到笛卡尔坐标）
    glm::vec3 light_dir_ws = polar_to_cartesian(_light_yaw, _light_pitch);
    // 转换到视图空间
    glm::vec3 light_dir_vs = glm::mat3(view) * light_dir_ws;
    _material->light_dir_vs = glm::normalize(light_dir_vs);
    _material->light_radiance = _light_color * _light_strength;
    _material->ambient_radiance = _ambient_color * _ambient_strength;

    // 使用材质并绘制
    _material->use();
    _cube_mesh->draw();
  }

  void update() override {
    draw_ui();
    draw_scene();
  }

  // 相机
  std::unique_ptr<ModelViewerCamera> _camera;

  // 纹理
  std::unique_ptr<Texture2D> _base_color_texture;
  std::unique_ptr<Texture2D> _normal_texture;

  // 网格
  std::unique_ptr<Mesh> _cube_mesh;

  // 材质
  std::unique_ptr<CubeMaterial> _material;

  // 模型变换参数
  glm::vec3 _model_rotation = glm::vec3(0.0f);
  float _model_scale = 1.0f;

  // 光源参数
  float _light_yaw = glm::radians(45.0f);
  float _light_pitch = glm::radians(60.0f);
  float _light_strength = 2.0f;
  glm::vec3 _light_color = glm::vec3(1.0f, 1.0f, 1.0f);

  // 环境光参数
  float _ambient_strength = 0.3f;
  glm::vec3 _ambient_color = glm::vec3(0.3f, 0.3f, 0.4f);
};

int main() {
  try {
    HomeworkApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
