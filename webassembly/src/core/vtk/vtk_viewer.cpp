/**
 * @file core/vtk/vtk_viewer.cpp
 * @brief VTK viewer core with an ImGui-hosted render texture.
 */
#include "vtk_viewer.h"

#include "mouse_interactor.h"

#define GLFW_INCLUDE_ES3
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include <emscripten/html5.h>

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkMath.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkWebAssemblyOpenGLRenderWindow.h>
#include <vtkWebAssemblyRenderWindowInteractor.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace core::vtk {
namespace {

constexpr int kMinViewportSize = 16;
constexpr double kDefaultCameraDistance = 10.0;
constexpr float kFpsSmoothingFactor = 0.15f;
constexpr auto kWheelInteractionLodHoldDuration = std::chrono::milliseconds(150);

bool normalize(double (&value)[3]) {
    const double norm = vtkMath::Norm(value);
    if (norm <= 1.0e-9) {
        return false;
    }
    value[0] /= norm;
    value[1] /= norm;
    value[2] /= norm;
    return true;
}

bool boundsInitialized(const double (&bounds)[6]) {
    return bounds[1] >= bounds[0] && bounds[3] >= bounds[2] && bounds[5] >= bounds[4];
}

bool isWheelInteractionLodHoldActive(
    const std::chrono::steady_clock::time_point& lastWheelTime,
    const std::chrono::steady_clock::time_point& now) {
    if (lastWheelTime == std::chrono::steady_clock::time_point{}) {
        return false;
    }
    return (now - lastWheelTime) <= kWheelInteractionLodHoldDuration;
}

} // namespace

VtkViewer& VtkViewer::Instance() {
    static VtkViewer instance;
    return instance;
}

