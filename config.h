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

// =====================================================================
// Puzzle Game Parameters
// =====================================================================

// Interactive objects positions (desk surface at y~0.52, floor at y~-0.48)
// Vase and Book on desk at sandbox center line (Z~0.6), Scroll on floor symmetric to lamp
const glm::vec3 VASE_POSITION(-0.2f, 0.54f, 0.6f);        // Vase on desk left of sandbox center
const glm::vec3 BOOK_POSITION(0.2f, 0.54f, 0.6f);         // Book on desk right of sandbox center
const glm::vec3 SCROLL_POSITION(-0.33f, 0.3f, 0.58f);    // Scroll on floor, symmetric to lamp (lamp at x=0.33)

// Object scales (models are ~1 unit, scale to fit scene ~0.1 units)
const float VASE_SCALE = 0.08f;      // Height ~1.15 -> 0.09 units
const float BOOK_SCALE = 0.08f;      // Height ~0.96 -> 0.08 units  
const float SCROLL_SCALE = 0.12f;    // Length ~1.04 -> 0.12 units

// Interaction parameters
const float OBJECT_ROTATE_SPEED = 60.0f;    // Degrees per second
const float OBJECT_MOVE_SPEED = 0.3f;       // Units per second
const float OBJECT_PICK_RADIUS = 0.08f;     // Picking bounding sphere

// Floor height (ground level in the room)
// Room floor is at Y = DEFAULT_CUBE_POS.y - 0.5 = 0.8 - 0.5 = 0.3
const float FLOOR_HEIGHT = 0.3f;  // Ground level Y coordinate

// Secret tile position (where the spirit orb is hidden) - between desk and window
// Must be within room bounds: X near 0, Z between desk(0.6) and window(-0.4)
// Room center Z is at DEFAULT_CUBE_POS.z = 0.2
// Tile thickness is 0.01, so position Y = FLOOR_HEIGHT - 0.01 to make top surface flush with floor
const float TILE_THICKNESS = 0.01f;
const glm::vec3 SECRET_TILE_POSITION(0.0f, FLOOR_HEIGHT - TILE_THICKNESS, 0.1f);  // Top surface at floor level
const glm::vec3 COMPARTMENT_SIZE_CONFIG(0.15f, 0.2f, 0.15f);  // Compartment size (deeper)

// Spirit orb parameters
const glm::vec3 ORB_COLOR_CORE(0.4f, 0.8f, 1.0f);
const glm::vec3 ORB_COLOR_GLOW(0.2f, 0.6f, 0.9f);
const float ORB_RADIUS = 0.05f;            // Larger orb for visibility
const float ORB_LIGHT_INTENSITY = 2.5f;
const float ORB_LIGHT_RADIUS = 1.0f;        // Light affects wider area

// Spirit orb animation parameters
// Window center is at Y = 0.8 (DEFAULT_CUBE_POS.y)
const float ORB_START_HEIGHT = -0.1f;      // Initial height inside compartment (relative to tile position)
const float ORB_FINAL_HEIGHT = 0.5f;       // Final height: 0.3 + 0.5 = 0.8 (window center)
const float ORB_RISE_DURATION = 3.0f;      // Seconds to rise to final position
const float ORB_RISE_SPEED = 0.15f;        // Units per second

#endif // CONFIG_H
