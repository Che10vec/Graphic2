#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>

/// @note Documentation in this file was written with AI assistance.

/// @struct Camera
/// @brief Represents a 3D camera with position, rotation, and projection settings.
/// This structure manages camera properties including position, orientation (yaw/pitch),
/// projection blend, and movement/mouse sensitivity parameters. It provides methods to
/// compute view and projection matrices for rendering.
struct Camera
{
    /// @brief Camera position in world space
    glm::vec3 pos{0.0f, 0.5f, 3.0f};
    /// @brief Yaw rotation in degrees (horizontal angle)
    float yawDeg = -90.0f; 
    /// @brief Pitch rotation in degrees (vertical angle)
    float pitchDeg = 0.0f;
    /// @brief Blend factor from perspective (0.0) to orthographic (1.0)
    float projectionBlend = 0.0f;

    /// @brief Movement speed for keyboard input (units per second)
    float moveSpeed = 3.5f;
    /// @brief Mouse sensitivity multiplier
    float mouseSens = 0.12f;

    /// @brief Compute the forward direction vector based on yaw and pitch.
    /// @return Normalized forward direction vector
    glm::vec3 Forward() const
    {
        float yaw = glm::radians(yawDeg);
        float pitch = glm::radians(pitchDeg);
        glm::vec3 f;
        f.x = cosf(yaw) * cosf(pitch);
        f.y = sinf(pitch);
        f.z = sinf(yaw) * cosf(pitch);
        return glm::normalize(f);
    }

    /// @brief Compute the right direction vector perpendicular to forward.
    /// @return Normalized right direction vector
    glm::vec3 Right() const
    {
        return glm::normalize(glm::cross(Forward(), glm::vec3(0, 1, 0)));
    }

    /// @brief Compute the view matrix for this camera.
    /// @return 4x4 view transformation matrix
    glm::mat4 View() const
    {
        return glm::lookAt(pos, pos + Forward(), glm::vec3(0, 1, 0));
    }

    /// @brief Handle a mouse-wheel scroll event by blending toward orthographic projection.
    /// @param xoffset Horizontal scroll delta (ignored by this implementation).
    /// @param yoffset Vertical scroll delta (positive typically means wheel up).
    /// @note 0.0 is full perspective and 1.0 is full orthographic.
    void HandleScroll(double xoffset, double yoffset)
    {
        (void)xoffset;
        const float blendStep = 0.08f;
        projectionBlend = glm::clamp(projectionBlend + (float)yoffset * blendStep, 0.0f, 1.0f);
    }

    /// @brief GLFW-compatible scroll callback that forwards to the `Camera` instance
    /// stored in the GLFW window user pointer.
    /// @param window The GLFW window whose user pointer must be a `Camera*`.
    /// @param xoffset Horizontal scroll delta passed by GLFW.
    /// @param yoffset Vertical scroll delta passed by GLFW.
    /// @note Scroll is handled only when mouse capture is enabled
    /// (`glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)`).
    /// Register with: glfwSetScrollCallback(window, Camera::GLFWScrollCallback);
    static inline void GLFWScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
            return;

        Camera *cam = static_cast<Camera *>(glfwGetWindowUserPointer(window));
        if (cam)
            cam->HandleScroll(xoffset, yoffset);
    }

    /// @brief Compute the projection matrix for this camera.
    /// @param aspect The aspect ratio (width/height) of the viewport
    /// @return 4x4 blended projection matrix
    glm::mat4 Projection(float aspect) const
    {
        constexpr float kVerticalFovDeg = 60.0f;
        const float nearPlane = 0.05f;
        const float farPlane = 500.0f;

        glm::mat4 perspective = glm::perspective(glm::radians(kVerticalFovDeg), aspect, nearPlane, farPlane);

        const float orthoReferenceDistance = 6.0f;
        const float orthoHalfHeight = tanf(glm::radians(kVerticalFovDeg * 0.5f)) * orthoReferenceDistance;
        const float orthoHalfWidth = orthoHalfHeight * aspect;
        glm::mat4 ortho = glm::ortho(-orthoHalfWidth, orthoHalfWidth,
                                     -orthoHalfHeight, orthoHalfHeight,
                                     nearPlane, farPlane);

        return perspective * (1.0f - projectionBlend) + ortho * projectionBlend;
    }
};

/// @brief Construct a transformation matrix from translation, rotation, and scale.
/// @param t Translation vector
/// @param rDeg Rotation angles in degrees (X, Y, Z axes)
/// @param s Scale factors (X, Y, Z axes)
/// @return 4x4 transformation matrix (TRS order)
inline glm::mat4 TRS(const glm::vec3 &t, const glm::vec3 &rDeg, const glm::vec3 &s)
{
    glm::mat4 m(1.0f);
    m = glm::translate(m, t);
    m = m * glm::toMat4(glm::quat(glm::radians(rDeg)));
    m = glm::scale(m, s);
    return m;
}
