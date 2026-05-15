/**
 * @file app/app.cpp
 * @brief Application bootstrap and dockspace renderer.
 */
#include "app.h"

#include "../core/scene/scene_state.h"
#include "../core/vtk/mouse_interactor.h"
#include "../core/vtk/vtk_viewer.h"
#include "../features/build/build_menu.h"
#include "../features/data/data_menu.h"
#include "../features/edit/edit_menu.h"
#include "../features/file/file_menu.h"
#include "../features/measurement/measurement_menu.h"
#include "../features/utilities/brillouin_zone/bz_menu.h"
#include "../features/viewer/viewer_menu.h"

#define GLFW_INCLUDE_ES3
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <vtkRenderWindowInteractor.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

namespace app {

namespace {
constexpr const char* kIdbfsMountPath = "/settings";
constexpr const char* kImGuiIniPath = "/settings/imgui.ini";
constexpr const char* kDockspaceIdName = "Phase1Dockspace";
constexpr const char* kViewerWindowName = "Viewer";
constexpr const char* kCreatedAtomsWindowName = "Created Atoms";
constexpr const char* kBondsManagementWindowName = "Bonds Management";
constexpr const char* kCellInformationWindowName = "Cell Information";
constexpr const char* kPeriodicTableWindowName = "Periodic Table";
constexpr const char* kBravaisLatticeWindowName = "Bravais Lattice Templates";
constexpr const char* kBrillouinZoneWindowName = "Brillouin Zone Plot";
constexpr const char* kChargeDensityWindowName = "Charge Density Viewer";
constexpr const char* kSliceViewerWindowName = "Slice Viewer";

core::scene::SceneState g_sceneState;
vtkSmartPointer<core::vtk::MouseInteractor> g_mouseInteractor;

struct ResetWindowLayout {
    ImVec2 viewerPos;
    ImVec2 viewerSize;
    ImVec2 leftPanelPos;
    ImVec2 secondLeftPanelPos;
    ImVec2 rightPanelPos;
    ImVec2 secondRightPanelPos;
    ImVec2 panelSize;
    ImVec2 widePanelSize;
};

void addTooltip(const char* title, const char* text) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("%s\n%s", title, text);
    }
}

void requestViewerWindow() {
    features::viewer::HandleRequest(features::viewer::Request{features::viewer::Request::Type::ShowViewer});
}

void requestEditWindows() {
    features::edit::HandleRequest(features::edit::Request{features::edit::Request::Type::ShowCreatedAtoms});
    features::edit::HandleRequest(features::edit::Request{features::edit::Request::Type::ShowBondsManagement});
    features::edit::HandleRequest(features::edit::Request{features::edit::Request::Type::ShowCellInformation});
}

void requestBuildWindows() {
    features::build::HandleRequest(features::build::Request{features::build::Request::Type::ShowPeriodicTable});
    features::build::HandleRequest(features::build::Request{features::build::Request::Type::ShowBravaisLattice});
}

void requestDataWindows() {
    features::data::HandleRequest(features::data::Request{features::data::Request::Type::ShowIsosurface});
    features::data::HandleRequest(features::data::Request{features::data::Request::Type::ShowPlane});
}

void requestUtilityWindows() {
    features::utilities::bz::HandleRequest(features::utilities::bz::Request{features::utilities::bz::Request::Type::Show});
}

void requestAllLayoutWindows() {
    requestViewerWindow();
    requestEditWindows();
    requestBuildWindows();
    requestDataWindows();
    requestUtilityWindows();
}

void dockToolWindows(ImGuiID dockNodeId) {
    ImGui::DockBuilderDockWindow(kCreatedAtomsWindowName, dockNodeId);
    ImGui::DockBuilderDockWindow(kBondsManagementWindowName, dockNodeId);
    ImGui::DockBuilderDockWindow(kCellInformationWindowName, dockNodeId);
    ImGui::DockBuilderDockWindow(kPeriodicTableWindowName, dockNodeId);
    ImGui::DockBuilderDockWindow(kBravaisLatticeWindowName, dockNodeId);
    ImGui::DockBuilderDockWindow(kBrillouinZoneWindowName, dockNodeId);
}

