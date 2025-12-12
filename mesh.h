#pragma once
#ifndef MESH_H
#define MESH_H

#include "types.h"

// Setup Mesh OpenGL buffers (VAO, VBO, EBO)
void setupMesh(Mesh& mesh);

// Cleanup Mesh OpenGL resources
void cleanupMesh(Mesh& mesh);

#endif // MESH_H
