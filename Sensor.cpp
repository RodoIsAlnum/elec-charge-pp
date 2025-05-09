#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "Sensor.hpp"

Sensor::Sensor(TextRender* textRenderer, GLFWwindow* window)
    : textRenderer(textRenderer), window(window), position(0.0f, 0.0f), fieldVector(0.0f, 0.0f), active(false) {
    setupSensor();
}

Sensor::~Sensor() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Sensor::setupSensor() {
    // Create a circle for the sensor
    const int numSegments = 32;
    const float radius = 0.05f;
    std::vector<float> vertices;
    
    // Center of the circle
    float centerX = 0.0f;
    float centerY = 0.0f;
    
    // Add triangles to form the circle (fan style)
    for (int i = 0; i < numSegments; ++i) {
        // Center of the circle
        vertices.push_back(centerX);
        vertices.push_back(centerY);
        vertices.push_back(0.0f);
        
        // First point on the perimeter
        float angle1 = 2.0f * M_PI * i / numSegments;
        vertices.push_back(centerX + radius * cos(angle1));
        vertices.push_back(centerY + radius * sin(angle1));
        vertices.push_back(0.0f);
        
        // Second point on the perimeter
        float angle2 = 2.0f * M_PI * (i + 1) / numSegments;
        vertices.push_back(centerX + radius * cos(angle2));
        vertices.push_back(centerY + radius * sin(angle2));
        vertices.push_back(0.0f);
    }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Sensor::setPosition(float x, float y) {
    position.x = x;
    position.y = y;
}

glm::vec2 Sensor::getPosition() const {
    return position;
}

void Sensor::updateFieldVector(const ElectricField& field) {
    fieldVector = field.getFieldAt(position.x, position.y);
}

bool Sensor::isPointOnSensor(float x, float y, float radius) const {
    float dx = position.x - x;
    float dy = position.y - y;
    float distSquared = dx*dx + dy*dy;
    return distSquared < radius*radius;
}

bool Sensor::isActive() const {
    return active;
}

void Sensor::setActive(bool active) {
    this->active = active;
}

// Helper method to get the current window's aspect ratio
float Sensor::getAspectRatio() const {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    return (float)windowWidth / (float)windowHeight;
}

// Helper method to convert world coordinates to screen coordinates
glm::vec2 Sensor::worldToScreenCoords(const glm::vec2& worldPos) const {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    float aspectRatio = getAspectRatio();
    
    float screenX, screenY;
    
    if (aspectRatio >= 1.0f) {
        // Wider window
        screenX = (worldPos.x / aspectRatio + 1.0f) * 0.5f * windowWidth;
        screenY = (worldPos.y + 1.0f) * 0.5f * windowHeight;
    } else {
        // Taller window
        screenX = (worldPos.x + 1.0f) * 0.5f * windowWidth;
        screenY = (worldPos.y * aspectRatio + 1.0f) * 0.5f * windowHeight;
    }
    
    return glm::vec2(screenX, screenY);
}

void Sensor::render(GLuint shaderProgram) {
    if (!active) return;
    
    // Save current program to restore later
    GLint originalProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &originalProgram);
    
    // Use the provided shader program
    glUseProgram(shaderProgram);
    
    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    // Check if uniforms were found
    if (modelLoc == -1) {
        std::cerr << "Warning: 'model' uniform not found in sensor shader" << std::endl;
    }
    if (viewLoc == -1) {
        std::cerr << "Warning: 'view' uniform not found in sensor shader" << std::endl;
    }
    if (projLoc == -1) {
        std::cerr << "Warning: 'projection' uniform not found in sensor shader" << std::endl;
    }
    if (colorLoc == -1) {
        std::cerr << "Warning: 'color' uniform not found in sensor shader" << std::endl;
    }
    
    // Get current view and projection matrices
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    // Copy current view and projection matrices if available
    if (viewLoc != -1) {
        float viewData[16];
        glGetUniformfv(originalProgram, glGetUniformLocation(originalProgram, "view"), viewData);
        view = glm::make_mat4(viewData);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    
    // Create projection matrix using aspect ratio - consistent with main.cpp
    float aspectRatio = getAspectRatio();
    if (projLoc != -1) {
        if (aspectRatio >= 1.0f) {
            // Wider window
            projection = glm::ortho(-1.0f * aspectRatio, 1.0f * aspectRatio, -1.0f, 1.0f);
        } else {
            // Taller window
            projection = glm::ortho(-1.0f, 1.0f, -1.0f / aspectRatio, 1.0f / aspectRatio);
        }
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    
    // Render the vector arrow FIRST (so it appears behind the circle)
    float magnitude = glm::length(fieldVector);
    
    if (magnitude > 0.001f) { // Check if the field magnitude is significant
        // Calculate arrow length based on field magnitude
        // Use logarithmic scale for better visualization of different magnitudes
        float maxArrowLength = 10.0f; // Allow the arrow to reach the screen edge
        float arrowLength = 0.1f * (1.0f + log(1.0f + magnitude));
        
        // Limit maximum length
        arrowLength = std::min(arrowLength, maxArrowLength);
        
        // Scale and rotate the arrow to match field direction
        float angle = atan2(fieldVector.y, fieldVector.x);
        
        glm::mat4 arrowModel = glm::mat4(1.0f);
        arrowModel = glm::translate(arrowModel, glm::vec3(position.x, position.y, 0.0f));
        arrowModel = glm::rotate(arrowModel, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        arrowModel = glm::scale(arrowModel, glm::vec3(arrowLength, arrowLength, 1.0f));
        
        if (modelLoc != -1) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(arrowModel));
        }
        
        // Red color for the field vector
        if (colorLoc != -1) {
            glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
        }
        
        // Draw the arrow (using vertex data similar to Arrow class)
        // Modified to start at origin (center of the circle)
        float arrowVertices[] = {
            // Base Rectangle
            0.0f, -0.02f, 0.0f,   // left bottom (now at origin)
            0.4f, -0.02f, 0.0f,   // right bottom
            0.4f,  0.02f, 0.0f,   // right top
            0.0f, -0.02f, 0.0f,   // left bottom (now at origin)
            0.4f,  0.02f, 0.0f,   // right top
            0.0f,  0.02f, 0.0f,   // left top (now at origin)
            
            // Tip
            0.4f, -0.06f, 0.0f,   // bottom of tip
            0.5f,  0.00f, 0.0f,   // point of tip
            0.4f,  0.06f, 0.0f,   // top of tip
        };
        
        // Create temporary VAO and VBO for the arrow
        GLuint tempVAO, tempVBO;
        glGenVertexArrays(1, &tempVAO);
        glGenBuffers(1, &tempVBO);
        
        // Bind the temporary VAO and configure attributes
        glBindVertexArray(tempVAO);
        glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(arrowVertices), arrowVertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Draw the arrow
        glDrawArrays(GL_TRIANGLES, 0, 9);
        
        // Clean up
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &tempVAO);
        glDeleteBuffers(1, &tempVBO);
    }
    
    // THEN render the sensor (yellow circle) - now it will be on top of the arrow
    glBindVertexArray(VAO);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f));
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    }
    
    // Yellow color for the sensor
    if (colorLoc != -1) {
        glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f);
    }
    
    // Draw the sensor circle (32 triangles = 96 vertices)
    glDrawArrays(GL_TRIANGLES, 0, 32 * 3);
    
    glBindVertexArray(0);
    glUseProgram(originalProgram);
    
    // Render text information about the field at sensor position
    renderSensorData();
}

