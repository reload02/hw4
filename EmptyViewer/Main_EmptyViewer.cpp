
#include <Windows.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>

#define M_PI 3.14159265358979323846

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// ======================= Sphere Scene ========================
struct Vertex {
    float x, y, z;
};

int gNumVertices = 0;
int gNumTriangles = 0;
int* gIndexBuffer = NULL;
Vertex* gVertexArray = NULL;

const int WIDTH = 512;
const int HEIGHT = 512;
float depth_buffer[WIDTH * HEIGHT];
unsigned char framebuffer[HEIGHT][WIDTH][3];

void clear_buffers() {
    std::fill_n(depth_buffer, WIDTH * HEIGHT, 1e9f);
    std::fill_n(&framebuffer[0][0][0], WIDTH * HEIGHT * 3, 0);
}

void create_scene()
{
    int width = 32;
    int height = 16;

    float theta, phi;
    int t;

    gNumVertices = (height - 2) * width + 2;
    gNumTriangles = 2 * (height - 3) * width + 2 * width;

    gVertexArray = new Vertex[gNumVertices];
    gIndexBuffer = new int[3 * gNumTriangles];

    t = 0;
    for (int j = 1; j < height - 1; ++j) {
        for (int i = 0; i < width; ++i) {
            theta = (float)j / (height - 1) * M_PI;
            phi = (float)i / width * M_PI * 2;

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);

            gVertexArray[t++] = { x, y, z };
        }
    }

    gVertexArray[t++] = { 0.0f, 1.0f, 0.0f };
    gVertexArray[t++] = { 0.0f, -1.0f, 0.0f };

    t = 0;
    for (int j = 0; j < height - 3; ++j) {
        for (int i = 0; i < width; ++i) {
            int idx0 = j * width + i;
            int idx1 = (j + 1) * width + i;
            int idx2 = j * width + (i + 1) % width;
            int idx3 = (j + 1) * width + (i + 1) % width;

            gIndexBuffer[t++] = idx0;
            gIndexBuffer[t++] = idx3;
            gIndexBuffer[t++] = idx2;

            gIndexBuffer[t++] = idx0;
            gIndexBuffer[t++] = idx1;
            gIndexBuffer[t++] = idx3;
        }
    }

    int top_index = gNumVertices - 2;
    for (int i = 0; i < width; ++i) {
        gIndexBuffer[t++] = top_index;
        gIndexBuffer[t++] = i;
        gIndexBuffer[t++] = (i + 1) % width;
    }

    int bottom_index = gNumVertices - 1;
    int base = (height - 3) * width;
    for (int i = 0; i < width; ++i) {
        gIndexBuffer[t++] = bottom_index;
        gIndexBuffer[t++] = base + (i + 1) % width;
        gIndexBuffer[t++] = base + i;
    }
}

// ===================== Rasterizer Section =========================
mat4 get_model_matrix() {
    return translate(mat4(1.0f), vec3(0, 0, -7)) * scale(mat4(1.0f), vec3(2.0f));
}

mat4 get_view_matrix() {
    mat4 V = mat4(1.0f);
    return V; // eye at origin looking -z
}

mat4 get_projection_matrix() {
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, n = -0.1f, f = -1000.0f;
    mat4 P(0.0f);
    P[0][0] = 2.0f * n / (r - l);
    P[1][1] = 2.0f * n / (t - b);
    P[2][0] = (r + l) / (r - l);
    P[2][1] = (t + b) / (t - b);
    P[2][2] = (n + f) / (n - f);
    P[2][3] = -1.0f;
    P[3][2] = 2.0f * f * n / (n - f);
    return P;
}

mat4 get_viewport_matrix(int nx, int ny) {
    mat4 V = mat4(1.0f);
    V[0][0] = nx / 2.0f; V[3][0] = nx / 2.0f;
    V[1][1] = ny / 2.0f; V[3][1] = ny / 2.0f;
    V[2][2] = 1.0f;
    return V;
}
float cross2d(const vec2& a, const vec2& b) {
    return a.x * b.y - a.y * b.x;
}

bool inside_triangle(float x, float y, vec4 v0, vec4 v1, vec4 v2) {
    vec2 p(x, y), a(v0.x, v0.y), b(v1.x, v1.y), c(v2.x, v2.y);
    float area = cross2d(b - a, c - a);
    float alpha = cross2d(b - p, c - p) / area;
    float beta = cross2d(c - p, a - p) / area;
    float gamma = cross2d(a - p, b - p) / area;
    return alpha >= 0 && beta >= 0 && gamma >= 0;
}

void rasterize_triangle(vec4 v0, vec4 v1, vec4 v2) {
    int minX = int(std::min({ v0.x, v1.x, v2.x }));
    int maxX = int(std::max({ v0.x, v1.x, v2.x }));
    int minY = int(std::min({ v0.y, v1.y, v2.y }));
    int maxY = int(std::max({ v0.y, v1.y, v2.y }));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) continue;
            if (inside_triangle(x + 0.5f, y + 0.5f, v0, v1, v2)) {
                float z = (v0.z + v1.z + v2.z) / 3.0f;
                int idx = y * WIDTH + x;
                if (z < depth_buffer[idx]) {
                    depth_buffer[idx] = z;
                    framebuffer[y][x][0] = 255;
                    framebuffer[y][x][1] = 255;
                    framebuffer[y][x][2] = 255;
                }
            }
        }
    }
}

void render_scene() {
    clear_buffers();
    mat4 MVP = get_viewport_matrix(WIDTH, HEIGHT) * get_projection_matrix() * get_view_matrix() * get_model_matrix();

    for (int i = 0; i < gNumTriangles; ++i) {
        int i0 = gIndexBuffer[3 * i];
        int i1 = gIndexBuffer[3 * i + 1];
        int i2 = gIndexBuffer[3 * i + 2];

        vec4 v0(gVertexArray[i0].x, gVertexArray[i0].y, gVertexArray[i0].z, 1.0f);
        vec4 v1(gVertexArray[i1].x, gVertexArray[i1].y, gVertexArray[i1].z, 1.0f);
        vec4 v2(gVertexArray[i2].x, gVertexArray[i2].y, gVertexArray[i2].z, 1.0f);

        v0 = MVP * v0; v0 /= v0.w;
        v1 = MVP * v1; v1 /= v1.w;
        v2 = MVP * v2; v2 /= v2.w;

        rasterize_triangle(v0, v1, v2);
    }
}

// ==================== GLFW Viewer ========================
int Width = WIDTH;
int Height = HEIGHT;
std::vector<float> OutputImage;

void render() {
    render_scene();
    OutputImage.clear();
    for (int j = 0; j < HEIGHT; ++j)
        for (int i = 0; i < WIDTH; ++i)
            for (int c = 0; c < 3; ++c)
                OutputImage.push_back(framebuffer[j][i][c] / 255.0f);
}

void resize_callback(GLFWwindow*, int nw, int nh) {
    Width = nw;
    Height = nh;
    glViewport(0, 0, nw, nh);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Width, 0.0, Height, 1.0, -1.0);
    OutputImage.reserve(Width * Height * 3);
    render();
}

int main(int argc, char* argv[]) {
    GLFWwindow* window;
    if (!glfwInit()) return -1;

    window = glfwCreateWindow(Width, Height, "Software Rasterizer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    create_scene();
    resize_callback(NULL, Width, Height);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
