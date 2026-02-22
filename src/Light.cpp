#include "Light.h"

#include <iostream>

/// @note Documentation in this file was written with AI assistance.

/// @brief Create default point lights used by the scene.
/// Returns three lights in an equilateral triangle layout above ground.
/// @return Vector of default scene lights.
std::vector<SceneLight> CreateExampleLightsLayout()
{
    return {
        SceneLight{glm::vec3(0.0f, 3.0f, 4.0f), glm::vec3(1.00f, 0.22f, 0.22f), 2.2f},
        SceneLight{glm::vec3(-3.4641f, 3.0f, -2.0f), glm::vec3(0.18f, 1.00f, 0.22f), 2.0f},
        SceneLight{glm::vec3(3.4641f, 3.0f, -2.0f), glm::vec3(0.22f, 0.40f, 1.00f), 1.8f},
    };
}

/// @brief Initialize marker rendering resources for light visualization.
/// Loads marker shaders, creates a VAO for point drawing, and caches uniforms.
/// @param projectRoot Project root path used to resolve marker shader files.
/// @return true if initialization succeeds, false otherwise.
bool LightMarker::Initialize(const std::filesystem::path &projectRoot)
{
    const std::filesystem::path vsPath = projectRoot / "src" / "shaders" / "light_marker.vert";
    const std::filesystem::path fsPath = projectRoot / "src" / "shaders" / "light_marker.frag";

    if (!shader.CreateFromFiles(vsPath, fsPath))
    {
        std::cerr << "Failed to create light marker shader program\n";
        return false;
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);

    uViewProj = glGetUniformLocation(shader.prog, "uViewProj");
    uCenter = glGetUniformLocation(shader.prog, "uCenter");
    uPointSize = glGetUniformLocation(shader.prog, "uPointSize");
    uColor = glGetUniformLocation(shader.prog, "uColor");
    glEnable(GL_PROGRAM_POINT_SIZE);
    return true;
}

/// @brief Draw visible point markers for all provided lights.
/// @param viewProj Combined view-projection matrix.
/// @param lights Scene lights to visualize.
void LightMarker::Draw(const glm::mat4 &viewProj, const std::vector<SceneLight> &lights) const
{
    shader.Use();
    glUniformMatrix4fv(uViewProj, 1, GL_FALSE, &viewProj[0][0]);
    glUniform1f(uPointSize, 12.0f);
    glBindVertexArray(vao);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    for (const SceneLight &light : lights)
    {
        glUniform3fv(uCenter, 1, &light.position[0]);
        glm::vec3 markerColor = light.color * (0.4f + 0.25f * light.intensity);
        glUniform3fv(uColor, 1, &markerColor[0]);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    glBindVertexArray(0);
}

/// @brief Release marker GPU resources created during initialization.
void LightMarker::Shutdown()
{
    if (vao)
        glDeleteVertexArrays(1, &vao);
    vao = 0;
}
