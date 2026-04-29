#pragma once

#include "../scene/events.h"

#include <functional>

#include <vtkInteractorStyleTrackballCamera.h>

namespace core::vtk {

class MouseInteractor : public vtkInteractorStyleTrackballCamera {
public:
    static MouseInteractor* New();
    vtkTypeMacro(MouseInteractor, vtkInteractorStyleTrackballCamera);

    void SetEventBus(core::scene::EventBus* eventBus);
    void SetRenderRequestHandler(std::function<void()> handler);
    void SetActiveStructureId(int32_t structureId);

    void OnLeftButtonDown() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;

private:
    core::scene::EventBus* eventBus_ = nullptr;
    std::function<void()> requestRender_;
    int32_t activeStructureId_ = -1;
};

} // namespace core::vtk
