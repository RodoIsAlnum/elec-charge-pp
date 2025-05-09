#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>

#include "ElectricField.hpp"
#include "TextRender.hpp"

class Sensor {
public:
    Sensor(TextRender* textRenderer, GLFWwindow* window);
    ~Sensor();
    
    // Initialize the sensor at a specific position
    void setPosition(float x, float y);
    
    // Get the current position
    glm::vec2 getPosition() const;
    
    // Calculate and update the field vector at the sensor position
    void updateFieldVector(const ElectricField& field);
    
    // Render the sensor and field vector
    void render(GLuint shaderProgram);
    
    // Check if the sensor is being clicked
    bool isPointOnSensor(float x, float y, float radius = 0.1f) const;
    
    // Check if the sensor is active/visible
    bool isActive() const;
    
    // Activate/deactivate the sensor
    void setActive(bool active);

private:
    TextRender* textRenderer;
    GLFWwindow* window;
    
    glm::vec2 position;            // Position of the sensor
    glm::vec2 fieldVector;         // Direction and magnitude of the electric field
    
    GLuint VAO, VBO;               // OpenGL objects for sensor rendering
    
    bool active;                   // Is the sensor active/visible?
    
    void setupSensor();            // Initialize sensor geometry
    void renderSensorData();       // Render text information about field at sensor
    
    // Helper methods for coordinate transformations
    float getAspectRatio() const;
    glm::vec2 worldToScreenCoords(const glm::vec2& worldPos) const;
};