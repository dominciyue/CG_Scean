#include "model_loader.h"
#include "texture_loader.h"
#include "mesh.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Basic OBJ loading function (without materials)
bool loadOBJ(const std::string& path, 
             std::vector<Vertex>& vertices, 
             std::vector<unsigned int>& indices) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> posIndices, normalIndices, texIndices;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (prefix == "vt") {
            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        } else if (prefix == "f") {
            std::string vertex;
            while (iss >> vertex) {
                std::istringstream vStream(vertex);
                std::string posStr, texStr, normalStr;
                
                std::getline(vStream, posStr, '/');
                std::getline(vStream, texStr, '/');
                std::getline(vStream, normalStr, '/');
                
                try {
                    posIndices.push_back(std::stoi(posStr) - 1);
                    if (!texStr.empty()) texIndices.push_back(std::stoi(texStr) - 1);
                    if (!normalStr.empty()) normalIndices.push_back(std::stoi(normalStr) - 1);
                } catch (const std::exception&) {
                    continue;
                }
            }
        }
    }
    
    // Build vertex data
    for (size_t i = 0; i < posIndices.size(); i++) {
        Vertex vertex;
        
        if (posIndices[i] < positions.size()) {
            vertex.Position = positions[posIndices[i]];
        } else {
            vertex.Position = glm::vec3(0.0f);
        }
        
        if (i < texIndices.size() && texIndices[i] < texCoords.size()) {
            vertex.TexCoords = texCoords[texIndices[i]];
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        
        if (i < normalIndices.size() && normalIndices[i] < normals.size()) {
            vertex.Normal = normals[normalIndices[i]];
        } else {
            vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        
        vertices.push_back(vertex);
        indices.push_back(static_cast<unsigned int>(i));
    }
    
    // Validate data
    if (vertices.empty()) {
        std::cerr << "Error: No vertices loaded from " << path << std::endl;
        return false;
    }
    
    if (indices.empty()) {
        std::cerr << "Error: No indices loaded from " << path << std::endl;
        return false;
    }
    
    // Check index range
    for (size_t i = 0; i < indices.size(); i++) {
        if (indices[i] >= vertices.size()) {
            std::cerr << "Error: Index out of range in " << path << std::endl;
            return false;
        }
    }
    
    return true;
}

// Load MTL material file
bool loadMTL(const std::string& path, std::map<std::string, Material>& materials) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open MTL file: " << path << std::endl;
        return false;
    }
    
    Material* currentMaterial = nullptr;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "newmtl") {
            std::string name;
            std::getline(iss >> std::ws, name);
            materials[name] = Material();
            materials[name].name = name;
            currentMaterial = &materials[name];
        } else if (currentMaterial) {
            if (prefix == "Ka") {
                iss >> currentMaterial->Ka.x >> currentMaterial->Ka.y >> currentMaterial->Ka.z;
            } else if (prefix == "Kd") {
                iss >> currentMaterial->Kd.x >> currentMaterial->Kd.y >> currentMaterial->Kd.z;
            } else if (prefix == "Ks") {
                iss >> currentMaterial->Ks.x >> currentMaterial->Ks.y >> currentMaterial->Ks.z;
            } else if (prefix == "Ns") {
                iss >> currentMaterial->Ns;
            } else if (prefix == "d") {
                iss >> currentMaterial->d;
            } else if (prefix == "map_Kd") {
                iss >> currentMaterial->map_Kd;
            }
        }
    }
    
    return true;
}

// Face group structure for material-based grouping
struct FaceGroup {
    std::string materialName;
    std::vector<unsigned int> posIndices;
    std::vector<unsigned int> normalIndices;
    std::vector<unsigned int> texIndices;
};

