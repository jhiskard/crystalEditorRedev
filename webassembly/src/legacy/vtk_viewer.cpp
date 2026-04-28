#include "vtk_viewer.h"
#include "app.h"
#include "atoms/domain/cell_manager.h"
#include "font_manager.h"
#include "toolbar.h"
#include "mesh_manager.h"

// GLFW
#define GLFW_INCLUDE_ES3    // Include OpenGL ES 3.0 headers
#define GLFW_INCLUDE_GLEXT  // Include to OpenGL ES extension headers
#include <GLFW/glfw3.h>

// Emscripten
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

// Standard library
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

// VTK
#include <vtkOpenGLFramebufferObject.h>
#include <vtkProp.h>
#include <vtkPropCollection.h>
#include <vtkActor2D.h>
#include <vtkVolume.h>
// #include <vtkAxesActor.h>
// #include <vtkOrientationMarkerWidget.h>
#include <vtkCameraOrientationWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkCameraOrientationRepresentation.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include "atoms/atoms_template.h"

namespace {
constexpr int kInteractionSampleDistanceIndex = 5;
constexpr Mesh::VolumeQuality kInteractionVolumeQuality = Mesh::VolumeQuality::Low;
constexpr float kFpsSmoothingFactor = 0.15f;
constexpr auto kWheelInteractionLodHoldDuration = std::chrono::milliseconds(150);

bool isWheelInteractionLodHoldActive(const std::chrono::steady_clock::time_point& lastWheelTime,
    const std::chrono::steady_clock::time_point& now) {
    if (lastWheelTime == std::chrono::steady_clock::time_point{}) {
        return false;
    }
    return (now - lastWheelTime) <= kWheelInteractionLodHoldDuration;
}

void flipRgbaRows(std::vector<uint8_t>& pixels, int width, int height) {
    if (width <= 0 || height <= 1) {
        return;
    }
    const size_t rowBytes = static_cast<size_t>(width) * 4u;
    if (rowBytes == 0) {
        return;
    }
    std::vector<uint8_t> temp(rowBytes, 0);
    const int half = height / 2;
    for (int y = 0; y < half; ++y) {
        const size_t topOffset = static_cast<size_t>(y) * rowBytes;
        const size_t bottomOffset = static_cast<size_t>(height - 1 - y) * rowBytes;
        std::copy(pixels.begin() + topOffset, pixels.begin() + topOffset + rowBytes, temp.begin());
        std::copy(pixels.begin() + bottomOffset, pixels.begin() + bottomOffset + rowBytes, pixels.begin() + topOffset);
        std::copy(temp.begin(), temp.begin() + rowBytes, pixels.begin() + bottomOffset);
    }
}

double norm3(const double v[3]) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

bool normalize3(double v[3], double epsilon = 1e-8) {
    const double n = norm3(v);
    if (n <= epsilon) {
        return false;
    }
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
    return true;
}

void setVec3(double dst[3], const double src[3]) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

bool isFiniteBounds(const double bounds[6]) {
    return std::isfinite(bounds[0]) && std::isfinite(bounds[1]) &&
           std::isfinite(bounds[2]) && std::isfinite(bounds[3]) &&
           std::isfinite(bounds[4]) && std::isfinite(bounds[5]) &&
           bounds[1] >= bounds[0] && bounds[3] >= bounds[2] && bounds[5] >= bounds[4];
}
}


VtkViewer::VtkViewer()
    : m_Renderer(vtkSmartPointer<vtkRenderer>::New())
    , m_MeasurementOverlayRenderer(vtkSmartPointer<vtkRenderer>::New())
    , m_RenderWindow(vtkSmartPointer<vtkWebAssemblyOpenGLRenderWindow>::New())
    , m_Interactor(vtkSmartPointer<vtkWebAssemblyRenderWindowInteractor>::New())
    , m_InteractorStyle(vtkSmartPointer<MouseInteractorStyle>::New())
    // , m_AxesActor(vtkSmartPointer<vtkAxesActor>::New())
    // , m_OrientationMarkerWidget(vtkSmartPointer<vtkOrientationMarkerWidget>::New()) {
    , m_CameraOrientationWidget(vtkSmartPointer<vtkCameraOrientationWidget>::New()) {
    init();
}

VtkViewer::~VtkViewer() {
    if (m_Framebuffer != 0) {
        glDeleteFramebuffers(1, &m_Framebuffer);
        glDeleteTextures(1, &m_ColorTexture);
        // glDeleteTextures(1, &m_DepthTexture);
    }
}

void VtkViewer::init() {
    m_RenderWindow->SetNumberOfLayers(2);
    m_Renderer->SetLayer(0);

    m_MeasurementOverlayRenderer->SetLayer(1);
    m_MeasurementOverlayRenderer->InteractiveOff();
    m_MeasurementOverlayRenderer->SetPreserveColorBuffer(true);
    m_MeasurementOverlayRenderer->SetPreserveDepthBuffer(false);
    m_MeasurementOverlayRenderer->SetActiveCamera(m_Renderer->GetActiveCamera());

    m_RenderWindow->AddRenderer(m_Renderer);
    m_RenderWindow->AddRenderer(m_MeasurementOverlayRenderer);
    m_RenderWindow->SetMultiSamples(0);
    m_RenderWindow->SetSize(960, 640);
    m_RenderWindow->SwapBuffersOn();
    m_RenderWindow->SetOffScreenRendering(true);
	m_RenderWindow->SetFrameBlitModeToNoBlit();
    m_RenderWindow->SetInteractor(m_Interactor);
    m_Picker = vtkSmartPointer<vtkCellPicker>::New();
    m_Picker->SetTolerance(0.005);

    float bgColorTop[3] = { 1.0, 1.0, 1.0 };
    float bgColorBottom[3] = { 0.0, 0.0, 0.0 };
    // Get background color from local storage, if exists.
    // If not, set default color.
    loadBgColorFromLocalStorage(bgColorTop, bgColorBottom);
    
    m_Renderer->SetGradientBackground(true);
    m_Renderer->SetBackground2((double)bgColorTop[0], (double)bgColorTop[1], (double)bgColorTop[2]);  // Top color
    m_Renderer->SetBackground((double)bgColorBottom[0], (double)bgColorBottom[1], (double)bgColorBottom[2]);   // Bottom color
    m_Renderer->ResetCamera();

    m_Interactor->SetRenderWindow(m_RenderWindow);
    m_Interactor->SetInteractorStyle(m_InteractorStyle);
    m_Interactor->EnableRenderOff();

    m_InteractorStyle->SetDefaultRenderer(m_Renderer);
    setInteractionCallbacks();

    // m_AxesActor->SetShaftTypeToCylinder();

    // m_OrientationMarkerWidget->SetOrientationMarker(m_AxesActor);
    // m_OrientationMarkerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
    // m_OrientationMarkerWidget->SetDefaultRenderer(m_Renderer);
    // m_OrientationMarkerWidget->SetInteractor(m_Interactor);
    // m_OrientationMarkerWidget->EnabledOn();
    // m_OrientationMarkerWidget->InteractiveOff();

    m_CameraOrientationWidget->SetParentRenderer(m_Renderer);
    vtkCameraOrientationRepresentation* rep = 
        vtkCameraOrientationRepresentation::SafeDownCast(m_CameraOrientationWidget->GetRepresentation());
    rep->SetPadding(20, 20);
    rep->SetSize(140, 140);
    setCameraWidgetCallback();
    m_CameraOrientationWidget->On();

    // Initialize toolbar
    Toolbar& toolbar = Toolbar::Instance();
}

