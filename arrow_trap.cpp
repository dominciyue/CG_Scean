#include "arrow_trap.h"
#include "config.h"
#include "ray_picking.h"
#include "mesh_collision.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <random>
#include <iostream>

// =====================================================================
// Global Variables
// =====================================================================
TrapState g_trapState = TrapState::IDLE;
BookcaseState g_bookcaseState;
WallCompartment g_wallCompartment;
std::vector<Arrow> g_arrows;
float g_trapTime = 0.0f;
bool g_decoyTriggered = false;

// Internal state
static float s_arrowFireTimer = 0.0f;
static int s_arrowsFiredFromCompartment = 0;
static int s_arrowsFiredFromWindow = 0;
static std::random_device s_rd;
static std::mt19937 s_gen(s_rd());
static std::uniform_real_distribution<float> s_randFloat(-1.0f, 1.0f);

// =====================================================================
// Initialization
// =====================================================================
void initArrowTrap() {
    g_trapState = TrapState::IDLE;
    g_trapTime = 0.0f;
    g_decoyTriggered = false;
    
    // Initialize bookcase state
    g_bookcaseState.originalPosition = BOOKCASE_POSITION;
    g_bookcaseState.currentPosition = BOOKCASE_POSITION;
    // Bookcase slides in Z direction (left/right, parallel to wall)
    g_bookcaseState.targetPosition = BOOKCASE_POSITION + glm::vec3(0.0f, 0.0f, BOOKCASE_MOVE_DISTANCE);
    g_bookcaseState.animationProgress = 0.0f;
    g_bookcaseState.isMoving = false;
    g_bookcaseState.hasMoved = false;
    
    // Initialize wall compartment (on the wall surface, behind bookcase)
    // Position it at the wall (X = right wall position)
    float wallX = 0.5f * ROOM_SCALE_X + DEFAULT_CUBE_POS.x;  // At wall surface
    // Compartment opening is at wall surface, hole extends INTO wall
    g_wallCompartment.position = glm::vec3(wallX - 0.01f, 0.7f, 0.2f);  // At wall surface
    g_wallCompartment.size = WALL_COMPARTMENT_SIZE;
    g_wallCompartment.isVisible = false;
    g_wallCompartment.doorOpen = false;
    g_wallCompartment.trapTriggered = false;
    g_wallCompartment.doorRotation = 0.0f;
    g_wallCompartment.panelSlideProgress = 0.0f;
    g_wallCompartment.panelOpening = false;
    g_wallCompartment.panelFullyOpen = false;
    
    // Initialize arrows
    g_arrows.clear();
    g_arrows.reserve(MAX_ARROWS);
    
    s_arrowFireTimer = 0.0f;
    s_arrowsFiredFromCompartment = 0;
    s_arrowsFiredFromWindow = 0;
}

void cleanupArrowTrap() {
    g_arrows.clear();
}

// =====================================================================
// Main Update Function
// =====================================================================
void updateArrowTrap(float deltaTime) {
    g_trapTime += deltaTime;
    
    switch (g_trapState) {
        case TrapState::IDLE:
            // Waiting for decoy trigger
            break;
            
        case TrapState::BOOKCASE_MOVING:
            updateBookcaseAnimation(deltaTime);
            break;
            
        case TrapState::COMPARTMENT_REVEALED:
            // Waiting for player to click compartment door
            updateCompartmentDoor(deltaTime);
            break;
            
        case TrapState::TRAP_FIRING:
            updateCompartmentDoor(deltaTime);
            
            // Fire arrows at intervals
            s_arrowFireTimer += deltaTime;
            if (s_arrowFireTimer >= ARROW_FIRE_INTERVAL) {
                s_arrowFireTimer = 0.0f;
                
                // Fire from compartment
                if (s_arrowsFiredFromCompartment < ARROWS_PER_BURST) {
                    spawnArrowFromCompartment();
                    s_arrowsFiredFromCompartment++;
                }
                
                // Fire from window (with slight delay)
                if (s_arrowsFiredFromWindow < ARROWS_FROM_WINDOW && 
                    s_arrowsFiredFromCompartment > 2) {
                    spawnArrowFromWindow();
                    s_arrowsFiredFromWindow++;
                }
                
                // Check if all arrows fired
                if (s_arrowsFiredFromCompartment >= ARROWS_PER_BURST &&
                    s_arrowsFiredFromWindow >= ARROWS_FROM_WINDOW) {
                    // Wait for all arrows to finish
                    bool allDone = true;
                    for (const auto& arrow : g_arrows) {
                        if (arrow.active && !arrow.hasCollided) {
                            allDone = false;
                            break;
                        }
                    }
                    if (allDone && g_arrows.size() > 0) {
                        g_trapState = TrapState::TRAP_COMPLETE;
                    }
                }
            }
            break;
            
        case TrapState::TRAP_COMPLETE:
            // Trap finished
            break;
    }
    
    // Always update arrows
    updateArrows(deltaTime);
}

