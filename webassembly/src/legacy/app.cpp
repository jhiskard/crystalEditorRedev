#include "app.h"
#include "font_manager.h"
#include "file_loader.h"
#include "vtk_viewer.h"
#include "test_window.h"
#include "model_tree.h"
#include "mesh_detail.h"
#include "mesh_manager.h"
#include "mesh_group_detail.h"

// Add AtomsTemplate
// #include "atoms_template.h"
#include "atoms/atoms_template.h"

// ImGui
#include <imgui.h>
#include <imgui_internal.h>

// Standard library
#include <fstream>

// Emscripten
#include <emscripten/emscripten.h>

// Third-party libraries
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace {
const char* kViewerWindowName = ICON_FA6_DISPLAY"  Viewer";
const char* kModelTreeWindowName = ICON_FA6_FOLDER_TREE"  Model Tree";
const char* kPeriodicTableWindowName = "Periodic Table";
const char* kCrystalTemplatesWindowName = "Crystal Templates";
const char* kBrillouinZonePlotWindowName = "Brillouin Zone Plot";
const char* kCreatedAtomsWindowName = "Created Atoms";
const char* kBondsManagementWindowName = "Bonds Management";
const char* kCellInformationWindowName = "Cell Information";
const char* kChargeDensityViewerWindowName = "Charge Density Viewer";
const char* kSliceViewerWindowName = "2D Slice Viewer";

bool parseVisibilityValue(const std::string& value, bool& outValue) {
    if (value == "1" || value == "true" || value == "TRUE") {
        outValue = true;
        return true;
    }
    if (value == "0" || value == "false" || value == "FALSE") {
        outValue = false;
        return true;
    }
    return false;
}

float toUiScale(App::FontSizePreset preset) {
    switch (preset) {
    case App::FontSizePreset::Small:
        return 1.0f;
    case App::FontSizePreset::Medium:
        return 1.2f;
    case App::FontSizePreset::Large:
        return 1.5f;
    }
    return 1.0f;
}

App::FontSizePreset fontSizePresetFromInt(int value) {
    switch (value) {
    case 1:
        return App::FontSizePreset::Medium;
    case 2:
        return App::FontSizePreset::Large;
    default:
        return App::FontSizePreset::Small;
    }
}

struct ResetWindowLayout {
    ImVec2 viewerPos;
    ImVec2 viewerSize;
    ImVec2 modelTreePos;
    ImVec2 advancedViewPos;
    ImVec2 crystalBuilderPos;
    ImVec2 crystalEditorPos;
    ImVec2 panelSize;
};

ResetWindowLayout buildResetWindowLayout(const ImGuiViewport* viewport) {
    const ImVec2 workPos = viewport->WorkPos;
    const ImVec2 workSize = viewport->WorkSize;

    const ImVec2 viewerSize(workSize.x * 0.5f, workSize.y * 0.5f);
    const ImVec2 viewerPos(
        workPos.x + (workSize.x - viewerSize.x) * 0.5f,
        workPos.y + (workSize.y - viewerSize.y) * 0.5f
    );

    const ImVec2 panelSize(workSize.x * 0.3f, workSize.y * 0.5f);
    const float inset5x = workSize.x * 0.05f;
    const float inset5y = workSize.y * 0.05f;
    const float inset10x = workSize.x * 0.10f;
    const float inset10y = workSize.y * 0.10f;

    ResetWindowLayout layout {};
    layout.viewerPos = viewerPos;
    layout.viewerSize = viewerSize;
    layout.panelSize = panelSize;
    layout.modelTreePos = ImVec2(
        workPos.x + workSize.x - panelSize.x - inset5x,
        workPos.y + inset5y
    );
    layout.advancedViewPos = ImVec2(
        workPos.x + workSize.x - panelSize.x - inset10x,
        workPos.y + inset10y
    );
    layout.crystalBuilderPos = ImVec2(workPos.x + inset5x, workPos.y + inset5y);
    layout.crystalEditorPos = ImVec2(workPos.x + inset10x, workPos.y + inset10y);
    return layout;
}
}


App::App() {
    initLogger();
}

App::~App() {
    shutdownLogger();
}

void App::initLogger(bool useConsole, bool newFile) {
    if (useConsole) {
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
    }
    else {
        auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/vtk-workbench.log", newFile);
        spdlog::set_default_logger(file_logger);
    }
#ifdef DEBUG_BUILD
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
#else
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%^%l%$] %v");
#endif
    SPDLOG_DEBUG("Logger initialized");
}

void App::shutdownLogger() {
    spdlog::shutdown();
    SPDLOG_DEBUG("Logger shutdown");
}

void App::InitIdbfs() {
    EM_ASM({
        const path = UTF8ToString($0);
        VtkModule.FS.mkdir(path);
        VtkModule.FS.mount(IDBFS, {}, path); 
        console.log(`IDBFS mounted on "${path}"`);
    }, Instance().IDBFS_MOUNT_PATH);
}