void VtkViewer::setCameraWidgetCallback() {
    // Camera aligned case
    // 1. The left/right axis rotates around the up/down axis.
    // 2. The up/down axis rotates around the left/right axis.
    // 3. The front axis rotates clockwise.

    // Camera not aligned case
    // 1. If the X axis is clicked, the X axis is in the screen direction and the Z axis is up.
    // 2. If the Y axis is clicked, the Y axis is in the screen direction and the X axis is up.
    // 3. If the Z axis is clicked, the Z axis is in the screen direction and the Y axis is up.

    // Get clicked position
    vtkNew<vtkCallbackCommand> clickCallback;
    clickCallback->SetClientData(this);  // Pass VtkViewer* to clickCallback as clientData
    clickCallback->SetCallback([](vtkObject* caller, unsigned long eventId, void* clientData, void* callData) {
        VtkViewer* viewer = static_cast<VtkViewer*>(clientData);  // Cast clientData to VtkViewer*
        viewer->setMousePosition(viewer->m_Interactor->GetEventPosition());
        SPDLOG_DEBUG("Mouse start position: {}, {}", viewer->getMousePosition().at(0), viewer->getMousePosition().at(1));
    });

    // Get release position and set camera callback
    vtkNew<vtkCallbackCommand> releaseCallback;
    releaseCallback->SetClientData(this);  // Pass VtkViewer* to releaseCallback as clientData
    releaseCallback->SetCallback([](vtkObject* caller, unsigned long eventId, void* clientData, void* callData) {
        VtkViewer* viewer = static_cast<VtkViewer*>(clientData);
        int32_t* newPosition = viewer->m_Interactor->GetEventPosition();
        int32_t movedDist = abs(newPosition[0] - viewer->getMousePosition().at(0)) +
            abs(newPosition[1] - viewer->getMousePosition().at(1));  // Calculate the moved dx + dy
        
        // If the mouse moved more than 4 pixels, ignore the event
        if (movedDist > 4) {
            return;
        }
        
        vtkRenderer* renderer = viewer->m_Renderer;
        
        vtkCameraOrientationWidget* widget = static_cast<vtkCameraOrientationWidget*>(caller);
        vtkCameraOrientationRepresentation* rep = 
            vtkCameraOrientationRepresentation::SafeDownCast(widget->GetRepresentation());

        int pickedAxis = rep->GetPickedAxis();  // 0: X, 1: Y, 2: Z
        int pickedDir = rep->GetPickedDir();    // 0: +, 1: -
        SPDLOG_DEBUG("Picked axis: {}, picked dir: {}", pickedAxis, pickedDir);

        if (!rep->IsAnyHandleSelected() || pickedAxis < 0 || pickedDir < 0) {
            // If no handle is selected, return
            return;
        }

        switch (viewer->getCameraDirection()) {
        case CameraDirection::XPLUS_YPLUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YPLUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XPLUS_ZPLUS);
            }
            break;
        case CameraDirection::XPLUS_YMINUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YMINUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XPLUS_ZMINUS);
            }
            break;
        case CameraDirection::XPLUS_ZPLUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XPLUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XPLUS_YMINUS);
            }
            break;
        case CameraDirection::XPLUS_ZMINUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XMINUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XPLUS_YPLUS);
            }
            break;
        case CameraDirection::XMINUS_YPLUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YPLUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XMINUS_ZMINUS);
            }
            break;
        case CameraDirection::XMINUS_YMINUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YMINUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XMINUS_ZPLUS);
            }
            break;
        case CameraDirection::XMINUS_ZPLUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XMINUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XMINUS_YPLUS);
            }
            break;
        case CameraDirection::XMINUS_ZMINUS:
            if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XPLUS);
                }
            }
            else {
                // +X axis
                viewer->setCameraDirection(CameraDirection::XMINUS_YMINUS);
            }
            break;
        case CameraDirection::YPLUS_XPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XPLUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YPLUS_ZMINUS);
            }
            break;
        case CameraDirection::YPLUS_XMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XMINUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YPLUS_ZPLUS);
            }
            break;
        case CameraDirection::YPLUS_ZPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YPLUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YPLUS_XPLUS);
            }
            break;
        case CameraDirection::YPLUS_ZMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YMINUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YPLUS_XMINUS);
            }
            break;
        case CameraDirection::YMINUS_XPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XPLUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YMINUS_ZPLUS);
            }
            break;
        case CameraDirection::YMINUS_XMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_XMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_XMINUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YMINUS_ZMINUS);
            }
            break;
        case CameraDirection::YMINUS_ZPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YMINUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YMINUS_XMINUS);
            }
            break;
        case CameraDirection::YMINUS_ZMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZMINUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YMINUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YPLUS);
                }
            }
            else {
                // +Y axis
                viewer->setCameraDirection(CameraDirection::YMINUS_XPLUS);
            }
            break;
        case CameraDirection::ZPLUS_XPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZPLUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XPLUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZPLUS_YPLUS);
            }
            break;
        case CameraDirection::ZPLUS_XMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZMINUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XMINUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZPLUS_YMINUS);
            }
            break;
        case CameraDirection::ZPLUS_YPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YPLUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZPLUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZPLUS_XMINUS);
            }
            break;
        case CameraDirection::ZPLUS_YMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YMINUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZMINUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZPLUS_XPLUS);
            }
            break;
        case CameraDirection::ZMINUS_XPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZMINUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XPLUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZMINUS_YMINUS);
            }
            break;
        case CameraDirection::ZMINUS_XMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZPLUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XMINUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZMINUS_YMINUS);
            }
            break;
        case CameraDirection::ZMINUS_YPLUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YPLUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZMINUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZMINUS_XPLUS);
            }
            break;
        case CameraDirection::ZMINUS_YMINUS:
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_YMINUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_YMINUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_ZMINUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_ZPLUS);
                }
            }
            else {
                // +Z axis
                viewer->setCameraDirection(CameraDirection::ZMINUS_XMINUS);
            }
            break;
        case CameraDirection::NOT_ALIGNED: 
            // If the camera is not aligned, set the camera direction based on the picked axis and direction
            if (pickedAxis == 0) {
                if (pickedDir == 0) {
                    // +X axis
                    viewer->setCameraDirection(CameraDirection::XPLUS_ZPLUS);
                }
                else {
                    // -X axis
                    viewer->setCameraDirection(CameraDirection::XMINUS_ZPLUS);
                }
            }
            else if (pickedAxis == 1) {
                if (pickedDir == 0) {
                    // +Y axis
                    viewer->setCameraDirection(CameraDirection::YPLUS_XPLUS);
                }
                else {
                    // -Y axis
                    viewer->setCameraDirection(CameraDirection::YMINUS_XPLUS);
                }
            }
            else if (pickedAxis == 2) {
                if (pickedDir == 0) {
                    // +Z axis
                    viewer->setCameraDirection(CameraDirection::ZPLUS_YPLUS);
                }
                else {
                    // -Z axis
                    viewer->setCameraDirection(CameraDirection::ZMINUS_YPLUS);
                }
            }
            break;
        }

        // Rotate the camera based on the picked axis and direction
        vtkCamera* camera = renderer->GetActiveCamera();

        // Focal point: The point the camera is looking at
        double focalPoint[3];
        camera->GetFocalPoint(focalPoint);
        
        // Get the current camera position
        double cameraPos[3];
        camera->GetPosition(cameraPos);
        
        // Distance between camera and focal point
        double dist = sqrt(vtkMath::Distance2BetweenPoints(cameraPos, focalPoint));
        
        // Calculate new camera position and up vector
        double newPos[3] = { focalPoint[0], focalPoint[1], focalPoint[2] };
        double viewUp[3] = { 0.0, 0.0, 0.0 };

        switch(viewer->getCameraDirection()) {
        case CameraDirection::XPLUS_YPLUS:
            newPos[0] = focalPoint[0] + dist;            
            viewUp[1] = 1.0; // Y axis is up
            break;
        case CameraDirection::XPLUS_YMINUS:
            newPos[0] = focalPoint[0] + dist;            
            viewUp[1] = -1.0; // Y axis is up
            break;
        case CameraDirection::XPLUS_ZPLUS:
            newPos[0] = focalPoint[0] + dist;            
            viewUp[2] = 1.0; // Z axis is up
            break;
        case CameraDirection::XPLUS_ZMINUS:
            newPos[0] = focalPoint[0] + dist;            
            viewUp[2] = -1.0; // Z axis is up
            break;
        case CameraDirection::XMINUS_YPLUS:
            newPos[0] = focalPoint[0] - dist;            
            viewUp[1] = 1.0; // Y axis is up
            break;
        case CameraDirection::XMINUS_YMINUS:
            newPos[0] = focalPoint[0] - dist;            
            viewUp[1] = -1.0; // Y axis is up
            break;
        case CameraDirection::XMINUS_ZPLUS:
            newPos[0] = focalPoint[0] - dist;            
            viewUp[2] = 1.0; // Z axis is up
            break;
        case CameraDirection::XMINUS_ZMINUS:
            newPos[0] = focalPoint[0] - dist;            
            viewUp[2] = -1.0; // Z axis is up
            break;
        case CameraDirection::YPLUS_XPLUS:
            newPos[1] = focalPoint[1] + dist;            
            viewUp[0] = 1.0; // X axis is up
            break;
        case CameraDirection::YPLUS_XMINUS:
            newPos[1] = focalPoint[1] + dist;            
            viewUp[0] = -1.0; // X axis is up
            break;
        case CameraDirection::YPLUS_ZPLUS:
            newPos[1] = focalPoint[1] + dist;            
            viewUp[2] = 1.0; // Z axis is up
            break;
        case CameraDirection::YPLUS_ZMINUS:
            newPos[1] = focalPoint[1] + dist;            
            viewUp[2] = -1.0; // Z axis is up
            break;
        case CameraDirection::YMINUS_XPLUS:
            newPos[1] = focalPoint[1] - dist;            
            viewUp[0] = 1.0; // X axis is up
            break;
        case CameraDirection::YMINUS_XMINUS:
            newPos[1] = focalPoint[1] - dist;            
            viewUp[0] = -1.0; // X axis is up
            break;
        case CameraDirection::YMINUS_ZPLUS:
            newPos[1] = focalPoint[1] - dist;            
            viewUp[2] = 1.0; // Z axis is up
            break;
        case CameraDirection::YMINUS_ZMINUS:
            newPos[1] = focalPoint[1] - dist;            
            viewUp[2] = -1.0; // Z axis is up
            break;
        case CameraDirection::ZPLUS_XPLUS:
            newPos[2] = focalPoint[2] + dist;            
            viewUp[0] = 1.0; // X axis is up
            break;
        case CameraDirection::ZPLUS_XMINUS:
            newPos[2] = focalPoint[2] + dist;            
            viewUp[0] = -1.0; // X axis is up
            break;
        case CameraDirection::ZPLUS_YPLUS:
            newPos[2] = focalPoint[2] + dist;            
            viewUp[1] = 1.0; // Y axis is up
            break;
        case CameraDirection::ZPLUS_YMINUS:
            newPos[2] = focalPoint[2] + dist;            
            viewUp[1] = -1.0; // Y axis is up
            break;
        case CameraDirection::ZMINUS_XPLUS:
            newPos[2] = focalPoint[2] - dist;            
            viewUp[0] = 1.0; // X axis is up
            break;
        case CameraDirection::ZMINUS_XMINUS:
            newPos[2] = focalPoint[2] - dist;            
            viewUp[0] = -1.0; // X axis is up
            break;
        case CameraDirection::ZMINUS_YPLUS:
            newPos[2] = focalPoint[2] - dist;            
            viewUp[1] = 1.0; // Y axis is up
            break;
        case CameraDirection::ZMINUS_YMINUS:
            newPos[2] = focalPoint[2] - dist;            
            viewUp[1] = -1.0; // Y axis is up
            break;
        case CameraDirection::NOT_ALIGNED:
            assert(false && "Camera direction is not set.");
            break;
        default:
            assert(false && "Wront camera direction");
            break;
        }

        // Set the new camera position and up vector
        camera->SetPosition(newPos);
        camera->SetViewUp(viewUp);
        
        // Update the camera settings
        camera->OrthogonalizeViewUp();
        renderer->ResetCameraClippingRange();
    });

    // Add observers to the camera orientation widget
    m_CameraOrientationWidget->AddObserver(vtkCommand::StartInteractionEvent, clickCallback);
    m_CameraOrientationWidget->AddObserver(vtkCommand::EndInteractionEvent, releaseCallback);
}