VtkViewer::~VtkViewer() {
    if (m_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    if (m_colorTexture != 0) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
}

void VtkViewer::Init() {
    if (m_initialized) {
        return;
    }

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderWindow = vtkSmartPointer<vtkWebAssemblyOpenGLRenderWindow>::New();
    m_interactor = vtkSmartPointer<vtkWebAssemblyRenderWindowInteractor>::New();

    m_renderer->SetGradientBackground(true);
    m_renderer->SetBackground(0.02, 0.02, 0.025);
    m_renderer->SetBackground2(0.94, 0.94, 0.96);
    m_renderer->ResetCamera();

    m_renderWindow->AddRenderer(m_renderer);
    m_renderWindow->SetMultiSamples(0);
    m_renderWindow->SetSize(m_width, m_height);
    m_renderWindow->SwapBuffersOn();
    m_renderWindow->SetOffScreenRendering(true);
    m_renderWindow->SetFrameBlitModeToNoBlit();
    m_renderWindow->SetInteractor(m_interactor);

    m_interactor->SetRenderWindow(m_renderWindow);
    m_interactor->SetSize(m_width, m_height);
    m_interactor->EnableRenderOff();

    m_initialized = true;
    initFramebuffer();
    RequestRender();
}

void VtkViewer::Render() {
    RequestRender();
}

bool VtkViewer::DrawRenderTexture(const ImVec2& viewportSize, const ImVec2& viewportScreenPos) {
    if (!m_initialized || m_renderWindow == nullptr || m_colorTexture == 0) {
        return false;
    }

    const int width = std::max(kMinViewportSize, static_cast<int>(std::round(viewportSize.x)));
    const int height = std::max(kMinViewportSize, static_cast<int>(std::round(viewportSize.y)));
    if (width != m_width || height != m_height) {
        resizeFramebuffer(width, height);
    }

    const auto now = std::chrono::steady_clock::now();
    const bool interactionRender = m_interactionLodActive;
    bool didRender = false;
    float renderMs = 0.0f;
    if (m_renderDirty || interactionRender) {
        const auto renderStart = std::chrono::steady_clock::now();
        m_renderWindow->Render();
        m_renderWindow->WaitForCompletion();
        const auto renderEnd = std::chrono::steady_clock::now();
        renderMs = std::chrono::duration<float, std::milli>(renderEnd - renderStart).count();
        didRender = true;
        m_renderDirty = false;
    }
    updatePerformanceStats(now, didRender, renderMs, interactionRender);

    ImGui::Image(
        static_cast<ImTextureID>(static_cast<uintptr_t>(m_colorTexture)),
        ImVec2(static_cast<float>(m_width), static_cast<float>(m_height)),
        ImVec2(0.0f, 1.0f),
        ImVec2(1.0f, 0.0f));

    const bool imageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool windowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool windowFocused = ImGui::IsWindowFocused();
    processViewerInput(
        ImVec2(static_cast<float>(m_width), static_cast<float>(m_height)),
        viewportScreenPos,
        imageHovered,
        windowHovered,
        windowFocused);
    drawDragSelectionOverlay();
    drawPerformanceOverlay(viewportScreenPos, ImVec2(static_cast<float>(m_width), static_cast<float>(m_height)));
    return true;
}

void VtkViewer::Resize(int w, int h) {
    resizeFramebuffer(w, h);
}

void VtkViewer::AddActor(vtkActor* actor) const {
    if (m_renderer == nullptr || actor == nullptr) {
        return;
    }
    m_renderer->AddActor(actor);
    RequestRender();
}

void VtkViewer::RemoveActor(vtkActor* actor) const {
    if (m_renderer == nullptr || actor == nullptr) {
        return;
    }
    m_renderer->RemoveActor(actor);
    RequestRender();
}

void VtkViewer::AddActor2D(vtkActor2D* actor) const {
    if (m_renderer == nullptr || actor == nullptr) {
        return;
    }
    m_renderer->AddActor2D(actor);
    RequestRender();
}

void VtkViewer::RemoveActor2D(vtkActor2D* actor) const {
    if (m_renderer == nullptr || actor == nullptr) {
        return;
    }
    m_renderer->RemoveActor2D(actor);
    RequestRender();
}

void VtkViewer::RequestRender() const {
    m_renderDirty = true;
}

void VtkViewer::FitViewToVisibleProps() const {
    if (m_renderer == nullptr) {
        return;
    }
    m_renderer->ResetCamera();
    m_renderer->ResetCameraClippingRange();
    RequestRender();
}

void VtkViewer::ResetView() {
    if (vtkCamera* camera = GetActiveCamera(); camera != nullptr) {
        camera->SetViewUp(0.0, 1.0, 0.0);
    }
    m_cameraDirection = CameraDirection::ZPlus;
    FitViewToVisibleProps();
}

void VtkViewer::SetProjectionMode(ProjectionMode mode) {
    m_projectionMode = mode;

    vtkCamera* camera = GetActiveCamera();
    if (camera == nullptr) {
        return;
    }

    if (mode == ProjectionMode::Parallel) {
        camera->ParallelProjectionOn();
    } else {
        camera->ParallelProjectionOff();
    }
    RequestRender();
}

void VtkViewer::SetPerformanceOverlayEnabled(bool enabled) {
    if (m_performanceOverlayEnabled == enabled) {
        return;
    }
    m_performanceOverlayEnabled = enabled;
    RequestRender();
}

void VtkViewer::SetArrowRotateStepDeg(float degrees) {
    m_arrowRotateStepDeg = std::clamp(degrees, 1.0f, 180.0f);
}

bool VtkViewer::RotateCameraByKeyboard(float azimuthDeg, float elevationDeg) {
    vtkCamera* camera = GetActiveCamera();
    if (camera == nullptr || (azimuthDeg == 0.0f && elevationDeg == 0.0f)) {
        return false;
    }

    if (azimuthDeg != 0.0f) {
        camera->Azimuth(static_cast<double>(azimuthDeg));
    }
    if (elevationDeg != 0.0f) {
        camera->Elevation(static_cast<double>(elevationDeg));
    }
    camera->OrthogonalizeViewUp();
    if (m_renderer != nullptr) {
        m_renderer->ResetCameraClippingRange();
    }
    m_cameraDirection = CameraDirection::NotAligned;
    RequestRender();
    return true;
}

bool VtkViewer::AlignCameraToCellAxis(const std::array<std::array<float, 3>, 3>& cellMatrix, int axisIndex) {
    if (axisIndex < 0 || axisIndex > 2 || m_renderer == nullptr) {
        return false;
    }

    vtkCamera* camera = GetActiveCamera();
    if (camera == nullptr) {
        return false;
    }

    double direction[3] = {
        static_cast<double>(cellMatrix[axisIndex][0]),
        static_cast<double>(cellMatrix[axisIndex][1]),
        static_cast<double>(cellMatrix[axisIndex][2]),
    };
    if (!normalize(direction)) {
        return false;
    }

    int upIndex = (axisIndex + 1) % 3;
    double up[3] = {
        static_cast<double>(cellMatrix[upIndex][0]),
        static_cast<double>(cellMatrix[upIndex][1]),
        static_cast<double>(cellMatrix[upIndex][2]),
    };
    if (!normalize(up) || std::abs(vtkMath::Dot(direction, up)) > 0.95) {
        up[0] = 0.0;
        up[1] = axisIndex == 1 ? 0.0 : 1.0;
        up[2] = axisIndex == 1 ? 1.0 : 0.0;
    }

    const double dot = vtkMath::Dot(direction, up);
    up[0] -= dot * direction[0];
    up[1] -= dot * direction[1];
    up[2] -= dot * direction[2];
    normalize(up);

    double bounds[6] = {1.0, -1.0, 1.0, -1.0, 1.0, -1.0};
    m_renderer->ComputeVisiblePropBounds(bounds);

    double center[3] = {0.0, 0.0, 0.0};
    double distance = kDefaultCameraDistance;
    if (boundsInitialized(bounds)) {
        center[0] = 0.5 * (bounds[0] + bounds[1]);
        center[1] = 0.5 * (bounds[2] + bounds[3]);
        center[2] = 0.5 * (bounds[4] + bounds[5]);
        const double dx = bounds[1] - bounds[0];
        const double dy = bounds[3] - bounds[2];
        const double dz = bounds[5] - bounds[4];
        distance = std::max(kDefaultCameraDistance, 1.8 * std::sqrt(dx * dx + dy * dy + dz * dz));
        camera->SetParallelScale(std::max(1.0, 0.65 * std::sqrt(dx * dx + dy * dy + dz * dz)));
    }

    camera->SetFocalPoint(center);
    camera->SetPosition(
        center[0] + direction[0] * distance,
        center[1] + direction[1] * distance,
        center[2] + direction[2] * distance);
    camera->SetViewUp(up);
    camera->OrthogonalizeViewUp();

    m_renderer->ResetCameraClippingRange();
    m_cameraDirection = axisIndex == 0 ? CameraDirection::XPlus :
        (axisIndex == 1 ? CameraDirection::YPlus : CameraDirection::ZPlus);
    RequestRender();
    return true;
}

vtkCamera* VtkViewer::GetActiveCamera() const {
    if (m_renderer == nullptr) {
        return nullptr;
    }
    return m_renderer->GetActiveCamera();
}

vtkRenderer* VtkViewer::GetRenderer() const {
    return m_renderer.GetPointer();
}

vtkRenderWindow* VtkViewer::GetRenderWindow() const {
    return m_renderWindow.GetPointer();
}

vtkRenderWindowInteractor* VtkViewer::GetInteractor() const {
    return m_interactor.GetPointer();
}

void VtkViewer::initFramebuffer() {
    if (!m_initialized || m_renderWindow == nullptr || m_interactor == nullptr) {
        return;
    }

    if (m_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    if (m_colorTexture != 0) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }

    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

    int canvasWidth = 0;
    int canvasHeight = 0;
    emscripten_get_canvas_element_size("canvas", &canvasWidth, &canvasHeight);
    double cssWidth = 0.0;
    double cssHeight = 0.0;
    emscripten_get_element_css_size("canvas", &cssWidth, &cssHeight);

    m_renderWindow->InitializeFromCurrentContext();
    m_renderWindow->SetSize(m_width, m_height);
    m_interactor->SetSize(m_width, m_height);

    if (vtkOpenGLFramebufferObject* framebufferObject = m_renderWindow->GetDisplayFramebuffer();
        framebufferObject != nullptr) {
        framebufferObject->Bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
        framebufferObject->UnBind();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    restoreCanvasSizeAfterVtkResize(canvasWidth, canvasHeight, cssWidth, cssHeight);
    RequestRender();
}

void VtkViewer::resizeFramebuffer(int width, int height) {
    width = std::max(kMinViewportSize, width);
    height = std::max(kMinViewportSize, height);
    if (width == m_width && height == m_height && m_colorTexture != 0) {
        return;
    }

    m_width = width;
    m_height = height;
    initFramebuffer();
}

void VtkViewer::restoreCanvasSizeAfterVtkResize(
    int canvasWidth,
    int canvasHeight,
    double cssWidth,
    double cssHeight) const {
    if (canvasWidth > 0 && canvasHeight > 0) {
        if (GLFWwindow* window = glfwGetCurrentContext(); window != nullptr) {
            glfwSetWindowSize(window, canvasWidth, canvasHeight);
        }
    }
    if (cssWidth > 0.0 && cssHeight > 0.0) {
        emscripten_set_element_css_size("canvas", cssWidth, cssHeight);
    }
}

void VtkViewer::processViewerInput(
    const ImVec2& viewportSize,
    const ImVec2& viewportScreenPos,
    bool imageHovered,
    bool windowHovered,
    bool windowFocused) {
    if (m_interactor == nullptr) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    const auto now = std::chrono::steady_clock::now();
    auto setInteractionLodActive = [&](bool active) {
        if (m_interactionLodActive == active) {
            return;
        }
        m_interactionLodActive = active;
        RequestRender();
    };

    const bool pointerHeld = m_leftButtonDown || m_rightButtonDown || m_middleButtonDown;
    const bool viewportActive = imageHovered || windowHovered || windowFocused || pointerHeld;
    if (!viewportActive || io.WantTextInput) {
        if (!pointerHeld && !isWheelInteractionLodHoldActive(m_lastWheelInteractionTime, now)) {
            setInteractionLodActive(false);
        }
        return;
    }

    const int x = std::clamp(
        static_cast<int>(std::round(io.MousePos.x - viewportScreenPos.x)),
        0,
        std::max(0, static_cast<int>(viewportSize.x) - 1));
    const int y = std::clamp(
        static_cast<int>(std::round(io.MousePos.y - viewportScreenPos.y)),
        0,
        std::max(0, static_cast<int>(viewportSize.y) - 1));

    const bool selectionModifier = io.KeyCtrl || io.KeySuper;

    if (imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImGui::SetWindowFocus();
        setInteractionLodActive(true);
        setInteractorEventPosition(x, y);
        m_leftButtonDown = true;
        m_dragSelectionActive = false;
        m_dragStart = ImVec2(static_cast<float>(x), static_cast<float>(y));
        m_dragCurrent = m_dragStart;
        m_interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
        RequestRender();
    }

    if (imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::SetWindowFocus();
        setInteractionLodActive(true);
        setInteractorEventPosition(x, y);
        m_rightButtonDown = true;
        m_interactor->InvokeEvent(vtkCommand::RightButtonPressEvent, nullptr);
        RequestRender();
    }

    if (imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        ImGui::SetWindowFocus();
        setInteractionLodActive(true);
        setInteractorEventPosition(x, y);
        m_middleButtonDown = true;
        m_interactor->InvokeEvent(vtkCommand::MiddleButtonPressEvent, nullptr);
        RequestRender();
    }

    const bool anyTrackedButtonDown =
        (m_leftButtonDown && io.MouseDown[ImGuiMouseButton_Left]) ||
        (m_rightButtonDown && io.MouseDown[ImGuiMouseButton_Right]) ||
        (m_middleButtonDown && io.MouseDown[ImGuiMouseButton_Middle]);
    if (anyTrackedButtonDown) {
        setInteractorEventPosition(x, y);
        if (m_leftButtonDown) {
            m_dragCurrent = ImVec2(static_cast<float>(x), static_cast<float>(y));
            const float dx = m_dragCurrent.x - m_dragStart.x;
            const float dy = m_dragCurrent.y - m_dragStart.y;
            if (selectionModifier && (dx * dx + dy * dy) > 16.0f) {
                m_dragSelectionActive = true;
            }
        }
        m_interactor->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
        RequestRender();
    }

    if (m_leftButtonDown && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        setInteractorEventPosition(x, y);
        m_interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
        m_leftButtonDown = false;
        m_dragSelectionActive = false;
        RequestRender();
    }

    if (m_rightButtonDown && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        setInteractorEventPosition(x, y);
        m_interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent, nullptr);
        m_rightButtonDown = false;
        RequestRender();
    }
    if (m_middleButtonDown && ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
        setInteractorEventPosition(x, y);
        m_interactor->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, nullptr);
        m_middleButtonDown = false;
        RequestRender();
    }

    if (m_leftButtonDown && !io.MouseDown[ImGuiMouseButton_Left]) {
        m_leftButtonDown = false;
        m_dragSelectionActive = false;
    }
    if (m_rightButtonDown && !io.MouseDown[ImGuiMouseButton_Right]) {
        m_rightButtonDown = false;
    }
    if (m_middleButtonDown && !io.MouseDown[ImGuiMouseButton_Middle]) {
        m_middleButtonDown = false;
    }

    if (imageHovered && io.MouseWheel != 0.0f) {
        m_lastWheelInteractionTime = now;
        setInteractionLodActive(true);
        setInteractorEventPosition(x, y);
        m_interactor->InvokeEvent(
            io.MouseWheel > 0.0f ? vtkCommand::MouseWheelForwardEvent : vtkCommand::MouseWheelBackwardEvent,
            nullptr);
        RequestRender();
    }

    const bool typingInWidget = io.WantTextInput || ImGui::IsAnyItemActive();
    if ((windowFocused || imageHovered) && !typingInWidget) {
        const float step = GetArrowRotateStepDeg();
        bool rotated = false;
        rotated = ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ? RotateCameraByKeyboard(-step, 0.0f) : rotated;
        rotated = ImGui::IsKeyPressed(ImGuiKey_RightArrow) ? RotateCameraByKeyboard(step, 0.0f) : rotated;
        rotated = ImGui::IsKeyPressed(ImGuiKey_UpArrow) ? RotateCameraByKeyboard(0.0f, step) : rotated;
        rotated = ImGui::IsKeyPressed(ImGuiKey_DownArrow) ? RotateCameraByKeyboard(0.0f, -step) : rotated;
        if (rotated) {
            RequestRender();
        }
    }

    const bool keepInteractionLod =
        m_leftButtonDown ||
        m_rightButtonDown ||
        m_middleButtonDown ||
        isWheelInteractionLodHoldActive(m_lastWheelInteractionTime, std::chrono::steady_clock::now());
    if (!keepInteractionLod) {
        setInteractionLodActive(false);
    }
}

