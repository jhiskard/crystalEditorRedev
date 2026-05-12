#include "measurement_store.h"

#include "angle.h"
#include "center.h"
#include "dihedral.h"
#include "distance.h"

#include "core/vtk/vtk_viewer.h"

#include <vtkActor.h>
#include <vtkActor2D.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace features::measurement {
namespace {

bool AllUnique(const std::vector<uint32_t>& atomIds) {
    std::unordered_set<uint32_t> unique;
    unique.reserve(atomIds.size());
    for (uint32_t atomId : atomIds) {
        if (atomId == 0 || !unique.insert(atomId).second) {
            return false;
        }
    }
    return true;
}

} // namespace

MeasurementStore::MeasurementStore(core::scene::SceneState& scene)
    : scene_(scene) {
}

MeasurementStore::~MeasurementStore() {
    Clear();
}

void MeasurementStore::Subscribe() {
    if (subscribed_) {
        return;
    }
    subscribed_ = true;

    scene_.events.onAtomsChanged.Subscribe([this](const core::scene::AtomsChangedEvent& event) {
        RefreshStructure(event.structureId);
    });
    scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
        RemoveByStructure(event.structureId);
    });
    scene_.events.onStructureVisibilityChanged.Subscribe(
        [this](const core::scene::StructureVisibilityChangedEvent& event) {
            RefreshStructure(event.structureId);
        });
}

uint32_t MeasurementStore::AddDistance(int32_t structureId, uint32_t atomId1, uint32_t atomId2) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0 || atomId1 == atomId2) {
        return 0;
    }

    MeasurementRecord record;
    record.id = nextMeasurementId_++;
    record.structureId = sid;
    record.type = MeasurementType::Distance;
    record.atomIds = {atomId1, atomId2};
    record.visible = true;

    if (!AllUnique(record.atomIds) || !HasAllAtoms(record) || !BuildActors(record)) {
        return 0;
    }
    AttachActors(record);
    records_.push_back(std::move(record));
    core::vtk::VtkViewer::Instance().RequestRender();
    return records_.back().id;
}

uint32_t MeasurementStore::AddAngle(int32_t structureId, uint32_t atomId1, uint32_t atomId2, uint32_t atomId3) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return 0;
    }

    MeasurementRecord record;
    record.id = nextMeasurementId_++;
    record.structureId = sid;
    record.type = MeasurementType::Angle;
    record.atomIds = {atomId1, atomId2, atomId3};
    record.visible = true;

    if (!AllUnique(record.atomIds) || !HasAllAtoms(record) || !BuildActors(record)) {
        return 0;
    }
    AttachActors(record);
    records_.push_back(std::move(record));
    core::vtk::VtkViewer::Instance().RequestRender();
    return records_.back().id;
}

uint32_t MeasurementStore::AddDihedral(
    int32_t structureId,
    uint32_t atomId1,
    uint32_t atomId2,
    uint32_t atomId3,
    uint32_t atomId4) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return 0;
    }

    MeasurementRecord record;
    record.id = nextMeasurementId_++;
    record.structureId = sid;
    record.type = MeasurementType::Dihedral;
    record.atomIds = {atomId1, atomId2, atomId3, atomId4};
    record.visible = true;

    if (!AllUnique(record.atomIds) || !HasAllAtoms(record) || !BuildActors(record)) {
        return 0;
    }
    AttachActors(record);
    records_.push_back(std::move(record));
    core::vtk::VtkViewer::Instance().RequestRender();
    return records_.back().id;
}

uint32_t MeasurementStore::AddCenter(
    int32_t structureId,
    MeasurementType type,
    const std::vector<uint32_t>& atomIds) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0 ||
        (type != MeasurementType::GeometricCenter && type != MeasurementType::CenterOfMass) ||
        atomIds.size() < 2) {
        return 0;
    }

    MeasurementRecord record;
    record.id = nextMeasurementId_++;
    record.structureId = sid;
    record.type = type;
    record.atomIds = atomIds;
    record.visible = true;

    if (!AllUnique(record.atomIds) || !HasAllAtoms(record) || !BuildActors(record)) {
        return 0;
    }
    AttachActors(record);
    records_.push_back(std::move(record));
    core::vtk::VtkViewer::Instance().RequestRender();
    return records_.back().id;
}

bool MeasurementStore::SetVisible(uint32_t measurementId, bool visible) {
    auto it = std::find_if(records_.begin(), records_.end(), [measurementId](const MeasurementRecord& record) {
        return record.id == measurementId;
    });
    if (it == records_.end()) {
        return false;
    }

    it->visible = visible;
    ApplyVisibility(*it);
    core::vtk::VtkViewer::Instance().RequestRender();
    return true;
}

bool MeasurementStore::Remove(uint32_t measurementId) {
    auto it = std::find_if(records_.begin(), records_.end(), [measurementId](const MeasurementRecord& record) {
        return record.id == measurementId;
    });
    if (it == records_.end()) {
        return false;
    }

    DetachActors(*it);
    records_.erase(it);
    core::vtk::VtkViewer::Instance().RequestRender();
    return true;
}