void VtkViewer::setInteractionCallbacks() {
    vtkNew<vtkCallbackCommand> startCallback;
    startCallback->SetClientData(this);
    startCallback->SetCallback([](vtkObject*, unsigned long, void* clientData, void*) {
        VtkViewer* viewer = static_cast<VtkViewer*>(clientData);
        if (viewer) {
            viewer->BeginInteractionLod();
        }
    });

    vtkNew<vtkCallbackCommand> endCallback;
    endCallback->SetClientData(this);
    endCallback->SetCallback([](vtkObject*, unsigned long, void* clientData, void*) {
        VtkViewer* viewer = static_cast<VtkViewer*>(clientData);
        if (viewer) {
            viewer->EndInteractionLod();
        }
    });

    m_InteractorStyle->AddObserver(vtkCommand::StartInteractionEvent, startCallback);
    m_InteractorStyle->AddObserver(vtkCommand::EndInteractionEvent, endCallback);
}

void VtkViewer::setMousePosition(const int32_t* pos) {
    if (pos == nullptr) {
        return;
    }

    m_MousePosition[0] = pos[0];
    m_MousePosition[1] = pos[1];
}

const std::array<int32_t, 2>& VtkViewer::getMousePosition() const {
    return m_MousePosition;
}

void VtkViewer::initFramebuffer() {
    if (m_Framebuffer != 0) {
        glDeleteFramebuffers(1, &m_Framebuffer);
        glDeleteTextures(1, &m_ColorTexture);
        // glDeleteTextures(1, &m_DepthTexture);
    }

    // Create framebuffer
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    // Create color texture
    glGenTextures(1, &m_ColorTexture);
    glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);

    // Create depth texture
    // glGenTextures(1, &m_DepthTexture);
    // glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

    // Check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPDLOG_ERROR("Scene framebuffer is not complete!");
    }

    // Note: The following "SetSize()" functions changes the size of canvas internally.
    // The canvas size and the css size should be recovered after calling "SetSize()" functions.

    // Get the actual canvas size
    int canvasWidth, canvasHeight;
    emscripten_get_canvas_element_size("canvas", &canvasWidth, &canvasHeight);

    // Get canvas css size
    double css_width, css_height;
    emscripten_get_element_css_size("canvas", &css_width, &css_height);

    m_RenderWindow->InitializeFromCurrentContext();
	m_RenderWindow->SetSize(m_Width, m_Height);
	m_Interactor->SetSize(m_Width, m_Height);

    vtkOpenGLFramebufferObject* framebufferObj = m_RenderWindow->GetDisplayFramebuffer();
	framebufferObj->Bind();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);
	framebufferObj->UnBind();

    // Bind to default buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    SPDLOG_DEBUG("Framebuffer initialized: ({} x {})", m_Width, m_Height);

    glfwSetWindowSize(glfwGetCurrentContext(), canvasWidth, canvasHeight);  // Set canvas size
    emscripten_set_element_css_size("canvas", css_width, css_height);       // Set css size
}

void VtkViewer::resizeFramebuffer(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0)
        return;

    m_Width = width;
    m_Height = height;

    initFramebuffer();
    RequestRender();
}

void VtkViewer::clearFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, m_Width, m_Height);

    // Render background
    // glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Bind to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VtkViewer::resetDragSelection() {
    m_DragSelection = DragSelectionState {};
}

void VtkViewer::renderDragSelectionOverlay(const ImVec2& windowPos, const ImVec2& viewportPos) const {
    if (!m_DragSelection.active) {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList) {
        return;
    }

    const float minX = std::min(m_DragSelection.startPos.x, m_DragSelection.currentPos.x);
    const float maxX = std::max(m_DragSelection.startPos.x, m_DragSelection.currentPos.x);
    const float minY = std::min(m_DragSelection.startPos.y, m_DragSelection.currentPos.y);
    const float maxY = std::max(m_DragSelection.startPos.y, m_DragSelection.currentPos.y);
    if (maxX <= minX || maxY <= minY) {
        return;
    }

    const ImVec2 boxMin(windowPos.x + viewportPos.x + minX, windowPos.y + viewportPos.y + minY);
    const ImVec2 boxMax(windowPos.x + viewportPos.x + maxX, windowPos.y + viewportPos.y + maxY);
    drawList->AddRectFilled(boxMin, boxMax, IM_COL32(90, 170, 255, 45));
    drawList->AddRect(boxMin, boxMax, IM_COL32(90, 170, 255, 220), 0.0f, 0, 1.8f);
}

