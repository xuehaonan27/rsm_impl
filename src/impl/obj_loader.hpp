#pragma once

#include "../common/mesh.hpp"
#include "../common/texture.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// Represents a loaded OBJ mesh with its material
struct ObjMesh {
    std::unique_ptr<Mesh> mesh;
    std::string materialName;
    int materialIndex = -1;
};

// Represents a material loaded from MTL file
struct ObjMaterial {
    std::string name;
    glm::vec3 diffuseColor = glm::vec3(1.0f);
    glm::vec3 specularColor = glm::vec3(0.0f);
    float shininess = 1.0f;
    std::string diffuseTexturePath;
    std::unique_ptr<Texture2D> diffuseTexture;
    bool hasTexture = false;
};

// OBJ model loader using tinyobjloader
class ObjLoader {
public:
    // Load an OBJ file and its associated MTL materials
    bool load(const std::string& filepath);
    
    // Get the loaded meshes
    const std::vector<ObjMesh>& getMeshes() const { return _meshes; }
    std::vector<ObjMesh>& getMeshes() { return _meshes; }
    
    // Get the loaded materials
    const std::vector<ObjMaterial>& getMaterials() const { return _materials; }
    std::vector<ObjMaterial>& getMaterials() { return _materials; }
    
    // Draw all meshes
    void draw();
    
    // Get bounding box for camera setup
    glm::vec3 getMinBounds() const { return _minBounds; }
    glm::vec3 getMaxBounds() const { return _maxBounds; }
    glm::vec3 getCenter() const { return (_minBounds + _maxBounds) * 0.5f; }
    float getRadius() const;

private:
    std::vector<ObjMesh> _meshes;
    std::vector<ObjMaterial> _materials;
    glm::vec3 _minBounds = glm::vec3(FLT_MAX);
    glm::vec3 _maxBounds = glm::vec3(-FLT_MAX);
    std::string _basePath;
    
    void loadTextures();
    void calculateBounds(const std::vector<Mesh::Vertex>& vertices);
};
