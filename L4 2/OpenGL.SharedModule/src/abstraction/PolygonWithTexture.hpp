#pragma once

#include "Drawer.hpp"
#include "OpenGlShape.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>

class PolygonWithTexture
{
public:
    PolygonWithTexture(const OpenGlShape &shape, GLuint texture = 0, const glm::vec4 &tint = glm::vec4(1.0f))
        : shape_(shape)
    {
        material_.texture = texture;
        material_.tint = tint;
        material_.uvScale = glm::vec2(1.0f, 1.0f);
        material_.specular = 0.25f;
        material_.shininess = 48.0f;
        material_.mode = ShadeMode::Card;
    }

    void setTexture(GLuint texture)
    {
        material_.texture = texture;
    }

    void setTint(const glm::vec4 &tint)
    {
        material_.tint = tint;
    }

    void setUvScale(const glm::vec2 &uvScale)
    {
        material_.uvScale = uvScale;
    }

    void setMaterialParams(float specular, float shininess, ShadeMode mode)
    {
        material_.specular = specular;
        material_.shininess = shininess;
        material_.mode = mode;
    }

    void transformation(const glm::mat4 &modelMatrix)
    {
        model_ = modelMatrix;
    }

    void draw(const Drawer &drawer) const
    {
        drawer.draw(shape_, model_, material_);
    }

private:
    const OpenGlShape &shape_;
    glm::mat4 model_{1.0f};
    Material material_{};
};