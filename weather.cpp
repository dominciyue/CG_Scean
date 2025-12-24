#include "weather.h"
#include <random>
#include <algorithm>
#include <cstdlib>

// =====================================================================
// Global Variable Definitions
// =====================================================================
bool cloudVisible = false;
bool isRaining = false;
bool isSnowing = false;
bool cloudControlMode = false;
glm::vec3 cloudPosition(DEFAULT_CLOUD_POS);

std::vector<Particle> rainParticles;
std::vector<Particle> snowParticles;
std::vector<Lightning> lightnings;

float lightningTimer = 0.0f;
float lightningInterval = DEFAULT_LIGHTNING_INTERVAL;

std::vector<float> snowHeightMap;

// Random number generator
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

// =====================================================================
// Weather System Initialization
// =====================================================================

void initWeatherSystem() {
    // Initialize rain particles
    rainParticles.resize(RAIN_PARTICLE_COUNT);
    for (auto& particle : rainParticles) {
        particle.active = false;
    }
    
    // Initialize snow particles
    snowParticles.resize(SNOW_PARTICLE_COUNT);
    for (auto& particle : snowParticles) {
        particle.active = false;
    }
    
    // Initialize lightning
    lightnings.resize(LIGHTNING_COUNT);
    for (auto& lightning : lightnings) {
        lightning.active = false;
    }
    
    // Initialize snow height map
    snowHeightMap.resize((TERRAIN_GRID_SIZE + 1) * (TERRAIN_GRID_SIZE + 1), 0.0f);
}

void cleanupWeatherSystem() {
    rainParticles.clear();
    snowParticles.clear();
    lightnings.clear();
    snowHeightMap.clear();
}

// =====================================================================
// Weather System Update Functions
// =====================================================================

void updateRainParticles(float deltaTime) {
    if (!isRaining || !cloudVisible) return;
    
    for (auto& particle : rainParticles) {
        if (particle.active) {
            // Update position
            particle.position += particle.velocity * deltaTime;
            particle.life -= deltaTime;

            // Collision detection with terrain (terrain at sandbox Y position)
            if (particle.position.y <= 0.6f || particle.life <= 0.0f) {
                particle.active = false;
            }
        } else {
            // Activate new particle (spawn from cloud position, scaled to sandbox size)
            if (dist(gen) > 0.3f) {
                particle.position = cloudPosition + glm::vec3(
                    dist(gen) * SANDBOX_HALF_WIDTH, 
                    -0.05f, 
                    dist(gen) * SANDBOX_HALF_DEPTH);
                particle.velocity = glm::vec3(0.0f, -5.0f, 0.0f);
                particle.life = 3.0f;
                particle.active = true;
            }
        }
    }
}

void updateSnowParticles(float deltaTime, std::vector<Vertex>& terrainVertices) {
    if (!isSnowing || !cloudVisible) return;
    
    for (auto& particle : snowParticles) {
        if (particle.active) {
            // Update position (snow falls with horizontal drift)
            particle.position += particle.velocity * deltaTime;
            particle.position.x += sin(particle.life * 2.0f) * 0.05f * deltaTime;  // Reduced drift for smaller sandbox
            particle.life += deltaTime;

            // Collision detection - use sandbox center from config
            float terrainCenterX = SANDBOX_CENTER_X;
            float terrainCenterZ = SANDBOX_CENTER_Z;
            
            // Particle position relative to terrain center
            float relativeX = particle.position.x - terrainCenterX;
            float relativeZ = particle.position.z - terrainCenterZ;
            
            // Convert to grid coordinates [0, TERRAIN_GRID_SIZE]
            float gridX = (relativeX / TERRAIN_SCALE_X + 0.5f) * TERRAIN_GRID_SIZE;
            float gridZ = (relativeZ / TERRAIN_SCALE_Z + 0.5f) * TERRAIN_GRID_SIZE;
            
            // Check if within terrain bounds
            if (gridX >= 0 && gridX <= TERRAIN_GRID_SIZE && 
                gridZ >= 0 && gridZ <= TERRAIN_GRID_SIZE) {
                
                int ix = (int)gridX;
                int iz = (int)gridZ;
                
                if (ix >= 0 && ix < TERRAIN_GRID_SIZE && iz >= 0 && iz < TERRAIN_GRID_SIZE) {
                    int vertexIndex = iz * (TERRAIN_GRID_SIZE + 1) + ix;
                    float currentSnowHeight = snowHeightMap[vertexIndex];
                    
                    // Simple collision detection
                    if (particle.position.y <= 0.6f + currentSnowHeight) {
                        particle.active = false;
                        // Accumulate snow at this vertex
                        snowHeightMap[vertexIndex] += SNOW_ACCUMULATION_RATE;
                        if (snowHeightMap[vertexIndex] > MAX_SNOW_HEIGHT) {
                            snowHeightMap[vertexIndex] = MAX_SNOW_HEIGHT;
                        }
                    }
                }
            } else {
                // Outside terrain bounds
                if (particle.position.y <= 0.6f) {
                    particle.active = false;
                }
            }
        } else {
            // Activate new particle (spawn from cloud position, scaled to sandbox size)
            if (dist(gen) > 0.4f) {
                particle.position = cloudPosition + glm::vec3(
                    dist(gen) * SANDBOX_HALF_WIDTH, 
                    -0.05f, 
                    dist(gen) * SANDBOX_HALF_DEPTH);
                particle.velocity = glm::vec3(0.0f, -1.5f, 0.0f); // Snow falls slower than rain
                particle.life = 0.0f;
                particle.active = true;
            }
        }
    }
}