int MeasurementStore::RemoveByStructure(int32_t structureId) {
    int removed = 0;
    for (auto it = records_.begin(); it != records_.end();) {
        if (it->structureId == structureId) {
            DetachActors(*it);
            it = records_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    if (removed > 0) {
        core::vtk::VtkViewer::Instance().RequestRender();
    }
    return removed;
}

void MeasurementStore::Clear() {
    for (MeasurementRecord& record : records_) {
        DetachActors(record);
    }
    records_.clear();
    core::vtk::VtkViewer::Instance().RequestRender();
}

std::vector<MeasurementListItem> MeasurementStore::MeasurementsForStructure(int32_t structureId) const {
    const int32_t sid = ResolveStructureId(structureId);
    std::vector<MeasurementListItem> result;
    for (const MeasurementRecord& record : records_) {
        if (sid >= 0 && record.structureId != sid) {
            continue;
        }
        result.push_back(MeasurementListItem{
            record.id,
            record.structureId,
            record.type,
            record.displayName,
            record.visible,
        });
    }
    return result;
}

const core::scene::AtomRecord* MeasurementStore::FindAtom(int32_t structureId, uint32_t atomId) const {
    auto recordIt = scene_.structureRecords.find(structureId);
    if (recordIt == scene_.structureRecords.end()) {
        return nullptr;
    }
    const auto& atoms = recordIt->second.atoms;
    auto atomIt = std::find_if(atoms.begin(), atoms.end(), [atomId](const core::scene::AtomRecord& atom) {
        return atom.id == atomId;
    });
    return atomIt != atoms.end() ? &(*atomIt) : nullptr;
}

int32_t MeasurementStore::ResolveStructureId(int32_t structureId) const {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0 || !scene_.structures.Exists(sid)) {
        return -1;
    }
    return sid;
}

bool MeasurementStore::BuildActors(MeasurementRecord& record) {
    record.actors.clear();
    record.textActors.clear();

    switch (record.type) {
    case MeasurementType::Distance: {
        if (record.atomIds.size() != 2) {
            return false;
        }
        const core::scene::AtomRecord* atom1 = FindAtom(record.structureId, record.atomIds[0]);
        const core::scene::AtomRecord* atom2 = FindAtom(record.structureId, record.atomIds[1]);
        if (atom1 == nullptr || atom2 == nullptr) {
            return false;
        }

        DistanceVisual visual = BuildDistanceVisual(*atom1, *atom2);
        record.value = visual.distance;
        record.displayName = std::string("Distance (") +
            BuildAtomLabel(record.structureId, atom1->id) + "-" +
            BuildAtomLabel(record.structureId, atom2->id) + "): " +
            visual.valueText;
        record.actors.push_back(visual.lineActor);
        record.textActors.push_back(visual.textActor);
        break;
    }
    case MeasurementType::Angle: {
        if (record.atomIds.size() != 3) {
            return false;
        }
        const core::scene::AtomRecord* atom1 = FindAtom(record.structureId, record.atomIds[0]);
        const core::scene::AtomRecord* atom2 = FindAtom(record.structureId, record.atomIds[1]);
        const core::scene::AtomRecord* atom3 = FindAtom(record.structureId, record.atomIds[2]);
        if (atom1 == nullptr || atom2 == nullptr || atom3 == nullptr) {
            return false;
        }

        AngleVisual visual;
        if (!BuildAngleVisual(*atom1, *atom2, *atom3, visual)) {
            return false;
        }
        record.value = visual.angleDeg;
        record.displayName = std::string("Angle (") +
            BuildAtomLabel(record.structureId, atom1->id) + "-" +
            BuildAtomLabel(record.structureId, atom2->id) + "-" +
            BuildAtomLabel(record.structureId, atom3->id) + "): " +
            visual.valueText;
        record.actors.push_back(visual.lineActor12);
        record.actors.push_back(visual.lineActor23);
        record.actors.push_back(visual.arcActor);
        record.textActors.push_back(visual.textActor);
        break;
    }
    case MeasurementType::Dihedral: {
        if (record.atomIds.size() != 4) {
            return false;
        }
        const core::scene::AtomRecord* atom1 = FindAtom(record.structureId, record.atomIds[0]);
        const core::scene::AtomRecord* atom2 = FindAtom(record.structureId, record.atomIds[1]);
        const core::scene::AtomRecord* atom3 = FindAtom(record.structureId, record.atomIds[2]);
        const core::scene::AtomRecord* atom4 = FindAtom(record.structureId, record.atomIds[3]);
        if (atom1 == nullptr || atom2 == nullptr || atom3 == nullptr || atom4 == nullptr) {
            return false;
        }

        DihedralVisual visual;
        if (!BuildDihedralVisual(*atom1, *atom2, *atom3, *atom4, visual)) {
            return false;
        }
        record.value = visual.dihedralDeg;
        record.displayName = std::string("Dihedral (") +
            BuildAtomLabel(record.structureId, atom1->id) + "-" +
            BuildAtomLabel(record.structureId, atom2->id) + "-" +
            BuildAtomLabel(record.structureId, atom3->id) + "-" +
            BuildAtomLabel(record.structureId, atom4->id) + "): " +
            visual.valueText;
        record.actors.push_back(visual.lineActor12);
        record.actors.push_back(visual.lineActor23);
        record.actors.push_back(visual.lineActor34);
        record.actors.push_back(visual.helperPlaneActor1);
        record.actors.push_back(visual.helperPlaneActor2);
        record.actors.push_back(visual.helperArcActor);
        record.textActors.push_back(visual.textActor);
        break;
    }
    case MeasurementType::GeometricCenter:
    case MeasurementType::CenterOfMass: {
        std::vector<const core::scene::AtomRecord*> atoms;
        atoms.reserve(record.atomIds.size());
        for (uint32_t atomId : record.atomIds) {
            const core::scene::AtomRecord* atom = FindAtom(record.structureId, atomId);
            if (atom == nullptr) {
                return false;
            }
            atoms.push_back(atom);
        }

        CenterVisual visual;
        if (!BuildCenterVisual(record.type, atoms, visual)) {
            return false;
        }
        record.center = visual.center;
        record.displayName = BuildCenterDisplayName(record.type, record.structureId, record.atomIds, record.center);
        record.actors = std::move(visual.actors);
        record.textActors = std::move(visual.textActors);
        break;
    }
    default:
        return false;
    }

    ApplyVisibility(record);
    return true;
}

bool MeasurementStore::HasAllAtoms(const MeasurementRecord& record) const {
    if (record.structureId < 0) {
        return false;
    }
    for (uint32_t atomId : record.atomIds) {
        if (FindAtom(record.structureId, atomId) == nullptr) {
            return false;
        }
    }
    return true;
}

bool MeasurementStore::EffectiveVisible(const MeasurementRecord& record) const {
    if (!record.visible || record.structureId < 0 || !scene_.structures.IsVisible(record.structureId)) {
        return false;
    }
    for (uint32_t atomId : record.atomIds) {
        const core::scene::AtomRecord* atom = FindAtom(record.structureId, atomId);
        if (atom == nullptr || !atom->visible) {
            return false;
        }
    }
    return true;
}

void MeasurementStore::ApplyVisibility(MeasurementRecord& record) {
    const int visible = EffectiveVisible(record) ? 1 : 0;
    for (auto& actor : record.actors) {
        if (actor != nullptr) {
            actor->SetVisibility(visible);
        }
    }
    for (auto& actor : record.textActors) {
        if (actor != nullptr) {
            actor->SetVisibility(visible);
        }
    }
}

void MeasurementStore::AttachActors(MeasurementRecord& record) {
    ApplyVisibility(record);
    for (auto& actor : record.actors) {
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().AddActor(actor);
        }
    }
    for (auto& actor : record.textActors) {
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().AddActor2D(actor);
        }
    }
}