void App::SaveImGuiIniFile() {
    // Get ImGui setting data
    size_t dataSize = 0;
    const char* data = ImGui::SaveIniSettingsToMemory(&dataSize);

    std::ofstream iniFile(Instance().IMGUI_SETTING_FILE_PATH, std::ios::out | std::ios::trunc);
    if (!iniFile) {
        SPDLOG_ERROR("Failed to open ImGui setting file for writing: {}",
            Instance().IMGUI_SETTING_FILE_PATH);
        return;
    }
    iniFile.write(data, dataSize);
    iniFile.close();

    std::ofstream windowFile(Instance().WINDOW_SETTING_FILE_PATH, std::ios::out | std::ios::trunc);
    if (!windowFile) {
        SPDLOG_ERROR("Failed to open window visibility file for writing: {}",
            Instance().WINDOW_SETTING_FILE_PATH);
        return;
    }

    const bool anyBuilderWindow =
        Instance().m_bShowPeriodicTableWindow ||
        Instance().m_bShowCrystalTemplatesWindow ||
        Instance().m_bShowBrillouinZonePlotWindow;
    const bool anyEditorWindow =
        Instance().m_bShowCreatedAtomsWindow ||
        Instance().m_bShowBondsManagementWindow ||
        Instance().m_bShowCellInformationWindow;
    const bool anyDataWindow =
        Instance().m_bShowChargeDensityViewerWindow ||
        Instance().m_bShowSliceViewerWindow;

    // Legacy group keys for backward compatibility.
    windowFile << "viewer=" << (Instance().m_bShowVtkViewer ? 1 : 0) << "\n";
    windowFile << "model_tree=" << (Instance().m_bShowModelTree ? 1 : 0) << "\n";
    windowFile << "crystal_builder=" << (anyBuilderWindow ? 1 : 0) << "\n";
    windowFile << "crystal_editor=" << (anyEditorWindow ? 1 : 0) << "\n";
    windowFile << "advanced_view=" << (anyDataWindow ? 1 : 0) << "\n";

    // Split-window keys.
    windowFile << "periodic_table_window=" << (Instance().m_bShowPeriodicTableWindow ? 1 : 0) << "\n";
    windowFile << "crystal_templates_window=" << (Instance().m_bShowCrystalTemplatesWindow ? 1 : 0) << "\n";
    windowFile << "brillouin_zone_plot_window=" << (Instance().m_bShowBrillouinZonePlotWindow ? 1 : 0) << "\n";
    windowFile << "created_atoms_window=" << (Instance().m_bShowCreatedAtomsWindow ? 1 : 0) << "\n";
    windowFile << "bonds_management_window=" << (Instance().m_bShowBondsManagementWindow ? 1 : 0) << "\n";
    windowFile << "cell_information_window=" << (Instance().m_bShowCellInformationWindow ? 1 : 0) << "\n";
    windowFile << "charge_density_viewer_window=" << (Instance().m_bShowChargeDensityViewerWindow ? 1 : 0) << "\n";
    windowFile << "slice_viewer_window=" << (Instance().m_bShowSliceViewerWindow ? 1 : 0) << "\n";
    windowFile.close();
}

void App::LoadImGuiIniFile() {
    std::ifstream iniFile(Instance().IMGUI_SETTING_FILE_PATH, std::ifstream::in);
    if (!iniFile) {
        SPDLOG_INFO("ImGui setting file not found. Using default layout.");
    }
    else {
        std::ostringstream ss;
        ss << iniFile.rdbuf();
        iniFile.close();
        std::string fileContents = ss.str();

        ImGui::LoadIniSettingsFromMemory(fileContents.c_str(), fileContents.size());
    }

    std::ifstream windowFile(Instance().WINDOW_SETTING_FILE_PATH, std::ifstream::in);
    if (!windowFile) {
        SPDLOG_INFO("Window visibility file not found. Using default window visibility.");
        Instance().m_ShouldApplyInitialLayout = true;
        Instance().m_bShowVtkViewer = true;
        Instance().m_bShowModelTree = false;
        Instance().m_bShowPeriodicTableWindow = false;
        Instance().m_bShowCrystalTemplatesWindow = false;
        Instance().m_bShowBrillouinZonePlotWindow = false;
        Instance().m_bShowCreatedAtomsWindow = false;
        Instance().m_bShowBondsManagementWindow = false;
        Instance().m_bShowCellInformationWindow = false;
        Instance().m_bShowChargeDensityViewerWindow = false;
        Instance().m_bShowSliceViewerWindow = false;
        return;
    }
    Instance().m_ShouldApplyInitialLayout = false;

    std::string line;
    while (std::getline(windowFile, line)) {
        const std::size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, delimiterPos);
        const std::string value = line.substr(delimiterPos + 1);
        bool parsedValue = false;
        if (!parseVisibilityValue(value, parsedValue)) {
            continue;
        }

        if (key == "viewer") {
            Instance().m_bShowVtkViewer = parsedValue;
        }
        else if (key == "model_tree") {
            Instance().m_bShowModelTree = parsedValue;
        }
        else if (key == "crystal_builder") {
            Instance().m_bShowPeriodicTableWindow = parsedValue;
            Instance().m_bShowCrystalTemplatesWindow = parsedValue;
            Instance().m_bShowBrillouinZonePlotWindow = parsedValue;
        }
        else if (key == "crystal_editor") {
            Instance().m_bShowCreatedAtomsWindow = parsedValue;
            Instance().m_bShowBondsManagementWindow = parsedValue;
            Instance().m_bShowCellInformationWindow = parsedValue;
        }
        else if (key == "advanced_view") {
            Instance().m_bShowChargeDensityViewerWindow = parsedValue;
            Instance().m_bShowSliceViewerWindow = parsedValue;
        }
        else if (key == "periodic_table_window") {
            Instance().m_bShowPeriodicTableWindow = parsedValue;
        }
        else if (key == "crystal_templates_window") {
            Instance().m_bShowCrystalTemplatesWindow = parsedValue;
        }
        else if (key == "brillouin_zone_plot_window") {
            Instance().m_bShowBrillouinZonePlotWindow = parsedValue;
        }
        else if (key == "created_atoms_window") {
            Instance().m_bShowCreatedAtomsWindow = parsedValue;
        }
        else if (key == "bonds_management_window") {
            Instance().m_bShowBondsManagementWindow = parsedValue;
        }
        else if (key == "cell_information_window") {
            Instance().m_bShowCellInformationWindow = parsedValue;
        }
        else if (key == "charge_density_viewer_window") {
            Instance().m_bShowChargeDensityViewerWindow = parsedValue;
        }
        else if (key == "slice_viewer_window") {
            Instance().m_bShowSliceViewerWindow = parsedValue;
        }
    }

    if (!Instance().m_bShowModelTree) {
        Instance().m_RequestModelTreeFocus = false;
    }
}

