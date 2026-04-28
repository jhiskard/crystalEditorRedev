/**
 * @file app/app.h
 * @brief Application shell bootstrap for Phase 1.
 */
#pragma once

struct GLFWwindow;

namespace app {

/**
 * @class App
 * @brief Owns minimal GLFW + ImGui lifecycle and renders an empty dockspace.
 */
class App {
public:
    /**
     * @brief Returns the singleton application instance.
     */
    static App& Instance();

    /**
     * @brief Initializes GLFW/OpenGL/ImGui bootstrap resources.
     * @return 0 on success, non-zero on failure.
     */
    int Init();

    /**
     * @brief Renders one frame of the Phase 1 shell.
     */
    void RenderFrame();

    /**
     * @brief Mounts IDBFS at `/settings`.
     */
    static void InitIdbfs();

    /**
     * @brief Saves ImGui ini settings to `/settings/imgui.ini`.
     */
    static void SaveImGuiIniFile();

    /**
     * @brief Loads ImGui ini settings from `/settings/imgui.ini`.
     */
    static void LoadImGuiIniFile();

    /**
     * @brief Returns current device pixel ratio.
     */
    static float DevicePixelRatio();

private:
    App() = default;
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /**
     * @brief Draws the root dockspace window and placeholder menu.
     */
    void renderDockSpace();

    GLFWwindow* window_ = nullptr;
    bool initialized_ = false;
};

} // namespace app
