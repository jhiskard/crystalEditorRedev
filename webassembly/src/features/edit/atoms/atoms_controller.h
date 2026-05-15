/**
 * @file features/edit/atoms/atoms_controller.h
 * @brief Integration controller for Edit/Atoms.
 */
#pragma once

#include "atom_manager.h"
#include "atom_renderer.h"
#include "surrounding_atom_manager.h"

#include "core/vtk/mouse_interactor.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace features::edit::atoms {

class AtomsController {
public:
    AtomsController(core::scene::SceneState& scene, cell::CellManager& cellManager);

    void SubscribeEvents();
    void Subscribe(core::vtk::MouseInteractor& mouseInteractor);

    AtomRenderer& Renderer() { return renderer_; }
    const AtomRenderer& Renderer() const { return renderer_; }

    int32_t ActiveStructureId() const;
    int32_t EnsureActiveStructure();

    const std::vector<core::scene::AtomRecord>* ActiveAtoms() const;
    std::vector<core::scene::AtomRecord>* ActiveAtomsMutable();
    size_t ActiveAtomCount() const;
    const core::scene::AtomRecord* AtomAt(size_t index) const;

    int AddAtom(const std::string& symbol);
    int AddAtomAt(const std::string& symbol, const std::array<float, 3>& position, bool fractionalPosition);
    bool RemoveAtomByIndex(size_t index);
    int RemoveSelectedAtoms();

    bool SetSelectedByIndex(size_t index, bool selected);
    bool SetVisibleByIndex(size_t index, bool visible);
    bool SetSymbolByIndex(size_t index, const std::string& symbol);
    bool SetGroupByIndex(size_t index, const std::string& group);
    bool SetRadiusByIndex(size_t index, float radius);
    bool SetPositionByIndex(size_t index, const std::array<float, 3>& value, bool fractionalInput);

    void SelectAll();
    void SelectNone();
    void InvertSelection();
    int TranslateSelected(const std::array<float, 3>& delta, bool fractionalSpace);
    int SetRadiusSelected(float radius);

    void SetBoundaryAtomsEnabled(bool enabled);
    bool BoundaryAtomsEnabled() const;

private:
    core::scene::StructureRecord* ResolveActiveRecord();
    const core::scene::StructureRecord* ResolveActiveRecordConst() const;
    const core::scene::AtomRecord* ResolvePickedAtom(const core::scene::AtomPickedEvent& event) const;
    std::unordered_set<uint32_t> CollectAtomsInRect(const core::scene::DragSelectionEvent& event) const;
    void SelectSameElement(int32_t structureId, const std::string& symbol);
    void HandleAtomPicked(const core::scene::AtomPickedEvent& event);
    void HandleEmptyClick(const core::scene::EmptyClickEvent& event);
    void HandleDragSelection(const core::scene::DragSelectionEvent& event);
    void SyncMouseInteractorStructure();
    void SyncSelectionFlagsFromSelectionSet();

    core::scene::SceneState& scene_;
    cell::CellManager& cellManager_;
    AtomManager manager_;
    SurroundingAtomManager surroundingManager_;
    AtomRenderer renderer_;
    core::vtk::MouseInteractor* mouseInteractor_ = nullptr;
    bool subscribedEvents_ = false;
};

} // namespace features::edit::atoms

