#include "lamp_light.h"
#include "config.h"
#include <cmath>

// =====================================================================
// Global Variable Definitions
// =====================================================================

bool lampIsOn = false;  // Lamp starts OFF

glm::vec3 lampLightColor = DEFAULT_LAMP_LIGHT_COLOR;
float lampLightIntensity = DEFAULT_LAMP_LIGHT_INTENSITY;
float lampLightRadius = DEFAULT_LAMP_LIGHT_RADIUS;

// =====================================================================
// Lamp Light Functions Implementation
// =====================================================================

void initLampLight() {
    lampIsOn = false;
    lampLightColor = DEFAULT_LAMP_LIGHT_COLOR;
    lampLightIntensity = DEFAULT_LAMP_LIGHT_INTENSITY;
    lampLightRadius = DEFAULT_LAMP_LIGHT_RADIUS;
}

void toggleLamp() {
    lampIsOn = !lampIsOn;
}

void setLampState(bool on) {
    lampIsOn = on;
}

bool isLampOn() {
    return lampIsOn;
}

glm::vec3 getLampLightPosition() {
    // Light emanates from the top of the lamp shade
    // Offset upward from lamp base position
    return LAMP_POSITION + glm::vec3(0.0f, 0.15f * LAMP_SCALE, 0.0f);
}

float calculateLampAttenuation(float distance) {
    // Quadratic attenuation
    float constant = 1.0f;
    float linear = 0.7f;
    float quadratic = 1.8f;
    
    return 1.0f / (constant + linear * distance + quadratic * distance * distance);
}

