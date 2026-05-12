/**
 * @file features/measurement/measurement_controller.h
 * @brief Measurement mode state machine and mouse-event subscriber.
 */
#pragma once

#include "measurement_mode.h"
#include "measurement_store.h"

#include "core/scene/events.h"

#include <vtkSmartPointer.h>

#include <cstdint>
#include <vector>

class vtkActor;
class vtkActor2D;

namespace core::scene {
struct SceneState;
}

namespace core::vtk {
class MouseInteractor;
}

namespace features::measurement {

class MeasurementController {
public:
    MeasurementController(core::scene::SceneState& scene, MeasurementStore& store);
    ~MeasurementController();

    void SubscribeEvents();
    void Subscribe(core::vtk::MouseInteractor& mouseInteractor);

    void EnterMode(MeasurementMode mode);
    void ExitMode();
    MeasurementMode CurrentMode() const { return mode_; }
    bool IsActive() const { return mode_ != MeasurementMode::None; }

    const std::vector<uint32_t>& PickedAtomIds() const { return pickedAtomIds_; }
    void ClearPickedAtoms();
    bool ApplyCenterMeasurement();

private:
    struct PickVisual {
        uint32_t atomId = 0;
        vtkSmartPointer<vtkActor> shellActor;
        vtkSmartPointer<vtkActor2D> labelActor;
    };

    void HandleAtomPicked(const core::scene::AtomPickedEvent& event);
    void HandleEmptyClick(const core::scene::EmptyClickEvent& event);
    void HandleDragSelection(const core::scene::DragSelectionEvent& event);
    void HandleAtomsChanged(const core::scene::AtomsChangedEvent& event);
    void HandleStructureRemoved(const core::scene::StructureRemovedEvent& event);

    int32_t ActiveStructureId() const;
    const core::scene::AtomRecord* ResolvePickedAtom(
        const core::scene::AtomPickedEvent& event,
        int32_t& structureId) const;
    std::vector<uint32_t> CollectAtomsInRect(const core::scene::DragSelectionEvent& event) const;

    void AddPickedAtom(int32_t structureId, uint32_t atomId);
    bool CreateMeasurementFromPickedAtoms();
    void ClearPickVisuals();
    void SyncPickVisuals();

    core::scene::SceneState& scene_;
    MeasurementStore& store_;
    core::vtk::MouseInteractor* mouseInteractor_ = nullptr;
    MeasurementMode mode_ = MeasurementMode::None;
    std::vector<uint32_t> pickedAtomIds_;
    std::vector<PickVisual> pickVisuals_;
    bool subscribedEvents_ = false;
};

} // namespace features::measurement