void VtkViewer::processEvents() {
    ImGuiIO& io = ImGui::GetIO();
    const auto now = std::chrono::steady_clock::now();
    const bool anyMouseDown = io.MouseDown[ImGuiMouseButton_Left] ||
        io.MouseDown[ImGuiMouseButton_Right] ||
        io.MouseDown[ImGuiMouseButton_Middle];
    const bool hasWheelInput = io.MouseWheel != 0.0f;
    if (hasWheelInput) {
        m_LastWheelInteractionTime = now;
    }
    const bool wheelLodHoldActive = isWheelInteractionLodHoldActive(m_LastWheelInteractionTime, now);
    const bool shouldKeepInteractionLod = anyMouseDown || wheelLodHoldActive;
    
    static bool isDraggingWindow = false;
    static ImVec2 dragStartPos;
    
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 viewportPos = ImGui::GetCursorStartPos();
    auto toViewerLocalClamped = [&](const ImVec2& mousePos) -> ImVec2 {
        const float relX = mousePos.x - windowPos.x - viewportPos.x;
        const float relY = mousePos.y - windowPos.y - viewportPos.y;
        const float maxX = static_cast<float>(std::max(0, m_Width));
        const float maxY = static_cast<float>(std::max(0, m_Height));
        return ImVec2(
            std::clamp(relX, 0.0f, maxX),
            std::clamp(relY, 0.0f, maxY));
    };
    
    // Dragging check
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        dragStartPos = io.MousePos;
        
        if (io.MousePos.y >= windowPos.y && io.MousePos.y <= windowPos.y + App::TitleBarHeight() &&
            io.MousePos.x >= windowPos.x && io.MousePos.x <= windowPos.x + ImGui::GetWindowWidth()) {
            isDraggingWindow = true;
        }
    }
    
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        isDraggingWindow = false;
    }
    
    if (isDraggingWindow) {
        if (io.MouseReleased[ImGuiMouseButton_Left] && m_DragSelection.tracking) {
            resetDragSelection();
            RequestRender();
        }
        if (m_InteractionLodActive && !shouldKeepInteractionLod) {
            EndInteractionLod();
            RequestRender();
        }
        return;
    }
    
    if (!ImGui::IsWindowFocused() && !ImGui::IsWindowHovered()) {
        if (io.MouseReleased[ImGuiMouseButton_Left] && m_DragSelection.tracking) {
            resetDragSelection();
            RequestRender();
        }
        if (m_InteractionLodActive && !shouldKeepInteractionLod) {
            EndInteractionLod();
            RequestRender();
        }
        return;
    }

    io.ConfigWindowsMoveFromTitleBarOnly = true;

    int xPos = static_cast<int>(io.MousePos.x - windowPos.x - viewportPos.x);
    int yPos = static_cast<int>(io.MousePos.y - windowPos.y - viewportPos.y);
    int ctrl = static_cast<int>(io.KeyCtrl);
    int shift = static_cast<int>(io.KeyShift);
    bool dclick = io.MouseDoubleClicked[0] || io.MouseDoubleClicked[1] || io.MouseDoubleClicked[2];

    m_Interactor->SetEventInformationFlipY(xPos, yPos, ctrl, shift, dclick);

    static int32_t clickXPos = 0;
    static int32_t clickYPos = 0;
    static bool hasLastPos = false;
    static int32_t lastXPos = 0;
    static int32_t lastYPos = 0;
    static bool leftPressForwardedToInteractor = false;
    static bool pendingMeasurementPick = false;
    bool requestRender = false;
    AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
    const bool measurementModeActive = atomsTemplate.IsMeasurementModeActive();
    const bool measurementDragSelectionEnabled =
        measurementModeActive && atomsTemplate.IsMeasurementDragSelectionEnabled();
    const bool selectionModifierDown = io.KeySuper || io.KeyCtrl;
    const bool canStartModifierDragSelection =
        selectionModifierDown && (!measurementModeActive || measurementDragSelectionEnabled);
    const bool windowHovered = ImGui::IsWindowHovered();

    if (windowHovered) {
        float relX = io.MousePos.x - windowPos.x - viewportPos.x;
        float relY = io.MousePos.y - windowPos.y - viewportPos.y;
        
        int pickX = static_cast<int>(relX);
        int pickY = static_cast<int>(m_Height - relY);
        const ImVec2 clampedLocalPos = toViewerLocalClamped(io.MousePos);

        if ((io.MouseClicked[ImGuiMouseButton_Left] ||
            io.MouseClicked[ImGuiMouseButton_Right] ||
            io.MouseClicked[ImGuiMouseButton_Middle]) &&
            !m_InteractionLodActive) {
            BeginInteractionLod();
        }
        
        if (io.MouseClicked[ImGuiMouseButton_Left]) {
            clickXPos = io.MousePos.x;
            clickYPos = io.MousePos.y;

            leftPressForwardedToInteractor = false;
            pendingMeasurementPick = false;
            const bool sceneInputBlockedByUi = io.WantCaptureMouse && ImGui::IsAnyItemHovered();
            if (sceneInputBlockedByUi) {
                requestRender = true;
                if (m_DragSelection.tracking) {
                    resetDragSelection();
                }
            } else if (canStartModifierDragSelection) {
                m_DragSelection.tracking = true;
                m_DragSelection.active = false;
                m_DragSelection.measurementModeAtStart = measurementModeActive;
                m_DragSelection.sameElementSelectAtStart = io.MouseDoubleClicked[ImGuiMouseButton_Left];
                m_DragSelection.startPos = clampedLocalPos;
                m_DragSelection.currentPos = clampedLocalPos;
                requestRender = true;
            } else if (measurementModeActive) {
                m_Interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
                leftPressForwardedToInteractor = true;
                pendingMeasurementPick = true;
                requestRender = true;
            } else {
                m_Interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
                leftPressForwardedToInteractor = true;
                requestRender = true;
            }
        }
        else if (io.MouseClicked[ImGuiMouseButton_Right]) {
            m_Interactor->InvokeEvent(vtkCommand::RightButtonPressEvent, nullptr);
            ImGui::SetWindowFocus();
            requestRender = true;
        }
        else if (io.MouseClicked[ImGuiMouseButton_Middle]) {
            m_Interactor->InvokeEvent(vtkCommand::MiddleButtonPressEvent, nullptr);
            requestRender = true;
        }
        else if (io.MouseWheel > 0) {
            if (!m_InteractionLodActive) {
                BeginInteractionLod();
            }
            m_LastWheelInteractionTime = now;
            m_Interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent, nullptr);
            requestRender = true;
        }
        else if (io.MouseWheel < 0) {
            if (!m_InteractionLodActive) {
                BeginInteractionLod();
            }
            m_LastWheelInteractionTime = now;
            m_Interactor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, nullptr);
            requestRender = true;
        }
    }

    if (m_DragSelection.tracking && io.MouseDown[ImGuiMouseButton_Left]) {
        m_DragSelection.currentPos = toViewerLocalClamped(io.MousePos);
        const float movedDist =
            std::fabs(m_DragSelection.currentPos.x - m_DragSelection.startPos.x) +
            std::fabs(m_DragSelection.currentPos.y - m_DragSelection.startPos.y);
        if (!m_DragSelection.active && movedDist > 4.0f) {
            m_DragSelection.active = true;
        }
        if (m_DragSelection.active) {
            requestRender = true;
        }
    }

    bool releasedAnyButton = false;
    if (io.MouseReleased[ImGuiMouseButton_Left]) {
        int32_t movedDist = abs(static_cast<int32_t>(io.MousePos.x) - clickXPos) +
            abs(static_cast<int32_t>(io.MousePos.y) - clickYPos);
        if (m_DragSelection.tracking) {
            m_DragSelection.currentPos = toViewerLocalClamped(io.MousePos);
            if (m_DragSelection.active) {
                const int dragX0 = static_cast<int>(std::lround(m_DragSelection.startPos.x));
                const int dragY0 = static_cast<int>(std::lround(m_DragSelection.startPos.y));
                const int dragX1 = static_cast<int>(std::lround(m_DragSelection.currentPos.x));
                const int dragY1 = static_cast<int>(std::lround(m_DragSelection.currentPos.y));
                atomsTemplate.HandleDragSelectionInScreenRect(
                    dragX0,
                    dragY0,
                    dragX1,
                    dragY1,
                    m_Renderer,
                    m_Height,
                    true);
            } else {
                float relX = io.MousePos.x - windowPos.x - viewportPos.x;
                float relY = io.MousePos.y - windowPos.y - viewportPos.y;
                int pickX = static_cast<int>(relX);
                int pickY = static_cast<int>(m_Height - relY);
                if (m_DragSelection.measurementModeAtStart) {
                    if (m_Picker->Pick(pickX, pickY, 0, m_Renderer)) {
                        vtkActor* pickedActor = m_Picker->GetActor();
                        double* pickPos = m_Picker->GetPickPosition();
                        atomsTemplate.HandleMeasurementClickByPicker(pickedActor, pickPos);
                    } else {
                        atomsTemplate.HandleMeasurementEmptyClick();
                    }
                } else {
                    if (m_Picker->Pick(pickX, pickY, 0, m_Renderer)) {
                        vtkActor* pickedActor = m_Picker->GetActor();
                        double* pickPos = m_Picker->GetPickPosition();
                        if (m_DragSelection.sameElementSelectAtStart) {
                            atomsTemplate.SelectSameElementAtomsByPicker(pickedActor, pickPos);
                        } else {
                            atomsTemplate.SelectAtomByPicker(pickedActor, pickPos);
                        }
                    } else {
                        atomsTemplate.ClearCreatedAtomSelection();
                    }
                }
            }
            resetDragSelection();
            pendingMeasurementPick = false;
            leftPressForwardedToInteractor = false;
            requestRender = true;
            releasedAnyButton = true;
        } else if (leftPressForwardedToInteractor) {
            if (movedDist > 4) {
                setCameraDirection(CameraDirection::NOT_ALIGNED);
            }
            m_Interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
            requestRender = true;
            releasedAnyButton = true;

            if (!measurementModeActive && movedDist <= 4) {
                float relX = io.MousePos.x - windowPos.x - viewportPos.x;
                float relY = io.MousePos.y - windowPos.y - viewportPos.y;
                int pickX = static_cast<int>(relX);
                int pickY = static_cast<int>(m_Height - relY);
                if (!m_Picker->Pick(pickX, pickY, 0, m_Renderer)) {
                    atomsTemplate.ClearCreatedAtomSelection();
                }
            }
        }

        if (pendingMeasurementPick && movedDist <= 4) {
            float relX = io.MousePos.x - windowPos.x - viewportPos.x;
            float relY = io.MousePos.y - windowPos.y - viewportPos.y;
            int pickX = static_cast<int>(relX);
            int pickY = static_cast<int>(m_Height - relY);
            if (m_Picker->Pick(pickX, pickY, 0, m_Renderer)) {
                vtkActor* pickedActor = m_Picker->GetActor();
                double* pickPos = m_Picker->GetPickPosition();
                atomsTemplate.HandleMeasurementClickByPicker(pickedActor, pickPos);
            } else {
                atomsTemplate.HandleMeasurementEmptyClick();
            }
            requestRender = true;
        }

        if (!m_DragSelection.tracking) {
            pendingMeasurementPick = false;
            leftPressForwardedToInteractor = false;
        }
    }
    if (io.MouseReleased[ImGuiMouseButton_Right]) {
        m_Interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent, nullptr);
        requestRender = true;
        releasedAnyButton = true;
    }
    if (io.MouseReleased[ImGuiMouseButton_Middle]) {
        m_Interactor->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, nullptr);
        requestRender = true;
        releasedAnyButton = true;
    }

    bool isDragging = (io.MouseDown[ImGuiMouseButton_Left] && leftPressForwardedToInteractor) ||
        (io.MouseDown[ImGuiMouseButton_Left] && m_DragSelection.tracking) ||
        io.MouseDown[ImGuiMouseButton_Right] ||
        io.MouseDown[ImGuiMouseButton_Middle];
    if (releasedAnyButton && !isDragging && m_InteractionLodActive && !wheelLodHoldActive) {
        EndInteractionLod();
        requestRender = true;
    }
    bool moved = !hasLastPos || lastXPos != xPos || lastYPos != yPos;
    if ((moved || isDragging) && !m_DragSelection.tracking) {
        m_Interactor->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
        requestRender = true;
    }
    lastXPos = xPos;
    lastYPos = yPos;
    hasLastPos = true;

    if (!isDragging && !wheelLodHoldActive && m_InteractionLodActive) {
        EndInteractionLod();
        requestRender = true;
    }

    const bool isTypingInTextField = io.WantTextInput || ImGui::IsAnyItemActive();
    bool rotatedByArrowKey = false;
    if (!isTypingInTextField) {
        const float stepDeg = GetArrowRotateStepDeg();
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
            rotatedByArrowKey = RotateCameraByKeyboard(0.0f, stepDeg) || rotatedByArrowKey;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
            rotatedByArrowKey = RotateCameraByKeyboard(0.0f, -stepDeg) || rotatedByArrowKey;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
            rotatedByArrowKey = RotateCameraByKeyboard(-stepDeg, 0.0f) || rotatedByArrowKey;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
            rotatedByArrowKey = RotateCameraByKeyboard(stepDeg, 0.0f) || rotatedByArrowKey;
        }
    }
    if (rotatedByArrowKey) {
        requestRender = true;
    }

    if (requestRender) {
        RequestRender();
    }
}

