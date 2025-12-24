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
#include "interactive_object.h"
#include "puzzle_game.h"
#include "spirit_orb.h"
#include "arrow_trap.h"

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
    Shader orbShader("orb.vs", "orb.fs");   // 灵珠发光着色器
    Shader particleShader("particle.vs", "particle.fs");  // 粒子着色器

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
    
    // Helper lambda to compute and print model bounds
    auto printModelBounds = [](const std::vector<Mesh>& meshes, const char* name) {
        if (meshes.empty()) return;
        glm::vec3 minPos(1e9f), maxPos(-1e9f);
        for (const auto& mesh : meshes) {
            for (const auto& v : mesh.vertices) {
                minPos = glm::min(minPos, v.Position);
                maxPos = glm::max(maxPos, v.Position);
    }
        }
        glm::vec3 size = maxPos - minPos;
        std::cout << name << " bounds: min(" << minPos.x << "," << minPos.y << "," << minPos.z 
                  << ") max(" << maxPos.x << "," << maxPos.y << "," << maxPos.z 
                  << ") size(" << size.x << "," << size.y << "," << size.z << ")" << std::endl;
    };
    
    // 花瓶模型 (directory must match OBJ file location for mtllib resolution)
    std::vector<Mesh> vaseMeshes;
    bool vaseLoaded = false;
    if (loadOBJWithMaterials("obj/vase_obj/vase.obj", vaseMeshes, "obj/vase_obj")) {
        vaseLoaded = true;
        std::cout << "Vase model loaded: " << vaseMeshes.size() << " meshes" << std::endl;
        printModelBounds(vaseMeshes, "Vase");
    } else {
        std::cerr << "Failed to load vase model!" << std::endl;
    }
    
    // 书籍模型
    std::vector<Mesh> bookMeshes;
    bool bookLoaded = false;
    if (loadOBJWithMaterials("obj/book_obj/book.obj", bookMeshes, "obj/book_obj")) {
        bookLoaded = true;
        std::cout << "Book model loaded: " << bookMeshes.size() << " meshes" << std::endl;
        printModelBounds(bookMeshes, "Book");
    } else {
        std::cerr << "Failed to load book model!" << std::endl;
    }
    
    // 卷轴模型
    std::vector<Mesh> scrollMeshes;
    bool scrollLoaded = false;
    if (loadOBJWithMaterials("obj/scroll_obj/scroll.obj", scrollMeshes, "obj/scroll_obj")) {
        scrollLoaded = true;
        std::cout << "Scroll model loaded: " << scrollMeshes.size() << " meshes" << std::endl;
        printModelBounds(scrollMeshes, "Scroll");
    } else {
        std::cerr << "Failed to load scroll model!" << std::endl;
    }
    
    // 书柜模型 (against the opposite wall, will move to reveal secret compartment)
    std::vector<Mesh> bookcaseMeshes;
    bool bookcaseLoaded = false;
    if (loadOBJWithMaterials("obj/bookcase_obj/bookcase1.obj", bookcaseMeshes, "obj/bookcase_obj")) {
        bookcaseLoaded = true;
        std::cout << "Bookcase model loaded: " << bookcaseMeshes.size() << " meshes" << std::endl;
        printModelBounds(bookcaseMeshes, "Bookcase");
    } else {
        std::cerr << "Failed to load bookcase model!" << std::endl;
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

    // =====================================================================
    // Initialize Puzzle Game System
    // =====================================================================
    initInteractiveObjects();
    initPuzzleGame();
    initSpiritOrb();
    initArrowTrap();
    
    // Add interactive objects to the scene
    // Vase (DECOY) - click to trigger arrow trap mechanism
    int vaseIdx = addInteractiveObject("Vase", ObjectType::DECOY, VASE_POSITION, 0.08f);
    g_interactiveObjects[vaseIdx].scale = glm::vec3(VASE_SCALE);
    g_interactiveObjects[vaseIdx].baseColor = glm::vec3(0.9f, 0.9f, 0.95f);  // Blue-white porcelain
    g_interactiveObjects[vaseIdx].highlightColor = glm::vec3(1.0f, 0.5f, 0.3f);  // Warning orange when triggered
    
    // Book (movable) - click to slide, on desk right of sandbox
    int bookIdx = addInteractiveObject("Book", ObjectType::MOVABLE, BOOK_POSITION, 0.08f);
    g_interactiveObjects[bookIdx].scale = glm::vec3(BOOK_SCALE);
    g_interactiveObjects[bookIdx].baseColor = glm::vec3(0.6f, 0.4f, 0.3f);  // Brown leather
    g_interactiveObjects[bookIdx].highlightColor = glm::vec3(1.0f, 0.9f, 0.5f);
    
    // Scroll - THE MECHANISM (correct puzzle trigger) - click to slide and reveal orb
    int scrollIdx = addInteractiveObject("Scroll", ObjectType::MECHANISM, SCROLL_POSITION, 0.1f);
    g_interactiveObjects[scrollIdx].scale = glm::vec3(SCROLL_SCALE);
    g_interactiveObjects[scrollIdx].baseColor = glm::vec3(0.85f, 0.75f, 0.6f);  // Bamboo/parchment
    g_interactiveObjects[scrollIdx].highlightColor = glm::vec3(1.0f, 0.8f, 0.3f);
    
    // Generate puzzle meshes
    Mesh floorTileMesh, compartmentMesh, orbMesh, glowQuadMesh;
    generateFloorTileMesh(floorTileMesh);
    setupMesh(floorTileMesh);
    generateCompartmentMesh(compartmentMesh);
    setupMesh(compartmentMesh);
    generateOrbMesh(orbMesh);
    
    // Generate arrow trap meshes
    Mesh arrowMesh, compartmentDoorMesh;
    generateArrowMesh(arrowMesh);
    generateCompartmentDoorMesh(compartmentDoorMesh);
    setupMesh(orbMesh);
    generateGlowQuadMesh(glowQuadMesh);
    setupMesh(glowQuadMesh);

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
        
        // 更新解谜游戏系统
        updateInteractiveObjects(deltaTime);
        updatePuzzleGame(deltaTime);
        updateArrowTrap(deltaTime);
        if (shouldRenderOrb()) {
            updateSpiritOrb(deltaTime, getOrbPosition());
        }

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
        
        // Handle object selection via mouse hover (after matrices are ready)
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        int windowW, windowH;
        glfwGetWindowSize(window, &windowW, &windowH);
        g_hoveredObjectIndex = pickObject(static_cast<float>(mouseX), static_cast<float>(mouseY),
                                          windowW, windowH, view, projection, camera.Position);

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
        
        // Set spirit orb light uniforms
        glm::vec3 orbLightPos = getOrbPosition();
        lightingShader.setBool("orbEmitting", isOrbEmittingLight());
        lightingShader.setVec3("orbLightPos", orbLightPos);
        lightingShader.setVec3("orbLightColor", ORB_COLOR_CORE);
        lightingShader.setFloat("orbLightIntensity", getOrbLightIntensity());
        lightingShader.setFloat("orbLightRadius", ORB_LIGHT_RADIUS);

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

        // Render hole in floor when tile is open (black square to cover floor)
        if (g_secretTile.isOpen || g_secretTile.isAnimating) {
            lightingShader.setVec3("objectColor", 0.02f, 0.01f, 0.01f);  // Very dark (hole)
            lightingShader.setBool("hasTexture", false);
            
            // Position the hole at tile location, at floor level to cover it
            // lightCubeVAO is a unit cube (-0.5 to 0.5), need to position and scale correctly
            glm::mat4 holeModel = glm::mat4(1.0f);
            // Move to center of where tile should be (tile starts at position, extends +X and +Z)
            float halfW = COMPARTMENT_SIZE_CONFIG.x * 0.5f;
            float halfD = COMPARTMENT_SIZE_CONFIG.z * 0.5f;
            holeModel = glm::translate(holeModel, glm::vec3(
                g_secretTile.position.x + halfW,  // Center X
                FLOOR_HEIGHT + 0.002f,            // At floor level (tiny offset to avoid z-fighting)
                g_secretTile.position.z + halfD   // Center Z
            ));
            holeModel = glm::scale(holeModel, glm::vec3(
                COMPARTMENT_SIZE_CONFIG.x + 0.005f,  // Slightly wider to cover edges
                0.003f,                              // Very thin
                COMPARTMENT_SIZE_CONFIG.z + 0.005f   // Slightly deeper to cover edges
            ));
            lightingShader.setMat4("model", holeModel);
            
            // Use light cube as a flat box to cover the floor
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

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
        // Render Window (with alpha blending for transparent parts)
        // =====================================================================
        // Enable blending for transparent texture regions
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("alpha", 1.0f);  // Use texture's own alpha
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
        
        glDisable(GL_BLEND);
        lightingShader.setFloat("alpha", 1.0f);

        // =====================================================================
        // 渲染桌子 (against right wall, facing left wall)
        // =====================================================================
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("alpha", 1.0f);
            model = glm::mat4(1.0f);
        model = glm::translate(model, TABLE_POSITION);
        model = glm::rotate(model, glm::radians(TABLE_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(TABLE_SCALE));
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
            
        // 地台 (sandbox platform on desk)
        lightingShader.setVec3("objectColor", 0.5f, 0.4f, 0.3f);
            lightingShader.setBool("hasTexture", false);
            lightingShader.setFloat("alpha", 1.0f);
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(SANDBOX_CENTER_X, 0.54f, SANDBOX_CENTER_Z));
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
            
            // Set spirit orb light uniforms for terrain
            terrainShader.setBool("orbEmitting", isOrbEmittingLight());
            terrainShader.setVec3("orbLightPos", orbLightPos);
            terrainShader.setVec3("orbLightColor", ORB_COLOR_CORE);
            terrainShader.setFloat("orbLightIntensity", getOrbLightIntensity());
            terrainShader.setFloat("orbLightRadius", ORB_LIGHT_RADIUS);
            
        model = glm::translate(glm::mat4(1.0f), glm::vec3(SANDBOX_CENTER_X, 0.57f, SANDBOX_CENTER_Z));
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

        // =====================================================================
        // 渲染解谜游戏元素
        // =====================================================================
        
        // Render interactive objects (vase, book, scroll)
        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setFloat("alpha", 1.0f);
        
        for (size_t i = 0; i < g_interactiveObjects.size(); i++) {
            const InteractiveObject& obj = g_interactiveObjects[i];
            
            // Set color (highlight if selected or hovered)
            glm::vec3 color = obj.baseColor;
            if (obj.isSelected) {
                color = obj.highlightColor;
            } else if (static_cast<int>(i) == g_hoveredObjectIndex) {
                color = glm::mix(obj.baseColor, obj.highlightColor, 0.5f);
            }
            lightingShader.setVec3("objectColor", color);
            
            // Apply object transform
            glm::mat4 objModel = getObjectModelMatrix(static_cast<int>(i));
            lightingShader.setMat4("model", objModel);
            
            // Render with actual model meshes
            if (i == 0 && vaseLoaded && !vaseMeshes.empty()) {
                // Vase
                for (const auto& mesh : vaseMeshes) {
                    if (!mesh.textures.empty() && mesh.textures[0].id != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
                        lightingShader.setInt("texture1", 0);
                        lightingShader.setBool("hasTexture", true);
                    } else {
                        lightingShader.setBool("hasTexture", false);
                    }
                    glBindVertexArray(mesh.VAO);
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
                }
            } else if (i == 1 && bookLoaded && !bookMeshes.empty()) {
                // Book
                for (const auto& mesh : bookMeshes) {
                    if (!mesh.textures.empty() && mesh.textures[0].id != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
                        lightingShader.setInt("texture1", 0);
                        lightingShader.setBool("hasTexture", true);
                    } else {
                        lightingShader.setBool("hasTexture", false);
                    }
                    glBindVertexArray(mesh.VAO);
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
                }
            } else if (i == 2 && scrollLoaded && !scrollMeshes.empty()) {
                // Scroll
                for (const auto& mesh : scrollMeshes) {
                    if (!mesh.textures.empty() && mesh.textures[0].id != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
                        lightingShader.setInt("texture1", 0);
                        lightingShader.setBool("hasTexture", true);
                    } else {
                        lightingShader.setBool("hasTexture", false);
                    }
                    glBindVertexArray(mesh.VAO);
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
                }
            } else {
                // Fallback to cube
                lightingShader.setBool("hasTexture", false);
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
            }
        
        // =====================================================================
        // 渲染书柜 (with dynamic position for trap animation)
        // =====================================================================
        if (bookcaseLoaded && !bookcaseMeshes.empty()) {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setFloat("alpha", 1.0f);
            
            // Use dynamic bookcase matrix (animated position)
            model = getBookcaseMatrix();
            lightingShader.setMat4("model", model);
            
            for (const auto& mesh : bookcaseMeshes) {
                if (!mesh.textures.empty() && mesh.textures[0].id != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
                    lightingShader.setInt("texture1", 0);
                    lightingShader.setBool("hasTexture", true);
                } else {
                    lightingShader.setBool("hasTexture", false);
                }
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
            }
        }
        
        // =====================================================================
        // 渲染墙面暗格 (on wall surface, visible after bookcase moves)
        // =====================================================================
        if (isCompartmentVisible()) {
            lightingShader.use();
            lightingShader.setBool("hasTexture", false);
            
            glm::vec3 compartmentPos = g_wallCompartment.position;
            glm::vec3 compartmentSize = g_wallCompartment.size;
            float panelProgress = g_wallCompartment.panelSlideProgress;
            
            // Render the dark hole (extends INTO wall from wall surface)
            lightingShader.setVec3("objectColor", 0.01f, 0.005f, 0.005f);  // Pure black hole
            model = glm::mat4(1.0f);
            // Hole extends into wall (+X direction)
            model = glm::translate(model, compartmentPos + glm::vec3(0.08f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(
                0.15f,                      // Depth into wall
                compartmentSize.y,          // Full height
                compartmentSize.z           // Full width
            ));
            lightingShader.setMat4("model", model);
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            
            // Render the sliding panels (same color as right wall)
            // Two panels that start covering the hole and slide up/down from center
            if (panelProgress < 1.0f) {
                lightingShader.setVec3("objectColor", 0.7f, 0.4f, 0.3f);  // Same as right wall
                
                float halfHeight = compartmentSize.y * 0.5f;
                float slideOffset = panelProgress * halfHeight;
                float panelX = -0.02f;  // Slightly in front of wall surface (toward room)
                float panelThickness = 0.015f;
                
                // Upper panel - covers top half, slides UP to disappear
                float upperVisible = halfHeight - slideOffset;
                if (upperVisible > 0.002f) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, compartmentPos + glm::vec3(panelX, 
                        slideOffset + upperVisible * 0.5f, 0.0f));
                    model = glm::scale(model, glm::vec3(panelThickness, upperVisible, compartmentSize.z));
                    lightingShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                
                // Lower panel - covers bottom half, slides DOWN to disappear
                float lowerVisible = halfHeight - slideOffset;
                if (lowerVisible > 0.002f) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, compartmentPos + glm::vec3(panelX, 
                        -slideOffset - lowerVisible * 0.5f, 0.0f));
                    model = glm::scale(model, glm::vec3(panelThickness, lowerVisible, compartmentSize.z));
                    lightingShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }
        
        // =====================================================================
        // 渲染箭矢 (arrow particles)
        // =====================================================================
        if (!g_arrows.empty()) {
            lightingShader.use();
            lightingShader.setBool("hasTexture", false);
            
            for (const auto& arrow : g_arrows) {
                if (!arrow.active) continue;
                
                // Arrow color based on source
                if (arrow.sourceType == 0) {
                    lightingShader.setVec3("objectColor", 0.6f, 0.4f, 0.2f);  // Brown (from wall)
    } else {
                    lightingShader.setVec3("objectColor", 0.3f, 0.3f, 0.35f); // Dark gray (from window)
                }
                
                // Fade out collided arrows
                if (arrow.hasCollided) {
                    lightingShader.setFloat("alpha", 0.5f);
            } else {
                    lightingShader.setFloat("alpha", 1.0f);
                }
                
                model = getArrowMatrix(arrow);
                lightingShader.setMat4("model", model);
                glBindVertexArray(arrowMesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(arrowMesh.indices.size()), GL_UNSIGNED_INT, 0);
            }
            lightingShader.setFloat("alpha", 1.0f);
        }
        
        // Render secret compartment (dark hole box) - visible when tile opens
        if (g_compartment.isVisible || g_secretTile.isOpen || g_secretTile.isAnimating) {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.08f, 0.05f, 0.03f);  // Dark brown/black interior
            lightingShader.setBool("hasTexture", false);
            
            // Compartment is below the floor tile
            // Position: SECRET_TILE_POS - (0, COMPARTMENT_SIZE.y, 0)
            glm::mat4 compModel = glm::mat4(1.0f);
            compModel = glm::translate(compModel, g_compartment.position);
            lightingShader.setMat4("model", compModel);
            
            glBindVertexArray(compartmentMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(compartmentMesh.indices.size()), GL_UNSIGNED_INT, 0);
        }
        
        // Render floor tile - ALWAYS render (as part of floor when closed, animated when open)
        {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // White to show texture
            
            // Apply floor texture to the tile
            if (floorTexture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, floorTexture);
                lightingShader.setInt("texture1", 0);
                lightingShader.setBool("hasTexture", true);
            } else {
                lightingShader.setBool("hasTexture", false);
            }
            
            glm::mat4 tileModel = getFloorTileMatrix();
            lightingShader.setMat4("model", tileModel);
            
            glBindVertexArray(floorTileMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(floorTileMesh.indices.size()), GL_UNSIGNED_INT, 0);
            lightingShader.setBool("hasTexture", false);
        }
        
        // Render spirit orb with glow effect
        if (shouldRenderOrb()) {
            // Enable additive blending for glow
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            
            orbShader.use();
            orbShader.setMat4("projection", projection);
            orbShader.setMat4("view", view);
            orbShader.setMat4("model", getOrbModelMatrix());
            orbShader.setVec3("viewPos", camera.Position);
            orbShader.setVec3("coreColor", g_spiritOrb.coreColor);
            orbShader.setVec3("glowColor", g_spiritOrb.glowColor);
            orbShader.setFloat("glowIntensity", getOrbGlowIntensity());
            orbShader.setFloat("time", static_cast<float>(glfwGetTime()));
            
            glBindVertexArray(orbMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(orbMesh.indices.size()), GL_UNSIGNED_INT, 0);
            
            // Render glow particles
            particleShader.use();
            particleShader.setMat4("projection", projection);
            particleShader.setMat4("view", view);
            particleShader.setVec3("particleColor", g_spiritOrb.particleColor);
            
            const auto& particles = getOrbParticles();
            for (const auto& particle : particles) {
                if (particle.active) {
                    particleShader.setVec3("particlePos", particle.position);
                    particleShader.setFloat("particleSize", particle.size);
                    particleShader.setFloat("particleAlpha", particle.alpha * 0.6f);
                    
                    glBindVertexArray(glowQuadMesh.VAO);
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(glowQuadMesh.indices.size()), GL_UNSIGNED_INT, 0);
                }
            }
            
            // Reset blending
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        // Game complete message (could add UI rendering here)
        if (getGameState() == GameState::GAME_COMPLETE) {
            // Victory! The orb has been collected
            // Could render a congratulations message or effect here
        }

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
    for (auto& mesh : vaseMeshes) {
        cleanupMesh(mesh);
    }
    for (auto& mesh : bookMeshes) {
        cleanupMesh(mesh);
    }
    for (auto& mesh : scrollMeshes) {
        cleanupMesh(mesh);
    }
    for (auto& mesh : bookcaseMeshes) {
        cleanupMesh(mesh);
    }

    cleanupWeatherSystem();
    cleanupInteractiveObjects();
    cleanupPuzzleGame();
    cleanupSpiritOrb();
    cleanupArrowTrap();
    
    cleanupMesh(floorTileMesh);
    cleanupMesh(compartmentMesh);
    cleanupMesh(orbMesh);
    cleanupMesh(arrowMesh);
    cleanupMesh(compartmentDoorMesh);
    cleanupMesh(glowQuadMesh);

    glfwTerminate();
    return 0;
}
