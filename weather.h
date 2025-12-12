#pragma once
#ifndef WEATHER_H
#define WEATHER_H

#include "types.h"
#include "config.h"
#include <vector>
#include <glm/glm.hpp>

// =====================================================================
// Weather System State (Global Variable Declarations)
// =====================================================================
extern bool cloudVisible;
extern bool isRaining;
extern bool isSnowing;
extern bool cloudControlMode;
extern glm::vec3 cloudPosition;

extern std::vector<Particle> rainParticles;
extern std::vector<Particle> snowParticles;
extern std::vector<Lightning> lightnings;

extern float lightningTimer;
extern float lightningInterval;

extern std::vector<float> snowHeightMap;

// =====================================================================
// Weather System Initialization
// =====================================================================

void initWeatherSystem();
void cleanupWeatherSystem();

// =====================================================================
// Weather System Update Functions
// =====================================================================

void updateRainParticles(float deltaTime);
void updateSnowParticles(float deltaTime, std::vector<Vertex>& terrainVertices);
void updateLightning(float deltaTime);
void updateWeather(float deltaTime, std::vector<Vertex>& terrainVertices);

// =====================================================================
// Weather State Control
// =====================================================================

void toggleCloud();
void toggleRain();
void toggleSnow();
void moveCloud(const glm::vec3& direction, float deltaTime);

#endif // WEATHER_H
