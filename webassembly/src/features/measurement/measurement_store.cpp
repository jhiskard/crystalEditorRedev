#include "measurement_store.h"

#include "angle.h"
#include "center.h"
#include "dihedral.h"
#include "distance.h"

#include "core/vtk/vtk_viewer.h"

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkProperty.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace features::measurement {
namespace {

constexpr float kMinMeasurementLineWidth = 2.0f;
constexpr float kMaxMeasurementLineWidth = 18.0f;
constexpr float kMinMeasurementLabelScale = 1.0f;
constexpr float kMaxMeasurementLabelScale = 3.0f;
constexpr float kMinAngleArcRadiusScale = 0.6f;
constexpr float kMaxAngleArcRadiusScale = 2.0f;
constexpr float kMinDihedralPlaneOpacity = 0.05f;
constexpr float kMaxDihedralPlaneOpacity = 0.60f;

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

float ClampFloatRange(float value, float minValue, float maxValue) {
    return std::clamp(value, minValue, maxValue);
}

float SnapToOneDecimal(float value) {
    return std::round(value * 10.0f) / 10.0f;
}

int ScaledFontSize(int baseFontSize, float scale) {
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(baseFontSize) * scale)));
}

vtkTextProperty* TextPropertyFromActor(vtkActor2D* actor) {
    vtkTextActor* textActor = vtkTextActor::SafeDownCast(actor);
    return textActor != nullptr ? textActor->GetTextProperty() : nullptr;
}

void ApplyLineStyleToActor(vtkActor* actor, const std::array<float, 3>& color, float width) {
    if (actor == nullptr) {
        return;
    }
    vtkProperty* property = actor->GetProperty();
    if (property == nullptr) {
        return;
    }
    property->SetColor(
        static_cast<double>(color[0]),
        static_cast<double>(color[1]),
        static_cast<double>(color[2]));
    property->SetLineWidth(static_cast<double>(width));
}

