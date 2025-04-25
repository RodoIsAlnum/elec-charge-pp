#version 330 core
out vec4 FragColor;

uniform float charge; // Positive or negative charge value

void main() {
    // Positive charge: red
    // Negative charge: blue
    vec3 color = (charge > 0.0) ? vec3(1.0, 0.2, 0.2) : vec3(0.2, 0.4, 1.0);
    
    FragColor = vec4(color, 1.0);
}