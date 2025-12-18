#pragma once
#ifndef PUZZLE_GAME_H
#define PUZZLE_GAME_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "types.h"
#include "mesh.h"

// Game states
enum class GameState {
    PLAYING,            // Player is searching
    MECHANISM_TRIGGERED,// Mechanism has been activated
    TILE_ANIMATING,     // Floor tile is flipping
    COMPARTMENT_OPEN,   // Secret compartment revealed
    ORB_REVEALED,       // Spirit orb is visible
    GAME_COMPLETE       // Player has found the orb
};

// Floor tile animation structure
struct FloorTile {
    glm::vec3 position;
    float rotation;         // Current rotation angle (0 = closed, 90 = open)
    float targetRotation;
    bool isAnimating;
    bool isOpen;
};

// Secret compartment structure
struct SecretCompartment {
    glm::vec3 position;
    glm::vec3 size;
    bool isVisible;
};

// Spirit orb rising animation
struct OrbAnimation {
    float currentHeight;      // Current Y position relative to tile
    float targetHeight;       // Final Y position
    float riseProgress;       // 0.0 to 1.0
    bool isRising;            // Currently animating
    bool hasReachedFinal;     // Reached final position
};

// Puzzle game state
extern GameState g_gameState;
extern FloorTile g_secretTile;
extern SecretCompartment g_compartment;
extern OrbAnimation g_orbAnimation;
extern float g_gameTime;
extern bool g_orbCollected;

// Initialize puzzle game
void initPuzzleGame();

// Update puzzle game state
void updatePuzzleGame(float deltaTime);

// Trigger the mechanism (called when correct object is interacted with)
void triggerMechanism();

// Check if player clicked on the orb
bool checkOrbClick(float mouseX, float mouseY, int screenWidth, int screenHeight,
                   const glm::mat4& view, const glm::mat4& projection,
                   const glm::vec3& cameraPos);

// Collect the orb
void collectOrb();

// Get floor tile model matrix
glm::mat4 getFloorTileMatrix();

// Generate floor tile mesh (the one that flips)
void generateFloorTileMesh(Mesh& mesh);

// Generate compartment mesh (the dark hole beneath)
void generateCompartmentMesh(Mesh& mesh);

// Get current game state
GameState getGameState();

// Check if orb should be rendered
bool shouldRenderOrb();

// Get orb position (with rising animation)
glm::vec3 getOrbPosition();

// Get current orb Y height (for animation)
float getOrbCurrentHeight();

// Check if orb should emit light (after fully risen)
bool isOrbEmittingLight();

// Get orb light intensity (pulsing effect)
float getOrbLightIntensity();

// Reset game
void resetPuzzleGame();

// Cleanup
void cleanupPuzzleGame();

#endif // PUZZLE_GAME_H






