/**
 * @file core/vtk/vtk_viewer.h
 * @brief Minimal VTK viewer shell for Phase 1.
 */
#pragma once

#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

namespace core::vtk {

/**
 * @class VtkViewer
 * @brief Owns an empty VTK scene and renders a placeholder viewer panel.
 */
class VtkViewer {
public:
    /**
     * @brief Returns the singleton viewer instance.
     */
    static VtkViewer& Instance();

    /**
     * @brief Initializes VTK renderer/window/interactor objects.
     */
    void Init();

    /**
     * @brief Draws the viewer panel for the current frame.
     */
    void Render();

    /**
     * @brief Updates render window pixel size.
     * @param w Width in pixels.
     * @param h Height in pixels.
     */
    void Resize(int w, int h);

    /**
     * @brief Returns the owned VTK renderer.
     */
    vtkRenderer* GetRenderer() const;

    /**
     * @brief Returns the owned VTK render window.
     */
    vtkRenderWindow* GetRenderWindow() const;

    /**
     * @brief Returns the owned VTK interactor.
     */
    vtkRenderWindowInteractor* GetInteractor() const;

private:
    VtkViewer() = default;
    ~VtkViewer() = default;
    VtkViewer(const VtkViewer&) = delete;
    VtkViewer& operator=(const VtkViewer&) = delete;

    vtkSmartPointer<vtkRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
    bool m_initialized = false;
};

} // namespace core::vtk
