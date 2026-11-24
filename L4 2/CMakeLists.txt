cmake_minimum_required(VERSION 3.16)
project(L4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(SHARED_DIR ${CMAKE_SOURCE_DIR}/OpenGL.SharedModule)

# ---------------------------------------------------------------------------
#  OpenGL.SharedModule  (static library)
# ---------------------------------------------------------------------------
add_library(OpenGLSharedModule STATIC
    ${SHARED_DIR}/src/glad.c
    ${SHARED_DIR}/src/stb_image.cpp
    ${SHARED_DIR}/src/implementation/SharedModule.cpp
    ${SHARED_DIR}/src/implementation/Drawer.cpp
    ${SHARED_DIR}/src/implementation/OpenGlShape.cpp
    ${SHARED_DIR}/src/implementation/Camera.cpp
)

target_include_directories(OpenGLSharedModule PUBLIC
    ${SHARED_DIR}/src/abstraction
    ${SHARED_DIR}/src
    ${SHARED_DIR}/dependencies/include      # glad, KHR, stb_image.h, GLFW, glm ...
)

# Textures / shaders are looked up relative to GLProject/ (so you can run the exe from anywhere)
target_compile_definitions(OpenGLSharedModule PUBLIC ASSET_ROOT="${CMAKE_SOURCE_DIR}/GLProject/")

if(WIN32)
    # GLFW import/static library lives in OpenGL.SharedModule/dependencies/libs  (glfw3.lib or glfw3dll.lib)
    find_library(GLFW_LIBRARY NAMES glfw3 glfw3dll glfw libglfw3.a
             PATHS ${SHARED_DIR}/dependencies/libs NO_DEFAULT_PATH)
    if(NOT GLFW_LIBRARY)
        message(FATAL_ERROR "glfw3.lib not found in ${SHARED_DIR}/dependencies/libs")
    endif()
    target_link_libraries(OpenGLSharedModule PUBLIC ${GLFW_LIBRARY} opengl32)
else()
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(OpenGLSharedModule PUBLIC glfw OpenGL::GL ${CMAKE_DL_LIBS})
endif()

# ---------------------------------------------------------------------------
#  GLProject  (executable)
# ---------------------------------------------------------------------------
add_executable(GLProject GLProject/Main.cpp)
target_link_libraries(GLProject PRIVATE OpenGLSharedModule)

# Optional: scripted self-test that saves screenshots (cmake -DSELFTEST=ON ..)
option(SELFTEST "Build a scripted self-test run" OFF)
if(SELFTEST)
    target_compile_definitions(GLProject PRIVATE SELFTEST)
endif()