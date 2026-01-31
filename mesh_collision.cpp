#include "mesh_collision.h"
#include <algorithm>
#include <limits>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

// =====================================================================
// Global Variables
// =====================================================================
std::vector<CollisionMesh> g_collisionMeshes;

// =====================================================================
// Constants
// =====================================================================
const float EPSILON = 1e-6f;

// =====================================================================
// Initialization
// =====================================================================
void initMeshCollision() {
    g_collisionMeshes.clear();
    g_collisionMeshes.reserve(20);  // Reserve space for expected meshes
}

void cleanupMeshCollision() {
    g_collisionMeshes.clear();
}

void clearCollisionMeshes() {
    g_collisionMeshes.clear();
}

// =====================================================================
// Mesh Registration
// =====================================================================

// Helper: Calculate AABB from vertices
static void calculateAABB(const std::vector<glm::vec3>& vertices,
                          glm::vec3& aabbMin, glm::vec3& aabbMax) {
    if (vertices.empty()) {
        aabbMin = glm::vec3(0.0f);
        aabbMax = glm::vec3(0.0f);
        return;
    }
    
    aabbMin = glm::vec3(std::numeric_limits<float>::max());
    aabbMax = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& v : vertices) {
        aabbMin = glm::min(aabbMin, v);
        aabbMax = glm::max(aabbMax, v);
    }
}

// Helper: Transform vertices to world space and update AABB
static void updateWorldSpaceData(CollisionMesh& mesh) {
    if (mesh.vertices.empty()) return;
    
    // Transform vertices to world space
    for (auto& v : mesh.vertices) {
        glm::vec4 worldPos = mesh.modelMatrix * glm::vec4(v, 1.0f);
        v = glm::vec3(worldPos);
    }
    
    // Recalculate AABB
    calculateAABB(mesh.vertices, mesh.aabbMin, mesh.aabbMax);
    mesh.isDirty = false;
}

void registerCollisionMesh(const std::string& name,
                          const std::vector<Vertex>& vertices,
                          const std::vector<unsigned int>& indices,
                          const glm::mat4& modelMatrix) {
    CollisionMesh mesh;
    mesh.name = name;
    mesh.modelMatrix = modelMatrix;
    mesh.isDirty = false;
    
    // Copy vertex positions (local space initially)
    mesh.vertices.reserve(vertices.size());
    for (const auto& v : vertices) {
        mesh.vertices.push_back(v.Position);
    }
    
    // Copy indices
    mesh.indices = indices;
    
    // Transform to world space and calculate AABB
    updateWorldSpaceData(mesh);
    
    g_collisionMeshes.push_back(std::move(mesh));
    
    std::cout << "[Collision] Registered mesh: " << name 
              << " (" << vertices.size() << " vertices, " 
              << indices.size() / 3 << " triangles)" << std::endl;
}

void registerCollisionMesh(const std::string& name,
                          const Mesh& mesh,
                          const glm::mat4& modelMatrix) {
    registerCollisionMesh(name, mesh.vertices, mesh.indices, modelMatrix);
}

void updateCollisionMeshTransform(const std::string& name, const glm::mat4& modelMatrix) {
    for (auto& mesh : g_collisionMeshes) {
        if (mesh.name == name) {
            mesh.modelMatrix = modelMatrix;
            mesh.isDirty = true;
            // Note: In a real implementation, you'd store local vertices separately
            // and recompute world vertices. For simplicity, we skip this here.
            break;
        }
    }
}

// =====================================================================
// Ray-Triangle Intersection (Möller–Trumbore Algorithm)
// =====================================================================
bool rayTriangleIntersect(const glm::vec3& rayOrigin,
                          const glm::vec3& rayDir,
                          const glm::vec3& v0,
                          const glm::vec3& v1,
                          const glm::vec3& v2,
                          float& t,
                          glm::vec3& hitPoint,
                          glm::vec3& normal) {
    // Edge vectors
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    
    // Calculate determinant
    glm::vec3 h = glm::cross(rayDir, edge2);
    float det = glm::dot(edge1, h);
    
    // If determinant is near zero, ray is parallel to triangle
    if (det > -EPSILON && det < EPSILON) {
        return false;
    }
    
    float invDet = 1.0f / det;
    
    // Calculate u parameter
    glm::vec3 s = rayOrigin - v0;
    float u = invDet * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    
    // Calculate v parameter
    glm::vec3 q = glm::cross(s, edge1);
    float v = invDet * glm::dot(rayDir, q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    
    // Calculate t (distance along ray)
    t = invDet * glm::dot(edge2, q);
    
    if (t > EPSILON) {
        // Hit!
        hitPoint = rayOrigin + rayDir * t;
        normal = glm::normalize(glm::cross(edge1, edge2));
        return true;
    }
    
    return false;
}

// =====================================================================
// Ray-AABB Intersection (Slab Method)
// =====================================================================
bool rayAABBIntersect(const glm::vec3& rayOrigin,
                      const glm::vec3& rayDir,
                      const glm::vec3& aabbMin,
                      const glm::vec3& aabbMax,
                      float& tNear,
                      float& tFar) {
    tNear = std::numeric_limits<float>::lowest();
    tFar = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; i++) {
        if (std::abs(rayDir[i]) < EPSILON) {
            // Ray is parallel to this axis
            if (rayOrigin[i] < aabbMin[i] || rayOrigin[i] > aabbMax[i]) {
                return false;
            }
        } else {
            float invD = 1.0f / rayDir[i];
            float t1 = (aabbMin[i] - rayOrigin[i]) * invD;
            float t2 = (aabbMax[i] - rayOrigin[i]) * invD;
            
            if (t1 > t2) std::swap(t1, t2);
            
            tNear = std::max(tNear, t1);
            tFar = std::min(tFar, t2);
            
            if (tNear > tFar || tFar < 0.0f) {
                return false;
            }
        }
    }
    
    return true;
}

