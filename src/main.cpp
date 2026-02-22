#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "AppCallbacks.h"
#include "Camera.h"
#include "GltfModel.h"
#include "Light.h"
#include "Shader.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <filesystem>
#include <iostream>
#include <vector>

/// @note Documentation in this file was written with AI assistance.

/// @brief Main application entry point.
/// Initializes OpenGL, creates a window, loads glTF models, and runs the main
/// rendering loop with ImGui UI for model manipulation and free camera controls.
/// @param argc Command-line argument count
/// @param argv Command-line arguments
/// @return Exit code (0 on success)
int main(int argc, char **argv)
{
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit())
        return 1;

    // OpenGL 3.3 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    GLFWwindow *window = glfwCreateWindow(1600, 900, "Two gltf cars", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        glfwTerminate();
        return 1;
    }

    if (glDebugMessageCallback)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(GlDebugCallback, nullptr);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::filesystem::path root = PROJECT_ROOT_DIR;
    const std::filesystem::path vsPath = root / "src" / "shaders" / "basic.vert";
    const std::filesystem::path fsPath = root / "src" / "shaders" / "basic.frag";

    Shader shader;
    if (!shader.CreateFromFiles(vsPath, fsPath))
    {
        std::cerr << "Failed to create shader program\n";
        return 1;
    }

    LightMarker lightMarkerRenderer;
    if (!lightMarkerRenderer.Initialize(root))
    {
        std::cerr << "Failed to initialize light marker renderer\n";
        return 1;
    }

    std::filesystem::path model_1_Path = root / "assets" / "130" / "scene.gltf";
    std::filesystem::path model_2_Path = root / "assets" / "vette" / "scene.gltf";

    SceneModel model_1, model_2;
    model_1.filePath = model_1_Path;
    model_2.filePath = model_2_Path;
    model_1.label = model_1_Path.filename().string();
    model_2.label = model_2_Path.filename().string();

    if (!LoadGLTFToGPU(model_1.gltf, model_1.gpu, model_1.filePath))
    {
        std::cerr << "Failed to load: " << model_1_Path << "\n";
        return 1;
    }
    if (!LoadGLTFToGPU(model_2.gltf, model_2.gpu, model_2.filePath))
    {
        std::cerr << "Failed to load: " << model_2_Path << "\n";
        return 1;
    }

    model_1.userT = glm::vec3(-3.0f, 0.0f, 0.0f);
    model_2.userT = glm::vec3(3.0f, 0.0f, 0.0f);

    Camera cam;
    glfwSetWindowUserPointer(window, &cam);
    glfwSetScrollCallback(window, Camera::GLFWScrollCallback);

    bool captureMouse = false;
    bool prevC = false;

    double lastX = 0.0, lastY = 0.0;
    bool firstMouse = true;

    float lastTime = (float)glfwGetTime();
    float ambientStrength = 0.12f;

    std::vector<SceneLight> lights = CreateExampleLightsLayout();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        bool cDown = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        if (cDown && !prevC)
        {
            captureMouse = !captureMouse;
            glfwSetInputMode(window, GLFW_CURSOR, captureMouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            firstMouse = true;
        }
        prevC = cDown;

        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        if (captureMouse)
        {
            glm::vec3 fwd = cam.Forward();
            glm::vec3 right = cam.Right();
            glm::vec3 up(0, 1, 0);

            float speed = cam.moveSpeed;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                cam.pos += fwd * speed * dt;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                cam.pos -= fwd * speed * dt;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                cam.pos -= right * speed * dt;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                cam.pos += right * speed * dt;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                cam.pos += up * speed * dt;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
                cam.pos -= up * speed * dt;

            double x, y;
            glfwGetCursorPos(window, &x, &y);
            if (firstMouse)
            {
                lastX = x;
                lastY = y;
                firstMouse = false;
            }

            float dx = (float)(x - lastX);
            float dy = (float)(lastY - y); // invert Y
            lastX = x;
            lastY = y;

            cam.yawDeg += dx * cam.mouseSens;
            cam.pitchDeg += dy * cam.mouseSens;
            cam.pitchDeg = glm::clamp(cam.pitchDeg, -89.0f, 89.0f);
        }

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        float aspect = (h > 0) ? (float)w / (float)h : 1.0f;

        glViewport(0, 0, w, h);
        float clearGray = glm::clamp(ambientStrength, 0.0f, 1.0f);
        glClearColor(clearGray, clearGray, clearGray, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 eyePos = cam.EyePosition();
        glm::mat4 viewProj = cam.Projection(aspect) * cam.View();

        std::array<glm::vec3, kMaxLights> lightPositions;
        std::array<glm::vec3, kMaxLights> lightColors;
        std::array<float, kMaxLights> lightIntensities;
        for (int i = 0; i < kMaxLights; i++)
        {
            lightPositions[i] = lights[i].position;
            lightColors[i] = lights[i].color;
            lightIntensities[i] = lights[i].intensity;
        }

          DrawModel(model_1.gltf, model_1.gpu, shader, viewProj, eyePos,
              ambientStrength,
              lightPositions, lightColors, lightIntensities,
              TRS(model_1.userT, model_1.userRdeg, model_1.userS));
          DrawModel(model_2.gltf, model_2.gpu, shader, viewProj, eyePos,
              ambientStrength,
              lightPositions, lightColors, lightIntensities,
              TRS(model_2.userT, model_2.userRdeg, model_2.userS));

        lightMarkerRenderer.Draw(viewProj, lights);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Controls");

        ImGui::Text("Camera:");
        ImGui::Text("  Toggle mouse capture: C  ");
        ImGui::Text("  -- When captured --  ");
        ImGui::Text("  Move (WASD)  ");
        ImGui::Text("  Up (Space)  ");
        ImGui::Text("  Down (LCtrl)  ");
        ImGui::Text("  Camera offsets (scroll wheel)  ");
        ImGui::Separator();

        ImGui::Text("Model 1: %s", model_1.label.c_str());
        ImGui::PushID("Model1");
        ImGui::DragFloat3("Translate", &model_1.userT.x, 0.01f);
        ImGui::DragFloat3("Rotate (deg)", &model_1.userRdeg.x, 0.5f);
        ImGui::DragFloat3("Scale", &model_1.userS.x, 0.01f, 0.0001f, 1000.0f);
        ImGui::PopID();

        ImGui::Text("Model 2: %s", model_2.label.c_str());
        ImGui::PushID("Model2");
        ImGui::DragFloat3("Translate", &model_2.userT.x, 0.01f);
        ImGui::DragFloat3("Rotate (deg)", &model_2.userRdeg.x, 0.5f);
        ImGui::DragFloat3("Scale", &model_2.userS.x, 0.01f, 0.0001f, 1000.0f);
        ImGui::PopID();
        ImGui::Separator();

        ImGui::Text("Lights");
        ImGui::DragFloat("Ambient", &ambientStrength, 0.001f, 0.0f, 1.0f);
        for (size_t i = 0; i < lights.size(); i++)
        {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("Light %d", static_cast<int>(i + 1));
            ImGui::DragFloat3("Position", &lights[i].position.x, 0.05f);
            ImGui::ColorEdit3("Color", &lights[i].color.x);
            ImGui::DragFloat("Intensity", &lights[i].intensity, 0.05f, 0.0f, 20.0f);
            ImGui::PopID();
        }

        if (ImGui::Button("Reset camera"))
        {
            cam.pos = glm::vec3(0.0f, 0.5f, 3.0f);
            cam.yawDeg = -90.0f;
            cam.pitchDeg = 0.0f;
            cam.forwardOffset = 0.0f;
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    model_1.gpu.DestroyGL();
    model_2.gpu.DestroyGL();

    lightMarkerRenderer.Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