void VtkViewer::setInteractorEventPosition(int x, int y) const {
    if (m_interactor == nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const int ctrl = (io.KeyCtrl || io.KeySuper) ? 1 : 0;
    const int shift = io.KeyShift ? 1 : 0;
    const int doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ? 1 : 0;
    m_interactor->SetEventInformationFlipY(x, y, ctrl, shift, '\0', doubleClick, nullptr);
    if (MouseInteractor* style = MouseInteractor::SafeDownCast(m_interactor->GetInteractorStyle());
        style != nullptr && doubleClick != 0) {
        style->SetDoubleClickHint(true);
    }
}

void VtkViewer::pushDurationSample(std::array<float, 120>& history, int& head, int& count, float value) {
    if (history.empty()) {
        return;
    }

    history[static_cast<size_t>(head)] = value;
    head = (head + 1) % static_cast<int>(history.size());
    if (count < static_cast<int>(history.size())) {
        ++count;
    }
}

void VtkViewer::computeDurationStats(
    const std::array<float, 120>& history,
    int count,
    float& avg,
    float& p95,
    float& maxValue) const {
    avg = 0.0f;
    p95 = 0.0f;
    maxValue = 0.0f;
    if (count <= 0) {
        return;
    }

    std::vector<float> samples;
    samples.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const float value = history[static_cast<size_t>(i)];
        samples.push_back(value);
        avg += value;
        if (i == 0 || value > maxValue) {
            maxValue = value;
        }
    }
    avg /= static_cast<float>(count);

    std::sort(samples.begin(), samples.end());
    size_t p95Index = static_cast<size_t>(std::ceil(static_cast<float>(count) * 0.95f));
    if (p95Index == 0) {
        p95Index = 1;
    }
    if (p95Index > samples.size()) {
        p95Index = samples.size();
    }
    p95 = samples[p95Index - 1];
}