ResetWindowLayout buildResetWindowLayout(const ImGuiViewport* viewport) {
    const ImVec2 workPos = viewport != nullptr ? viewport->WorkPos : ImVec2(0.0f, 0.0f);
    const ImVec2 workSize = viewport != nullptr ? viewport->WorkSize : ImVec2(1280.0f, 720.0f);

    ResetWindowLayout layout{};
    layout.viewerSize = ImVec2(workSize.x * 0.54f, workSize.y * 0.58f);
    layout.viewerPos = ImVec2(
        workPos.x + (workSize.x - layout.viewerSize.x) * 0.5f,
        workPos.y + (workSize.y - layout.viewerSize.y) * 0.5f);
    layout.panelSize = ImVec2(std::max(300.0f, workSize.x * 0.28f), std::max(320.0f, workSize.y * 0.48f));
    layout.widePanelSize = ImVec2(std::max(420.0f, workSize.x * 0.36f), std::max(300.0f, workSize.y * 0.42f));

    const float insetX = workSize.x * 0.04f;
    const float insetY = workSize.y * 0.05f;
    layout.leftPanelPos = ImVec2(workPos.x + insetX, workPos.y + insetY);
    layout.secondLeftPanelPos = ImVec2(workPos.x + insetX * 1.7f, workPos.y + insetY * 1.7f);
    layout.rightPanelPos = ImVec2(workPos.x + workSize.x - layout.panelSize.x - insetX, workPos.y + insetY);
    layout.secondRightPanelPos = ImVec2(workPos.x + workSize.x - layout.widePanelSize.x - insetX * 1.6f,
                                        workPos.y + insetY * 1.8f);
    return layout;
}

void setWindowGeometry(const char* name, const ImVec2& pos, const ImVec2& size) {
    ImGui::SetWindowPos(name, pos, ImGuiCond_Always);
    ImGui::SetWindowSize(name, size, ImGuiCond_Always);
}
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
    if (g_mouseInteractor == nullptr) {
        g_mouseInteractor = vtkSmartPointer<core::vtk::MouseInteractor>::New();
        if (g_mouseInteractor == nullptr) {
            return 3;
        }
    }

    g_mouseInteractor->SetEventBus(&g_sceneState.events);
    g_mouseInteractor->SetRenderRequestHandler([]() {
        core::vtk::VtkViewer::Instance().RequestRender();
    });
    g_mouseInteractor->SetActiveStructureId(g_sceneState.currentStructureId);
    g_mouseInteractor->SetDefaultRenderer(core::vtk::VtkViewer::Instance().GetRenderer());

    if (vtkRenderWindowInteractor* interactor = core::vtk::VtkViewer::Instance().GetInteractor();
        interactor != nullptr) {
        interactor->SetInteractorStyle(g_mouseInteractor.GetPointer());
    }

    features::file::InitOnce(g_sceneState);
    features::utilities::bz::InitOnce(g_sceneState);
    features::data::InitOnce(g_sceneState);
    features::build::InitOnce(g_sceneState);
    features::edit::InitOnce(g_sceneState, *g_mouseInteractor);
    features::measurement::InitOnce(g_sceneState, *g_mouseInteractor);
    features::viewer::InitOnce(g_sceneState, *g_mouseInteractor);

    initialized_ = true;
    return 0;
}

void App::RenderFrame() {
    if (!initialized_) {
        return;
    }

    glfwPollEvents();
    if (g_mouseInteractor != nullptr) {
        g_mouseInteractor->SetActiveStructureId(g_sceneState.currentStructureId);
    }
    if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0) {
        ImGui_ImplGlfw_Sleep(10);
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderDockSpace();

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

    const ImGuiID dockspaceId = ImGui::GetID(kDockspaceIdName);
    applyPendingLayout(dockspaceId, viewport);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Crystal Viewer")) {
            ImGui::TextUnformatted("VTK-Workbench");
            ImGui::EndMenu();
        }
        features::file::DrawMenu();
        features::edit::DrawMenu();
        features::build::DrawMenu();
        features::measurement::DrawMenu();
        features::data::DrawMenu();
        features::utilities::bz::DrawMenu();
        features::viewer::DrawMenus();
        ImGui::Separator();
        renderLayoutButtons();
        ImGui::EndMenuBar();
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    features::viewer::RenderWindows();
    features::file::RenderWindows();
    features::edit::RenderWindows();
    features::build::RenderWindows();
    features::measurement::RenderWindows();
    features::data::RenderWindows();
    features::utilities::bz::RenderWindows();
    applyResetWindowGeometry(viewport);
}

void App::renderLayoutButtons() {
    if (ImGui::SmallButton("Layout 1")) {
        pendingLayoutPreset_ = LayoutPreset::Layout1;
    }
    addTooltip("Layout 1", "Viewer centered with tool windows stacked on the left.");

    ImGui::SameLine();
    if (ImGui::SmallButton("Layout 2")) {
        pendingLayoutPreset_ = LayoutPreset::Layout2;
    }
    addTooltip("Layout 2", "Tools on the left, data viewers on the right, Viewer in the center.");

    ImGui::SameLine();
    if (ImGui::SmallButton("Layout 3")) {
        pendingLayoutPreset_ = LayoutPreset::Layout3;
    }
    addTooltip("Layout 3", "Viewer on top with tool windows docked along the bottom.");

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset")) {
        pendingLayoutPreset_ = LayoutPreset::Reset;
    }
    addTooltip("Reset", "Clear docking and restore default window geometry.");
}

