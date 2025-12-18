#include "interactive_object.h"
#include "ray_picking.h"
#include <algorithm>
#include <cmath>

// Global variables
std::vector<InteractiveObject> g_interactiveObjects;
int g_selectedObjectIndex = -1;
int g_hoveredObjectIndex = -1;

// Animation parameters
const float MECHANISM_TRIGGER_ANGLE = 90.0f;
const float ROTATION_ANIMATION_SPEED = 120.0f;  // degrees per second
const float MOVE_ANIMATION_DISTANCE = 0.15f;    // units to move
const float MOVE_ANIMATION_SPEED = 0.3f;        // units per second

void initInteractiveObjects() {
    g_interactiveObjects.clear();
    g_selectedObjectIndex = -1;
    g_hoveredObjectIndex = -1;
}

int addInteractiveObject(const std::string& name, ObjectType type,
                         const glm::vec3& position, float boundingRadius,
                         int meshId) {
    InteractiveObject obj;
    obj.name = name;
    obj.type = type;
    obj.position = position;
    obj.originalPosition = position;  // Save original for reset
    obj.boundingRadius = boundingRadius;
    obj.meshId = meshId;
    obj.scale = glm::vec3(1.0f);
    obj.rotation = glm::vec3(0.0f);
    obj.originalRotation = glm::vec3(0.0f);  // Save original rotation
    obj.boundingCenter = glm::vec3(0.0f);
    obj.hasAnimated = false;
    obj.isResetting = false;
    
    // Set colors based on type
    switch (type) {
        case ObjectType::MOVABLE:
            obj.baseColor = glm::vec3(0.8f, 0.7f, 0.6f);
            break;
        case ObjectType::ROTATABLE:
            obj.baseColor = glm::vec3(0.7f, 0.6f, 0.5f);
            break;
        case ObjectType::MECHANISM:
            obj.baseColor = glm::vec3(0.6f, 0.5f, 0.4f);
            break;
        default:
            obj.baseColor = glm::vec3(0.5f);
    }
    
    g_interactiveObjects.push_back(obj);
    return static_cast<int>(g_interactiveObjects.size()) - 1;
}

int pickObject(float mouseX, float mouseY, int screenWidth, int screenHeight,
               const glm::mat4& view, const glm::mat4& projection,
               const glm::vec3& cameraPos) {
    
    // Generate ray from mouse position
    Ray ray = screenToWorldRay(mouseX, mouseY, screenWidth, screenHeight,
                               view, projection, cameraPos);
    
    int closestIndex = -1;
    float closestDist = 1e9f;
    
    for (size_t i = 0; i < g_interactiveObjects.size(); i++) {
        const InteractiveObject& obj = g_interactiveObjects[i];
        
        // Skip static objects
        if (obj.type == ObjectType::STATIC) continue;
        
        // Calculate world-space bounding center
        glm::vec3 worldCenter = obj.position + obj.boundingCenter;
        
        // Ray-sphere intersection
        if (rayIntersectsSphere(ray, worldCenter, obj.boundingRadius)) {
            // Calculate distance to camera
            float dist = glm::length(worldCenter - cameraPos);
            if (dist < closestDist) {
                closestDist = dist;
                closestIndex = static_cast<int>(i);
            }
        }
    }
    
    return closestIndex;
}

void selectObject(int index) {
    // Deselect previous
    if (g_selectedObjectIndex >= 0 && g_selectedObjectIndex < (int)g_interactiveObjects.size()) {
        g_interactiveObjects[g_selectedObjectIndex].isSelected = false;
    }
    
    g_selectedObjectIndex = index;
    
    // Select new
    if (index >= 0 && index < (int)g_interactiveObjects.size()) {
        g_interactiveObjects[index].isSelected = true;
    }
}

void deselectAll() {
    for (auto& obj : g_interactiveObjects) {
        obj.isSelected = false;
        obj.isHighlighted = false;
    }
    g_selectedObjectIndex = -1;
    g_hoveredObjectIndex = -1;
}

