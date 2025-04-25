#include "TextRender.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <GLFW/glfw3.h>

// Shaders para renderizar texto (inline como strings para simplificar)
const char* textVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

const char* textFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}
)";

// Función auxiliar para compilar shaders
unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    // Verificar errores de compilación
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    return shader;
}

TextRender::TextRender(const std::string& fontPath, unsigned int fontSize) {
    if (FT_Init_FreeType(&ft))
    {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }
    
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
    {
        std::cerr << "ERROR::FREETYPE: Failed to load font: " << fontPath << std::endl;
        return;
    }
    
    // Establecer el tamaño de la fuente
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    
    // Deshabilitar la restricción de alineación de bytes
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    initialized = true;
    
    // No cargamos todos los caracteres de una vez, sino que los cargaremos a demanda
    // para manejar mejor caracteres UTF-8
}

bool TextRender::init() {
    // Compilar y enlazar los shaders para texto
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, textVertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, textFragmentShaderSource);
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Verificar errores de enlace
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }
    
    // Liberar los shaders después de enlazar
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Configurar VAO/VBO para el texto
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return true;
}

// Método para cargar un carácter específico si no está ya cargado
bool TextRender::loadCharacter(unsigned long codepoint) {
    if (Characters.find(codepoint) != Characters.end()) {
        // El carácter ya está cargado
        return true;
    }
    
    // Cargar el glifo para este codepoint
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
        std::cerr << "ERROR::FREETYTPE: Failed to load Glyph: " << codepoint << std::endl;
        return false;
    }
    
    // Generar textura
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer
    );
    
    // Configurar opciones de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Almacenar el carácter para uso posterior
    Character character = {
        texture,
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        static_cast<unsigned int>(face->glyph->advance.x)
    };
    
    Characters.insert(std::pair<unsigned long, Character>(codepoint, character));
    return true;
}

void TextRender::renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
    if (!initialized) {
        std::cerr << "ERROR: TextRender not properly initialized" << std::endl;
        return;
    }
    
    // Convertir string UTF-8 a codepoints Unicode
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string utf32 = converter.from_bytes(text);
    
    // Activar el shader para texto
    glUseProgram(shaderProgram);
    
    // Configurar la matriz de proyección ortográfica
    // Nota: ajustar según las dimensiones de la ventana
    int windowWidth, windowHeight;
    GLFWwindow* window = glfwGetCurrentContext();
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth), 0.0f, static_cast<float>(windowHeight));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Establecer el color del texto
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color.r, color.g, color.b);
    
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    
    // Iterar a través de todos los caracteres
    float x_pos = x;
    for (char32_t c : utf32) {
        // Cargar el carácter si es necesario
        if (!loadCharacter(c)) {
            continue; // Saltar si no se puede cargar
        }
        
        Character ch = Characters[c];
        
        float xpos = x_pos + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        
        // Actualizar VBO para cada carácter
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        
        // Renderizar textura del glifo
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        
        // Actualizar contenido del VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // Renderizar quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Avanzar posición de cursor para el siguiente glifo (avance está en 1/64 pixels)
        x_pos += (ch.Advance >> 6) * scale; // Desplazamiento de 6 bits para dividir por 64
    }
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

TextRender::~TextRender() {
    if (initialized) {
        // Liberar las texturas de los caracteres
        for (auto& pair : Characters) {
            glDeleteTextures(1, &pair.second.TextureID);
        }
        
        // Liberar VAO y VBO
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        
        // Liberar programa de shader
        glDeleteProgram(shaderProgram);
        
        // Liberar recursos de FreeType
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }
}