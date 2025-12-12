#pragma once
#ifndef TERRAIN_H
#define TERRAIN_H

#include "types.h"
#include <vector>

// =====================================================================
// Noise Generation Functions
// =====================================================================

float noise2D(float x, float z, int seed);
float interpolate(float a, float b, float x);
float smoothNoise(float x, float z, int seed);
float interpolatedNoise(float x, float z, int seed);
float perlinNoise(float x, float z, int octaves, float persistence, int seed);

// =====================================================================
// Terrain Generation Functions
// =====================================================================

void generateTerrain(std::vector<Vertex>& vertices, 
                     std::vector<unsigned int>& indices,
                     int gridWidth, int gridHeight, 
                     float scaleX, float scaleZ, 
                     float heightScale);

#endif // TERRAIN_H