void MeasurementStore::DetachActors(MeasurementRecord& record) {
    for (auto& actor : record.actors) {
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(actor);
        }
    }
    for (auto& actor : record.textActors) {
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor2D(actor);
        }
    }
    record.actors.clear();
    record.textActors.clear();
}

void MeasurementStore::RefreshStructure(int32_t structureId) {
    for (auto it = records_.begin(); it != records_.end();) {
        if (structureId >= 0 && it->structureId != structureId) {
            ++it;
            continue;
        }

        DetachActors(*it);
        if (!HasAllAtoms(*it) || !BuildActors(*it)) {
            it = records_.erase(it);
            continue;
        }
        AttachActors(*it);
        ++it;
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

std::string MeasurementStore::BuildAtomLabel(int32_t structureId, uint32_t atomId) const {
    auto recordIt = scene_.structureRecords.find(structureId);
    if (recordIt == scene_.structureRecords.end()) {
        return "?#?";
    }

    const auto& atoms = recordIt->second.atoms;
    for (size_t index = 0; index < atoms.size(); ++index) {
        if (atoms[index].id == atomId) {
            return atoms[index].symbol + "#" + std::to_string(index + 1);
        }
    }
    return "?#?";
}

std::string MeasurementStore::BuildCenterDisplayName(
    MeasurementType type,
    int32_t structureId,
    const std::vector<uint32_t>& atomIds,
    const std::array<double, 3>& center) const {
    constexpr size_t kPreviewLimit = 3;
    std::ostringstream oss;
    oss << TypeLabel(type) << " (" << atomIds.size() << " atoms: ";
    const size_t previewCount = std::min(atomIds.size(), kPreviewLimit);
    for (size_t i = 0; i < previewCount; ++i) {
        if (i > 0) {
            oss << "-";
        }
        oss << BuildAtomLabel(structureId, atomIds[i]);
    }
    if (atomIds.size() > previewCount) {
        oss << "-...";
    }
    oss << ") ";
    oss << std::fixed << std::setprecision(4)
        << "(" << center[0] << ", " << center[1] << ", " << center[2] << ")";
    return oss.str();
}

} // namespace features::measurement
