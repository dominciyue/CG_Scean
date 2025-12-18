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

// Check if ray intersects a vertical cylinder (Y-axis aligned)
// cylinderBase: center point at bottom of cylinder
// radius: cylinder radius
// height: cylinder height (extends upward from base)
bool rayIntersectsCylinder(const Ray& ray, 
                           const glm::vec3& cylinderBase,
                           float radius, float height);

// Check if mouse click hit the lamp shade (cylinder only, not base)
bool checkLampClick(float mouseX, float mouseY,
                    int screenWidth, int screenHeight,
                    const glm::mat4& view, const glm::mat4& projection,
                    const glm::vec3& cameraPos);

#endif // RAY_PICKING_H

