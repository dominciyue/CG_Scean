#include "input.h"
#include "weather.h"
#include "config.h"
#include "lamp_light.h"
#include "ray_picking.h"
#include "interactive_object.h"
#include "puzzle_game.h"
#include "arrow_trap.h"

// Must include glad before GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

// =====================================================================
// Global Variable Definitions
// =====================================================================
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool keyMPressed = false;
bool keyRPressed = false;
bool keySPressed = false;

// Mouse click state
bool mouseLeftClicked = false;
double mouseClickX = 0.0;
double mouseClickY = 0.0;

// Mouse drag state for object interaction
bool mouseDragging = false;
double dragStartX = 0.0;
double dragStartY = 0.0;

// Actual window size (updated on resize)
int actualWindowWidth = SCR_WIDTH;
int actualWindowHeight = SCR_HEIGHT;

// Matrices for ray picking
glm::mat4 g_viewMatrix = glm::mat4(1.0f);
glm::mat4 g_projectionMatrix = glm::mat4(1.0f);

// Camera pointer (for callback functions)
static Camera* g_camera = nullptr;

void setCamera(Camera* cam) {
    g_camera = cam;
}

void setViewProjectionMatrices(const glm::mat4& view, const glm::mat4& projection) {
    g_viewMatrix = view;
    g_projectionMatrix = projection;
}

// =====================================================================
// Callback Functions
// =====================================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    // Update actual window size for ray picking
    actualWindowWidth = width;
    actualWindowHeight = height;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Handle object rotation when dragging with left button
    if (mouseDragging && g_selectedObjectIndex >= 0) {
        // Rotate selected object based on mouse movement
        rotateSelectedObject(xoffset * 0.5f, yoffset * 0.5f);
    } else if (g_camera && !mouseDragging) {
        // Normal camera control when not dragging object
        // Only process camera if right mouse button is held (optional)
        int rightButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        if (rightButton == GLFW_PRESS) {
            g_camera->ProcessMouseMovement(xoffset, yoffset);
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_camera) {
        g_camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Get current cursor position
            glfwGetCursorPos(window, &mouseClickX, &mouseClickY);
            mouseLeftClicked = true;
            dragStartX = mouseClickX;
            dragStartY = mouseClickY;
            
            if (g_camera) {
                // Spirit orb click - do nothing (orb should stay visible)
                // Players can admire the orb but it won't disappear
                
                // Check if wall compartment door was clicked (arrow trap)
                if (checkCompartmentClick(
                    static_cast<float>(mouseClickX),
                    static_cast<float>(mouseClickY),
                    actualWindowWidth, actualWindowHeight,
                    g_viewMatrix, g_projectionMatrix,
                    g_camera->Position
                )) {
                    triggerArrowTrap();
                    return;
                }
                
                // Check if lamp shade (cylinder) was clicked
                bool lampHit = checkLampClick(
                    static_cast<float>(mouseClickX), 
                    static_cast<float>(mouseClickY),
                    actualWindowWidth, actualWindowHeight,
                    g_viewMatrix, g_projectionMatrix,
                    g_camera->Position
                );
                
                if (lampHit) {
                    toggleLamp();
                    return;
                }
                
                // Check if an interactive object was clicked
                int pickedIdx = pickObject(
                    static_cast<float>(mouseClickX),
                    static_cast<float>(mouseClickY),
                    actualWindowWidth, actualWindowHeight,
                    g_viewMatrix, g_projectionMatrix,
                    g_camera->Position
                );
                
                if (pickedIdx >= 0) {
                    // Click triggers automatic animation (no drag needed)
                    triggerObjectAnimation(pickedIdx);
                    selectObject(pickedIdx);  // Visual feedback
                } else {
                    deselectAll();
                }
            }
        } else if (action == GLFW_RELEASE) {
            mouseDragging = false;
        }
    }
}

void clearMouseClick() {
    mouseLeftClicked = false;
}

bool consumeMouseClick(double& outX, double& outY) {
    if (mouseLeftClicked) {
        outX = mouseClickX;
        outY = mouseClickY;
        mouseLeftClicked = false;
        return true;
    }
    return false;
}

// =====================================================================
// Input Processing Functions
// =====================================================================

void processInput(GLFWwindow* window, Camera& camera) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // M key toggles cloud display and control mode
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        if (!keyMPressed) {
            toggleCloud();
            keyMPressed = true;
        }
    } else {
        keyMPressed = false;
    }

    // R key toggles rain
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!keyRPressed) {
            toggleRain();
            keyRPressed = true;
        }
    } else {
        keyRPressed = false;
    }

    // S key toggles snow (only in cloud control mode)
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (!keySPressed && cloudControlMode) {
            toggleSnow();
            keySPressed = true;
        }
    } else {
        keySPressed = false;
    }

    // WAXD controls (cloud control mode or camera control mode)
    if (cloudControlMode) {
        // Cloud control mode
        glm::vec3 direction(0.0f);
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            direction.z += 1.0f;  // Forward
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            direction.z -= 1.0f;  // Backward (X key)
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            direction.x -= 1.0f;  // Left
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            direction.x += 1.0f;  // Right
        
        if (glm::length(direction) > 0.0f) {
            moveCloud(direction, deltaTime);
        }
    } else {
        // Camera control mode
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);  // Backward (X key)
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}