void VtkViewer::updatePerformanceStats(
    const std::chrono::steady_clock::time_point& now,
    bool didRender,
    float renderMs,
    bool interactionRender) {
    auto smoothFps = [](float current, float measured) {
        if (measured <= 0.0f) {
            return current;
        }
        if (current <= 0.0f) {
            return measured;
        }
        return current + (measured - current) * kFpsSmoothingFactor;
    };

    if (m_lastUiFrameTime != std::chrono::steady_clock::time_point{}) {
        const float deltaSec = std::chrono::duration<float>(now - m_lastUiFrameTime).count();
        if (deltaSec > 0.0f) {
            m_uiFps = smoothFps(m_uiFps, 1.0f / deltaSec);
        }
    }
    m_lastUiFrameTime = now;

    if (!didRender) {
        return;
    }

    m_lastRenderMs = renderMs;
    pushDurationSample(m_renderMsHistory, m_renderMsHistoryHead, m_renderMsHistoryCount, renderMs);

    if (m_lastRenderTime != std::chrono::steady_clock::time_point{}) {
        const float deltaSec = std::chrono::duration<float>(now - m_lastRenderTime).count();
        if (deltaSec > 0.0f) {
            m_renderFps = smoothFps(m_renderFps, 1.0f / deltaSec);
        }
    }
    m_lastRenderTime = now;

    if (!interactionRender) {
        return;
    }

    pushDurationSample(
        m_interactionRenderMsHistory,
        m_interactionRenderMsHistoryHead,
        m_interactionRenderMsHistoryCount,
        renderMs);

    if (m_lastInteractionRenderTime != std::chrono::steady_clock::time_point{}) {
        const float deltaSec = std::chrono::duration<float>(now - m_lastInteractionRenderTime).count();
        if (deltaSec > 0.0f) {
            m_interactionRenderFps = smoothFps(m_interactionRenderFps, 1.0f / deltaSec);
        }
    }
    m_lastInteractionRenderTime = now;
}

