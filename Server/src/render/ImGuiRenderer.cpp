//
// Created by Anatol on 25/06/2022.
//

#include "render/ImGuiRenderer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include <iostream>

std::map<ImGuiContext*, ImGuiRenderer*> s_contextToRenderer;

ImGuiRenderer::ImGuiRenderer() {
    state = NOT_INITIALIZED;
}

ImGuiRenderer::~ImGuiRenderer() {
    cleanup();
}

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW error: " << error << ": " << description << std::endl;
}

bool ImGuiRenderer::init() {
    if (state != NOT_INITIALIZED) {
        return true;
    }

    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(800, 600, "Snake", NULL, NULL);
    if(!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return false;
    }

    IMGUI_CHECKVERSION();
    s_contextToRenderer[ImGui::CreateContext()] = this;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DpiEnableScaleViewports;
    ImGuiIO io = ImGui::GetIO(); (void) io;

    ImGui::GetPlatformIO().Platform_OnChangedViewport = [](ImGuiViewport* viewport) {
        ImGuiRenderer* renderer = s_contextToRenderer[ImGui::GetCurrentContext()];

        //ImGui::GetStyle().ScaleAllSizes(viewport->DpiScale * renderer->ref / ImGui::GetStyle().IndentSpacing);
        ImGui::GetIO().FontDefault = renderer->getFont(viewport->DpiScale);
    };

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    this->ref = ImGui::GetStyle().IndentSpacing;

    return true;
}

ImFont *ImGuiRenderer::getFont(float dpiScale) {
    auto it = this->fonts.find(dpiScale);

    if (it == this->fonts.end()) {
        ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(R"(C:\Users\Anatol\CLionProjects\Snake\courbd.ttf)", dpiScale * 24.f);
        this->fonts[dpiScale] = font;
        return font;
    }

    return it->second;
}

void ImGuiRenderer::renderFrame() {
    glfwPollEvents();

    for (auto& monitor: ImGui::GetPlatformIO().Monitors) {
        getFont(monitor.DpiScale); //Prepare all fonts
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    this->render();

    ImGui::Render();
    int display_W, display_H;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &display_W, &display_H);
    glViewport(0, 0, display_W, display_H);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, ImGui::IsKeyDown(ImGuiKey_::ImGuiKey_W) ? GL_LINE : GL_FILL);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}

void ImGuiRenderer::cleanup() {
    if (state == CLEANED_UP) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(ImGui::GetCurrentContext());
    glfwDestroyWindow(window);
    glfwTerminate();

    s_contextToRenderer.erase(ImGui::GetCurrentContext());

    state = CLEANED_UP;
}

void ImGuiRenderer::mainloop() {
    while (!glfwWindowShouldClose(window)) {
        renderFrame();
    }
}
