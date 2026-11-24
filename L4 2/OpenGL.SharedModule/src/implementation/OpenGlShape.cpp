#include "OpenGlShape.hpp"

#include <glm/glm.hpp>
#include <cmath>
#include <utility>

namespace
{
    // Appends one vertex (pos, normal, uv = 8 floats).
    void pushVertex(std::vector<float>& v, const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv)
    {
        v.insert(v.end(), { p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y });
    }
}

OpenGlShape::~OpenGlShape() { release(); }

OpenGlShape::OpenGlShape(OpenGlShape&& o) noexcept
    : m_vao(o.m_vao), m_vbo(o.m_vbo), m_ebo(o.m_ebo), m_indexCount(o.m_indexCount)
{
    o.m_vao = o.m_vbo = o.m_ebo = 0;
    o.m_indexCount = 0;
}

OpenGlShape& OpenGlShape::operator=(OpenGlShape&& o) noexcept
{
    if (this != &o)
    {
        release();
        m_vao = o.m_vao; m_vbo = o.m_vbo; m_ebo = o.m_ebo; m_indexCount = o.m_indexCount;
        o.m_vao = o.m_vbo = o.m_ebo = 0;
        o.m_indexCount = 0;
    }
    return *this;
}

void OpenGlShape::release()
{
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vbo = m_ebo = 0;
}

void OpenGlShape::upload(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
{
    m_indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void OpenGlShape::draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

OpenGlShape OpenGlShape::QuadXZ()
{
    std::vector<float> v;
    const glm::vec3 n(0, 1, 0);
    pushVertex(v, { -0.5f, 0, -0.5f }, n, { 0, 1 });
    pushVertex(v, {  0.5f, 0, -0.5f }, n, { 1, 1 });
    pushVertex(v, {  0.5f, 0,  0.5f }, n, { 1, 0 });
    pushVertex(v, { -0.5f, 0,  0.5f }, n, { 0, 0 });
    OpenGlShape s;
    s.upload(v, { 0, 3, 2, 0, 2, 1 });
    return s;
}

OpenGlShape OpenGlShape::QuadXY()
{
    std::vector<float> v;
    const glm::vec3 n(0, 0, 1);
    pushVertex(v, { -0.5f, -0.5f, 0 }, n, { 0, 0 });
    pushVertex(v, {  0.5f, -0.5f, 0 }, n, { 1, 0 });
    pushVertex(v, {  0.5f,  0.5f, 0 }, n, { 1, 1 });
    pushVertex(v, { -0.5f,  0.5f, 0 }, n, { 0, 1 });
    OpenGlShape s;
    s.upload(v, { 0, 1, 2, 0, 2, 3 });
    return s;
}

OpenGlShape OpenGlShape::Box()
{
    // For every face: normal n, and two tangent axes u, v with cross(u, v) = n.
    struct Face { glm::vec3 n, u, v; };
    const Face faces[6] = {
        { { 1, 0, 0 }, { 0, 0, -1 }, { 0, 1, 0 } },
        { { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 } },
        { { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0, -1 } },
        { { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 } },
        { { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 } },
        { { 0, 0, -1 }, { -1, 0, 0 }, { 0, 1, 0 } },
    };
    std::vector<float> v;
    std::vector<unsigned int> idx;
    for (int f = 0; f < 6; ++f)
    {
        const Face& fc = faces[f];
        const unsigned int base = static_cast<unsigned int>(v.size() / 8);
        const glm::vec2 corners[4] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } };
        for (const glm::vec2& c : corners)
            pushVertex(v, fc.n * 0.5f + fc.u * c.x + fc.v * c.y, fc.n, c + glm::vec2(0.5f));
        idx.insert(idx.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }
    OpenGlShape s;
    s.upload(v, idx);
    return s;
}

OpenGlShape OpenGlShape::Cylinder(int segments)
{
    std::vector<float> v;
    std::vector<unsigned int> idx;
    const float twoPi = 6.28318530718f;

    // Side
    for (int i = 0; i <= segments; ++i)
    {
        const float t = static_cast<float>(i) / segments;
        const float a = t * twoPi;
        const glm::vec3 n(std::cos(a), 0, std::sin(a));
        pushVertex(v, { n.x, 0, n.z }, n, { t, 0 });
        pushVertex(v, { n.x, 1, n.z }, n, { t, 1 });
    }
    for (int i = 0; i < segments; ++i)
    {
        const unsigned int b0 = 2 * i, t0 = 2 * i + 1, b1 = 2 * i + 2, t1 = 2 * i + 3;
        idx.insert(idx.end(), { b0, t0, t1, b0, t1, b1 });
    }

    // Caps (top y = 1 with normal +Y, bottom y = 0 with normal -Y)
    for (int cap = 0; cap < 2; ++cap)
    {
        const float y = cap == 0 ? 1.0f : 0.0f;
        const glm::vec3 n(0, cap == 0 ? 1.0f : -1.0f, 0);
        const unsigned int center = static_cast<unsigned int>(v.size() / 8);
        pushVertex(v, { 0, y, 0 }, n, { 0.5f, 0.5f });
        for (int i = 0; i <= segments; ++i)
        {
            const float a = static_cast<float>(i) / segments * twoPi;
            pushVertex(v, { std::cos(a), y, std::sin(a) }, n,
                       { 0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a) });
        }
        for (int i = 0; i < segments; ++i)
            idx.insert(idx.end(), { center, center + 1 + i, center + 2 + i });
    }
    OpenGlShape s;
    s.upload(v, idx);
    return s;
}

OpenGlShape OpenGlShape::Sphere(int stacks, int slices)
{
    std::vector<float> v;
    std::vector<unsigned int> idx;
    const float pi = 3.14159265359f;

    for (int i = 0; i <= stacks; ++i)
    {
        const float tv = static_cast<float>(i) / stacks;      // 0 at the top pole, 1 at the bottom pole
        const float phi = tv * pi;
        for (int j = 0; j <= slices; ++j)
        {
            const float tu = static_cast<float>(j) / slices;
            const float theta = tu * 2.0f * pi;
            const glm::vec3 p(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            pushVertex(v, p, p, { tu, 1.0f - tv });
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j)
        {
            const unsigned int a = i * (slices + 1) + j;
            const unsigned int b = a + slices + 1;
            idx.insert(idx.end(), { a, b, a + 1, a + 1, b, b + 1 });
        }
    OpenGlShape s;
    s.upload(v, idx);
    return s;
}
