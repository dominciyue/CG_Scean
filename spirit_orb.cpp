#include "spirit_orb.h"
#include <cmath>
#include <random>
#include <glm/gtc/matrix_transform.hpp>

// Global variables
SpiritOrb g_spiritOrb;
std::vector<GlowParticle> g_orbParticles;
bool g_orbInitialized = false;

// Constants
const int PARTICLE_COUNT = 50;
const float PARTICLE_SPAWN_RADIUS = 0.06f;
const float PARTICLE_MAX_LIFE = 2.0f;

// Random number generator
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
static std::uniform_real_distribution<float> distAngle(0.0f, 6.28318f);

void initSpiritOrb() {
    // Initialize orb properties
    g_spiritOrb.position = glm::vec3(0.0f);
    g_spiritOrb.radius = 0.035f;
    g_spiritOrb.glowRadius = 0.08f;
    g_spiritOrb.pulsePhase = 0.0f;
    
    // Mystical blue-cyan color scheme
    g_spiritOrb.coreColor = glm::vec3(0.4f, 0.8f, 1.0f);
    g_spiritOrb.glowColor = glm::vec3(0.2f, 0.6f, 0.9f);
    g_spiritOrb.particleColor = glm::vec3(0.5f, 0.9f, 1.0f);
    
    g_spiritOrb.rotationY = 0.0f;
    g_spiritOrb.bobOffset = 0.0f;
    g_spiritOrb.bobSpeed = 2.0f;
    
    g_spiritOrb.lightIntensity = 2.0f;
    g_spiritOrb.lightRadius = 0.5f;
    
    // Initialize particles
    g_orbParticles.resize(PARTICLE_COUNT);
    for (auto& particle : g_orbParticles) {
        particle.active = false;
        particle.life = 0.0f;
    }
    
    g_orbInitialized = true;
}

void updateSpiritOrb(float deltaTime, const glm::vec3& position) {
    if (!g_orbInitialized) return;
    
    g_spiritOrb.position = position;
    
    // Update animation
    g_spiritOrb.pulsePhase += deltaTime * 3.0f;
    g_spiritOrb.rotationY += deltaTime * 45.0f;  // Slow rotation
    g_spiritOrb.bobOffset = sin(g_spiritOrb.pulsePhase * g_spiritOrb.bobSpeed) * 0.01f;
    
    // Update particles
    for (auto& particle : g_orbParticles) {
        if (particle.active) {
            // Update position
            particle.position += particle.velocity * deltaTime;
            particle.life -= deltaTime;
            
            // Fade out
            particle.alpha = particle.life / particle.maxLife;
            particle.size = 0.015f * (0.5f + 0.5f * particle.alpha);
            
            // Spiral motion
            float angle = atan2(particle.position.z - g_spiritOrb.position.z,
                               particle.position.x - g_spiritOrb.position.x);
            angle += deltaTime * 2.0f;
            float dist = glm::length(glm::vec2(particle.position.x - g_spiritOrb.position.x,
                                               particle.position.z - g_spiritOrb.position.z));
            particle.position.x = g_spiritOrb.position.x + cos(angle) * dist;
            particle.position.z = g_spiritOrb.position.z + sin(angle) * dist;
            
            if (particle.life <= 0.0f) {
                particle.active = false;
            }
        } else {
            // Spawn new particle
            if (dist01(gen) > 0.7f) {
                float theta = distAngle(gen);
                float phi = distAngle(gen) * 0.5f;
                float r = PARTICLE_SPAWN_RADIUS * dist01(gen);
                
                particle.position = g_spiritOrb.position + glm::vec3(
                    cos(theta) * sin(phi) * r,
                    cos(phi) * r * 0.5f,
                    sin(theta) * sin(phi) * r
                );
                
                // Velocity pointing outward and upward
                glm::vec3 dir = glm::normalize(particle.position - g_spiritOrb.position);
                particle.velocity = dir * 0.02f + glm::vec3(0.0f, 0.03f, 0.0f);
                
                particle.maxLife = PARTICLE_MAX_LIFE * (0.5f + 0.5f * dist01(gen));
                particle.life = particle.maxLife;
                particle.size = 0.015f;
                particle.alpha = 1.0f;
                particle.active = true;
            }
        }
    }
}

void generateOrbMesh(Mesh& mesh, int segments, int rings) {
    mesh.vertices.clear();
    mesh.indices.clear();
    
    float radius = 1.0f;  // Unit sphere, scale in shader
    
    // Generate sphere vertices
    for (int ring = 0; ring <= rings; ring++) {
        float phi = 3.14159f * ring / rings;
        float y = cos(phi);
        float ringRadius = sin(phi);
        
        for (int seg = 0; seg <= segments; seg++) {
            float theta = 2.0f * 3.14159f * seg / segments;
            float x = ringRadius * cos(theta);
            float z = ringRadius * sin(theta);
            
            Vertex v;
            v.Position = glm::vec3(x, y, z) * radius;
            v.Normal = glm::normalize(v.Position);
            v.TexCoords = glm::vec2((float)seg / segments, (float)ring / rings);
            
            mesh.vertices.push_back(v);
        }
    }
    
    // Generate indices
    for (int ring = 0; ring < rings; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;
            
            mesh.indices.push_back(current);
            mesh.indices.push_back(next);
            mesh.indices.push_back(current + 1);
            
            mesh.indices.push_back(current + 1);
            mesh.indices.push_back(next);
            mesh.indices.push_back(next + 1);
        }
    }
}

void generateGlowQuadMesh(Mesh& mesh) {
    mesh.vertices.clear();
    mesh.indices.clear();
    
    // Simple quad for billboard rendering
    Vertex v;
    v.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
    
    v.Position = glm::vec3(-0.5f, -0.5f, 0.0f); v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3( 0.5f, -0.5f, 0.0f); v.TexCoords = glm::vec2(1, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3( 0.5f,  0.5f, 0.0f); v.TexCoords = glm::vec2(1, 1); mesh.vertices.push_back(v);
    v.Position = glm::vec3(-0.5f,  0.5f, 0.0f); v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    
    mesh.indices = {0, 1, 2, 0, 2, 3};
}

glm::vec3 getAnimatedOrbPosition() {
    return g_spiritOrb.position + glm::vec3(0.0f, g_spiritOrb.bobOffset, 0.0f);
}

glm::mat4 getOrbModelMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, getAnimatedOrbPosition());
    model = glm::rotate(model, glm::radians(g_spiritOrb.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(g_spiritOrb.radius));
    return model;
}

float getOrbGlowIntensity() {
    // Pulsing glow effect
    return 0.8f + 0.2f * sin(g_spiritOrb.pulsePhase);
}

const std::vector<GlowParticle>& getOrbParticles() {
    return g_orbParticles;
}

void cleanupSpiritOrb() {
    g_orbParticles.clear();
    g_orbInitialized = false;
}