bool VtkViewer::RotateCameraByKeyboard(float azimuthDeg, float elevationDeg) {
    if (!m_Renderer) {
        return false;
    }

    vtkCamera* camera = m_Renderer->GetActiveCamera();
    if (!camera) {
        return false;
    }

    constexpr float kEpsilon = 1e-6f;
    const bool hasAzimuth = std::abs(azimuthDeg) > kEpsilon;
    const bool hasElevation = std::abs(elevationDeg) > kEpsilon;
    if (!hasAzimuth && !hasElevation) {
        return false;
    }

    if (hasAzimuth) {
        camera->Azimuth(static_cast<double>(azimuthDeg));
    }
    if (hasElevation) {
        camera->Elevation(static_cast<double>(elevationDeg));
    }

    camera->OrthogonalizeViewUp();
    m_Renderer->ResetCameraClippingRange();
    setCameraDirection(CameraDirection::NOT_ALIGNED);
    return true;
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

void VtkViewer::computeDurationStats(const std::array<float, 120>& history,
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

void VtkViewer::updatePerformanceStats(const std::chrono::steady_clock::time_point& now,
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

    if (m_LastUiFrameTime != std::chrono::steady_clock::time_point{}) {
        const float deltaSec = std::chrono::duration<float>(now - m_LastUiFrameTime).count();
        if (deltaSec > 0.0f) {
            m_UiFps = smoothFps(m_UiFps, 1.0f / deltaSec);
        }
    }
    m_LastUiFrameTime = now;

    if (!didRender) {
        return;
    }

    m_LastRenderMs = renderMs;
    pushDurationSample(m_RenderMsHistory, m_RenderMsHistoryHead, m_RenderMsHistoryCount, renderMs);

    if (m_LastRenderTime != std::chrono::steady_clock::time_point{}) {
        const float deltaSec = std::chrono::duration<float>(now - m_LastRenderTime).count();
        if (deltaSec > 0.0f) {
            m_RenderFps = smoothFps(m_RenderFps, 1.0f / deltaSec);
        }
    }
    m_LastRenderTime = now;

    if (!interactionRender) {
        return;
    }

    pushDurationSample(m_InteractionRenderMsHistory,
        m_InteractionRenderMsHistoryHead,
        m_InteractionRenderMsHistoryCount,
        renderMs);

    if (m_LastInteractionRenderTime != std::chrono::steady_clock::time_point{}) {
        const float deltaSec = std::chrono::duration<float>(now - m_LastInteractionRenderTime).count();
        if (deltaSec > 0.0f) {
            m_InteractionRenderFps = smoothFps(m_InteractionRenderFps, 1.0f / deltaSec);
        }
    }
    m_LastInteractionRenderTime = now;
}

void VtkViewer::renderPerformanceOverlay(const ImVec2& windowPos, const ImVec2& viewportPos) const {
    float renderAvg = 0.0f;
    float renderP95 = 0.0f;
    float renderMax = 0.0f;
    computeDurationStats(m_RenderMsHistory, m_RenderMsHistoryCount, renderAvg, renderP95, renderMax);

    float interactionAvg = 0.0f;
    float interactionP95 = 0.0f;
    float interactionMax = 0.0f;
    computeDurationStats(m_InteractionRenderMsHistory,
        m_InteractionRenderMsHistoryCount,
        interactionAvg,
        interactionP95,
        interactionMax);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList) {
        return;
    }

    char line[256];
    std::array<std::string, 6> lines;
    lines[0] = "Viewer FPS Metrics";
    std::snprintf(line, sizeof(line), "UI FPS: %.1f | VTK FPS: %.1f", m_UiFps, m_RenderFps);
    lines[1] = line;
    std::snprintf(line, sizeof(line),
        "Render ms (last/avg/p95/max): %.2f / %.2f / %.2f / %.2f",
        m_LastRenderMs, renderAvg, renderP95, renderMax);
    lines[2] = line;
    std::snprintf(line, sizeof(line), "Interaction LOD: %s", m_InteractionLodActive ? "ON" : "OFF");
    lines[3] = line;
    std::snprintf(line, sizeof(line), "Interaction FPS: %.1f", m_InteractionRenderFps);
    lines[4] = line;
    std::snprintf(line, sizeof(line),
        "Interaction ms (avg/p95/max): %.2f / %.2f / %.2f",
        interactionAvg, interactionP95, interactionMax);
    lines[5] = line;

    float maxTextWidth = 0.0f;
    for (const std::string& textLine : lines) {
        maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(textLine.c_str()).x);
    }

    const float uiScale = std::max(1.0f, App::UiScale());
    const float lineHeight = ImGui::GetTextLineHeight();
    const float lineSpacing = 4.0f * uiScale;
    const float paddingX = 8.0f * uiScale;
    const float paddingY = 8.0f * uiScale;
    const float margin = 4.0f * uiScale;
    const float lineCount = static_cast<float>(lines.size());
    const float width = maxTextWidth + paddingX * 2.0f;
    const float height = paddingY * 2.0f + lineCount * lineHeight + (lineCount - 1.0f) * lineSpacing;
    ImVec2 origin(windowPos.x + viewportPos.x + 12.0f * uiScale,
                  windowPos.y + viewportPos.y + 12.0f * uiScale);

    const float minX = windowPos.x + viewportPos.x + margin;
    const float minY = windowPos.y + viewportPos.y + margin;
    const float maxX = windowPos.x + viewportPos.x + static_cast<float>(m_Width) - width - margin;
    const float maxY = windowPos.y + viewportPos.y + static_cast<float>(m_Height) - height - margin;
    if (maxX >= minX) {
        origin.x = std::clamp(origin.x, minX, maxX);
    } else {
        origin.x = minX;
    }
    if (maxY >= minY) {
        origin.y = std::clamp(origin.y, minY, maxY);
    } else {
        origin.y = minY;
    }

    drawList->AddRectFilled(
        origin,
        ImVec2(origin.x + width, origin.y + height),
        IM_COL32(0, 0, 0, 160),
        6.0f * uiScale);
    drawList->AddRect(
        origin,
        ImVec2(origin.x + width, origin.y + height),
        IM_COL32(255, 255, 255, 80),
        6.0f * uiScale);

    float y = origin.y + paddingY;
    for (size_t i = 0; i < lines.size(); ++i) {
        ImU32 color = IM_COL32(255, 255, 255, 255);
        if (i == 3) {
            color = m_InteractionLodActive ? IM_COL32(120, 255, 120, 255) : IM_COL32(255, 255, 255, 220);
        }
        drawList->AddText(ImVec2(origin.x + paddingX, y), color, lines[i].c_str());
        y += lineHeight + lineSpacing;
    }
}

void VtkViewer::Render(bool* openWindow) {
    const auto now = std::chrono::steady_clock::now();
    const bool interactionRender = m_InteractionLodActive;
    bool didRender = false;
    float renderMs = 0.0f;
    if (!m_RenderPaused && (m_RenderDirty || interactionRender)) {
        const auto renderStart = std::chrono::steady_clock::now();
        m_RenderWindow->Render();
        m_RenderWindow->WaitForCompletion();
        const auto renderEnd = std::chrono::steady_clock::now();
        renderMs = std::chrono::duration<float, std::milli>(renderEnd - renderStart).count();
        didRender = true;
        m_RenderDirty = false;
    }
    updatePerformanceStats(now, didRender, renderMs, interactionRender);
    
    // 강제 레이아웃 요청이 있으면 도킹을 해제하고 해당 값을 우선 적용한다.
    if (m_HasForcedWindowLayout && m_ForcedWindowLayoutFramesRemaining > 0) {
        ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
        ImGui::SetNextWindowPos(m_ForcedWindowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_ForcedWindowSize, ImGuiCond_Always);
        m_FirstRender = false;
        m_RequestInitialLayout = false;
        --m_ForcedWindowLayoutFramesRemaining;
        if (m_ForcedWindowLayoutFramesRemaining <= 0) {
            m_HasForcedWindowLayout = false;
        }
    }
    // 초기 창 설정 (첫 렌더링 시 또는 레이아웃 요청 시)
    else if (m_FirstRender || m_RequestInitialLayout) {
        ImGuiCond cond = m_RequestInitialLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
        m_FirstRender = false;
        m_RequestInitialLayout = false;
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, cond, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(m_PreferredSize, cond);
    }
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(ICON_FA6_DISPLAY"  Viewer", openWindow, ImGuiWindowFlags_NoScrollbar);
    
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 viewportPos = ImGui::GetCursorStartPos();
    
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    if (windowSize.x > 0 && windowSize.y > 0 && 
        (m_Width != static_cast<int32_t>(windowSize.x) || m_Height != static_cast<int32_t>(windowSize.y))) {
        resizeFramebuffer(static_cast<int32_t>(windowSize.x), static_cast<int32_t>(windowSize.y));
    }
    
    ImGui::Image((ImTextureID)(intptr_t)m_ColorTexture, ImVec2(m_Width, m_Height), 
        ImVec2(0, 1), ImVec2(1, 0));
    
    Toolbar::Instance().Render(windowSize);
    AtomsTemplate::Instance().RenderMeasurementModeOverlay();

    ImGuiIO& io = ImGui::GetIO();
    bool isHovered = ImGui::IsWindowHovered();

    if (!m_RenderPaused) {
        processEvents();

        const bool nodeInfoEnabled = AtomsTemplate::Instance().IsNodeInfoEnabled();
        const bool isDragging = io.MouseDown[ImGuiMouseButton_Left] ||
            io.MouseDown[ImGuiMouseButton_Right] ||
            io.MouseDown[ImGuiMouseButton_Middle];
        const bool hasWheelInput = io.MouseWheel != 0.0f;
        const bool shouldHoverPick = isHovered &&
            nodeInfoEnabled &&
            !isDragging &&
            !hasWheelInput &&
            !m_InteractionLodActive;

        if (shouldHoverPick) {
            static constexpr auto kHoverPickInterval = std::chrono::milliseconds(33);
            static auto lastHoverPickTime = std::chrono::steady_clock::time_point{};
            static bool hasLastHoverPickPos = false;
            static int lastHoverPickX = 0;
            static int lastHoverPickY = 0;

            float relX = io.MousePos.x - windowPos.x - viewportPos.x;
            float relY = io.MousePos.y - windowPos.y - viewportPos.y;
            int pickX = static_cast<int>(relX);
            int pickY = static_cast<int>(m_Height - relY);

            const auto now = std::chrono::steady_clock::now();
            const bool moved = !hasLastHoverPickPos ||
                pickX != lastHoverPickX ||
                pickY != lastHoverPickY;
            const bool due = lastHoverPickTime == std::chrono::steady_clock::time_point{} ||
                (now - lastHoverPickTime) >= kHoverPickInterval;

            if (moved || due) {
                if (m_Picker->Pick(pickX, pickY, 0, m_Renderer)) {
                    vtkActor* pickedActor = m_Picker->GetActor();
                    double* pickPos = m_Picker->GetPickPosition();
                    AtomsTemplate::Instance().UpdateHoveredAtomByPicker(pickedActor, pickPos);
                } else {
                    AtomsTemplate::Instance().ClearHover();
                }
                lastHoverPickX = pickX;
                lastHoverPickY = pickY;
                lastHoverPickTime = now;
                hasLastHoverPickPos = true;
            }
        } else {
            AtomsTemplate::Instance().ClearHover();
        }
    } else {
        AtomsTemplate::Instance().ClearHover();
    }

    renderDragSelectionOverlay(windowPos, viewportPos);

    if (m_ShowPerformanceOverlay) {
        renderPerformanceOverlay(windowPos, viewportPos);
    }
    
    // 툴팁 마우스 좌표 저장
    float tooltipMouseX = io.MousePos.x;
    float tooltipMouseY = io.MousePos.y;
    
    ImGui::End();
    ImGui::PopStyleVar();
    
    // ========== 툴팁 렌더링 (윈도우 외부에서) ==========
    AtomsTemplate::Instance().RenderAtomTooltip(tooltipMouseX, tooltipMouseY);
}

