// =====================================================================
// SimpleScene - 中式场景渲染项目
// 重构版本：模块化代码结构
// 
// 【项目概述】
// 这是一个基于OpenGL的3D场景渲染项目，模拟一个中式风格的房间场景。
// 主要功能包括：
// 1. 3D房间渲染（墙壁、地板、天花板、窗户）
// 2. 3D模型加载与渲染（桌子、台灯、花瓶、书籍、卷轴、书柜）
// 3. 程序化地形生成（桌上沙盘的山丘地形）
// 4. 天气系统（云层、下雨、下雪、闪电效果）
// 5. 解谜游戏系统（地砖翻转、暗格打开、灵珠发光）
// 6. 箭矢陷阱系统（诱饵触发、书柜移动）
// 7. 光照系统（台灯开关、灵珠光源）
// 8. 鼠标点击交互（射线拾取选取3D对象）
// =====================================================================

// ============================ 头文件包含 ============================
// 【OpenGL核心库】
#include <glad/glad.h>    // GLAD: OpenGL函数指针加载库，必须在GLFW之前包含
#include <GLFW/glfw3.h>   // GLFW: 窗口管理、输入处理、OpenGL上下文创建

// 【GLM数学库】用于3D图形的数学运算
#include <glm/glm.hpp>                    // 基础数学类型（vec3, mat4等）
#include <glm/gtc/matrix_transform.hpp>   // 变换函数（translate, rotate, scale, perspective）
#include <glm/gtc/type_ptr.hpp>           // 类型指针转换（value_ptr用于传递数据给OpenGL）

// 【核心渲染模块】
#include "shader.h"       // 着色器类：编译、链接、使用GLSL着色器程序
#include "camera.h"       // 相机类：视图变换、键盘/鼠标输入控制

// 【stb_image 图像加载库】
// STB_IMAGE_IMPLEMENTATION 宏定义必须且只能在一个.cpp文件中出现
// 它会生成stb_image的实现代码
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"    // 图像加载：支持JPG/PNG等格式

// ============================ 自定义模块 ============================
#include "config.h"           // 全局配置：窗口尺寸、房间大小、物体位置等常量
#include "types.h"            // 数据类型：Vertex、Texture、Mesh、Particle等结构体
#include "geometry_data.h"    // 几何数据：房间各面的顶点数据数组
#include "texture_loader.h"   // 纹理加载：从文件加载纹理到GPU
#include "model_loader.h"     // 模型加载：OBJ模型和MTL材质文件解析
#include "mesh.h"             // 网格管理：VAO/VBO/EBO的创建、绑定和清理
#include "terrain.h"          // 地形生成：程序化生成山丘地形网格
#include "weather.h"          // 天气系统：云层移动、雨雪粒子、闪电效果
#include "input.h"            // 输入处理：键盘移动、鼠标旋转、点击回调
#include "lamp_light.h"       // 台灯光源：开关控制、光照衰减计算
#include "ray_picking.h"      // 射线拾取：鼠标点击位置转换为3D射线
#include "interactive_object.h"  // 可交互对象：花瓶、书籍、卷轴的点击响应
#include "puzzle_game.h"      // 解谜游戏：地砖翻转、暗格打开的状态机
#include "spirit_orb.h"       // 灵珠系统：发光效果、粒子、光线投射
#include "arrow_trap.h"       // 箭矢陷阱：诱饵触发、书柜移动、箭矢飞射
#include "mesh_collision.h"   // Mesh碰撞检测：箭矢与模型的精确碰撞

// 【C++标准库】
#include <iostream>   // 控制台输出（调试信息）
#include <vector>     // 动态数组容器
#include <cmath>      // 数学函数（sin, cos, abs等）

// =====================================================================
// 【程序化纹理生成】中式地砖纹理
// =====================================================================
// 这个函数不从文件加载纹理，而是通过代码算法生成一个中式风格的地砖纹理
// 优点：不需要外部纹理文件，可以自定义任意图案
// 返回值：OpenGL纹理ID，可直接用于渲染
unsigned int generateChineseFloorTexture() {
    // 纹理尺寸：512x512像素（必须是2的幂次方以支持mipmap）
    const int texSize = 512;
    // 单块地砖尺寸：128x128像素（一个纹理包含4x4=16块地砖）
    const int tileSize = 128;
    // 纹理数据缓冲区：RGB格式，每像素3字节
    std::vector<unsigned char> textureData(texSize * texSize * 3);
    
    // 【配色方案】中式地砖的颜色定义
    glm::vec3 baseColor(180, 160, 140);      // 基础色：温暖的米黄/棕褐色
    glm::vec3 groutColor(120, 110, 100);     // 勾缝色：较深的灰棕色
    glm::vec3 patternColor(160, 140, 120);   // 图案色：略深的棕色
    glm::vec3 accentColor(140, 100, 80);     // 强调色：红棕色点缀
    
    // 【像素级纹理生成】遍历每个像素进行绘制
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            // 计算当前像素在一维数组中的索引（RGB每像素3字节）
            int idx = (y * texSize + x) * 3;
            
            // 计算当前像素在单块地砖内的局部坐标（0~127）
            int tileX = x % tileSize;
            int tileY = y % tileSize;
            // 计算当前像素属于第几块地砖（0~3）
            int tileIdxX = x / tileSize;
            int tileIdxY = y / tileSize;
            
            // 【勾缝线判断】地砖边缘3像素宽度为勾缝
            bool isGrout = (tileX < 3 || tileX > tileSize - 4 || 
                           tileY < 3 || tileY > tileSize - 4);
            
            glm::vec3 color;  // 当前像素的最终颜色
            
            if (isGrout) {
                // 勾缝线使用深色
                color = groutColor;
            } else {
                // 【基础色+噪点】为地砖添加细微的颜色变化，增加真实感
                // 使用伪随机噪点：基于坐标的简单哈希
                float noise = ((float)((x * 7 + y * 13) % 20) / 20.0f - 0.5f) * 15.0f;
                color = baseColor + glm::vec3(noise, noise * 0.8f, noise * 0.6f);
                
                // 【交替图案】棋盘格式交替不同的图案风格
                bool altTile = ((tileIdxX + tileIdxY) % 2 == 0);
                
                // 【内边框】中式风格的回形边框装饰
                int innerMargin = 15;  // 边框距离地砖边缘的距离
                bool isInnerBorder = (tileX >= innerMargin && tileX <= innerMargin + 4) ||
                                    (tileX >= tileSize - innerMargin - 4 && tileX <= tileSize - innerMargin) ||
                                    (tileY >= innerMargin && tileY <= innerMargin + 4) ||
                                    (tileY >= tileSize - innerMargin - 4 && tileY <= tileSize - innerMargin);
                
                if (isInnerBorder && !isGrout) {
                    color = accentColor;  // 边框用红棕色强调
    }
    
                // 【中心图案】简单的中式几何纹样
                int centerX = tileSize / 2;   // 地砖中心X坐标
                int centerY = tileSize / 2;   // 地砖中心Y坐标
                int distX = abs(tileX - centerX);  // 到中心的X距离
                int distY = abs(tileY - centerY);  // 到中心的Y距离
                
                if (altTile) {
                    // 【菱形图案】奇数地砖使用菱形装饰
                    // 菱形外轮廓：曼哈顿距离在20~25像素之间
                    if (distX + distY < 25 && distX + distY > 20) {
                        color = patternColor;
                    }
                    // 菱形内部填充：曼哈顿距离小于12像素
                    if (distX + distY < 12) {
                        color = accentColor * 1.1f;  // 略亮的强调色
                    }
                } else {
                    // 【方形图案】偶数地砖使用方形装饰
                    // 方形轮廓：X或Y距离在20~25像素之间
                    if ((distX < 25 && distX > 20 && distY < 25) ||
                        (distY < 25 && distY > 20 && distX < 25)) {
                        color = patternColor;
                    }
                    // 方形中心填充：X和Y距离都小于10像素
                    if (distX < 10 && distY < 10) {
                        color = accentColor * 1.1f;
                    }
                }
                
                // 【角落装饰】四个角落的小菱形点缀
                int cornerDist = 30;  // 角落区域范围
                bool inCorner = (tileX < cornerDist && tileY < cornerDist) ||
                               (tileX < cornerDist && tileY > tileSize - cornerDist) ||
                               (tileX > tileSize - cornerDist && tileY < cornerDist) ||
                               (tileX > tileSize - cornerDist && tileY > tileSize - cornerDist);
                
                if (inCorner && !isInnerBorder && !isGrout) {
                    // 计算角落装饰的中心点
                    int cx = (tileX < tileSize/2) ? 20 : tileSize - 20;
                    int cy = (tileY < tileSize/2) ? 20 : tileSize - 20;
                    int cd = abs(tileX - cx) + abs(tileY - cy);  // 到角落中心的曼哈顿距离
                    if (cd < 8) {
                        color = accentColor;  // 小菱形装饰
                    }
                }
            }
            
            // 【颜色截断】确保RGB值在0~255范围内
            color = glm::clamp(color, glm::vec3(0), glm::vec3(255));
            
            // 【写入纹理数据】将颜色存入缓冲区
            textureData[idx] = (unsigned char)color.r;      // R通道
            textureData[idx + 1] = (unsigned char)color.g;  // G通道
            textureData[idx + 2] = (unsigned char)color.b;  // B通道
        }
    }
    
    // ============================ 上传纹理到GPU ============================
    unsigned int textureID;
    glGenTextures(1, &textureID);              // 生成纹理对象ID
    glBindTexture(GL_TEXTURE_2D, textureID);   // 绑定为当前2D纹理
    
    // 上传纹理数据到GPU显存
    // 参数说明：
    // - GL_TEXTURE_2D: 2D纹理目标
    // - 0: mipmap级别（0为基础级别）
    // - GL_RGB: 纹理内部格式
    // - texSize, texSize: 纹理宽高
    // - 0: 边框（必须为0）
    // - GL_RGB: 源数据格式
    // - GL_UNSIGNED_BYTE: 源数据类型
    // - textureData.data(): 源数据指针
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
    
    // 自动生成多级渐远纹理（Mipmap）
    // Mipmap是预先计算的低分辨率版本，用于远距离渲染时减少锯齿
        glGenerateMipmap(GL_TEXTURE_2D);
        
    // 【纹理采样参数设置】
    // GL_REPEAT: 纹理坐标超出0~1时重复平铺（适合地板这种大面积使用）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // S轴（水平）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // T轴（垂直）
    // GL_LINEAR_MIPMAP_LINEAR: 三线性过滤，在mipmap层级间进行线性插值，画质最好
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // 缩小滤波
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // 放大滤波（双线性）
    
    return textureID;  // 返回纹理ID供后续渲染使用
}

