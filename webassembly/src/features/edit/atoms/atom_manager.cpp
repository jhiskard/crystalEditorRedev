#include "atom_manager.h"

#include "../../build/periodic_table/periodic_table.h"

#include "core/data/element_database.h"

#include <algorithm>
#include <unordered_set>
#include <utility>
#include <vector>

namespace features::edit::atoms {
namespace {

core::scene::AtomRecord* FindAtomById(core::scene::StructureRecord& record, uint32_t atomId) {
    auto it = std::find_if(record.atoms.begin(), record.atoms.end(), [atomId](const core::scene::AtomRecord& atom) {
        return atom.id == atomId;
    });
    if (it == record.atoms.end()) {
        return nullptr;
    }
    return &(*it);
}

} // namespace

AtomManager::AtomManager(core::scene::SceneState& scene)
    : scene_(scene) {
}

int AtomManager::Add(int32_t structureId, const std::string& symbol, const std::array<float, 3>& position) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return -1;
    }

    core::scene::StructureRecord& record = scene_.structureRecords[sid];

    const core::data::ElementInfo* elementInfo = core::data::ElementDatabase::getInstance().getElementInfo(symbol);
    if (elementInfo == nullptr) {
        return -1;
    }

    core::scene::AtomRecord atom;
    atom.id = scene_.nextAtomId++;
    atom.symbol = symbol;
    atom.radius = std::max(0.001f, elementInfo->covalentRadius);
    atom.cartesian = position;
    atom.group = "Default";
    atom.visible = true;
    atom.selected = false;

    if (record.cell.hasCell) {
        atom.fractional = features::build::periodic_table::CartesianToFractional(position, record.cell.matrix);
    }

    record.atoms.push_back(atom);
    EmitAtomsChanged(sid);
    return static_cast<int>(atom.id);
}

bool AtomManager::Remove(int32_t structureId, uint32_t atomId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }

    core::scene::StructureRecord& record = recordIt->second;
    auto eraseIt = std::remove_if(record.atoms.begin(), record.atoms.end(), [atomId](const core::scene::AtomRecord& atom) {
        return atom.id == atomId;
    });
    if (eraseIt == record.atoms.end()) {
        return false;
    }

    record.atoms.erase(eraseIt, record.atoms.end());
    scene_.selection.RemoveAtom(atomId);
    EmitAtomsChanged(sid);
    return true;
}

bool AtomManager::Move(int32_t structureId, uint32_t atomId, const std::array<float, 3>& position) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }

    core::scene::AtomRecord* atom = FindAtomById(recordIt->second, atomId);
    if (atom == nullptr) {
        return false;
    }

    atom->cartesian = position;
    if (recordIt->second.cell.hasCell) {
        atom->fractional = features::build::periodic_table::CartesianToFractional(position, recordIt->second.cell.matrix);
    }

    EmitAtomsChanged(sid);
    return true;
}

bool AtomManager::MoveFractional(int32_t structureId, uint32_t atomId, const std::array<float, 3>& fractional) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }
    if (!recordIt->second.cell.hasCell) {
        return false;
    }

    core::scene::AtomRecord* atom = FindAtomById(recordIt->second, atomId);
    if (atom == nullptr) {
        return false;
    }

    atom->fractional = fractional;
    atom->cartesian = features::build::periodic_table::FractionalToCartesian(fractional, recordIt->second.cell.matrix);
    EmitAtomsChanged(sid);
    return true;
}

bool AtomManager::SetVisible(int32_t structureId, uint32_t atomId, bool visible) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }

    core::scene::AtomRecord* atom = FindAtomById(recordIt->second, atomId);
    if (atom == nullptr) {
        return false;
    }

    atom->visible = visible;
    EmitAtomsChanged(sid);
    return true;
}

bool AtomManager::SetSelected(int32_t structureId, uint32_t atomId, bool selected) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }

    core::scene::AtomRecord* atom = FindAtomById(recordIt->second, atomId);
    if (atom == nullptr) {
        return false;
    }

    if (atom->selected == selected) {
        return false;
    }
    atom->selected = selected;
    if (selected) {
        scene_.selection.AddAtom(atomId);
    } else {
        scene_.selection.RemoveAtom(atomId);
    }
    return true;
}

int AtomManager::SetSelectionByIds(
    int32_t structureId,
    const std::unordered_set<uint32_t>& selectedIds,
    bool additive) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return 0;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return 0;
    }

    std::unordered_set<uint32_t> nextSelection = scene_.selection.Atoms();
    int changed = 0;
    for (auto& atom : recordIt->second.atoms) {
        const bool selectedByBatch = selectedIds.find(atom.id) != selectedIds.end();
        const bool shouldSelect = additive ? (atom.selected || selectedByBatch) : selectedByBatch;
        if (atom.selected == shouldSelect) {
            continue;
        }

        atom.selected = shouldSelect;
        if (shouldSelect) {
            nextSelection.insert(atom.id);
        } else {
            nextSelection.erase(atom.id);
        }
        ++changed;
    }

    if (changed > 0) {
        scene_.selection.SetAtoms(std::move(nextSelection));
    }
    return changed;
}

bool AtomManager::SetSymbol(int32_t structureId, uint32_t atomId, const std::string& symbol) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }

    const core::data::ElementInfo* elementInfo = core::data::ElementDatabase::getInstance().getElementInfo(symbol);
    if (elementInfo == nullptr) {
        return false;
    }

    core::scene::AtomRecord* atom = FindAtomById(recordIt->second, atomId);
    if (atom == nullptr) {
        return false;
    }

    atom->symbol = symbol;
    atom->radius = std::max(0.001f, elementInfo->covalentRadius);
    EmitAtomsChanged(sid);
    return true;
}

