#pragma once

#include <glad/glad.h>

#include <iostream>

/// @note Documentation in this file was written with AI assistance.

/// @brief GLFW error callback for handling window/context errors.
/// @param error The error code from GLFW.
/// @param description Human-readable error message.
inline void GlfwErrorCallback(int error, const char *description)
{
    std::cerr << "[GLFW] Error " << error << ": " << description << "\n";
}

/// @brief OpenGL debug callback for printing debug/error messages from the GPU.
/// @param source Source of the debug message (driver, application, etc.).
/// @param type Type of message (error, warning, notification, etc.).
/// @param id Unique identifier for the message.
/// @param severity Severity level of the message.
/// @param length Length of the message string.
/// @param message The debug message string.
/// @param userParam User-defined pointer (unused).
inline void APIENTRY GlDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                     GLsizei length, const GLchar *message, const void *userParam)
{
    (void)source;
    (void)type;
    (void)id;
    (void)severity;
    (void)length;
    (void)userParam;
    std::cerr << "[GL] " << message << "\n";
}