// =====================================================================
// Bookcase Animation
// =====================================================================
void updateBookcaseAnimation(float deltaTime) {
    if (!g_bookcaseState.isMoving) return;
    
    g_bookcaseState.animationProgress += deltaTime / BOOKCASE_MOVE_DURATION;
    
    if (g_bookcaseState.animationProgress >= 1.0f) {
        g_bookcaseState.animationProgress = 1.0f;
        g_bookcaseState.isMoving = false;
        g_bookcaseState.hasMoved = true;
        g_bookcaseState.currentPosition = g_bookcaseState.targetPosition;
        
        // Reveal compartment
        g_wallCompartment.isVisible = true;
        g_trapState = TrapState::COMPARTMENT_REVEALED;
        
        std::cout << "Bookcase moved! Compartment revealed." << std::endl;
    } else {
        // Smooth ease-out animation
        float t = g_bookcaseState.animationProgress;
        float easeT = 1.0f - (1.0f - t) * (1.0f - t);  // Quadratic ease-out
        
        g_bookcaseState.currentPosition = glm::mix(
            g_bookcaseState.originalPosition,
            g_bookcaseState.targetPosition,
            easeT
        );
    }
}

// =====================================================================
// Compartment Door Animation
// =====================================================================
void updateCompartmentDoor(float deltaTime) {
    // Update sliding trapdoor panels animation
    if (g_wallCompartment.panelOpening && !g_wallCompartment.panelFullyOpen) {
        float slideSpeed = 0.6f;  // Speed of panel slide
        g_wallCompartment.panelSlideProgress += slideSpeed * deltaTime;
        
        if (g_wallCompartment.panelSlideProgress >= 1.0f) {
            g_wallCompartment.panelSlideProgress = 1.0f;
            g_wallCompartment.panelFullyOpen = true;
            g_wallCompartment.doorOpen = true;
            
            // Start firing arrows after panels are fully open
            if (!g_wallCompartment.trapTriggered) {
                g_wallCompartment.trapTriggered = true;
                g_trapState = TrapState::TRAP_FIRING;
                std::cout << "Trapdoor panels open! Arrows firing!" << std::endl;
            }
        }
    }
}