bool AtomManager::SetRadius(int32_t structureId, uint32_t atomId, float radius) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }

    core::scene::AtomRecord* atom = FindAtomById(recordIt->second, atomId);
    if (atom == nullptr) {
        return false;
    }

    atom->radius = std::max(0.001f, radius);
    EmitAtomsChanged(sid);
    return true;
}

bool AtomManager::SetGroup(int32_t structureId, uint32_t atomId, const std::string& group) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return false;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return false;
    }

    core::scene::AtomRecord* atom = FindAtomById(recordIt->second, atomId);
    if (atom == nullptr) {
        return false;
    }

    atom->group = group.empty() ? std::string("Default") : group;
    EmitAtomsChanged(sid);
    return true;
}

int AtomManager::RemoveSelected(int32_t structureId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return 0;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return 0;
    }

    core::scene::StructureRecord& record = recordIt->second;
    const size_t before = record.atoms.size();

    std::vector<uint32_t> selectedIds;
    selectedIds.reserve(record.atoms.size());
    for (const auto& atom : record.atoms) {
        if (atom.selected) {
            selectedIds.push_back(atom.id);
        }
    }

    record.atoms.erase(
        std::remove_if(record.atoms.begin(), record.atoms.end(), [](const core::scene::AtomRecord& atom) {
            return atom.selected;
        }),
        record.atoms.end());

    for (uint32_t atomId : selectedIds) {
        scene_.selection.RemoveAtom(atomId);
    }

    const int removed = static_cast<int>(before - record.atoms.size());
    if (removed > 0) {
        EmitAtomsChanged(sid);
    }
    return removed;
}

int AtomManager::TranslateSelected(int32_t structureId, const std::array<float, 3>& delta, bool fractionalSpace) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return 0;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return 0;
    }

    core::scene::StructureRecord& record = recordIt->second;
    int changed = 0;
    for (auto& atom : record.atoms) {
        if (!atom.selected) {
            continue;
        }

        if (fractionalSpace && record.cell.hasCell) {
            atom.fractional[0] += delta[0];
            atom.fractional[1] += delta[1];
            atom.fractional[2] += delta[2];
            atom.cartesian = features::build::periodic_table::FractionalToCartesian(atom.fractional, record.cell.matrix);
        } else {
            atom.cartesian[0] += delta[0];
            atom.cartesian[1] += delta[1];
            atom.cartesian[2] += delta[2];
            if (record.cell.hasCell) {
                atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, record.cell.matrix);
            }
        }
        ++changed;
    }

    if (changed > 0) {
        EmitAtomsChanged(sid);
    }
    return changed;
}

int AtomManager::SetRadiusSelected(int32_t structureId, float radius) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return 0;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return 0;
    }

    const float clamped = std::max(0.001f, radius);
    int changed = 0;
    for (auto& atom : recordIt->second.atoms) {
        if (!atom.selected) {
            continue;
        }
        atom.radius = clamped;
        ++changed;
    }

    if (changed > 0) {
        EmitAtomsChanged(sid);
    }
    return changed;
}

void AtomManager::SelectAll(int32_t structureId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return;
    }

    std::unordered_set<uint32_t> nextSelection = scene_.selection.Atoms();
    bool changed = false;
    for (auto& atom : recordIt->second.atoms) {
        if (!atom.selected) {
            atom.selected = true;
            nextSelection.insert(atom.id);
            changed = true;
        }
    }

    if (changed) {
        scene_.selection.SetAtoms(std::move(nextSelection));
    }
}

void AtomManager::SelectNone(int32_t structureId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return;
    }

    std::unordered_set<uint32_t> nextSelection = scene_.selection.Atoms();
    bool changed = false;
    for (auto& atom : recordIt->second.atoms) {
        if (atom.selected) {
            atom.selected = false;
            nextSelection.erase(atom.id);
            changed = true;
        }
    }

    if (changed) {
        scene_.selection.SetAtoms(std::move(nextSelection));
    }
}

void AtomManager::InvertSelection(int32_t structureId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return;
    }

    std::unordered_set<uint32_t> nextSelection = scene_.selection.Atoms();
    bool changed = false;
    for (auto& atom : recordIt->second.atoms) {
        const bool selected = !atom.selected;
        atom.selected = selected;
        if (selected) {
            nextSelection.insert(atom.id);
        } else {
            nextSelection.erase(atom.id);
        }
        changed = true;
    }

    if (changed) {
        scene_.selection.SetAtoms(std::move(nextSelection));
    }
}

void AtomManager::OnCellChanged(int32_t structureId, bool preserveFractional) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end() || !recordIt->second.cell.hasCell) {
        return;
    }

    auto& record = recordIt->second;
    for (auto& atom : record.atoms) {
        if (preserveFractional) {
            atom.cartesian = features::build::periodic_table::FractionalToCartesian(atom.fractional, record.cell.matrix);
        } else {
            atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, record.cell.matrix);
        }
    }

    EmitAtomsChanged(sid);
}

int32_t AtomManager::ResolveStructureId(int32_t structureId) const {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0 || !scene_.structures.Exists(sid)) {
        return -1;
    }
    return sid;
}

void AtomManager::EmitAtomsChanged(int32_t structureId) const {
    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});
}

} // namespace features::edit::atoms
