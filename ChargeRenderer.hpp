#pragma once
#include <glad/glad.h>
#include <vector>

#include "ElectricField.hpp"

class ChargeRenderer {
public:
    ChargeRenderer(int segments = 32);
    ~ChargeRenderer();
    
    void draw(const std::vector<ElectricCharge>& charges, GLuint shaderProgram);

private:
    GLuint VAO, VBO, EBO;
    int vertexCount;
    void setupCircle(int segments);
};