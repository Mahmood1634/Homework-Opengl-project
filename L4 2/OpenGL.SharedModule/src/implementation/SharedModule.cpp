#include "SharedModule.hpp"

#include <stb_image.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifndef ASSET_ROOT
#define ASSET_ROOT "./"
#endif

namespace SharedModule
{
    std::string assetPath(const std::string& relativePath)
    {
        return std::string(ASSET_ROOT) + relativePath;
    }

    GLFWwindow* createWindow(int width, int height, const std::string& title)
    {
        if (!glfwInit())
        {
            std::fprintf(stderr, "Failed to initialise GLFW\n");
            return nullptr;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

        GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window)
        {
            std::fprintf(stderr, "Failed to create the GLFW window\n");
            glfwTerminate();
            return nullptr;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::fprintf(stderr, "Failed to initialise glad\n");
            return nullptr;
        }
        return window;
    }

    static std::string readFile(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            std::fprintf(stderr, "Cannot open file: %s\n", path.c_str());
            std::exit(EXIT_FAILURE);
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    static GLuint compile(GLenum type, const std::string& source, const std::string& name)
    {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[2048];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader compile error (%s):\n%s\n", name.c_str(), log);
            std::exit(EXIT_FAILURE);
        }
        return shader;
    }

    GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
    {
        GLuint vs = compile(GL_VERTEX_SHADER, readFile(assetPath(vertexPath)), vertexPath);
        GLuint fs = compile(GL_FRAGMENT_SHADER, readFile(assetPath(fragmentPath)), fragmentPath);

        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint ok = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[2048];
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Program link error:\n%s\n", log);
            std::exit(EXIT_FAILURE);
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
        return program;
    }

    GLuint loadTexture(const std::string& relativePath, bool repeat)
    {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        stbi_set_flip_vertically_on_load(true); // OpenGL's v = 0 is the bottom of the image
        int w = 0, h = 0, channels = 0;
        unsigned char* data = stbi_load(assetPath(relativePath).c_str(), &w, &h, &channels, 0);

        if (data)
        {
            GLenum format = channels == 4 ? GL_RGBA : (channels == 3 ? GL_RGB : (channels == 2 ? GL_RG : GL_RED));
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
        }
        else
        {
            std::fprintf(stderr, "WARNING: could not load texture '%s' (using magenta placeholder)\n",
                         relativePath.c_str());
            const unsigned char magenta[3] = { 255, 0, 255 };
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, magenta);
        }

        const GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return tex;
    }
}
