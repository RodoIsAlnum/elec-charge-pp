#pragma once

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glad/glad.h>

// Character struct
struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

class TextRender {
public:
    TextRender(const std::string& fontPath, unsigned int fontSize);
    ~TextRender();
    
    // Initializes text shaders
    bool init();
    
    // Renders text on a specific position with a color and scale
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    
private:
    FT_Library ft;
    FT_Face face;
    bool initialized = false;
    // Cambiado para usar unsigned long (codepoints Unicode) en lugar de char
    std::map<unsigned long, Character> Characters;
    
    // Shader específico para texto
    unsigned int shaderProgram;
    unsigned int VAO, VBO;
    
    // Método para cargar un carácter a demanda
    bool loadCharacter(unsigned long codepoint);
};