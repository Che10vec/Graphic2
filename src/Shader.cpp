#include "Shader.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

/// @note Documentation in this file was written with AI assistance.

/// @brief Compile a single shader stage from source code.
/// @param stage OpenGL shader stage (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.)
/// @param src GLSL source code string
/// @return Compiled shader handle, or 0 if compilation failed
static GLuint CompileShader(GLenum stage, const char *src)
{
    GLuint s = glCreateShader(stage);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint logLen = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &logLen);
        std::string log((size_t)logLen, '\0');
        glGetShaderInfoLog(s, logLen, nullptr, log.data());
        std::cerr << "Shader compile failed:\n" << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}

/// @brief Link compiled vertex and fragment shaders into a program.
/// @param vs Compiled vertex shader handle
/// @param fs Compiled fragment shader handle
/// @return Linked program handle, or 0 if linking failed
static GLuint LinkProgram(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint logLen = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &logLen);
        std::string log((size_t)logLen, '\0');
        glGetProgramInfoLog(p, logLen, nullptr, log.data());
        std::cerr << "Program link failed:\n" << log << "\n";
        glDetachShader(p, vs);
        glDetachShader(p, fs);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

/// @brief Read an entire text file into a string.
/// @param p File path to read
/// @return File contents as a string, or std::nullopt if file cannot be opened
static std::optional<std::string> ReadTextFile(const std::filesystem::path &p)
{
    std::ifstream f(p, std::ios::in | std::ios::binary);
    if (!f)
        return std::nullopt;

    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool Shader::CreateFromSource(const char *vsSrc, const char *fsSrc)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs)
    {
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        return false;
    }

    prog = LinkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog)
        return false;

    uModel = glGetUniformLocation(prog, "uModel");
    uViewProj = glGetUniformLocation(prog, "uViewProj");
    uCamPos = glGetUniformLocation(prog, "uCamPos");
    uBaseColorFactor = glGetUniformLocation(prog, "uBaseColorFactor");
    uHasBaseColorTex = glGetUniformLocation(prog, "uHasBaseColorTex");
    uBaseColorTex = glGetUniformLocation(prog, "uBaseColorTex");
    uAlphaMode = glGetUniformLocation(prog, "uAlphaMode");
    uAlphaCutoff = glGetUniformLocation(prog, "uAlphaCutoff");
    uAmbientStrength = glGetUniformLocation(prog, "uAmbientStrength");
    uLightCount = glGetUniformLocation(prog, "uLightCount");
    uLightPos = glGetUniformLocation(prog, "uLightPos");
    uLightColor = glGetUniformLocation(prog, "uLightColor");
    uLightIntensity = glGetUniformLocation(prog, "uLightIntensity");
    return true;
}

bool Shader::CreateFromFiles(const std::filesystem::path &vsPath, const std::filesystem::path &fsPath)
{
    auto vs = ReadTextFile(vsPath);
    auto fs = ReadTextFile(fsPath);
    if (!vs || !fs)
    {
        std::cerr << "Failed to read shader files:\n"
                  << "  VS: " << vsPath << "\n"
                  << "  FS: " << fsPath << "\n";
        return false;
    }

    return CreateFromSource(vs->c_str(), fs->c_str());
}

Shader::~Shader()
{
    if (prog)
    {
        GLint cur = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &cur);
        if ((GLuint)cur == prog)
            glUseProgram(0);
        glDeleteProgram(prog);
        prog = 0;
    }
}
