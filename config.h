#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <glm/glm.hpp>

// =====================================================================
// Window and Screen Settings
// =====================================================================
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// =====================================================================
// Light and Scene Position
// =====================================================================
const glm::vec3 DEFAULT_LIGHT_POS(0.0f, 1.5f, 0.0f);
const glm::vec3 DEFAULT_CUBE_POS(0.0f, 0.8f, 0.2f);

// =====================================================================
// Room Size Parameters (can be adjusted to enlarge room)
// =====================================================================
const float ROOM_SCALE_X = 1.2f;   // Width scale (default 1.0)
const float ROOM_SCALE_Y = 1.0f;   // Height scale
const float ROOM_SCALE_Z = 1.2f;   // Depth scale

// =====================================================================
// Terrain Parameters
// =====================================================================
const int TERRAIN_GRID_SIZE = 128;
const float TERRAIN_SCALE_X = 0.3f;
const float TERRAIN_SCALE_Z = 0.225f;
const float TERRAIN_HEIGHT_SCALE = 0.12f;

// =====================================================================
// Weather System Parameters
// =====================================================================
const float DEFAULT_LIGHTNING_INTERVAL = 0.8f;
const int RAIN_PARTICLE_COUNT = 800;
const int SNOW_PARTICLE_COUNT = 500;
const int LIGHTNING_COUNT = 20;
const float MAX_SNOW_HEIGHT = 0.05f;
const float SNOW_ACCUMULATION_RATE = 0.0002f;

// =====================================================================
// Cloud Parameters
// =====================================================================
const glm::vec3 DEFAULT_CLOUD_POS(0.0f, 0.8f, 0.5f);
const float CLOUD_MOVE_SPEED = 2.5f;

// =====================================================================
// Sandbox Boundary Parameters
// =====================================================================
const float SANDBOX_CENTER_X = 0.0f;
const float SANDBOX_CENTER_Z = 0.6f;
const float SANDBOX_HALF_WIDTH = 0.15f;
const float SANDBOX_HALF_DEPTH = 0.1125f;

// =====================================================================
// Camera Default Parameters
// =====================================================================
const glm::vec3 DEFAULT_CAMERA_POS(0.0f, 1.0f, 2.0f);

// =====================================================================
// Lamp Model Parameters
// =====================================================================
const glm::vec3 LAMP_POSITION(0.33f, 0.485f, 0.58f);
const float LAMP_SCALE = 0.08f;
const float LAMP_ROTATION = 0.0f;

#endif // CONFIG_H
