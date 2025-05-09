#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <iomanip>
#include <string>
#include <algorithm>

#include "Arrow.hpp"
#include "ElectricField.hpp"
#include "ChargeRenderer.hpp"
#include "TextRender.hpp"
#include "Menu.hpp"
#include "Sensor.hpp"


//todo: Add charge values text into the charge

// Global variables for window size
int windowWidth = 1280;
int windowHeight = 720;

// Global variables for framerate rendering
double lastTime = 0.0;
int frameCount = 0;
float fps = 0.0f;

//Global variables for the menu
bool showMenu = false;
Menu* mainMenu = nullptr;
int menuOption;

// Global variables for dragging and drop
bool draggingCharge = false;
int selectedChargeIndex = -1;

// Global variables for sensor
Sensor* fieldSensor = nullptr;
bool draggingSensor = false;


// Window resizing callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    std::cout << "Window resized to: " << width << "x" << height << std::endl;
}

// Load shader files
std::string loadShaderCode(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Couldn't open file " << filepath << std::endl;
        return "";
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// Function to create a program shader
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertCode = loadShaderCode(vertexPath);
    std::string fragCode = loadShaderCode(fragmentPath);
    
    if (vertCode.empty() || fragCode.empty()) {
        std::cerr << "ERROR: Empty shader" << std::endl;
        return 0;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vCode = vertCode.c_str();
    glShaderSource(vertexShader, 1, &vCode, nullptr);
    glCompileShader(vertexShader);
    
    // Check for compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fCode = fragCode.c_str();
    glShaderSource(fragmentShader, 1, &fCode, nullptr);
    glCompileShader(fragmentShader);
    
    // Check for compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // Check for link problems
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Directional field define function
using VectorField = std::function<glm::vec2(float, float)>;

// Rotational field example: (-y, x)
glm::vec2 rotationalField(float x, float y) {
    return glm::vec2(-y, x);
}

// Directional field example for dy/dx = cos(y)
glm::vec2 cosineField(float x, float y) {
    return glm::vec2(1.0f, cos(y));
}

// Function to set up some test electric charges
void setupTestCharges(ElectricField& field) {
    // Clear any existing charges
    field.clearCharges();
    
    // Add a positive charge at (0.5, 0.0)
    field.addCharge(0.5f, 0.0f, 1.0f);
    
    // Add a negative charge at (-0.5, 0.0)
    field.addCharge(-0.5f, 0.0f, -1.0f);
}

// Menu voids
// Mouse event callbacks
/*
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (mainMenu) {
        mainMenu -> processMouseClick(button, action);
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mainMenu) {
        mainMenu -> processMouseMovement(xpos, ypos);
        }
        }
        */
ElectricField electricField;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Get cursor position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    // Convert screen coordinates to world coordinates
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float aspectRatio = (float)width / (float)height;
    
    // Map from screen to world coordinates
    float worldX = (2.0f * xpos / width - 1.0f) * (aspectRatio >= 1.0f ? aspectRatio : 1.0f);
    float worldY = -(2.0f * ypos / height - 1.0f) * (aspectRatio < 1.0f ? 1.0f / aspectRatio : 1.0f);
    
    if (showMenu) {
        mainMenu->processMouseClick(button, action);
    } else {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                // First check if sensor is clicked (sensor has priority)
                if (fieldSensor && fieldSensor->isActive() && fieldSensor->isPointOnSensor(worldX, worldY)) {
                    draggingSensor = true;
                } else {
                    // Then check charges
                    selectedChargeIndex = electricField.findChargeAt(worldX, worldY);
                    if (selectedChargeIndex >= 0) {
                        draggingCharge = true;
                    }
                }
            } else if (action == GLFW_RELEASE) {
                draggingCharge = false;
                selectedChargeIndex = -1;
                draggingSensor = false;
            }
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // Convert screen coordinates to world coordinates
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float aspectRatio = (float)width / (float)height;
    
    // Map from screen to world coordinates
    float worldX = (2.0f * xpos / width - 1.0f) * (aspectRatio >= 1.0f ? aspectRatio : 1.0f);
    float worldY = -(2.0f * ypos / height - 1.0f) * (aspectRatio < 1.0f ? 1.0f / aspectRatio : 1.0f);
    
    if (showMenu) {
        mainMenu->processMouseMovement(xpos, ypos);
    } else {
        if (draggingSensor && fieldSensor) {
            // Move sensor to new position
            fieldSensor->setPosition(worldX, worldY);
            // Update field vector at sensor position
            fieldSensor->updateFieldVector(electricField);
        } else if (draggingCharge && selectedChargeIndex >= 0) {
            // Move the selected charge to the new position
            electricField.moveCharge(selectedChargeIndex, worldX, worldY);
            
            // Update field vector at sensor position if sensor is active
            if (fieldSensor && fieldSensor->isActive()) {
                fieldSensor->updateFieldVector(electricField);
            }
        }
    }

    if (!draggingSensor) {
        selectedChargeIndex = electricField.findChargeAt(worldX, worldY);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset != 0 && selectedChargeIndex >= 0 ) electricField.changeChargeSize(selectedChargeIndex, yoffset);
    //std::cout << yoffset << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        showMenu = !showMenu;
        if (mainMenu) mainMenu -> setVisible(showMenu);
    }
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        mainMenu -> processKeyPress(key, action);
    }
    if ((key == GLFW_KEY_DOWN || key == GLFW_KEY_UP) && action == GLFW_PRESS) {
        if (!showMenu) {
        showMenu = true;
        mainMenu -> setVisible(showMenu);
        }
        else {
            if (key == GLFW_KEY_DOWN) mainMenu -> switchOptionDown(key, action);
            if (key == GLFW_KEY_UP) mainMenu -> switchOptionUp(key, action);
        }
    }
}