// =====================================================================
// Arrow Updates
// =====================================================================
void updateArrows(float deltaTime) {
    for (auto& arrow : g_arrows) {
        if (!arrow.active) continue;
        
        if (!arrow.hasCollided) {
            // Update position with smooth interpolation
            arrow.position += arrow.velocity * deltaTime;
            
            // Apply gravity (slight drop for realistic arc)
            arrow.velocity.y -= 0.8f * deltaTime;
            
            // Update rotation to match velocity direction smoothly
            if (glm::length(arrow.velocity) > 0.01f) {
                glm::vec3 dir = glm::normalize(arrow.velocity);
                float targetYaw = atan2(dir.x, dir.z);
                float targetPitch = -asin(glm::clamp(dir.y, -1.0f, 1.0f));
                
                // Smooth rotation interpolation
                float rotationSmooth = 10.0f * deltaTime;
                arrow.rotation.y += (targetYaw - arrow.rotation.y) * rotationSmooth;
                arrow.rotation.x += (targetPitch - arrow.rotation.x) * rotationSmooth;
            }
            
            // Check collision
            checkArrowCollision(arrow, deltaTime);
        } else {
            // Arrow has collided - update wobble effect
            arrow.collisionTime += deltaTime;
            arrow.wobblePhase += deltaTime * 25.0f;  // Wobble frequency
            arrow.wobbleAmplitude *= 0.95f;  // Decay wobble amplitude
            
            // Add subtle wobble to embedded arrow
            if (arrow.wobbleAmplitude > 0.001f) {
                float wobble = sin(arrow.wobblePhase) * arrow.wobbleAmplitude;
                // Apply wobble perpendicular to collision normal
                if (std::abs(arrow.collisionNormal.y) > 0.9f) {
                    arrow.rotation.z = wobble;
                } else {
                    arrow.rotation.x += wobble * 0.1f;
                }
            }
        }
        
        // Update lifetime (slower decay for collided arrows)
        if (arrow.hasCollided) {
            arrow.life -= deltaTime * 0.5f;  // Collided arrows last longer
        } else {
            arrow.life -= deltaTime;
        }
        
        if (arrow.life <= 0.0f) {
            arrow.active = false;
        }
    }
}

// =====================================================================
// Trigger Functions
// =====================================================================
void triggerDecoyMechanism() {
    if (g_trapState != TrapState::IDLE) return;
    
    g_decoyTriggered = true;
    g_trapState = TrapState::BOOKCASE_MOVING;
    g_bookcaseState.isMoving = true;
    
    std::cout << "Decoy mechanism triggered! Bookcase starting to move..." << std::endl;
}

void triggerArrowTrap() {
    if (g_trapState != TrapState::COMPARTMENT_REVEALED) return;
    if (g_wallCompartment.panelOpening) return;  // Already triggered
    
    // Start trapdoor panel sliding animation (don't fire arrows immediately)
    g_wallCompartment.panelOpening = true;
    g_wallCompartment.panelSlideProgress = 0.0f;
    // Stay in COMPARTMENT_REVEALED state until panels finish opening
    
    s_arrowFireTimer = 0.0f;
    s_arrowsFiredFromCompartment = 0;
    s_arrowsFiredFromWindow = 0;
    
    std::cout << "Compartment clicked! Trapdoor panels sliding open..." << std::endl;
}

bool checkCompartmentClick(float mouseX, float mouseY, int screenWidth, int screenHeight,
                          const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& cameraPos) {
    if (!g_wallCompartment.isVisible) return false;
    if (g_wallCompartment.panelOpening || g_wallCompartment.trapTriggered) return false;
    
    Ray ray = screenToWorldRay(mouseX, mouseY, screenWidth, screenHeight,
                               view, projection, cameraPos);
    
    // Check ray intersection with compartment door
    glm::vec3 doorCenter = g_wallCompartment.position;
    doorCenter.x -= 0.05f;  // Door is on the front face
    
    // Simple sphere check for the door area
    float doorRadius = 0.15f;
    if (rayIntersectsSphere(ray, doorCenter, doorRadius)) {
        return true;
    }
    
    return false;
}

