#pragma once

#include "periodic_table.h"

#include "core/data/element_database.h"
#include "core/scene/scene_state.h"

#include <array>
#include <string>

namespace features::build::periodic_table {

class PeriodicTableController {
public:
    explicit PeriodicTableController(core::scene::SceneState& scene);

    void AddAtom(const std::string& symbol);
    void AddAtomAt(const std::string& symbol, const std::array<float, 3>& position);
    void AddAtomAtFractional(const std::string& symbol, const std::array<float, 3>& fractional);

    const std::string& SelectedSymbol() const { return selectedSymbol_; }
    void SetSelectedSymbol(const std::string& symbol) { selectedSymbol_ = symbol; }

    const core::data::ElementInfo* SelectedElementInfo() const;

    bool HasActiveUnitCell() const;
    std::array<std::array<float, 3>, 3> ActiveUnitCell() const;
    int32_t ActiveStructureId() const;
    size_t ActiveStructureAtomCount() const;

private:
    int32_t EnsureActiveStructure();
    core::scene::StructureRecord& EnsureStructureRecord(int32_t structureId);

    core::scene::SceneState& scene_;
    std::string selectedSymbol_;
};

} // namespace features::build::periodic_table
