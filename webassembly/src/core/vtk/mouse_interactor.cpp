#include "mouse_interactor.h"

#include <vtkCellPicker.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

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

void MouseInteractor::SetDoubleClickHint(bool doubleClick) {
    doubleClickHint_ = doubleClick;
}

void MouseInteractor::OnLeftButtonDown() {
    if (this->Interactor != nullptr) {
        const int* pos = this->Interactor->GetEventPosition();
        dragStartX_ = pos != nullptr ? pos[0] : 0;
        dragStartY_ = pos != nullptr ? pos[1] : 0;
        selectionModifierDown_ = this->Interactor->GetControlKey() != 0;
        additiveDrag_ = selectionModifierDown_;
    }
    leftButtonDown_ = true;
    dragging_ = false;

    if (selectionModifierDown_) {
        return;
    }
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void MouseInteractor::OnMouseMove() {
    if (leftButtonDown_ && this->Interactor != nullptr) {
        const int* pos = this->Interactor->GetEventPosition();
        if (pos != nullptr) {
            const int dx = pos[0] - dragStartX_;
            const int dy = pos[1] - dragStartY_;
            if ((dx * dx + dy * dy) > 16) {
                dragging_ = true;
            }
        }
    }
    if (selectionModifierDown_) {
        if (requestRender_) {
            requestRender_();
        }
        return;
    }
    vtkInteractorStyleTrackballCamera::OnMouseMove();
}

void MouseInteractor::OnLeftButtonUp() {
    if (leftButtonDown_ && this->Interactor != nullptr && eventBus_ != nullptr) {
        const int* pos = this->Interactor->GetEventPosition();
        const int x = pos != nullptr ? pos[0] : dragStartX_;
        const int y = pos != nullptr ? pos[1] : dragStartY_;

        if (selectionModifierDown_ && dragging_) {
            int viewportHeight = 0;
            if (vtkRenderWindow* window = this->Interactor->GetRenderWindow(); window != nullptr) {
                const int* size = window->GetSize();
                viewportHeight = size != nullptr ? size[1] : 0;
            }
            const bool additive =
                additiveDrag_ ||
                this->Interactor->GetControlKey() != 0;
            eventBus_->onDragSelection.Emit(core::scene::DragSelectionEvent{
                activeStructureId_,
                dragStartX_,
                dragStartY_,
                x,
                y,
                viewportHeight,
                additive,
                selectionModifierDown_});
        } else if (!dragging_) {
            emitPickOrEmptyClick(x, y);
        }
    }

    leftButtonDown_ = false;
    dragging_ = false;
    selectionModifierDown_ = false;
    doubleClickHint_ = false;
    if (additiveDrag_) {
        additiveDrag_ = false;
        if (requestRender_) {
            requestRender_();
        }
        return;
    }
    vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
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

void MouseInteractor::emitPickOrEmptyClick(int x, int y) {
    if (eventBus_ == nullptr) {
        return;
    }

    vtkRenderer* renderer = this->GetDefaultRenderer();
    if (renderer == nullptr && this->Interactor != nullptr) {
        renderer = this->Interactor->FindPokedRenderer(x, y);
    }

    if (renderer == nullptr) {
        eventBus_->onEmptyClick.Emit(core::scene::EmptyClickEvent{activeStructureId_, x, y, selectionModifierDown_});
        return;
    }

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.01);
    const int picked = picker->Pick(static_cast<double>(x), static_cast<double>(y), 0.0, renderer);
    if (picked == 0) {
        eventBus_->onEmptyClick.Emit(core::scene::EmptyClickEvent{activeStructureId_, x, y, selectionModifierDown_});
        return;
    }

    double pickPos[3] = {0.0, 0.0, 0.0};
    picker->GetPickPosition(pickPos);
    eventBus_->onAtomPicked.Emit(core::scene::AtomPickedEvent{
        activeStructureId_,
        {pickPos[0], pickPos[1], pickPos[2]},
        x,
        y,
        selectionModifierDown_,
        doubleClickHint_});
}

} // namespace core::vtk
