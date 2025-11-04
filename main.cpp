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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// OBJ模型加载相关结构体和函数
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

// 加载纹理
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

// 简单的OBJ加载器
bool loadOBJ(const std::string& path, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> posIndices, normalIndices, texIndices;
    
    std::string line;
    while (std::getline(file, line)) {
        
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
                    continue;
                }
            }
        }
    }
    
    // 构建顶点数据
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
    
    // 验证数据完整性
    if (vertices.empty() || indices.empty()) {
        return false;
    }
    
    // 检查索引是否超出顶点范围
    for (size_t i = 0; i < indices.size(); i++) {
        if (indices[i] >= vertices.size()) {
            return false;
        }
    }
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
    
    // 加载OBJ模型和纹理
    // ------------------------------------
    Mesh tableMesh;
    bool objLoaded = false;
    
    if (loadOBJ("obj/table3.obj", tableMesh.vertices, tableMesh.indices)) {
        setupMesh(tableMesh);
        objLoaded = true;
        
        // 加载桌子纹理
        Texture woodTexture;
        woodTexture.id = loadTexture("obj/wood.jpg");
        woodTexture.type = "texture_diffuse";
        woodTexture.path = "obj/wood.jpg";
        tableMesh.textures.push_back(woodTexture);
        
        Texture pillowTexture;
        pillowTexture.id = loadTexture("obj/pillow.jpg");
        pillowTexture.type = "texture_diffuse";
        pillowTexture.path = "obj/pillow.jpg";
        tableMesh.textures.push_back(pillowTexture);
    }
    
    // 加载窗户纹理
    unsigned int windowTexture = loadTexture("window.png");

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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
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

        // ��ʼ��Ⱦ
        // ------
        
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);  // 增加背景亮度
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ȷ�������� Uniforms/Drawing ����ʱ���� Shader
        //---------------------------------------------------------------------
        lightingShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        //�����컨��
        {
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
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.49f));  // 放在前墙面上
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
        }

        // 绘制桌子模型（后绘制前景物体）
        {
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
            model = glm::translate(model, glm::vec3(0.0f, 0.3f, 0.4f));  // 进一步前移，确保椅子部分遮挡窗户
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
                
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tableMesh.indices.size()), GL_UNSIGNED_INT, 0);
                
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
        // ���ƵƷ���
        {
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
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &VBO3);
    glDeleteBuffers(1, &VBO4);
    glDeleteBuffers(1, &VBO5);
    glDeleteBuffers(1, &VBO6);
    glDeleteBuffers(1, &VBO7);
    glDeleteBuffers(1, &VBO8);
    glDeleteBuffers(1, &WindowEBO);

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

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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