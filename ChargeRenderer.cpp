#include "ChargeRenderer.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>

ChargeRenderer::ChargeRenderer(int segments) {
    setupCircle(segments);
}

ChargeRenderer::~ChargeRenderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void ChargeRenderer::setupCircle(int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Center vertex
    vertices.push_back(0.0f);  // x
    vertices.push_back(0.0f);  // y
    vertices.push_back(0.0f);  // z
    
    // Outer vertices
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float y = sin(angle);
        
        vertices.push_back(x);  // x
        vertices.push_back(y);  // y
        vertices.push_back(0.0f);  // z
    }
    
    // Indices for triangles
    for (int i = 0; i < segments; i++) {
        indices.push_back(0);  // Center vertex
        indices.push_back(1 + i);
        indices.push_back(1 + ((i + 1) % segments));
    }
    
    vertexCount = indices.size();
    
    // Create and bind buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ChargeRenderer::draw(const std::vector<ElectricCharge>& charges, GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint chargeLoc = glGetUniformLocation(shaderProgram, "charge");
    
    glBindVertexArray(VAO);
    
    for (const auto& charge : charges) {
        // Set the charge value for coloring
        glUniform1f(chargeLoc, charge.charge);
        
        // Create model matrix for position and size
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(charge.position.x, charge.position.y, 0.0f));
        
        // Size based on charge magnitude (with a minimum size and some scaling)
        float size = 0.05f + 0.03f * std::abs(charge.charge);
        model = glm::scale(model, glm::vec3(size, size, 1.0f));
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        // Draw the circle
        glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    }
    
    glBindVertexArray(0);
}