// =====================================================================
// Arrow Spawning
// =====================================================================
void spawnArrowFromCompartment() {
    if (g_arrows.size() >= MAX_ARROWS) return;
    
    Arrow arrow;
    arrow.active = true;
    arrow.hasCollided = false;
    arrow.life = ARROW_LIFETIME;
    arrow.sourceType = 0;
    arrow.collisionTime = 0.0f;
    arrow.wobblePhase = 0.0f;
    arrow.wobbleAmplitude = 0.0f;
    arrow.collisionNormal = glm::vec3(0.0f);
    arrow.collisionPoint = glm::vec3(0.0f);
    
    // Start from compartment position (arrows emerge from the recessed compartment)
    arrow.position = g_wallCompartment.position;
    arrow.position.x -= 0.08f;  // At the opening of the compartment
    
    // Add randomness to spawn position within compartment opening
    arrow.position.y += s_randFloat(s_gen) * g_wallCompartment.size.y * 0.35f;
    arrow.position.z += s_randFloat(s_gen) * g_wallCompartment.size.z * 0.35f;
    
    // Direction: toward room center/desk with spread (shooting into the room)
    glm::vec3 targetDir = glm::normalize(glm::vec3(-1.0f, 0.0f, 0.0f));  // Toward -X (into room)
    targetDir.y += s_randFloat(s_gen) * 0.18f;   // Vertical spread
    targetDir.z += s_randFloat(s_gen) * 0.22f;   // Horizontal spread
    targetDir = glm::normalize(targetDir);
    
    arrow.velocity = targetDir * ARROW_SPEED;
    
    // Initial rotation matches velocity direction
    arrow.rotation.y = atan2(targetDir.x, targetDir.z);
    arrow.rotation.x = -asin(targetDir.y);
    arrow.rotation.z = 0.0f;
    
    g_arrows.push_back(arrow);
}

void spawnArrowFromWindow() {
    if (g_arrows.size() >= MAX_ARROWS) return;
    
    Arrow arrow;
    arrow.active = true;
    arrow.hasCollided = false;
    arrow.life = ARROW_LIFETIME;
    arrow.sourceType = 1;
    arrow.collisionTime = 0.0f;
    arrow.wobblePhase = 0.0f;
    arrow.wobbleAmplitude = 0.0f;
    arrow.collisionNormal = glm::vec3(0.0f);
    arrow.collisionPoint = glm::vec3(0.0f);
    
    // Start from window position (back wall - the window)
    float windowZ = -0.5f * ROOM_SCALE_Z + DEFAULT_CUBE_POS.z + 0.05f;
    arrow.position = glm::vec3(
        s_randFloat(s_gen) * 0.3f,           // Random X within window area
        0.6f + s_randFloat(s_gen) * 0.3f,    // Around window height with spread
        windowZ
    );
    
    // Direction: into the room (+Z) with spread, targeting the room center
    glm::vec3 targetDir = glm::normalize(glm::vec3(
        s_randFloat(s_gen) * 0.35f,          // Horizontal spread
        -0.1f + s_randFloat(s_gen) * 0.12f,  // Slightly downward arc
        1.0f
    ));
    targetDir = glm::normalize(targetDir);
    
    float speed = ARROW_SPEED * (1.2f + s_randFloat(s_gen) * 0.2f);  // Variable speed
    arrow.velocity = targetDir * speed;
    
    // Initial rotation matches velocity direction
    arrow.rotation.y = atan2(targetDir.x, targetDir.z);
    arrow.rotation.x = -asin(targetDir.y);
    arrow.rotation.z = 0.0f;
    
    g_arrows.push_back(arrow);
}

// =====================================================================
// Collision Detection Helper - finds collision normal for AABB
// =====================================================================
glm::vec3 getAABBCollisionNormal(const glm::vec3& point, const glm::vec3& boxMin, const glm::vec3& boxMax) {
    // Find which face the point is closest to
    float distToMinX = std::abs(point.x - boxMin.x);
    float distToMaxX = std::abs(point.x - boxMax.x);
    float distToMinY = std::abs(point.y - boxMin.y);
    float distToMaxY = std::abs(point.y - boxMax.y);
    float distToMinZ = std::abs(point.z - boxMin.z);
    float distToMaxZ = std::abs(point.z - boxMax.z);
    
    float minDist = distToMinX;
    glm::vec3 normal(-1.0f, 0.0f, 0.0f);
    
    if (distToMaxX < minDist) { minDist = distToMaxX; normal = glm::vec3(1.0f, 0.0f, 0.0f); }
    if (distToMinY < minDist) { minDist = distToMinY; normal = glm::vec3(0.0f, -1.0f, 0.0f); }
    if (distToMaxY < minDist) { minDist = distToMaxY; normal = glm::vec3(0.0f, 1.0f, 0.0f); }
    if (distToMinZ < minDist) { minDist = distToMinZ; normal = glm::vec3(0.0f, 0.0f, -1.0f); }
    if (distToMaxZ < minDist) { normal = glm::vec3(0.0f, 0.0f, 1.0f); }
    
    return normal;
}

