#pragma once
#ifndef ARROW_TRAP_H
#define ARROW_TRAP_H

#include <glm/glm.hpp>
#include <vector>
#include "mesh.h"

// =====================================================================
// Arrow Particle Structure
// =====================================================================
struct Arrow {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 rotation;          // Euler angles for arrow orientation
    float life;                  // Remaining lifetime
    bool active;
    bool hasCollided;            // Has hit something
    glm::vec3 collisionPoint;    // Where it hit
    glm::vec3 collisionNormal;   // Surface normal at collision point
    int sourceType;              // 0 = wall compartment, 1 = window
    float collisionTime;         // Time since collision (for effects)
    float wobblePhase;           // For arrow wobble effect after impact
    float wobbleAmplitude;       // Decreasing amplitude for wobble
};

// =====================================================================
// Bookcase Animation State
// =====================================================================
struct BookcaseState {
    glm::vec3 originalPosition;
    glm::vec3 targetPosition;
    glm::vec3 currentPosition;
    float animationProgress;  // 0.0 to 1.0
    bool isMoving;
    bool hasMoved;            // Animation complete
};

// =====================================================================
// Wall Compartment (hidden behind bookcase)
// =====================================================================
struct WallCompartment {
    glm::vec3 position;
    glm::vec3 size;
    bool isVisible;           // Only visible after bookcase moves
    bool doorOpen;            // Door clicked open
    bool trapTriggered;       // Arrows fired
    float doorRotation;       // Door swing animation (legacy)
    
    // Trapdoor sliding animation
    float panelSlideProgress; // 0.0 = closed, 1.0 = fully open
    bool panelOpening;        // Is panel currently sliding open
    bool panelFullyOpen;      // Panel has finished opening
};

// =====================================================================
// Arrow Trap System State
// =====================================================================
enum class TrapState {
    IDLE,                     // Waiting for trigger
    BOOKCASE_MOVING,          // Bookcase sliding
    COMPARTMENT_REVEALED,     // Waiting for click
    TRAP_FIRING,              // Arrows shooting
    TRAP_COMPLETE             // All arrows fired/stopped
};

// =====================================================================
// Global Variables (extern declarations)
// =====================================================================
extern TrapState g_trapState;
extern BookcaseState g_bookcaseState;
extern WallCompartment g_wallCompartment;
extern std::vector<Arrow> g_arrows;
extern float g_trapTime;
extern bool g_decoyTriggered;

// =====================================================================
// Configuration Constants
// =====================================================================
const int MAX_ARROWS = 100;                   // Increased max arrows
const float ARROW_SPEED = 2.5f;               // Slightly slower for visibility
const float ARROW_LIFETIME = 5.0f;            // Longer lifetime
const float ARROW_LENGTH = 0.15f;
const float ARROW_RADIUS = 0.008f;            // Slightly thicker

const float BOOKCASE_MOVE_DISTANCE = 0.5f;    // How far bookcase slides (in Z direction) - more distance
const float BOOKCASE_MOVE_SPEED = 0.15f;      // Units per second
const float BOOKCASE_MOVE_DURATION = 3.5f;    // Seconds to complete

const glm::vec3 WALL_COMPARTMENT_SIZE(0.28f, 0.28f, 0.08f);  // Larger, near-square, recessed into wall
const float COMPARTMENT_DOOR_SPEED = 120.0f;  // Degrees per second

const float ARROW_FIRE_INTERVAL = 0.08f;      // Faster firing (more arrows)
const int ARROWS_PER_BURST = 20;              // More arrows from compartment
const int ARROWS_FROM_WINDOW = 12;            // More arrows from window

// =====================================================================
// Function Declarations
// =====================================================================

// Initialization and cleanup
void initArrowTrap();
void cleanupArrowTrap();

// Update functions
void updateArrowTrap(float deltaTime);
void updateBookcaseAnimation(float deltaTime);
void updateArrows(float deltaTime);
void updateCompartmentDoor(float deltaTime);

// Trigger functions
void triggerDecoyMechanism();      // Called when vase is interacted
void triggerArrowTrap();           // Called when compartment door clicked
bool checkCompartmentClick(float mouseX, float mouseY, int screenWidth, int screenHeight,
                          const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& cameraPos);

// Arrow spawning
void spawnArrowFromCompartment();
void spawnArrowFromWindow();
void fireArrowBurst();

// Collision detection
bool checkArrowCollision(Arrow& arrow, float deltaTime);
bool rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                       const glm::vec3& boxMin, const glm::vec3& boxMax, float& t);

// Mesh-based collision detection
bool checkArrowMeshCollision(Arrow& arrow, float deltaTime);

// Rendering helpers
glm::mat4 getBookcaseMatrix();
glm::mat4 getCompartmentMatrix();
glm::mat4 getCompartmentDoorMatrix();
glm::mat4 getArrowMatrix(const Arrow& arrow);

// Mesh generation
void generateCompartmentDoorMesh(Mesh& mesh);
void generateArrowMesh(Mesh& mesh);

// State queries
bool isBookcaseMoving();
bool isCompartmentVisible();
bool isTrapActive();
glm::vec3 getBookcaseCurrentPosition();

#endif // ARROW_TRAP_H

