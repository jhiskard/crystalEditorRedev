/**
 * @file core/vtk/vtk_viewer.h
 * @brief Minimal VTK viewer shell for Phase 1.
 */
#pragma once

#include <vtkSmartPointer.h>

class vtkActor;
class vtkActor2D;
class vtkCamera;
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
     * @brief Adds a 3D actor to renderer.
     */
    void AddActor(vtkActor* actor) const;

    /**
     * @brief Removes a 3D actor from renderer.
     */
    void RemoveActor(vtkActor* actor) const;

    /**
     * @brief Adds a 2D actor to renderer.
     */
    void AddActor2D(vtkActor2D* actor) const;

    /**
     * @brief Removes a 2D actor from renderer.
     */
    void RemoveActor2D(vtkActor2D* actor) const;

    /**
     * @brief Requests a render on the owned window.
     */
    void RequestRender() const;

    /**
     * @brief Resets camera to fit visible props and renders.
     */
    void FitViewToVisibleProps() const;

    /**
     * @brief Returns active camera.
     */
    vtkCamera* GetActiveCamera() const;

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