// Helper function to handle collision response
void handleCollision(Arrow& arrow, const glm::vec3& collisionPoint, const glm::vec3& normal) {
    arrow.hasCollided = true;
    arrow.collisionPoint = collisionPoint;
    arrow.collisionNormal = normal;
    arrow.collisionTime = 0.0f;
    arrow.wobblePhase = 0.0f;
    arrow.wobbleAmplitude = 0.05f;  // Initial wobble amplitude
    
    // Position arrow slightly off the surface (embedded effect)
    float embedDepth = ARROW_LENGTH * 0.3f;  // 30% of arrow embedded
    arrow.position = collisionPoint - normal * embedDepth;
    
    // Stop velocity
    arrow.velocity = glm::vec3(0.0f);
}

// =====================================================================
// Collision Detection
// =====================================================================
bool checkArrowCollision(Arrow& arrow, float deltaTime) {
    // Room boundaries
    float roomMinX = -0.5f * ROOM_SCALE_X + DEFAULT_CUBE_POS.x;
    float roomMaxX = 0.5f * ROOM_SCALE_X + DEFAULT_CUBE_POS.x;
    float roomMinZ = -0.5f * ROOM_SCALE_Z + DEFAULT_CUBE_POS.z;
    float roomMaxZ = 0.5f * ROOM_SCALE_Z + DEFAULT_CUBE_POS.z;
    float floorY = FLOOR_HEIGHT;
    float ceilingY = DEFAULT_CUBE_POS.y + 0.5f * ROOM_SCALE_Y;
    
    // Use previous position for smoother collision (sweep test approximation)
    glm::vec3 prevPos = arrow.position - arrow.velocity * deltaTime;
    
    // Check floor collision
    if (arrow.position.y <= floorY + ARROW_RADIUS) {
        glm::vec3 hitPoint = arrow.position;
        hitPoint.y = floorY;
        handleCollision(arrow, hitPoint, glm::vec3(0.0f, 1.0f, 0.0f));
        // Adjust rotation for floor hit - arrow sticks at an angle
        arrow.rotation.x = -0.3f;  // Slight upward tilt
        return true;
    }
    
    // Check ceiling collision
    if (arrow.position.y >= ceilingY - ARROW_RADIUS) {
        glm::vec3 hitPoint = arrow.position;
        hitPoint.y = ceilingY;
        handleCollision(arrow, hitPoint, glm::vec3(0.0f, -1.0f, 0.0f));
        return true;
    }
    
    // Check left wall collision
    if (arrow.position.x <= roomMinX + ARROW_RADIUS) {
        glm::vec3 hitPoint = arrow.position;
        hitPoint.x = roomMinX;
        handleCollision(arrow, hitPoint, glm::vec3(1.0f, 0.0f, 0.0f));
        return true;
    }
    
    // Check right wall collision
    if (arrow.position.x >= roomMaxX - ARROW_RADIUS) {
        glm::vec3 hitPoint = arrow.position;
        hitPoint.x = roomMaxX;
        handleCollision(arrow, hitPoint, glm::vec3(-1.0f, 0.0f, 0.0f));
        return true;
    }
    
    // Check back wall collision
    if (arrow.position.z <= roomMinZ + ARROW_RADIUS) {
        glm::vec3 hitPoint = arrow.position;
        hitPoint.z = roomMinZ;
        handleCollision(arrow, hitPoint, glm::vec3(0.0f, 0.0f, 1.0f));
        return true;
    }
    
    // Check front wall collision
    if (arrow.position.z >= roomMaxZ - ARROW_RADIUS) {
        glm::vec3 hitPoint = arrow.position;
        hitPoint.z = roomMaxZ;
        handleCollision(arrow, hitPoint, glm::vec3(0.0f, 0.0f, -1.0f));
        return true;
    }
    
    // Check table collision (simplified AABB)
    glm::vec3 tableMin = TABLE_POSITION - glm::vec3(0.3f, 0.0f, 0.2f);
    glm::vec3 tableMax = TABLE_POSITION + glm::vec3(0.3f, 0.25f, 0.2f);
    
    if (arrow.position.x >= tableMin.x && arrow.position.x <= tableMax.x &&
        arrow.position.y >= tableMin.y && arrow.position.y <= tableMax.y &&
        arrow.position.z >= tableMin.z && arrow.position.z <= tableMax.z) {
        glm::vec3 normal = getAABBCollisionNormal(prevPos, tableMin, tableMax);
        handleCollision(arrow, arrow.position, normal);
        return true;
    }
    
    // Check bookcase collision
    glm::vec3 bcPos = g_bookcaseState.currentPosition;
    glm::vec3 bcMin = bcPos - glm::vec3(0.2f, 0.0f, 0.15f);
    glm::vec3 bcMax = bcPos + glm::vec3(0.2f, 0.6f, 0.15f);
    
    if (arrow.position.x >= bcMin.x && arrow.position.x <= bcMax.x &&
        arrow.position.y >= bcMin.y && arrow.position.y <= bcMax.y &&
        arrow.position.z >= bcMin.z && arrow.position.z <= bcMax.z) {
        glm::vec3 normal = getAABBCollisionNormal(prevPos, bcMin, bcMax);
        handleCollision(arrow, arrow.position, normal);
        return true;
    }
    
    // Check mesh-based collision (for lamp, vase, etc.)
    if (checkArrowMeshCollision(arrow, deltaTime)) {
        return true;
    }
    
    return false;
}

