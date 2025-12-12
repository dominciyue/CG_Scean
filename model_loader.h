#pragma once
#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "types.h"
#include <string>
#include <vector>
#include <map>

// =====================================================================
// OBJ Model Loading Functions
// =====================================================================

// Basic OBJ loading function (without materials)
bool loadOBJ(const std::string& path, 
             std::vector<Vertex>& vertices, 
             std::vector<unsigned int>& indices);

// Load OBJ with materials (for lamp and complex models)
bool loadOBJWithMaterials(const std::string& path,
                          std::vector<Mesh>& meshes,
                          const std::string& directory);

// =====================================================================
// MTL Material File Loading Function
// =====================================================================

bool loadMTL(const std::string& path, 
             std::map<std::string, Material>& materials);

#endif // MODEL_LOADER_H
