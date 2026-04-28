#include "legacy/app.h"
#include "legacy/font_manager.h"
#include "legacy/mesh_manager.h"

// GLFW
#define GLFW_INCLUDE_ES3    // Include OpenGL ES 3.0 headers
#define GLFW_INCLUDE_GLEXT  // Include to OpenGL ES extension headers
#include <GLFW/glfw3.h>

// ImGui
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Emscripten
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <functional>
static std::function<void()>            MainLoopForEmscriptenP;
static void MainLoopForEmscripten()     { MainLoopForEmscriptenP(); }
#define EMSCRIPTEN_MAINLOOP_BEGIN       MainLoopForEmscriptenP = [&]() { do
#define EMSCRIPTEN_MAINLOOP_END         while (0); }; emscripten_set_main_loop(MainLoopForEmscripten, 0, true)


void OnGlfwError(int errorCode, const char* description) {
    SPDLOG_ERROR("GLFW error (code {}}): {}", errorCode, description);
    
    switch (errorCode)
    {
    case GLFW_NOT_INITIALIZED:
        SPDLOG_ERROR("GLFW has not been initialized");
        break;
    case GLFW_NO_CURRENT_CONTEXT:
        SPDLOG_ERROR("No OpenGL context is current"); 
        break;
    case GLFW_INVALID_ENUM:
        SPDLOG_ERROR("Invalid enum value was passed");
        break;
    case GLFW_INVALID_VALUE:
        SPDLOG_ERROR("Invalid value was passed");
        break;
    case GLFW_OUT_OF_MEMORY:
        SPDLOG_ERROR("Memory allocation failed");
        break;
    case GLFW_API_UNAVAILABLE:
        SPDLOG_ERROR("The requested API is unavailable");
        break;
    case GLFW_VERSION_UNAVAILABLE:
        SPDLOG_ERROR("The requested OpenGL version is unavailable");
        break;
    case GLFW_PLATFORM_ERROR:
        SPDLOG_ERROR("A platform-specific error occurred");
        break;
    case GLFW_FORMAT_UNAVAILABLE:
        SPDLOG_ERROR("The requested format is unavailable");
        break;
    }
}

int main(int argc, char* argv[]) {
    SPDLOG_DEBUG("Start Vtk-Workbench");

    // Initialize application
    App& app = App::Instance();

    // Initialize data managers
    MeshManager& meshManager = MeshManager::Instance();

    // Initialize glfw
    if (!glfwInit()) {
        glfwSetErrorCallback(OnGlfwError);
        return -1;
    }
    SPDLOG_DEBUG("GLFW initialized");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // For MSAA

    int width, height;
    // For wasm, width and height can be obtained from canvas.
    emscripten_get_canvas_element_size("canvas", &width, &height);
    auto window = glfwCreateWindow(width, height, "VTK-Workbench", nullptr, nullptr);
    if (!window) {
        SPDLOG_ERROR("Failed to create glfw window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    SPDLOG_DEBUG("GLFW window created");

    auto glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    SPDLOG_DEBUG("OpenGL context version: {}", glVersion);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Keep ini persistence on the explicit App::Save/LoadImGuiIniFile path.
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoAutoMerge = false;
    io.ConfigViewportsNoTaskBarIcon = true;
    io.ConfigDockingTransparentPayload = true;

    // Setup fonts
    // Initialize font manager after ImGui context is created.
    FontManager& fontManager = FontManager::Instance();

    // Setup ImGui style
    app.SetInitColorStyle();  // Initialize ImGui style after ImGui context is created.
    app.SetInitFontSize();    // Initialize global font size after ImGui context is created.
    app.SetDetailedStyle();   // Setup detailed ImGui style

    ImGui::GetStyle().ScaleAllSizes(app.DevicePixelRatio());

    const char* glsl_version = "#version 300 es";    // OpenGL ES for WebGL

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize ImGui-based windows
    // They should be initialized after ImGui context is created.
    app.InitImGuiWindows();

    // Main loop
    SPDLOG_DEBUG("Start main loop");
    EMSCRIPTEN_MAINLOOP_BEGIN
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render all ImGui-based windows
        app.Render();

        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
    EMSCRIPTEN_MAINLOOP_END;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
