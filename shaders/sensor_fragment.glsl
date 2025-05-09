#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main() {
    // Use the provided color with full opacity
    FragColor = vec4(color, 1.0);
}