// =====================================================================
// Ray-Mesh Intersection
// =====================================================================
CollisionResult rayMeshIntersect(const glm::vec3& rayOrigin,
                                 const glm::vec3& rayDir,
                                 const CollisionMesh& mesh) {
    CollisionResult result;
    result.hit = false;
    result.distance = std::numeric_limits<float>::max();
    result.meshName = mesh.name;
    
    // First, test against AABB for fast rejection
    float tNear, tFar;
    if (!rayAABBIntersect(rayOrigin, rayDir, mesh.aabbMin, mesh.aabbMax, tNear, tFar)) {
        return result;
    }
    
    // Test against all triangles
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        unsigned int i0 = mesh.indices[i];
        unsigned int i1 = mesh.indices[i + 1];
        unsigned int i2 = mesh.indices[i + 2];
        
        // Bounds check
        if (i0 >= mesh.vertices.size() || 
            i1 >= mesh.vertices.size() || 
            i2 >= mesh.vertices.size()) {
            continue;
        }
        
        const glm::vec3& v0 = mesh.vertices[i0];
        const glm::vec3& v1 = mesh.vertices[i1];
        const glm::vec3& v2 = mesh.vertices[i2];
        
        float t;
        glm::vec3 hitPoint, normal;
        
        if (rayTriangleIntersect(rayOrigin, rayDir, v0, v1, v2, t, hitPoint, normal)) {
            if (t > 0 && t < result.distance) {
                result.hit = true;
                result.point = hitPoint;
                result.normal = normal;
                result.distance = t;
            }
        }
    }
    
    return result;
}

// =====================================================================
// Test Against All Meshes
// =====================================================================
CollisionResult rayAllMeshesIntersect(const glm::vec3& rayOrigin,
                                      const glm::vec3& rayDir) {
    CollisionResult closest;
    closest.hit = false;
    closest.distance = std::numeric_limits<float>::max();
    
    for (const auto& mesh : g_collisionMeshes) {
        CollisionResult result = rayMeshIntersect(rayOrigin, rayDir, mesh);
        
        if (result.hit && result.distance < closest.distance) {
            closest = result;
        }
    }
    
    return closest;
}

// =====================================================================
// Point-Based Collision
// =====================================================================
bool pointInAnyMeshAABB(const glm::vec3& point) {
    for (const auto& mesh : g_collisionMeshes) {
        if (point.x >= mesh.aabbMin.x && point.x <= mesh.aabbMax.x &&
            point.y >= mesh.aabbMin.y && point.y <= mesh.aabbMax.y &&
            point.z >= mesh.aabbMin.z && point.z <= mesh.aabbMax.z) {
            return true;
        }
    }
    return false;
}

// =====================================================================
// Sweep Test (for moving objects)
// =====================================================================
CollisionResult sweepTestAllMeshes(const glm::vec3& startPos,
                                   const glm::vec3& endPos,
                                   float radius) {
    glm::vec3 movement = endPos - startPos;
    float moveLength = glm::length(movement);
    
    CollisionResult result;
    result.hit = false;
    result.distance = std::numeric_limits<float>::max();
    
    if (moveLength < EPSILON) {
        // No movement
        return result;
    }
    
    glm::vec3 rayDir = movement / moveLength;
    
    // Cast ray from start to end
    result = rayAllMeshesIntersect(startPos, rayDir);
    
    // Check if hit is within movement distance
    if (result.hit && result.distance <= moveLength + radius) {
        // Adjust hit point back by radius
        if (radius > 0.0f) {
            result.point -= result.normal * radius;
        }
        return result;
    }
    
    result.hit = false;
    return result;
}

