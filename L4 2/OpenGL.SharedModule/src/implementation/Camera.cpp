#include "Camera.hpp"

#include <algorithm>
#include <cmath>

Camera::Camera(const CameraPose &topPose, const CameraPose &playerPose, float blendDuration)
    : topPose_(topPose), playerPose_(playerPose), blendDuration_(std::max(blendDuration, 0.0001f))
{
}

float Camera::smoothstep01(float t)
{
    t = glm::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float Camera::moveToward(float current, float target, float maxDelta)
{
    if (std::fabs(target - current) <= maxDelta)
        return target;
    return current + (target > current ? maxDelta : -maxDelta);
}

void Camera::setTargetPose(int poseIndex)
{
    if (poseIndex != 0 && poseIndex != 1)
        return;
    targetPose_ = poseIndex;
}

void Camera::update(float dt)
{
    blend_ = moveToward(blend_, static_cast<float>(targetPose_), dt / blendDuration_);
}

glm::mat4 Camera::getViewMatrix(glm::vec3 &eyeOut) const
{
    const float t = smoothstep01(blend_);
    eyeOut = glm::mix(topPose_.eye, playerPose_.eye, t);

    glm::vec3 target = glm::mix(topPose_.target, playerPose_.target, t);
    const glm::vec3 up = glm::normalize(glm::mix(topPose_.up, playerPose_.up, t));

    if (yawOffset_ != 0.0f || pitchOffset_ != 0.0f)
    {
        const glm::vec3 dir = target - eyeOut;
        const glm::mat4 rotYaw = glm::rotate(glm::mat4(1.0f), glm::radians(yawOffset_), up);
        const glm::vec3 right = glm::normalize(glm::cross(dir, up));
        const glm::mat4 rotPitch = glm::rotate(glm::mat4(1.0f), glm::radians(pitchOffset_), right);
        target = eyeOut + glm::vec3(rotYaw * rotPitch * glm::vec4(dir, 0.0f));
    }

    return glm::lookAt(eyeOut, target, up);
}

void Camera::processInput(GLFWwindow *window, int key)
{
    if (!window)
        return;
    if (key == GLFW_KEY_1)
    {
        setTargetPose(0);
        yawOffset_ = 0.0f;
        pitchOffset_ = 0.0f;
    }
    else if (key == GLFW_KEY_2)
    {
        setTargetPose(1);
        yawOffset_ = 0.0f;
        pitchOffset_ = 0.0f;
    }
}

void Camera::mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    if (firstMouse_)
    {
        lastX_ = xposIn;
        lastY_ = yposIn;
        firstMouse_ = false;
        return;
    }

    const double dx = xposIn - lastX_;
    const double dy = lastY_ - yposIn;
    lastX_ = xposIn;
    lastY_ = yposIn;

    if (window && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        constexpr float sensitivity = 0.08f;
        yawOffset_ = glm::clamp(yawOffset_ - static_cast<float>(dx) * sensitivity, -15.0f, 15.0f);
        pitchOffset_ = glm::clamp(pitchOffset_ + static_cast<float>(dy) * sensitivity, -12.0f, 12.0f);
    }
}

void Camera::framebuffer_size_callback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, std::max(width, 1), std::max(height, 1));
}