bool rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                       const glm::vec3& boxMin, const glm::vec3& boxMax, float& t) {
    glm::vec3 invDir = 1.0f / rayDir;
    glm::vec3 t1 = (boxMin - rayOrigin) * invDir;
    glm::vec3 t2 = (boxMax - rayOrigin) * invDir;
    
    glm::vec3 tMin = glm::min(t1, t2);
    glm::vec3 tMax = glm::max(t1, t2);
    
    float tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
    float tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);
    
    if (tNear > tFar || tFar < 0.0f) return false;
    
    t = tNear > 0.0f ? tNear : tFar;
    return true;
}

// =====================================================================
// Mesh-Based Collision Detection
// =====================================================================
bool checkArrowMeshCollision(Arrow& arrow, float deltaTime) {
    // Use sweep test from previous position to current position
    glm::vec3 prevPos = arrow.position - arrow.velocity * deltaTime;
    
    // Perform sweep test against all registered collision meshes
    CollisionResult result = sweepTestAllMeshes(prevPos, arrow.position, ARROW_RADIUS);
    
    if (result.hit) {
        // Handle collision
        handleCollision(arrow, result.point, result.normal);
        
        std::cout << "[Arrow] Hit mesh: " << result.meshName 
                  << " at (" << result.point.x << ", " << result.point.y << ", " << result.point.z << ")" 
                  << std::endl;
        
        return true;
    }
    
    return false;
}

// =====================================================================
// Matrix Helpers
// =====================================================================
glm::mat4 getBookcaseMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, g_bookcaseState.currentPosition);
    model = glm::rotate(model, glm::radians(BOOKCASE_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(BOOKCASE_SCALE));
    return model;
}

glm::mat4 getCompartmentMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, g_wallCompartment.position);
    return model;
}

glm::mat4 getCompartmentDoorMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    
    // Door position (on the front face of compartment)
    glm::vec3 doorPos = g_wallCompartment.position;
    doorPos.x -= g_wallCompartment.size.x * 0.5f;
    
    model = glm::translate(model, doorPos);
    
    // Rotate door on hinge (left edge)
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -g_wallCompartment.size.z * 0.5f));
    model = glm::rotate(model, glm::radians(-g_wallCompartment.doorRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, g_wallCompartment.size.z * 0.5f));
    
    return model;
}

