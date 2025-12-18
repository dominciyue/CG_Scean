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
    
    // Animation
    float rotationY;
    float bobOffset;
    float bobSpeed;
    
    // Light properties
    float lightIntensity;
    float lightRadius;
};

// Global orb state
extern SpiritOrb g_spiritOrb;
extern std::vector<GlowParticle> g_orbParticles;
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

// Cleanup
void cleanupSpiritOrb();

#endif // SPIRIT_ORB_H