// Load OBJ with materials (for lamp and complex models)
bool loadOBJWithMaterials(const std::string& path,
                          std::vector<Mesh>& meshes,
                          const std::string& directory) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }
    
    // Global data
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    
    // Material data
    std::map<std::string, Material> materials;
    std::string currentMaterialName = "";
    
    // Face groups by material
    std::vector<FaceGroup> faceGroups;
    FaceGroup* currentGroup = nullptr;
    
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (prefix == "vt") {
            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        } else if (prefix == "mtllib") {
            std::string mtlFile;
            iss >> mtlFile;
            std::string mtlPath = directory + "/" + mtlFile;
            loadMTL(mtlPath, materials);
        } else if (prefix == "usemtl") {
            std::string matName;
            std::getline(iss >> std::ws, matName);
            
            // Create new face group
            FaceGroup newGroup;
            newGroup.materialName = matName;
            faceGroups.push_back(newGroup);
            currentGroup = &faceGroups.back();
            currentMaterialName = matName;
        } else if (prefix == "f") {
            // Ensure we have a current group
            if (!currentGroup) {
                FaceGroup newGroup;
                newGroup.materialName = "";
                faceGroups.push_back(newGroup);
                currentGroup = &faceGroups.back();
            }
            
            // Collect face vertices
            std::vector<unsigned int> facePos, faceNorm, faceTex;
            std::string vertex;
            while (iss >> vertex) {
                std::istringstream vStream(vertex);
                std::string posStr, texStr, normalStr;
                
                std::getline(vStream, posStr, '/');
                std::getline(vStream, texStr, '/');
                std::getline(vStream, normalStr, '/');
                
                try {
                    facePos.push_back(std::stoi(posStr) - 1);
                    if (!texStr.empty()) faceTex.push_back(std::stoi(texStr) - 1);
                    if (!normalStr.empty()) faceNorm.push_back(std::stoi(normalStr) - 1);
                } catch (...) {
                    continue;
                }
            }
            
            // Triangulate polygon
            for (size_t i = 1; i + 1 < facePos.size(); i++) {
                currentGroup->posIndices.push_back(facePos[0]);
                currentGroup->posIndices.push_back(facePos[i]);
                currentGroup->posIndices.push_back(facePos[i + 1]);
                
                if (!faceNorm.empty()) {
                    currentGroup->normalIndices.push_back(faceNorm[0]);
                    currentGroup->normalIndices.push_back(faceNorm[i]);
                    currentGroup->normalIndices.push_back(faceNorm[i + 1]);
                }
                
                if (!faceTex.empty()) {
                    currentGroup->texIndices.push_back(faceTex[0]);
                    currentGroup->texIndices.push_back(faceTex[i]);
                    currentGroup->texIndices.push_back(faceTex[i + 1]);
                }
            }
        }
    }
    
    // Create Mesh for each face group
    for (size_t g = 0; g < faceGroups.size(); g++) {
        FaceGroup& group = faceGroups[g];
        if (group.posIndices.empty()) continue;
        
        Mesh mesh;
        
        // Build vertex data
        for (size_t i = 0; i < group.posIndices.size(); i++) {
            Vertex vertex;
            
            // Position
            if (group.posIndices[i] < positions.size()) {
                vertex.Position = positions[group.posIndices[i]];
            }
            
            // Normal
            if (i < group.normalIndices.size() && group.normalIndices[i] < normals.size()) {
                vertex.Normal = normals[group.normalIndices[i]];
            } else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            
            // Texture coordinates
            if (i < group.texIndices.size() && group.texIndices[i] < texCoords.size()) {
                vertex.TexCoords = texCoords[group.texIndices[i]];
            }
            
            mesh.vertices.push_back(vertex);
            mesh.indices.push_back(static_cast<unsigned int>(i));
        }
        
        // Load texture and/or material color
        if (materials.find(group.materialName) != materials.end()) {
            Material& mat = materials[group.materialName];
            
            // Always save the diffuse color from material
            mesh.diffuseColor = mat.Kd;
            mesh.hasDiffuseColor = true;
            
            if (!mat.map_Kd.empty()) {
                Texture tex;
                tex.type = "texture_diffuse";
                
                // Try from directory
                tex.path = directory + "/" + mat.map_Kd;
                tex.id = loadTexture(tex.path.c_str());
                
                // If failed, try from root obj directory
                if (tex.id == 0) {
                    tex.path = "obj/" + mat.map_Kd;
                    tex.id = loadTexture(tex.path.c_str());
                }
                
                // If still failed, try from current directory
                if (tex.id == 0) {
                    tex.path = mat.map_Kd;
                    tex.id = loadTexture(tex.path.c_str());
                }
                
                if (tex.id != 0) {
                    mesh.textures.push_back(tex);
                    std::cout << "Loaded texture: " << tex.path << " for material: " << group.materialName << std::endl;
                } else {
                    std::cerr << "Failed to load texture: " << mat.map_Kd << " for material: " << group.materialName << std::endl;
                }
            } else {
                // No texture, will use diffuse color
                std::cout << "Material " << group.materialName << " uses color only: (" 
                          << mat.Kd.x << ", " << mat.Kd.y << ", " << mat.Kd.z << ")" << std::endl;
            }
        } else {
            // Material not found in MTL file
            std::cerr << "Warning: Material '" << group.materialName << "' not found in MTL file" << std::endl;
        }
        
        // Setup OpenGL buffers
        setupMesh(mesh);
        meshes.push_back(mesh);
    }
    
    return !meshes.empty();
}
