// RSM (Reflective Shadow Maps) 实现主程序
// 基于 Dachsbacher & Stamminger 2005 论文

#include "../common/application.hpp"
#include "../common/framebuffer.hpp"
#include "../common/shader.hpp"
#include "../common/mesh.hpp"
#include "../common/texture.hpp"
#include "../common/data.hpp"
#include "obj_loader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <random>

class RSMApp final : public Application {
public:
    RSMApp() : Application("RSM - Reflective Shadow Maps", 1024, 768) {}

private:
    //窗口
    int _currentWidth = 1024;
    int _currentHeight = 768;
    // 分辨率设置
    static constexpr int RSM_SIZE = 2048;
    static constexpr int MAX_VPL_NUM = 512;
    
    // 着色器程序
    std::unique_ptr<Program> _gbufferProgram;
    std::unique_ptr<Program> _rsmBufferProgram;
    std::unique_ptr<Program> _shadingProgram;
    std::unique_ptr<Program> _displayProgram;
    
    // G-Buffer 纹理
    std::unique_ptr<Texture2D> _gbufferAlbedo;
    std::unique_ptr<Texture2D> _gbufferNormal;
    std::unique_ptr<Texture2D> _gbufferPosition;
    std::unique_ptr<Texture2D> _gbufferDepth;
    std::unique_ptr<Framebuffer> _gbufferFBO;
    
    // RSM Buffer 纹理
    std::unique_ptr<Texture2D> _rsmFlux;
    std::unique_ptr<Texture2D> _rsmNormal;
    std::unique_ptr<Texture2D> _rsmPosition;
    std::unique_ptr<Texture2D> _rsmDepth;
    std::unique_ptr<Framebuffer> _rsmFBO;
    
    // 输出纹理和 FBO (用于 shading pass)
    std::unique_ptr<Texture2D> _outputTexture;
    std::unique_ptr<Texture2D> _outputDepth;
    std::unique_ptr<Framebuffer> _outputFBO;
    
    // 全屏四边形
    std::unique_ptr<Mesh> _screenQuad;
    
    // 场景
    std::unique_ptr<ObjLoader> _scene;
    
    // VPL 采样数据
    std::vector<glm::vec4> _vplSamples;
    
    // 相机参数（独立于 ModelViewerCamera）
    float _cameraYaw = 180.0f;
    float _cameraPitch = 0.0f;
    float _cameraDistance = 1.0f;
    glm::vec3 _cameraTarget = glm::vec3(0.0f, 100.0f, 0.0f);
    
    // 光源参数
    glm::vec3 _lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    glm::vec3 _lightColor = glm::vec3(1.0f, 0.95f, 0.9f);
    float _lightYaw = 45.0f;
    float _lightPitch = 45.0f;
    
    // RSM 参数
    bool _enableRSM = true;  // RSM 开关
    int _vplNum = 64;
    float _maxSampleRadius = 25.0f;
    float _indirectStrength = 5.0f;
    int _displayMode = 0;
    
    // Shadow Map (用于普通阴影模式)
    std::unique_ptr<Texture2D> _shadowMapTexture;
    std::unique_ptr<Framebuffer> _shadowMapFBO;
    static constexpr int SHADOW_MAP_SIZE = 1024;
    
    // 窗口尺寸
    int _width = 1024;
    int _height = 768;

    void init() override {
        // 获取窗口尺寸
        glfwGetFramebufferSize(_window, &_currentWidth, &_currentHeight);
        _width = _currentWidth;
        _height = _currentHeight;    
        // 加载场景
        loadScene();
        
        // 创建着色器
        createShaders();
        
        // 创建 Framebuffers
        createFramebuffers();
        
        // 创建全屏四边形
        createScreenQuad();
        
        // 生成 VPL 采样点
        generateVPLSamples();
    }
    
    void loadScene() {
        _scene = std::make_unique<ObjLoader>();
        // 使用 Sponza 场景
        fs::path modelPath = Data::data_path() / ".." / "Model" / "sponza" / "sponza.obj";
        if (!_scene->load(modelPath.string())) {
            std::cerr << "Failed to load scene: " << modelPath << std::endl;
        }
        
        // 根据场景设置相机初始位置
        _cameraTarget = glm::vec3(0.8f,-0.1f,3.3f);
        _cameraDistance =1.5f;
        _cameraYaw=210.0f;
        _cameraPitch=10.0f;
    }
    
