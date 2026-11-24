#pragma once
// SharedModule: small helpers shared by every lecture project
// (window creation, shader compilation, texture loading, asset paths).
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

namespace SharedModule
{
    // Full path of a file inside GLProject/ (ASSET_ROOT is injected by CMake).
    std::string assetPath(const std::string& relativePath);

    // Creates a GLFW window with an OpenGL 3.3 core context and loads glad. Returns nullptr on failure.
    GLFWwindow* createWindow(int width, int height, const std::string& title);

    // Compiles + links a vertex and a fragment shader (paths relative to GLProject/). Exits on error.
    GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);

    // Loads an image (path relative to GLProject/) into a 2D texture with mipmaps.
    // If the file is missing a magenta 1x1 texture is returned so the program still runs.
    GLuint loadTexture(const std::string& relativePath, bool repeat = true);
}
