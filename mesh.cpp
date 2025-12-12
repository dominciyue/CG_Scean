#include "mesh.h"
#include <glad/glad.h>

void setupMesh(Mesh& mesh) {
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    
    glBindVertexArray(mesh.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), &mesh.vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);
    
    // Vertex position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    
    // Texture coordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    
    glBindVertexArray(0);
}

void cleanupMesh(Mesh& mesh) {
    if (mesh.VAO != 0) {
        glDeleteVertexArrays(1, &mesh.VAO);
        mesh.VAO = 0;
    }
    if (mesh.VBO != 0) {
        glDeleteBuffers(1, &mesh.VBO);
        mesh.VBO = 0;
    }
    if (mesh.EBO != 0) {
        glDeleteBuffers(1, &mesh.EBO);
        mesh.EBO = 0;
    }
}
