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
const float ROOM_SCALE_X = 1.8f;   // Width scale (default 1.0)
const float ROOM_SCALE_Y = 1.0f;   // Height scale
const float ROOM_SCALE_Z = 1.2f;   // Depth scale

// =====================================================================
// Terrain Parameters (sandbox terrain size - smaller than table width)
// =====================================================================
const int TERRAIN_GRID_SIZE = 128;
const float TERRAIN_SCALE_X = 0.18f;    // Reduced to fit on table
const float TERRAIN_SCALE_Z = 0.135f;   // Reduced proportionally
const float TERRAIN_HEIGHT_SCALE = 0.08f; // Lower height for smaller sandbox

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
// Sandbox Boundary Parameters (on desk, adjusted for new table position)
// =====================================================================
const float SANDBOX_CENTER_X = -0.38f;   // Aligned with table X
const float SANDBOX_CENTER_Z = 0.2f;    // Aligned with table Z
const float SANDBOX_HALF_WIDTH = 0.09f;  // Half of terrain width
const float SANDBOX_HALF_DEPTH = 0.0675f; // Half of terrain depth

// =====================================================================
// Cloud Parameters (aligned with sandbox center)
// =====================================================================
const glm::vec3 DEFAULT_CLOUD_POS(-0.38f, 0.8f, 0.2f);  // Same X,Z as sandbox center
const float CLOUD_MOVE_SPEED = 2.5f;

// =====================================================================
// Camera Default Parameters
// =====================================================================
const glm::vec3 DEFAULT_CAMERA_POS(0.0f, 1.0f, 2.0f);

// =====================================================================
// Table Position Parameters (table against right wall, facing left wall)
// =====================================================================
const glm::vec3 TABLE_POSITION(-0.52f, 0.3f, 0.2f);  // Near right wall, centered Z
const float TABLE_ROTATION = 0.0f;                  // Facing left wall (toward -X)
const float TABLE_SCALE = 0.08f;

// =====================================================================
// Bookcase Position Parameters (against left wall, facing right)
// =====================================================================
const glm::vec3 BOOKCASE_POSITION(0.80f, 0.6f, 0.2f);  // Near left wall (X positive side)
const float BOOKCASE_ROTATION = 270.0f;               // Facing toward desk
const float BOOKCASE_SCALE = 0.03f;                   // Scale to fit room height

// =====================================================================
// Lamp Model Parameters (on desk, relative to new table position)
// =====================================================================
const glm::vec3 LAMP_POSITION(-0.4f, 0.485f, 0.6f);  // On desk, front-left corner
const float LAMP_SCALE = 0.08f;
const float LAMP_ROTATION = 0.0f;

// =====================================================================
// Puzzle Game Parameters
// =====================================================================

// Interactive objects positions (desk surface at y~0.52, floor at y~0.3)
// Objects on desk near table position (X~0.45, Z~0.2)
const glm::vec3 VASE_POSITION(-0.42f, 0.54f, 0.05f);       // Vase on desk, back side
const glm::vec3 BOOK_POSITION(-0.42f, 0.54f, 0.4f);       // Book on desk, front side (near sandbox)
const glm::vec3 SCROLL_POSITION(-0.2f, 0.3f, 0.5f);        // Scroll on floor, in front of desk

// Object scales (models are ~1 unit, scale to fit scene ~0.1 units)
const float VASE_SCALE = 0.08f;      // Height ~1.15 -> 0.09 units
const float VASE_ROTATION = 0.0f;    // Rotation around Y axis (degrees)
const float BOOK_SCALE = 0.08f;      // Height ~0.96 -> 0.08 units  
const float SCROLL_SCALE = 0.12f;    // Length ~1.04 -> 0.12 units

// Interaction parameters
const float OBJECT_ROTATE_SPEED = 60.0f;    // Degrees per second
const float OBJECT_MOVE_SPEED = 0.3f;       // Units per second
const float OBJECT_PICK_RADIUS = 0.08f;     // Picking bounding sphere

// Floor height (ground level in the room)
// Room floor is at Y = DEFAULT_CUBE_POS.y - 0.5 = 0.8 - 0.5 = 0.3
const float FLOOR_HEIGHT = 0.3f;  // Ground level Y coordinate

// Secret tile position (where the spirit orb is hidden)
// Now in center of room, away from new desk position (desk is at X=0.45)
// Room center is near (0, 0.2), placing tile at center-left area
const float TILE_THICKNESS = 0.01f;
const glm::vec3 SECRET_TILE_POSITION(-0.1f, FLOOR_HEIGHT - TILE_THICKNESS, 0.2f);  // Center-left area
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
