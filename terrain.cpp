#include "terrain.h"
#include <cmath>
#include <glm/glm.hpp>

// 2D noise function
float noise2D(float x, float z, int seed) {
    int n = (int)x + (int)z * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

// Interpolation function
float interpolate(float a, float b, float x) {
    float ft = x * 3.1415927f;
    float f = (1.0f - cos(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

// Smooth noise
float smoothNoise(float x, float z, int seed) {
    float corners = (noise2D(x - 1.0f, z - 1.0f, seed) + noise2D(x + 1.0f, z - 1.0f, seed) +
                     noise2D(x - 1.0f, z + 1.0f, seed) + noise2D(x + 1.0f, z + 1.0f, seed)) / 16.0f;
    float sides = (noise2D(x - 1.0f, z, seed) + noise2D(x + 1.0f, z, seed) +
                   noise2D(x, z - 1.0f, seed) + noise2D(x, z + 1.0f, seed)) / 8.0f;
    float center = noise2D(x, z, seed) / 4.0f;
    return corners + sides + center;
}

// Interpolated noise
float interpolatedNoise(float x, float z, int seed) {
    int intX = (int)x;
    float fracX = x - (float)intX;
    int intZ = (int)z;
    float fracZ = z - (float)intZ;

    float v1 = smoothNoise((float)intX, (float)intZ, seed);
    float v2 = smoothNoise((float)(intX + 1), (float)intZ, seed);
    float v3 = smoothNoise((float)intX, (float)(intZ + 1), seed);
    float v4 = smoothNoise((float)(intX + 1), (float)(intZ + 1), seed);

    float i1 = interpolate(v1, v2, fracX);
    float i2 = interpolate(v3, v4, fracX);
    return interpolate(i1, i2, fracZ);
}

// Perlin noise
float perlinNoise(float x, float z, int octaves, float persistence, int seed) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += interpolatedNoise(x * frequency, z * frequency, seed) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

// Generate terrain mesh
void generateTerrain(std::vector<Vertex>& vertices, 
                     std::vector<unsigned int>& indices,
                     int gridWidth, int gridHeight, 
                     float scaleX, float scaleZ, 
                     float heightScale) {
    vertices.clear();
    indices.clear();

    // Generate top surface vertices
    std::vector<float> heights;
    for (int z = 0; z <= gridHeight; z++) {
        for (int x = 0; x <= gridWidth; x++) {
            Vertex vertex;
            
            // Vertex position on X-Z plane
            float xPos = (x / (float)gridWidth - 0.5f) * scaleX;
            float zPos = (z / (float)gridHeight - 0.5f) * scaleZ;
            
            // Use Perlin Noise for height, absolute value to prevent going below platform
            float height = perlinNoise(x * 0.2f, z * 0.2f, 4, 0.5f, 42) * heightScale;
            height = fabs(height);
            heights.push_back(height);
            
            vertex.Position = glm::vec3(xPos, height, zPos);
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.TexCoords = glm::vec2(x / (float)gridWidth, z / (float)gridHeight);
            
            vertices.push_back(vertex);
        }
    }
    
    int topVertexCount = (gridWidth + 1) * (gridHeight + 1);
    
    // Bottom surface at Y=0 to close the model
    for (int z = 0; z <= gridHeight; z++) {
        for (int x = 0; x <= gridWidth; x++) {
            Vertex vertex;
            float xPos = (x / (float)gridWidth - 0.5f) * scaleX;
            float zPos = (z / (float)gridHeight - 0.5f) * scaleZ;
            
            vertex.Position = glm::vec3(xPos, 0.0f, zPos);
            vertex.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
            vertex.TexCoords = glm::vec2(x / (float)gridWidth, z / (float)gridHeight);
            
            vertices.push_back(vertex);
        }
    }

    // Generate top surface indices
    for (int z = 0; z < gridHeight; z++) {
        for (int x = 0; x < gridWidth; x++) {
            int topLeft = z * (gridWidth + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridWidth + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    // Generate bottom surface indices (reversed for viewing from below)
    for (int z = 0; z < gridHeight; z++) {
        for (int x = 0; x < gridWidth; x++) {
            int topLeft = topVertexCount + z * (gridWidth + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = topVertexCount + (z + 1) * (gridWidth + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);

            indices.push_back(topRight);
            indices.push_back(bottomRight);
            indices.push_back(bottomLeft);
        }
    }
    
    // Four side faces connecting top and bottom
    // Front side (z=0)
    for (int x = 0; x < gridWidth; x++) {
        int topLeft = x;
        int topRight = x + 1;
        int bottomLeft = topVertexCount + x;
        int bottomRight = topVertexCount + x + 1;
        
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
    }
    
    // Back side (z=gridHeight)
    for (int x = 0; x < gridWidth; x++) {
        int topLeft = gridHeight * (gridWidth + 1) + x;
        int topRight = topLeft + 1;
        int bottomLeft = topVertexCount + gridHeight * (gridWidth + 1) + x;
        int bottomRight = bottomLeft + 1;
        
        indices.push_back(topLeft);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(bottomRight);
        indices.push_back(bottomLeft);
    }
    
    // Left side (x=0)
    for (int z = 0; z < gridHeight; z++) {
        int topLeft = z * (gridWidth + 1);
        int topRight = (z + 1) * (gridWidth + 1);
        int bottomLeft = topVertexCount + z * (gridWidth + 1);
        int bottomRight = topVertexCount + (z + 1) * (gridWidth + 1);
        
        indices.push_back(topLeft);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(bottomRight);
        indices.push_back(bottomLeft);
    }
    
    // Right side (x=gridWidth)
    for (int z = 0; z < gridHeight; z++) {
        int topLeft = z * (gridWidth + 1) + gridWidth;
        int topRight = (z + 1) * (gridWidth + 1) + gridWidth;
        int bottomLeft = topVertexCount + z * (gridWidth + 1) + gridWidth;
        int bottomRight = topVertexCount + (z + 1) * (gridWidth + 1) + gridWidth;
        
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
    }

    // Calculate normals (for top surface only)
    for (size_t i = 0; i < (size_t)(gridWidth * gridHeight * 6); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].Position;
        glm::vec3 v1 = vertices[i1].Position;
        glm::vec3 v2 = vertices[i2].Position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        vertices[i0].Normal += normal;
        vertices[i1].Normal += normal;
        vertices[i2].Normal += normal;
    }

    // Normalize top surface normals
    for (int i = 0; i < topVertexCount; i++) {
        vertices[i].Normal = glm::normalize(vertices[i].Normal);
    }
}
