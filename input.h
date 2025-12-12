#pragma once
#ifndef INPUT_H
#define INPUT_H

#include "camera.h"
#include <glm/glm.hpp>

// Forward declaration of GLFWwindow to avoid header conflicts
struct GLFWwindow;

// =====================================================================
// Input State (Global Variable Declarations)
// =====================================================================
extern float deltaTime;
extern float lastFrame;

extern float lastX;
extern float lastY;
extern bool firstMouse;

// Key states (prevent repeated triggers on hold)
extern bool keyMPressed;
extern bool keyRPressed;
extern bool keySPressed;

// Mouse click state
extern bool mouseLeftClicked;       // True when left button just clicked
extern double mouseClickX;          // X position of click
extern double mouseClickY;          // Y position of click

// Actual window size (updated on resize)
extern int actualWindowWidth;
extern int actualWindowHeight;

// =====================================================================
// Callback Functions
// =====================================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// =====================================================================
// Input Processing Functions
// =====================================================================

void processInput(GLFWwindow* window, Camera& camera);
void setCamera(Camera* cam);

// Clear mouse click state (call after handling click)
void clearMouseClick();

// Check and consume mouse click
bool consumeMouseClick(double& outX, double& outY);

// =====================================================================
// Matrices for ray picking (set each frame)
// =====================================================================
extern glm::mat4 g_viewMatrix;
extern glm::mat4 g_projectionMatrix;

void setViewProjectionMatrices(const glm::mat4& view, const glm::mat4& projection);

#endif // INPUT_H
