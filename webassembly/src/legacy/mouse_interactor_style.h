#pragma once

// VTK
#include <vtkInteractorStyleTrackballCamera.h>


// Custom interactor style class for mouse events
class MouseInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static MouseInteractorStyle* New();
    vtkTypeMacro(MouseInteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnLeftButtonDown() override;
    virtual void OnMouseWheelForward() override;
    virtual void OnMouseWheelBackward() override;
};