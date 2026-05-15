/**
 * @file features/edit/atoms/atom_manager.h
 * @brief Atom data mutator for Edit/Atoms.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_set>

namespace features::edit::atoms {

class AtomManager {
public:
    explicit AtomManager(core::scene::SceneState& scene);

    int Add(int32_t structureId, const std::string& symbol, const std::array<float, 3>& position);
    bool Remove(int32_t structureId, uint32_t atomId);
    bool Move(int32_t structureId, uint32_t atomId, const std::array<float, 3>& position);
    bool MoveFractional(int32_t structureId, uint32_t atomId, const std::array<float, 3>& fractional);
    bool SetVisible(int32_t structureId, uint32_t atomId, bool visible);
    bool SetSelected(int32_t structureId, uint32_t atomId, bool selected);
    int SetSelectionByIds(int32_t structureId, const std::unordered_set<uint32_t>& selectedIds, bool additive);
    bool SetSymbol(int32_t structureId, uint32_t atomId, const std::string& symbol);
    bool SetRadius(int32_t structureId, uint32_t atomId, float radius);
    bool SetGroup(int32_t structureId, uint32_t atomId, const std::string& group);

    int RemoveSelected(int32_t structureId);
    int TranslateSelected(int32_t structureId, const std::array<float, 3>& delta, bool fractionalSpace);
    int SetRadiusSelected(int32_t structureId, float radius);

    void SelectAll(int32_t structureId);
    void SelectNone(int32_t structureId);
    void InvertSelection(int32_t structureId);

    void OnCellChanged(int32_t structureId, bool preserveFractional);

private:
    int32_t ResolveStructureId(int32_t structureId) const;
    void EmitAtomsChanged(int32_t structureId) const;

    core::scene::SceneState& scene_;
};

} // namespace features::edit::atoms
