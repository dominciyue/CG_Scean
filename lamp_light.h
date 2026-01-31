#pragma once
#ifndef LAMP_LIGHT_H
#define LAMP_LIGHT_H

#include <glm/glm.hpp>

// =====================================================================
// Lamp Light State and Properties
// =====================================================================

// Lamp light state
extern bool lampIsOn;

// Lamp light properties
extern glm::vec3 lampLightColor;
extern float lampLightIntensity;
extern float lampLightRadius;

// =====================================================================
// Lamp Light Configuration
// =====================================================================

// Default lamp light settings
const glm::vec3 DEFAULT_LAMP_LIGHT_COLOR(1.0f, 0.9f, 0.7f);  // Warm white
const float DEFAULT_LAMP_LIGHT_INTENSITY = 1.5f;             // Light intensity
const float DEFAULT_LAMP_LIGHT_RADIUS = 0.8f;                // Light radius

// Lamp shade (cylinder) collision parameters for mouse picking
// Based on lamp1.obj: shade is roughly from Y=0.5 to Y=1.4 in model space
// After scaling by LAMP_SCALE=0.08, height ~0.072
const float LAMP_SHADE_RADIUS = 0.045f;      // Cylinder radius of lamp shade
const float LAMP_SHADE_HEIGHT_MIN = 0.04f;   // Bottom of shade relative to LAMP_POSITION.y
const float LAMP_SHADE_HEIGHT_MAX = 0.12f;   // Top of shade relative to LAMP_POSITION.y

// =====================================================================
// Lamp Light Functions
// =====================================================================

// Initialize lamp light system
void initLampLight();

// Toggle lamp on/off
void toggleLamp();

// Set lamp state directly
void setLampState(bool on);

// Get lamp state
bool isLampOn();

// Get lamp light position (calculated from lamp model position)
glm::vec3 getLampLightPosition();

// Calculate light attenuation at a given distance
float calculateLampAttenuation(float distance);

#endif // LAMP_LIGHT_H