// Setup menu
void setupMenu(Menu* menu) {
    float menuX = 20.0f;
    //float menuY = windowHeight - 50.0f;
    float menuY = windowHeight - 50.0f;

    glm::vec3 normalColor(0.75f, 0.75f, 0.75f);
    glm::vec3 hoverColor (0.95f, 0.95f, 0.95f);

    // Menu elements
    menu -> addItem("Continue simulation", menuX, menuY, 0.66f, normalColor, hoverColor, []() {
        showMenu = false;
        if (mainMenu) mainMenu -> setVisible(false);
    });

    menuY -= 50.0f;
    menu -> addItem("Add positive charge", menuX, menuY, 0.66f, normalColor, hoverColor, []() {
        float x = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.8f;
        float y = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.8f;
        electricField.addCharge(x, y, 1.0f);
    });

    menuY -= 50.0f;
    menu -> addItem("Add negative charge", menuX, menuY, 0.66f, normalColor, hoverColor, []() {
        float x = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.8f;
        float y = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.8f;
        electricField.addCharge(x, y, -1.0f);
    });

    menuY -= 50.0f;
    menu -> addItem("Clear charges", menuX, menuY, 0.66f, normalColor, hoverColor, []() {
        electricField.clearCharges();
    });

    menuY -= 50.0f;
    menu -> addItem("Exit", menuX, menuY, 0.66f, normalColor, hoverColor, []() {
        glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
    });

    menuY -= 50.0f;
    menu -> addItem("Toggle sensor", menuX, menuY, 0.66f, normalColor, hoverColor, []() {
        if (fieldSensor) {
            fieldSensor->setActive(!fieldSensor->isActive());
        }
        showMenu = false;
        if (mainMenu) mainMenu -> setVisible(false);
    });
    }



