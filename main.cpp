#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <cmath>
#include <random>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// 一些 OBJ 模型相关的基础结构，提前放在这里
// ------------------------------------------------------------------
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id = 0;
    std::string type;
    std::string path;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
};

// 地形沙盘相关的工具函数
// ------------------------------------------------------------------
// 下面几段是生成噪声的工具函数，用来给地形制造起伏
float noise2D(float x, float z, int seed) {
    // 非常朴素的伪随机噪声
    int n = (int)x + (int)z * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float interpolate(float a, float b, float x) {
    float ft = x * 3.1415927f;
    float f = (1.0f - cos(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

float smoothNoise(float x, float z, int seed) {
    float corners = (noise2D(x - 1.0f, z - 1.0f, seed) + noise2D(x + 1.0f, z - 1.0f, seed) +
                     noise2D(x - 1.0f, z + 1.0f, seed) + noise2D(x + 1.0f, z + 1.0f, seed)) / 16.0f;
    float sides = (noise2D(x - 1.0f, z, seed) + noise2D(x + 1.0f, z, seed) +
                   noise2D(x, z - 1.0f, seed) + noise2D(x, z + 1.0f, seed)) / 8.0f;
    float center = noise2D(x, z, seed) / 4.0f;
    return corners + sides + center;
}

float interpolatedNoise(float x, float z, int seed) {
    int intX = (int)x;
    float fracX = x - (float)intX;
    int intZ = (int)z;
    float fracZ = z - (float)intZ;

    float v1 = smoothNoise((float)intX, (float)intZ, seed);
    float v2 = smoothNoise((float)(intX + 1), (float)intZ, seed);
    float v3 = smoothNoise((float)intX, (float)(intZ + 1), seed);
    float v4 = smoothNoise((float)(intX + 1), (float)(intZ + 1), seed);

    float i1 = interpolate(v1, v2, fracX);
    float i2 = interpolate(v3, v4, fracX);
    return interpolate(i1, i2, fracZ);
}

float perlinNoise(float x, float z, int octaves, float persistence, int seed) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += interpolatedNoise(x * frequency, z * frequency, seed) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

// 负责生成地形网格，X/Z 可分别缩放，同时补齐侧面和底面
void generateTerrain(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                     int gridWidth, int gridHeight, float scaleX, float scaleZ, float heightScale) {
    vertices.clear();
    indices.clear();

    // 生成顶面顶点，顺便记录高度供侧面使用
    std::vector<float> heights;
    for (int z = 0; z <= gridHeight; z++) {
        for (int x = 0; x <= gridWidth; x++) {
            Vertex vertex;
            
            // 顶点在 X-Z 平面的坐标，X/Z 各自缩放
            float xPos = (x / (float)gridWidth - 0.5f) * scaleX;
            float zPos = (z / (float)gridHeight - 0.5f) * scaleZ;
            
            // 用 Perlin Noise 抬起地形，取绝对值防止穿台
            float height = perlinNoise(x * 0.2f, z * 0.2f, 4, 0.5f, 42) * heightScale;
            height = fabs(height); // 确保高度为正
            heights.push_back(height);
            
            vertex.Position = glm::vec3(xPos, height, zPos);
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // 先放个朝上的法线
            vertex.TexCoords = glm::vec2(x / (float)gridWidth, z / (float)gridHeight);
            
            vertices.push_back(vertex);
        }
    }
    
    int topVertexCount = (gridWidth + 1) * (gridHeight + 1);
    
    // 底面在 Y=0，用来封住模型
    for (int z = 0; z <= gridHeight; z++) {
        for (int x = 0; x <= gridWidth; x++) {
            Vertex vertex;
            float xPos = (x / (float)gridWidth - 0.5f) * scaleX;
            float zPos = (z / (float)gridHeight - 0.5f) * scaleZ;
            
            vertex.Position = glm::vec3(xPos, 0.0f, zPos);
            vertex.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
            vertex.TexCoords = glm::vec2(x / (float)gridWidth, z / (float)gridHeight);
            
            vertices.push_back(vertex);
        }
    }

    // 生成顶面索引
    for (int z = 0; z < gridHeight; z++) {
        for (int x = 0; x < gridWidth; x++) {
            int topLeft = z * (gridWidth + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridWidth + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    // 生成底面索引（反向，因为从下往上看）
    for (int z = 0; z < gridHeight; z++) {
        for (int x = 0; x < gridWidth; x++) {
            int topLeft = topVertexCount + z * (gridWidth + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = topVertexCount + (z + 1) * (gridWidth + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);

            indices.push_back(topRight);
            indices.push_back(bottomRight);
            indices.push_back(bottomLeft);
        }
    }
    
    // 四个侧面把顶、底连接起来
    // 前侧面 (z=0)
    for (int x = 0; x < gridWidth; x++) {
        int topLeft = x;
        int topRight = x + 1;
        int bottomLeft = topVertexCount + x;
        int bottomRight = topVertexCount + x + 1;
        
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
    }
    
    // 后侧面 (z=gridHeight)
    for (int x = 0; x < gridWidth; x++) {
        int topLeft = gridHeight * (gridWidth + 1) + x;
        int topRight = topLeft + 1;
        int bottomLeft = topVertexCount + gridHeight * (gridWidth + 1) + x;
        int bottomRight = bottomLeft + 1;
        
        indices.push_back(topLeft);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(bottomRight);
        indices.push_back(bottomLeft);
    }
    
    // 左侧面 (x=0)
    for (int z = 0; z < gridHeight; z++) {
        int topLeft = z * (gridWidth + 1);
        int topRight = (z + 1) * (gridWidth + 1);
        int bottomLeft = topVertexCount + z * (gridWidth + 1);
        int bottomRight = topVertexCount + (z + 1) * (gridWidth + 1);
        
        indices.push_back(topLeft);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(bottomRight);
        indices.push_back(bottomLeft);
    }
    
    // 右侧面 (x=gridWidth)
    for (int z = 0; z < gridHeight; z++) {
        int topLeft = z * (gridWidth + 1) + gridWidth;
        int topRight = (z + 1) * (gridWidth + 1) + gridWidth;
        int bottomLeft = topVertexCount + z * (gridWidth + 1) + gridWidth;
        int bottomRight = topVertexCount + (z + 1) * (gridWidth + 1) + gridWidth;
        
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
    }

    // 计算法向量（仅针对顶面）
    for (size_t i = 0; i < (gridWidth * gridHeight * 6); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].Position;
        glm::vec3 v1 = vertices[i1].Position;
        glm::vec3 v2 = vertices[i2].Position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        vertices[i0].Normal += normal;
        vertices[i1].Normal += normal;
        vertices[i2].Normal += normal;
    }

    // 归一化顶面法向量
    for (size_t i = 0; i < topVertexCount; i++) {
        vertices[i].Normal = glm::normalize(vertices[i].Normal);
    }
}

// 粒子系统
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life; // 剩余寿命
    bool active;
};

// 闪电
struct Lightning {
    glm::vec3 startPos;  // 起点（云层）
    glm::vec3 endPos;    // 终点（地面）
    float life;          // 剩余时间
    float maxLife;       // 总持续时间
    bool active;
    std::vector<glm::vec3> segments; // 折线分段点
};

// 天气状态相关变量
bool cloudVisible = false;
bool isRaining = false;
bool isSnowing = false;
bool cloudControlMode = false;
glm::vec3 cloudPosition(0.0f, 0.8f, 0.5f); // 雨云位置（降低高度，不在天花板上）
std::vector<Particle> rainParticles;
std::vector<Particle> snowParticles;
std::vector<Lightning> lightnings; // 闪电列表
float lightningTimer = 0.0f;       // 闪电生成计时器
float lightningInterval = 0.8f;    // 闪电生成间隔（秒）

// 记录每个地形顶点上的积雪厚度
const int TERRAIN_GRID_SIZE = 128;
std::vector<float> snowHeightMap((TERRAIN_GRID_SIZE + 1) * (TERRAIN_GRID_SIZE + 1), 0.0f);

// 防止长按键盘导致状态反复切换
bool keyMPressed = false;
bool keyRPressed = false;
bool keySPressed = false; // S键控制下雪

// 简单的纹理加载器
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;  // 默认格式
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
            
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    
    return textureID;
}

// 非常朴素的 OBJ 读取函数，够当前场景使用
bool loadOBJ(const std::string& path, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    std::cout << "Loading OBJ file: " << path << std::endl;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> posIndices, normalIndices, texIndices;
    
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (lineCount % 10000 == 0) {
            std::cout << "Processed " << lineCount << " lines..." << std::endl;
        }
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (prefix == "vt") {
            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        } else if (prefix == "f") {
            std::string vertex;
            while (iss >> vertex) {
                std::istringstream vStream(vertex);
                std::string posStr, texStr, normalStr;
                
                std::getline(vStream, posStr, '/');
                std::getline(vStream, texStr, '/');
                std::getline(vStream, normalStr, '/');
                
                try {
                    posIndices.push_back(std::stoi(posStr) - 1);
                    if (!texStr.empty()) texIndices.push_back(std::stoi(texStr) - 1);
                    if (!normalStr.empty()) normalIndices.push_back(std::stoi(normalStr) - 1);
                } catch (const std::exception& e) {
                    std::cout << "Error parsing face data at line " << lineCount << std::endl;
                    continue;
                }
            }
        }
    }
    
    std::cout << "Finished reading file. Positions: " << positions.size() 
              << ", Normals: " << normals.size() 
              << ", TexCoords: " << texCoords.size() 
              << ", Faces: " << posIndices.size() / 3 << std::endl;
    
    // 构建顶点数据
    std::cout << "Building vertex data..." << std::endl;
    for (size_t i = 0; i < posIndices.size(); i++) {
        Vertex vertex;
        
        if (posIndices[i] < positions.size()) {
            vertex.Position = positions[posIndices[i]];
        } else {
            vertex.Position = glm::vec3(0.0f);
        }
        
        if (i < texIndices.size() && texIndices[i] < texCoords.size()) {
            vertex.TexCoords = texCoords[texIndices[i]];
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        
        if (i < normalIndices.size() && normalIndices[i] < normals.size()) {
            vertex.Normal = normals[normalIndices[i]];
        } else {
            vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        
        vertices.push_back(vertex);
        indices.push_back(static_cast<unsigned int>(i));
    }
    
    std::cout << "OBJ loading completed. Vertices: " << vertices.size() << std::endl;
    
    // 验证数据完整性
    if (vertices.empty()) {
        std::cout << "Error: No vertices loaded!" << std::endl;
        return false;
    }
    
    if (indices.empty()) {
        std::cout << "Error: No indices loaded!" << std::endl;
        return false;
    }
    
    // 检查索引是否超出顶点范围
    for (size_t i = 0; i < indices.size(); i++) {
        if (indices[i] >= vertices.size()) {
            std::cout << "Error: Index " << indices[i] << " out of range (max: " << vertices.size() - 1 << ")" << std::endl;
            return false;
        }
    }
    
    std::cout << "Data validation passed!" << std::endl;
    return true;
}

// 设置网格
void setupMesh(Mesh& mesh) {
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    
    glBindVertexArray(mesh.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), &mesh.vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);
    
    // 顶点位置
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // 法向量
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    
    // 纹理坐标
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    
    glBindVertexArray(0);
}

// ��������
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// ���������
Camera camera(glm::vec3(0.0f, 1.0f, 2.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// ʱ������
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ��������
glm::vec3 lightPos(0.0f, 1.5f, 0.0f);  // 提高光源位置，增加亮度
glm::vec3 cubePos(0.0f, 0.8f, 0.2f);

int main()
{
    // ��ʼ��������glfw
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw��������
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // ���� GLFW �������ǵ����
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad���������� OpenGL ����ָ��
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // ����ȫ�� OpenGL ״̬
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    
    // 启用混合以支持透明度
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ����shader����
    // ------------------------------------
    Shader lightingShader("lighting.vs", "lighting.fs");
    Shader lightCubeShader("lightcube.vs", "lightcube.fs");
    Shader terrainShader("terrain.vs", "terrain.fs");  // 独立的地形着色器
    
    // 加载OBJ模型和纹理
    // ------------------------------------
    Mesh tableMesh;
    bool objLoaded = false;
    
    std::cout << "Attempting to load OBJ model..." << std::endl;
    if (loadOBJ("obj/table3.obj", tableMesh.vertices, tableMesh.indices)) {
        std::cout << "Successfully loaded table model with " << tableMesh.vertices.size() << " vertices" << std::endl;
        setupMesh(tableMesh);
        objLoaded = true;
        
        // 加载桌子纹理
        Texture woodTexture;
        woodTexture.id = loadTexture("obj/wood.jpg");
        woodTexture.type = "texture_diffuse";
        woodTexture.path = "obj/wood.jpg";
        std::cout << "Wood texture ID: " << woodTexture.id << std::endl;
        tableMesh.textures.push_back(woodTexture);
        
        Texture pillowTexture;
        pillowTexture.id = loadTexture("obj/pillow.jpg");
        pillowTexture.type = "texture_diffuse";
        pillowTexture.path = "obj/pillow.jpg";
        std::cout << "Pillow texture ID: " << pillowTexture.id << std::endl;
        tableMesh.textures.push_back(pillowTexture);
    } else {
        std::cout << "Failed to load table model, using simple geometry instead" << std::endl;
        objLoaded = false;
    }
    
    // 加载窗户纹理
    unsigned int windowTexture = loadTexture("window.png");
    std::cout << "Window texture ID: " << windowTexture << std::endl;

    // ͳһ�����õ���������Ϣ(ÿһ��ǰ��������Ϊ������꣬������Ϊ������)
    // ------------------------------------------------------------------
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    //��ȡ����ƽ�������
    // ------------------------------------------------------------------
    float CeilingVertices[36];
    std::copy(vertices + 180, vertices + 216, CeilingVertices);
    float FloorVertices[36];
    std::copy(vertices + 144, vertices + 180, FloorVertices);
    float LWallVertices[36];
    std::copy(vertices + 72, vertices + 108, LWallVertices);
    float RWallVertices[36];
    std::copy(vertices + 108, vertices + 144, RWallVertices);
    float FWallVertices[36];
    std::copy(vertices + 0, vertices + 36, FWallVertices);

    // 中式圆形镂空窗户几何数据 - 完美圆形
    // ------------------------------------------------------------------
    // 中式书桌几何数据 - 传统中式书桌模型
// ------------------------------------------------------------------
    float deskVertices[] = {
        // 桌面 - 矩形桌面 (6个面，每个面2个三角形)
        // 顶面
        -0.6f, 0.4f, -0.3f,  0.0f, 1.0f, 0.0f,  // 左上
         0.6f, 0.4f, -0.3f,  0.0f, 1.0f, 0.0f,  // 右上
         0.6f, 0.4f,  0.3f,  0.0f, 1.0f, 0.0f,  // 右下
         0.6f, 0.4f,  0.3f,  0.0f, 1.0f, 0.0f,  // 右下
        -0.6f, 0.4f,  0.3f,  0.0f, 1.0f, 0.0f,  // 左下
        -0.6f, 0.4f, -0.3f,  0.0f, 1.0f, 0.0f,  // 左上

        // 底面
        -0.6f, 0.35f, -0.3f,  0.0f, -1.0f, 0.0f,  // 左上
         0.6f, 0.35f, -0.3f,  0.0f, -1.0f, 0.0f,  // 右上
         0.6f, 0.35f,  0.3f,  0.0f, -1.0f, 0.0f,  // 右下
         0.6f, 0.35f,  0.3f,  0.0f, -1.0f, 0.0f,  // 右下
        -0.6f, 0.35f,  0.3f,  0.0f, -1.0f, 0.0f,  // 左下
        -0.6f, 0.35f, -0.3f,  0.0f, -1.0f, 0.0f,  // 左上

        // 前面
        -0.6f, 0.35f,  0.3f,  0.0f, 0.0f, 1.0f,  // 左下
         0.6f, 0.35f,  0.3f,  0.0f, 0.0f, 1.0f,  // 右下
         0.6f, 0.4f,   0.3f,  0.0f, 0.0f, 1.0f,  // 右上
         0.6f, 0.4f,   0.3f,  0.0f, 0.0f, 1.0f,  // 右上
        -0.6f, 0.4f,   0.3f,  0.0f, 0.0f, 1.0f,  // 左上
        -0.6f, 0.35f,  0.3f,  0.0f, 0.0f, 1.0f,  // 左下

        // 后面
        -0.6f, 0.35f, -0.3f,  0.0f, 0.0f, -1.0f,  // 左下
         0.6f, 0.35f, -0.3f,  0.0f, 0.0f, -1.0f,  // 右下
         0.6f, 0.4f,  -0.3f,  0.0f, 0.0f, -1.0f,  // 右上
         0.6f, 0.4f,  -0.3f,  0.0f, 0.0f, -1.0f,  // 右上
        -0.6f, 0.4f,  -0.3f,  0.0f, 0.0f, -1.0f,  // 左上
        -0.6f, 0.35f, -0.3f,  0.0f, 0.0f, -1.0f,  // 左下

        // 左面
        -0.6f, 0.35f, -0.3f,  -1.0f, 0.0f, 0.0f,  // 左下
        -0.6f, 0.35f,  0.3f,  -1.0f, 0.0f, 0.0f,  // 右下
        -0.6f, 0.4f,   0.3f,  -1.0f, 0.0f, 0.0f,  // 右上
        -0.6f, 0.4f,   0.3f,  -1.0f, 0.0f, 0.0f,  // 右上
        -0.6f, 0.4f,  -0.3f,  -1.0f, 0.0f, 0.0f,  // 左上
        -0.6f, 0.35f, -0.3f,  -1.0f, 0.0f, 0.0f,  // 左下

        // 右面
         0.6f, 0.35f, -0.3f,  1.0f, 0.0f, 0.0f,  // 左下
         0.6f, 0.35f,  0.3f,  1.0f, 0.0f, 0.0f,  // 右下
         0.6f, 0.4f,   0.3f,  1.0f, 0.0f, 0.0f,  // 右上
         0.6f, 0.4f,   0.3f,  1.0f, 0.0f, 0.0f,  // 右上
         0.6f, 0.4f,  -0.3f,  1.0f, 0.0f, 0.0f,  // 左上
         0.6f, 0.35f, -0.3f,  1.0f, 0.0f, 0.0f,  // 左下

        // 桌腿1 - 左前腿
        -0.5f, 0.0f,  0.2f,  0.0f, 0.0f, 1.0f,  // 左下
        -0.4f, 0.0f,  0.2f,  0.0f, 0.0f, 1.0f,  // 右下
        -0.4f, 0.35f, 0.2f,  0.0f, 0.0f, 1.0f,  // 右上
        -0.4f, 0.35f, 0.2f,  0.0f, 0.0f, 1.0f,  // 右上
        -0.5f, 0.35f, 0.2f,  0.0f, 0.0f, 1.0f,  // 左上
        -0.5f, 0.0f,  0.2f,  0.0f, 0.0f, 1.0f,  // 左下

        // 桌腿2 - 右前腿
         0.4f, 0.0f,  0.2f,  0.0f, 0.0f, 1.0f,  // 左下
         0.5f, 0.0f,  0.2f,  0.0f, 0.0f, 1.0f,  // 右下
         0.5f, 0.35f, 0.2f,  0.0f, 0.0f, 1.0f,  // 右上
         0.5f, 0.35f, 0.2f,  0.0f, 0.0f, 1.0f,  // 右上
         0.4f, 0.35f, 0.2f,  0.0f, 0.0f, 1.0f,  // 左上
         0.4f, 0.0f,  0.2f,  0.0f, 0.0f, 1.0f,  // 左下

        // 桌腿3 - 左后腿
        -0.5f, 0.0f,  -0.2f,  0.0f, 0.0f, -1.0f,  // 左下
        -0.4f, 0.0f,  -0.2f,  0.0f, 0.0f, -1.0f,  // 右下
        -0.4f, 0.35f, -0.2f,  0.0f, 0.0f, -1.0f,  // 右上
        -0.4f, 0.35f, -0.2f,  0.0f, 0.0f, -1.0f,  // 右上
        -0.5f, 0.35f, -0.2f,  0.0f, 0.0f, -1.0f,  // 左上
        -0.5f, 0.0f,  -0.2f,  0.0f, 0.0f, -1.0f,  // 左下

        // 桌腿4 - 右后腿
         0.4f, 0.0f,  -0.2f,  0.0f, 0.0f, -1.0f,  // 左下
         0.5f, 0.0f,  -0.2f,  0.0f, 0.0f, -1.0f,  // 右下
         0.5f, 0.35f, -0.2f,  0.0f, 0.0f, -1.0f,  // 右上
         0.5f, 0.35f, -0.2f,  0.0f, 0.0f, -1.0f,  // 右上
         0.4f, 0.35f, -0.2f,  0.0f, 0.0f, -1.0f,  // 左上
         0.4f, 0.0f,  -0.2f,  0.0f, 0.0f, -1.0f,  // 左下

        // 装饰围板 - 前面围板
        -0.55f, 0.1f,  0.25f,  0.0f, 0.0f, 1.0f,  // 左下
         0.55f, 0.1f,  0.25f,  0.0f, 0.0f, 1.0f,  // 右下
         0.55f, 0.3f,  0.25f,  0.0f, 0.0f, 1.0f,  // 右上
         0.55f, 0.3f,  0.25f,  0.0f, 0.0f, 1.0f,  // 右上
        -0.55f, 0.3f,  0.25f,  0.0f, 0.0f, 1.0f,  // 左上
        -0.55f, 0.1f,  0.25f,  0.0f, 0.0f, 1.0f,  // 左下

        // 装饰围板 - 后面围板
        -0.55f, 0.1f,  -0.25f,  0.0f, 0.0f, -1.0f,  // 左下
         0.55f, 0.1f,  -0.25f,  0.0f, 0.0f, -1.0f,  // 右下
         0.55f, 0.3f,  -0.25f,  0.0f, 0.0f, -1.0f,  // 右上
         0.55f, 0.3f,  -0.25f,  0.0f, 0.0f, -1.0f,  // 右上
        -0.55f, 0.3f,  -0.25f,  0.0f, 0.0f, -1.0f,  // 左上
        -0.55f, 0.1f,  -0.25f,  0.0f, 0.0f, -1.0f,  // 左下

        // 装饰围板 - 左面围板
        -0.6f, 0.1f,  -0.25f,  -1.0f, 0.0f, 0.0f,  // 左下
        -0.6f, 0.1f,   0.25f,  -1.0f, 0.0f, 0.0f,  // 右下
        -0.6f, 0.3f,   0.25f,  -1.0f, 0.0f, 0.0f,  // 右上
        -0.6f, 0.3f,   0.25f,  -1.0f, 0.0f, 0.0f,  // 右上
        -0.6f, 0.3f,  -0.25f,  -1.0f, 0.0f, 0.0f,  // 左上
        -0.6f, 0.1f,  -0.25f,  -1.0f, 0.0f, 0.0f,  // 左下

        // 装饰围板 - 右面围板
         0.6f, 0.1f,  -0.25f,  1.0f, 0.0f, 0.0f,  // 左下
         0.6f, 0.1f,   0.25f,  1.0f, 0.0f, 0.0f,  // 右下
         0.6f, 0.3f,   0.25f,  1.0f, 0.0f, 0.0f,  // 右上
         0.6f, 0.3f,   0.25f,  1.0f, 0.0f, 0.0f,  // 右上
         0.6f, 0.3f,  -0.25f,  1.0f, 0.0f, 0.0f,  // 左上
         0.6f, 0.1f,  -0.25f,  1.0f, 0.0f, 0.0f,  // 左下
    };

    // 简单窗户几何数据 - 带纹理坐标的矩形
// ------------------------------------------------------------------
    float windowVertices[] = {
        // 位置 (x, y, z)    法向量 (nx, ny, nz)    纹理坐标 (u, v)
        -0.4f,  0.4f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,  // 左上
         0.4f,  0.4f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  // 右上
         0.4f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,  // 右下
        -0.4f, -0.4f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,  // 左下
    };
    
    unsigned int windowIndices[] = {
        0, 1, 2,  // 第一个三角形
        2, 3, 0   // 第二个三角形
    };
    // �����컨��Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO1, CeilingVAO;
    {
        glGenVertexArrays(1, &CeilingVAO);
        glGenBuffers(1, &VBO1);

        glBindBuffer(GL_ARRAY_BUFFER, VBO1);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CeilingVertices), CeilingVertices, GL_STATIC_DRAW);

        glBindVertexArray(CeilingVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //����ذ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO2, FloorVAO;
    {
        glGenVertexArrays(1, &FloorVAO);
        glGenBuffers(1, &VBO2);

        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FloorVertices), FloorVertices, GL_STATIC_DRAW);

        glBindVertexArray(FloorVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //������ǽ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO3, LWallVAO;
    {
        glGenVertexArrays(1, &LWallVAO);
        glGenBuffers(1, &VBO3);

        glBindBuffer(GL_ARRAY_BUFFER, VBO3);
        glBufferData(GL_ARRAY_BUFFER, sizeof(LWallVertices), LWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(LWallVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //������ǽ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO4, RWallVAO;
    {
        glGenVertexArrays(1, &RWallVAO);
        glGenBuffers(1, &VBO4);

        glBindBuffer(GL_ARRAY_BUFFER, VBO4);
        glBufferData(GL_ARRAY_BUFFER, sizeof(RWallVertices), RWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(RWallVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //����ǰǽ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO5, FWallVAO;
    {
        glGenVertexArrays(1, &FWallVAO);
        glGenBuffers(1, &VBO5);

        glBindBuffer(GL_ARRAY_BUFFER, VBO5);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FWallVertices), FWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(FWallVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }



    // ���뷽��ƵĶ�����Ϣ
    unsigned int VBO6, lightCubeVAO;
    {
        glGenVertexArrays(1, &lightCubeVAO);
        glGenBuffers(1, &VBO6);

        glBindBuffer(GL_ARRAY_BUFFER, VBO6);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(lightCubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO6);
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // 创建书桌的几何信息
// ------------------------------------------------------------------
    unsigned int VBO7, DeskVAO;
    {
        glGenVertexArrays(1, &DeskVAO);
        glGenBuffers(1, &VBO7);

        glBindBuffer(GL_ARRAY_BUFFER, VBO7);
        glBufferData(GL_ARRAY_BUFFER, sizeof(deskVertices), deskVertices, GL_STATIC_DRAW);

        glBindVertexArray(DeskVAO);

        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 法向量属性
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // 创建窗户的几何信息
// ------------------------------------------------------------------
    unsigned int VBO8, WindowVAO, WindowEBO;
    {
        glGenVertexArrays(1, &WindowVAO);
        glGenBuffers(1, &VBO8);
        glGenBuffers(1, &WindowEBO);

        glBindVertexArray(WindowVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO8);
        glBufferData(GL_ARRAY_BUFFER, sizeof(windowVertices), windowVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WindowEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(windowIndices), windowIndices, GL_STATIC_DRAW);

        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 法向量属性
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // 纹理坐标属性
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    // 地台几何数据
    float platformVertices[] = {
        // 顶面
        -0.15f, 0.03f, -0.1125f,  0.0f, 1.0f, 0.0f,
         0.15f, 0.03f, -0.1125f,  0.0f, 1.0f, 0.0f,
         0.15f, 0.03f,  0.1125f,  0.0f, 1.0f, 0.0f,
         0.15f, 0.03f,  0.1125f,  0.0f, 1.0f, 0.0f,
        -0.15f, 0.03f,  0.1125f,  0.0f, 1.0f, 0.0f,
        -0.15f, 0.03f, -0.1125f,  0.0f, 1.0f, 0.0f,

        // 底面
        -0.15f, 0.0f, -0.1125f,  0.0f, -1.0f, 0.0f,
         0.15f, 0.0f, -0.1125f,  0.0f, -1.0f, 0.0f,
         0.15f, 0.0f,  0.1125f,  0.0f, -1.0f, 0.0f,
         0.15f, 0.0f,  0.1125f,  0.0f, -1.0f, 0.0f,
        -0.15f, 0.0f,  0.1125f,  0.0f, -1.0f, 0.0f,
        -0.15f, 0.0f, -0.1125f,  0.0f, -1.0f, 0.0f,

        // 前面
        -0.15f, 0.0f,  0.1125f,  0.0f, 0.0f, 1.0f,
         0.15f, 0.0f,  0.1125f,  0.0f, 0.0f, 1.0f,
         0.15f, 0.03f, 0.1125f,  0.0f, 0.0f, 1.0f,
         0.15f, 0.03f, 0.1125f,  0.0f, 0.0f, 1.0f,
        -0.15f, 0.03f, 0.1125f,  0.0f, 0.0f, 1.0f,
        -0.15f, 0.0f,  0.1125f,  0.0f, 0.0f, 1.0f,

        // 后面
        -0.15f, 0.0f, -0.1125f,  0.0f, 0.0f, -1.0f,
         0.15f, 0.0f, -0.1125f,  0.0f, 0.0f, -1.0f,
         0.15f, 0.03f,-0.1125f,  0.0f, 0.0f, -1.0f,
         0.15f, 0.03f,-0.1125f,  0.0f, 0.0f, -1.0f,
        -0.15f, 0.03f,-0.1125f,  0.0f, 0.0f, -1.0f,
        -0.15f, 0.0f, -0.1125f,  0.0f, 0.0f, -1.0f,

        // 左面
        -0.15f, 0.0f, -0.1125f,  -1.0f, 0.0f, 0.0f,
        -0.15f, 0.0f,  0.1125f,  -1.0f, 0.0f, 0.0f,
        -0.15f, 0.03f, 0.1125f,  -1.0f, 0.0f, 0.0f,
        -0.15f, 0.03f, 0.1125f,  -1.0f, 0.0f, 0.0f,
        -0.15f, 0.03f,-0.1125f,  -1.0f, 0.0f, 0.0f,
        -0.15f, 0.0f, -0.1125f,  -1.0f, 0.0f, 0.0f,

        // 右面
         0.15f, 0.0f, -0.1125f,  1.0f, 0.0f, 0.0f,
         0.15f, 0.0f,  0.1125f,  1.0f, 0.0f, 0.0f,
         0.15f, 0.03f, 0.1125f,  1.0f, 0.0f, 0.0f,
         0.15f, 0.03f, 0.1125f,  1.0f, 0.0f, 0.0f,
         0.15f, 0.03f,-0.1125f,  1.0f, 0.0f, 0.0f,
         0.15f, 0.0f, -0.1125f,  1.0f, 0.0f, 0.0f,
    };

    unsigned int VBO9, PlatformVAO;
    {
        glGenVertexArrays(1, &PlatformVAO);
        glGenBuffers(1, &VBO9);

        glBindBuffer(GL_ARRAY_BUFFER, VBO9);
        glBufferData(GL_ARRAY_BUFFER, sizeof(platformVertices), platformVertices, GL_STATIC_DRAW);

        glBindVertexArray(PlatformVAO);

        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 法向量属性
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // 生成地形网格 - 完全匹配地台大小(0.3x0.225)，提高分辨率到128
    Mesh terrainMesh;
    generateTerrain(terrainMesh.vertices, terrainMesh.indices, 128, 128, 0.3f, 0.225f, 0.12f);
    setupMesh(terrainMesh);
    std::cout << "Terrain mesh created with " << terrainMesh.vertices.size() << " vertices" << std::endl;
    
    // 创建积雪网格（初始时复制地形网格，后续动态更新）
    Mesh snowMesh;
    snowMesh.vertices = terrainMesh.vertices; // 复制地形顶点
    snowMesh.indices = terrainMesh.indices;   // 复制地形索引
    setupMesh(snowMesh);
    std::cout << "Snow mesh created with " << snowMesh.vertices.size() << " vertices" << std::endl;

    // 初始化雨粒子（细雨，数量多但体积小）
    rainParticles.resize(800);
    for (auto& particle : rainParticles) {
        particle.active = false;
    }

    // 初始化雪粒子
    snowParticles.resize(500);
    for (auto& particle : snowParticles) {
        particle.active = false;
    }

    // 初始化闪电（大幅增加数量以支持多分支震撼效果）
    lightnings.resize(20);
    for (auto& lightning : lightnings) {
        lightning.active = false;
    }

    // 创建雨云几何（多个椭球体组成的蓬松云朵）- 匹配地台大小(0.3x0.225)
    std::vector<glm::vec3> cloudSpheres = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.06f, 0.015f, 0.0f),
        glm::vec3(-0.06f, 0.015f, 0.0f),
        glm::vec3(0.0f, 0.015f, 0.045f),   // Z方向缩小（0.06*0.75=0.045）
        glm::vec3(0.0f, 0.015f, -0.045f),
        glm::vec3(0.03f, -0.008f, 0.0225f),
        glm::vec3(-0.03f, -0.008f, 0.0225f),
        glm::vec3(0.03f, -0.008f, -0.0225f),
        glm::vec3(-0.03f, -0.008f, -0.0225f),
    };

    std::cout << "Starting render loop..." << std::endl;

    // ��Ⱦѭ��
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // ʱ���߼�
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ����
        // -----
        processInput(window);

        // 更新粒子系统
        // ------
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
        
        // 更新雨粒子
        if (isRaining && cloudVisible) {
            for (auto& particle : rainParticles) {
                if (particle.active) {
                    // 更新位置
                    particle.position += particle.velocity * deltaTime;
                    particle.life -= deltaTime;

                    // 简单碰撞检测（与地形高度比较）- 地形从0.6开始
                    if (particle.position.y <= 0.6f || particle.life <= 0.0f) {
                        particle.active = false;
                    }
                } else {
                    // 激活新粒子（范围精确匹配地台：X=0.3, Z=0.225）
                    if (dist(gen) > 0.3f) {
                        particle.position = cloudPosition + glm::vec3(dist(gen) * 0.15f, -0.05f, dist(gen) * 0.1125f);
                        particle.velocity = glm::vec3(0.0f, -5.0f, 0.0f);
                        particle.life = 3.0f;
                        particle.active = true;
                    }
                }
            }
        }

        // 更新雪粒子（需要云层可见）
        if (isSnowing && cloudVisible) {
            for (auto& particle : snowParticles) {
                if (particle.active) {
                    // 更新位置（雪花飘落，带有水平飘动）
                    particle.position += particle.velocity * deltaTime;
                    particle.position.x += sin(particle.life * 2.0f) * 0.1f * deltaTime;
                    particle.life += deltaTime;

                    // 碰撞检测 - 地形从0.6开始
                    // 将世界坐标转换为地形网格坐标
                    float terrainCenterX = 0.0f;
                    float terrainCenterZ = 0.6f;
                    float terrainScaleX = 0.3f;
                    float terrainScaleZ = 0.225f;
                    
                    // 粒子相对于地形中心的位置
                    float relativeX = particle.position.x - terrainCenterX;
                    float relativeZ = particle.position.z - terrainCenterZ;
                    
                    // 转换为网格坐标 [0, TERRAIN_GRID_SIZE]
                    float gridX = (relativeX / terrainScaleX + 0.5f) * TERRAIN_GRID_SIZE;
                    float gridZ = (relativeZ / terrainScaleZ + 0.5f) * TERRAIN_GRID_SIZE;
                    
                    // 检查是否在地形范围内
                    if (gridX >= 0 && gridX <= TERRAIN_GRID_SIZE && 
                        gridZ >= 0 && gridZ <= TERRAIN_GRID_SIZE) {
                        
                        int ix = (int)gridX;
                        int iz = (int)gridZ;
                        
                        if (ix >= 0 && ix < TERRAIN_GRID_SIZE && iz >= 0 && iz < TERRAIN_GRID_SIZE) {
                            int vertexIndex = iz * (TERRAIN_GRID_SIZE + 1) + ix;
                            float currentSnowHeight = snowHeightMap[vertexIndex];
                            
                            // 简单碰撞检测
                            if (particle.position.y <= 0.6f + currentSnowHeight) {
                                particle.active = false;
                                // 在该顶点堆积雪（速度加倍）
                                snowHeightMap[vertexIndex] += 0.0002f;  // 从0.0001f增加到0.0002f
                                if (snowHeightMap[vertexIndex] > 0.05f) {
                                    snowHeightMap[vertexIndex] = 0.05f;
                                }
                            }
                        }
                    } else {
                        // 超出地形范围，直接消失
                        if (particle.position.y <= 0.6f) {
                            particle.active = false;
                        }
                    }
                } else {
                    // 激活新粒子（从云层位置生成，和下雨一样）
                    if (dist(gen) > 0.4f) {
                        particle.position = cloudPosition + glm::vec3(dist(gen) * 0.15f, -0.05f, dist(gen) * 0.1125f);
                        particle.velocity = glm::vec3(0.0f, -1.5f, 0.0f); // 雪比雨慢一点
                        particle.life = 0.0f;
                        particle.active = true;
                    }
                }
            }
        }

        // 更新闪电（仅在下雨时生成）
        if (isRaining && cloudVisible) {
            lightningTimer += deltaTime;
            
            // 定时生成新闪电
            if (lightningTimer >= lightningInterval) {
                lightningTimer = 0.0f;
                
                // 随机选择一个位置生成闪电
                std::uniform_real_distribution<float> lightningDist(-0.1f, 0.1f);
                glm::vec3 strikePos = cloudPosition + glm::vec3(lightningDist(gen) * 0.15f, 0.0f, lightningDist(gen) * 0.1125f);
                
                // 生成多条闪电（主电+分支）
                std::cout << "Lightning strike at (" << strikePos.x << ", " << strikePos.z << ")" << std::endl;
                
                int lightningCount = 0;
                int maxLightnings = 5 + (rand() % 3); // 一次生成5-7条闪电
                
                for (auto& lightning : lightnings) {
                    if (!lightning.active && lightningCount < maxLightnings) {
                        lightning.active = true;
                        
                        // 主闪电和分支有不同的起点，分支更散开
                        if (lightningCount == 0) {
                            // 主闪电
                            lightning.startPos = strikePos;
                        } else {
                            // 分支闪电，大幅偏移形成扇形散开
                            float branchOffset = 0.06f * lightningCount;
                            lightning.startPos = strikePos + glm::vec3(
                                lightningDist(gen) * branchOffset,
                                -0.04f * lightningCount,
                                lightningDist(gen) * branchOffset
                            );
                        }
                        
                        // 终点也大幅偏移，增加震撼感
                        lightning.endPos = glm::vec3(
                            lightning.startPos.x + lightningDist(gen) * 0.08f,
                            0.6f,
                            lightning.startPos.z + lightningDist(gen) * 0.08f
                        );
                        
                        lightning.life = 0.25f; // 持续时间
                        lightning.maxLife = 0.25f;
                        
                        // 生成大量折线段，极度曲折
                        lightning.segments.clear();
                        lightning.segments.push_back(lightning.startPos);
                        
                        int segmentCount = 12 + (rand() % 6); // 12-17个分段
                        float segmentHeight = (lightning.startPos.y - lightning.endPos.y) / segmentCount;
                        
                        glm::vec3 currentPos = lightning.startPos;
                        for (int i = 1; i < segmentCount; i++) {
                            currentPos.y -= segmentHeight;
                            // 大幅增加随机偏移，形成剧烈曲折
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
            lightningTimer = 0.0f; // 不下雨时重置计时器
        }
        
        // 更新所有活跃的闪电
        for (auto& lightning : lightnings) {
            if (lightning.active) {
                lightning.life -= deltaTime;
                if (lightning.life <= 0.0f) {
                    lightning.active = false;
                }
            }
        }

        // ��ʼ��Ⱦ
        // ------
        static int frameCount = 0;
        frameCount++;
        if (frameCount == 1) {
            std::cout << "First frame rendering..." << std::endl;
        }
        
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);  // 增加背景亮度
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 确保每帧开始时OpenGL状态正确
        glDepthMask(GL_TRUE);  // 确保深度写入开启
        glEnable(GL_BLEND);    // 确保混合开启
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // ȷ�������� Uniforms/Drawing ����ʱ���� Shader
        //---------------------------------------------------------------------
        if (frameCount == 1) {
            std::cout << "Using lighting shader..." << std::endl;
        }
        lightingShader.use();
        
        if (frameCount == 1) {
            std::cout << "Setting up matrices..." << std::endl;
        }
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        //�����컨��
        {
            if (frameCount == 1) {
                std::cout << "Rendering ceiling..." << std::endl;
            }
            //lightingShader.setVec3("objectColor", 0.5, 0.5f, 0.5f);
            lightingShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);  // 米黄色
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 增加光源强度
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(CeilingVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ���Ƶذ�
        {
            if (frameCount == 1) {
                std::cout << "Rendering floor..." << std::endl;
            }
            //lightingShader.setVec3("objectColor", 0.5f, 0.5f, 0.5f);
            lightingShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);  // 米黄色
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 增加光源强度
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(FloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ������ǽ
        {
            if (frameCount == 1) {
                std::cout << "Rendering left wall..." << std::endl;
            }
            //lightingShader.setVec3("objectColor", 1.0f, 0.0f, 0.31f);
            lightingShader.setVec3("objectColor", 0.6f, 0.3f, 0.2f);  // 深棕色
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 增加光源强度
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(LWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ������ǽ
        {
            if (frameCount == 1) {
                std::cout << "Rendering right wall..." << std::endl;
            }
            //lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
            lightingShader.setVec3("objectColor", 0.7f, 0.4f, 0.3f);  // 浅棕色
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 增加光源强度
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(RWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ����ǰǽ
        {
            if (frameCount == 1) {
                std::cout << "Rendering front wall..." << std::endl;
            }
            //lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.8f);  // 浅米色
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 增加光源强度
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(FWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        // 绘制窗户 - 带纹理的简单矩形（先绘制背景物体）
        {
            if (frameCount == 1) {
                std::cout << "Rendering window..." << std::endl;
            }
            // 设置光照参数
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // 白色，让纹理显示
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 增加光源强度
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setFloat("alpha", 0.7f);  // 设置透明度，0.7表示30%透明

            // view/projection 变换
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // 模型变换 - 将窗户放在前墙上
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);  // 先移动到场景中心
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.499f));  // 放在前墙面上
            model = glm::scale(model, glm::vec3(0.6f));  // 调整窗户大小
            lightingShader.setMat4("model", model);

            // 绑定窗户纹理
            if (windowTexture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, windowTexture);
                lightingShader.setInt("texture1", 0);
                lightingShader.setBool("hasTexture", true);
            } else {
                lightingShader.setBool("hasTexture", false);
            }

            // 绘制窗户（使用填充模式）
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(WindowVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 保持填充模式
            
            // 恢复默认状态
            lightingShader.setFloat("alpha", 1.0f);  // 恢复完全不透明
        }

        // 绘制桌子模型（后绘制前景物体）
        {
            if (frameCount == 1) {
                std::cout << "Rendering table..." << std::endl;
            }
            // 设置光照参数
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // 白色，让纹理显示
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 增加光源强度
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setFloat("alpha", 1.0f);  // 桌子完全不透明

            // view/projection 变换
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // 模型变换 - 将桌子放在窗户前面，屋子中间
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.33f, 0.5f));  // 进一步前移，确保椅子部分遮挡窗户
            model = glm::rotate(model, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // 绕Y轴旋转180度，让桌椅正对相机
            model = glm::scale(model, glm::vec3(0.08f));  // 稍微增大桌子尺寸
            lightingShader.setMat4("model", model);

            if (objLoaded && !tableMesh.vertices.empty()) {  // 启用OBJ渲染
                // 绘制OBJ模型
                // 绑定纹理
                if (!tableMesh.textures.empty() && tableMesh.textures[0].id != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, tableMesh.textures[0].id);
                    lightingShader.setInt("texture1", 0);
                    lightingShader.setBool("hasTexture", true);
                } else {
                    // 如果没有纹理，绑定一个默认纹理或禁用纹理
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    lightingShader.setInt("texture1", 0);
                    lightingShader.setBool("hasTexture", false);
                }

                // 绘制桌子（使用填充模式）
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glBindVertexArray(tableMesh.VAO);
                
                // 添加错误检查
                GLenum error = glGetError();
                if (error != GL_NO_ERROR) {
                    std::cout << "OpenGL error before drawing: " << error << std::endl;
                }
                
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tableMesh.indices.size()), GL_UNSIGNED_INT, 0);
                
                error = glGetError();
                if (error != GL_NO_ERROR) {
                    std::cout << "OpenGL error after drawing: " << error << std::endl;
                }
                
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 保持填充模式
            } else {
                // 回退到简单几何体
                lightingShader.setVec3("objectColor", 0.3f, 0.15f, 0.05f);  // 深红棕色木质
                lightingShader.setBool("hasTexture", false);  // 简单几何体不使用纹理
                
                // 绘制简单桌子（使用原有的DeskVAO）
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glBindVertexArray(DeskVAO);
                glDrawArrays(GL_TRIANGLES, 0, 120);  // 绘制整个书桌
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // 保持填充模式
            }
        }
        // 绘制地形沙盘系统
        {
            lightingShader.use();
            
            // 绘制地台
            lightingShader.setVec3("objectColor", 0.5f, 0.4f, 0.3f);  // 棕灰色
            lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);
            lightingShader.setFloat("alpha", 1.0f);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // 地台位置（在桌面上，略高于桌面）
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.57f, 0.6f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(PlatformVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // 绘制地形 - 使用独立的地形着色器，实现高度渐变效果
            terrainShader.use();
            terrainShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);
            terrainShader.setVec3("lightPos", lightPos);
            terrainShader.setVec3("viewPos", camera.Position);
            terrainShader.setFloat("alpha", 1.0f);
            terrainShader.setMat4("projection", projection);
            terrainShader.setMat4("view", view);
            
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.6f, 0.6f)); // 0.57(地台)+0.03(高度)=0.6，地形位置固定
            terrainShader.setMat4("model", model);

            glBindVertexArray(terrainMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(terrainMesh.indices.size()), GL_UNSIGNED_INT, 0);
            
            // 绘制积雪层（检查是否有任何积雪）
            bool hasSnow = false;
            for (float height : snowHeightMap) {
                if (height > 0.001f) {
                    hasSnow = true;
                    break;
                }
            }
            
            if (hasSnow) {
                // 更新积雪网格的顶点高度（只更新顶面顶点，不更新底面和侧面）
                int topVertexCount = (TERRAIN_GRID_SIZE + 1) * (TERRAIN_GRID_SIZE + 1);
                for (int i = 0; i < topVertexCount; i++) {
                    // 将积雪网格的Y坐标设置为地形Y坐标 + 该顶点的积雪厚度
                    snowMesh.vertices[i].Position.y = terrainMesh.vertices[i].Position.y + snowHeightMap[i];
                }
                
                // 重新上传积雪网格数据到GPU
                glBindBuffer(GL_ARRAY_BUFFER, snowMesh.VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, snowMesh.vertices.size() * sizeof(Vertex), &snowMesh.vertices[0]);
                
                // 切换回lighting着色器绘制积雪（不需要额外开关混合，保持全局状态）
                lightingShader.use();
                lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // 纯白色积雪
                lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);
                lightingShader.setVec3("lightPos", lightPos);
                lightingShader.setVec3("viewPos", camera.Position);
                lightingShader.setBool("hasTexture", false);
                lightingShader.setFloat("alpha", 0.95f);  // 几乎不透明
                lightingShader.setMat4("projection", projection);
                lightingShader.setMat4("view", view);
                
                // 积雪层位置：与地形相同
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, 0.6f, 0.6f));
                lightingShader.setMat4("model", model);
                
                glBindVertexArray(snowMesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(snowMesh.indices.size()), GL_UNSIGNED_INT, 0);
                
                lightingShader.setFloat("alpha", 1.0f);  // 恢复不透明
            }
        }

        // 绘制雨云（如果可见）- 使用半透明效果
        if (cloudVisible) {
            // 禁用深度写入,避免半透明云层遮挡后面的物体（保持全局混合状态）
            glDepthMask(GL_FALSE);
            
            lightingShader.use();
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);
            lightingShader.setVec3("objectColor", 0.9f, 0.9f, 0.95f); // 浅灰白色
            lightingShader.setVec3("lightColor", 1.2f, 1.2f, 1.2f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setBool("hasTexture", false);
            lightingShader.setFloat("alpha", 0.5f); // 半透明

            // 绘制多个椭球体组成蓬松的云朵 - 匹配地台矩形(0.3x0.225)
            for (size_t i = 0; i < cloudSpheres.size(); i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, cloudPosition + cloudSpheres[i]);
                // 不同大小的椭球体创造云朵的蓬松感，Z方向缩小75%
                float scale = (i == 0) ? 0.09f : 0.06f;
                model = glm::scale(model, glm::vec3(scale, scale * 0.6f, scale * 0.75f)); // 扁平椭球，Z方向更短
                lightingShader.setMat4("model", model);

                glBindVertexArray(lightCubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            
            // 恢复深度写入
            glDepthMask(GL_TRUE);
        }

        // 绘制雨粒子（细密的小雨点）
        if (isRaining) {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            lightCubeShader.setVec3("lightColor", 0.7f, 0.75f, 0.9f); // 半透明蓝白色

            for (const auto& particle : rainParticles) {
                if (particle.active) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, particle.position);
                    model = glm::scale(model, glm::vec3(0.005f, 0.015f, 0.005f)); // 更细小的雨点
                    lightCubeShader.setMat4("model", model);

                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // 绘制雪粒子
        if (isSnowing && cloudVisible) {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            lightCubeShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f); // 白色雪花

            for (const auto& particle : snowParticles) {
                if (particle.active) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, particle.position);
                    // 雪花旋转效果
                    model = glm::rotate(model, particle.life * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.025f)); // 雪花比雨大一点
                    lightCubeShader.setMat4("model", model);

                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // 绘制闪电（禁用深度测试，确保始终可见）
        glDisable(GL_DEPTH_TEST);
        for (const auto& lightning : lightnings) {
            if (lightning.active) {
                lightCubeShader.use();
                lightCubeShader.setMat4("projection", projection);
                lightCubeShader.setMat4("view", view);
                
                // 闪电颜色：极亮的蓝白电光
                float brightness = lightning.life / lightning.maxLife;
                if (brightness < 0.5f) brightness = 1.0f;
                lightCubeShader.setVec3("lightColor", 3.0f * brightness, 3.2f * brightness, 4.0f * brightness);

                // 绘制闪电的每一段（适中粗细的曲折电弧）
                for (size_t i = 0; i < lightning.segments.size() - 1; i++) {
                    glm::vec3 segStart = lightning.segments[i];
                    glm::vec3 segEnd = lightning.segments[i + 1];
                    glm::vec3 segCenter = (segStart + segEnd) * 0.5f;
                    glm::vec3 segDir = glm::normalize(segEnd - segStart);
                    float segLength = glm::length(segEnd - segStart);

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, segCenter);
                    
                    // 旋转使立方体沿着闪电方向
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                    if (abs(glm::dot(segDir, up)) < 0.999f) {
                        glm::vec3 rotAxis = glm::normalize(glm::cross(up, segDir));
                        float rotAngle = acos(glm::dot(up, segDir));
                        model = glm::rotate(model, rotAngle, rotAxis);
                    }
                    
                    model = glm::scale(model, glm::vec3(0.015f, segLength, 0.015f)); // 稍粗一点的闪电
                    lightCubeShader.setMat4("model", model);

                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }
        glEnable(GL_DEPTH_TEST); // 恢复深度测试

        // ���ƵƷ���
        {
            if (frameCount == 1) {
                std::cout << "Rendering light cube..." << std::endl;
            }
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPos);
            model = glm::scale(model, glm::vec3(0.1f)); // a smaller cube
            lightCubeShader.setMat4("model", model);

            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        
        if (frameCount == 1) {
            std::cout << "First frame completed successfully!" << std::endl;
        }


        // glfw����������������ѯ IO �¼�������/�ͷż����ƶ����ȣ�
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ����ѡ��һ����Դ��������;����ȡ������������Դ��
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &CeilingVAO);
    glDeleteVertexArrays(1, &FloorVAO);
    glDeleteVertexArrays(1, &RWallVAO);
    glDeleteVertexArrays(1, &LWallVAO);
    glDeleteVertexArrays(1, &FWallVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &DeskVAO);
    glDeleteVertexArrays(1, &WindowVAO);
    glDeleteVertexArrays(1, &PlatformVAO);
    glDeleteVertexArrays(1, &terrainMesh.VAO);
    glDeleteVertexArrays(1, &snowMesh.VAO);
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
    glDeleteBuffers(1, &terrainMesh.VBO);
    glDeleteBuffers(1, &terrainMesh.EBO);
    glDeleteBuffers(1, &snowMesh.VBO);
    glDeleteBuffers(1, &snowMesh.EBO);

    // glfw����ֹ�����������ǰ����� GLFW ��Դ��
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

//��ѯ GLFW �Ƿ���/�ͷ��˸�֡����ؼ���������Ӧ�ķ�Ӧ
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // M键切换雨云显示和控制模式
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        if (!keyMPressed) {
            cloudVisible = !cloudVisible;
            cloudControlMode = !cloudControlMode;
            
            // 当云层消失时,停止所有天气效果并清空粒子
            if (!cloudVisible) {
                isRaining = false;
                isSnowing = false;
                // 清空所有雨粒子
                for (auto& particle : rainParticles) {
                    particle.active = false;
                }
                // 清空所有雪粒子
                for (auto& particle : snowParticles) {
                    particle.active = false;
                }
            }
            
            keyMPressed = true;
            std::cout << "Cloud " << (cloudVisible ? "visible" : "hidden") 
                      << ", control mode " << (cloudControlMode ? "ON" : "OFF") << std::endl;
        }
    } else {
        keyMPressed = false;
    }

    // R键切换下雨
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!keyRPressed) {
            if (cloudVisible) {
                isRaining = !isRaining;
                keyRPressed = true;
                std::cout << "Rain " << (isRaining ? "started" : "stopped") << std::endl;
            } else {
                std::cout << "Need cloud to rain! Press M first." << std::endl;
                keyRPressed = true;
            }
        }
    } else {
        keyRPressed = false;
    }

    // S键切换下雪
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (!keySPressed) {
            if (cloudVisible) {
                isSnowing = !isSnowing;
                keySPressed = true;
                std::cout << "Snow " << (isSnowing ? "started" : "stopped") << std::endl;
                if (!isSnowing) {
                    // 停止下雪时，清空积雪高度图
                    std::fill(snowHeightMap.begin(), snowHeightMap.end(), 0.0f);
                }
            } else {
                std::cout << "Need cloud to snow! Press M first." << std::endl;
                keySPressed = true;
            }
        }
    } else {
        keySPressed = false;
    }

    // WAXD控制（雨云控制模式或相机控制模式）
    if (cloudControlMode) {
        // 雨云控制模式
        float cloudSpeed = 2.5f * deltaTime;
        
        // 沙盘边界（地台大小：0.3x0.225，中心在(0.0, 0.6)）
        // X范围：[-0.15, 0.15]，Z范围：[-0.1125, 0.1125]
        const float sandboxCenterX = 0.0f;
        const float sandboxCenterZ = 0.6f;
        const float sandboxHalfWidth = 0.15f;   // X方向半宽
        const float sandboxHalfDepth = 0.1125f; // Z方向半深
        
        glm::vec3 newCloudPos = cloudPosition;
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            newCloudPos.z += cloudSpeed;  // 向前
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            newCloudPos.z -= cloudSpeed;  // 向后（X键）
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            newCloudPos.x -= cloudSpeed;  // 向左
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            newCloudPos.x += cloudSpeed;  // 向右
        
        // 限制云层在沙盘范围内
        newCloudPos.x = glm::clamp(newCloudPos.x, 
                                    sandboxCenterX - sandboxHalfWidth, 
                                    sandboxCenterX + sandboxHalfWidth);
        newCloudPos.z = glm::clamp(newCloudPos.z, 
                                    sandboxCenterZ - sandboxHalfDepth, 
                                    sandboxCenterZ + sandboxHalfDepth);
        
        cloudPosition = newCloudPos;
    } else {
        // 相机控制模式
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);  // 向后（X键）
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

// glfw��ÿ�����ڴ�С�����仯��ͨ������ϵͳ���û�������С��ʱ���˻ص���������ִ��
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // ȷ���������µĴ��ڳߴ�ƥ��;��ע�⣬width��height�����Դ��� Retina ��ʾ����ָ���ĸ߶�
    glViewport(0, 0, width, height);
}


// glfw: ÿ������ƶ�ʱ���ûص����ᱻ����
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // ��ת����Ϊ y ������µ���

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw:ÿ�������ֹ���ʱ���ûص����ᱻ����
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}