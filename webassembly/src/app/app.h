/**
 * @file app/app.h
 * @brief Application shell bootstrap.
 */
#pragma once

struct GLFWwindow;
struct ImGuiViewport;

namespace app {

enum class LayoutPreset {
    None,
    Layout1,
    Layout2,
    Layout3,
    Reset,
};

/**
 * @class App
 * @brief Owns GLFW, ImGui, and feature window lifecycle.
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
     * @brief Renders one application frame.
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
     * @brief Draws the root dockspace window and menu bar.
     */
    void renderDockSpace();
    void renderLayoutButtons();
    void applyPendingLayout(unsigned int dockspaceId, const ImGuiViewport* viewport);
    void applyResetWindowGeometry(const ImGuiViewport* viewport);

    GLFWwindow* window_ = nullptr;
    LayoutPreset pendingLayoutPreset_ = LayoutPreset::None;
    int resetWindowGeometryPassesRemaining_ = 0;
    bool initialized_ = false;
};

} // namespace app
