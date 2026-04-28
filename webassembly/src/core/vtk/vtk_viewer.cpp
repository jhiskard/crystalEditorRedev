/**
 * @file core/vtk/vtk_viewer.cpp
 * @brief Minimal VTK viewer shell for Phase 1.
 */
#include "vtk_viewer.h"

#include <imgui.h>

#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

namespace core::vtk {

VtkViewer& VtkViewer::Instance() {
    static VtkViewer instance;
    return instance;
}

void VtkViewer::Init() {
    if (m_initialized) {
        return;
    }

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

    m_renderer->SetBackground(0.12, 0.12, 0.14);
    m_renderWindow->AddRenderer(m_renderer);
    m_renderWindow->SetInteractor(m_interactor);
    m_renderWindow->SetSize(960, 540);

    m_initialized = true;
}

void VtkViewer::Render() {
    ImGui::Begin("Viewer");
    ImGui::TextUnformatted("Phase 1 bootstrap: empty VTK scene");
    ImGui::TextUnformatted("Rendering features will be restored in later phases.");
    ImGui::End();
}

void VtkViewer::Resize(int w, int h) {
    if (!m_renderWindow) {
        return;
    }
    m_renderWindow->SetSize(w, h);
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

} // namespace core::vtk
