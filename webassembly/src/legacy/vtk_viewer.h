#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "macro/singleton_macro.h"
#include "mouse_interactor_style.h"
#include "enum/viewer_enums.h"

// Standard library
#include <array>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

// VTK
#include <vtkRenderer.h>
#include <vtkWebAssemblyOpenGLRenderWindow.h>
#include <vtkWebAssemblyRenderWindowInteractor.h>
#include <vtkCellPicker.h>

// class vtkAxesActor;
// class vtkOrientationMarkerWidget;
class vtkActor;
class vtkCamera;
class vtkCameraOrientationWidget;
class vtkVolume;


class VtkViewer {
    DECLARE_SINGLETON(VtkViewer)

public:
    void Render(bool* openWindow = nullptr);
    void RenderBgColorPopup(bool* openWindow = nullptr);
    void RequestInitialLayout();
    void RequestForcedWindowLayout(const ImVec2& pos, const ImVec2& size, int frames = 1);
    void ResetView();
    void FitViewToVisibleProps();
    void AlignCameraToCellAxis(int axisIndex);
    void AlignCameraToIcellAxis(int axisIndex);
    void SetRenderPaused(bool paused);
    bool IsRenderPaused() const { return m_RenderPaused; }
    void RequestRender();
    void SetPerformanceOverlayEnabled(bool enabled);
    bool IsPerformanceOverlayEnabled() const { return m_ShowPerformanceOverlay; }
    void BeginInteractionLod();
    void EndInteractionLod();
    bool IsInteractionLodActive() const { return m_InteractionLodActive; }

    void AddActor(vtkSmartPointer<vtkActor> actor, bool resetCamera = true);
    void AddMeasurementOverlayActor(vtkSmartPointer<vtkActor> actor, bool resetCamera = true);
    void AddActor2D(
        vtkSmartPointer<vtkActor2D> actor2D,
        bool resetCamera = true,
        bool measurementOverlay = false);
    void AddVolume(vtkSmartPointer<vtkVolume> volume, bool resetCamera = true);
    bool CaptureActorImage(vtkActor* actor,
                           std::vector<uint8_t>& rgbaPixels,
                           int& width,
                           int& height,
                           bool isolateActor = true,
                           const double* viewDirection = nullptr,
                           const double* viewUpHint = nullptr,
                           const double* backgroundColorRgb = nullptr);

    vtkCamera* GetActiveCamera();

    void RemoveActor(vtkSmartPointer<vtkActor> actor);
    void RemoveMeasurementOverlayActor(vtkSmartPointer<vtkActor> actor);
    void RemoveActor2D(vtkSmartPointer<vtkActor2D> actor2D);
    void RemoveVolume(vtkSmartPointer<vtkVolume> volume);

    void SetProjectionMode(ProjectionMode mode);
    void SetArrowRotateStepDeg(float stepDeg);
    float GetArrowRotateStepDeg() const { return m_ArrowRotateStepDeg; }
    
private:
    bool m_FirstRender = true;
    bool m_RequestInitialLayout = false;
    bool m_RenderPaused = false;
    bool m_RenderDirty = true;
    ImVec2 m_PreferredSize = ImVec2(1280, 960);  // 선호하는 창 크기
    ImVec2 m_PreferredPos = ImVec2(300, 50);    // 선호하는 창 위치
    bool m_HasForcedWindowLayout = false;
    ImVec2 m_ForcedWindowPos = ImVec2(0.0f, 0.0f);
    ImVec2 m_ForcedWindowSize = ImVec2(0.0f, 0.0f);
    int m_ForcedWindowLayoutFramesRemaining = 0;
    vtkSmartPointer<vtkCellPicker> m_Picker;

    const char* IMGUI_BGCOLOR_KEY { "VtkViewer-BgColor" };

    vtkSmartPointer<vtkRenderer> m_Renderer;
    vtkSmartPointer<vtkRenderer> m_MeasurementOverlayRenderer;
    vtkSmartPointer<vtkWebAssemblyOpenGLRenderWindow> m_RenderWindow;
    vtkSmartPointer<vtkWebAssemblyRenderWindowInteractor> m_Interactor;
    vtkSmartPointer<MouseInteractorStyle> m_InteractorStyle;

