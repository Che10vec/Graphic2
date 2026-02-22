#pragma once

#include "Shader.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <vector>

/// @note Documentation in this file was written with AI assistance.

/// @struct SceneLight
/// @brief Editable point light settings used by the shader and marker renderer.
struct SceneLight
{
    /// @brief Light position in world space.
    glm::vec3 position;
    /// @brief Light color multiplier (RGB).
    glm::vec3 color;
    /// @brief Scalar brightness/intensity for this light.
    float intensity;
};

/// @brief Create default scene lights.
/// Returns three lights in an equilateral triangle parallel to the ground.
/// @return Default scene light array.
std::vector<SceneLight> CreateExampleLightsLayout();

/// @struct LightMarker
/// @brief Renders simple point markers at scene light positions.
struct LightMarker
{
    /// @brief Marker shader program.
    Shader shader;
    /// @brief OpenGL Vertex Array Object for point-marker draw calls.
    GLuint vao = 0;
    /// @brief Uniform location for view-projection matrix.
    GLint uViewProj = -1;
    /// @brief Uniform location for marker center position.
    GLint uCenter = -1;
    /// @brief Uniform location for marker point size.
    GLint uPointSize = -1;
    /// @brief Uniform location for marker color.
    GLint uColor = -1;

    /// @brief Initialize marker shader and minimal GPU resources.
    /// @param projectRoot Project root path used to resolve marker shader files.
    /// @return true if initialization succeeded, false otherwise.
    bool Initialize(const std::filesystem::path &projectRoot);

    /// @brief Draw one marker point for each scene light.
    /// @param viewProj Combined view-projection matrix.
    /// @param lights Scene light array to visualize.
    void Draw(const glm::mat4 &viewProj, const std::vector<SceneLight> &lights) const;

    /// @brief Release marker GPU resources.
    void Shutdown();
};
