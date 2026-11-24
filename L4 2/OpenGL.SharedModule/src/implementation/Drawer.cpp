#include "Drawer.hpp"

#include <glm/gtc/type_ptr.hpp>

Drawer::Drawer(GLuint program) : m_program(program)
{
    m_uModel = glGetUniformLocation(program, "uModel");
    m_uView = glGetUniformLocation(program, "uView");
    m_uProj = glGetUniformLocation(program, "uProj");
    m_uNormalMat = glGetUniformLocation(program, "uNormalMat");
    m_uUvScale = glGetUniformLocation(program, "uUvScale");
    m_uMode = glGetUniformLocation(program, "uMode");
    m_uTint = glGetUniformLocation(program, "uTint");
    m_uSpec = glGetUniformLocation(program, "uSpec");
    m_uShine = glGetUniformLocation(program, "uShine");
    m_uTex = glGetUniformLocation(program, "uTex");
    m_uViewPos = glGetUniformLocation(program, "uViewPos");
    m_uTime = glGetUniformLocation(program, "uTime");
    m_uLightPos = glGetUniformLocation(program, "uLightPos");
    m_uLightColor = glGetUniformLocation(program, "uLightColor");
    m_uAmbient = glGetUniformLocation(program, "uAmbient");
}

void Drawer::beginFrame(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &eye, float time, float tableLightScale)
{
    glUseProgram(m_program);
    glUniformMatrix4fv(m_uView, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(m_uProj, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(m_uViewPos, 1, glm::value_ptr(eye));
    glUniform1f(m_uTime, time);

    const glm::vec3 lightPos[2] = {glm::vec3(0.0f, 3.6f, 1.2f), glm::vec3(0.0f, 5.5f, -5.0f)};
    const glm::vec3 lightColor[2] = {
        glm::vec3(1.00f, 0.92f, 0.78f) * (2.40f * tableLightScale),
        glm::vec3(0.75f, 0.72f, 0.85f) * 0.10f};
    glUniform3fv(m_uLightPos, 2, glm::value_ptr(lightPos[0]));
    glUniform3fv(m_uLightColor, 2, glm::value_ptr(lightColor[0]));
    glUniform3f(m_uAmbient, 0.045f, 0.042f, 0.045f);
    glUniform1i(m_uTex, 0);
}

void Drawer::draw(const OpenGlShape &shape, const glm::mat4 &model, const Material &m) const
{
    const glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix4fv(m_uModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(m_uNormalMat, 1, GL_FALSE, glm::value_ptr(normalMat));
    glUniform2fv(m_uUvScale, 1, glm::value_ptr(m.uvScale));
    glUniform1i(m_uMode, static_cast<int>(m.mode));
    glUniform4fv(m_uTint, 1, glm::value_ptr(m.tint));
    glUniform1f(m_uSpec, m.specular);
    glUniform1f(m_uShine, m.shininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m.texture);
    shape.draw();
}