void App::setColorStyle(ColorStyle style) {
    m_ColorStyle = style;

    switch (style) {
    case ColorStyle::Dark:
        ImGui::StyleColorsDark();
        break;
    case ColorStyle::Light:
        ImGui::StyleColorsLight();
        break;
    case ColorStyle::Classic:
        ImGui::StyleColorsClassic();
        break;
    }

    // Save viewer style on local storage
    EM_ASM({
        const styleKey = UTF8ToString($0);
        const style = $1;
        localStorage.setItem(styleKey, style);
    }, IMGUI_STYLE_KEY, static_cast<int>(m_ColorStyle));
}

void App::setFontSizePreset(FontSizePreset preset) {
    m_FontSizePreset = preset;
    m_kUiScale = toUiScale(preset);
    applyFontScale();

    // Save font size preset on local storage.
    EM_ASM({
        const fontSizeKey = UTF8ToString($0);
        const fontSizePreset = $1;
        localStorage.setItem(fontSizeKey, fontSizePreset);
    }, IMGUI_FONT_SIZE_KEY, static_cast<int>(m_FontSizePreset));
}

void App::applyFontScale() {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = m_kUiScale;
}

void App::SetDetailedStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Sizes
    style.WindowPadding = ImVec2(6.0f, 6.0f);
    style.FramePadding = ImVec2(5.0f, 3.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 8.0f;
    style.ScrollbarSize = 9.0f;
    style.GrabMinSize = 8.0f;

    // Borders
    style.FrameBorderSize = 0.5f;
    style.TabBorderSize = 1.0f;

    // Rounding
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;

    // Tables
    style.CellPadding = ImVec2(6.0f, 3.0f);

    // Colors
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.Colors[ImGuiCol_Button].w = 0.2f;
}

void App::SetInitColorStyle() {
    int colorStyle = EM_ASM_INT({
        const styleKey = UTF8ToString($0);
        const style = localStorage.getItem(styleKey);
        if (style) {
            return style;
        }
        else {         // If not found in local storage, set default style.
            return 0;  // 0 is dark style.
        }
    }, IMGUI_STYLE_KEY);

    ColorStyle initColorStyle = static_cast<ColorStyle>(colorStyle);
    setColorStyle(initColorStyle);
}

void App::SetInitFontSize() {
    int fontSizePreset = EM_ASM_INT({
        const fontSizeKey = UTF8ToString($0);
        const fontSize = localStorage.getItem(fontSizeKey);
        if (fontSize) {
            const parsed = parseInt(fontSize, 10);
            return Number.isNaN(parsed) ? 0 : parsed;
        }
        else {
            return 0;  // 0 is small size.
        }
    }, IMGUI_FONT_SIZE_KEY);

    setFontSizePreset(fontSizePresetFromInt(fontSizePreset));
}

void App::Render() {
    renderDockSpaceAndMenu();
    renderImGuiWindows();
}

