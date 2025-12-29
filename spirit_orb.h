#pragma once
#ifndef SPIRIT_ORB_H
#define SPIRIT_ORB_H

#include <glm/glm.hpp>
#include <vector>
#include "types.h"
#include "mesh.h"

// Glow particle for orb light effect
struct GlowParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
    float maxLife;
    float size;
    float alpha;
    bool active;
};

// Light ray emanating from orb
struct LightRay {
    float angle;           // Base angle around Y axis (radians)
    float elevation;       // Elevation angle (radians)
    float length;          // Ray length
    float width;           // Ray width
    float intensity;       // Brightness (0-1)
    float pulseOffset;     // Phase offset for pulsing
};

// Projected light spot on surfaces
struct LightSpot {
    glm::vec3 position;    // Position on surface
    glm::vec3 normal;      // Surface normal (for orientation)
    float radius;          // Spot radius
    float intensity;       // Brightness (0-1)
    bool onFloor;          // True = floor, False = wall
};

// Spirit orb rendering parameters
struct SpiritOrb {
    glm::vec3 position;
    float radius;
    float glowRadius;
    float pulsePhase;
    
    // Colors
    glm::vec3 coreColor;
    glm::vec3 glowColor;
    glm::vec3 particleColor;
    glm::vec3 rayColor;     // Color for light rays
    glm::vec3 spotColor;    // Color for projected spots
    
    // Animation
    float rotationY;
    float spinSpeed;        // Rotation speed (degrees/sec)
    float bobOffset;
    float bobSpeed;
    
    // Light properties
    float lightIntensity;
    float lightRadius;
    
    // Ray properties
    int rayCount;
    float rayBaseLength;
    float rayBaseWidth;
};

// Global orb state
extern SpiritOrb g_spiritOrb;
extern std::vector<GlowParticle> g_orbParticles;
extern std::vector<LightRay> g_orbRays;
extern std::vector<LightSpot> g_orbSpots;
extern bool g_orbInitialized;

// Initialize spirit orb system
void initSpiritOrb();

// Update orb animation and particles
void updateSpiritOrb(float deltaTime, const glm::vec3& position);

// Generate orb sphere mesh
void generateOrbMesh(Mesh& mesh, int segments = 32, int rings = 16);

// Generate glow quad mesh (for billboard particles)
void generateGlowQuadMesh(Mesh& mesh);

// Get current orb position (with bob animation)
glm::vec3 getAnimatedOrbPosition();

// Get orb model matrix
glm::mat4 getOrbModelMatrix();

// Get orb glow intensity (pulsing)
float getOrbGlowIntensity();

// Get particle positions for rendering
const std::vector<GlowParticle>& getOrbParticles();

// Get light rays for rendering
const std::vector<LightRay>& getOrbRays();

// Get projected light spots for rendering
const std::vector<LightSpot>& getOrbSpots();

// Generate ray mesh (elongated quad)
void generateRayMesh(Mesh& mesh);

// Get model matrix for a specific ray
glm::mat4 getRayModelMatrix(const LightRay& ray);

// Get model matrix for a specific spot
glm::mat4 getSpotModelMatrix(const LightSpot& spot);

// Get current spin rotation in radians
float getOrbSpinRotation();

// Cleanup
void cleanupSpiritOrb();

#endif // SPIRIT_ORB_H






