#pragma once

#include <string>
#include <vector>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "TextRender.hpp"

// Define a menu item structure
struct MenuItem {
    std::string text;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec3 normalColor;
    glm::vec3 hoverColor;
    std::function<void()> callback;
    bool isHovered;
};

class Menu {
public:
    Menu(TextRender* textRenderer, GLFWwindow* window);
    ~Menu();
    
    // Add a new menu item
    void addItem(const std::string& text, float x, float y, float scale, 
                const glm::vec3& normalColor, const glm::vec3& hoverColor, 
                std::function<void()> callback);
    
    // Process mouse movement (for hover effects)
    void processMouseMovement(double xpos, double ypos);
    
    // Process mouse clicks
    void processMouseClick(int button, int action);

    // Process key press
    void processKeyPress(int button, int action);

    // Switch options
    void switchOptionDown(int button, int action);
    void switchOptionUp(int button, int action);
    
    // Render the menu
    void render();
    
    // Set menu visibility
    void setVisible(bool visible);
    
    // Check if menu is visible
    bool isVisible() const;
    std::vector<MenuItem> items;
    
private:
    TextRender* textRenderer;
    GLFWwindow* window;
    bool visible;
    double lastMouseX, lastMouseY;
    
    // Check if a point is inside a menu item
    bool isPointInItem(double x, double y, const MenuItem& item) const;
};