/**
 * @file app/app.cpp
 * @brief Minimal Phase 1 application bootstrap and empty dockspace renderer.
 */
#include "app.h"

#include "../core/scene/scene_state.h"
#include "../core/vtk/vtk_viewer.h"
#include "../features/build/build_menu.h"
#include "../features/data/data_menu.h"
#include "../features/utilities/brillouin_zone/bz_menu.h"

#define GLFW_INCLUDE_ES3
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <fstream>
#include <iterator>
#include <string>

namespace app {

namespace {
constexpr const char* kIdbfsMountPath = "/settings";
constexpr const char* kImGuiIniPath = "/settings/imgui.ini";
core::scene::SceneState g_sceneState;
}

App& App::Instance() {
    static App instance;
    return instance;
}

int App::Init() {
    if (initialized_) {
        return 0;
    }

    if (!glfwInit()) {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    int width = 960;
    int height = 540;
    emscripten_get_canvas_element_size("canvas", &width, &height);
    if (width <= 0 || height <= 0) {
        width = 960;
        height = 540;
    }

    window_ = glfwCreateWindow(width, height, "VTK-Workbench", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return 2;
    }
    glfwMakeContextCurrent(window_);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoAutoMerge = false;
    io.ConfigViewportsNoTaskBarIcon = true;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window_, "#canvas");
    ImGui_ImplOpenGL3_Init("#version 300 es");

    core::vtk::VtkViewer::Instance().Init();
    features::utilities::bz::InitOnce(g_sceneState);
    features::data::InitOnce(g_sceneState);
    features::build::InitOnce(g_sceneState);

    initialized_ = true;
    return 0;
}

void App::RenderFrame() {
    if (!initialized_) {
        return;
    }

    glfwPollEvents();
    if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0) {
        ImGui_ImplGlfw_Sleep(10);
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderDockSpace();
    core::vtk::VtkViewer::Instance().Render();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backupCurrentContext);
    }

    glfwSwapBuffers(window_);
}

void App::InitIdbfs() {
    EM_ASM({
        if (typeof FS === "undefined" || typeof IDBFS === "undefined") {
            return;
        }
        const mountPath = UTF8ToString($0);
        try { FS.mkdir(mountPath); } catch (e) {}
        try { FS.mount(IDBFS, {}, mountPath); } catch (e) {}
    }, kIdbfsMountPath);
}

void App::SaveImGuiIniFile() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    size_t dataSize = 0;
    const char* data = ImGui::SaveIniSettingsToMemory(&dataSize);
    if (data == nullptr || dataSize == 0) {
        return;
    }

    std::ofstream out(kImGuiIniPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        return;
    }
    out.write(data, static_cast<std::streamsize>(dataSize));
}

void App::LoadImGuiIniFile() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    std::ifstream in(kImGuiIniPath, std::ios::binary);
    if (!in) {
        return;
    }

    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (!data.empty()) {
        ImGui::LoadIniSettingsFromMemory(data.c_str(), data.size());
    }
}

float App::DevicePixelRatio() {
    return static_cast<float>(emscripten_get_device_pixel_ratio());
}

void App::renderDockSpace() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Phase1DockspaceRoot", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Crystal Viewer (rebuilding...)")) {
            ImGui::TextUnformatted("Phase 1: app/+core bootstrap");
            ImGui::TextUnformatted("Features will return in later phases.");
            ImGui::EndMenu();
        }
        features::utilities::bz::DrawMenu();
        features::data::DrawMenu();
        features::build::DrawMenu();
        ImGui::EndMenuBar();
    }

    const ImGuiID dockspaceId = ImGui::GetID("Phase1Dockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    features::utilities::bz::RenderWindows();
    features::data::RenderWindows();
    features::build::RenderWindows();
}

} // namespace app
