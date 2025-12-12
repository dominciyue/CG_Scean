#pragma once
#ifndef RAY_PICKING_H
#define RAY_PICKING_H

#include <glm/glm.hpp>
#include "camera.h"

// =====================================================================
// Ray Structure
// =====================================================================

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

// =====================================================================
// Ray Picking Functions
// =====================================================================

// Generate a ray from screen coordinates
// screenX, screenY: mouse position in screen coordinates
// screenWidth, screenHeight: window dimensions
// view: view matrix
// projection: projection matrix
// cameraPos: camera position
Ray screenToWorldRay(float screenX, float screenY, 
                     int screenWidth, int screenHeight,
                     const glm::mat4& view, const glm::mat4& projection,
                     const glm::vec3& cameraPos);

// Check if a ray intersects with a sphere
// ray: the ray to test
// sphereCenter: center of the sphere
// sphereRadius: radius of the sphere
// Returns true if intersection occurs
bool rayIntersectsSphere(const Ray& ray, const glm::vec3& sphereCenter, float sphereRadius);

// Check if mouse click hit the lamp
// Uses the lamp's bounding sphere for collision detection
bool checkLampClick(float mouseX, float mouseY,
                    int screenWidth, int screenHeight,
                    const glm::mat4& view, const glm::mat4& projection,
                    const glm::vec3& cameraPos,
                    const glm::vec3& lampPosition, float lampRadius);

#endif // RAY_PICKING_H