void VtkViewer::drawPerformanceOverlay(const ImVec2& viewportScreenPos, const ImVec2& viewportSize) const {
    if (!m_performanceOverlayEnabled) {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList == nullptr) {
        return;
    }

    float renderAvg = 0.0f;
    float renderP95 = 0.0f;
    float renderMax = 0.0f;
    computeDurationStats(m_renderMsHistory, m_renderMsHistoryCount, renderAvg, renderP95, renderMax);

    float interactionAvg = 0.0f;
    float interactionP95 = 0.0f;
    float interactionMax = 0.0f;
    computeDurationStats(
        m_interactionRenderMsHistory,
        m_interactionRenderMsHistoryCount,
        interactionAvg,
        interactionP95,
        interactionMax);

    const ImGuiIO& io = ImGui::GetIO();
    char line[256];
    std::array<std::string, 6> lines;
    lines[0] = "Viewer FPS Metrics";
    std::snprintf(line, sizeof(line), "UI FPS: %.1f | VTK FPS: %.1f", m_uiFps, m_renderFps);
    lines[1] = line;
    std::snprintf(
        line,
        sizeof(line),
        "Render ms (last/avg/p95/max): %.2f / %.2f / %.2f / %.2f",
        static_cast<double>(m_lastRenderMs),
        static_cast<double>(renderAvg),
        static_cast<double>(renderP95),
        static_cast<double>(renderMax));
    lines[2] = line;
    std::snprintf(line, sizeof(line), "Interaction LOD: %s", m_interactionLodActive ? "ON" : "OFF");
    lines[3] = line;
    std::snprintf(line, sizeof(line), "Interaction FPS: %.1f", m_interactionRenderFps);
    lines[4] = line;
    std::snprintf(
        line,
        sizeof(line),
        "Interaction ms (avg/p95/max): %.2f / %.2f / %.2f",
        static_cast<double>(interactionAvg),
        static_cast<double>(interactionP95),
        static_cast<double>(interactionMax));
    lines[5] = line;

    float maxTextWidth = 0.0f;
    for (const std::string& textLine : lines) {
        maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(textLine.c_str()).x);
    }

    const float uiScale = std::max(1.0f, io.FontGlobalScale);
    const float lineHeight = ImGui::GetTextLineHeight();
    const float lineSpacing = 4.0f * uiScale;
    const float paddingX = 8.0f * uiScale;
    const float paddingY = 8.0f * uiScale;
    const float margin = 4.0f * uiScale;
    const float toolbarReservedHeight = 58.0f * uiScale;
    const float width = maxTextWidth + paddingX * 2.0f;
    const float height = paddingY * 2.0f +
                         static_cast<float>(lines.size()) * lineHeight +
                         static_cast<float>(lines.size() - 1) * lineSpacing;

    ImVec2 origin(
        viewportScreenPos.x + 12.0f * uiScale,
        viewportScreenPos.y + toolbarReservedHeight);

    const float minX = viewportScreenPos.x + margin;
    const float minY = viewportScreenPos.y + margin;
    const float maxX = viewportScreenPos.x + viewportSize.x - width - margin;
    const float maxY = viewportScreenPos.y + viewportSize.y - height - margin;
    origin.x = (maxX >= minX) ? std::clamp(origin.x, minX, maxX) : minX;
    origin.y = (maxY >= minY) ? std::clamp(origin.y, minY, maxY) : minY;

    const ImVec2 max(origin.x + width, origin.y + height);
    drawList->AddRectFilled(origin, max, IM_COL32(0, 0, 0, 160), 6.0f * uiScale);
    drawList->AddRect(origin, max, IM_COL32(255, 255, 255, 80), 6.0f * uiScale);

    float y = origin.y + paddingY;
    for (size_t i = 0; i < lines.size(); ++i) {
        ImU32 color = IM_COL32(255, 255, 255, 255);
        if (i == 3) {
            color = m_interactionLodActive ? IM_COL32(120, 255, 120, 255) : IM_COL32(255, 255, 255, 220);
        }
        drawList->AddText(ImVec2(origin.x + paddingX, y), color, lines[i].c_str());
        y += lineHeight + lineSpacing;
    }
}

void VtkViewer::drawDragSelectionOverlay() const {
    if (!m_dragSelectionActive) {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList == nullptr) {
        return;
    }

    const ImVec2 origin = ImGui::GetItemRectMin();
    const ImVec2 a(origin.x + m_dragStart.x, origin.y + m_dragStart.y);
    const ImVec2 b(origin.x + m_dragCurrent.x, origin.y + m_dragCurrent.y);
    drawList->AddRectFilled(a, b, IM_COL32(255, 216, 64, 36));
    drawList->AddRect(a, b, IM_COL32(255, 216, 64, 220), 0.0f, 0, 1.5f);
}

} // namespace core::vtk