void ApplyTextScaleToActor(vtkActor2D* actor, float scale, int baseFontSize) {
    vtkTextProperty* property = TextPropertyFromActor(actor);
    if (property == nullptr) {
        return;
    }
    property->SetFontSize(ScaledFontSize(baseFontSize > 0 ? baseFontSize : property->GetFontSize(), scale));
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

void MeasurementStore::ResetStyle(MeasurementMode mode) {
    switch (mode) {
    case MeasurementMode::Distance:
        distanceStyle_ = DistanceStyle {};
        break;
    case MeasurementMode::Angle:
        angleStyle_ = AngleStyle {};
        break;
    case MeasurementMode::Dihedral:
        dihedralStyle_ = DihedralStyle {};
        break;
    case MeasurementMode::GeometricCenter:
        geometricCenterStyle_ = CenterStyle {};
        break;
    case MeasurementMode::CenterOfMass:
        centerOfMassStyle_ = CenterStyle {};
        break;
    case MeasurementMode::None:
    default:
        break;
    }
}

void MeasurementStore::ApplyStyleForMode(MeasurementMode mode, bool rebuildGeometry) {
    if (mode == MeasurementMode::None) {
        return;
    }

    ClampStyles();
    const MeasurementType targetType = TypeFromMode(mode);
    bool changed = false;
    for (auto it = records_.begin(); it != records_.end();) {
        if (it->type != targetType) {
            ++it;
            continue;
        }

        if (rebuildGeometry) {
            DetachActors(*it);
            if (!BuildActors(*it)) {
                it = records_.erase(it);
                changed = true;
                continue;
            }
            AttachActors(*it);
        } else {
            ApplyStyle(*it);
            ApplyVisibility(*it);
        }
        changed = true;
        ++it;
    }

    if (changed) {
        core::vtk::VtkViewer::Instance().RequestRender();
    }
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
        if (!BuildAngleVisual(*atom1, *atom2, *atom3, visual, angleStyle_.arcRadiusScale)) {
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

    CaptureTextBaseFontSizes(record);
    ApplyStyle(record);
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

void MeasurementStore::ClampStyles() {
    auto clampColor = [](std::array<float, 3>& color) {
        for (float& channel : color) {
            channel = ClampFloatRange(channel, 0.0f, 1.0f);
        }
    };

    clampColor(distanceStyle_.lineColor);
    distanceStyle_.lineWidth = ClampFloatRange(
        distanceStyle_.lineWidth,
        kMinMeasurementLineWidth,
        kMaxMeasurementLineWidth);
    distanceStyle_.labelScale = SnapToOneDecimal(ClampFloatRange(
        distanceStyle_.labelScale,
        kMinMeasurementLabelScale,
        kMaxMeasurementLabelScale));

    clampColor(angleStyle_.lineColor);
    angleStyle_.lineWidth = ClampFloatRange(
        angleStyle_.lineWidth,
        kMinMeasurementLineWidth,
        kMaxMeasurementLineWidth);
    angleStyle_.arcRadiusScale = SnapToOneDecimal(ClampFloatRange(
        angleStyle_.arcRadiusScale,
        kMinAngleArcRadiusScale,
        kMaxAngleArcRadiusScale));
    angleStyle_.labelScale = SnapToOneDecimal(ClampFloatRange(
        angleStyle_.labelScale,
        kMinMeasurementLabelScale,
        kMaxMeasurementLabelScale));

    clampColor(dihedralStyle_.baseLineColor);
    dihedralStyle_.baseLineWidth = ClampFloatRange(
        dihedralStyle_.baseLineWidth,
        kMinMeasurementLineWidth,
        kMaxMeasurementLineWidth);
    clampColor(dihedralStyle_.helperPlane1Color);
    dihedralStyle_.helperPlane1Opacity = ClampFloatRange(
        dihedralStyle_.helperPlane1Opacity,
        kMinDihedralPlaneOpacity,
        kMaxDihedralPlaneOpacity);
    clampColor(dihedralStyle_.helperPlane2Color);
    dihedralStyle_.helperPlane2Opacity = ClampFloatRange(
        dihedralStyle_.helperPlane2Opacity,
        kMinDihedralPlaneOpacity,
        kMaxDihedralPlaneOpacity);
    dihedralStyle_.labelScale = SnapToOneDecimal(ClampFloatRange(
        dihedralStyle_.labelScale,
        kMinMeasurementLabelScale,
        kMaxMeasurementLabelScale));

    auto clampCenterStyle = [&](CenterStyle& style) {
        clampColor(style.centerPlusColor);
        style.centerPlusScale = SnapToOneDecimal(ClampFloatRange(
            style.centerPlusScale,
            kMinMeasurementLabelScale,
            kMaxMeasurementLabelScale));
        clampColor(style.selectedPlusColor);
        style.selectedPlusScale = SnapToOneDecimal(ClampFloatRange(
            style.selectedPlusScale,
            kMinMeasurementLabelScale,
            kMaxMeasurementLabelScale));
        style.coordinateLabelScale = SnapToOneDecimal(ClampFloatRange(
            style.coordinateLabelScale,
            kMinMeasurementLabelScale,
            kMaxMeasurementLabelScale));
    };
    clampCenterStyle(geometricCenterStyle_);
    clampCenterStyle(centerOfMassStyle_);
}

void MeasurementStore::CaptureTextBaseFontSizes(MeasurementRecord& record) {
    record.textBaseFontSizes.clear();
    record.textBaseFontSizes.reserve(record.textActors.size());
    for (const auto& actor : record.textActors) {
        vtkTextProperty* property = TextPropertyFromActor(actor.GetPointer());
        record.textBaseFontSizes.push_back(property != nullptr ? property->GetFontSize() : 0);
    }
}

void MeasurementStore::ApplyStyle(MeasurementRecord& record) {
    switch (record.type) {
    case MeasurementType::Distance:
        if (!record.actors.empty()) {
            ApplyLineStyleToActor(record.actors[0].GetPointer(), distanceStyle_.lineColor, distanceStyle_.lineWidth);
        }
        if (!record.textActors.empty()) {
            const int baseFontSize = !record.textBaseFontSizes.empty() ? record.textBaseFontSizes[0] : 21;
            ApplyTextScaleToActor(record.textActors[0].GetPointer(), distanceStyle_.labelScale, baseFontSize);
        }
        break;
    case MeasurementType::Angle:
        for (size_t i = 0; i < std::min<size_t>(record.actors.size(), 3); ++i) {
            ApplyLineStyleToActor(record.actors[i].GetPointer(), angleStyle_.lineColor, angleStyle_.lineWidth);
        }
        if (!record.textActors.empty()) {
            const int baseFontSize = !record.textBaseFontSizes.empty() ? record.textBaseFontSizes[0] : 21;
            ApplyTextScaleToActor(record.textActors[0].GetPointer(), angleStyle_.labelScale, baseFontSize);
        }
        break;
    case MeasurementType::Dihedral:
        for (size_t i = 0; i < std::min<size_t>(record.actors.size(), 3); ++i) {
            ApplyLineStyleToActor(
                record.actors[i].GetPointer(),
                dihedralStyle_.baseLineColor,
                dihedralStyle_.baseLineWidth);
        }
        if (record.actors.size() > 3 && record.actors[3] != nullptr) {
            if (vtkProperty* property = record.actors[3]->GetProperty(); property != nullptr) {
                property->SetColor(
                    static_cast<double>(dihedralStyle_.helperPlane1Color[0]),
                    static_cast<double>(dihedralStyle_.helperPlane1Color[1]),
                    static_cast<double>(dihedralStyle_.helperPlane1Color[2]));
                property->SetOpacity(static_cast<double>(dihedralStyle_.helperPlane1Opacity));
                property->SetRepresentationToSurface();
                property->SetEdgeVisibility(false);
            }
        }
        if (record.actors.size() > 4 && record.actors[4] != nullptr) {
            if (vtkProperty* property = record.actors[4]->GetProperty(); property != nullptr) {
                property->SetColor(
                    static_cast<double>(dihedralStyle_.helperPlane2Color[0]),
                    static_cast<double>(dihedralStyle_.helperPlane2Color[1]),
                    static_cast<double>(dihedralStyle_.helperPlane2Color[2]));
                property->SetOpacity(static_cast<double>(dihedralStyle_.helperPlane2Opacity));
                property->SetRepresentationToSurface();
                property->SetEdgeVisibility(false);
            }
        }
        if (!record.textActors.empty()) {
            const int baseFontSize = !record.textBaseFontSizes.empty() ? record.textBaseFontSizes[0] : 21;
            ApplyTextScaleToActor(record.textActors[0].GetPointer(), dihedralStyle_.labelScale, baseFontSize);
        }
        break;
    case MeasurementType::GeometricCenter:
    case MeasurementType::CenterOfMass: {
        const CenterStyle& style = record.type == MeasurementType::CenterOfMass
            ? centerOfMassStyle_
            : geometricCenterStyle_;

        if (!record.textActors.empty()) {
            if (vtkTextProperty* property = TextPropertyFromActor(record.textActors[0].GetPointer());
                property != nullptr) {
                property->SetColor(
                    static_cast<double>(style.centerPlusColor[0]),
                    static_cast<double>(style.centerPlusColor[1]),
                    static_cast<double>(style.centerPlusColor[2]));
            }
            const int baseFontSize = !record.textBaseFontSizes.empty() ? record.textBaseFontSizes[0] : 21;
            ApplyTextScaleToActor(record.textActors[0].GetPointer(), style.centerPlusScale, baseFontSize);
        }
        if (record.textActors.size() > 1) {
            const int baseFontSize = record.textBaseFontSizes.size() > 1 ? record.textBaseFontSizes[1] : 21;
            ApplyTextScaleToActor(record.textActors[1].GetPointer(), style.coordinateLabelScale, baseFontSize);
        }
        for (size_t i = 2; i < record.textActors.size(); ++i) {
            if (vtkTextProperty* property = TextPropertyFromActor(record.textActors[i].GetPointer());
                property != nullptr) {
                property->SetColor(
                    static_cast<double>(style.selectedPlusColor[0]),
                    static_cast<double>(style.selectedPlusColor[1]),
                    static_cast<double>(style.selectedPlusColor[2]));
            }
            const int baseFontSize = i < record.textBaseFontSizes.size() ? record.textBaseFontSizes[i] : 21;
            ApplyTextScaleToActor(record.textActors[i].GetPointer(), style.selectedPlusScale, baseFontSize);
        }
        break;
    }
    default:
        break;
    }
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
