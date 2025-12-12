#include "ray_picking.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// =====================================================================
// Ray Picking Implementation
// =====================================================================

Ray screenToWorldRay(float screenX, float screenY, 
                     int screenWidth, int screenHeight,
                     const glm::mat4& view, const glm::mat4& projection,
                     const glm::vec3& cameraPos) {
    Ray ray;
    ray.origin = cameraPos;
    
    // Convert screen coordinates to normalized device coordinates (NDC)
    // Screen coordinates: (0,0) at top-left, (width, height) at bottom-right
    // NDC: (-1,-1) at bottom-left, (1,1) at top-right
    float x = (2.0f * screenX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / screenHeight;  // Flip Y axis
    float z = 1.0f;
    
    // Ray in NDC (pointing into the screen)
    glm::vec3 rayNDC(x, y, z);
    
    // Convert to clip coordinates
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    
    // Convert to eye coordinates
    glm::mat4 invProjection = glm::inverse(projection);
    glm::vec4 rayEye = invProjection * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);  // Direction, not point
    
    // Convert to world coordinates
    glm::mat4 invView = glm::inverse(view);
    glm::vec4 rayWorld = invView * rayEye;
    ray.direction = glm::normalize(glm::vec3(rayWorld.x, rayWorld.y, rayWorld.z));
    
    return ray;
}

bool rayIntersectsSphere(const Ray& ray, const glm::vec3& sphereCenter, float sphereRadius) {
    // Ray-sphere intersection using geometric method
    // Vector from ray origin to sphere center
    glm::vec3 oc = ray.origin - sphereCenter;
    
    // Quadratic equation coefficients: at^2 + bt + c = 0
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    
    // Discriminant
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0) {
        return false;  // No intersection
    }
    
    // Check if intersection is in front of the ray (t > 0)
    float sqrtDisc = sqrt(discriminant);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);
    
    // Return true if any intersection is in front of the camera
    return (t1 > 0.0f || t2 > 0.0f);
}

bool checkLampClick(float mouseX, float mouseY,
                    int screenWidth, int screenHeight,
                    const glm::mat4& view, const glm::mat4& projection,
                    const glm::vec3& cameraPos,
                    const glm::vec3& lampPosition, float lampRadius) {
    // Generate ray from mouse position
    Ray ray = screenToWorldRay(mouseX, mouseY, screenWidth, screenHeight,
                               view, projection, cameraPos);
    
    // Check intersection with lamp's bounding sphere
    return rayIntersectsSphere(ray, lampPosition, lampRadius);
}

