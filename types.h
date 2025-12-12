#pragma once
#ifndef TYPES_H
#define TYPES_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

// =====================================================================
// OBJ Model Structures
// =====================================================================

// Vertex structure: position, normal, texture coordinates
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

// Texture structure: ID, type, path
struct Texture {
    unsigned int id = 0;
    std::string type;
    std::string path;
};

// Mesh structure: vertices, indices, textures, OpenGL buffers
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
};

// =====================================================================
// Particle System Structures
// =====================================================================

// Particle structure (for rain and snow)
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
    bool active;
};

// Lightning structure
struct Lightning {
    glm::vec3 startPos;
    glm::vec3 endPos;
    float life;
    float maxLife;
    bool active;
    std::vector<glm::vec3> segments;
};

// =====================================================================
// Material Structure (for OBJ material file parsing)
// =====================================================================

struct Material {
    std::string name;
    glm::vec3 Ka;  // Ambient color
    glm::vec3 Kd;  // Diffuse color
    glm::vec3 Ks;  // Specular color
    float Ns;      // Specular exponent
    float d;       // Transparency
    std::string map_Kd;  // Diffuse texture path
};

#endif // TYPES_H