    // vtkSmartPointer<vtkAxesActor> m_AxesActor;
    // vtkSmartPointer<vtkOrientationMarkerWidget> m_OrientationMarkerWidget;
    vtkSmartPointer<vtkCameraOrientationWidget> m_CameraOrientationWidget;

    uint32_t m_Framebuffer { 0 };
    uint32_t m_ColorTexture { 0 };
    // uint32_t m_DepthTexture { 0 };
    int32_t m_Width { 960 };
    int32_t m_Height { 640 };

    std::array<int32_t, 2> m_MousePosition { 0, 0 };
    CameraDirection m_CameraDirection { CameraDirection::ZPLUS_YPLUS };

    const std::array<int32_t, 2>& getMousePosition() const;
    CameraDirection getCameraDirection() const { return m_CameraDirection; }

    void init();
    void initFramebuffer();
    void resizeFramebuffer(int32_t width, int32_t height);
    void clearFramebuffer();
    void setInteractionCallbacks();
    void processEvents();
    void resetDragSelection();
    void renderDragSelectionOverlay(const ImVec2& windowPos, const ImVec2& viewportPos) const;
    bool RotateCameraByKeyboard(float azimuthDeg, float elevationDeg);
    void setCameraWidgetCallback();
    void setMousePosition(const int32_t* pos);
    void setCameraDirection(CameraDirection dir) { m_CameraDirection = dir; }
    void updatePerformanceStats(const std::chrono::steady_clock::time_point& now,
        bool didRender,
        float renderMs,
        bool interactionRender);
    void pushDurationSample(std::array<float, 120>& history, int& head, int& count, float value);
    void computeDurationStats(const std::array<float, 120>& history,
        int count,
        float& avg,
        float& p95,
        float& maxValue) const;
    void renderPerformanceOverlay(const ImVec2& windowPos, const ImVec2& viewportPos) const;

    void saveBgColorToLocalStorage(const float* colorTop, const float* colorBottom);
    void loadBgColorFromLocalStorage(float* colorTop, float* colorBottom);

    struct InteractionLodState {
        int volumeQualityIndex { 0 };
        int sampleDistanceIndex { 0 };
        bool autoAdjustSampleDistances { false };
        bool interactiveAdjustSampleDistances { false };
    };
    bool m_InteractionLodActive { false };
    std::unordered_map<int32_t, InteractionLodState> m_InteractionLodStates;
    std::chrono::steady_clock::time_point m_LastWheelInteractionTime {};

    bool m_ShowPerformanceOverlay { false };
    std::chrono::steady_clock::time_point m_LastUiFrameTime {};
    std::chrono::steady_clock::time_point m_LastRenderTime {};
    std::chrono::steady_clock::time_point m_LastInteractionRenderTime {};
    float m_UiFps { 0.0f };
    float m_RenderFps { 0.0f };
    float m_InteractionRenderFps { 0.0f };
    float m_LastRenderMs { 0.0f };
    float m_ArrowRotateStepDeg { 45.0f };
    std::array<float, 120> m_RenderMsHistory {};
    std::array<float, 120> m_InteractionRenderMsHistory {};
    int m_RenderMsHistoryHead { 0 };
    int m_RenderMsHistoryCount { 0 };
    int m_InteractionRenderMsHistoryHead { 0 };
    int m_InteractionRenderMsHistoryCount { 0 };

    struct DragSelectionState {
        bool tracking { false };
        bool active { false };
        bool measurementModeAtStart { false };
        bool sameElementSelectAtStart { false };
        ImVec2 startPos = ImVec2(0.0f, 0.0f);   // Viewer local (top-left origin)
        ImVec2 currentPos = ImVec2(0.0f, 0.0f); // Viewer local (top-left origin)
    };
    DragSelectionState m_DragSelection {};
};
