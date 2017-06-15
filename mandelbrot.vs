#version 130

uniform vec2 focus;
uniform float scale;

in vec2 vertex_position;

out vec2 fragment_position;

void main(void) {
    vec2 t = focus + vertex_position / scale;
    gl_Position = vec4(vertex_position, 0, 1);
    fragment_position = t;
}