int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Campos Eléctricos - Hokzaap Software", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
    glViewport(0, 0, windowWidth, windowHeight);

    GLuint shader = createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (shader == 0) {
        std::cerr << "Error creating shader program" << std::endl;
        glfwTerminate();
        return -1;
    }
    glUseProgram(shader);

    Arrow arrow;
    
    // Create an electric field
    // setupTestCharges(electricField);

    // Create the charge rendering object
    
    // Create the text rendering object
    TextRender textRenderer("fonts/GohuFortuni.ttf", 24); // Aumenté el tamaño para mejor visibilidad
    if (!textRenderer.init()) {
        std::cerr << "Error: Failed to initialize text renderer" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    ChargeRenderer chargeRenderer(&textRenderer, window);
    // Create the sensor
    fieldSensor = new Sensor(&textRenderer, window);
    fieldSensor->setPosition(0.0f, 0.0f);  // Default position at center
    
    mainMenu = new Menu(&textRenderer, window);
    setupMenu(mainMenu);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Choose the vector field to use
    // Options: rotationalField, cosineField, or electricField.getVectorField()
    VectorField vectorField = electricField.getVectorField(); 
    
    // Grid density
    int gridDensity = 25;
    float gridSpacing = 2.0f / gridDensity;

    GLint modelLoc = glGetUniformLocation(shader, "model");
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    
    if (modelLoc == -1 || viewLoc == -1 || projLoc == -1) {
        std::cerr << "Error: Couldn't find uniforms" << std::endl;
    }

    glm::mat4 view = glm::mat4(1.0f);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Enable transparency mix
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create a fragment shader for charges to visualize them
    GLuint chargeShader = createShaderProgram("shaders/vertex.glsl", "shaders/charge_fragment.glsl");
    if (chargeShader == 0) {
    std::cerr << "Error: Could not create charge shader program" << std::endl;
    glfwTerminate();
    return -1;
    }

    // Create the sensor shader early in your initialization code
    GLuint sensorShader = createShaderProgram("shaders/sensor_vertex.glsl", "shaders/sensor_fragment.glsl");
    if (sensorShader == 0) {
        std::cerr << "Error: Could not create sensor shader program" << std::endl;
          
        // Simple vertex shader that just transforms vertices
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 model;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
            "}\n";
        
        // Simple fragment shader that just outputs color
        const char* fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "uniform vec3 color;\n"
            "void main() {\n"
            "    FragColor = vec4(color, 1.0);\n"
            "}\n";
        
        // Create and compile vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        
        // Check vertex shader compile status
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        
        // Create and compile fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        
        // Check fragment shader compile status
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        
        // Create shader program and link shaders
        sensorShader = glCreateProgram();
        glAttachShader(sensorShader, vertexShader);
        glAttachShader(sensorShader, fragmentShader);
        glLinkProgram(sensorShader);
        
        // Check program link status
        glGetProgramiv(sensorShader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(sensorShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        
        // Delete the individual shaders as they're now linked into the program
        //glDeleteShader(vertexShader);
        //glDeleteShader(fragmentShader);
    }


    

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Update projection to keep propotions
        float aspectRatio = (float)windowWidth / (float)windowHeight;
        glm::mat4 projection;
        
        if (aspectRatio >= 1.0f) {
            // Wider window
            projection = glm::ortho(-1.0f * aspectRatio, 1.0f * aspectRatio, -1.0f, 1.0f);
        } else {
            // Higher window
            projection = glm::ortho(-1.0f, 1.0f, -1.0f / aspectRatio, 1.0f / aspectRatio);
        }
        
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Regenarate grid based on the vector field
        std::vector<glm::vec2> positions;
        std::vector<glm::vec2> directions;
        
        // Get field bounds adjusted for aspect ratio
        float xMin = -1.0f * aspectRatio;
        float xMax = 1.0f * aspectRatio;
        float yMin = -1.0f;
        float yMax = 1.0f;
        
        if (aspectRatio < 1.0f) {
            yMin = -1.0f / aspectRatio;
            yMax = 1.0f / aspectRatio;
        }
        
        for (float x = xMin; x <= xMax; x += gridSpacing) {
            for (float y = yMin; y <= yMax; y += gridSpacing) {
                // Skip points very close to charges to avoid extreme vectors
                bool skipPoint = false;
                for (const auto& charge : electricField.getCharges()) {
                    float distSquared = pow(x - charge.position.x, 2) + pow(y - charge.position.y, 2);
                    if (distSquared < 0.01f) {
                        skipPoint = true;
                        break;
                    }
                }
                
                if (skipPoint) continue;
                
                positions.emplace_back(x, y);
                
                // Get vector field direction
                glm::vec2 dir = vectorField(x, y);
                
                // Calculate magnitude of the field
                float magnitude = glm::length(dir);
                
                // Normalize and scale for visualization
                // Use a log scale to handle wide range of magnitudes
                float scaleFactor = 0.05f;
                if (magnitude > 0.0f) {
                    scaleFactor += 0.025f * log(1 + magnitude);
                }
                
                directions.emplace_back(glm::normalize(dir) * scaleFactor);
            }
        }
        
        // Draw Arrows
        for (size_t i = 0; i < positions.size(); ++i) {
            glm::vec2 pos = positions[i];
            glm::vec2 dir = directions[i];
            float angle = atan2(dir.y, dir.x);
            float magnitude = glm::length(dir);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pos, 0.0f));
            model = glm::rotate(model, angle, glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(magnitude, magnitude, 1.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            arrow.draw();
        }

        glUseProgram(chargeShader);
        glUniformMatrix4fv(glGetUniformLocation(chargeShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(chargeShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        chargeRenderer.draw(electricField.getCharges(), chargeShader);

        if (mainMenu && showMenu) {
            mainMenu -> render();
        }

        if (fieldSensor && fieldSensor->isActive()) {
            // Get view and projection matrices from the current shader
            glm::mat4 view = glm::mat4(1.0f);
            glm::mat4 projection = glm::mat4(1.0f);
            
            // Update the sensor shader with current view and projection matrices
            glUseProgram(sensorShader);
            glUniformMatrix4fv(glGetUniformLocation(sensorShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(sensorShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            
            // Update and render the sensor
            fieldSensor->updateFieldVector(electricField);
            fieldSensor->render(sensorShader);
        }
        
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0) { // update every second
            fps = static_cast<float>(frameCount) / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
        }
        std::stringstream ss;
        ss << "FPS: " << std::fixed << std::setprecision(1) << fps;
        textRenderer.renderText(ss.str(), 20.0f, windowHeight - ((windowHeight / 2.0f) + 30.0f), 0.75f, glm::vec3(1.0f, 1.0f, 0.0f));
        
        textRenderer.renderText("Simulación de cargas eléctricas", 20.0f, 17.5f, 0.66f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRenderer.renderText("Programado por: Juan Manuel Ley", (windowWidth / 2.0f) - 300.0f, 25.0f, 0.5f, glm::vec3(0.7f, 0.7f, 0.7f));
        textRenderer.renderText("© 2025 - Hokzaap Software", (windowWidth / 2.0f) - 300.0f, 10.0f, 0.5f, glm::vec3(0.7f,0.7f,0.7f));
        
        
        glUseProgram(shader);
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        
        //std::cout << "" << std::endl;
    }

    delete mainMenu;
    delete fieldSensor;
    glfwTerminate();
    return 0;
}
