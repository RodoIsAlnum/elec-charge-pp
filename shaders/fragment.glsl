#version 330 core
out vec4 FragColor;
uniform float magnitude;

void main() {
    //FragColor = vec4(1.0, 0.7, 0.2, 1.0); //orange

    vec3 color = mix(
        vec3(0.0,0.4,0.8), // low magnitude  (blue)
        vec3(1.0,0.3,0.2), // high magnitude (red)
        magnitude
    );

    FragColor = vec4(color, 1.0);
}