void Sensor::renderSensorData() {
    // Now using our helper method to convert world coords to screen coords
    glm::vec2 screenPos = worldToScreenCoords(position);
    float screenX = screenPos.x;
    float screenY = screenPos.y;
    
    float magnitude = glm::length(fieldVector);
    
    // Calculate field direction in degrees (0-360)
    float direction = 0.0f;
    if (magnitude > 0.001f) {
        direction = atan2(fieldVector.y, fieldVector.x) * 180.0f / M_PI;
        if (direction < 0) direction += 360.0f;
    }
    
    // Prepare text to display
    std::stringstream posText, magText, dirText;
    
    // Format position with 2 decimals
    posText << "Pos: (" << std::fixed << std::setprecision(2) << position.x << ", " 
            << std::fixed << std::setprecision(2) << position.y << ")";
    
    // Format magnitude with appropriate scientific notation
    if (magnitude < 0.001f) {
        magText << "E: ~0 N/C";
    } else if (magnitude < 0.01f) {
        magText << "E: " << std::fixed << std::setprecision(5) << magnitude << " N/C";
    } else if (magnitude < 100.0f) {
        magText << "E: " << std::fixed << std::setprecision(3) << magnitude << " N/C";
    } else {
        magText << "E: " << std::scientific << std::setprecision(2) << magnitude << " N/C";
    }
    
    // Format direction with 1 decimal
    if (magnitude < 0.001f) {
        dirText << "Dir: N/A";
    } else {
        dirText << "Dir: " << std::fixed << std::setprecision(1) << direction << "°";
    }
    
    // Render text information
    glm::vec3 textColor(1.0f, 1.0f, 1.0f);
    float textScale = 0.5f;
    float textOffsetY = 15.0f;
    
    textRenderer->renderText(posText.str(), screenX + 15.0f, screenY - textOffsetY, textScale, textColor);
    textRenderer->renderText(magText.str(), screenX + 15.0f, screenY - 2*textOffsetY, textScale, textColor);
    textRenderer->renderText(dirText.str(), screenX + 15.0f, screenY - 3*textOffsetY, textScale, textColor);
}