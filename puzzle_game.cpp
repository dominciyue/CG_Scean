#include "puzzle_game.h"
#include "interactive_object.h"
#include "ray_picking.h"
#include "config.h"
#include <cmath>

// Global variables
GameState g_gameState = GameState::PLAYING;
FloorTile g_secretTile;
SecretCompartment g_compartment;
OrbAnimation g_orbAnimation;
float g_gameTime = 0.0f;
bool g_orbCollected = false;

// Animation parameters
const float TILE_FLIP_SPEED = 120.0f;  // Degrees per second
const float TILE_OPEN_ANGLE = -90.0f;   // Rotation when fully open (positive = flip toward window/back)

// Positions (relative to room) - use config values
const glm::vec3 SECRET_TILE_POS = SECRET_TILE_POSITION;
const glm::vec3 COMPARTMENT_SIZE = COMPARTMENT_SIZE_CONFIG;

void initPuzzleGame() {
    g_gameState = GameState::PLAYING;
    g_gameTime = 0.0f;
    g_orbCollected = false;
    
    // Initialize secret floor tile
    g_secretTile.position = SECRET_TILE_POS;
    g_secretTile.rotation = 0.0f;
    g_secretTile.targetRotation = 0.0f;
    g_secretTile.isAnimating = false;
    g_secretTile.isOpen = false;
    
    // Initialize compartment - positioned below the tile
    // When tile flips toward window (negative Z), compartment opens toward camera (positive Z)
    g_compartment.position = glm::vec3(
        SECRET_TILE_POS.x,
        SECRET_TILE_POS.y - COMPARTMENT_SIZE.y,  // Below the tile
        SECRET_TILE_POS.z                         // Same Z as tile front edge
    );
    g_compartment.size = COMPARTMENT_SIZE;
    g_compartment.isVisible = false;
    
    // Initialize orb animation
    g_orbAnimation.currentHeight = ORB_START_HEIGHT;
    g_orbAnimation.targetHeight = ORB_FINAL_HEIGHT;
    g_orbAnimation.riseProgress = 0.0f;
    g_orbAnimation.isRising = false;
    g_orbAnimation.hasReachedFinal = false;
}

void updatePuzzleGame(float deltaTime) {
    g_gameTime += deltaTime;
    
    // Check if mechanism was triggered
    if (g_gameState == GameState::PLAYING && checkMechanismTriggered()) {
        triggerMechanism();
    }
    
    // Update floor tile animation
    if (g_secretTile.isAnimating) {
        float diff = g_secretTile.targetRotation - g_secretTile.rotation;
        float step = TILE_FLIP_SPEED * deltaTime;
        
        if (std::abs(diff) <= step) {
            g_secretTile.rotation = g_secretTile.targetRotation;
            g_secretTile.isAnimating = false;
            
            // State transition when animation completes
            if (g_gameState == GameState::TILE_ANIMATING) {
                g_gameState = GameState::COMPARTMENT_OPEN;
                g_compartment.isVisible = true;
                
                // Start orb rising animation
                g_orbAnimation.isRising = true;
                g_gameState = GameState::ORB_REVEALED;
            }
        } else {
            g_secretTile.rotation += (diff > 0 ? step : -step);
        }
    }
    
    // Update orb rising animation
    if (g_orbAnimation.isRising && !g_orbAnimation.hasReachedFinal) {
        // Smooth ease-out animation
        g_orbAnimation.riseProgress += deltaTime / ORB_RISE_DURATION;
        
        if (g_orbAnimation.riseProgress >= 1.0f) {
            g_orbAnimation.riseProgress = 1.0f;
            g_orbAnimation.currentHeight = g_orbAnimation.targetHeight;
            g_orbAnimation.hasReachedFinal = true;
            g_orbAnimation.isRising = false;
        } else {
            // Smooth ease-out interpolation
            float t = g_orbAnimation.riseProgress;
            float easeOut = 1.0f - (1.0f - t) * (1.0f - t);  // Quadratic ease-out
            g_orbAnimation.currentHeight = ORB_START_HEIGHT + 
                (ORB_FINAL_HEIGHT - ORB_START_HEIGHT) * easeOut;
        }
    }
}

