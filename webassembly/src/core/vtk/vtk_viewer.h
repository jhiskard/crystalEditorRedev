/**
 * @file core/vtk/vtk_viewer.h
 * @brief VTK viewer core with an ImGui-hosted render texture.
 */
#pragma once

#include "viewer_types.h"

#include <imgui.h>
#include <vtkSmartPointer.h>

#include <array>
#include <chrono>
#include <cstdint>

class vtkActor;
class vtkActor2D;
class vtkCamera;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkWebAssemblyOpenGLRenderWindow;
class vtkWebAssemblyRenderWindowInteractor;

namespace core::vtk {

class VtkViewer {
public:
    static VtkViewer& Instance();

    void Init();

    /**
     * @brief Compatibility entry point retained for old callers.
     *
     * Phase 3.7 moves viewer UI ownership to features/viewer.
     */
    void Render();

    bool DrawRenderTexture(const ImVec2& viewportSize, const ImVec2& viewportScreenPos);
    void Resize(int w, int h);

    void AddActor(vtkActor* actor) const;
    void RemoveActor(vtkActor* actor) const;
    void AddActor2D(vtkActor2D* actor) const;
    void RemoveActor2D(vtkActor2D* actor) const;

    void RequestRender() const;
    void FitViewToVisibleProps() const;
    void ResetView();

    void SetProjectionMode(ProjectionMode mode);
    ProjectionMode GetProjectionMode() const { return m_projectionMode; }

    void SetPerformanceOverlayEnabled(bool enabled);
    bool PerformanceOverlayEnabled() const { return m_performanceOverlayEnabled; }

    void SetArrowRotateStepDeg(float degrees);
    float GetArrowRotateStepDeg() const { return m_arrowRotateStepDeg; }
    bool RotateCameraByKeyboard(float azimuthDeg, float elevationDeg);

    bool AlignCameraToCellAxis(const std::array<std::array<float, 3>, 3>& cellMatrix, int axisIndex);

    vtkCamera* GetActiveCamera() const;
    vtkRenderer* GetRenderer() const;
    vtkRenderWindow* GetRenderWindow() const;
    vtkRenderWindowInteractor* GetInteractor() const;

private:
    VtkViewer() = default;
    ~VtkViewer();
    VtkViewer(const VtkViewer&) = delete;
    VtkViewer& operator=(const VtkViewer&) = delete;

    void initFramebuffer();
    void resizeFramebuffer(int width, int height);
    void restoreCanvasSizeAfterVtkResize(int canvasWidth, int canvasHeight, double cssWidth, double cssHeight) const;
    void processViewerInput(
        const ImVec2& viewportSize,
        const ImVec2& viewportScreenPos,
        bool imageHovered,
        bool windowHovered,
        bool windowFocused);
    void setInteractorEventPosition(int x, int y) const;
    void updatePerformanceStats(
        const std::chrono::steady_clock::time_point& now,
        bool didRender,
        float renderMs,
        bool interactionRender);
    void pushDurationSample(std::array<float, 120>& history, int& head, int& count, float value);
    void computeDurationStats(
        const std::array<float, 120>& history,
        int count,
        float& avg,
        float& p95,
        float& maxValue) const;
    void drawPerformanceOverlay(const ImVec2& viewportScreenPos, const ImVec2& viewportSize) const;
    void drawDragSelectionOverlay() const;

    vtkSmartPointer<vtkWebAssemblyOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkWebAssemblyRenderWindowInteractor> m_interactor;

    bool m_initialized = false;
    mutable bool m_renderDirty = true;
    uint32_t m_framebuffer = 0;
    uint32_t m_colorTexture = 0;
    int m_width = 960;
    int m_height = 540;

    ProjectionMode m_projectionMode = ProjectionMode::Perspective;
    CameraDirection m_cameraDirection = CameraDirection::ZPlus;
    bool m_performanceOverlayEnabled = false;
    float m_arrowRotateStepDeg = 45.0f;

    bool m_interactionLodActive = false;
    std::chrono::steady_clock::time_point m_lastWheelInteractionTime {};
    std::chrono::steady_clock::time_point m_lastUiFrameTime {};
    std::chrono::steady_clock::time_point m_lastRenderTime {};
    std::chrono::steady_clock::time_point m_lastInteractionRenderTime {};
    float m_uiFps = 0.0f;
    float m_renderFps = 0.0f;
    float m_interactionRenderFps = 0.0f;
    float m_lastRenderMs = 0.0f;
    std::array<float, 120> m_renderMsHistory {};
    std::array<float, 120> m_interactionRenderMsHistory {};
    int m_renderMsHistoryHead = 0;
    int m_renderMsHistoryCount = 0;
    int m_interactionRenderMsHistoryHead = 0;
    int m_interactionRenderMsHistoryCount = 0;

    bool m_leftButtonDown = false;
    bool m_rightButtonDown = false;
    bool m_middleButtonDown = false;
    bool m_dragSelectionActive = false;
    ImVec2 m_dragStart = ImVec2(0.0f, 0.0f);
    ImVec2 m_dragCurrent = ImVec2(0.0f, 0.0f);
};

} // namespace core::vtk
