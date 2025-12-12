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
const float DEFAULT_LAMP_LIGHT_INTENSITY = 1.5f;
const float DEFAULT_LAMP_LIGHT_RADIUS = 0.8f;  // Light falloff radius

// Lamp collision detection radius (for mouse picking)
const float LAMP_CLICK_RADIUS = 0.15f;

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