void triggerMechanism() {
    if (g_gameState != GameState::PLAYING) return;
    
    g_gameState = GameState::MECHANISM_TRIGGERED;
    
    // Start floor tile flip animation
    g_secretTile.targetRotation = TILE_OPEN_ANGLE;
    g_secretTile.isAnimating = true;
    g_secretTile.isOpen = true;
    
    g_gameState = GameState::TILE_ANIMATING;
}

bool checkOrbClick(float mouseX, float mouseY, int screenWidth, int screenHeight,
                   const glm::mat4& view, const glm::mat4& projection,
                   const glm::vec3& cameraPos) {
    
    if (g_gameState != GameState::ORB_REVEALED) return false;
    
    Ray ray = screenToWorldRay(mouseX, mouseY, screenWidth, screenHeight,
                               view, projection, cameraPos);
    
    glm::vec3 orbPos = getOrbPosition();
    return rayIntersectsSphere(ray, orbPos, ORB_RADIUS * 2.0f);
}

void collectOrb() {
    if (g_gameState == GameState::ORB_REVEALED) {
        g_orbCollected = true;
        g_gameState = GameState::GAME_COMPLETE;
    }
}

glm::mat4 getFloorTileMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, g_secretTile.position);
    
    // Rotate around the front edge (hinge point at z = 0)
    // Tile flips open toward the window (backward direction)
    // When rotation is 90 degrees, tile stands vertical behind the compartment
    model = glm::rotate(model, glm::radians(g_secretTile.rotation), glm::vec3(1.0f, 0.0f, 0.0f));
    
    return model;
}

void generateFloorTileMesh(Mesh& mesh) {
    mesh.vertices.clear();
    mesh.indices.clear();
    
    // Tile dimensions - match compartment size
    float w = COMPARTMENT_SIZE.x;  // Width
    float h = 0.01f;               // Height (thickness) - thinner to avoid z-fighting
    float d = COMPARTMENT_SIZE.z;  // Depth
    
    // Calculate texture coordinates to match floor tiling
    // Room floor: local coords (-0.5 to 0.5), scaled by ROOM_SCALE, offset by cubePos
    // Room floor center: cubePos = (0, 0.8, 0.2)
    // Floor local X range: -0.5 to 0.5 -> world: -0.6 to 0.6 (with scale 1.2)
    // Floor local Z range: -0.5 to 0.5 -> world: -0.4 to 0.8 (with scale 1.2, offset 0.2)
    // Texture: 0 to 4 over this range
    
    float texScale = 4.0f;
    
    // Convert world position to floor local position, then to UV
    // SECRET_TILE_POS is in world coords, need to convert to room-local
    float roomCenterX = 0.0f;  // DEFAULT_CUBE_POS.x
    float roomCenterZ = 0.2f;  // DEFAULT_CUBE_POS.z
    
    // Tile position relative to room center
    float localX = (SECRET_TILE_POS.x - roomCenterX) / ROOM_SCALE_X;  // -0.5 to 0.5 range
    float localZ = (SECRET_TILE_POS.z - roomCenterZ) / ROOM_SCALE_Z;
    
    // UV coordinates (0 to 4 over -0.5 to 0.5 range)
    float baseU = (localX + 0.5f) * texScale;
    float baseV = (localZ + 0.5f) * texScale;
    float tileU = (w / ROOM_SCALE_X) * texScale;
    float tileV = (d / ROOM_SCALE_Z) * texScale;
    
    // Define vertices for the tile (a flat box)
    // Top face - normal pointing DOWN to match floor's outward normal
    Vertex v;
    v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);  // Same as room floor
    
    // Top face vertices with aligned texture coordinates
    v.Position = glm::vec3(0, h, 0);     v.TexCoords = glm::vec2(baseU, baseV); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, 0);     v.TexCoords = glm::vec2(baseU + tileU, baseV); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, d);     v.TexCoords = glm::vec2(baseU + tileU, baseV + tileV); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, d);     v.TexCoords = glm::vec2(baseU, baseV + tileV); mesh.vertices.push_back(v);
    
    // Bottom face
    v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    v.Position = glm::vec3(0, 0, d);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, d);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, 0);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, 0, 0);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Front face
    v.Normal = glm::vec3(0.0f, 0.0f, -1.0f);
    v.Position = glm::vec3(0, 0, 0);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, 0);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, 0);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, 0);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Back face
    v.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v.Position = glm::vec3(w, 0, d);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, 0, d);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, d);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, d);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Left face
    v.Normal = glm::vec3(-1.0f, 0.0f, 0.0f);
    v.Position = glm::vec3(0, 0, d);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, 0, 0);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, 0);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, d);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Right face
    v.Normal = glm::vec3(1.0f, 0.0f, 0.0f);
    v.Position = glm::vec3(w, 0, 0);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, d);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, d);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, 0);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Indices for 6 faces (2 triangles each)
    for (unsigned int face = 0; face < 6; face++) {
        unsigned int base = face * 4;
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }
}

