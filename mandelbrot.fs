#version 130

#define MAX_ITERATIONS 200

uniform sampler1D palette;

in vec2 fragment_position;

out vec4 fragment_color;

void main(void) {
    float x = fragment_position.x;
    float y = fragment_position.y;
    int i;
    for (i = 0; i < MAX_ITERATIONS; ++i) {
        float sx = x * x - y * y;
        float sy = 2 * x * y;
        x = sx + fragment_position.x;
        y = sy + fragment_position.y;
        if (4 < x * x + y * y) break;
    }
    float f = i;
    float c = f / MAX_ITERATIONS;
    fragment_color = texture(palette, c);/*vec4(c, c, 1 - c, 1);*/
}