void rotateSelectedObject(float deltaYaw, float deltaPitch) {
    if (g_selectedObjectIndex < 0 || g_selectedObjectIndex >= (int)g_interactiveObjects.size()) {
        return;
    }
    
    InteractiveObject& obj = g_interactiveObjects[g_selectedObjectIndex];
    
    // Only rotate rotatable or mechanism objects
    if (obj.type != ObjectType::ROTATABLE && obj.type != ObjectType::MECHANISM) {
        return;
    }
    
    // Apply rotation
    obj.rotation.y += deltaYaw;
    obj.rotation.x += deltaPitch;
    
    // Clamp rotation
    obj.rotation.x = glm::clamp(obj.rotation.x, -45.0f, 45.0f);
    
    // Check if mechanism triggered (rotated past threshold)
    if (obj.type == ObjectType::MECHANISM && !obj.hasBeenTriggered) {
        if (std::abs(obj.rotation.y) >= MECHANISM_TRIGGER_ANGLE) {
            obj.hasBeenTriggered = true;
        }
    }
}

void moveSelectedObject(const glm::vec3& delta) {
    if (g_selectedObjectIndex < 0 || g_selectedObjectIndex >= (int)g_interactiveObjects.size()) {
        return;
    }
    
    InteractiveObject& obj = g_interactiveObjects[g_selectedObjectIndex];
    
    // Only move movable objects
    if (obj.type != ObjectType::MOVABLE && obj.type != ObjectType::MECHANISM) {
        return;
    }
    
    obj.position += delta;
}

// Trigger automatic animation when object is clicked
void triggerObjectAnimation(int index) {
    if (index < 0 || index >= (int)g_interactiveObjects.size()) {
        return;
    }
    
    InteractiveObject& obj = g_interactiveObjects[index];
    
    // Don't start new animation if already animating
    if (obj.isAnimating) {
        return;
    }
    
    // Start animation based on type
    switch (obj.type) {
        case ObjectType::ROTATABLE:
            // Toggle: rotate or reset
            if (obj.hasAnimated && !obj.isResetting) {
                // Reset to original
                obj.isAnimating = true;
                obj.isResetting = true;
                obj.animationProgress = 0.0f;
            } else {
                // Rotate 90 degrees
                obj.isAnimating = true;
                obj.isResetting = false;
                obj.animationProgress = 0.0f;
            }
            break;
            
        case ObjectType::MOVABLE:
            // Toggle: move or reset
            if (obj.hasAnimated && !obj.isResetting) {
                // Reset to original position
                obj.isAnimating = true;
                obj.isResetting = true;
                obj.animationProgress = 0.0f;
            } else {
                // Move forward
                obj.isAnimating = true;
                obj.isResetting = false;
                obj.animationProgress = 0.0f;
            }
            break;
            
        case ObjectType::MECHANISM:
            // Mechanism: toggle move, but only trigger once
            if (obj.hasAnimated && !obj.isResetting) {
                // Reset to original position
                obj.isAnimating = true;
                obj.isResetting = true;
                obj.animationProgress = 0.0f;
            } else if (!obj.hasBeenTriggered) {
                // Move and trigger
                obj.isAnimating = true;
                obj.isResetting = false;
                obj.animationProgress = 0.0f;
            }
            break;
            
        default:
            break;
    }
}

