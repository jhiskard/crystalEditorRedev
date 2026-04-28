#pragma once

#include "macro/singleton_macro.h"
#include "config/log_config.h"
#include "enum/app_enums.h"


// Application class
// - Initialize logger
// - Initialize IdbFS
// - Render Dockspace & Menu
// - Render all ImGui windows
// - Save/Load ImGui settings
// - Set ImGui style
class App {
    DECLARE_SINGLETON(App)

public:
    enum class FontSizePreset {
        Small = 0,
        Medium = 1,
        Large = 2
    };

    static void InitIdbfs();  // Mount IdbFS
    static void SaveImGuiIniFile();
    static void LoadImGuiIniFile();

    void SetInitColorStyle();
    void SetInitFontSize();
    static void SetColorStyle(ColorStyle style) { Instance().setColorStyle(style); }
    static ColorStyle GetColorStyle() { return Instance().getColorStyle(); }
    static void SetFontSizePreset(FontSizePreset preset) { Instance().setFontSizePreset(preset); }
    static FontSizePreset GetFontSizePreset() { return Instance().getFontSizePreset(); }
    static float UiScale() { return Instance().m_kUiScale; }
    void SetDetailedStyle();

    void InitImGuiWindows();
    void Render();
    void renderImGuiWindows(); // Changed to public to allow calling from FileLoader

    static double DevicePixelRatio();
    static float TextBaseWidth();
    static float TextBaseHeight();
    static float TextBaseHeightWithSpacing();
    static float TitleBarHeight();

    static void AddTooltip(const char* label, const char* desc = nullptr);

    void SetProgress(float progress) { m_Progress = progress; }
    static void ShowProgressPopup(bool show) { Instance().showProgressPopup(show); }
    static void SetProgressPopupText(const std::string& title, const std::string& text);

    // Helpers to show/hide split windows mapped from old grouped names.
    static void ShowCrystalBuilderWindow(bool show = true) { Instance().m_bShowPeriodicTableWindow = show; }
    static void ShowCrystalEditorWindow(bool show = true) { Instance().m_bShowCreatedAtomsWindow = show; }
    static void ShowAdvancedViewWindow(bool show = true) { Instance().m_bShowChargeDensityViewerWindow = show; }
    static void ShowSliceViewerWindow(bool show = true) { Instance().m_bShowSliceViewerWindow = show; }
    // Backward-compatible wrapper for the old Atomistic Model Builder name.
    static void ShowAtomsTemplateWindow(bool show = true) { Instance().m_bShowPeriodicTableWindow = show; }
    static void RequestLayout1();

private:
    enum class LayoutPreset {
        None,
        DefaultFloating,
        DockRight,
        DockBottom,
        ResetDocking,
    };
    enum class FocusTarget {
        None,
        ModelTree,
        PeriodicTable,
        CrystalTemplates,
        BrillouinZonePlot,
        CreatedAtoms,
        BondsManagement,
        CellInformation,
        ChargeDensityViewer,
        SliceViewer,
    };

    const char* IDBFS_MOUNT_PATH { "/settings" };
    const char* IMGUI_SETTING_FILE_PATH { "/settings/imgui.ini" };
    const char* WINDOW_SETTING_FILE_PATH { "/settings/window_visibility.ini" };
    const char* IMGUI_STYLE_KEY { "App-ColorStyle" };
    const char* IMGUI_FONT_SIZE_KEY { "App-FontSize" };

    ColorStyle m_ColorStyle { ColorStyle::Dark };
    FontSizePreset m_FontSizePreset { FontSizePreset::Small };
    float m_kUiScale { 1.0f };

    bool m_bShowAboutPopup = false;

    bool m_bFullDockSpace = true;
#ifdef SHOW_IMGUI_DEMO
    bool m_bShowDemoWindow = false;
#endif
#ifdef SHOW_FONT_ICONS
    bool m_bShowFontIcons = true;
#endif
    bool m_bShowVtkViewer = true;
    // bool m_bShowVtkViewer2 = true;
    bool m_bShowModelTree = false;
    bool m_bShowTestWindow = false;
    bool m_bShowPeriodicTableWindow = false;
    bool m_bShowCrystalTemplatesWindow = false;
    bool m_bShowBrillouinZonePlotWindow = false;
    bool m_bShowCreatedAtomsWindow = false;
    bool m_bShowBondsManagementWindow = false;
    bool m_bShowCellInformationWindow = false;
    bool m_bShowChargeDensityViewerWindow = false;
    bool m_bShowSliceViewerWindow = false;
    bool m_bShowBgColorPopup = false;
    bool m_bShowProgressPopup = false;
    bool m_bShowMeshDetail = false;
    bool m_RequestModelTreeFocus = false;
    FocusTarget m_PendingFocusTarget { FocusTarget::None };
    int m_PendingFocusPassesRemaining = 0;
    int m_ResetWindowGeometryPassesRemaining = 0;
    bool m_ShouldApplyInitialLayout = false;


    LayoutPreset m_PendingLayoutPreset { LayoutPreset::None };

    float m_Progress { 0.0f };
    std::string m_PopupTitle { "Loading..." };
    std::string m_PopupText { "Loading..." };

    // useConsole: true - log to console, false - log to file
    // newFile: true - create new log file, false - append to the previous log file
    // If useConsole is true, newFile is ignored.
    void initLogger(bool useConsole = true, bool newFile = true);
    void shutdownLogger();

    void setColorStyle(ColorStyle style);
    ColorStyle getColorStyle() const { return m_ColorStyle; }
    void setFontSizePreset(FontSizePreset preset);
    FontSizePreset getFontSizePreset() const { return m_FontSizePreset; }
    void applyFontScale();

    void renderDockSpaceAndMenu();
    // renderImGuiWindows is now public

    void renderProgressPopup();
    void showProgressPopup(bool show);
    void setProgressPopupText(const std::string& title, const std::string& text);
    void renderAboutPopup();

};
