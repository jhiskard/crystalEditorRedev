#include "mouse_interactor.h"

#include <vtkObjectFactory.h>

namespace core::vtk {

vtkStandardNewMacro(MouseInteractor);

void MouseInteractor::SetEventBus(core::scene::EventBus* eventBus) {
    eventBus_ = eventBus;
}

void MouseInteractor::SetRenderRequestHandler(std::function<void()> handler) {
    requestRender_ = std::move(handler);
}

void MouseInteractor::SetActiveStructureId(int32_t structureId) {
    activeStructureId_ = structureId;
}

void MouseInteractor::OnLeftButtonDown() {
    if (eventBus_ != nullptr) {
        eventBus_->onSelectionChanged.Emit(core::scene::SelectionChangedEvent{});
    }
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void MouseInteractor::OnMouseWheelForward() {
    if (eventBus_ != nullptr) {
        eventBus_->onAtomsChanged.Emit(core::scene::AtomsChangedEvent{activeStructureId_});
    }
    if (requestRender_) {
        requestRender_();
    }
    vtkInteractorStyleTrackballCamera::OnMouseWheelForward();
}

void MouseInteractor::OnMouseWheelBackward() {
    if (eventBus_ != nullptr) {
        eventBus_->onAtomsChanged.Emit(core::scene::AtomsChangedEvent{activeStructureId_});
    }
    if (requestRender_) {
        requestRender_();
    }
    vtkInteractorStyleTrackballCamera::OnMouseWheelBackward();
}

} // namespace core::vtk