void VtkViewer::SetRenderPaused(bool paused) {
    if (m_RenderPaused == paused) {
        return;
    }
    m_RenderPaused = paused;
    if (!m_RenderPaused) {
        RequestRender();
    }
}

void VtkViewer::RequestRender() {
    m_RenderDirty = true;
}

void VtkViewer::SetPerformanceOverlayEnabled(bool enabled) {
    if (m_ShowPerformanceOverlay == enabled) {
        return;
    }
    m_ShowPerformanceOverlay = enabled;
    RequestRender();
}

void VtkViewer::BeginInteractionLod() {
    if (m_RenderPaused || m_InteractionLodActive) {
        return;
    }

    m_InteractionLodStates.clear();
    MeshManager& meshManager = MeshManager::Instance();
    meshManager.GetMeshTree()->TraverseTree([&](const TreeNode* node, void*) {
        Mesh* mesh = meshManager.GetMeshByIdMutable(node->GetId());
        if (!mesh) {
            return;
        }
        if (!mesh->IsVolumeRenderingAvailable()) {
            return;
        }
        if (!mesh->GetVolumeMeshVisibility() || !mesh->GetVolumeRenderVisibility()) {
            return;
        }

        InteractionLodState state;
        state.volumeQualityIndex = static_cast<int>(mesh->GetVolumeQuality());
        state.sampleDistanceIndex = mesh->GetVolumeSampleDistanceIndex();
        state.autoAdjustSampleDistances = mesh->IsVolumeAutoAdjustSampleDistances();
        state.interactiveAdjustSampleDistances = mesh->IsVolumeInteractiveAdjustSampleDistances();
        m_InteractionLodStates[node->GetId()] = state;

        if (mesh->GetVolumeQuality() != kInteractionVolumeQuality) {
            mesh->SetVolumeQuality(kInteractionVolumeQuality);
        }
        if (!state.autoAdjustSampleDistances || !state.interactiveAdjustSampleDistances) {
            mesh->SetVolumeSampleDistanceAutoMode(true, true);
        }
        if (state.sampleDistanceIndex != kInteractionSampleDistanceIndex) {
            mesh->SetVolumeSampleDistanceIndex(kInteractionSampleDistanceIndex);
        }
    });

    if (!m_InteractionLodStates.empty()) {
        m_InteractionLodActive = true;
    }
}

void VtkViewer::EndInteractionLod() {
    if (!m_InteractionLodActive) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const bool anyMouseDown = io.MouseDown[ImGuiMouseButton_Left] ||
        io.MouseDown[ImGuiMouseButton_Right] ||
        io.MouseDown[ImGuiMouseButton_Middle];
    const auto now = std::chrono::steady_clock::now();
    if (anyMouseDown || isWheelInteractionLodHoldActive(m_LastWheelInteractionTime, now)) {
        return;
    }

    MeshManager& meshManager = MeshManager::Instance();
    for (auto it = m_InteractionLodStates.begin(); it != m_InteractionLodStates.end();) {
        Mesh* mesh = meshManager.GetMeshByIdMutable(it->first);
        if (mesh) {
            const auto originalQuality = static_cast<Mesh::VolumeQuality>(it->second.volumeQualityIndex);
            if (mesh->GetVolumeQuality() != originalQuality) {
                mesh->SetVolumeQuality(originalQuality);
            }

            const bool currentAutoAdjust = mesh->IsVolumeAutoAdjustSampleDistances();
            const bool currentInteractiveAdjust = mesh->IsVolumeInteractiveAdjustSampleDistances();
            if (currentAutoAdjust != it->second.autoAdjustSampleDistances ||
                currentInteractiveAdjust != it->second.interactiveAdjustSampleDistances) {
                mesh->SetVolumeSampleDistanceAutoMode(
                    it->second.autoAdjustSampleDistances,
                    it->second.interactiveAdjustSampleDistances);
            }

            if (!it->second.autoAdjustSampleDistances &&
                !it->second.interactiveAdjustSampleDistances &&
                mesh->GetVolumeSampleDistanceIndex() != it->second.sampleDistanceIndex) {
                mesh->SetVolumeSampleDistanceIndex(it->second.sampleDistanceIndex);
            }
        }
        it = m_InteractionLodStates.erase(it);
    }

    m_InteractionLodActive = false;
    m_LastWheelInteractionTime = std::chrono::steady_clock::time_point{};
}

void VtkViewer::RequestInitialLayout() {
    m_RequestInitialLayout = true;
}

void VtkViewer::RequestForcedWindowLayout(const ImVec2& pos, const ImVec2& size, int frames) {
    m_ForcedWindowPos = pos;
    m_ForcedWindowSize = size;
    m_ForcedWindowLayoutFramesRemaining = std::max(frames, 1);
    m_HasForcedWindowLayout = true;
}

void VtkViewer::ResetView() {
    if (!m_Renderer) {
        return;
    }

    m_Renderer->ResetCamera();

    vtkCamera* camera = m_Renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    double focalPoint[3];
    camera->GetFocalPoint(focalPoint);

    double cameraPos[3];
    camera->GetPosition(cameraPos);

    double dist = std::sqrt(vtkMath::Distance2BetweenPoints(cameraPos, focalPoint));
    double newPos[3] = { focalPoint[0], focalPoint[1], focalPoint[2] + dist };
    double viewUp[3] = { 0.0, 1.0, 0.0 };

    camera->SetPosition(newPos);
    camera->SetViewUp(viewUp);
    camera->OrthogonalizeViewUp();

    m_Renderer->ResetCameraClippingRange();
    setCameraDirection(CameraDirection::ZPLUS_YPLUS);
    RequestRender();
}

void VtkViewer::FitViewToVisibleProps() {
    if (!m_Renderer) {
        return;
    }
    m_Renderer->ResetCamera();
    m_Renderer->ResetCameraClippingRange();
    RequestRender();
}

namespace {
bool AlignCameraToAxisImpl(vtkRenderer* renderer, const double axis[3], const double upCandidate[3]) {
    if (!renderer) {
        return false;
    }

    double dir[3] = { axis[0], axis[1], axis[2] };
    double dirNorm = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);

    if (dirNorm < 1e-6) {
        return false;
    }
    dir[0] /= dirNorm;
    dir[1] /= dirNorm;
    dir[2] /= dirNorm;

    double up[3] = { upCandidate[0], upCandidate[1], upCandidate[2] };
    double dot = up[0] * dir[0] + up[1] * dir[1] + up[2] * dir[2];

    up[0] -= dot * dir[0];
    up[1] -= dot * dir[1];
    up[2] -= dot * dir[2];
    double upNorm = std::sqrt(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);

    if (upNorm < 1e-6) {

        up[0] = 0.0;
        up[1] = 1.0;
        up[2] = 0.0;
        dot = up[0] * dir[0] + up[1] * dir[1] + up[2] * dir[2];
        up[0] -= dot * dir[0];
        up[1] -= dot * dir[1];
        up[2] -= dot * dir[2];
        upNorm = std::sqrt(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
        if (upNorm < 1e-6) {
            up[0] = 1.0;
            up[1] = 0.0;
            up[2] = 0.0;
            dot = up[0] * dir[0] + up[1] * dir[1] + up[2] * dir[2];
            up[0] -= dot * dir[0];
            up[1] -= dot * dir[1];
            up[2] -= dot * dir[2];
            upNorm = std::sqrt(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
            if (upNorm < 1e-6) {
                return false;
            }
        }
    }

    up[0] /= upNorm;
    up[1] /= upNorm;
    up[2] /= upNorm;

    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) {
        SPDLOG_WARN("  -> No active camera!");

        return false;
    }

    double focalPoint[3];
    camera->GetFocalPoint(focalPoint);

    double cameraPos[3];
    camera->GetPosition(cameraPos);

    double dx = cameraPos[0] - focalPoint[0];
    double dy = cameraPos[1] - focalPoint[1];
    double dz = cameraPos[2] - focalPoint[2];
    double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist < 1e-6) {
        dist = 1.0;
    }

    double newPos[3] = {
        focalPoint[0] + dir[0] * dist,
        focalPoint[1] + dir[1] * dist,
        focalPoint[2] + dir[2] * dist
    };


    camera->SetPosition(newPos);
    camera->SetViewUp(up);
    camera->OrthogonalizeViewUp();
    renderer->ResetCameraClippingRange();

    // ✅ 설정 후 확인
    double finalPos[3], finalUp[3];
    camera->GetPosition(finalPos);
    camera->GetViewUp(finalUp);
        
    return true;
}
} // namespace

void VtkViewer::AlignCameraToCellAxis(int axisIndex) {
    if (!m_Renderer || axisIndex < 0 || axisIndex > 2) {
        return;
    }
    if (!atoms::domain::hasUnitCell()) {
        return;
    }

    float cell[3][3];
    atoms::domain::getCellMatrix(cell);
    // ✅ 디버그 로그 추가
    SPDLOG_INFO("AlignCameraToCellAxis({}) - Cell matrix:", axisIndex);
    SPDLOG_INFO("  a1 (cell[0]): [{:.4f}, {:.4f}, {:.4f}]", cell[0][0], cell[0][1], cell[0][2]);
    SPDLOG_INFO("  a2 (cell[1]): [{:.4f}, {:.4f}, {:.4f}]", cell[1][0], cell[1][1], cell[1][2]);
    SPDLOG_INFO("  a3 (cell[2]): [{:.4f}, {:.4f}, {:.4f}]", cell[2][0], cell[2][1], cell[2][2]);

    double axis[3] = {
        static_cast<double>(cell[axisIndex][0]),
        static_cast<double>(cell[axisIndex][1]),
        static_cast<double>(cell[axisIndex][2])
    };
    SPDLOG_INFO("  Selected axis[{}]: [{:.4f}, {:.4f}, {:.4f}]", axisIndex, axis[0], axis[1], axis[2]);
    // axis 벡터 길이 체크
    double axisNorm = std::sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
    SPDLOG_INFO("  Axis norm: {:.6f}", axisNorm);

    
    int upIndex = (axisIndex == 0) ? 2 : (axisIndex == 1) ? 0 : 1;
    double up[3] = {
        static_cast<double>(cell[upIndex][0]),
        static_cast<double>(cell[upIndex][1]),
        static_cast<double>(cell[upIndex][2])
    };

    if (AlignCameraToAxisImpl(m_Renderer, axis, up)) {
        setCameraDirection(CameraDirection::NOT_ALIGNED);
        RequestRender();
    }
}

