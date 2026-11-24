#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CameraPose
{
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up;
};

class Camera
{
public:
    Camera(const CameraPose& topPose, const CameraPose& playerPose, float blendDuration = 0.9f);

    void setTargetPose(int poseIndex); // 0 = top view, 1 = player view
    int getTargetPose() const { return targetPose_; }
    float getBlend() const { return blend_; }

    void update(float dt);
    glm::mat4 getViewMatrix(glm::vec3& eyeOut) const;

    // دوال معالجة الإدخال وتغيير حجم النافذة وفق المحاضرة الرابعة
    void processInput(GLFWwindow* window, int key);
    void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
    void framebuffer_size_callback(GLFWwindow* window, int width, int height);

private:
    static float smoothstep01(float t);
    static float moveToward(float current, float target, float maxDelta);

    CameraPose topPose_;
    CameraPose playerPose_;
    float blendDuration_;

    int targetPose_ = 0;
    float blend_ = 0.0f;

    bool firstMouse_ = true;
    double lastX_ = 0.0;
    double lastY_ = 0.0;
    float yawOffset_ = 0.0f;
    float pitchOffset_ = 0.0f;
};