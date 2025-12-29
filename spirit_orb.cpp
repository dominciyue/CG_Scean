#include "spirit_orb.h"
#include "config.h"
#include <cmath>
#include <random>
#include <glm/gtc/matrix_transform.hpp>

// Global variables
SpiritOrb g_spiritOrb;
std::vector<GlowParticle> g_orbParticles;
std::vector<LightRay> g_orbRays;
std::vector<LightSpot> g_orbSpots;
bool g_orbInitialized = false;

// Constants
const int PARTICLE_COUNT = 50;
const float PARTICLE_SPAWN_RADIUS = 0.06f;
const float PARTICLE_MAX_LIFE = 2.0f;
const int RAY_COUNT = 8;          // Number of light rays
const int SPOT_COUNT = 8;         // Number of projected spots
const float RAY_LENGTH = 0.15f;   // Base ray length
const float RAY_WIDTH = 0.01f;    // Base ray width
const float SPIN_SPEED = 60.0f;   // Degrees per second

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
    g_spiritOrb.rayColor = glm::vec3(0.6f, 0.9f, 1.0f);   // Bright cyan rays
    g_spiritOrb.spotColor = glm::vec3(0.3f, 0.7f, 0.9f);  // Softer spot color
    
    g_spiritOrb.rotationY = 0.0f;
    g_spiritOrb.spinSpeed = SPIN_SPEED;
    g_spiritOrb.bobOffset = 0.0f;
    g_spiritOrb.bobSpeed = 2.0f;
    
    g_spiritOrb.lightIntensity = 2.0f;
    g_spiritOrb.lightRadius = 0.5f;
    
    // Ray properties
    g_spiritOrb.rayCount = RAY_COUNT;
    g_spiritOrb.rayBaseLength = RAY_LENGTH;
    g_spiritOrb.rayBaseWidth = RAY_WIDTH;
    
    // Initialize particles
    g_orbParticles.resize(PARTICLE_COUNT);
    for (auto& particle : g_orbParticles) {
        particle.active = false;
        particle.life = 0.0f;
    }
    
    // Initialize light rays (evenly distributed around orb)
    g_orbRays.resize(RAY_COUNT);
    for (int i = 0; i < RAY_COUNT; i++) {
        LightRay& ray = g_orbRays[i];
        ray.angle = (2.0f * 3.14159f * i) / RAY_COUNT;  // Evenly spaced
        ray.elevation = 0.0f;  // Horizontal rays
        ray.length = RAY_LENGTH * (0.8f + 0.4f * dist01(gen));
        ray.width = RAY_WIDTH;
        ray.intensity = 1.0f;
        ray.pulseOffset = (float)i / RAY_COUNT * 2.0f * 3.14159f;
    }
    
    // Initialize projected spots (will be updated each frame)
    g_orbSpots.resize(SPOT_COUNT);
    for (int i = 0; i < SPOT_COUNT; i++) {
        LightSpot& spot = g_orbSpots[i];
        spot.position = glm::vec3(0.0f);
        spot.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        spot.radius = 0.03f;
        spot.intensity = 0.0f;
        spot.onFloor = true;
    }
    
    g_orbInitialized = true;
}

