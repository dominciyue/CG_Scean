#pragma once
#ifndef INTERACTIVE_OBJECT_H
#define INTERACTIVE_OBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <functional>

// Interactive object types
enum class ObjectType {
    STATIC,         // Cannot be interacted with
    MOVABLE,        // Can be moved
    ROTATABLE,      // Can be rotated
    MECHANISM,      // Trigger mechanism (correct puzzle answer)
    DECOY           // Decoy trap trigger (triggers arrow trap)
};

// Interactive object structure
struct InteractiveObject {
    std::string name;
    ObjectType type;
    
    // Transform
    glm::vec3 position;
    glm::vec3 rotation;      // Euler angles in degrees
    glm::vec3 scale;
    
    // Original transform (for reset)
    glm::vec3 originalPosition;
    glm::vec3 originalRotation;
    
    // Bounding sphere for picking
    float boundingRadius;
    glm::vec3 boundingCenter; // Offset from position
    
    // Interaction state
    bool isSelected;
    bool isHighlighted;
    bool hasBeenTriggered;   // For mechanism objects
    bool hasAnimated;        // Has completed animation (for toggle reset)
    bool isResetting;        // Currently resetting to original state
    
    // Animation
    float animationProgress; // 0.0 to 1.0
    bool isAnimating;
    
    // Visual properties
    glm::vec3 baseColor;
    glm::vec3 highlightColor;
    
    // Render ID (for identifying which mesh to render)
    int meshId;
    
    // Constructor
    InteractiveObject() 
        : name(""), type(ObjectType::STATIC),
          position(0.0f), rotation(0.0f), scale(1.0f),
          originalPosition(0.0f), originalRotation(0.0f),
          boundingRadius(0.1f), boundingCenter(0.0f),
          isSelected(false), isHighlighted(false), hasBeenTriggered(false),
          hasAnimated(false), isResetting(false),
          animationProgress(0.0f), isAnimating(false),
          baseColor(1.0f), highlightColor(1.0f, 0.8f, 0.3f),
          meshId(-1) {}
};

// Global interactive objects list
extern std::vector<InteractiveObject> g_interactiveObjects;
extern int g_selectedObjectIndex;
extern int g_hoveredObjectIndex;

// Initialize interactive objects system
void initInteractiveObjects();

// Add an interactive object
int addInteractiveObject(const std::string& name, ObjectType type,
                         const glm::vec3& position, float boundingRadius,
                         int meshId = -1);

// Ray-object intersection test
int pickObject(float mouseX, float mouseY, int screenWidth, int screenHeight,
               const glm::mat4& view, const glm::mat4& projection,
               const glm::vec3& cameraPos);

// Select/deselect object
void selectObject(int index);
void deselectAll();

// Object interaction (legacy drag-based)
void rotateSelectedObject(float deltaYaw, float deltaPitch);
void moveSelectedObject(const glm::vec3& delta);

// Click-based interaction - triggers automatic animation
void triggerObjectAnimation(int index);

// Update all interactive objects (animations, etc.)
void updateInteractiveObjects(float deltaTime);

// Get object model matrix
glm::mat4 getObjectModelMatrix(int index);

// Check if mechanism was triggered
bool checkMechanismTriggered();

// Cleanup
void cleanupInteractiveObjects();

#endif // INTERACTIVE_OBJECT_H





