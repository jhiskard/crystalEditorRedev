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
    void SetDoubleClickHint(bool doubleClick);

    void OnLeftButtonDown() override;
    void OnMouseMove() override;
    void OnLeftButtonUp() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;

private:
    void emitPickOrEmptyClick(int x, int y);

    core::scene::EventBus* eventBus_ = nullptr;
    std::function<void()> requestRender_;
    int32_t activeStructureId_ = -1;
    bool leftButtonDown_ = false;
    bool dragging_ = false;
    bool additiveDrag_ = false;
    bool selectionModifierDown_ = false;
    bool doubleClickHint_ = false;
    int dragStartX_ = 0;
    int dragStartY_ = 0;
};

} // namespace core::vtk