void generateCompartmentMesh(Mesh& mesh) {
    mesh.vertices.clear();
    mesh.indices.clear();
    
    // Compartment is a box with open top
    float w = COMPARTMENT_SIZE.x;
    float h = COMPARTMENT_SIZE.y;
    float d = COMPARTMENT_SIZE.z;
    
    Vertex v;
    
    // Bottom face (inside visible)
    v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v.Position = glm::vec3(0, 0, 0);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, 0);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, d);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, 0, d);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Front inner wall
    v.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v.Position = glm::vec3(0, 0, 0);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, 0);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, 0);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, 0);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Back inner wall
    v.Normal = glm::vec3(0.0f, 0.0f, -1.0f);
    v.Position = glm::vec3(w, 0, d);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, 0, d);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, d);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, d);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Left inner wall
    v.Normal = glm::vec3(1.0f, 0.0f, 0.0f);
    v.Position = glm::vec3(0, 0, d);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, 0, 0);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, 0);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0, h, d);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Right inner wall
    v.Normal = glm::vec3(-1.0f, 0.0f, 0.0f);
    v.Position = glm::vec3(w, 0, 0);     v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, 0, d);     v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, d);     v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(w, h, 0);     v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    // Indices for 5 faces
    for (unsigned int face = 0; face < 5; face++) {
        unsigned int base = face * 4;
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }
}

GameState getGameState() {
    return g_gameState;
}

bool shouldRenderOrb() {
    return (g_gameState == GameState::ORB_REVEALED || 
            g_gameState == GameState::GAME_COMPLETE) && !g_orbCollected;
}

glm::vec3 getOrbPosition() {
    // Orb position based on animation state
    // Base X,Z is at center of the tile
    float baseX = g_secretTile.position.x + COMPARTMENT_SIZE.x * 0.5f;
    float baseZ = g_secretTile.position.z + COMPARTMENT_SIZE.z * 0.5f;
    
    // Y position from animation (relative to floor level -0.5)
    float floorY = g_secretTile.position.y;
    float orbY = floorY + g_orbAnimation.currentHeight;
    
    return glm::vec3(baseX, orbY, baseZ);
}

float getOrbCurrentHeight() {
    return g_orbAnimation.currentHeight;
}

bool isOrbEmittingLight() {
    // Orb emits light once it starts rising
    return (g_gameState == GameState::ORB_REVEALED || 
            g_gameState == GameState::GAME_COMPLETE) && 
           g_orbAnimation.riseProgress > 0.3f;  // Start emitting after 30% rise
}

float getOrbLightIntensity() {
    if (!isOrbEmittingLight()) return 0.0f;
    
    // Pulsing effect after reaching final position
    float baseIntensity = ORB_LIGHT_INTENSITY;
    
    if (g_orbAnimation.hasReachedFinal) {
        // Add pulsing glow
        baseIntensity *= (0.8f + 0.2f * std::sin(g_gameTime * 3.0f));
    } else {
        // Gradual brightness increase during rise
        baseIntensity *= g_orbAnimation.riseProgress;
    }
    
    return baseIntensity;
}

void resetPuzzleGame() {
    initPuzzleGame();
    
    // Reset mechanism objects
    for (auto& obj : g_interactiveObjects) {
        if (obj.type == ObjectType::MECHANISM) {
            obj.hasBeenTriggered = false;
            obj.rotation = glm::vec3(0.0f);
        }
    }
}

void cleanupPuzzleGame() {
    // Nothing to clean up for now
}