void App::renderDockSpaceAndMenu() {
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
    FontManager& fontManager = FontManager::Instance();

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (m_bFullDockSpace) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    } else {
        dockspaceFlags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
        windowFlags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar();

    if (m_bFullDockSpace)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    ImGuiID dockspace_id = ImGui::GetID("DockSpace");


    auto dockLeftPanelWindows = [&](ImGuiID dockNodeId) {
        ImGui::DockBuilderDockWindow(kModelTreeWindowName, dockNodeId);
        ImGui::DockBuilderDockWindow(kCreatedAtomsWindowName, dockNodeId);
        ImGui::DockBuilderDockWindow(kBondsManagementWindowName, dockNodeId);
        ImGui::DockBuilderDockWindow(kCellInformationWindowName, dockNodeId);
        ImGui::DockBuilderDockWindow(kPeriodicTableWindowName, dockNodeId);
        ImGui::DockBuilderDockWindow(kCrystalTemplatesWindowName, dockNodeId);
        ImGui::DockBuilderDockWindow(kBrillouinZonePlotWindowName, dockNodeId);
    };

    if (m_ShouldApplyInitialLayout && m_PendingLayoutPreset == LayoutPreset::None) {
        m_PendingLayoutPreset = LayoutPreset::DefaultFloating;
        m_ShouldApplyInitialLayout = false;
    }

    if (m_PendingLayoutPreset != LayoutPreset::None) {
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            // 레이아웃 전환 시 기존 메뉴 기반 포커스 요청은 초기화한다.
            m_PendingFocusTarget = FocusTarget::None;
            m_PendingFocusPassesRemaining = 0;

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace | dockspaceFlags);
            ImGui::DockBuilderSetNodePos(dockspace_id, viewport->WorkPos);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            if (m_PendingLayoutPreset == LayoutPreset::DefaultFloating) {
                // Layout 1: Viewer 중심 + 좌측 30% 도킹 패널(기본은 숨김)
                ImGuiID dock_main_id = dockspace_id;
                ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(
                    dock_main_id, ImGuiDir_Left, 0.30f, nullptr, &dock_main_id);
                ImGui::DockBuilderDockWindow(kViewerWindowName, dock_main_id);
                dockLeftPanelWindows(dock_left_id);
                if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_left_id)) {
                    node->SelectedTabId = ImGui::GetID(kModelTreeWindowName);
                }

                m_bShowVtkViewer = true;
                m_bShowModelTree = false;
                m_bShowPeriodicTableWindow = false;
                m_bShowCrystalTemplatesWindow = false;
                m_bShowBrillouinZonePlotWindow = false;
                m_bShowCreatedAtomsWindow = false;
                m_bShowBondsManagementWindow = false;
                m_bShowCellInformationWindow = false;
                m_bShowChargeDensityViewerWindow = false;
                m_bShowSliceViewerWindow = false;
                m_RequestModelTreeFocus = false;
            }
            else if (m_PendingLayoutPreset == LayoutPreset::DockRight) {
                // Layout 2:
                // - Left 30%: Model Tree + Builder/Editor windows (tab stack)
                // - Right 15% (top): Charge Density Viewer
                // - Right 15% (bottom): 2D Slice Viewer
                // - Center: Viewer
                m_bShowVtkViewer = true;
                m_bShowModelTree = true;
                m_bShowPeriodicTableWindow = false;
                m_bShowCrystalTemplatesWindow = false;
                m_bShowBrillouinZonePlotWindow = false;
                m_bShowCreatedAtomsWindow = false;
                m_bShowBondsManagementWindow = false;
                m_bShowCellInformationWindow = false;
                m_bShowChargeDensityViewerWindow = true;
                m_bShowSliceViewerWindow = true;
                m_RequestModelTreeFocus = true;

                ImGuiID dock_main_id = dockspace_id;
                ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(
                    dock_main_id, ImGuiDir_Left, 0.30f, nullptr, &dock_main_id);
                // Remaining width is 70%. To allocate 15% of total width on the right:
                // ratio_on_remaining = 15 / 70.
                ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(
                    dock_main_id, ImGuiDir_Right, 15.0f / 70.0f, nullptr, &dock_main_id);
                ImGui::DockBuilderDockWindow(kViewerWindowName, dock_main_id);

                dockLeftPanelWindows(dock_left_id);
                if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_left_id)) {
                    node->SelectedTabId = ImGui::GetID(kModelTreeWindowName);
                }

                // Dock Charge Density Viewer first on the right.
                ImGui::DockBuilderDockWindow(kChargeDensityViewerWindowName, dock_right_id);
                // Then split right area into top/bottom and dock Plane(2D Slice Viewer) to bottom.
                ImGuiID dock_right_bottom_id = ImGui::DockBuilderSplitNode(
                    dock_right_id, ImGuiDir_Down, 0.50f, nullptr, &dock_right_id);
                ImGui::DockBuilderDockWindow(kSliceViewerWindowName, dock_right_bottom_id);
                if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_right_id)) {
                    node->SelectedTabId = ImGui::GetID(kChargeDensityViewerWindowName);
                }
            }
            else if (m_PendingLayoutPreset == LayoutPreset::DockBottom) {
                // Layout 3: Viewer 60% 위, 좌측 패널 창 40% 아래
                m_bShowVtkViewer = true;
                m_bShowModelTree = true;
                m_bShowPeriodicTableWindow = false;
                m_bShowCrystalTemplatesWindow = false;
                m_bShowBrillouinZonePlotWindow = false;
                m_bShowCreatedAtomsWindow = false;
                m_bShowBondsManagementWindow = false;
                m_bShowCellInformationWindow = false;
                m_bShowChargeDensityViewerWindow = false;
                m_bShowSliceViewerWindow = false;
                m_RequestModelTreeFocus = true;

                ImGuiID dock_main_id = dockspace_id;
                ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(
                    dock_main_id, ImGuiDir_Down, 0.40f, nullptr, &dock_main_id);
                ImGui::DockBuilderDockWindow(kViewerWindowName, dock_main_id);
                dockLeftPanelWindows(dock_bottom_id);
                if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_bottom_id)) {
                    node->SelectedTabId = ImGui::GetID(kModelTreeWindowName);
                }
            }
            else if (m_PendingLayoutPreset == LayoutPreset::ResetDocking) {
                // Reset: 모든 창 도킹 상태 초기화 + 창 기본 배치 적용.
                // DockBuilderRemoveNode/AddNode 이후 추가 도킹을 수행하지 않으면
                // 기존 창은 도킹 해제되어 floating 상태로 복원된다.
                m_bShowVtkViewer = true;
                m_bShowModelTree = true;
                m_bShowPeriodicTableWindow = true;
                m_bShowCrystalTemplatesWindow = true;
                m_bShowBrillouinZonePlotWindow = true;
                m_bShowCreatedAtomsWindow = true;
                m_bShowBondsManagementWindow = true;
                m_bShowCellInformationWindow = true;
                m_bShowChargeDensityViewerWindow = true;
                m_bShowSliceViewerWindow = false;
                m_RequestModelTreeFocus = false;
                m_ResetWindowGeometryPassesRemaining = 2;
            }
            ImGui::DockBuilderFinish(dockspace_id);
            ImGui::MarkIniSettingsDirty();
        }

        m_PendingLayoutPreset = LayoutPreset::None;
    }

    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("  Crystal Viewer")) {
            if (ImGui::MenuItem(ICON_FA6_CIRCLE_INFO "  About")) {
                m_bShowAboutPopup = true;
            }
            ImGui::EndMenu();
        }
        auto requestFocus = [&](FocusTarget target) {
            m_PendingFocusTarget = target;
            m_PendingFocusPassesRemaining = 2;
        };

        if (ImGui::BeginMenu("  File")) {
            if (ImGui::MenuItem("Open Structure File")) {
                FileLoader::Instance().RequestOpenStructureImport();
            }
            AddTooltip("Open Structure File", "Import XSF, XSF(Grid), vasp CHGCAR");

            ImGui::BeginDisabled();
            ImGui::MenuItem("Open Recent");
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("  Edit")) {
            if (ImGui::MenuItem("Atoms")) {
                m_bShowCreatedAtomsWindow = true;
                requestFocus(FocusTarget::CreatedAtoms);
                AtomsTemplate::Instance().RequestEditorSection(
                    AtomsTemplate::EditorSectionRequest::Atoms);
            }
            if (ImGui::MenuItem("Bonds")) {
                m_bShowBondsManagementWindow = true;
                requestFocus(FocusTarget::BondsManagement);
                AtomsTemplate::Instance().RequestEditorSection(
                    AtomsTemplate::EditorSectionRequest::Bonds);
            }
            if (ImGui::MenuItem("Cell")) {
                m_bShowCellInformationWindow = true;
                requestFocus(FocusTarget::CellInformation);
                AtomsTemplate::Instance().RequestEditorSection(
                    AtomsTemplate::EditorSectionRequest::Cell);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("  Build")) {
            if (ImGui::MenuItem("Add atoms")) {
                m_bShowPeriodicTableWindow = true;
                requestFocus(FocusTarget::PeriodicTable);
                AtomsTemplate::Instance().RequestBuilderSection(
                    AtomsTemplate::BuilderSectionRequest::AddAtoms);
            }
            if (ImGui::MenuItem("Bravais Lattice Templates")) {
                m_bShowCrystalTemplatesWindow = true;
                requestFocus(FocusTarget::CrystalTemplates);
                AtomsTemplate::Instance().RequestBuilderSection(
                    AtomsTemplate::BuilderSectionRequest::BravaisLatticeTemplates);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("  Measurement")) {
            AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
            const bool isDistanceMode =
                atomsTemplate.GetMeasurementMode() == AtomsTemplate::MeasurementMode::Distance;
            const bool isAngleMode =
                atomsTemplate.GetMeasurementMode() == AtomsTemplate::MeasurementMode::Angle;
            const bool isDihedralMode =
                atomsTemplate.GetMeasurementMode() == AtomsTemplate::MeasurementMode::Dihedral;
            const bool isGeometricCenterMode =
                atomsTemplate.GetMeasurementMode() == AtomsTemplate::MeasurementMode::GeometricCenter;
            const bool isCenterOfMassMode =
                atomsTemplate.GetMeasurementMode() == AtomsTemplate::MeasurementMode::CenterOfMass;
            if (ImGui::MenuItem("Distance", nullptr, isDistanceMode)) {
                atomsTemplate.EnterMeasurementMode(AtomsTemplate::MeasurementMode::Distance);
            }
            if (ImGui::MenuItem("Angle", nullptr, isAngleMode)) {
                atomsTemplate.EnterMeasurementMode(AtomsTemplate::MeasurementMode::Angle);
            }
            if (ImGui::MenuItem("Dihedral", nullptr, isDihedralMode)) {
                atomsTemplate.EnterMeasurementMode(AtomsTemplate::MeasurementMode::Dihedral);
            }
            if (ImGui::MenuItem("Geometric Center", nullptr, isGeometricCenterMode)) {
                atomsTemplate.EnterMeasurementMode(AtomsTemplate::MeasurementMode::GeometricCenter);
            }
            if (ImGui::MenuItem("Center of Mass", nullptr, isCenterOfMassMode)) {
                atomsTemplate.EnterMeasurementMode(AtomsTemplate::MeasurementMode::CenterOfMass);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("  Data")) {
            if (ImGui::MenuItem("Isosurface")) {
                m_bShowModelTree = true;
                m_bShowChargeDensityViewerWindow = true;
                requestFocus(FocusTarget::ChargeDensityViewer);
                AtomsTemplate::Instance().RequestDataMenu(
                    AtomsTemplate::DataMenuRequest::Isosurface);
            }
            if (ImGui::MenuItem("Surface")) {
                m_bShowModelTree = true;
                m_bShowChargeDensityViewerWindow = true;
                requestFocus(FocusTarget::ChargeDensityViewer);
                AtomsTemplate::Instance().RequestDataMenu(
                    AtomsTemplate::DataMenuRequest::Surface);
            }
            if (ImGui::MenuItem("Volumetric")) {
                m_bShowModelTree = true;
                m_bShowChargeDensityViewerWindow = true;
                requestFocus(FocusTarget::ChargeDensityViewer);
                AtomsTemplate::Instance().RequestDataMenu(
                    AtomsTemplate::DataMenuRequest::Volumetric);
            }
            if (ImGui::MenuItem("Plane")) {
                m_bShowModelTree = true;
                m_bShowSliceViewerWindow = true;
                requestFocus(FocusTarget::SliceViewer);
                AtomsTemplate::Instance().RequestDataMenu(
                    AtomsTemplate::DataMenuRequest::Plane);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("  Utilities")) {
            if (ImGui::MenuItem("Brillouin Zone")) {
                m_bShowBrillouinZonePlotWindow = true;
                requestFocus(FocusTarget::BrillouinZonePlot);
                AtomsTemplate::Instance().RequestBuilderSection(
                    AtomsTemplate::BuilderSectionRequest::BrillouinZone);
            }
            ImGui::EndMenu();
        }

        bool openSettings = ImGui::BeginMenu(ICON_FA6_GEAR"  Settings");
        if (openSettings) {
            bool nodeInfoEnabled = AtomsTemplate::Instance().IsNodeInfoEnabled();
            VtkViewer& viewer = VtkViewer::Instance();
            bool viewerFpsOverlayEnabled = viewer.IsPerformanceOverlayEnabled();

            if (ImGui::MenuItem("Node Tooltip", nullptr, &nodeInfoEnabled)) {
                AtomsTemplate::Instance().SetNodeInfoEnabled(nodeInfoEnabled);
            }
            if (ImGui::MenuItem("Viewer FPS Overlay", nullptr, &viewerFpsOverlayEnabled)) {
                viewer.SetPerformanceOverlayEnabled(viewerFpsOverlayEnabled);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Style")) {
                if (ImGui::MenuItem("Dark", nullptr, GetColorStyle() == ColorStyle::Dark)) {
                    SetColorStyle(ColorStyle::Dark);
                }
                if (ImGui::MenuItem("Light", nullptr, GetColorStyle() == ColorStyle::Light)) {
                    SetColorStyle(ColorStyle::Light);
                }
                if (ImGui::MenuItem("Classic", nullptr, GetColorStyle() == ColorStyle::Classic)) {
                    SetColorStyle(ColorStyle::Classic);
                }
                ImGui::EndMenu();
            }
            ImGui::MenuItem("Background Color", nullptr, &m_bShowBgColorPopup);
            if (ImGui::BeginMenu("Font Size")) {
                if (ImGui::MenuItem("small", nullptr, GetFontSizePreset() == FontSizePreset::Small)) {
                    SetFontSizePreset(FontSizePreset::Small);
                }
                if (ImGui::MenuItem("medium", nullptr, GetFontSizePreset() == FontSizePreset::Medium)) {
                    SetFontSizePreset(FontSizePreset::Medium);
                }
                if (ImGui::MenuItem("large", nullptr, GetFontSizePreset() == FontSizePreset::Large)) {
                    SetFontSizePreset(FontSizePreset::Large);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA6_EXPAND"  Full Screen")) {
                EM_ASM({
                    const canvas = document.getElementById("canvas");
                    canvas.requestFullscreen();
                });
            }
            ImGui::EndMenu();
        }
        bool openWindows = ImGui::BeginMenu(ICON_FA6_WINDOW_RESTORE"  Windows");
        if (openWindows) {
            ImGui::MenuItem(ICON_FA6_DISPLAY"  Viewer", nullptr, &m_bShowVtkViewer);
            ImGui::MenuItem(ICON_FA6_FOLDER_TREE"  Model Tree", nullptr, &m_bShowModelTree);
            ImGui::Separator();
            ImGui::MenuItem("Created Atoms", nullptr, &m_bShowCreatedAtomsWindow);
            ImGui::MenuItem("Bonds Management", nullptr, &m_bShowBondsManagementWindow);
            ImGui::MenuItem("Cell Information", nullptr, &m_bShowCellInformationWindow);
            ImGui::Separator();
            ImGui::MenuItem("Periodic Table", nullptr, &m_bShowPeriodicTableWindow);
            ImGui::MenuItem("Crystal Templates", nullptr, &m_bShowCrystalTemplatesWindow);
            ImGui::MenuItem("Brillouin Zone Plot", nullptr, &m_bShowBrillouinZonePlotWindow);
            ImGui::Separator();
            ImGui::MenuItem("Charge Density Viewer", nullptr, &m_bShowChargeDensityViewerWindow);
            ImGui::MenuItem("2D Slice Viewer", nullptr, &m_bShowSliceViewerWindow);

        #ifdef DEBUG_BUILD
            ImGui::MenuItem("Full Dockspace", nullptr, &m_bFullDockSpace);
        #endif
        //#ifdef SHOW_IMGUI_DEMO
        //    ImGui::MenuItem("Show Demo Window", nullptr, &m_bShowDemoWindow);
        //#endif
        #ifdef SHOW_FONT_ICONS
            ImGui::MenuItem("Show Font Icons", nullptr, &m_bShowFontIcons);
        #endif
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::SmallButton("Layout 1")) {
            m_PendingLayoutPreset = LayoutPreset::DefaultFloating;
        }
        AddTooltip("Layout 1", "Viewer window uses the default floating placement.");
        ImGui::SameLine();
        if (ImGui::SmallButton("Layout 2")) {
            m_PendingLayoutPreset = LayoutPreset::DockRight;
        }
        AddTooltip("Layout 2", "Left 30%: Model Tree/tool windows, Right 15%: Charge Density(top)+Plane(bottom), Center: Viewer.");
        ImGui::SameLine();
        if (ImGui::SmallButton("Layout 3")) {
            m_PendingLayoutPreset = LayoutPreset::DockBottom;
        }
        AddTooltip("Layout 3", "Dock Viewer (60%) top -- Model Tree and split tool windows (40%) bottom.");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset")) {
            m_PendingLayoutPreset = LayoutPreset::ResetDocking;
        }
        AddTooltip("Reset", "Reset docking and restore default window geometry.");
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void App::InitImGuiWindows() {
    // ImGui::LoadIniSettingsFromMemory("", 0); // 저장된 레이아웃 무시
    VtkViewer& viewer = VtkViewer::Instance();
    // VtkViewer& viewer2 = VtkViewer::Instance();
    TestWindow& testWindow = TestWindow::Instance();
    ModelTree& modelTree = ModelTree::Instance();
    MeshDetail& meshDetail = MeshDetail::Instance();
    MeshGroupDetail& meshGroupDetail = MeshGroupDetail::Instance();

    // Add AtomsTemplate
    AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
}

void App::renderAboutPopup() {
    ImGui::OpenPopup("About Crystal Viewer");
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 280), ImGuiCond_Appearing);
    
    if (ImGui::BeginPopupModal("About Crystal Viewer", &m_bShowAboutPopup, 
                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        
        ImGui::Dummy(ImVec2(0, 25));
        
        float windowWidth = ImGui::GetWindowSize().x;
        
        // 타이틀 (큰 글씨 - 스케일 사용)
        const char* title = ICON_FA6_ATOM "  Crystal Viewer";
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        float originalScale = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale = 1.8f;
        ImGui::PushFont(ImGui::GetFont());
        
        float titleWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - titleWidth) * 0.5f);
        ImGui::Text("%s", title);
        
        ImGui::GetFont()->Scale = originalScale;
        ImGui::PopFont();
        ImGui::PopStyleColor();
        
        // 버전 정보
        ImGui::Dummy(ImVec2(0, 8));
        const char* version = "Version 1.0";
        float versionWidth = ImGui::CalcTextSize(version).x;
        ImGui::SetCursorPosX((windowWidth - versionWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", version);
        
        // 구분선
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 20));
        
        // 저작권 정보
        const char* copyright = "Copyright (c) 2025 by KISTI";
        float copyrightWidth = ImGui::CalcTextSize(copyright).x;
        ImGui::SetCursorPosX((windowWidth - copyrightWidth) * 0.5f);
        ImGui::Text("%s", copyright);
        
        // 기관명
        ImGui::Dummy(ImVec2(0, 5));
        const char* subtitle = "Korea Institute of Science and Technology Information";
        float subtitleWidth = ImGui::CalcTextSize(subtitle).x;
        ImGui::SetCursorPosX((windowWidth - subtitleWidth) * 0.5f);
        ImGui::TextDisabled("%s", subtitle);
        
        // 닫기 버튼
        ImGui::Dummy(ImVec2(0, 25));
        
        float buttonWidth = 120.0f;
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        if (ImGui::Button("Close", ImVec2(buttonWidth, 32))) {
            m_bShowAboutPopup = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void App::renderImGuiWindows() {
    // About 팝업 렌더링
    if (m_bShowAboutPopup) {
        renderAboutPopup();
    }

    const bool applyResetLayoutThisFrame = m_ResetWindowGeometryPassesRemaining > 0;
    bool hasResetLayout = false;
    ResetWindowLayout resetLayout {};
    if (applyResetLayoutThisFrame) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (viewport != nullptr) {
            resetLayout = buildResetWindowLayout(viewport);
            hasResetLayout = true;

            VtkViewer::Instance().RequestForcedWindowLayout(resetLayout.viewerPos, resetLayout.viewerSize);

            AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
            atomsTemplate.RequestForcedBuilderWindowLayout(resetLayout.crystalBuilderPos, resetLayout.panelSize);
            atomsTemplate.RequestForcedEditorWindowLayout(resetLayout.crystalEditorPos, resetLayout.panelSize);
            atomsTemplate.RequestForcedAdvancedWindowLayout(resetLayout.advancedViewPos, resetLayout.panelSize);
        }
    }

    if (m_bShowVtkViewer) {
        VtkViewer::Instance().Render();
    }
    // if (m_bShowVtkViewer2) {
    //     VtkViewer::Instance().Render();
    // }

    const bool requestModelTreeFocus = m_RequestModelTreeFocus;

    if (m_bShowModelTree) {
        if (hasResetLayout) {
            ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
            ImGui::SetNextWindowPos(resetLayout.modelTreePos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(resetLayout.panelSize, ImGuiCond_Always);
        }
        if (requestModelTreeFocus) {
            ImGui::SetNextWindowFocus();
        }
        ModelTree::Instance().Render(&m_bShowModelTree);
    }

    AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
    if (m_bShowPeriodicTableWindow) {
        atomsTemplate.RenderPeriodicTableWindow(&m_bShowPeriodicTableWindow);
    }
    if (m_bShowCrystalTemplatesWindow) {
        atomsTemplate.RenderCrystalTemplatesWindow(&m_bShowCrystalTemplatesWindow);
    }
    if (m_bShowBrillouinZonePlotWindow) {
        atomsTemplate.RenderBrillouinZonePlotWindow(&m_bShowBrillouinZonePlotWindow);
    }
    if (m_bShowCreatedAtomsWindow) {
        atomsTemplate.RenderCreatedAtomsWindow(&m_bShowCreatedAtomsWindow);
    }
    if (m_bShowBondsManagementWindow) {
        atomsTemplate.RenderBondsManagementWindow(&m_bShowBondsManagementWindow);
    }
    if (m_bShowCellInformationWindow) {
        atomsTemplate.RenderCellInformationWindow(&m_bShowCellInformationWindow);
    }
    if (m_bShowChargeDensityViewerWindow) {
        atomsTemplate.RenderChargeDensityViewerWindow(&m_bShowChargeDensityViewerWindow);
    }
    if (m_bShowSliceViewerWindow) {
        atomsTemplate.RenderSliceViewerWindow(&m_bShowSliceViewerWindow);
    }

    const int32_t selectedMeshId = ModelTree::Instance().GetSelectedMeshId();
    if (selectedMeshId != -1 && m_bShowMeshDetail) {
        MeshDetail::Instance().Render(selectedMeshId, &m_bShowMeshDetail);
    }
    const Mesh* mesh = MeshManager::Instance().GetMeshById(selectedMeshId);
    if (mesh != nullptr) {
        if (mesh->GetMeshGroupCount() > 0) {
            MeshGroupDetail::Instance().Render(selectedMeshId);
        }
    }
    // if (m_bShowTestWindow) {
    //     TestWindow::Instance().Render(&m_bShowTestWindow);
    // }
#ifdef SHOW_IMGUI_DEMO
    if (m_bShowDemoWindow) {
        ImGui::ShowDemoWindow(&m_bShowDemoWindow);
    }
#endif
#ifdef SHOW_FONT_ICONS
    if (m_bShowFontIcons) {
        FontManager::Instance().Render();
    }
#endif
    if (m_bShowBgColorPopup) {
        VtkViewer::Instance().RenderBgColorPopup(&m_bShowBgColorPopup);
    }
    if (m_bShowProgressPopup) {
        renderProgressPopup();
    }
    FileLoader::Instance().RenderXsfGridImportPopups();

    if (hasResetLayout) {
        ImGui::MarkIniSettingsDirty();
        --m_ResetWindowGeometryPassesRemaining;
    }

    if (requestModelTreeFocus && m_bShowModelTree) {
        ImGui::SetWindowFocus(kModelTreeWindowName);
        m_RequestModelTreeFocus = false;
    }

    if (m_PendingFocusTarget != FocusTarget::None && m_PendingFocusPassesRemaining > 0) {
        const char* targetWindowName = nullptr;
        switch (m_PendingFocusTarget) {
        case FocusTarget::ModelTree:
            if (m_bShowModelTree) {
                targetWindowName = kModelTreeWindowName;
            }
            break;
        case FocusTarget::PeriodicTable:
            if (m_bShowPeriodicTableWindow) {
                targetWindowName = kPeriodicTableWindowName;
            }
            break;
        case FocusTarget::CrystalTemplates:
            if (m_bShowCrystalTemplatesWindow) {
                targetWindowName = kCrystalTemplatesWindowName;
            }
            break;
        case FocusTarget::BrillouinZonePlot:
            if (m_bShowBrillouinZonePlotWindow) {
                targetWindowName = kBrillouinZonePlotWindowName;
            }
            break;
        case FocusTarget::CreatedAtoms:
            if (m_bShowCreatedAtomsWindow) {
                targetWindowName = kCreatedAtomsWindowName;
            }
            break;
        case FocusTarget::BondsManagement:
            if (m_bShowBondsManagementWindow) {
                targetWindowName = kBondsManagementWindowName;
            }
            break;
        case FocusTarget::CellInformation:
            if (m_bShowCellInformationWindow) {
                targetWindowName = kCellInformationWindowName;
            }
            break;
        case FocusTarget::ChargeDensityViewer:
            if (m_bShowChargeDensityViewerWindow) {
                targetWindowName = kChargeDensityViewerWindowName;
            }
            break;
        case FocusTarget::SliceViewer:
            if (m_bShowSliceViewerWindow) {
                targetWindowName = kSliceViewerWindowName;
            }
            break;
        case FocusTarget::None:
            break;
        }

        if (targetWindowName != nullptr) {
            ImGui::SetWindowFocus(targetWindowName);
            --m_PendingFocusPassesRemaining;
        } else {
            m_PendingFocusPassesRemaining = 0;
        }

        if (m_PendingFocusPassesRemaining <= 0) {
            m_PendingFocusTarget = FocusTarget::None;
        }
    }
}

double App::DevicePixelRatio() {
    return emscripten_get_device_pixel_ratio();
}

float App::TextBaseWidth() {
    return ImGui::CalcTextSize("A").x;
}

float App::TextBaseHeight() {
    return ImGui::GetTextLineHeight();
}

float App::TextBaseHeightWithSpacing() {
    return ImGui::GetTextLineHeightWithSpacing();
}

float App::TitleBarHeight() {
    return ImGui::GetFrameHeight();
}

void App::AddTooltip(const char* label, const char* desc) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(label);
        if (desc) {
            ImGui::Separator();
            ImGui::TextDisabled("%s", desc);            
        }
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void App::renderProgressPopup() {
    // Force the popup to be open
    ImGui::OpenPopup(m_PopupTitle.c_str());
    
    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    
    // Create a modal popup that blocks inputs to other windows
    if (ImGui::BeginPopupModal(m_PopupTitle.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("%s", m_PopupText.c_str());
        
        char progressText[32];
        snprintf(progressText, sizeof(progressText), "%.1f%%", m_Progress * 100.0f);
        
        // Progress bar with percentage text
        ImGui::ProgressBar(m_Progress, ImVec2(-1, 0), progressText);
        
        // Only end the popup if it's showing - don't close it here
        ImGui::EndPopup();
    }
}

void App::showProgressPopup(bool show) {
    m_bShowProgressPopup = show;
    VtkViewer::Instance().SetRenderPaused(show);
    if (show) {
        m_Progress = 0.0f;  // Reset progress when showing the popup
    }
    else {
        // Set default values when hiding the popup
        m_PopupTitle = "Loading...";
        m_PopupText = "Loading...";
    }
}

void App::SetProgressPopupText(const std::string& title, const std::string& text) {
    Instance().setProgressPopupText(title, text);
}

void App::RequestLayout1() {
    Instance().m_PendingLayoutPreset = LayoutPreset::DefaultFloating;
    Instance().m_RequestModelTreeFocus = false;
    Instance().m_PendingFocusTarget = FocusTarget::None;
    Instance().m_PendingFocusPassesRemaining = 0;
}

void App::setProgressPopupText(const std::string& title, const std::string& text) {
    m_PopupTitle = title;
    m_PopupText = text;
}