void updateLightning(float deltaTime) {
    // Only generate new lightning when raining
    if (isRaining && cloudVisible) {
        lightningTimer += deltaTime;
        
        // Generate new lightning at intervals
        if (lightningTimer >= lightningInterval) {
            lightningTimer = 0.0f;
            
            // Random position for lightning strike (scaled to sandbox size)
            std::uniform_real_distribution<float> lightningDist(-0.1f, 0.1f);
            glm::vec3 strikePos = cloudPosition + glm::vec3(
                lightningDist(gen) * SANDBOX_HALF_WIDTH, 
                0.0f, 
                lightningDist(gen) * SANDBOX_HALF_DEPTH);
            
            int lightningCount = 0;
            int maxLightnings = 5 + (rand() % 3); // Generate 5-7 lightning bolts at once
            
            for (auto& lightning : lightnings) {
                if (!lightning.active && lightningCount < maxLightnings) {
                    lightning.active = true;
                    
                    // Main lightning and branches have different starting points
                    if (lightningCount == 0) {
                        lightning.startPos = strikePos;
                    } else {
                        float branchOffset = 0.06f * lightningCount;
                        lightning.startPos = strikePos + glm::vec3(
                            lightningDist(gen) * branchOffset,
                            -0.04f * lightningCount,
                            lightningDist(gen) * branchOffset
                        );
                    }
                    
                    // End point offset
                    lightning.endPos = glm::vec3(
                        lightning.startPos.x + lightningDist(gen) * 0.08f,
                        0.6f,
                        lightning.startPos.z + lightningDist(gen) * 0.08f
                    );
                    
                    lightning.life = 0.25f;
                    lightning.maxLife = 0.25f;
                    
                    // Generate zigzag segments
                    lightning.segments.clear();
                    lightning.segments.push_back(lightning.startPos);
                    
                    int segmentCount = 12 + (rand() % 6);
                    float segmentHeight = (lightning.startPos.y - lightning.endPos.y) / segmentCount;
                    
                    glm::vec3 currentPos = lightning.startPos;
                    for (int i = 1; i < segmentCount; i++) {
                        currentPos.y -= segmentHeight;
                        currentPos.x += lightningDist(gen) * 0.06f;
                        currentPos.z += lightningDist(gen) * 0.06f;
                        lightning.segments.push_back(currentPos);
                    }
                    
                    lightning.segments.push_back(lightning.endPos);
                    lightningCount++;
                }
                
                if (lightningCount >= maxLightnings) break;
            }
        }
    } else {
        lightningTimer = 0.0f;
    }
    
    // Update all active lightning
    for (auto& lightning : lightnings) {
        if (lightning.active) {
            lightning.life -= deltaTime;
            if (lightning.life <= 0.0f) {
                lightning.active = false;
            }
        }
    }
}

void updateWeather(float deltaTime, std::vector<Vertex>& terrainVertices) {
    updateRainParticles(deltaTime);
    updateSnowParticles(deltaTime, terrainVertices);
    updateLightning(deltaTime);
}

// =====================================================================
// Weather State Control
// =====================================================================

void toggleCloud() {
    cloudVisible = !cloudVisible;
    cloudControlMode = !cloudControlMode;
    
    // When cloud disappears, stop all weather effects and clear particles
    if (!cloudVisible) {
        isRaining = false;
        isSnowing = false;
        for (auto& particle : rainParticles) {
            particle.active = false;
        }
        for (auto& particle : snowParticles) {
            particle.active = false;
        }
    }
    
}

void toggleRain() {
    if (cloudVisible) {
        isRaining = !isRaining;
    }
}

void toggleSnow() {
    if (cloudVisible) {
        isSnowing = !isSnowing;
        if (!isSnowing) {
            // Clear snow height map when snow stops
            std::fill(snowHeightMap.begin(), snowHeightMap.end(), 0.0f);
        }
    }
}

void moveCloud(const glm::vec3& direction, float deltaTime) {
    if (!cloudControlMode) return;
    
    glm::vec3 newCloudPos = cloudPosition + direction * CLOUD_MOVE_SPEED * deltaTime;
    
    // Limit cloud to sandbox bounds
    newCloudPos.x = glm::clamp(newCloudPos.x, 
                                SANDBOX_CENTER_X - SANDBOX_HALF_WIDTH, 
                                SANDBOX_CENTER_X + SANDBOX_HALF_WIDTH);
    newCloudPos.z = glm::clamp(newCloudPos.z, 
                                SANDBOX_CENTER_Z - SANDBOX_HALF_DEPTH, 
                                SANDBOX_CENTER_Z + SANDBOX_HALF_DEPTH);
    
    cloudPosition = newCloudPos;
}
