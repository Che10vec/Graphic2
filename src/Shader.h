#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <string>

/// @brief Maximum number of dynamic point lights supported by the shader.
constexpr int kMaxLights = 3;

/// @note Documentation in this file was written with AI assistance.

/// @struct Shader
/// @brief Manages OpenGL shader programs and uniform locations.
/// This structure encapsulates an OpenGL shader program and caches the locations
/// of commonly used uniforms (model/view/projection matrices, material properties, etc.).
/// It provides methods to create shaders from source code or files, and to activate
/// the shader program for rendering.
struct Shader
{
    /// @brief OpenGL shader program handle
    GLuint prog = 0;
    /// @brief Uniform location for model matrix
    GLint uModel = -1;
    /// @brief Uniform location for view-projection matrix
    GLint uViewProj = -1;
    /// @brief Uniform location for camera position
    GLint uCamPos = -1;
    /// @brief Uniform location for base color factor
    GLint uBaseColorFactor = -1;
    /// @brief Uniform location for base color texture presence flag
    GLint uHasBaseColorTex = -1;
    /// @brief Uniform location for base color texture sampler
    GLint uBaseColorTex = -1;
    /// @brief Uniform location for alpha blending mode
    GLint uAlphaMode = -1;
    /// @brief Uniform location for alpha cutoff threshold
    GLint uAlphaCutoff = -1;
    /// @brief Uniform location for ambient strength
    GLint uAmbientStrength = -1;
    /// @brief Uniform location for light count
    GLint uLightCount = -1;
    /// @brief Uniform location for light positions array
    GLint uLightPos = -1;
    /// @brief Uniform location for light colors array
    GLint uLightColor = -1;
    /// @brief Uniform location for light intensities array
    GLint uLightIntensity = -1;
    
    /// @brief Create a shader program from GLSL source code strings.
    /// @param vsSrc Vertex shader source code
    /// @param fsSrc Fragment shader source code
    /// @return true if compilation and linking succeeded, false otherwise
    bool CreateFromSource(const char *vsSrc, const char *fsSrc);
    
    /// @brief Create a shader program from GLSL source files.
    /// @param vsPath Path to the vertex shader source file
    /// @param fsPath Path to the fragment shader source file
    /// @return true if files were loaded, compiled, and linked successfully, false otherwise
    bool CreateFromFiles(const std::filesystem::path &vsPath, const std::filesystem::path &fsPath);
    
    /// @brief Destructor that releases OpenGL shader program resources.
    ~Shader();
    
    /// @brief Activate this shader program for rendering.
    void Use() const { glUseProgram(prog); }
};