void updateSpiritOrb(float deltaTime, const glm::vec3& position) {
    if (!g_orbInitialized) return;
    
    g_spiritOrb.position = position;
    
    // Update animation
    g_spiritOrb.pulsePhase += deltaTime * 3.0f;
    g_spiritOrb.rotationY += deltaTime * g_spiritOrb.spinSpeed;  // Continuous spin
    if (g_spiritOrb.rotationY > 360.0f) g_spiritOrb.rotationY -= 360.0f;
    g_spiritOrb.bobOffset = sin(g_spiritOrb.pulsePhase * g_spiritOrb.bobSpeed) * 0.01f;
    
    // Update light rays (pulsing intensity)
    for (int i = 0; i < (int)g_orbRays.size(); i++) {
        LightRay& ray = g_orbRays[i];
        // Pulsing effect with phase offset
        ray.intensity = 0.6f + 0.4f * sin(g_spiritOrb.pulsePhase * 2.0f + ray.pulseOffset);
        // Slight length variation
        ray.length = g_spiritOrb.rayBaseLength * (0.8f + 0.3f * sin(g_spiritOrb.pulsePhase * 1.5f + ray.pulseOffset));
    }
    
    // Update projected light spots
    glm::vec3 orbPos = getAnimatedOrbPosition();
    float rotRad = glm::radians(g_spiritOrb.rotationY);
    
    // Room boundaries for spot projection
    float floorY = DEFAULT_CUBE_POS.y + FLOOR_HEIGHT;
    float leftWallX = DEFAULT_CUBE_POS.x - 0.5f * ROOM_SCALE_X;
    float rightWallX = DEFAULT_CUBE_POS.x + 0.5f * ROOM_SCALE_X;
    float backWallZ = DEFAULT_CUBE_POS.z - 0.5f * ROOM_SCALE_Z;
    float frontWallZ = DEFAULT_CUBE_POS.z + 0.5f * ROOM_SCALE_Z;
    
    for (int i = 0; i < (int)g_orbSpots.size(); i++) {
        LightSpot& spot = g_orbSpots[i];
        float spotAngle = rotRad + (2.0f * 3.14159f * i) / SPOT_COUNT;
        
        // Ray direction from orb
        glm::vec3 rayDir = glm::vec3(cos(spotAngle), -0.5f, sin(spotAngle));
        rayDir = glm::normalize(rayDir);
        
        // Calculate intersection with floor
        float t = (floorY - orbPos.y) / rayDir.y;
        if (t > 0 && rayDir.y < 0) {
            glm::vec3 hitPoint = orbPos + rayDir * t;
            
            // Check if within room bounds
            if (hitPoint.x >= leftWallX && hitPoint.x <= rightWallX &&
                hitPoint.z >= backWallZ && hitPoint.z <= frontWallZ) {
                spot.position = hitPoint;
                spot.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                spot.onFloor = true;
                spot.intensity = 0.7f + 0.3f * sin(g_spiritOrb.pulsePhase * 2.0f + spotAngle);
                spot.radius = 0.025f + 0.01f * (t / 0.5f);  // Larger spots further away
            }
        }
        // Also check wall intersections for some spots
        else if (i % 2 == 0) {
            // Try hitting a wall
            float wallT = 1000.0f;
            glm::vec3 wallNormal = glm::vec3(0.0f);
            
            // Right wall
            if (rayDir.x > 0.01f) {
                float tw = (rightWallX - orbPos.x) / rayDir.x;
                if (tw > 0 && tw < wallT) {
                    wallT = tw;
                    wallNormal = glm::vec3(-1.0f, 0.0f, 0.0f);
                }
            }
            // Left wall
            if (rayDir.x < -0.01f) {
                float tw = (leftWallX - orbPos.x) / rayDir.x;
                if (tw > 0 && tw < wallT) {
                    wallT = tw;
                    wallNormal = glm::vec3(1.0f, 0.0f, 0.0f);
                }
            }
            
            if (wallT < 100.0f) {
                spot.position = orbPos + rayDir * wallT;
                spot.normal = wallNormal;
                spot.onFloor = false;
                spot.intensity = 0.5f + 0.3f * sin(g_spiritOrb.pulsePhase * 2.0f + spotAngle);
                spot.radius = 0.02f + 0.015f * (wallT / 0.5f);
            }
        }
    }
    
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

const std::vector<LightRay>& getOrbRays() {
    return g_orbRays;
}

const std::vector<LightSpot>& getOrbSpots() {
    return g_orbSpots;
}

void generateRayMesh(Mesh& mesh) {
    mesh.vertices.clear();
    mesh.indices.clear();
    
    // Elongated quad for ray (from center outward)
    // Ray extends from 0 to 1 in X direction, centered in Y/Z
    Vertex v;
    v.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
    
    // Base of ray (near orb)
    v.Position = glm::vec3(0.0f, -0.5f, 0.0f); v.TexCoords = glm::vec2(0, 0); mesh.vertices.push_back(v);
    v.Position = glm::vec3(0.0f,  0.5f, 0.0f); v.TexCoords = glm::vec2(0, 1); mesh.vertices.push_back(v);
    // Tip of ray (far from orb, tapered)
    v.Position = glm::vec3(1.0f,  0.0f, 0.0f); v.TexCoords = glm::vec2(1, 0.5f); mesh.vertices.push_back(v);
    
    mesh.indices = {0, 1, 2};
}

glm::mat4 getRayModelMatrix(const LightRay& ray) {
    glm::vec3 orbPos = getAnimatedOrbPosition();
    float rotRad = glm::radians(g_spiritOrb.rotationY);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, orbPos);
    
    // Rotate with orb spin + ray's base angle
    model = glm::rotate(model, rotRad + ray.angle, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Apply elevation
    model = glm::rotate(model, ray.elevation, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Offset from center of orb
    model = glm::translate(model, glm::vec3(g_spiritOrb.radius * 0.8f, 0.0f, 0.0f));
    
    // Scale ray
    model = glm::scale(model, glm::vec3(ray.length, ray.width, ray.width));
    
    return model;
}

glm::mat4 getSpotModelMatrix(const LightSpot& spot) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, spot.position);
    
    // Orient to surface normal
    if (!spot.onFloor) {
        // Wall spot - rotate to face outward from wall
        if (spot.normal.x > 0.5f) {
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        } else if (spot.normal.x < -0.5f) {
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }
    } else {
        // Floor spot - slightly above floor to prevent z-fighting
        model = glm::translate(model, glm::vec3(0.0f, 0.002f, 0.0f));
    }
    
    model = glm::scale(model, glm::vec3(spot.radius));
    
    return model;
}

float getOrbSpinRotation() {
    return glm::radians(g_spiritOrb.rotationY);
}

void cleanupSpiritOrb() {
    g_orbParticles.clear();
    g_orbRays.clear();
    g_orbSpots.clear();
    g_orbInitialized = false;
}






