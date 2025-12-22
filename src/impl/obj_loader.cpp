#define TINYOBJLOADER_IMPLEMENTATION
#include "../../third_party/tiny_obj_loader.h"
#include "obj_loader.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>

bool ObjLoader::load(const std::string& filepath) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.vertex_color = false;
    
    if (!reader.ParseFromFile(filepath, config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader Error: " << reader.Error() << std::endl;
        }
        return false;
    }
    
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader Warning: " << reader.Warning() << std::endl;
    }
    
    // Get base path for textures
    size_t lastSlash = filepath.find_last_of("/\\");
    _basePath = (lastSlash != std::string::npos) ? filepath.substr(0, lastSlash + 1) : "";
    
    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();
    
    // Load materials
    _materials.clear();
    for (const auto& mat : materials) {
        ObjMaterial objMat;
        objMat.name = mat.name;
        objMat.diffuseColor = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        objMat.specularColor = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        objMat.shininess = mat.shininess;
        
        if (!mat.diffuse_texname.empty()) {
            objMat.diffuseTexturePath = mat.diffuse_texname;
            objMat.hasTexture = true;
        }
        
        _materials.push_back(std::move(objMat));
    }
    
    // Add a default material if none exist
    if (_materials.empty()) {
        ObjMaterial defaultMat;
        defaultMat.name = "default";
        defaultMat.diffuseColor = glm::vec3(0.8f);
        _materials.push_back(std::move(defaultMat));
    }
    
    // Load textures
    loadTextures();
    
    // Process each shape
    _meshes.clear();
    for (const auto& shape : shapes) {
        // Group faces by material
        std::unordered_map<int, std::vector<size_t>> materialFaces;
        
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int matId = shape.mesh.material_ids[f];
            if (matId < 0) matId = 0;  // Use default material
            materialFaces[matId].push_back(f);
        }
        
        // Create a mesh for each material group
        for (const auto& pair : materialFaces) {
            int matId = pair.first;
            const std::vector<size_t>& faceIndices = pair.second;
            
            std::vector<Mesh::Vertex> vertices;
            std::vector<uint32_t> indices;
            
            for (size_t f : faceIndices) {
                size_t indexOffset = 0;
                for (size_t i = 0; i < f; i++) {
                    indexOffset += shape.mesh.num_face_vertices[i];
                }
                
                unsigned int fv = shape.mesh.num_face_vertices[f];
                for (unsigned int v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    
                    Mesh::Vertex vertex{};
                    
                    // Position
                    vertex.position = glm::vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    );
                    
                    // Normal
                    if (idx.normal_index >= 0 && 3 * idx.normal_index + 2 < static_cast<int>(attrib.normals.size())) {
                        vertex.normal = glm::vec3(
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        );
                    } else {
                        vertex.normal = glm::vec3(0, 1, 0);  // Default up normal
                    }
                    
                    // Texture coordinates
                    if (idx.texcoord_index >= 0 && 2 * idx.texcoord_index + 1 < static_cast<int>(attrib.texcoords.size())) {
                        vertex.uv0 = glm::vec2(
                            attrib.texcoords[2 * idx.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]  // Flip Y
                        );
                    } else {
                        vertex.uv0 = glm::vec2(0);
                    }
                    
                    // Default tangent (would need proper calculation for normal mapping)
                    vertex.tangent = glm::vec4(1, 0, 0, 1);
                    vertex.uv1 = glm::vec2(0);
                    vertex.color = glm::vec4(1);
                    
                    indices.push_back(static_cast<uint32_t>(vertices.size()));
                    vertices.push_back(vertex);
                }
            }
            
            if (!vertices.empty()) {
                calculateBounds(vertices);
                
                ObjMesh objMesh;
                objMesh.mesh = std::make_unique<Mesh>(
                    vertices.data(),
                    static_cast<uint32_t>(vertices.size()),
                    indices.data(),
                    static_cast<uint32_t>(indices.size())
                );
                objMesh.materialIndex = matId;
                if (matId >= 0 && matId < static_cast<int>(_materials.size())) {
                    objMesh.materialName = _materials[matId].name;
                }
                
                _meshes.push_back(std::move(objMesh));
            }
        }
    }
    
    std::cout << "Loaded OBJ: " << filepath << std::endl;
    std::cout << "  Meshes: " << _meshes.size() << std::endl;
    std::cout << "  Materials: " << _materials.size() << std::endl;
    std::cout << "  Bounds: [" << _minBounds.x << ", " << _minBounds.y << ", " << _minBounds.z << "] to ["
              << _maxBounds.x << ", " << _maxBounds.y << ", " << _maxBounds.z << "]" << std::endl;
    
    return true;
}

void ObjLoader::loadTextures() {
    TextureSettings settings;
    settings.wrap_s = GL_REPEAT;
    settings.wrap_t = GL_REPEAT;
    settings.min_filter = GL_LINEAR_MIPMAP_LINEAR;
    settings.max_filter = GL_LINEAR;
    
    for (auto& mat : _materials) {
        if (mat.hasTexture && !mat.diffuseTexturePath.empty()) {
            std::string texPath = _basePath + mat.diffuseTexturePath;
            
            // Handle potential path separators
            std::replace(texPath.begin(), texPath.end(), '\\', '/');
            
            try {
                // Try to load directly from path
                mat.diffuseTexture = std::make_unique<Texture2D>(
                    fs::path(texPath), &settings
                );
                std::cout << "  Loaded texture: " << texPath << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "  Failed to load texture: " << texPath << " - " << e.what() << std::endl;
                mat.hasTexture = false;
            }
        }
    }
}

void ObjLoader::calculateBounds(const std::vector<Mesh::Vertex>& vertices) {
    for (const auto& v : vertices) {
        _minBounds = glm::min(_minBounds, v.position);
        _maxBounds = glm::max(_maxBounds, v.position);
    }
}

float ObjLoader::getRadius() const {
    glm::vec3 extent = _maxBounds - _minBounds;
    return glm::length(extent) * 0.5f;
}

void ObjLoader::draw() {
    for (auto& objMesh : _meshes) {
        if (objMesh.mesh) {
            objMesh.mesh->draw();
        }
    }
}
