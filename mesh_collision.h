#pragma once
#ifndef MESH_COLLISION_H
#define MESH_COLLISION_H

#include <glm/glm.hpp>
#include <vector>
#include "types.h"

// =====================================================================
// Collision Mesh Structure
// =====================================================================
// A simplified collision representation of a mesh with AABB for fast rejection

struct CollisionMesh {
    std::string name;                          // For debugging
    std::vector<glm::vec3> vertices;           // World-space vertices
    std::vector<unsigned int> indices;         // Triangle indices
    
    // AABB for fast rejection
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
    
    // Transform (to convert local to world space)
    glm::mat4 modelMatrix;
    bool isDirty;  // Need to recalculate world-space vertices
};

// =====================================================================
// Collision Result
// =====================================================================
struct CollisionResult {
    bool hit;
    glm::vec3 point;           // Hit point in world space
    glm::vec3 normal;          // Surface normal at hit point
    float distance;            // Distance from ray origin to hit point
    std::string meshName;      // Which mesh was hit
};

// =====================================================================
// Global Collision Mesh Registry
// =====================================================================
extern std::vector<CollisionMesh> g_collisionMeshes;

// =====================================================================
// Function Declarations
// =====================================================================

// Initialize collision system
void initMeshCollision();
void cleanupMeshCollision();

// Register a mesh for collision detection
// vertices and indices are in local space, modelMatrix transforms to world space
void registerCollisionMesh(const std::string& name,
                          const std::vector<Vertex>& vertices,
                          const std::vector<unsigned int>& indices,
                          const glm::mat4& modelMatrix);

// Register from existing Mesh struct
void registerCollisionMesh(const std::string& name,
                          const Mesh& mesh,
                          const glm::mat4& modelMatrix);

// Update mesh transform (call when object moves)
void updateCollisionMeshTransform(const std::string& name, const glm::mat4& modelMatrix);

// Clear all collision meshes
void clearCollisionMeshes();

// =====================================================================
// Ray-Mesh Intersection Tests
// =====================================================================

// Test ray against a single triangle (Möller–Trumbore algorithm)
bool rayTriangleIntersect(const glm::vec3& rayOrigin,
                          const glm::vec3& rayDir,
                          const glm::vec3& v0,
                          const glm::vec3& v1,
                          const glm::vec3& v2,
                          float& t,
                          glm::vec3& hitPoint,
                          glm::vec3& normal);

// Test ray against AABB (fast rejection)
bool rayAABBIntersect(const glm::vec3& rayOrigin,
                      const glm::vec3& rayDir,
                      const glm::vec3& aabbMin,
                      const glm::vec3& aabbMax,
                      float& tNear,
                      float& tFar);

// Test ray against a collision mesh
CollisionResult rayMeshIntersect(const glm::vec3& rayOrigin,
                                 const glm::vec3& rayDir,
                                 const CollisionMesh& mesh);

// Test ray against all registered collision meshes
// Returns the closest hit
CollisionResult rayAllMeshesIntersect(const glm::vec3& rayOrigin,
                                      const glm::vec3& rayDir);

// =====================================================================
// Point-Based Collision (for moving objects like arrows)
// =====================================================================

// Check if a point is inside any collision mesh AABB
bool pointInAnyMeshAABB(const glm::vec3& point);

// Sweep test: check collision along a movement vector
// Returns true if collision occurs, fills result with collision info
CollisionResult sweepTestAllMeshes(const glm::vec3& startPos,
                                   const glm::vec3& endPos,
                                   float radius = 0.0f);

#endif // MESH_COLLISION_H

