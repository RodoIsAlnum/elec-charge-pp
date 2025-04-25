#include <iostream>
#include <stack>

#include "Menu.hpp"

Menu::Menu(TextRender* textRenderer, GLFWwindow* window)
    : textRenderer(textRenderer), window(window), visible(false), lastMouseX(0), lastMouseY(0) {
}

Menu::~Menu() {
    // Nothing to clean up as the TextRender is owned elsewhere
}

void Menu::addItem(const std::string& text, float x, float y, float scale, 
                  const glm::vec3& normalColor, const glm::vec3& hoverColor, 
                  std::function<void()> callback) {
    // Estimate the size based on text length
    // This is approximate and would be better with actual text measurement
    float width = text.length() * 15.0f * scale;  // Approximation
    float height = 24.0f * scale;  // Approximation based on standard font height
    
    MenuItem item = {
        text,
        glm::vec2(x, y),
        glm::vec2(width, height),
        normalColor,
        hoverColor,
        callback,
        false
    };
    
    items.push_back(item);
}

bool Menu::isPointInItem(double x, double y, const MenuItem& item) const {
    // Get window height for coordinate conversion (OpenGL has origin at bottom-left)
    int windowHeight;
    glfwGetWindowSize(window, nullptr, &windowHeight);
    
    // Convert mouse y-coordinate (top-left origin) to OpenGL y-coordinate (bottom-left origin)
    double glY = windowHeight - y;
    
    return (x >= item.position.x && 
            x <= item.position.x + item.size.x && 
            glY >= item.position.y && 
            glY <= item.position.y + item.size.y);
}

void Menu::processMouseMovement(double xpos, double ypos) {
    if (!visible) return;
    
    lastMouseX = xpos;
    lastMouseY = ypos;
    
    // Check if mouse is over any menu item
    for (auto& item : items) {
        item.isHovered = isPointInItem(xpos, ypos, item);
    }
}

void Menu::processMouseClick(int button, int action) {
    if (!visible || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    // Check if any item was clicked
    for (auto& item : items) {
        if (item.isHovered) {
            // Execute the callback
            if (item.callback) {
                item.callback();
            }
            break;
        }
    }
}

void Menu::processKeyPress(int button, int action) {
    if (!visible || button != GLFW_KEY_ENTER || action != GLFW_PRESS) return;

    // Check if any item was clicked
    for (auto& item : items) {
        if (item.isHovered) {
            // Execute the callback
            if (item.callback) {
                item.callback();
            }
            break;
        }
    }
}

void Menu::switchOptionDown(int button, int action) {
    if (!visible || button != GLFW_KEY_DOWN || action != GLFW_PRESS) return;
    // Check if any item was selected
    //std::cout << "Down key pressed" << std::endl;
    int currentIndex = -1;
    for (size_t i = 0; i < items.size(); i++)
    {
        if (items[i].isHovered) {currentIndex = i;
        break;}
    }
    if (currentIndex == -1) items[0].isHovered = true;
    int newIndex = (currentIndex + 1) % items.size();
    items[currentIndex].isHovered = false;
    items[newIndex].isHovered = true; 
}

void Menu::switchOptionUp(int button, int action) {
    if (!visible || button != GLFW_KEY_UP || action != GLFW_PRESS) return;
    // Check if any item was selected
    //std::cout << "Up key pressed" << std::endl;
    int currentIndex = -1;
    for (size_t i = 0; i < items.size(); i++)
    {
        if (items[i].isHovered) {currentIndex = i;
        break;}
    }
    if (currentIndex == -1) items[items.size() -1 ].isHovered = true;
    int newIndex = (currentIndex - 1 + items.size()) % items.size();
    items[currentIndex].isHovered = false;
    items[newIndex].isHovered = true; 
}


void Menu::render() {
    if (!visible) return;
    
    for (const auto& item : items) {
        glm::vec3 color = item.isHovered ? item.hoverColor : item.normalColor;
        
        // Calculate scale based on original estimation
        float scale = item.size.y / 24.0f;
        
        // Render the menu item text
        textRenderer->renderText(item.text, item.position.x, item.position.y, scale, color);
    }
}

void Menu::setVisible(bool visible) {
    this->visible = visible;
}

bool Menu::isVisible() const {
    return visible;
}