    void createShaders() {
        // G-Buffer Pass
        _gbufferProgram = Program::create_from_files(
            Data::data_path() / "shaders" / "rsm" / "gbuffer.vert",
            Data::data_path() / "shaders" / "rsm" / "gbuffer.frag"
        );
        
        // RSM Buffer Pass
        _rsmBufferProgram = Program::create_from_files(
            Data::data_path() / "shaders" / "rsm" / "rsm_buffer.vert",
            Data::data_path() / "shaders" / "rsm" / "rsm_buffer.frag"
        );
        
        // Shading Pass (使用片段着色器，因为 macOS 不支持 Compute Shader)
        _shadingProgram = Program::create_from_files(
            Data::data_path() / "shaders" / "rsm" / "rsm_shading.vert",
            Data::data_path() / "shaders" / "rsm" / "rsm_shading.frag"
        );
        
        // Display Pass
        _displayProgram = Program::create_from_files(
            Data::data_path() / "shaders" / "rsm" / "display.vert",
            Data::data_path() / "shaders" / "rsm" / "display.frag"
        );
    }
    
    void createFramebuffers() {
        // G-Buffer 纹理
        _gbufferAlbedo = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _width, _height, GL_RGB16F, GL_RGB);
        _gbufferNormal = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _width, _height, GL_RGB16F, GL_RGB);
        _gbufferPosition = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _width, _height, GL_RGB32F, GL_RGB);
        _gbufferDepth = std::make_unique<Texture2D>(
            nullptr, GL_UNSIGNED_INT_24_8, _width, _height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL);
        
        Texture2D* gbufferColorAttachments[] = {
            _gbufferAlbedo.get(), _gbufferNormal.get(), _gbufferPosition.get()
        };
        _gbufferFBO = std::make_unique<Framebuffer>(
            gbufferColorAttachments, 3, _gbufferDepth.get());
        
        // RSM Buffer 纹理
        _rsmFlux = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, RSM_SIZE, RSM_SIZE, GL_RGB16F, GL_RGB);
        _rsmNormal = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, RSM_SIZE, RSM_SIZE, GL_RGB16F, GL_RGB);
        _rsmPosition = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, RSM_SIZE, RSM_SIZE, GL_RGB32F, GL_RGB);
        _rsmDepth = std::make_unique<Texture2D>(
            nullptr, GL_UNSIGNED_INT_24_8, RSM_SIZE, RSM_SIZE, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL);
        
        Texture2D* rsmColorAttachments[] = {
            _rsmFlux.get(), _rsmNormal.get(), _rsmPosition.get()
        };
        _rsmFBO = std::make_unique<Framebuffer>(
            rsmColorAttachments, 3, _rsmDepth.get());
        
        // 输出纹理和 FBO (需要 depth attachment 来确保 FBO 完整)
        _outputTexture = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _width, _height, GL_RGBA16F, GL_RGBA);
        _outputDepth = std::make_unique<Texture2D>(
            nullptr, GL_UNSIGNED_INT_24_8, _width, _height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL);
        Texture2D* outputColorAttachments[] = { _outputTexture.get() };
        _outputFBO = std::make_unique<Framebuffer>(
            outputColorAttachments, 1, _outputDepth.get());
    }
    
    void createScreenQuad() {
        std::vector<Mesh::Vertex> vertices = {
            {{-1, -1, 0}, {0,0,1}, {1,0,0,1}, {0,0}, {0,0}, {1,1,1,1}},
            {{ 3, -1, 0}, {0,0,1}, {1,0,0,1}, {2,0}, {0,0}, {1,1,1,1}},
            {{-1,  3, 0}, {0,0,1}, {1,0,0,1}, {0,2}, {0,0}, {1,1,1,1}}
        };
        std::vector<uint32_t> indices = {0, 1, 2};
        _screenQuad = std::make_unique<Mesh>(vertices.data(), 3, indices.data(), 3);
    }
    
    void generateVPLSamples() {
        _vplSamples.resize(MAX_VPL_NUM);
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < MAX_VPL_NUM; ++i) {
            float xi1 = dist(rng);
            float xi2 = dist(rng);
            float angle = 2.0f * 3.14159265f * xi2;
            
            _vplSamples[i] = glm::vec4(
                xi1 * std::sin(angle),
                xi1 * std::cos(angle),
                xi1 * xi1,
                0.0f
            );
        }
    }
    
    glm::mat4 getLightViewProjection() {
        float yawRad = glm::radians(_lightYaw);
        float pitchRad = glm::radians(_lightPitch);
        _lightDir = glm::normalize(glm::vec3(
            std::sin(yawRad) * std::sin(pitchRad),
            -std::cos(pitchRad),
            std::cos(yawRad) * std::sin(pitchRad)
        ));
        
        glm::vec3 center = _scene->getCenter();
        float radius = _scene->getRadius() * 1.5f;
        
        glm::vec3 lightPos = center - _lightDir * radius;
        glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0, 1, 0));
        glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, 0.1f, radius * 3.0f);
        
        return lightProj * lightView;
    }
    
    void setUniformMat4(GLuint program, const char* name, const glm::mat4& mat) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
    }
    
    void setUniformVec3(GLuint program, const char* name, const glm::vec3& vec) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(vec));
    }
    
    void setUniformVec4Array(GLuint program, const char* name, const glm::vec4* arr, int count) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform4fv(loc, count, glm::value_ptr(arr[0]));
    }
    
    void setUniformInt(GLuint program, const char* name, int value) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1i(loc, value);
    }
    
    void setUniformFloat(GLuint program, const char* name, float value) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1f(loc, value);
    }
    
    void setUniformBool(GLuint program, const char* name, bool value) {
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1i(loc, value ? 1 : 0);
    }
    
    void gbufferPass(const glm::mat4& view, const glm::mat4& proj) {
        glBindFramebuffer(GL_FRAMEBUFFER, _gbufferFBO->get());
        glViewport(0, 0, _width, _height);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        GLuint prog = _gbufferProgram->get();
        glUseProgram(prog);
        setUniformMat4(prog, "uModel", glm::mat4(1.0f));
        setUniformMat4(prog, "uView", view);
        setUniformMat4(prog, "uProjection", proj);
        
        for (size_t i = 0; i < _scene->getMeshes().size(); ++i) {
            auto& mesh = _scene->getMeshes()[i];
            int matIdx = mesh.materialIndex;
            if (matIdx >= 0 && matIdx < static_cast<int>(_scene->getMaterials().size())) {
                auto& mat = _scene->getMaterials()[matIdx];
                setUniformVec3(prog, "uDiffuseColor", mat.diffuseColor);
                setUniformBool(prog, "uHasTexture", mat.hasTexture);
                if (mat.hasTexture && mat.diffuseTexture) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture->get());
                    setUniformInt(prog, "uDiffuseTexture", 0);
                }
            }
            mesh.mesh->draw();
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void rsmBufferPass(const glm::mat4& lightview, const glm::mat4& lightVP) {
        glBindFramebuffer(GL_FRAMEBUFFER, _rsmFBO->get());
        glViewport(0, 0, RSM_SIZE, RSM_SIZE);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        
        GLuint prog = _rsmBufferProgram->get();
        glUseProgram(prog);
        setUniformMat4(prog, "uModel", glm::mat4(1.0f));
        setUniformMat4(prog, "uView", lightview);
        setUniformMat4(prog, "uLightVP", lightVP);
        setUniformVec3(prog, "uLightColor", _lightColor);

        glm::vec3 lightDirInView = glm::vec3(lightview * glm::vec4(_lightDir, 0.0f));
        setUniformVec3(prog, "uLightDirViewSpace", lightDirInView);
        
        for (size_t i = 0; i < _scene->getMeshes().size(); ++i) {
            auto& mesh = _scene->getMeshes()[i];
            int matIdx = mesh.materialIndex;
            if (matIdx >= 0 && matIdx < static_cast<int>(_scene->getMaterials().size())) {
                auto& mat = _scene->getMaterials()[matIdx];
                setUniformVec3(prog, "uDiffuseColor", mat.diffuseColor);
                setUniformBool(prog, "uHasTexture", mat.hasTexture);
            }
            mesh.mesh->draw();
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void shadingPass(const glm::mat4& view, const glm::mat4& lightVP) {
        // 使用片段着色器方式渲染到输出 FBO
        glBindFramebuffer(GL_FRAMEBUFFER, _outputFBO->get());
        glViewport(0, 0, _width, _height);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        
        GLuint prog = _shadingProgram->get();
        glUseProgram(prog);
        
        // 绑定 G-Buffer 纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _gbufferAlbedo->get());
        setUniformInt(prog, "uAlbedoTexture", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _gbufferNormal->get());
        setUniformInt(prog, "uNormalTexture", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, _gbufferPosition->get());
        setUniformInt(prog, "uPositionTexture", 2);
        
        // 绑定 RSM 纹理
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, _rsmFlux->get());
        setUniformInt(prog, "uRSMFluxTexture", 3);
        
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, _rsmNormal->get());
        setUniformInt(prog, "uRSMNormalTexture", 4);
        
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, _rsmPosition->get());
        setUniformInt(prog, "uRSMPositionTexture", 5);
        
        // 绑定 Shadow Map (使用 RSM depth)
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, _rsmDepth->get());
        setUniformInt(prog, "uShadowMap", 6);
        
        // 设置 uniforms
        glm::mat4 invView = glm::inverse(view);
        setUniformMat4(prog,"uInView",invView);
        setUniformVec3(prog,"uLightDirWorld",_lightDir);
        setUniformMat4(prog, "uLightVPMulInvCameraView", lightVP * invView);
        setUniformVec3(prog, "uLightDirViewSpace", glm::vec3(view * glm::vec4(_lightDir, 0.0f)));
        setUniformVec3(prog, "uLightColor", _lightColor);
        setUniformInt(prog, "uRSMResolution", RSM_SIZE);
        setUniformInt(prog, "uVPLNum", _vplNum);
        setUniformFloat(prog, "uMaxSampleRadius", _maxSampleRadius);
        setUniformFloat(prog, "uIndirectStrength", _indirectStrength);
        setUniformBool(prog, "uEnableRSM", _enableRSM);  // RSM 开关
        
        // 传递 VPL 采样数据
        setUniformVec4Array(prog, "uVPLSamples", _vplSamples.data(), MAX_VPL_NUM);
        
        // 绘制全屏四边形
        _screenQuad->draw();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void displayPass() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _width, _height);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        
        GLuint prog = _displayProgram->get();
        glUseProgram(prog);
        
        glActiveTexture(GL_TEXTURE0);
        switch (_displayMode) {
            case 0: glBindTexture(GL_TEXTURE_2D, _outputTexture->get()); break;
            case 1: glBindTexture(GL_TEXTURE_2D, _gbufferAlbedo->get()); break;
            case 2: glBindTexture(GL_TEXTURE_2D, _gbufferNormal->get()); break;
            case 3: glBindTexture(GL_TEXTURE_2D, _gbufferPosition->get()); break;
            case 4: glBindTexture(GL_TEXTURE_2D, _rsmFlux->get()); break;
            case 5: glBindTexture(GL_TEXTURE_2D, _rsmNormal->get()); break;
            case 6: glBindTexture(GL_TEXTURE_2D, _rsmPosition->get()); break;
            default: glBindTexture(GL_TEXTURE_2D, _outputTexture->get()); break;
        }
        setUniformInt(prog, "uTexture", 0);
        setUniformInt(prog, "uDisplayMode", _displayMode);
        
        _screenQuad->draw();
    }
    
    glm::mat4 getCameraView() {
        float yawRad = glm::radians(_cameraYaw);
        float pitchRad = glm::radians(_cameraPitch);
        
        glm::vec3 cameraDir = glm::vec3(
            std::sin(yawRad) * std::cos(pitchRad),
            std::sin(pitchRad),
            std::cos(yawRad) * std::cos(pitchRad)
        );
        
        glm::vec3 cameraPos = _cameraTarget - cameraDir * _cameraDistance;
        return glm::lookAt(cameraPos, _cameraTarget, glm::vec3(0, 1, 0));
    }
    
    void drawUI() {
        ImGui::Text("FPS: %.1f", 1.0f / average_frame_time());
        
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("Light");
            ImGui::SliderFloat("Yaw##L", &_lightYaw, 0.0f, 360.0f);
            ImGui::SliderFloat("Pitch##L", &_lightPitch, 10.0f, 170.0f);
            ImGui::ColorEdit3("Color##L", &_lightColor.x);
            ImGui::PopID();
        }
        
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("Camera");
            ImGui::SliderFloat("Yaw##C", &_cameraYaw, 0.0f, 360.0f);
            ImGui::SliderFloat("Pitch##C", &_cameraPitch, -89.0f, 89.0f);
            ImGui::SliderFloat("Distance##C", &_cameraDistance, 0.1f, 50.0f);
            ImGui::DragFloat3("Target##C", &_cameraTarget.x, 0.1f);
            ImGui::PopID();
        }
        
        if (ImGui::CollapsingHeader("Rendering Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable RSM (Indirect Lighting)", &_enableRSM);
            if (_enableRSM) {
                ImGui::Text("Mode: RSM (Direct + Indirect Light)");
            } else {
                ImGui::Text("Mode: Shadow Mapping (Direct Light Only)");
            }
        }
        
        if (ImGui::CollapsingHeader("RSM Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("VPL Count", &_vplNum, 8, MAX_VPL_NUM);
            ImGui::SliderFloat("Sample Radius", &_maxSampleRadius, 5.0f, 100.0f);
            ImGui::SliderFloat("Indirect Strength", &_indirectStrength, 0.1f, 20.0f);
            if (!_enableRSM) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(RSM disabled - params inactive)");
            }
        }
        
        if (ImGui::CollapsingHeader("Debug")) {
            const char* modes[] = {"Final", "Albedo", "Normal", "Position", 
                                   "RSM Flux", "RSM Normal", "RSM Position"};
            ImGui::Combo("Display", &_displayMode, modes, 7);
        }
    }

    void recreateGBuffer() {
        // 释放旧资源
        _gbufferAlbedo.reset();
        _gbufferNormal.reset();
        _gbufferPosition.reset();
        _gbufferDepth.reset();
        _gbufferFBO.reset();
        
        _outputTexture.reset();
        _outputDepth.reset();
        _outputFBO.reset();
        
        // 重新创建 G-Buffer
        _gbufferAlbedo = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _currentWidth, _currentHeight, GL_RGB16F, GL_RGB);
        _gbufferNormal = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _currentWidth, _currentHeight, GL_RGB16F, GL_RGB);
        _gbufferPosition = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _currentWidth, _currentHeight, GL_RGB32F, GL_RGB);
        _gbufferDepth = std::make_unique<Texture2D>(
            nullptr, GL_UNSIGNED_INT_24_8, _currentWidth, _currentHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL);
        
        Texture2D* gbufferColorAttachments[] = {
            _gbufferAlbedo.get(), _gbufferNormal.get(), _gbufferPosition.get()
        };
        _gbufferFBO = std::make_unique<Framebuffer>(
            gbufferColorAttachments, 3, _gbufferDepth.get());
        
        // 重新创建 Output FBO
        _outputTexture = std::make_unique<Texture2D>(
            nullptr, GL_FLOAT, _currentWidth, _currentHeight, GL_RGBA16F, GL_RGBA);
        _outputDepth = std::make_unique<Texture2D>(
            nullptr, GL_UNSIGNED_INT_24_8, _currentWidth, _currentHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL);
        Texture2D* outputColorAttachments[] = { _outputTexture.get() };
        _outputFBO = std::make_unique<Framebuffer>(
            outputColorAttachments, 1, _outputDepth.get());
        
    // 更新主窗口尺寸
        _width = _currentWidth;
        _height = _currentHeight;
    }
    
    void update() override {
        drawUI();

        int newWidth, newHeight;
        glfwGetFramebufferSize(_window, &newWidth, &newHeight);
    
        // 如果窗口大小变化，重建 FBO
        if (newWidth != _currentWidth || newHeight != _currentHeight) {
            _currentWidth = newWidth;
            _currentHeight = newHeight;
            recreateGBuffer(); // 重建 G-Buffer 和 Output FBO
        }
    
        float aspect = float(_currentWidth) / float(_currentHeight);

        
        glm::mat4 view = getCameraView();
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 1.0f, 10000.0f);
        // glm::mat4 lightVP = getLightViewProjection();

        float yawRad = glm::radians(_lightYaw);
        float pitchRad = glm::radians(_lightPitch);
        _lightDir = glm::normalize(glm::vec3(std::sin(yawRad) * std::sin(pitchRad),-std::cos(pitchRad),std::cos(yawRad) * std::sin(pitchRad)));

        glm::vec3 center = _scene->getCenter();
        float radius = _scene->getRadius() * 1.5f;
        glm::vec3 lightPos = center - _lightDir * radius;
        glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0, 1, 0));
        glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, 0.1f, radius * 3.0f);
        glm::mat4 lightVP = lightProj * lightView;      
        
        gbufferPass(view, proj);
        rsmBufferPass(lightView, lightVP);
        shadingPass(view, lightVP);
        displayPass();
    }
};

int main() {
    try {
        RSMApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