// =====================================================================
// 【生成带圆形窗户洞的后墙顶点数据】
// =====================================================================
// 窗户是圆形的，所以需要在后墙挖一个圆形的洞
// 方法：从圆周上的每个点向外延伸射线到墙边缘，形成梯形条带
// 参数：
//   vertices - 输出的顶点数据数组
//   segments - 圆形分割的段数（越多越圆滑）
//   circleRadius - 圆形洞的半径（在单位立方体空间中）
void generateBackWallWithCircularHole(std::vector<float>& vertices, int segments, float circleRadius) {
    vertices.clear();
    
    // 墙面的Z坐标和法线
    const float wallZ = -0.5f;
    const float normalX = 0.0f, normalY = 0.0f, normalZ = -1.0f;
    
    // 墙的边界
    const float wallHalf = 0.5f;
    
    // 辅助函数：添加一个顶点（位置+法线）
    auto addVertex = [&](float x, float y) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(wallZ);
        vertices.push_back(normalX);
        vertices.push_back(normalY);
        vertices.push_back(normalZ);
    };
    
    // 辅助函数：从圆心向指定角度发射射线，计算与墙边缘的交点
    auto rayToWallEdge = [wallHalf](float angle) -> std::pair<float, float> {
        float dx = cos(angle);
        float dy = sin(angle);
        
        // 计算射线与四条边的交点，取最近的
        float t = 1e9f;
        
        // 与右边 x = wallHalf 的交点
        if (dx > 0.0001f) {
            float t_right = wallHalf / dx;
            if (t_right > 0 && t_right < t) {
                float y_at_right = dy * t_right;
                if (y_at_right >= -wallHalf && y_at_right <= wallHalf) {
                    t = t_right;
                }
            }
        }
        // 与左边 x = -wallHalf 的交点
        if (dx < -0.0001f) {
            float t_left = -wallHalf / dx;
            if (t_left > 0 && t_left < t) {
                float y_at_left = dy * t_left;
                if (y_at_left >= -wallHalf && y_at_left <= wallHalf) {
                    t = t_left;
                }
            }
        }
        // 与上边 y = wallHalf 的交点
        if (dy > 0.0001f) {
            float t_top = wallHalf / dy;
            if (t_top > 0 && t_top < t) {
                float x_at_top = dx * t_top;
                if (x_at_top >= -wallHalf && x_at_top <= wallHalf) {
                    t = t_top;
                }
            }
        }
        // 与下边 y = -wallHalf 的交点
        if (dy < -0.0001f) {
            float t_bottom = -wallHalf / dy;
            if (t_bottom > 0 && t_bottom < t) {
                float x_at_bottom = dx * t_bottom;
                if (x_at_bottom >= -wallHalf && x_at_bottom <= wallHalf) {
                    t = t_bottom;
                }
            }
        }
        
        return {dx * t, dy * t};
    };
    
    const float PI = 3.14159265f;
    float angleStep = (2.0f * PI) / segments;
    
    // 遍历圆周上的每一段，生成从圆弧到墙边缘的梯形
    for (int i = 0; i < segments; i++) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        // 圆周上的两个点
        float cx1 = circleRadius * cos(angle1);
        float cy1 = circleRadius * sin(angle1);
        float cx2 = circleRadius * cos(angle2);
        float cy2 = circleRadius * sin(angle2);
        
        // 墙边缘上的对应点（射线与墙的交点）
        std::pair<float, float> edge1 = rayToWallEdge(angle1);
        std::pair<float, float> edge2 = rayToWallEdge(angle2);
        float wx1 = edge1.first, wy1 = edge1.second;
        float wx2 = edge2.first, wy2 = edge2.second;
        
        // 生成两个三角形组成的梯形
        // 三角形1：墙边1 -> 圆边1 -> 圆边2
        addVertex(wx1, wy1);
        addVertex(cx1, cy1);
        addVertex(cx2, cy2);
        
        // 三角形2：墙边1 -> 圆边2 -> 墙边2
        addVertex(wx1, wy1);
        addVertex(cx2, cy2);
        addVertex(wx2, wy2);
    }
}

// =====================================================================
// 【全局变量】整个程序共享的状态数据
// =====================================================================

// 【相机对象】控制3D视角
// Camera类封装了视图矩阵计算、WASD移动、鼠标旋转、滚轮缩放
// DEFAULT_CAMERA_POS定义在config.h中，是相机的初始位置
Camera camera(DEFAULT_CAMERA_POS);
    
// 【光源位置】场景主光源的3D坐标（模拟太阳光或房间主灯）
glm::vec3 lightPos = DEFAULT_LIGHT_POS;

// 【房间中心位置】用于定位房间几何体
glm::vec3 cubePos = DEFAULT_CUBE_POS;

