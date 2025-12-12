// =====================================================================
// SimpleScene - 中式场景渲染项目
// 重构版本：模块化代码结构
// =====================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 核心模块
#include "shader.h"
#include "camera.h"

// stb_image 实现（只在这里定义一次）
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Custom modules
#include "config.h"
#include "types.h"
#include "geometry_data.h"
#include "texture_loader.h"
#include "model_loader.h"
#include "mesh.h"
#include "terrain.h"
#include "weather.h"
#include "input.h"
#include "lamp_light.h"
#include "ray_picking.h"

#include <iostream>
#include <vector>
#include <cmath>

// =====================================================================
// Chinese Floor Tile Texture Generation
// =====================================================================
unsigned int generateChineseFloorTexture() {
    const int texSize = 512;
    const int tileSize = 128;  // Each tile is 128x128 pixels
    std::vector<unsigned char> textureData(texSize * texSize * 3);
    
    // Chinese floor tile colors
    glm::vec3 baseColor(180, 160, 140);      // Warm beige/tan base
    glm::vec3 groutColor(120, 110, 100);     // Darker grout lines
    glm::vec3 patternColor(160, 140, 120);   // Slightly darker pattern
    glm::vec3 accentColor(140, 100, 80);     // Reddish-brown accent
    
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            int idx = (y * texSize + x) * 3;
            
            // Local position within tile
            int tileX = x % tileSize;
            int tileY = y % tileSize;
            int tileIdxX = x / tileSize;
            int tileIdxY = y / tileSize;
            
            // Grout lines (3 pixels wide)
            bool isGrout = (tileX < 3 || tileX > tileSize - 4 || 
                           tileY < 3 || tileY > tileSize - 4);
            
            glm::vec3 color;
            
            if (isGrout) {
                color = groutColor;
            } else {
                // Base tile color with subtle variation
                float noise = ((float)((x * 7 + y * 13) % 20) / 20.0f - 0.5f) * 15.0f;
                color = baseColor + glm::vec3(noise, noise * 0.8f, noise * 0.6f);
                
                // Chinese geometric pattern - alternating tiles have different patterns
                bool altTile = ((tileIdxX + tileIdxY) % 2 == 0);
                
                // Inner border pattern (Chinese style frame)
                int innerMargin = 15;
                bool isInnerBorder = (tileX >= innerMargin && tileX <= innerMargin + 4) ||
                                    (tileX >= tileSize - innerMargin - 4 && tileX <= tileSize - innerMargin) ||
                                    (tileY >= innerMargin && tileY <= innerMargin + 4) ||
                                    (tileY >= tileSize - innerMargin - 4 && tileY <= tileSize - innerMargin);
                
                if (isInnerBorder && !isGrout) {
                    color = accentColor;
                }
                
                // Center pattern - simple Chinese motif
                int centerX = tileSize / 2;
                int centerY = tileSize / 2;
                int distX = abs(tileX - centerX);
                int distY = abs(tileY - centerY);
                
                if (altTile) {
                    // Diamond pattern
                    if (distX + distY < 25 && distX + distY > 20) {
                        color = patternColor;
                    }
                    // Inner diamond
                    if (distX + distY < 12) {
                        color = accentColor * 1.1f;
                    }
                } else {
                    // Square pattern
                    if ((distX < 25 && distX > 20 && distY < 25) ||
                        (distY < 25 && distY > 20 && distX < 25)) {
                        color = patternColor;
                    }
                    // Center square
                    if (distX < 10 && distY < 10) {
                        color = accentColor * 1.1f;
                    }
                }
                
                // Corner decorations
                int cornerDist = 30;
                bool inCorner = (tileX < cornerDist && tileY < cornerDist) ||
                               (tileX < cornerDist && tileY > tileSize - cornerDist) ||
                               (tileX > tileSize - cornerDist && tileY < cornerDist) ||
                               (tileX > tileSize - cornerDist && tileY > tileSize - cornerDist);
                
                if (inCorner && !isInnerBorder && !isGrout) {
                    int cx = (tileX < tileSize/2) ? 20 : tileSize - 20;
                    int cy = (tileY < tileSize/2) ? 20 : tileSize - 20;
                    int cd = abs(tileX - cx) + abs(tileY - cy);
                    if (cd < 8) {
                        color = accentColor;
                    }
                }
            }
            
            // Clamp colors
            color = glm::clamp(color, glm::vec3(0), glm::vec3(255));
            
            textureData[idx] = (unsigned char)color.r;
            textureData[idx + 1] = (unsigned char)color.g;
            textureData[idx + 2] = (unsigned char)color.b;
        }
    }
    
    // Create OpenGL texture
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return textureID;
}

