#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include "framework.h"

static struct timespec last_reset_time;

static struct {
    int x;
    int y;
    int width;
    int height;
} window;

static struct {
    int x;
    int y;
} mouse;

static char *
load_file(const char * filename) {
    FILE * f = fopen(filename, "r");
    if (!f) {
        printf("Can't open file: %s\n", filename);
        exit(-1);
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char * buffer = (char *)malloc(size);
    size_t read = fread(buffer, 1, size, f);
    if (read != size) {
        printf("Can't read file: %s\n", filename);
        exit(-1);
    }
    return buffer;
}

static GLuint
compile_shader(const char * source, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint result = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (GL_TRUE != result) {
        char message[256];
        glGetShaderInfoLog(shader, sizeof(message), NULL, message);
        puts(message);
        exit(-1);
    }
    return shader;
}

static GLuint
compile_program(const char * vertex_shader_file,
        const char * fragment_shader_file) {
    char * vertex_shader = load_file(vertex_shader_file);
    GLuint vertex_shader_id = compile_shader(vertex_shader, GL_VERTEX_SHADER);
    free(vertex_shader);
    char * fragment_shader = load_file(fragment_shader_file);
    GLuint fragment_shader_id = compile_shader(fragment_shader,
        GL_FRAGMENT_SHADER);
    free(fragment_shader);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vertex_shader_id);
    glAttachShader(shader, fragment_shader_id);
    glLinkProgram(shader);
    GLint result = GL_FALSE;
    glGetProgramiv(shader, GL_LINK_STATUS, &result);
    if (GL_TRUE != result) {
        char message[256];
        glGetProgramInfoLog(shader, sizeof(message), NULL, message);
        puts(message);
        exit(-1);
    }
    return shader;
}

static GLuint simple_shader;
static GLuint mandelbrot_shader;

static GLuint background_mesh;
static GLuint next_scale_rect;

static GLuint
make_vertex_buffer(void * buffer_data, size_t size, GLenum type) {
    GLuint buffer_id;
    glGenBuffers(1, &buffer_id);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER, size, buffer_data, type);
    return buffer_id;
}

static void
init_scene() {
    clock_gettime(CLOCK_MONOTONIC, &last_reset_time);
    float background_mesh_data[] = {
        -1, -1,
        -1,  1,
         1,  1,
         1, -1,
    };
    background_mesh = make_vertex_buffer(background_mesh_data,
        sizeof(background_mesh_data), GL_STATIC_DRAW);
    float scale_rect_data[] = {
        -0.5, -0.5,
        -0.5,  0.5,
         0.5,  0.5,
         0.5, -0.5,
    };
    next_scale_rect = make_vertex_buffer(scale_rect_data,
        sizeof(scale_rect_data), GL_DYNAMIC_DRAW);
    mandelbrot_shader = compile_program("mandelbrot.vs", "mandelbrot.fs");
    simple_shader = compile_program("simple.vs", "simple.fs");
    glClearColor(0, 0, 0, 0);
}

static double
difference_in_seconds(struct timespec * a, struct timespec * b) {
    return (double) (a->tv_sec - b->tv_sec) + (a->tv_nsec - b->tv_nsec) / 1e9;
}

static double angle = 0;

static float focus[2] = { -0.6, 0 };
static float scale = 1/1.33;

static void
draw_scene() {
    static int frames = 0;
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    double dt = difference_in_seconds(&current_time, &last_reset_time);
    if (1 < dt) {
        printf("fps: %lf\n", frames / dt);
        last_reset_time = current_time;
        frames = 0;
    }
    ++frames;
    // Clear window
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw background mandelbrot set
    glUseProgram(mandelbrot_shader);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, background_mesh);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glUniform2fv(0, 1, focus);
    glUniform1f(1, scale);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(0);
    // Draw scale rectangle
    glUseProgram(simple_shader);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, next_scale_rect);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    static float color[3] = { 0, 1, 0 };
    glUniform3fv(0, 1, color);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableVertexAttribArray(0);
    // Draw focus point
    static float zero[2] = { 0, 0 };
    glVertexPointer(2, GL_FLOAT, 0, zero);
    glDrawArrays(GL_POINTS, 0, 1);
}

static void
process_resize_event(int x, int y, int width, int height) {
    glViewport(0, 0, width, height);
    window.x = x;
    window.y = y;
    window.width = width;
    window.height = height;
}

static void
move_function(int x, int y) {
    mouse.x = x;
    mouse.y = y;
    float half_width = window.width / 2.;
    float half_height = window.height / 2.;
    float center_x = (x - half_width) / half_width;
    float center_y = (-y + half_height) / half_height;
    float scale_rect_data[] = {
        center_x - 0.5, center_y - 0.5,
        center_x - 0.5, center_y + 0.5,
        center_x + 0.5, center_y + 0.5,
        center_x + 0.5, center_y - 0.5,
    };
    glBindBuffer(GL_ARRAY_BUFFER, next_scale_rect);
    glBufferData(GL_ARRAY_BUFFER, sizeof(scale_rect_data), scale_rect_data,
        GL_DYNAMIC_DRAW);
}

static void
click_function(int x, int y) {
    mouse.x = x;
    mouse.y = y;
    float half_width = window.width / 2.;
    float half_height = window.height / 2.;
    float click_x = (x - half_width) / half_width;
    float click_y = (-y + half_height) / half_height;
    focus[0] = focus[0] + click_x / scale;
    focus[1] = focus[1] + click_y / scale;
    scale *= 2;
}

int
main(int argc, char * argv[]) {
    window_init();
    init_scene();
    redraw_callback = draw_scene;
    move_callback = move_function;
    click_callback = click_function;
    window_configuration_callback = process_resize_event;
    window_event_loop();
    return 0;
}