glm::mat4 getArrowMatrix(const Arrow& arrow) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, arrow.position);
    model = glm::rotate(model, arrow.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, arrow.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(ARROW_RADIUS, ARROW_RADIUS, ARROW_LENGTH));
    return model;
}

// =====================================================================
// Mesh Generation
// =====================================================================
void generateCompartmentDoorMesh(Mesh& mesh) {
    mesh.vertices.clear();
    mesh.indices.clear();
    
    float w = WALL_COMPARTMENT_SIZE.x;
    float h = WALL_COMPARTMENT_SIZE.y;
    float d = 0.02f;  // Door thickness
    
    // Simple quad for door
    Vertex v;
    v.Normal = glm::vec3(-1.0f, 0.0f, 0.0f);  // Facing out
    
    // Front face
    v.Position = glm::vec3(0.0f, -h/2, -w/2); v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0.0f, -h/2,  w/2); v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0.0f,  h/2,  w/2); v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0.0f,  h/2, -w/2); v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    mesh.indices = {0, 1, 2, 2, 3, 0};
    
    setupMesh(mesh);
}

void generateArrowMesh(Mesh& mesh) {
    mesh.vertices.clear();
    mesh.indices.clear();
    
    // Arrow is a simple elongated shape pointing in +Z direction
    const int segments = 8;
    const float shaftLength = 0.8f;
    const float headLength = 0.2f;
    const float shaftRadius = 0.3f;
    const float headRadius = 0.6f;
    
    // Generate shaft vertices
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159f;
        float x = cos(angle) * shaftRadius;
        float y = sin(angle) * shaftRadius;
        
        Vertex v;
        v.Normal = glm::normalize(glm::vec3(x, y, 0.0f));
        
        // Back of shaft
        v.Position = glm::vec3(x, y, -0.5f);
        v.TexCoords = glm::vec2((float)i / segments, 0.0f);
        mesh.vertices.push_back(v);
        
        // Front of shaft (where head starts)
        v.Position = glm::vec3(x, y, shaftLength - 0.5f);
        v.TexCoords = glm::vec2((float)i / segments, 0.8f);
        mesh.vertices.push_back(v);
    }
    
    // Generate shaft indices
    for (int i = 0; i < segments; i++) {
        int base = i * 2;
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 3);
        mesh.indices.push_back(base + 2);
    }
    
    // Arrow head (cone) - tip vertex
    int tipIndex = mesh.vertices.size();
    Vertex tipVertex;
    tipVertex.Position = glm::vec3(0.0f, 0.0f, 0.5f);  // Tip at +Z
    tipVertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
    tipVertex.TexCoords = glm::vec2(0.5f, 1.0f);
    mesh.vertices.push_back(tipVertex);
    
    // Head base vertices
    int headBaseStart = mesh.vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159f;
        float x = cos(angle) * headRadius;
        float y = sin(angle) * headRadius;
        
        Vertex v;
        v.Position = glm::vec3(x, y, shaftLength - 0.5f);
        v.Normal = glm::normalize(glm::vec3(x, y, 0.3f));
        v.TexCoords = glm::vec2((float)i / segments, 0.8f);
        mesh.vertices.push_back(v);
    }
    
    // Head indices (cone)
    for (int i = 0; i < segments; i++) {
        mesh.indices.push_back(tipIndex);
        mesh.indices.push_back(headBaseStart + i + 1);
        mesh.indices.push_back(headBaseStart + i);
    }
    
    setupMesh(mesh);
}

// =====================================================================
// State Queries
// =====================================================================
bool isBookcaseMoving() {
    return g_bookcaseState.isMoving;
}

bool isCompartmentVisible() {
    return g_wallCompartment.isVisible;
}

bool isTrapActive() {
    return g_trapState == TrapState::TRAP_FIRING;
}

glm::vec3 getBookcaseCurrentPosition() {
    return g_bookcaseState.currentPosition;
}

