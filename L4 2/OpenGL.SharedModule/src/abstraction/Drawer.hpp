#pragma once
// Drawer: owns the shader program and knows how to draw an OpenGlShape
// with a model matrix + a Material (texture, tint, shading mode...).
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "OpenGlShape.hpp"

// How the fragment shader treats the surface (must match the numbers in L4.fs).
enum class ShadeMode : int
{
    Lit = 0,     // textured + Blinn-Phong lighting
    Card = 1,    // rounded corners + golden frame + lit
    Poster = 2,  // lit paper that is procedurally torn at the edges
    Unlit = 3,   // texture * tint, alpha-blended (zone slots, mats outlines)
    FaceDown = 4 // hidden card back with ornamental pattern + golden frame
};

struct Material
{
    GLuint texture = 0;
    glm::vec4 tint = glm::vec4(1.0f);
    glm::vec2 uvScale = glm::vec2(1.0f);
    ShadeMode mode = ShadeMode::Lit;
    float specular = 0.15f;
    float shininess = 32.0f;
};

class Drawer
{
public:
    explicit Drawer(GLuint program);

    // Call once per frame before any draw().
    // Call once per frame before any draw().
    void beginFrame(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &eye, float time, float tableLightScale = 1.0f);
    void draw(const OpenGlShape &shape, const glm::mat4 &model, const Material &material) const;

private:
    GLuint m_program = 0;
    GLint m_uModel = -1, m_uView = -1, m_uProj = -1, m_uNormalMat = -1;
    GLint m_uUvScale = -1, m_uMode = -1, m_uTint = -1, m_uSpec = -1, m_uShine = -1, m_uTex = -1;
    GLint m_uViewPos = -1, m_uTime = -1;
    GLint m_uLightPos = -1, m_uLightColor = -1, m_uAmbient = -1;
};