// =====================================================================
// 【主函数】程序入口点
// =====================================================================
int main()
{
    // ============================ GLFW初始化 ============================
    // GLFW是一个跨平台的窗口和输入管理库
    glfwInit();  // 初始化GLFW库
    
    // 【OpenGL版本设置】要求OpenGL 3.3
    // 这是现代OpenGL的基础版本，支持核心模式特性
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // 主版本号
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // 次版本号
    
    // 【核心模式】禁用旧版兼容特性，只使用现代OpenGL
    // 核心模式强制使用VAO/VBO/着色器，不允许立即模式(glBegin/glEnd)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    // macOS需要额外设置前向兼容性
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // ============================ 创建窗口 ============================
    // 创建一个SCR_WIDTH x SCR_HEIGHT大小的窗口
    // 窗口标题为"SimpleScene - Chinese Style Room"
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SimpleScene - Chinese Style Room", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // 将此窗口的OpenGL上下文设为当前线程的活动上下文
    glfwMakeContextCurrent(window);
    
    // ============================ 注册回调函数 ============================
    // 回调函数是事件驱动编程的核心，当特定事件发生时GLFW会调用这些函数
    
    // 将相机指针传递给输入模块（用于处理键盘/鼠标输入）
    setCamera(&camera);
    
    // 窗口大小改变回调：调整视口尺寸
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // 鼠标移动回调：控制相机旋转（第一人称视角）
    glfwSetCursorPosCallback(window, mouse_callback);
    // 滚轮滚动回调：控制相机缩放（FOV）
    glfwSetScrollCallback(window, scroll_callback);
    // 鼠标按键回调：处理点击选取3D对象
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // 【鼠标模式】使用普通模式（非捕获），允许用户点击场景中的对象
    // GLFW_CURSOR_DISABLED会隐藏鼠标并锁定到窗口中心（FPS游戏常用）
    // GLFW_CURSOR_NORMAL显示正常鼠标光标（适合本项目的点击交互）
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // ============================ GLAD初始化 ============================
    // GLAD是OpenGL函数指针加载库
    // OpenGL的函数地址在运行时由显卡驱动提供，需要动态获取
    // gladLoadGLLoader使用GLFW提供的地址获取函数来加载所有OpenGL函数
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // ============================ OpenGL全局状态配置 ============================
    // 【深度测试】启用Z缓冲，确保近处物体遮挡远处物体
    // 没有深度测试，后绘制的物体会覆盖先绘制的，不管远近
    glEnable(GL_DEPTH_TEST);
    
    // 【混合（透明度）】启用Alpha混合，支持半透明渲染
    glEnable(GL_BLEND);
    // 混合公式：finalColor = srcColor * srcAlpha + dstColor * (1 - srcAlpha)
    // 这是最常用的透明度混合方式
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // =====================================================================
    // 【着色器加载】编译和链接GLSL着色器程序
    // =====================================================================
    // 着色器是运行在GPU上的小程序，用于计算顶点位置和像素颜色
    // .vs = Vertex Shader（顶点着色器）：处理每个顶点的位置变换
    // .fs = Fragment Shader（片段着色器）：计算每个像素的颜色
    
    // 【主光照着色器】Phong光照模型，用于渲染大部分物体
    // 支持环境光、漫反射、镜面反射三种光照分量
    Shader lightingShader("lighting.vs", "lighting.fs");
    
    // 【光源立方体着色器】纯色渲染，不受光照影响（自发光）
    // 用于渲染光源本身、雨雪粒子、闪电等
    Shader lightCubeShader("lightcube.vs", "lightcube.fs");
    
    // 【地形着色器】带高度渐变的着色器
    // 根据顶点高度混合不同颜色（低处绿色→高处棕色）
    Shader terrainShader("terrain.vs", "terrain.fs");
    
    // 【台灯圆柱着色器】圆柱UV映射
    // 用于没有UV坐标的台灯灯罩，自动计算圆柱形纹理坐标
    Shader lampShader("cyl.vs", "cyl.fs");
    
    // 【灵珠发光着色器】特殊发光效果
    // 模拟内发光、边缘光晕的魔法球体效果
    Shader orbShader("orb.vs", "orb.fs");
    
    // 【粒子着色器】点精灵渲染
    // 用于渲染灵珠周围的漂浮粒子
    Shader particleShader("particle.vs", "particle.fs");
    
    // 【天空背景着色器】流动蓝天白云
    // 用于在窗户外渲染动态的天空效果，增强窗户通透感
    Shader skyShader("sky.vs", "sky.fs");

    // =====================================================================
    // 【3D模型加载】从OBJ文件加载网格数据
    // =====================================================================
    // OBJ是一种通用的3D模型格式，包含顶点位置、法线、纹理坐标和面索引
    // MTL是材质文件，定义颜色、纹理贴图等材质属性
    
    // -------------------- 桌子模型 --------------------
    Mesh tableMesh;           // 网格对象，包含顶点数据、VAO/VBO/EBO
    bool tableLoaded = false; // 加载成功标志
    if (loadOBJ("obj/table3.obj", tableMesh.vertices, tableMesh.indices)) {
        // setupMesh创建VAO/VBO/EBO并上传顶点数据到GPU
        setupMesh(tableMesh);
        tableLoaded = true;
        
        // 手动为桌子加载木纹纹理（OBJ文件中未指定）
        Texture woodTexture;
        woodTexture.id = loadTexture("obj/wood.jpg");  // 加载纹理到GPU
        woodTexture.type = "texture_diffuse";          // 漫反射纹理类型
        woodTexture.path = "obj/wood.jpg";
        tableMesh.textures.push_back(woodTexture);
    }

    // -------------------- 台灯模型（带材质） --------------------
    // 使用loadOBJWithMaterials可以同时加载MTL材质文件
    std::vector<Mesh> lampMeshes;  // 一个模型可能有多个材质组，每组一个Mesh
    bool lampLoaded = false;
    if (loadOBJWithMaterials("obj/lamp1.obj", lampMeshes, "obj")) {
        lampLoaded = true;
    }
    
    // 【调试辅助函数】打印模型的包围盒尺寸
    // Lambda表达式定义内联函数，用于调试时查看模型大小
    auto printModelBounds = [](const std::vector<Mesh>& meshes, const char* name) {
        if (meshes.empty()) return;
        glm::vec3 minPos(1e9f), maxPos(-1e9f);  // 初始化为极值
        // 遍历所有网格的所有顶点，找出最小和最大坐标
        for (const auto& mesh : meshes) {
            for (const auto& v : mesh.vertices) {
                minPos = glm::min(minPos, v.Position);  // 分量取最小
                maxPos = glm::max(maxPos, v.Position);  // 分量取最大
    }
        }
        glm::vec3 size = maxPos - minPos;  // 计算包围盒尺寸
        std::cout << name << " bounds: min(" << minPos.x << "," << minPos.y << "," << minPos.z 
                  << ") max(" << maxPos.x << "," << maxPos.y << "," << maxPos.z 
                  << ") size(" << size.x << "," << size.y << "," << size.z << ")" << std::endl;
    };
    
    // -------------------- 花瓶模型（可交互对象-诱饵） --------------------
    // 花瓶是陷阱诱饵，点击会触发箭矢陷阱
    std::vector<Mesh> vaseMeshes;
    bool vaseLoaded = false;
    // 第三个参数是材质文件的查找目录，必须与OBJ文件中的mtllib路径匹配
    if (loadOBJWithMaterials("obj/vase_obj/vase.obj", vaseMeshes, "obj/vase_obj")) {
        vaseLoaded = true;
        std::cout << "Vase model loaded: " << vaseMeshes.size() << " meshes" << std::endl;
        printModelBounds(vaseMeshes, "Vase");
    } else {
        std::cerr << "Failed to load vase model!" << std::endl;
    }
    
    // -------------------- 书籍模型（可交互对象-可移动） --------------------
    // 书籍可以点击移动，但不是解谜的关键
    std::vector<Mesh> bookMeshes;
    bool bookLoaded = false;
    if (loadOBJWithMaterials("obj/book_obj/book.obj", bookMeshes, "obj/book_obj")) {
        bookLoaded = true;
        std::cout << "Book model loaded: " << bookMeshes.size() << " meshes" << std::endl;
        printModelBounds(bookMeshes, "Book");
    } else {
        std::cerr << "Failed to load book model!" << std::endl;
    }
    
    // -------------------- 卷轴模型（可交互对象-机关） --------------------
    // 卷轴是正确的机关触发物，点击会打开地砖暗格
    std::vector<Mesh> scrollMeshes;
    bool scrollLoaded = false;
    if (loadOBJWithMaterials("obj/scroll_obj/scroll.obj", scrollMeshes, "obj/scroll_obj")) {
        scrollLoaded = true;
        std::cout << "Scroll model loaded: " << scrollMeshes.size() << " meshes" << std::endl;
        printModelBounds(scrollMeshes, "Scroll");
    } else {
        std::cerr << "Failed to load scroll model!" << std::endl;
    }
    
    // -------------------- 书柜模型（陷阱机关） --------------------
    // 书柜放置在墙边，触发陷阱后会移动，露出墙面暗格
    std::vector<Mesh> bookcaseMeshes;
    bool bookcaseLoaded = false;
    if (loadOBJWithMaterials("obj/bookcase_obj/bookcase1.obj", bookcaseMeshes, "obj/bookcase_obj")) {
        bookcaseLoaded = true;
        std::cout << "Bookcase model loaded: " << bookcaseMeshes.size() << " meshes" << std::endl;
        printModelBounds(bookcaseMeshes, "Bookcase");
    } else {
        std::cerr << "Failed to load bookcase model!" << std::endl;
    }
    
    // -------------------- 台灯Y坐标范围计算 --------------------
    // 用于圆柱投影着色器，需要知道模型的高度范围来计算V纹理坐标
    float lampYMin = 1e9f, lampYMax = -1e9f;
    if (lampLoaded) {
        for (const auto& mesh : lampMeshes) {
            for (const auto& vertex : mesh.vertices) {
                if (vertex.Position.y < lampYMin) lampYMin = vertex.Position.y;
                if (vertex.Position.y > lampYMax) lampYMax = vertex.Position.y;
            }
        }
    }
    
    // =====================================================================
    // 【纹理加载】
    // =====================================================================
    // 窗户纹理：PNG格式支持透明度（Alpha通道）
    unsigned int windowTexture = loadTexture("window.png");

    // 中式地砖纹理：程序化生成（见上方generateChineseFloorTexture函数）
    unsigned int floorTexture = generateChineseFloorTexture();

    // =====================================================================
    // 【VAO/VBO设置】房间几何体的顶点数据配置
    // =====================================================================
    // OpenGL现代渲染流程的核心概念：
    // - VAO (Vertex Array Object): 顶点数组对象，存储顶点属性配置状态
    // - VBO (Vertex Buffer Object): 顶点缓冲对象，在GPU中存储顶点数据
    // - EBO (Element Buffer Object): 元素缓冲对象，存储索引数据用于索引绘制
    //
    // 渲染流程：
    // 1. 创建VAO/VBO
    // 2. 上传顶点数据到VBO
    // 3. 配置顶点属性指针（告诉OpenGL如何解释顶点数据）
    // 4. 渲染时绑定VAO即可，所有配置自动应用
    
    // -------------------- 从完整立方体中提取各面数据 --------------------
    // CUBE_VERTICES定义了一个完整立方体的36个顶点（6面×2三角形×3顶点）
    // 每个顶点6个float：位置(x,y,z) + 法线(nx,ny,nz)
    // 这里提取出需要的面单独渲染（房间只需要部分面）
    float CeilingVertices[36], LWallVertices[36], RWallVertices[36];
    std::copy(CUBE_VERTICES + 180, CUBE_VERTICES + 216, CeilingVertices);  // 顶面（天花板）
    std::copy(CUBE_VERTICES + 72, CUBE_VERTICES + 108, LWallVertices);     // 左侧面（左墙）
    std::copy(CUBE_VERTICES + 108, CUBE_VERTICES + 144, RWallVertices);    // 右侧面（右墙）
    // 后墙（前墙）使用带窗户洞的顶点数据 BACK_WALL_WITH_HOLE，不再从CUBE_VERTICES提取

    // -------------------- 天花板 VAO/VBO --------------------
    unsigned int VBO1, CeilingVAO;
    glGenVertexArrays(1, &CeilingVAO);  // 生成VAO
    glGenBuffers(1, &VBO1);             // 生成VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO1);  // 绑定VBO到GL_ARRAY_BUFFER目标
    // 上传顶点数据到GPU，GL_STATIC_DRAW表示数据不会频繁修改
        glBufferData(GL_ARRAY_BUFFER, sizeof(CeilingVertices), CeilingVertices, GL_STATIC_DRAW);
    glBindVertexArray(CeilingVAO);  // 绑定VAO，之后的属性配置都会记录在这个VAO中
    // 配置位置属性（location=0）：每顶点6个float，从偏移0开始，取前3个
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);  // 启用位置属性
    // 配置法线属性（location=1）：每顶点6个float，从偏移12字节（3个float）开始，取3个
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);  // 启用法线属性

    // -------------------- 地板 VAO/VBO（带纹理坐标） --------------------
    // 地板需要纹理坐标来贴中式地砖纹理
    // 每顶点8个float：位置(3) + 法线(3) + 纹理坐标UV(2)
    unsigned int VBO2, FloorVAO;
        glGenVertexArrays(1, &FloorVAO);
        glGenBuffers(1, &VBO2);
        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FLOOR_VERTICES_TEXTURED), FLOOR_VERTICES_TEXTURED, GL_STATIC_DRAW);
        glBindVertexArray(FloorVAO);
    // 位置属性（location=0）：stride=8个float
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    // 法线属性（location=1）：偏移3个float
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    // 纹理坐标属性（location=2）：偏移6个float，2个分量(U,V)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

    // -------------------- 左墙 VAO/VBO --------------------
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

    // -------------------- 右墙 VAO/VBO --------------------
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

    // -------------------- 前墙（带圆形窗户洞） VAO/VBO --------------------
    // 动态生成带圆形洞的后墙顶点数据，完美匹配圆形窗户
    // 窗户顶点范围是-0.4到0.4，经过0.6缩放后是-0.24到0.24
    // 但从纹理图像看，圆形窗框几乎占满整个纹理，所以半径约为0.24
    std::vector<float> backWallVertices;
    const int circleSegments = 64;  // 圆形分割段数，越大越圆滑
    const float windowCircleRadius = 0.24f;  // 圆形窗户的半径
    generateBackWallWithCircularHole(backWallVertices, circleSegments, windowCircleRadius);
    int backWallVertexCount = static_cast<int>(backWallVertices.size() / 6);  // 每顶点6个float
    
    unsigned int VBO5, FWallVAO;
        glGenVertexArrays(1, &FWallVAO);
        glGenBuffers(1, &VBO5);
        glBindBuffer(GL_ARRAY_BUFFER, VBO5);
    glBufferData(GL_ARRAY_BUFFER, backWallVertices.size() * sizeof(float), backWallVertices.data(), GL_STATIC_DRAW);
        glBindVertexArray(FWallVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    // -------------------- 光源立方体 VAO/VBO --------------------
    // 用于渲染光源本身的可视化表示（一个小白色方块）
    // 也复用于渲染其他临时几何体（如暗格的黑色方块）
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

    // -------------------- 书桌 VAO/VBO --------------------
    // 备用简易书桌模型（如果OBJ加载失败时使用）
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

    // -------------------- 窗户 VAO/VBO/EBO --------------------
    // 窗户使用索引绘制（EBO），4个顶点+6个索引组成2个三角形
    // 索引绘制可以复用顶点，减少数据量
    unsigned int VBO8, WindowVAO, WindowEBO;
        glGenVertexArrays(1, &WindowVAO);
        glGenBuffers(1, &VBO8);
    glGenBuffers(1, &WindowEBO);  // 创建元素缓冲对象
        glBindVertexArray(WindowVAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO8);
    glBufferData(GL_ARRAY_BUFFER, sizeof(WINDOW_VERTICES), WINDOW_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WindowEBO);  // 绑定EBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(WINDOW_INDICES), WINDOW_INDICES, GL_STATIC_DRAW);
    // 窗户顶点也有纹理坐标（8个float）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

    // -------------------- 地台（沙盘底座） VAO/VBO --------------------
    // 桌上沙盘的木质底座平台
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

    // -------------------- 天空背景平面 VAO/VBO --------------------
    // 放置在窗户后方的天空平面，用于渲染流动的蓝天白云
    unsigned int VBO10, SkyVAO;
    glGenVertexArrays(1, &SkyVAO);
    glGenBuffers(1, &VBO10);
    glBindBuffer(GL_ARRAY_BUFFER, VBO10);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKY_PLANE_VERTICES), SKY_PLANE_VERTICES, GL_STATIC_DRAW);
    glBindVertexArray(SkyVAO);
    // 位置属性（location=0）：stride=8个float
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性（location=1）：偏移3个float
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性（location=2）：偏移6个float
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // =====================================================================
    // 【地形和积雪网格生成】
    // =====================================================================
    // 地形网格：程序化生成的山丘地形，放在桌上沙盘中
    Mesh terrainMesh;
    // generateTerrain使用Perlin噪声或类似算法生成高度起伏的网格
    // 参数：网格尺寸、X/Z缩放比例、高度缩放
    generateTerrain(terrainMesh.vertices, terrainMesh.indices, 
                    TERRAIN_GRID_SIZE, TERRAIN_GRID_SIZE, 
                    TERRAIN_SCALE_X, TERRAIN_SCALE_Z, TERRAIN_HEIGHT_SCALE);
    setupMesh(terrainMesh);  // 上传到GPU
    
    // 积雪网格：与地形形状相同，但可以动态更新高度（积雪效果）
    // 积雪覆盖在地形上方，高度随下雪而增加
    Mesh snowMesh;
    snowMesh.vertices = terrainMesh.vertices;  // 复制顶点数据
    snowMesh.indices = terrainMesh.indices;
    setupMesh(snowMesh);

    // =====================================================================
    // 【子系统初始化】各个功能模块的初始化
    // =====================================================================
    
    // 天气系统：初始化云层位置、雨雪粒子数组、闪电数据
    initWeatherSystem();

    // 台灯光源系统：初始化灯光参数
    initLampLight();

    // 解谜游戏系统初始化
    initInteractiveObjects();  // 可交互对象管理
    initPuzzleGame();          // 解谜状态机（地砖翻转、暗格打开）
    initSpiritOrb();           // 灵珠系统（发光效果、粒子）
    initArrowTrap();           // 箭矢陷阱系统
    initMeshCollision();       // Mesh碰撞检测系统
    
    // =====================================================================
    // 【注册碰撞Mesh】用于箭矢精确碰撞检测
    // =====================================================================
    // 注册台灯碰撞mesh
    if (lampLoaded) {
        glm::mat4 lampModelMatrix = glm::mat4(1.0f);
        lampModelMatrix = glm::translate(lampModelMatrix, LAMP_POSITION);
        lampModelMatrix = glm::rotate(lampModelMatrix, glm::radians(LAMP_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));
        lampModelMatrix = glm::scale(lampModelMatrix, glm::vec3(LAMP_SCALE));
        
        for (size_t i = 0; i < lampMeshes.size(); i++) {
            std::string meshName = "Lamp_" + std::to_string(i);
            registerCollisionMesh(meshName, lampMeshes[i], lampModelMatrix);
        }
    }
    
    // 注册花瓶碰撞mesh
    if (vaseLoaded) {
        glm::mat4 vaseModelMatrix = glm::mat4(1.0f);
        vaseModelMatrix = glm::translate(vaseModelMatrix, VASE_POSITION);
        vaseModelMatrix = glm::rotate(vaseModelMatrix, glm::radians(VASE_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));
        vaseModelMatrix = glm::scale(vaseModelMatrix, glm::vec3(VASE_SCALE));
        
        for (size_t i = 0; i < vaseMeshes.size(); i++) {
            std::string meshName = "Vase_" + std::to_string(i);
            registerCollisionMesh(meshName, vaseMeshes[i], vaseModelMatrix);
        }
    }
    
    // 注册桌子碰撞mesh
    if (tableLoaded) {
        glm::mat4 tableModelMatrix = glm::mat4(1.0f);
        tableModelMatrix = glm::translate(tableModelMatrix, TABLE_POSITION);
        tableModelMatrix = glm::rotate(tableModelMatrix, glm::radians(TABLE_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));
        tableModelMatrix = glm::scale(tableModelMatrix, glm::vec3(TABLE_SCALE));
        
        registerCollisionMesh("Table", tableMesh, tableModelMatrix);
    }

    // =====================================================================
    // 【添加可交互对象】花瓶、书籍、卷轴
    // =====================================================================
    
    // -------------------- 花瓶（诱饵DECOY） --------------------
    // 点击花瓶会触发陷阱：墙面暗格打开，箭矢射出
    int vaseIdx = addInteractiveObject("Vase", ObjectType::DECOY, VASE_POSITION, 0.08f);
    g_interactiveObjects[vaseIdx].scale = glm::vec3(VASE_SCALE);
    g_interactiveObjects[vaseIdx].baseColor = glm::vec3(0.9f, 0.9f, 0.95f);      // 青花瓷色（蓝白）
    g_interactiveObjects[vaseIdx].highlightColor = glm::vec3(1.0f, 0.5f, 0.3f);  // 触发时变橙色（警告色）
    
    // -------------------- 书籍（可移动MOVABLE） --------------------
    // 点击可以滑动，但不触发任何机关（干扰项）
    int bookIdx = addInteractiveObject("Book", ObjectType::MOVABLE, BOOK_POSITION, 0.08f);
    g_interactiveObjects[bookIdx].scale = glm::vec3(BOOK_SCALE);
    g_interactiveObjects[bookIdx].baseColor = glm::vec3(0.6f, 0.4f, 0.3f);       // 棕色皮革
    g_interactiveObjects[bookIdx].highlightColor = glm::vec3(1.0f, 0.9f, 0.5f);  // 高亮黄色
    
    // -------------------- 卷轴（机关MECHANISM） --------------------
    // 正确的解谜触发物，点击会打开地砖暗格，露出灵珠
    int scrollIdx = addInteractiveObject("Scroll", ObjectType::MECHANISM, SCROLL_POSITION, 0.1f);
    g_interactiveObjects[scrollIdx].scale = glm::vec3(SCROLL_SCALE);
    g_interactiveObjects[scrollIdx].baseColor = glm::vec3(0.85f, 0.75f, 0.6f);   // 竹简/羊皮纸色
    g_interactiveObjects[scrollIdx].highlightColor = glm::vec3(1.0f, 0.8f, 0.3f); // 高亮金色
    
    // =====================================================================
    // 【生成解谜游戏网格】
    // =====================================================================
    Mesh floorTileMesh, compartmentMesh, orbMesh, glowQuadMesh, rayMesh;
    
    // 地砖网格：可以翻转打开的地砖
    generateFloorTileMesh(floorTileMesh);
    setupMesh(floorTileMesh);
    
    // 暗格网格：地砖下方的暗格（黑色凹槽）
    generateCompartmentMesh(compartmentMesh);
    setupMesh(compartmentMesh);
    
    // 灵珠网格：球体网格
    generateOrbMesh(orbMesh);
    setupMesh(orbMesh);
    
    // 光晕四边形：用于渲染灵珠光斑效果
    generateGlowQuadMesh(glowQuadMesh);
    setupMesh(glowQuadMesh);
    
    // 光线网格：从灵珠发出的光线
    generateRayMesh(rayMesh);
    setupMesh(rayMesh);
    
    // =====================================================================
    // 【生成箭矢陷阱网格】
    // =====================================================================
    Mesh arrowMesh, compartmentDoorMesh;
    generateArrowMesh(arrowMesh);           // 箭矢模型
    generateCompartmentDoorMesh(compartmentDoorMesh);  // 暗格门

    // =====================================================================
    // 【云层球体位置】
    // =====================================================================
    // 云由多个重叠的球体组成，模拟蓬松的外观
    std::vector<glm::vec3> cloudSpheres;
    for (int i = 0; i < CLOUD_SPHERE_COUNT; i++) {
        cloudSpheres.push_back(glm::vec3(CLOUD_SPHERE_OFFSETS[i][0], 
                                          CLOUD_SPHERE_OFFSETS[i][1], 
                                          CLOUD_SPHERE_OFFSETS[i][2]));
    }

    // =====================================================================
    // 【渲染循环】程序的核心循环，每帧执行一次
    // =====================================================================
    // 游戏/图形应用的基本循环结构：
    // 1. 处理输入
    // 2. 更新状态
    // 3. 渲染画面
    // 4. 显示结果
    // 循环直到用户关闭窗口
    while (!glfwWindowShouldClose(window))
    {
        // ==================== 时间管理 ====================
        // deltaTime是两帧之间的时间间隔，用于使动画和移动速度独立于帧率
        // 例如：移动距离 = 速度 × deltaTime，这样无论60fps还是30fps，移动速度一致
        float currentFrame = static_cast<float>(glfwGetTime());  // 获取程序启动后的秒数
        deltaTime = currentFrame - lastFrame;  // 计算帧间隔
        lastFrame = currentFrame;

        // ==================== 输入处理 ====================
        // 处理键盘输入：WASD移动、空格上升、Ctrl下降、L开关灯等
        processInput(window, camera);
        
        // ==================== 状态更新 ====================
        // 更新天气系统：云层移动、雨雪粒子物理、闪电触发
        // 参数terrainMesh.vertices用于计算雪的积累高度
        updateWeather(deltaTime, terrainMesh.vertices);
        
        // 更新解谜游戏各子系统
        updateInteractiveObjects(deltaTime);  // 可交互对象动画（滑动）
        updatePuzzleGame(deltaTime);          // 解谜状态机（地砖翻转动画）
        updateArrowTrap(deltaTime);           // 箭矢陷阱（书柜移动、箭矢飞行）
        if (shouldRenderOrb()) {
            // 只有在灵珠应该显示时才更新其效果
            updateSpiritOrb(deltaTime, getOrbPosition());
        }

        // ==================== 清屏 ====================
        // 设置背景颜色（灰色）
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        // 清除颜色缓冲和深度缓冲
        // 每帧开始都要清空上一帧的内容，否则会出现重影
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 重置深度写入和混合设置
        glDepthMask(GL_TRUE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // ==================== 变换矩阵计算 ====================
        // 3D渲染的核心是三个变换矩阵：Model、View、Projection
        
        // 【投影矩阵】定义3D→2D的透视投影
        // - FOV: camera.Zoom（视野角度，影响缩放感）
        // - 宽高比: SCR_WIDTH / SCR_HEIGHT
        // - 近平面: 0.1（比这更近的物体不渲染）
        // - 远平面: 100.0（比这更远的物体不渲染）
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                                 (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                                 0.1f, 100.0f);
        
        // 【视图矩阵】定义相机位置和朝向
        // 将世界坐标转换为相机坐标（以相机为原点的坐标系）
        glm::mat4 view = camera.GetViewMatrix();
        
        // 【模型矩阵】定义物体的位置、旋转、缩放
        // 初始化为单位矩阵，每个物体会设置自己的模型矩阵
        glm::mat4 model = glm::mat4(1.0f);

        // ==================== 射线拾取准备 ====================
        // 保存矩阵供射线拾取模块使用（鼠标点击选取物体）
        setViewProjectionMatrices(view, projection);
        
        // 【鼠标悬停检测】判断鼠标当前指向哪个物体
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);  // 获取鼠标位置
        int windowW, windowH;
        glfwGetWindowSize(window, &windowW, &windowH);  // 获取窗口尺寸
        // pickObject将2D鼠标坐标转换为3D射线，检测与哪个物体相交
        g_hoveredObjectIndex = pickObject(static_cast<float>(mouseX), static_cast<float>(mouseY),
                                          windowW, windowH, view, projection, camera.Position);

        // 获取台灯光源的世界坐标（用于光照计算）
        glm::vec3 lampLightPos = getLampLightPosition();

        // =====================================================================
        // 【渲染房间】墙壁、天花板、地板
        // =====================================================================
        // 使用主光照着色器（Phong光照模型）
        lightingShader.use();
        
        // 设置着色器的uniform变量
        // uniform是从CPU传递给着色器的全局变量，对所有顶点/片段相同
        lightingShader.setMat4("projection", projection);  // 投影矩阵
        lightingShader.setMat4("view", view);              // 视图矩阵
        lightingShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);  // 主光源颜色（偏亮白色）
        lightingShader.setVec3("lightPos", lightPos);      // 主光源位置
        lightingShader.setVec3("viewPos", camera.Position); // 相机位置（用于镜面反射计算）
        lightingShader.setBool("hasTexture", false);       // 默认不使用纹理
        lightingShader.setFloat("alpha", 1.0f);            // 完全不透明
        
        // 【台灯光源参数】作为第二光源影响场景
        lightingShader.setBool("lampOn", isLampOn());              // 灯是否开启
        lightingShader.setVec3("lampLightPos", lampLightPos);      // 灯光位置
        lightingShader.setVec3("lampLightColor", lampLightColor);  // 灯光颜色（暖黄色）
        lightingShader.setFloat("lampLightIntensity", lampLightIntensity);  // 光照强度
        lightingShader.setFloat("lampLightRadius", lampLightRadius);        // 光照半径
        
        // 【灵珠光源参数】灵珠发光时作为第三光源
        glm::vec3 orbLightPos = getOrbPosition();
        lightingShader.setBool("orbEmitting", isOrbEmittingLight());
        lightingShader.setVec3("orbLightPos", orbLightPos);
        lightingShader.setVec3("orbLightColor", ORB_COLOR_CORE);
        lightingShader.setFloat("orbLightIntensity", getOrbLightIntensity());
        lightingShader.setFloat("orbLightRadius", ORB_LIGHT_RADIUS);

        // 【房间基础变换】缩放一个单位立方体到房间尺寸
        // 先平移到房间中心位置，再缩放到目标尺寸
        glm::mat4 roomModel = glm::translate(glm::mat4(1.0f), cubePos);
        roomModel = glm::scale(roomModel, glm::vec3(ROOM_SCALE_X, ROOM_SCALE_Y, ROOM_SCALE_Z));

        // -------------------- 天花板渲染 --------------------
        lightingShader.setVec3("objectColor", 0.8f, 0.7f, 0.6f);  // 米黄色
            lightingShader.setMat4("model", roomModel);
        glBindVertexArray(CeilingVAO);                // 绑定天花板VAO
        glDrawArrays(GL_TRIANGLES, 0, 6);             // 绘制6个顶点（2个三角形）

        // -------------------- 地板渲染（带中式地砖纹理） --------------------
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // 白色（让纹理颜色正常显示）
        if (floorTexture != 0) {
            // 绑定纹理到纹理单元0
            glActiveTexture(GL_TEXTURE0);                // 激活纹理单元0
            glBindTexture(GL_TEXTURE_2D, floorTexture);  // 绑定地砖纹理
            lightingShader.setInt("texture1", 0);        // 告诉着色器纹理在单元0
            lightingShader.setBool("hasTexture", true);  // 启用纹理采样
        }
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(FloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        lightingShader.setBool("hasTexture", false);  // 恢复无纹理状态

        // -------------------- 地板暗格洞口渲染 --------------------
        // 当地砖打开时，在地板上渲染一个黑色方块模拟洞口
        if (g_secretTile.isOpen || g_secretTile.isAnimating) {
            lightingShader.setVec3("objectColor", 0.02f, 0.01f, 0.01f);  // 几乎纯黑（洞口）
            lightingShader.setBool("hasTexture", false);
            
            // 计算洞口位置和尺寸
            // lightCubeVAO是一个中心在原点的单位立方体(-0.5到0.5)
            glm::mat4 holeModel = glm::mat4(1.0f);
            float halfW = COMPARTMENT_SIZE_CONFIG.x * 0.5f;
            float halfD = COMPARTMENT_SIZE_CONFIG.z * 0.5f;
            holeModel = glm::translate(holeModel, glm::vec3(
                g_secretTile.position.x + halfW,  // 洞口中心X
                FLOOR_HEIGHT + 0.002f,            // 略高于地板（避免Z-fighting闪烁）
                g_secretTile.position.z + halfD   // 洞口中心Z
            ));
            holeModel = glm::scale(holeModel, glm::vec3(
                COMPARTMENT_SIZE_CONFIG.x + 0.005f,  // 略宽于实际尺寸
                0.003f,                              // 非常薄
                COMPARTMENT_SIZE_CONFIG.z + 0.005f
            ));
            lightingShader.setMat4("model", holeModel);
            
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);  // 绘制立方体（36个顶点）
        }

        // -------------------- 左墙渲染 --------------------
        lightingShader.setVec3("objectColor", 0.6f, 0.3f, 0.2f);  // 暗红棕色（中式墙面）
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(LWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        // -------------------- 右墙渲染 --------------------
        lightingShader.setVec3("objectColor", 0.7f, 0.4f, 0.3f);  // 红棕色
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(RWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        // -------------------- 前墙渲染（带圆形窗户洞） --------------------
        // 前墙使用动态生成的带圆形洞的顶点数据，完美匹配圆形窗户
        lightingShader.setVec3("objectColor", 0.9f, 0.85f, 0.8f);  // 浅米色
            lightingShader.setMat4("model", roomModel);
            glBindVertexArray(FWallVAO);
        glDrawArrays(GL_TRIANGLES, 0, backWallVertexCount);  // 使用动态计算的顶点数量

        // =====================================================================
        // 【渲染窗外天空】流动的蓝天白云背景
        // =====================================================================
        // 天空平面放置在窗户后方，通过圆形窗户洞口可以看到流动的云
        // 先渲染天空，再渲染窗户，这样窗框可以正确遮挡天空边缘
        {
            skyShader.use();
            skyShader.setMat4("projection", projection);
            skyShader.setMat4("view", view);
            skyShader.setFloat("time", currentFrame);  // 传递时间让云流动
            
            // 天空平面变换：放置在窗户正后方，平行于后墙
            // 计算后墙的世界坐标Z位置
            float backWallZ = cubePos.z - 0.5f * ROOM_SCALE_Z;  // 后墙Z = 0.2 - 0.6 = -0.4
            
            // 天空平面位置：
            // X = 房间中心X (窗户在房间中心)
            // Y = 房间中心Y (窗户在墙的中心)
            // Z = 后墙Z - 0.1 (在后墙后面一点点)
            glm::vec3 skyPosition(cubePos.x, cubePos.y, backWallZ - 0.1f);
            
            // 天空平面大小：需要完全覆盖圆形窗户洞口
            // 窗户经过0.6缩放，还要考虑房间缩放(ROOM_SCALE_X=1.8, ROOM_SCALE_Y=1.0)
            // 窗户宽度 = 0.48 * 1.8 ≈ 0.86，高度 = 0.48 * 1.0 = 0.48
            // 天空平面需要比窗户更大以留有余量
            float skySizeX = 1.0f * ROOM_SCALE_X;  // 宽度考虑房间X缩放
            float skySizeY = 1.0f * ROOM_SCALE_Y;  // 高度考虑房间Y缩放
            
            glm::mat4 skyModel = glm::mat4(1.0f);
            skyModel = glm::translate(skyModel, skyPosition);
            skyModel = glm::scale(skyModel, glm::vec3(skySizeX, skySizeY, 1.0f));
            skyShader.setMat4("model", skyModel);
            
            glBindVertexArray(SkyVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);  // 6个顶点组成的四边形
        }
        
        // =====================================================================
        // 【渲染窗户】带Alpha透明度的纹理
        // =====================================================================
        // 窗户纹理(window.png)包含Alpha通道，窗格是透明的，窗框是不透明的
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        lightingShader.use();  // 切换回光照着色器
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("alpha", 1.0f);  // 使用纹理自身的Alpha
        
        // 窗户变换：先放置到房间位置，再移动到后墙，最后缩放
        model = glm::translate(glm::mat4(1.0f), cubePos);
        model = glm::scale(model, glm::vec3(ROOM_SCALE_X, ROOM_SCALE_Y, ROOM_SCALE_Z));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.499f));  // 贴在后墙上（略偏前避免穿模）
        model = glm::scale(model, glm::vec3(0.6f));  // 缩放到适当大小
            lightingShader.setMat4("model", model);
        
            if (windowTexture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, windowTexture);
                lightingShader.setInt("texture1", 0);
                lightingShader.setBool("hasTexture", true);
            }
            glBindVertexArray(WindowVAO);
        // 使用索引绘制：glDrawElements而不是glDrawArrays
        // 6个索引 = 2个三角形 = 1个四边形窗户
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glDisable(GL_BLEND);
        lightingShader.setFloat("alpha", 1.0f);

        // =====================================================================
        // 【渲染桌子】靠墙放置的中式书桌
        // =====================================================================
        lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("alpha", 1.0f);
        
        // 桌子的模型变换：平移→旋转→缩放
        // 变换顺序很重要，从右向左读：先缩放，再旋转，最后平移
            model = glm::mat4(1.0f);
        model = glm::translate(model, TABLE_POSITION);  // 移动到桌子位置
        model = glm::rotate(model, glm::radians(TABLE_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));  // Y轴旋转
        model = glm::scale(model, glm::vec3(TABLE_SCALE));  // 统一缩放
            lightingShader.setMat4("model", model);

        if (tableLoaded && !tableMesh.vertices.empty()) {
            // 使用加载的OBJ模型
                if (!tableMesh.textures.empty() && tableMesh.textures[0].id != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, tableMesh.textures[0].id);
                    lightingShader.setInt("texture1", 0);
                    lightingShader.setBool("hasTexture", true);
                } else {
                    lightingShader.setBool("hasTexture", false);
                }
                glBindVertexArray(tableMesh.VAO);
            // 索引绘制：使用indices数组中的索引
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tableMesh.indices.size()), GL_UNSIGNED_INT, 0);
        } else {
            // 【备用方案】如果OBJ加载失败，使用预定义的简单桌子模型
            lightingShader.setVec3("objectColor", 0.3f, 0.15f, 0.05f);  // 深棕色木纹
            lightingShader.setBool("hasTexture", false);
            glBindVertexArray(DeskVAO);
            glDrawArrays(GL_TRIANGLES, 0, 120);  // 120个顶点
        }

        // =====================================================================
        // 【渲染台灯】使用圆柱投影着色器
        // =====================================================================
        // 台灯OBJ模型没有UV坐标，使用圆柱投影算法自动计算纹理坐标
        // 圆柱投影：将顶点的XZ坐标转换为角度，作为U坐标；Y坐标作为V坐标
        if (lampLoaded && !lampMeshes.empty()) {
            lampShader.use();  // 切换到台灯专用着色器
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);
            
            // 光照参数
            lampShader.setVec3("lightPos", lightPos);
            lampShader.setVec3("viewPos", camera.Position);
            lampShader.setVec3("lightColor", glm::vec3(1.5f, 1.5f, 1.5f));
            lampShader.setVec3("objectColor", glm::vec3(0.9f, 0.85f, 0.8f));  // 淡米色灯罩
            lampShader.setBool("hasTexture", true);
            
            // 【自发光效果】当灯打开时，灯罩本身也会发光
            lampShader.setBool("lampOn", isLampOn());
            lampShader.setVec3("lampEmissionColor", lampLightColor);
            lampShader.setFloat("lampEmissionIntensity", isLampOn() ? lampLightIntensity : 0.0f);
            
            // 【圆柱映射参数】定义圆柱的Y坐标范围（灯罩部分）
            lampShader.setBool("useCylindricalMapping", true);
            lampShader.setFloat("cylYMin", -0.85f);  // 灯罩底部Y
            lampShader.setFloat("cylYMax", 1.38f);   // 灯罩顶部Y
            
            // 台灯模型变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, LAMP_POSITION);
            model = glm::rotate(model, glm::radians(LAMP_ROTATION), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(LAMP_SCALE));
            lampShader.setMat4("model", model);
            
            // 渲染台灯的每个材质组（一个模型可能有多种材质）
            for (const auto& mesh : lampMeshes) {
                if (!mesh.textures.empty() && mesh.textures[0].id != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
                    lampShader.setInt("texture_diffuse", 0);
                    lampShader.setBool("hasTexture", true);
                } else {
                    lampShader.setBool("hasTexture", false);
                    // Use material diffuse color when no texture
                    if (mesh.hasDiffuseColor) {
                        lampShader.setVec3("objectColor", mesh.diffuseColor);
                    }
                }
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
            }
        }

        // =====================================================================
        // 【渲染地形沙盘】桌上的微型山水景观
        // =====================================================================
            lightingShader.use();
            
        // -------------------- 沙盘底座渲染 --------------------
        // 一个木质平台，放置地形的基座
        lightingShader.setVec3("objectColor", 0.5f, 0.4f, 0.3f);  // 棕色木质
            lightingShader.setBool("hasTexture", false);
            lightingShader.setFloat("alpha", 1.0f);
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);
        // 放置在桌面上方（Y=0.54是桌面高度）
        model = glm::translate(glm::mat4(1.0f), glm::vec3(SANDBOX_CENTER_X, 0.54f, SANDBOX_CENTER_Z));
            lightingShader.setMat4("model", model);
            glBindVertexArray(PlatformVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        // -------------------- 地形渲染 --------------------
        // 使用地形专用着色器（根据高度渐变颜色）
            terrainShader.use();
            terrainShader.setVec3("lightColor", 1.5f, 1.5f, 1.5f);
            terrainShader.setVec3("lightPos", lightPos);
            terrainShader.setVec3("viewPos", camera.Position);
            terrainShader.setFloat("alpha", 1.0f);
            terrainShader.setMat4("projection", projection);
            terrainShader.setMat4("view", view);
            
        // 地形也受台灯光照影响
            terrainShader.setBool("lampOn", isLampOn());
            terrainShader.setVec3("lampLightPos", lampLightPos);
            terrainShader.setVec3("lampLightColor", lampLightColor);
            terrainShader.setFloat("lampLightIntensity", lampLightIntensity);
            terrainShader.setFloat("lampLightRadius", lampLightRadius);
            
        // 地形也受灵珠光照影响
            terrainShader.setBool("orbEmitting", isOrbEmittingLight());
            terrainShader.setVec3("orbLightPos", orbLightPos);
            terrainShader.setVec3("orbLightColor", ORB_COLOR_CORE);
            terrainShader.setFloat("orbLightIntensity", getOrbLightIntensity());
            terrainShader.setFloat("orbLightRadius", ORB_LIGHT_RADIUS);
            
        // 地形放置在底座上方（Y=0.57）
        model = glm::translate(glm::mat4(1.0f), glm::vec3(SANDBOX_CENTER_X, 0.57f, SANDBOX_CENTER_Z));
            terrainShader.setMat4("model", model);
            glBindVertexArray(terrainMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(terrainMesh.indices.size()), GL_UNSIGNED_INT, 0);
            
        // -------------------- 积雪层渲染 --------------------
        // 下雪时会有积雪覆盖在地形表面
        // 积雪高度存储在snowHeightMap数组中
            bool hasSnow = false;
            for (float height : snowHeightMap) {
            if (height > 0.001f) { hasSnow = true; break; }
                }
            if (hasSnow) {
            // 更新积雪网格的顶点高度
                int topVertexCount = (TERRAIN_GRID_SIZE + 1) * (TERRAIN_GRID_SIZE + 1);
                for (int i = 0; i < topVertexCount; i++) {
                // 积雪高度 = 地形高度 + 积雪深度
                    snowMesh.vertices[i].Position.y = terrainMesh.vertices[i].Position.y + snowHeightMap[i];
                }
            // 更新GPU中的顶点数据（使用glBufferSubData而不是重新创建）
                glBindBuffer(GL_ARRAY_BUFFER, snowMesh.VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, snowMesh.vertices.size() * sizeof(Vertex), &snowMesh.vertices[0]);
                
            // 渲染白色半透明积雪
                lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // 纯白色
                lightingShader.setBool("hasTexture", false);
            lightingShader.setFloat("alpha", 0.95f);  // 略微透明
                lightingShader.setMat4("projection", projection);
                lightingShader.setMat4("view", view);
                lightingShader.setMat4("model", model);
                glBindVertexArray(snowMesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(snowMesh.indices.size()), GL_UNSIGNED_INT, 0);
            lightingShader.setFloat("alpha", 1.0f);
        }

        // =====================================================================
        // 【渲染天气效果】云层、雨、雪、闪电
        // =====================================================================
        
        // -------------------- 云层渲染 --------------------
        // 云由多个重叠的椭球体组成，模拟蓬松的外观
        if (cloudVisible) {
            // 云是半透明的，禁用深度写入避免遮挡后面的云球
            glDepthMask(GL_FALSE);
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.9f, 0.9f, 0.95f);  // 浅灰白色
            lightingShader.setFloat("alpha", 0.5f);  // 半透明
            lightingShader.setBool("hasTexture", false);

            // 渲染组成云的每个球体
            for (size_t i = 0; i < cloudSpheres.size(); i++) {
                model = glm::mat4(1.0f);
                // cloudPosition是云的基础位置，cloudSpheres[i]是相对偏移
                model = glm::translate(model, cloudPosition + cloudSpheres[i]);
                // 中心球体更大，边缘球体更小
                float scale = (i == 0) ? 0.09f : 0.06f;
                // 非均匀缩放：扁平的椭球形状
                model = glm::scale(model, glm::vec3(scale, scale * 0.6f, scale * 0.75f));
                lightingShader.setMat4("model", model);
                glBindVertexArray(lightCubeVAO);  // 复用立方体VAO
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            glDepthMask(GL_TRUE);  // 恢复深度写入
        }

        // -------------------- 雨粒子渲染 --------------------
        // 每个雨滴是一个拉长的小立方体
        if (isRaining) {
            lightCubeShader.use();  // 使用自发光着色器（不受光照影响）
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            lightCubeShader.setVec3("lightColor", 0.7f, 0.75f, 0.9f);  // 淡蓝色水滴

            for (const auto& particle : rainParticles) {
                if (particle.active) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, particle.position);
                    // 雨滴形状：细长的竖条
                    model = glm::scale(model, glm::vec3(0.005f, 0.015f, 0.005f));
                    lightCubeShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // -------------------- 雪粒子渲染 --------------------
        // 雪花是旋转的小立方体，比雨滴更大
        if (isSnowing && cloudVisible) {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            lightCubeShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);  // 纯白色雪花

            for (const auto& particle : snowParticles) {
                if (particle.active) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, particle.position);
                    // 雪花飘落时缓慢旋转（基于生命时间）
                    model = glm::rotate(model, particle.life * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.025f));  // 均匀缩放
                    lightCubeShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }

        // -------------------- 闪电渲染 --------------------
        // 闪电由多段折线组成，每段是一个拉伸旋转的立方体
        glDisable(GL_DEPTH_TEST);  // 闪电总是可见，不被遮挡
        for (const auto& lightning : lightnings) {
            if (lightning.active) {
                lightCubeShader.use();
                lightCubeShader.setMat4("projection", projection);
                lightCubeShader.setMat4("view", view);
                
                // 闪电亮度随生命周期闪烁
                float brightness = lightning.life / lightning.maxLife;
                if (brightness < 0.5f) brightness = 1.0f;  // 闪烁效果
                // 蓝白色高亮，强度超过1.0产生HDR效果
                lightCubeShader.setVec3("lightColor", 3.0f * brightness, 3.2f * brightness, 4.0f * brightness);

                // 渲染闪电的每一段
                for (size_t i = 0; i < lightning.segments.size() - 1; i++) {
                    glm::vec3 segStart = lightning.segments[i];
                    glm::vec3 segEnd = lightning.segments[i + 1];
                    glm::vec3 segCenter = (segStart + segEnd) * 0.5f;  // 线段中点
                    glm::vec3 segDir = glm::normalize(segEnd - segStart);  // 线段方向
                    float segLength = glm::length(segEnd - segStart);  // 线段长度

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, segCenter);
                    
                    // 计算旋转：将Y轴对齐到线段方向
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                    if (abs(glm::dot(segDir, up)) < 0.999f) {
                        glm::vec3 rotAxis = glm::normalize(glm::cross(up, segDir));
                        float rotAngle = acos(glm::dot(up, segDir));
                        model = glm::rotate(model, rotAngle, rotAxis);
                    }
                    
                    // 缩放：细长的柱体
                    model = glm::scale(model, glm::vec3(0.015f, segLength, 0.015f));
                    lightCubeShader.setMat4("model", model);
                    glBindVertexArray(lightCubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }
        glEnable(GL_DEPTH_TEST);  // 恢复深度测试

        // =====================================================================
        // 【渲染解谜游戏元素】可交互对象
        // =====================================================================
        
        // -------------------- 渲染可交互对象（花瓶、书籍、卷轴） --------------------
        // 这些对象可以被鼠标点击选中，触发不同的游戏效果
        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setFloat("alpha", 1.0f);
        
        for (size_t i = 0; i < g_interactiveObjects.size(); i++) {
            const InteractiveObject& obj = g_interactiveObjects[i];
            
            // 【颜色选择】根据对象状态决定颜色
            // - 普通状态：使用baseColor
            // - 被选中：使用highlightColor
            // - 鼠标悬停：使用baseColor和highlightColor的混合
            glm::vec3 color = obj.baseColor;
            if (obj.isSelected) {
                color = obj.highlightColor;  // 点击选中时高亮
            } else if (static_cast<int>(i) == g_hoveredObjectIndex) {
                // glm::mix线性插值：result = a * (1-t) + b * t
                color = glm::mix(obj.baseColor, obj.highlightColor, 0.5f);
            }
            lightingShader.setVec3("objectColor", color);
            
            // 获取对象的模型变换矩阵（包含动画位移）
            glm::mat4 objModel = getObjectModelMatrix(static_cast<int>(i));
            lightingShader.setMat4("model", objModel);
            
            // 根据对象类型渲染对应的3D模型
            if (i == 0 && vaseLoaded && !vaseMeshes.empty()) {
                // ==================== 花瓶渲染 ====================
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
                // ==================== 书籍渲染 ====================
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
                // ==================== 卷轴渲染 ====================
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
                // 【备用渲染】如果模型加载失败，使用简单立方体
                lightingShader.setBool("hasTexture", false);
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
            }
        
        // =====================================================================
        // 【渲染书柜】陷阱机关的一部分，会移动露出墙面暗格
        // =====================================================================
        if (bookcaseLoaded && !bookcaseMeshes.empty()) {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setFloat("alpha", 1.0f);
            
            // getBookcaseMatrix()返回书柜的动态变换矩阵
            // 当陷阱触发时，书柜会滑动，这个矩阵会包含动画位移
            model = getBookcaseMatrix();
            lightingShader.setMat4("model", model);
            
            for (const auto& mesh : bookcaseMeshes) {
                if (!mesh.textures.empty() && mesh.textures[0].id != 0) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.textures[0].id);
                    lightingShader.setInt("texture1", 0);
                    lightingShader.setBool("hasTexture", true);
                    lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // White to show texture colors
                } else {
                    lightingShader.setBool("hasTexture", false);
                    // Use material diffuse color when no texture (e.g., bookcase handles)
                    if (mesh.hasDiffuseColor) {
                        lightingShader.setVec3("objectColor", mesh.diffuseColor);
                    } else {
                        lightingShader.setVec3("objectColor", 0.6f, 0.4f, 0.3f);  // Default wood color
                    }
                }
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
            }
        }
        
        // =====================================================================
        // 【渲染墙面暗格】书柜移开后露出的墙洞
        // =====================================================================
        // 暗格是陷阱机关的一部分，里面会射出箭矢
        if (isCompartmentVisible()) {
            lightingShader.use();
            lightingShader.setBool("hasTexture", false);
            
            glm::vec3 compartmentPos = g_wallCompartment.position;   // 暗格位置
            glm::vec3 compartmentSize = g_wallCompartment.size;      // 暗格尺寸
            float panelProgress = g_wallCompartment.panelSlideProgress;  // 面板滑动进度(0~1)
            
            // -------------------- 渲染黑色洞口 --------------------
            // 暗格深处是纯黑的（模拟深邃的洞穴）
            lightingShader.setVec3("objectColor", 0.01f, 0.005f, 0.005f);  // 几乎纯黑
            model = glm::mat4(1.0f);
            // 洞口延伸进入墙壁（+X方向）
            model = glm::translate(model, compartmentPos + glm::vec3(0.08f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(
                0.15f,                      // 深度（进入墙壁的距离）
                compartmentSize.y,          // 高度
                compartmentSize.z           // 宽度
            ));
            lightingShader.setMat4("model", model);
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            
            // -------------------- 渲染滑动面板 --------------------
            // 两块面板从中心向上下滑开，露出暗格
            if (panelProgress < 1.0f) {
                // 面板颜色与右墙相同（伪装效果）
                lightingShader.setVec3("objectColor", 0.7f, 0.4f, 0.3f);
                
                float halfHeight = compartmentSize.y * 0.5f;
                float slideOffset = panelProgress * halfHeight;  // 已滑动的距离
                float panelX = -0.02f;       // 面板在墙面前方
                float panelThickness = 0.015f;  // 面板厚度
                
                // 上面板：向上滑动
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
                
                // 下面板：向下滑动
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
        // 【渲染箭矢】从墙面暗格射出的陷阱箭矢
        // =====================================================================
        if (!g_arrows.empty()) {
            lightingShader.use();
            lightingShader.setBool("hasTexture", false);
            
            for (const auto& arrow : g_arrows) {
                if (!arrow.active) continue;  // 跳过非活动箭矢
                
                // 根据箭矢来源设置颜色
                if (arrow.sourceType == 0) {
                    lightingShader.setVec3("objectColor", 0.6f, 0.4f, 0.2f);  // 棕色（从墙壁来）
    } else {
                    lightingShader.setVec3("objectColor", 0.3f, 0.3f, 0.35f); // 深灰色（从窗户来）
                }
                
                // 碰撞后的箭矢淡出
                if (arrow.hasCollided) {
                    lightingShader.setFloat("alpha", 0.5f);  // 半透明
            } else {
                    lightingShader.setFloat("alpha", 1.0f);
                }
                
                // getArrowMatrix计算箭矢的变换矩阵（位置+朝向）
                model = getArrowMatrix(arrow);
                lightingShader.setMat4("model", model);
                glBindVertexArray(arrowMesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(arrowMesh.indices.size()), GL_UNSIGNED_INT, 0);
            }
            lightingShader.setFloat("alpha", 1.0f);
        }
        
        // =====================================================================
        // 【渲染地板暗格】地砖下方的秘密空间，藏有灵珠
        // =====================================================================
        if (g_compartment.isVisible || g_secretTile.isOpen || g_secretTile.isAnimating) {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 0.08f, 0.05f, 0.03f);  // 深棕色/黑色内部
            lightingShader.setBool("hasTexture", false);
            
            // 暗格位于地砖下方
            glm::mat4 compModel = glm::mat4(1.0f);
            compModel = glm::translate(compModel, g_compartment.position);
            lightingShader.setMat4("model", compModel);
            
            glBindVertexArray(compartmentMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(compartmentMesh.indices.size()), GL_UNSIGNED_INT, 0);
        }
        
        // =====================================================================
        // 【渲染可翻转地砖】解谜机关的一部分
        // =====================================================================
        // 地砖始终渲染，关闭时作为地板的一部分，打开时显示翻转动画
        {
            lightingShader.use();
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);  // 白色显示纹理
            
            // 地砖使用与地板相同的中式地砖纹理
            if (floorTexture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, floorTexture);
                lightingShader.setInt("texture1", 0);
                lightingShader.setBool("hasTexture", true);
            } else {
                lightingShader.setBool("hasTexture", false);
            }
            
            // getFloorTileMatrix()返回地砖的变换矩阵
            // 包含翻转动画（绕一边旋转）
            glm::mat4 tileModel = getFloorTileMatrix();
            lightingShader.setMat4("model", tileModel);
            
            glBindVertexArray(floorTileMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(floorTileMesh.indices.size()), GL_UNSIGNED_INT, 0);
            lightingShader.setBool("hasTexture", false);
        }
        
        // =====================================================================
        // 【渲染灵珠】带发光效果的魔法球体
        // =====================================================================
        // 灵珠是解谜的最终目标，触发机关后从暗格中出现
        if (shouldRenderOrb()) {
            // 【加性混合】使发光效果更亮
            // 加性混合公式：finalColor = srcColor * srcAlpha + dstColor
            // 这会让发光物体看起来真的在发光，而不是覆盖背景
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // 加性混合
            
            // -------------------- 灵珠本体渲染 --------------------
            orbShader.use();  // 使用灵珠专用着色器（内发光+边缘光晕）
            orbShader.setMat4("projection", projection);
            orbShader.setMat4("view", view);
            orbShader.setMat4("model", getOrbModelMatrix());
            orbShader.setVec3("viewPos", camera.Position);  // 用于计算边缘光
            orbShader.setVec3("coreColor", g_spiritOrb.coreColor);    // 核心颜色
            orbShader.setVec3("glowColor", g_spiritOrb.glowColor);    // 光晕颜色
            orbShader.setFloat("glowIntensity", getOrbGlowIntensity());  // 光晕强度
            orbShader.setFloat("time", static_cast<float>(glfwGetTime()));  // 动画时间
            
            glBindVertexArray(orbMesh.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(orbMesh.indices.size()), GL_UNSIGNED_INT, 0);
            
            // -------------------- 光线渲染 --------------------
            // 从灵珠发出的光线射向四周
            lightingShader.use();
            lightingShader.setBool("hasTexture", false);
            const auto& rays = getOrbRays();
            for (const auto& ray : rays) {
                glm::vec3 rayColor = g_spiritOrb.rayColor * ray.intensity;
                lightingShader.setVec3("objectColor", rayColor);
                lightingShader.setFloat("alpha", ray.intensity * 0.7f);
                lightingShader.setMat4("model", getRayModelMatrix(ray));
                glBindVertexArray(rayMesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(rayMesh.indices.size()), GL_UNSIGNED_INT, 0);
            }
            lightingShader.setFloat("alpha", 1.0f);
            
            // -------------------- 光斑渲染 --------------------
            // 光线投射到墙壁和地板上形成的光斑
            const auto& spots = getOrbSpots();
            for (const auto& spot : spots) {
                if (spot.intensity > 0.1f) {
                    glm::vec3 spotColor = g_spiritOrb.spotColor * spot.intensity;
                    lightingShader.setVec3("objectColor", spotColor);
                    lightingShader.setFloat("alpha", spot.intensity * 0.6f);
                    lightingShader.setMat4("model", getSpotModelMatrix(spot));
                    // 使用四边形网格渲染圆形光斑
                    glBindVertexArray(glowQuadMesh.VAO);
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(glowQuadMesh.indices.size()), GL_UNSIGNED_INT, 0);
                }
            }
            lightingShader.setFloat("alpha", 1.0f);
            
            // -------------------- 粒子渲染 --------------------
            // 灵珠周围漂浮的发光粒子
            particleShader.use();  // 粒子着色器
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
            
            // 恢复标准混合模式
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        // =====================================================================
        // 【游戏完成状态】
        // =====================================================================
        if (getGameState() == GameState::GAME_COMPLETE) {
            // 胜利！灵珠已被收集
            // 这里可以添加庆祝效果或UI提示
        }

        // =====================================================================
        // 【渲染光源立方体】场景主光源的可视化表示
        // =====================================================================
        // 一个小白色立方体，标示光源位置
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
        lightCubeShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);  // 白色
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.1f));  // 缩小为0.1单位大小
            lightCubeShader.setMat4("model", model);
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        // =====================================================================
        // 【帧结束】交换缓冲区并处理事件
        // =====================================================================
        // 双缓冲机制：渲染到后台缓冲区，完成后交换到前台显示
        // 这避免了画面撕裂和闪烁
        glfwSwapBuffers(window);
        // 处理所有等待的窗口事件（键盘、鼠标、窗口调整等）
        glfwPollEvents();
    }  // ============ 渲染循环结束 ============

    // =====================================================================
    // 【资源清理】释放所有分配的GPU和CPU资源
    // =====================================================================
    // 良好的编程习惯：程序退出前释放所有资源
    // 虽然操作系统会在进程结束时回收所有资源，但显式释放有助于：
    // 1. 发现资源泄漏
    // 2. 保持代码整洁
    // 3. 在长时间运行的应用中重要
    
    // -------------------- 删除房间几何体的VAO --------------------
    // glDeleteVertexArrays释放顶点数组对象
    glDeleteVertexArrays(1, &CeilingVAO);
    glDeleteVertexArrays(1, &FloorVAO);
    glDeleteVertexArrays(1, &RWallVAO);
    glDeleteVertexArrays(1, &LWallVAO);
    glDeleteVertexArrays(1, &FWallVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &DeskVAO);
    glDeleteVertexArrays(1, &WindowVAO);
    glDeleteVertexArrays(1, &PlatformVAO);
    glDeleteVertexArrays(1, &SkyVAO);  // 天空背景VAO
    
    // -------------------- 删除VBO --------------------
    // glDeleteBuffers释放顶点缓冲对象（GPU显存）
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &VBO3);
    glDeleteBuffers(1, &VBO4);
    glDeleteBuffers(1, &VBO5);
    glDeleteBuffers(1, &VBO6);
    glDeleteBuffers(1, &VBO7);
    glDeleteBuffers(1, &VBO8);
    glDeleteBuffers(1, &VBO9);
    glDeleteBuffers(1, &VBO10);  // 天空背景VBO
    glDeleteBuffers(1, &WindowEBO);  // 也要删除EBO
    
    // -------------------- 清理加载的模型网格 --------------------
    // cleanupMesh会删除网格的VAO、VBO、EBO，并清空顶点数据
    cleanupMesh(terrainMesh);
    cleanupMesh(snowMesh);
    cleanupMesh(tableMesh);
    
    // 清理台灯（可能有多个材质组）
    for (auto& mesh : lampMeshes) {
        cleanupMesh(mesh);
                }
    // 清理花瓶
    for (auto& mesh : vaseMeshes) {
        cleanupMesh(mesh);
    }
    // 清理书籍
    for (auto& mesh : bookMeshes) {
        cleanupMesh(mesh);
    }
    // 清理卷轴
    for (auto& mesh : scrollMeshes) {
        cleanupMesh(mesh);
    }
    // 清理书柜
    for (auto& mesh : bookcaseMeshes) {
        cleanupMesh(mesh);
    }

    // -------------------- 清理各子系统 --------------------
    cleanupWeatherSystem();       // 清理天气系统（粒子数组等）
    cleanupInteractiveObjects();  // 清理可交互对象
    cleanupPuzzleGame();          // 清理解谜游戏状态
    cleanupSpiritOrb();           // 清理灵珠系统
    cleanupArrowTrap();           // 清理箭矢陷阱系统
    cleanupMeshCollision();       // 清理Mesh碰撞检测系统
    
    // -------------------- 清理解谜游戏网格 --------------------
    cleanupMesh(floorTileMesh);
    cleanupMesh(compartmentMesh);
    cleanupMesh(orbMesh);
    cleanupMesh(arrowMesh);
    cleanupMesh(compartmentDoorMesh);
    cleanupMesh(glowQuadMesh);
    cleanupMesh(rayMesh);

    // -------------------- 终止GLFW --------------------
    // 释放GLFW分配的所有资源，销毁所有窗口
    glfwTerminate();
    
    return 0;  // 程序正常退出
}
