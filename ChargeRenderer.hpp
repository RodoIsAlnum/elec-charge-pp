#pragma once
#include <glad/glad.h>
#include <vector>
#include <GLFW/glfw3.h>

#include "ElectricField.hpp"

class ChargeRenderer {
public:
    ChargeRenderer(TextRender* textRenderer, GLFWwindow* window, int segments = 32);
    ~ChargeRenderer();
    
    void draw(const std::vector<ElectricCharge>& charges, GLuint shaderProgram);

private:
    TextRender* textRenderer;
    GLFWwindow* window;
    GLuint VAO, VBO, EBO;
    int vertexCount;
    void setupCircle(int segments);
};