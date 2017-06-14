#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include "framework.h"

// An array of 3 vectors which represents 3 vertices
static const GLfloat g_vertex_buffer_data[] = {
    50, 50, 0,
    300, 50, 0,
    75, 300, 0
};
// This will identify our vertex buffer
static GLuint vertexbuffer;
static struct timespec last_reset_time;
static GLuint shader_program_id;

static struct {
    int x;
    int y;
    int width;
    int height;
} window;

static const char * vertex_shader =
    "#version 130\n"
    "uniform mat4 projection;\n"
    "in vec3 in_Position;\n"
    "out vec3 ex_Color;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position = vec4(in_Position, 1.0) * projection;\n"
    "    ex_Color = vec3(244, 226, 66)/256;\n"
    "}";
static const char * fragment_shader =
    "#version 130\n"
    "in vec3 ex_Color;\n"
    "out vec4 out_Color;\n"
    "void main(void)\n"
    "{\n"
    "    out_Color = vec4(ex_Color,1.0);\n"
    "}";

static GLuint
compile_shader(const char * source, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    // Determine compile status
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

static float projection_matrix[4][4];
static float view_matrix[4][4];

static void
orthogonal_projection(float width, float height) {
    float l = 0;
    float r = width;
    float t = 0;
    float b = height;
    float f = 1;
    float n = -1;
    projection_matrix[0][0] = 2 / (r - l);
    projection_matrix[0][1] = 0;
    projection_matrix[0][2] = 0;
    projection_matrix[0][3] = -(r + l) / (r - l);
    projection_matrix[1][0] = 0;
    projection_matrix[1][1] = 2 / (t - b);
    projection_matrix[1][2] = 0;
    projection_matrix[1][3] = -(t + b) / (t - b);
    projection_matrix[2][0] = 0;
    projection_matrix[2][1] = 0;
    projection_matrix[2][2] = -2 / (f - n);
    projection_matrix[2][3] = -(f + n) / (f - n);
    projection_matrix[3][0] = 0;
    projection_matrix[3][1] = 0;
    projection_matrix[3][2] = 0;
    projection_matrix[3][3] = 1;
}

static void
mat4_multiply(float * a, float * b, float * out) {
    for (int i = 0; i < 4; ++i) {
        int i4 = 4 * i;
        for (int j = 0; j < 4; ++j) {
            out[i4 + j] = 0;
            for (int k = 0; k < 4; ++k) {
                out[i4 + j] += a[i4 + k] * b[4 * k + j];
            }
        }
    }
}

static void
rotate_z(double angle) {
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    view_matrix[0][0] = cos_angle;
    view_matrix[0][1] = -sin_angle;
    view_matrix[0][2] = 0;
    view_matrix[0][3] = 0;
    view_matrix[1][0] = sin_angle;
    view_matrix[1][1] = cos_angle;
    view_matrix[1][2] = 0;
    view_matrix[1][3] = 0;
    view_matrix[2][0] = 0;
    view_matrix[2][1] = 0;
    view_matrix[2][2] = 1;
    view_matrix[2][3] = 0;
    view_matrix[3][0] = 0;
    view_matrix[3][1] = 0;
    view_matrix[3][2] = 0;
    view_matrix[3][3] = 1;
}

static void
init_scene() {
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data),
        g_vertex_buffer_data, GL_STATIC_DRAW);
    clock_gettime(CLOCK_MONOTONIC, &last_reset_time);
    GLuint vertex_shader_id = compile_shader(vertex_shader, GL_VERTEX_SHADER);
    GLuint fragment_shader_id = compile_shader(fragment_shader,
        GL_FRAGMENT_SHADER);
    shader_program_id = glCreateProgram();
    glAttachShader(shader_program_id, vertex_shader_id);
    glAttachShader(shader_program_id, fragment_shader_id);
    glLinkProgram(shader_program_id);
    GLint result = GL_FALSE;
    glGetProgramiv(shader_program_id, GL_LINK_STATUS, &result);
    if (GL_TRUE != result) {
        char message[256];
        glGetProgramInfoLog(shader_program_id, sizeof(message), NULL, message);
        puts(message);
        exit(-1);
    }
    glClearColor(0, 0, 0, 0);
    orthogonal_projection(window.width, window.height);
}

static double
difference_in_seconds(struct timespec * a, struct timespec * b) {
    return (double) (a->tv_sec - b->tv_sec) + (a->tv_nsec - b->tv_nsec) / 1e9;
}

static double angle = 0;

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
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program_id);
    int projection_location =
        glGetUniformLocation(shader_program_id, "projection");
    angle += 0.02;
    for (int i = 0; i < 4; ++i) {
        rotate_z(angle + 3.14 / 2 * i);
        float accumulated_projection_matrix[4][4];
        mat4_multiply((float *)projection_matrix, (float *)view_matrix,
            (float *)accumulated_projection_matrix);
        glUniformMatrix4fv(projection_location, 1, GL_FALSE,
            (float *)accumulated_projection_matrix);
        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
           0,                  // attribute 0. No particular reason for 0,
                               //     but must match the layout in the shader.
           3,                  // size
           GL_FLOAT,           // type
           GL_FALSE,           // normalized?
           0,                  // stride
           (void*)0            // array buffer offset
        );
        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0;
                                          // 3 vertices total -> 1 triangle
        glDisableVertexAttribArray(0);
    }
}

static void
process_resize_event(int x, int y, int width, int height) {
    glViewport(0, 0, width, height);
    orthogonal_projection(width, height);
    window.x = x;
    window.y = y;
    window.width = width;
    window.height = height;
}

int
main(int argc, char * argv[]) {
    window_init();
    init_scene();
    redraw_callback = draw_scene;
    window_configuration_callback = process_resize_event;
    window_event_loop();
    return 0;
}