// =====================================================================
// Global Variables
// =====================================================================

// 相机
Camera camera(DEFAULT_CAMERA_POS);

// 光源位置
glm::vec3 lightPos = DEFAULT_LIGHT_POS;
glm::vec3 cubePos = DEFAULT_CUBE_POS;
    
// =====================================================================
// 主函数
// =====================================================================
int main()
{
    // 初始化 GLFW
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SimpleScene - Chinese Style Room", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Setup callback functions
    setCamera(&camera);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);  // Mouse click callback

    // Capture mouse (disabled for click interaction, use normal mode)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // 初始化 GLAD
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 配置全局 OpenGL 状态
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // =====================================================================
    // 着色器加载
    // =====================================================================
    Shader lightingShader("lighting.vs", "lighting.fs");
    Shader lightCubeShader("lightcube.vs", "lightcube.fs");
    Shader terrainShader("terrain.vs", "terrain.fs");
    Shader lampShader("cyl.vs", "cyl.fs");  // 台灯着色器（圆柱投影）

    // =====================================================================
    // 模型加载
    // =====================================================================
    
    // 桌子模型
    Mesh tableMesh;
    bool tableLoaded = false;
    if (loadOBJ("obj/table3.obj", tableMesh.vertices, tableMesh.indices)) {
        setupMesh(tableMesh);
        tableLoaded = true;
        
        // 加载桌子纹理
        Texture woodTexture;
        woodTexture.id = loadTexture("obj/wood.jpg");
        woodTexture.type = "texture_diffuse";
        woodTexture.path = "obj/wood.jpg";
        tableMesh.textures.push_back(woodTexture);
    }

    // 台灯模型（带材质）
    std::vector<Mesh> lampMeshes;
    bool lampLoaded = false;
    if (loadOBJWithMaterials("obj/lamp1.obj", lampMeshes, "obj")) {
        lampLoaded = true;
    }
    
    // 计算台灯的Y坐标范围（用于圆柱投影着色器）
    float lampYMin = 1e9f, lampYMax = -1e9f;
    if (lampLoaded) {
        for (const auto& mesh : lampMeshes) {
            for (const auto& vertex : mesh.vertices) {
                if (vertex.Position.y < lampYMin) lampYMin = vertex.Position.y;
                if (vertex.Position.y > lampYMax) lampYMax = vertex.Position.y;
            }
        }
    }
    
    // 窗户纹理
    unsigned int windowTexture = loadTexture("window.png");

    // 生成中式地砖纹理
    unsigned int floorTexture = generateChineseFloorTexture();

    // =====================================================================
    // VAO/VBO Setup - Room Geometry
    // =====================================================================
    
    // Extract individual faces from cube vertex data
    float CeilingVertices[36], LWallVertices[36], RWallVertices[36], FWallVertices[36];
    std::copy(CUBE_VERTICES + 180, CUBE_VERTICES + 216, CeilingVertices);  // Top face
    std::copy(CUBE_VERTICES + 72, CUBE_VERTICES + 108, LWallVertices);     // Left face
    std::copy(CUBE_VERTICES + 108, CUBE_VERTICES + 144, RWallVertices);    // Right face
    std::copy(CUBE_VERTICES + 0, CUBE_VERTICES + 36, FWallVertices);       // Back face

    // 天花板
    unsigned int VBO1, CeilingVAO;
        glGenVertexArrays(1, &CeilingVAO);
        glGenBuffers(1, &VBO1);
        glBindBuffer(GL_ARRAY_BUFFER, VBO1);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CeilingVertices), CeilingVertices, GL_STATIC_DRAW);
        glBindVertexArray(CeilingVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // 地板 (with texture coordinates for Chinese tile pattern)
    unsigned int VBO2, FloorVAO;
        glGenVertexArrays(1, &FloorVAO);
        glGenBuffers(1, &VBO2);
        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FLOOR_VERTICES_TEXTURED), FLOOR_VERTICES_TEXTURED, GL_STATIC_DRAW);
        glBindVertexArray(FloorVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

    // 左墙
    unsigned int VBO3, LWallVAO;
        glGenVertexArrays(1, &LWallVAO);
        glGenBuffers(1, &VBO3);
        glBindBuffer(GL_ARRAY_BUFFER, VBO3);
        glBufferData(GL_ARRAY_BUFFER, sizeof(LWallVertices), LWallVertices, GL_STATIC_DRAW);
        glBindVertexArray(LWallVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // 右墙
    unsigned int VBO4, RWallVAO;
        glGenVertexArrays(1, &RWallVAO);
        glGenBuffers(1, &VBO4);
        glBindBuffer(GL_ARRAY_BUFFER, VBO4);
        glBufferData(GL_ARRAY_BUFFER, sizeof(RWallVertices), RWallVertices, GL_STATIC_DRAW);
        glBindVertexArray(RWallVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // 前墙
    unsigned int VBO5, FWallVAO;
        glGenVertexArrays(1, &FWallVAO);
        glGenBuffers(1, &VBO5);
        glBindBuffer(GL_ARRAY_BUFFER, VBO5);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FWallVertices), FWallVertices, GL_STATIC_DRAW);
        glBindVertexArray(FWallVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // 光源立方体
    unsigned int VBO6, lightCubeVAO;
        glGenVertexArrays(1, &lightCubeVAO);
        glGenBuffers(1, &VBO6);
        glBindBuffer(GL_ARRAY_BUFFER, VBO6);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);
        glBindVertexArray(lightCubeVAO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // 书桌
    unsigned int VBO7, DeskVAO;
        glGenVertexArrays(1, &DeskVAO);
        glGenBuffers(1, &VBO7);
        glBindBuffer(GL_ARRAY_BUFFER, VBO7);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DESK_VERTICES), DESK_VERTICES, GL_STATIC_DRAW);
        glBindVertexArray(DeskVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // 窗户
    unsigned int VBO8, WindowVAO, WindowEBO;
        glGenVertexArrays(1, &WindowVAO);
        glGenBuffers(1, &VBO8);
        glGenBuffers(1, &WindowEBO);
        glBindVertexArray(WindowVAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO8);
    glBufferData(GL_ARRAY_BUFFER, sizeof(WINDOW_VERTICES), WINDOW_VERTICES, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WindowEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(WINDOW_INDICES), WINDOW_INDICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

    // 地台
    unsigned int VBO9, PlatformVAO;
        glGenVertexArrays(1, &PlatformVAO);
        glGenBuffers(1, &VBO9);
        glBindBuffer(GL_ARRAY_BUFFER, VBO9);
    glBufferData(GL_ARRAY_BUFFER, sizeof(PLATFORM_VERTICES), PLATFORM_VERTICES, GL_STATIC_DRAW);
        glBindVertexArray(PlatformVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // =====================================================================
    // 地形和积雪网格
    // =====================================================================
    Mesh terrainMesh;
    generateTerrain(terrainMesh.vertices, terrainMesh.indices, 
                    TERRAIN_GRID_SIZE, TERRAIN_GRID_SIZE, 
                    TERRAIN_SCALE_X, TERRAIN_SCALE_Z, TERRAIN_HEIGHT_SCALE);
    setupMesh(terrainMesh);
    
    Mesh snowMesh;
    snowMesh.vertices = terrainMesh.vertices;
    snowMesh.indices = terrainMesh.indices;
    setupMesh(snowMesh);

    // =====================================================================
    // Initialize Weather System
    // =====================================================================
    initWeatherSystem();

    // =====================================================================
    // Initialize Lamp Light System
    // =====================================================================
    initLampLight();

    // Cloud sphere positions
    std::vector<glm::vec3> cloudSpheres;
    for (int i = 0; i < CLOUD_SPHERE_COUNT; i++) {
        cloudSpheres.push_back(glm::vec3(CLOUD_SPHERE_OFFSETS[i][0], 
                                          CLOUD_SPHERE_OFFSETS[i][1], 
                                          CLOUD_SPHERE_OFFSETS[i][2]));
    }

    // =====================================================================
    // 渲染循环
    // =====================================================================
    while (!glfwWindowShouldClose(window))
    {
        // 时间更新
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 输入处理
        processInput(window, camera);

        // 更新天气系统
        updateWeather(deltaTime, terrainMesh.vertices);

        // 清屏
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_TRUE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Prepare transformation matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                                 (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                                 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // Store matrices for ray picking (mouse click detection)
        setViewProjectionMatrices(view, projection);

        // Get lamp light position
        glm::vec3 lampLightPos = getLampLightPosition();

        // =====================================================================
        // Render Room (with room scale applied)
        // =====================================================================
        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);
        lightingShader.setFloat("alpha", 1.0f);
        
        // Set lamp light uniforms
        lightingShader.setBool("lampOn", isLampOn());
        lightingShader.setVec3("lampLightPos", lampLightPos);
        lightingShader.setVec3("lampLightColor", lampLightColor);
        lightingShader.setFloat("lampLightIntensity", lampLightIntensity);
        lightingShader.setFloat("lampLightRadius", lampLightRadius);

        // Room base transform with scale
        glm::mat4 roomModel = glm::translate(glm::mat4(1.0f), cubePos);
        roomModel = glm::scale(roomModel, glm::vec3(ROOM_SCALE_X, ROOM_SCALE_Y, ROOM_SCALE_Z));

        // Ceiling
        lightingShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(CeilingVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        // Floor (with Chinese tile texture)
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // White to show texture colors
        if (floorTexture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorTexture);
            lightingShader.setInt("texture1", 0);
            lightingShader.setBool("hasTexture", true);
        }
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(FloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        lightingShader.setBool("hasTexture", false);

        // Left wall
        lightingShader.setVec3("objectColor", 0.6f, 0.3f, 0.2f);
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(LWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        // Right wall
        lightingShader.setVec3("objectColor", 0.7f, 0.4f, 0.3f);
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(RWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        // Front wall
        lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.8f);
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(FWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        // =====================================================================
        // Render Window (scaled with room)
        // =====================================================================
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("alpha", 0.7f);
        model = glm::translate(glm::mat4(1.0f), cubePos);
        model = glm::scale(model, glm::vec3(ROOM_SCALE_X, ROOM_SCALE_Y, ROOM_SCALE_Z));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.499f));
        model = glm::scale(model, glm::vec3(0.6f));
            lightingShader.setMat4("model", model);
            if (windowTexture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, windowTexture);
                lightingShader.setInt("texture1", 0);
                lightingShader.setBool("hasTexture", true);
            }
            glBindVertexArray(WindowVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        lightingShader.setFloat("alpha", 1.0f);

        // =====================================================================
        // 渲染桌子
        // =====================================================================
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("alpha", 1.0f);
            model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.3f, 0.47f));
        model = glm::rotate(model, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.08f));
            lightingShader.setMat4("model", model);

        if (tableLoaded && !tableMesh.vertices.empty()) {
                if (!tableMesh.textures.empty() && tableMesh.textures[0].id != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, tableMesh.textures[0].id);
                    lightingShader.setInt("texture1", 0);
                    lightingShader.setBool("hasTexture", true);
                } else {
                    lightingShader.setBool("hasTexture", false);
                }
                glBindVertexArray(tableMesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tableMesh.indices.size()), GL_UNSIGNED_INT, 0);
        } else {
            lightingShader.setVec3("objectColor", 0.3f, 0.15f, 0.05f);
            lightingShader.setBool("hasTexture", false);
            glBindVertexArray(DeskVAO);
            glDrawArrays(GL_TRIANGLES, 0, 120);
        }

        // =====================================================================
        // 渲染台灯（使用圆柱投影着色器）
        // =====================================================================
        if (lampLoaded && !lampMeshes.empty()) {
            lampShader.use();
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);
            
            // Lamp lighting uniforms
            lampShader.setVec3("lightPos", lightPos);
            lampShader.setVec3("viewPos", camera.Position);
            lampShader.setVec3("lightColor", glm::vec3(1.5f, 1.5f, 1.5f));
            lampShader.setVec3("objectColor", glm::vec3(0.9f, 0.85f, 0.8f));
            lampShader.setBool("hasTexture", true);
            
            // Lamp emission (when ON)
            lampShader.setBool("lampOn", isLampOn());
            lampShader.setVec3("lampEmissionColor", lampLightColor);
            lampShader.setFloat("lampEmissionIntensity", isLampOn() ? lampLightIntensity : 0.0f);
            
            // Cylindrical texture mapping for lamp shade (no UV in OBJ)
            lampShader.setBool("useCylindricalMapping", true);
            lampShader.setFloat("cylYMin", -0.85f);  // Lamp shade bottom Y
            lampShader.setFloat("cylYMax", 1.38f);   // Lamp shade top Y
            
            // Lamp model transform
            model = glm::mat4(1.0f);
            model = glm::translate(model, LAMP_POSITION);
            model = glm::rotate(model, glm::radians(LAMP_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(LAMP_SCALE));
            lampShader.setMat4("model", model);
            
            // Render each material group
            for (const auto& mesh : lampMeshes) {
                if (!mesh.textures.empty() && mesh.textures[0].id != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
                    lampShader.setInt("texture_diffuse", 0);
                    lampShader.setBool("hasTexture", true);
                } else {
                    lampShader.setBool("hasTexture", false);
                }
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
            }
        }

        // =====================================================================
        // 渲染地形沙盘
        // =====================================================================
            lightingShader.use();
            
        // 地台
        lightingShader.setVec3("objectColor", 0.5f, 0.4f, 0.3f);
            lightingShader.setBool("hasTexture", false);
            lightingShader.setFloat("alpha", 1.0f);
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.54f, 0.57f));
            lightingShader.setMat4("model", model);
            glBindVertexArray(PlatformVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        // Terrain
            terrainShader.use();
            terrainShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);
            terrainShader.setVec3("lightPos", lightPos);
            terrainShader.setVec3("viewPos", camera.Position);
            terrainShader.setFloat("alpha", 1.0f);
            terrainShader.setMat4("projection", projection);
            terrainShader.setMat4("view", view);
            
            // Set lamp light uniforms for terrain
            terrainShader.setBool("lampOn", isLampOn());
            terrainShader.setVec3("lampLightPos", lampLightPos);
            terrainShader.setVec3("lampLightColor", lampLightColor);
            terrainShader.setFloat("lampLightIntensity", lampLightIntensity);
            terrainShader.setFloat("lampLightRadius", lampLightRadius);
            
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.57f, 0.57f));
            terrainShader.setMat4("model", model);
            glBindVertexArray(terrainMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(terrainMesh.indices.size()), GL_UNSIGNED_INT, 0);
            
        // 积雪层
            bool hasSnow = false;
            for (float height : snowHeightMap) {
            if (height > 0.001f) { hasSnow = true; break; }
                }
            if (hasSnow) {
                int topVertexCount = (TERRAIN_GRID_SIZE + 1) * (TERRAIN_GRID_SIZE + 1);
                for (int i = 0; i < topVertexCount; i++) {
                    snowMesh.vertices[i].Position.y = terrainMesh.vertices[i].Position.y + snowHeightMap[i];
                }
                glBindBuffer(GL_ARRAY_BUFFER, snowMesh.VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, snowMesh.vertices.size() * sizeof(Vertex), &snowMesh.vertices[0]);
                
                lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                lightingShader.setBool("hasTexture", false);
            lightingShader.setFloat("alpha", 0.95f);
                lightingShader.setMat4("projection", projection);
                lightingShader.setMat4("view", view);
                lightingShader.setMat4("model", model);
                glBindVertexArray(snowMesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(snowMesh.indices.size()), GL_UNSIGNED_INT, 0);
            lightingShader.setFloat("alpha", 1.0f);
        }

        // =====================================================================
        // 渲染天气效果
        // =====================================================================
        
        // 云层
        if (cloudVisible) {
            glDepthMask(GL_FALSE);
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.9f, 0.9f, 0.95f);
            lightingShader.setFloat("alpha", 0.5f);
            lightingShader.setBool("hasTexture", false);

            for (size_t i = 0; i < cloudSpheres.size(); i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, cloudPosition + cloudSpheres[i]);
                float scale = (i == 0) ? 0.09f : 0.06f;
                model = glm::scale(model, glm::vec3(scale, scale * 0.6f, scale * 0.75f));
                lightingShader.setMat4("model", model);
                glBindVertexArray(lightCubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            glDepthMask(GL_TRUE);
        }

        // 雨粒子
        if (isRaining) {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            lightCubeShader.setVec3("lightColor", 0.7f, 0.75f, 0.9f);

            for (const auto& particle : rainParticles) {
                if (particle.active) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, particle.position);
                    model = glm::scale(model, glm::vec3(0.005f, 0.015f, 0.005f));
                    lightCubeShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // 雪粒子
        if (isSnowing && cloudVisible) {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            lightCubeShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);

            for (const auto& particle : snowParticles) {
                if (particle.active) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, particle.position);
                    model = glm::rotate(model, particle.life * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.025f));
                    lightCubeShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // 闪电
        glDisable(GL_DEPTH_TEST);
        for (const auto& lightning : lightnings) {
            if (lightning.active) {
                lightCubeShader.use();
                lightCubeShader.setMat4("projection", projection);
                lightCubeShader.setMat4("view", view);
                
                float brightness = lightning.life / lightning.maxLife;
                if (brightness < 0.5f) brightness = 1.0f;
                lightCubeShader.setVec3("lightColor", 3.0f * brightness, 3.2f * brightness, 4.0f * brightness);

                for (size_t i = 0; i < lightning.segments.size() - 1; i++) {
                    glm::vec3 segStart = lightning.segments[i];
                    glm::vec3 segEnd = lightning.segments[i + 1];
                    glm::vec3 segCenter = (segStart + segEnd) * 0.5f;
                    glm::vec3 segDir = glm::normalize(segEnd - segStart);
                    float segLength = glm::length(segEnd - segStart);

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, segCenter);
                    
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                    if (abs(glm::dot(segDir, up)) < 0.999f) {
                        glm::vec3 rotAxis = glm::normalize(glm::cross(up, segDir));
                        float rotAngle = acos(glm::dot(up, segDir));
                        model = glm::rotate(model, rotAngle, rotAxis);
                    }
                    
                    model = glm::scale(model, glm::vec3(0.015f, segLength, 0.015f));
                    lightCubeShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }
        glEnable(GL_DEPTH_TEST);

        // 光源立方体
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
        lightCubeShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.1f));
            lightCubeShader.setMat4("model", model);
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        // 交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // =====================================================================
    // 清理资源
    // =====================================================================
    glDeleteVertexArrays(1, &CeilingVAO);
    glDeleteVertexArrays(1, &FloorVAO);
    glDeleteVertexArrays(1, &RWallVAO);
    glDeleteVertexArrays(1, &LWallVAO);
    glDeleteVertexArrays(1, &FWallVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &DeskVAO);
    glDeleteVertexArrays(1, &WindowVAO);
    glDeleteVertexArrays(1, &PlatformVAO);
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &VBO3);
    glDeleteBuffers(1, &VBO4);
    glDeleteBuffers(1, &VBO5);
    glDeleteBuffers(1, &VBO6);
    glDeleteBuffers(1, &VBO7);
    glDeleteBuffers(1, &VBO8);
    glDeleteBuffers(1, &VBO9);
    glDeleteBuffers(1, &WindowEBO);
    
    cleanupMesh(terrainMesh);
    cleanupMesh(snowMesh);
    cleanupMesh(tableMesh);
    for (auto& mesh : lampMeshes) {
        cleanupMesh(mesh);
                }
    
    cleanupWeatherSystem();

    glfwTerminate();
    return 0;
}