void updateInteractiveObjects(float deltaTime) {
    for (auto& obj : g_interactiveObjects) {
        // Update animations based on type
        if (obj.isAnimating) {
            switch (obj.type) {
                case ObjectType::ROTATABLE: {
                    float rotationDelta = ROTATION_ANIMATION_SPEED * deltaTime;
                    
                    if (obj.isResetting) {
                        // Reverse rotation back to original
                        float diff = obj.originalRotation.y - obj.rotation.y;
                        if (std::abs(diff) > rotationDelta) {
                            obj.rotation.y += (diff > 0 ? rotationDelta : -rotationDelta);
                        } else {
                            obj.rotation.y = obj.originalRotation.y;
                        }
                    } else {
                        // Forward rotation
                        obj.rotation.y += rotationDelta;
                    }
                    
                    obj.animationProgress += deltaTime * 1.5f;
                    
                    if (obj.animationProgress >= 1.0f) {
                        obj.isAnimating = false;
                        if (obj.isResetting) {
                            obj.rotation = obj.originalRotation;
                            obj.hasAnimated = false;
                            obj.isResetting = false;
                        } else {
                            obj.rotation.y = std::round(obj.rotation.y / 90.0f) * 90.0f;
                            obj.hasAnimated = true;
                        }
                    }
                    break;
                }
                
                case ObjectType::MOVABLE: {
                    float moveDelta = MOVE_ANIMATION_SPEED * deltaTime;
                    
                    if (obj.isResetting) {
                        // Move back to original position
                        glm::vec3 diff = obj.originalPosition - obj.position;
                        float dist = glm::length(diff);
                        if (dist > moveDelta) {
                            obj.position += glm::normalize(diff) * moveDelta;
                        } else {
                            obj.position = obj.originalPosition;
                        }
                    } else {
                        // Move forward (toward camera)
                        obj.position.z -= moveDelta;
                    }
                    
                    obj.animationProgress += deltaTime * 2.0f;
                    
                    if (obj.animationProgress >= 1.0f) {
                        obj.isAnimating = false;
                        if (obj.isResetting) {
                            obj.position = obj.originalPosition;
                            obj.hasAnimated = false;
                            obj.isResetting = false;
                        } else {
                            obj.hasAnimated = true;
                        }
                    }
                    break;
                }
                
                case ObjectType::MECHANISM: {
                    float moveDelta = MOVE_ANIMATION_SPEED * deltaTime;
                    
                    if (obj.isResetting) {
                        // Move back to original position
                        glm::vec3 diff = obj.originalPosition - obj.position;
                        float dist = glm::length(diff);
                        if (dist > moveDelta) {
                            obj.position += glm::normalize(diff) * moveDelta;
                        } else {
                            obj.position = obj.originalPosition;
                        }
                    } else {
                        // Slide to the side
                        obj.position.x += moveDelta;
                    }
                    
                    obj.animationProgress += deltaTime * 2.0f;
                    
                    if (obj.animationProgress >= 1.0f) {
                        obj.isAnimating = false;
                        if (obj.isResetting) {
                            obj.position = obj.originalPosition;
                            obj.hasAnimated = false;
                            obj.isResetting = false;
                            // Note: hasBeenTriggered stays true - mechanism already triggered
                        } else {
                            obj.hasAnimated = true;
                            obj.hasBeenTriggered = true; // Trigger the mechanism!
                        }
                    }
                    break;
                }
                
                default:
                    break;
            }
        }
    }
    
    // Update hover state
    for (size_t i = 0; i < g_interactiveObjects.size(); i++) {
        g_interactiveObjects[i].isHighlighted = (static_cast<int>(i) == g_hoveredObjectIndex);
    }
}

glm::mat4 getObjectModelMatrix(int index) {
    if (index < 0 || index >= (int)g_interactiveObjects.size()) {
        return glm::mat4(1.0f);
    }
    
    const InteractiveObject& obj = g_interactiveObjects[index];
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obj.position);
    model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, obj.scale);
    
    return model;
}

bool checkMechanismTriggered() {
    for (const auto& obj : g_interactiveObjects) {
        if (obj.type == ObjectType::MECHANISM && obj.hasBeenTriggered) {
            return true;
        }
    }
    return false;
}

void cleanupInteractiveObjects() {
    g_interactiveObjects.clear();
    g_selectedObjectIndex = -1;
    g_hoveredObjectIndex = -1;
}





