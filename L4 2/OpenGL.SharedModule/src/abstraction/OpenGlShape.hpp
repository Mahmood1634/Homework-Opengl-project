#pragma once
// OpenGlShape: a GPU mesh (VAO + VBO + EBO).
// Vertex layout (same for every shape):  location 0 = position (vec3)
//                                         location 1 = normal   (vec3)
//                                         location 2 = uv       (vec2)
#include <glad/glad.h>
#include <vector>

class OpenGlShape
{
public:
    OpenGlShape() = default;
    ~OpenGlShape();
    OpenGlShape(const OpenGlShape&) = delete;
    OpenGlShape& operator=(const OpenGlShape&) = delete;
    OpenGlShape(OpenGlShape&& other) noexcept;
    OpenGlShape& operator=(OpenGlShape&& other) noexcept;

    // 1x1 quad lying on the ground (XZ plane), normal = +Y.
    // uv.v = 1 at z = -0.5 (the far side), so an image is upright when seen from +Z.
    static OpenGlShape QuadXZ();
    // 1x1 quad standing (XY plane), normal = +Z.
    static OpenGlShape QuadXY();
    // Unit box centred on the origin.
    static OpenGlShape Box();
    // Radius 1 cylinder, axis = Y, from y = 0 to y = 1 (with caps).
    static OpenGlShape Cylinder(int segments = 32);
    // Radius 1 sphere centred on the origin.
    static OpenGlShape Sphere(int stacks = 32, int slices = 48);

    void draw() const;

private:
    void upload(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void release();

    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    GLsizei m_indexCount = 0;
};