void App::applyPendingLayout(unsigned int dockspaceId, const ImGuiViewport* viewport) {
    if (pendingLayoutPreset_ == LayoutPreset::None) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) == 0) {
        pendingLayoutPreset_ = LayoutPreset::None;
        return;
    }

    requestViewerWindow();
    if (pendingLayoutPreset_ == LayoutPreset::Layout2 ||
        pendingLayoutPreset_ == LayoutPreset::Layout3 ||
        pendingLayoutPreset_ == LayoutPreset::Reset) {
        requestEditWindows();
        requestBuildWindows();
    }
    if (pendingLayoutPreset_ == LayoutPreset::Layout2 || pendingLayoutPreset_ == LayoutPreset::Reset) {
        requestDataWindows();
    }
    if (pendingLayoutPreset_ == LayoutPreset::Reset) {
        requestUtilityWindows();
    }

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    if (viewport != nullptr) {
        ImGui::DockBuilderSetNodePos(dockspaceId, viewport->WorkPos);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);
    }

    if (pendingLayoutPreset_ == LayoutPreset::Layout1) {
        ImGuiID dockMainId = dockspaceId;
        ImGuiID dockLeftId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.30f, nullptr, &dockMainId);
        ImGui::DockBuilderDockWindow(kViewerWindowName, dockMainId);
        dockToolWindows(dockLeftId);
        if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockLeftId); node != nullptr) {
            node->SelectedTabId = ImGui::GetID(kCreatedAtomsWindowName);
        }
    } else if (pendingLayoutPreset_ == LayoutPreset::Layout2) {
        ImGuiID dockMainId = dockspaceId;
        ImGuiID dockLeftId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.30f, nullptr, &dockMainId);
        ImGuiID dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.22f, nullptr, &dockMainId);
        ImGuiID dockRightBottomId = ImGui::DockBuilderSplitNode(dockRightId, ImGuiDir_Down, 0.50f, nullptr, &dockRightId);
        ImGui::DockBuilderDockWindow(kViewerWindowName, dockMainId);
        dockToolWindows(dockLeftId);
        ImGui::DockBuilderDockWindow(kChargeDensityWindowName, dockRightId);
        ImGui::DockBuilderDockWindow(kSliceViewerWindowName, dockRightBottomId);
    } else if (pendingLayoutPreset_ == LayoutPreset::Layout3) {
        ImGuiID dockMainId = dockspaceId;
        ImGuiID dockBottomId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 0.40f, nullptr, &dockMainId);
        ImGui::DockBuilderDockWindow(kViewerWindowName, dockMainId);
        dockToolWindows(dockBottomId);
        if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockBottomId); node != nullptr) {
            node->SelectedTabId = ImGui::GetID(kCreatedAtomsWindowName);
        }
    } else if (pendingLayoutPreset_ == LayoutPreset::Reset) {
        requestAllLayoutWindows();
        resetWindowGeometryPassesRemaining_ = 2;
    }

    ImGui::DockBuilderFinish(dockspaceId);
    ImGui::MarkIniSettingsDirty();
    pendingLayoutPreset_ = LayoutPreset::None;
}

void App::applyResetWindowGeometry(const ImGuiViewport* viewport) {
    if (resetWindowGeometryPassesRemaining_ <= 0) {
        return;
    }

    const ResetWindowLayout layout = buildResetWindowLayout(viewport);
    setWindowGeometry(kViewerWindowName, layout.viewerPos, layout.viewerSize);
    setWindowGeometry(kCreatedAtomsWindowName, layout.leftPanelPos, layout.panelSize);
    setWindowGeometry(kBondsManagementWindowName, layout.secondLeftPanelPos, layout.panelSize);
    setWindowGeometry(kCellInformationWindowName, ImVec2(layout.leftPanelPos.x + 36.0f, layout.leftPanelPos.y + 36.0f), layout.panelSize);
    setWindowGeometry(kPeriodicTableWindowName, layout.rightPanelPos, layout.panelSize);
    setWindowGeometry(kBravaisLatticeWindowName, layout.secondRightPanelPos, layout.widePanelSize);
    setWindowGeometry(kBrillouinZoneWindowName, ImVec2(layout.secondRightPanelPos.x - 32.0f, layout.secondRightPanelPos.y + 32.0f), layout.widePanelSize);
    setWindowGeometry(kChargeDensityWindowName, ImVec2(layout.rightPanelPos.x, layout.rightPanelPos.y + 56.0f), layout.panelSize);
    setWindowGeometry(kSliceViewerWindowName, ImVec2(layout.rightPanelPos.x - 36.0f, layout.rightPanelPos.y + 112.0f), layout.panelSize);

    --resetWindowGeometryPassesRemaining_;
}

} // namespace app
