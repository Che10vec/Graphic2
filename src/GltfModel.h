#pragma once

#include "Shader.h"
#include "Camera.h"

#include <glad/glad.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <tiny_gltf.h>

/// @note Documentation in this file was written with AI assistance.

/// @struct PrimitiveGPU
/// @brief A renderable mesh primitive with GPU resources and source glTF lookup.
/// Contains the OpenGL vertex array, vertex buffer, and index buffer objects
/// for a single mesh primitive, along with indices that map back to tinygltf mesh data.
struct PrimitiveGPU
{
    /// OpenGL Vertex Array Object handle
    GLuint vao = 0;
    /// OpenGL Vertex Buffer Object handle
    GLuint vbo = 0;
    /// OpenGL Element Buffer Object (index buffer) handle
    GLuint ebo = 0;
    /// Number of indices to draw
    GLsizei indexCount = 0;
    /// Index into tinygltf::Model::meshes
    int meshIndex = -1;
    /// Index into tinygltf::Mesh::primitives for meshIndex
    int meshPrimitiveIndex = -1;
};

/// @struct ModelGPUResources
/// @brief GPU-side resources built from a tinygltf model.
/// Contains OpenGL textures and primitive buffers only; glTF scene/material data stays
/// in tinygltf::Model to avoid redundant CPU-side duplication.
struct ResourcesGPU
{
    /// OpenGL texture handles loaded from model textures
    std::vector<GLuint> glTextures;
    /// Renderable primitives with GPU resources
    std::vector<PrimitiveGPU> primitives;
    /// Maps mesh indices to their primitive indices for quick lookup
    std::vector<std::vector<int>> meshToPrimitiveIndices;
    
    /// @brief Release all OpenGL resources associated with this model.
    /// Destroys textures, VAOs, VBOs, and EBOs. Should be called before destruction.
    void DestroyGL();
};

/// @struct SceneModel
/// @brief Scene-level container for one loaded glTF model and user transform controls.
/// Keeps file path, CPU glTF data, GPU resources, and user transform settings together.
struct SceneModel
{
    /// @brief Human-readable model label shown in the UI.
    std::string label;
    /// @brief Absolute path to the model file on disk.
    std::filesystem::path filePath;
    /// @brief CPU-side glTF representation used for scene/material traversal.
    tinygltf::Model gltf;
    /// @brief GPU buffers/textures associated with this model.
    ResourcesGPU gpu;
    /// @brief User translation offset in world space.
    glm::vec3 userT{0, 0, 0};
    /// @brief User Euler rotation in degrees.
    glm::vec3 userRdeg{0, 0, 0};
    /// @brief User scale multiplier.
    glm::vec3 userS{1, 1, 1};
};

/// @brief Load a glTF model file into tinygltf::Model.
/// @param outModel tinygltf model to populate
/// @param path File path to the glTF model file (.gltf or .glb)
/// @return true if the model was loaded successfully, false otherwise
bool LoadGLTFModel(tinygltf::Model &outModel, const std::filesystem::path &path);

/// @brief Build OpenGL resources from a loaded tinygltf::Model.
/// @param model Source tinygltf model
/// @param outGPU GPU resources to populate
/// @return true if GPU resources were built successfully, false otherwise
bool BuildGLTFGPUResources(const tinygltf::Model &model, ResourcesGPU &outGPU);

/// @brief Convenience helper to load a glTF file and build GPU resources.
/// @param outModel tinygltf model to populate
/// @param outGPU GPU resources to populate
/// @param path File path to the glTF model file (.gltf or .glb)
/// @return true on complete success, false otherwise
bool LoadGLTFToGPU(tinygltf::Model &outModel,
                   ResourcesGPU &outGPU,
                   const std::filesystem::path &path);

/// @brief Render a tinygltf model using prebuilt GPU resources.
/// Traverses the glTF scene hierarchy directly from tinygltf::Model.
/// @param model The tinygltf model to render
/// @param gpu The GPU resources for this model
/// @param sh Shader program to use for rendering
/// @param viewProj Combined view-projection matrix
/// @param camPos Camera position in world space (for lighting calculations)
/// @param ambientStrength Ambient lighting strength multiplier
/// @param lightPositions World-space positions for all active point lights
/// @param lightColors RGB colors for all active point lights
/// @param lightIntensities Scalar intensities for all active point lights
/// @param modelTransform User transform applied to the rendered model
void DrawModel(const tinygltf::Model &model,
               const ResourcesGPU &gpu,
               const Shader &sh,
               const glm::mat4 &viewProj,
               const glm::vec3 &camPos,
               float ambientStrength,
               const std::array<glm::vec3, kMaxLights> &lightPositions,
               const std::array<glm::vec3, kMaxLights> &lightColors,
               const std::array<float, kMaxLights> &lightIntensities,
               const glm::mat4 &modelTransform);
