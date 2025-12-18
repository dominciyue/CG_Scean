#include "ray_picking.h"
#include "lamp_light.h"
#include "config.h"
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

bool rayIntersectsCylinder(const Ray& ray, 
                           const glm::vec3& cylinderBase,
                           float radius, float height) {
    // Ray-cylinder intersection for Y-axis aligned cylinder
    // Project ray onto XZ plane for infinite cylinder test
    
    // Ray origin relative to cylinder base
    glm::vec3 oc = ray.origin - cylinderBase;
    
    // Coefficients for quadratic equation (only X and Z components)
    float a = ray.direction.x * ray.direction.x + ray.direction.z * ray.direction.z;
    float b = 2.0f * (oc.x * ray.direction.x + oc.z * ray.direction.z);
    float c = oc.x * oc.x + oc.z * oc.z - radius * radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0) {
        return false;  // No intersection with infinite cylinder
    }
    
    float sqrtDisc = sqrt(discriminant);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);
    
    // Check first intersection point
    if (t1 > 0.0f) {
        float y1 = ray.origin.y + t1 * ray.direction.y;
        if (y1 >= cylinderBase.y && y1 <= cylinderBase.y + height) {
            return true;
        }
    }
    
    // Check second intersection point
    if (t2 > 0.0f) {
        float y2 = ray.origin.y + t2 * ray.direction.y;
        if (y2 >= cylinderBase.y && y2 <= cylinderBase.y + height) {
            return true;
        }
    }
    
    return false;
}

bool checkLampClick(float mouseX, float mouseY,
                    int screenWidth, int screenHeight,
                    const glm::mat4& view, const glm::mat4& projection,
                    const glm::vec3& cameraPos) {
    // Generate ray from mouse position
    Ray ray = screenToWorldRay(mouseX, mouseY, screenWidth, screenHeight,
                               view, projection, cameraPos);
    
    // Calculate lamp shade cylinder position (only the shade, not base)
    glm::vec3 shadeBase = LAMP_POSITION + glm::vec3(0.0f, LAMP_SHADE_HEIGHT_MIN, 0.0f);
    float shadeHeight = LAMP_SHADE_HEIGHT_MAX - LAMP_SHADE_HEIGHT_MIN;
    
    // Check intersection with lamp shade cylinder
    return rayIntersectsCylinder(ray, shadeBase, LAMP_SHADE_RADIUS, shadeHeight);
}