void VtkViewer::AlignCameraToIcellAxis(int axisIndex) {
    if (!m_Renderer || axisIndex < 0 || axisIndex > 2) {
        return;
    }
    if (!atoms::domain::hasUnitCell()) {
        return;
    }

    float invmatrix[3][3];
    atoms::domain::getCellInverse(invmatrix);

    double icell[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            icell[i][j] = static_cast<double>(invmatrix[j][i]);
        }
    }

    double axis[3] = {
        icell[axisIndex][0],
        icell[axisIndex][1],
        icell[axisIndex][2]
    };

    int upIndex = (axisIndex == 0) ? 2 : (axisIndex == 1) ? 0 : 1;
    double up[3] = {
        icell[upIndex][0],
        icell[upIndex][1],
        icell[upIndex][2]
    };

    if (AlignCameraToAxisImpl(m_Renderer, axis, up)) {
        setCameraDirection(CameraDirection::NOT_ALIGNED);
        RequestRender();
    }
}

void VtkViewer::RenderBgColorPopup(bool* openWindow) {
    const char* popupTitle = ICON_FA6_PAINT_ROLLER"  Background Color";

    ImGui::OpenPopup(popupTitle);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // Get default background color
    double* curBgColorTop = m_Renderer->GetBackground2();
    static float imguiColorTop[3] = {
        (float)curBgColorTop[0], 
        (float)curBgColorTop[1],
        (float)curBgColorTop[2]
    };
    double* curBgColorBottom = m_Renderer->GetBackground();
    static float imguiColorBottom[3] = {
        (float)curBgColorBottom[0], 
        (float)curBgColorBottom[1],
        (float)curBgColorBottom[2]
    };

    if (ImGui::BeginPopupModal(popupTitle, openWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
        const ImVec2 btnSize = ImVec2(App::TextBaseWidth() * 12,
            App::TextBaseHeightWithSpacing() * 1.5);

        ImGui::ColorEdit3("Top Color", imguiColorTop);
        ImGui::ColorEdit3("Bottom Color", imguiColorBottom);

        if (ImGui::Button("Change Color", btnSize)) {
            m_Renderer->SetBackground2(
                (double)imguiColorTop[0], (double)imguiColorTop[1], (double)imguiColorTop[2]);
            m_Renderer->SetBackground(
                (double)imguiColorBottom[0], (double)imguiColorBottom[1], (double)imguiColorBottom[2]);
            RequestRender();

            saveBgColorToLocalStorage(imguiColorTop, imguiColorBottom);
        }
        ImGui::SetItemDefaultFocus();  // "OK" button is default focus.

        ImGui::Separator();

        if (ImGui::Button("Black", btnSize)) {
            m_Renderer->SetBackground2(0.0, 0.0, 0.0);
            m_Renderer->SetBackground(0.0, 0.0, 0.0);
            RequestRender();
            
            imguiColorTop[0] = 0.0;
            imguiColorTop[1] = 0.0;
            imguiColorTop[2] = 0.0;
            imguiColorBottom[0] = 0.0;
            imguiColorBottom[1] = 0.0;
            imguiColorBottom[2] = 0.0;

            saveBgColorToLocalStorage(imguiColorTop, imguiColorBottom);
        }
        ImGui::SameLine();
        if (ImGui::Button("Skyblue", btnSize))
        {
            m_Renderer->SetBackground2(1.0, 1.0, 1.0);
            m_Renderer->SetBackground(0.2, 0.4, 0.9);
            RequestRender();

            imguiColorTop[0] = 1.0;
            imguiColorTop[1] = 1.0;
            imguiColorTop[2] = 1.0;
            imguiColorBottom[0] = 0.2;
            imguiColorBottom[1] = 0.4;
            imguiColorBottom[2] = 0.9;

            saveBgColorToLocalStorage(imguiColorTop, imguiColorBottom);
        }
        ImGui::SameLine();
        if (ImGui::Button("White", btnSize))
        {
            m_Renderer->SetBackground2(1.0, 1.0, 1.0);
            m_Renderer->SetBackground(1.0, 1.0, 1.0);
            RequestRender();

            imguiColorTop[0] = 1.0;
            imguiColorTop[1] = 1.0;
            imguiColorTop[2] = 1.0;
            imguiColorBottom[0] = 1.0;
            imguiColorBottom[1] = 1.0;
            imguiColorBottom[2] = 1.0;

            saveBgColorToLocalStorage(imguiColorTop, imguiColorBottom);
        }
        ImGui::SameLine();
        if (ImGui::Button("Gray", btnSize))
        {
            m_Renderer->SetBackground2(1.0, 1.0, 1.0);
            m_Renderer->SetBackground(0.625, 0.625, 0.625);
            RequestRender();

            imguiColorTop[0] = 1.0;
            imguiColorTop[1] = 1.0;
            imguiColorTop[2] = 1.0;
            imguiColorBottom[0] = 0.625;
            imguiColorBottom[1] = 0.625;
            imguiColorBottom[2] = 0.625;

            saveBgColorToLocalStorage(imguiColorTop, imguiColorBottom);
        }

        ImGui::EndPopup();
    }

    // If the window is closed by 'X' button,
    // return imguiColorTop and imguicolorBottom to the current background color.
    if (*openWindow == false) {
        double* curBgColorTop = m_Renderer->GetBackground2();
        imguiColorTop[0] = curBgColorTop[0];
        imguiColorTop[1] = curBgColorTop[1];
        imguiColorTop[2] = curBgColorTop[2];
        double* curBgColorBottom = m_Renderer->GetBackground();
        imguiColorBottom[0] = curBgColorBottom[0];
        imguiColorBottom[1] = curBgColorBottom[1];
        imguiColorBottom[2] = curBgColorBottom[2];
        
        ImGui::CloseCurrentPopup();
    }
}

void VtkViewer::saveBgColorToLocalStorage(const float* colorTop, const float* colorBottom) {
    EM_ASM({
        const bgColorKey = UTF8ToString($0);
        const color = new Array($1, $2, $3, $4, $5, $6);

        localStorage.setItem(bgColorKey, JSON.stringify(color));
    }, IMGUI_BGCOLOR_KEY, colorTop[0], colorTop[1], colorTop[2], colorBottom[0], colorBottom[1], colorBottom[2]);
}

void VtkViewer::loadBgColorFromLocalStorage(float* colorTop, float* colorBottom) {
    EM_ASM({
        const bgColorKey = UTF8ToString($0);
        const color = JSON.parse(localStorage.getItem(bgColorKey));

        // Convert to float and set to color, if exists.
        if (color && color.length === 6) {
            setValue($1, color[0], 'float');
            setValue($2, color[1], 'float');
            setValue($3, color[2], 'float');
            setValue($4, color[3], 'float');
            setValue($5, color[4], 'float');
            setValue($6, color[5], 'float');
        }
    }, IMGUI_BGCOLOR_KEY, &colorTop[0], &colorTop[1], &colorTop[2], &colorBottom[0], &colorBottom[1], &colorBottom[2]);
}

void VtkViewer::AddActor(vtkSmartPointer<vtkActor> actor, bool resetCamera) {
    m_Renderer->AddActor(actor);
    if (resetCamera) {
        m_Renderer->ResetCamera();
    }
    RequestRender();
}

void VtkViewer::AddMeasurementOverlayActor(vtkSmartPointer<vtkActor> actor, bool resetCamera) {
    if (!actor || !m_MeasurementOverlayRenderer) {
        return;
    }
    m_MeasurementOverlayRenderer->AddActor(actor);
    if (resetCamera && m_Renderer) {
        m_Renderer->ResetCamera();
    }
    RequestRender();
}

void VtkViewer::AddActor2D(
    vtkSmartPointer<vtkActor2D> actor2D,
    bool resetCamera,
    bool measurementOverlay) {
    if (!actor2D) {
        return;
    }

    vtkRenderer* targetRenderer = m_Renderer;
    if (measurementOverlay && m_MeasurementOverlayRenderer) {
        targetRenderer = m_MeasurementOverlayRenderer;
    }
    if (!targetRenderer) {
        return;
    }

    targetRenderer->AddActor2D(actor2D);
    if (resetCamera && m_Renderer) {
        m_Renderer->ResetCamera();
    }
    RequestRender();
}

void VtkViewer::AddVolume(vtkSmartPointer<vtkVolume> volume, bool resetCamera) {
    if (!volume) {
        return;
    }
    m_Renderer->AddVolume(volume);
    if (resetCamera) {
        m_Renderer->ResetCamera();
    }
    RequestRender();
}

bool VtkViewer::CaptureActorImage(vtkActor* actor,
                                  std::vector<uint8_t>& rgbaPixels,
                                  int& width,
                                  int& height,
                                  bool isolateActor,
                                  const double* viewDirection,
                                  const double* viewUpHint,
                                  const double* backgroundColorRgb) {
    rgbaPixels.clear();
    width = 0;
    height = 0;

    if (!actor || !m_Renderer || !m_RenderWindow || m_Framebuffer == 0 || m_Width <= 0 || m_Height <= 0) {
        return false;
    }

    vtkPropCollection* props = m_Renderer->GetViewProps();
    if (!props) {
        return false;
    }

    struct PropVisibilityState {
        vtkProp* prop = nullptr;
        int visibility = 0;
    };

    std::vector<PropVisibilityState> visibilityStates;
    visibilityStates.reserve(static_cast<size_t>(props->GetNumberOfItems()));

    vtkCamera* camera = m_Renderer->GetActiveCamera();
    bool hasCameraState = false;
    double savedPosition[3] = {0.0, 0.0, 1.0};
    double savedFocalPoint[3] = {0.0, 0.0, 0.0};
    double savedViewUp[3] = {0.0, 1.0, 0.0};
    int savedParallelProjection = 0;
    if (camera) {
        camera->GetPosition(savedPosition);
        camera->GetFocalPoint(savedFocalPoint);
        camera->GetViewUp(savedViewUp);
        savedParallelProjection = camera->GetParallelProjection();
        hasCameraState = true;
    }

    const int savedGradientBackground = m_Renderer->GetGradientBackground();
    double savedBackground[3] = {0.0, 0.0, 0.0};
    double savedBackground2[3] = {0.0, 0.0, 0.0};
    m_Renderer->GetBackground(savedBackground);
    m_Renderer->GetBackground2(savedBackground2);

    if (backgroundColorRgb) {
        m_Renderer->SetGradientBackground(false);
        m_Renderer->SetBackground(backgroundColorRgb[0], backgroundColorRgb[1], backgroundColorRgb[2]);
        m_Renderer->SetBackground2(backgroundColorRgb[0], backgroundColorRgb[1], backgroundColorRgb[2]);
    }

    if (camera && viewDirection) {
        double direction[3] = {viewDirection[0], viewDirection[1], viewDirection[2]};
        if (normalize3(direction)) {
            double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            actor->GetBounds(bounds);
            double focalPoint[3] = {savedFocalPoint[0], savedFocalPoint[1], savedFocalPoint[2]};
            if (isFiniteBounds(bounds)) {
                focalPoint[0] = 0.5 * (bounds[0] + bounds[1]);
                focalPoint[1] = 0.5 * (bounds[2] + bounds[3]);
                focalPoint[2] = 0.5 * (bounds[4] + bounds[5]);
            }

            double distance = std::sqrt(
                (savedPosition[0] - savedFocalPoint[0]) * (savedPosition[0] - savedFocalPoint[0]) +
                (savedPosition[1] - savedFocalPoint[1]) * (savedPosition[1] - savedFocalPoint[1]) +
                (savedPosition[2] - savedFocalPoint[2]) * (savedPosition[2] - savedFocalPoint[2]));
            if (distance <= 1e-6) {
                if (isFiniteBounds(bounds)) {
                    const double dx = bounds[1] - bounds[0];
                    const double dy = bounds[3] - bounds[2];
                    const double dz = bounds[5] - bounds[4];
                    distance = std::max(1.0, std::sqrt(dx * dx + dy * dy + dz * dz) * 1.5);
                } else {
                    distance = 1.0;
                }
            }

            double position[3] = {
                focalPoint[0] + direction[0] * distance,
                focalPoint[1] + direction[1] * distance,
                focalPoint[2] + direction[2] * distance
            };

            double up[3] = {0.0, 1.0, 0.0};
            if (viewUpHint) {
                setVec3(up, viewUpHint);
            } else {
                setVec3(up, savedViewUp);
            }
            if (!normalize3(up)) {
                up[0] = 0.0;
                up[1] = 1.0;
                up[2] = 0.0;
            }

            const double dotUp = up[0] * direction[0] + up[1] * direction[1] + up[2] * direction[2];
            up[0] -= dotUp * direction[0];
            up[1] -= dotUp * direction[1];
            up[2] -= dotUp * direction[2];
            if (!normalize3(up)) {
                up[0] = 0.0;
                up[1] = 0.0;
                up[2] = 1.0;
                const double dotFallback = up[0] * direction[0] + up[1] * direction[1] + up[2] * direction[2];
                up[0] -= dotFallback * direction[0];
                up[1] -= dotFallback * direction[1];
                up[2] -= dotFallback * direction[2];
                if (!normalize3(up)) {
                    up[0] = 1.0;
                    up[1] = 0.0;
                    up[2] = 0.0;
                }
            }

            camera->SetFocalPoint(focalPoint);
            camera->SetPosition(position);
            camera->SetViewUp(up);
            camera->SetParallelProjection(true);
            camera->OrthogonalizeViewUp();
            m_Renderer->ResetCameraClippingRange();
        }
    }

    props->InitTraversal();
    while (vtkProp* prop = props->GetNextProp()) {
        visibilityStates.push_back({prop, prop->GetVisibility()});
        if (isolateActor && prop != actor) {
            prop->SetVisibility(0);
        }
    }

    actor->SetVisibility(1);

    m_RenderWindow->Render();
    m_RenderWindow->WaitForCompletion();

    width = m_Width;
    height = m_Height;
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    rgbaPixels.assign(pixelCount, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    flipRgbaRows(rgbaPixels, width, height);

    for (const auto& state : visibilityStates) {
        if (state.prop) {
            state.prop->SetVisibility(state.visibility);
        }
    }

    m_Renderer->SetGradientBackground(savedGradientBackground);
    m_Renderer->SetBackground(savedBackground[0], savedBackground[1], savedBackground[2]);
    m_Renderer->SetBackground2(savedBackground2[0], savedBackground2[1], savedBackground2[2]);
    if (camera && hasCameraState) {
        camera->SetPosition(savedPosition);
        camera->SetFocalPoint(savedFocalPoint);
        camera->SetViewUp(savedViewUp);
        camera->SetParallelProjection(savedParallelProjection);
        camera->OrthogonalizeViewUp();
        m_Renderer->ResetCameraClippingRange();
    }

    // Restore the viewer texture immediately to avoid one-frame actor-isolation artifacts.
    m_RenderWindow->Render();
    m_RenderWindow->WaitForCompletion();
    m_RenderDirty = false;
    return true;
}

vtkCamera* VtkViewer::GetActiveCamera() {
    if (!m_Renderer) {
        return nullptr;
    }
    return m_Renderer->GetActiveCamera();
}

void VtkViewer::RemoveActor(vtkSmartPointer<vtkActor> actor) {
    // If the actor is not in the renderer, do nothing in RemoveActor.
    m_Renderer->RemoveActor(actor);
    RequestRender();
}

void VtkViewer::RemoveMeasurementOverlayActor(vtkSmartPointer<vtkActor> actor) {
    if (!m_MeasurementOverlayRenderer) {
        return;
    }
    m_MeasurementOverlayRenderer->RemoveActor(actor);
    RequestRender();
}

void VtkViewer::RemoveActor2D(vtkSmartPointer<vtkActor2D> actor2D) {
    if (m_Renderer) {
        m_Renderer->RemoveActor2D(actor2D);
    }
    if (m_MeasurementOverlayRenderer) {
        m_MeasurementOverlayRenderer->RemoveActor2D(actor2D);
    }
    RequestRender();
}

void VtkViewer::RemoveVolume(vtkSmartPointer<vtkVolume> volume) {
    if (!volume) {
        return;
    }
    m_Renderer->RemoveVolume(volume);
    RequestRender();
}

void VtkViewer::SetProjectionMode(ProjectionMode mode) {
    if (!m_Renderer) {
        return;
    }

    vtkCamera* camera = m_Renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    const bool targetParallel = (mode == ProjectionMode::PARALLEL);
    const bool currentParallel = (camera->GetParallelProjection() != 0);
    if (currentParallel == targetParallel) {
        RequestRender();
        return;
    }

    constexpr double kPi = 3.14159265358979323846;
    constexpr double kDegToRad = kPi / 180.0;
    constexpr double kRadToDeg = 180.0 / kPi;
    constexpr double kEpsilon = 1e-9;

    double savedPosition[3] = {0.0, 0.0, 1.0};
    double savedFocalPoint[3] = {0.0, 0.0, 0.0};
    double savedViewUp[3] = {0.0, 1.0, 0.0};
    camera->GetPosition(savedPosition);
    camera->GetFocalPoint(savedFocalPoint);
    camera->GetViewUp(savedViewUp);

    const double dx = savedPosition[0] - savedFocalPoint[0];
    const double dy = savedPosition[1] - savedFocalPoint[1];
    const double dz = savedPosition[2] - savedFocalPoint[2];
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    const double savedParallelScale = camera->GetParallelScale();
    const double savedViewAngleDeg = std::clamp(camera->GetViewAngle(), 1.0, 179.0);

    if (targetParallel) {
        // Preserve apparent zoom when switching perspective -> parallel.
        double targetParallelScale = savedParallelScale;
        if (distance > kEpsilon) {
            const double halfAngleRad = 0.5 * savedViewAngleDeg * kDegToRad;
            targetParallelScale = distance * std::tan(halfAngleRad);
        }

        camera->SetParallelProjection(true);
        if (std::isfinite(targetParallelScale) && targetParallelScale > kEpsilon) {
            camera->SetParallelScale(targetParallelScale);
        }
    } else {
        // Preserve apparent zoom when switching parallel -> perspective.
        double targetViewAngleDeg = savedViewAngleDeg;
        if (distance > kEpsilon && savedParallelScale > kEpsilon) {
            targetViewAngleDeg =
                2.0 * std::atan(savedParallelScale / distance) * kRadToDeg;
        }
        targetViewAngleDeg = std::clamp(targetViewAngleDeg, 1.0, 179.0);

        camera->SetParallelProjection(false);
        camera->SetViewAngle(targetViewAngleDeg);
    }

    // Keep current viewpoint (position/focal/up) unchanged.
    camera->SetPosition(savedPosition);
    camera->SetFocalPoint(savedFocalPoint);
    camera->SetViewUp(savedViewUp);
    camera->OrthogonalizeViewUp();

    m_Renderer->ResetCameraClippingRange();
    RequestRender();
}

void VtkViewer::SetArrowRotateStepDeg(float stepDeg) {
    if (!std::isfinite(stepDeg)) {
        return;
    }

    constexpr float kMinStepDeg = 1.0f;
    constexpr float kMaxStepDeg = 180.0f;
    const float roundedStepDeg = std::round(stepDeg);
    m_ArrowRotateStepDeg = std::clamp(roundedStepDeg, kMinStepDeg, kMaxStepDeg);
}
