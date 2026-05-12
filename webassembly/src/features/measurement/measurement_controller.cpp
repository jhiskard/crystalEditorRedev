#include "measurement_controller.h"

#include "core/scene/scene_state.h"
#include "core/vtk/mouse_interactor.h"
#include "core/vtk/vtk_viewer.h"

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCoordinate.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <unordered_set>
#include <utility>

namespace features::measurement {
namespace {

std::array<double, 3> PickColor(size_t order) {
    switch (order) {
    case 1:
        return {1.0, 0.92, 0.16};
    case 2:
        return {0.2, 0.95, 1.0};
    case 3:
        return {1.0, 0.35, 0.85};
    default:
        return {0.65, 1.0, 0.4};
    }
}

double DistanceSquared(const std::array<double, 3>& lhs, const std::array<float, 3>& rhs) {
    const double dx = lhs[0] - static_cast<double>(rhs[0]);
    const double dy = lhs[1] - static_cast<double>(rhs[1]);
    const double dz = lhs[2] - static_cast<double>(rhs[2]);
    return dx * dx + dy * dy + dz * dz;
}

vtkSmartPointer<vtkActor> CreatePickShellActor(const core::scene::AtomRecord& atom, size_t order) {
    vtkSmartPointer<vtkSphereSource> source = vtkSmartPointer<vtkSphereSource>::New();
    source->SetCenter(atom.cartesian[0], atom.cartesian[1], atom.cartesian[2]);
    source->SetRadius(std::max(0.06, static_cast<double>(atom.radius) * 0.60 + 0.03));
    source->SetThetaResolution(24);
    source->SetPhiResolution(24);
    source->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(source->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPickable(false);
    if (vtkProperty* property = actor->GetProperty(); property != nullptr) {
        const auto color = PickColor(order);
        property->SetColor(color[0], color[1], color[2]);
        property->SetRepresentationToWireframe();
        property->SetLineWidth(2.0);
        property->SetAmbient(1.0);
        property->SetDiffuse(0.0);
    }
    return actor;
}

vtkSmartPointer<vtkActor2D> CreatePickLabelActor(const core::scene::AtomRecord& atom, size_t order) {
    vtkSmartPointer<vtkTextActor> actor = vtkSmartPointer<vtkTextActor>::New();
    const std::string text = "#" + std::to_string(order);
    actor->SetInput(text.c_str());
    actor->SetPickable(false);

    if (vtkTextProperty* property = actor->GetTextProperty(); property != nullptr) {
        const auto color = PickColor(order);
        property->SetFontSize(28);
        property->SetColor(color[0], color[1], color[2]);
        property->SetBackgroundColor(0.0, 0.0, 0.0);
        property->SetBackgroundOpacity(0.45);
        property->SetFrame(true);
        property->SetFrameColor(color[0], color[1], color[2]);
        property->SetJustificationToCentered();
        property->SetVerticalJustificationToCentered();
    }

    if (vtkCoordinate* coord = actor->GetPositionCoordinate(); coord != nullptr) {
        coord->SetCoordinateSystemToWorld();
        coord->SetValue(
            atom.cartesian[0],
            atom.cartesian[1],
            atom.cartesian[2] + std::max(0.08f, atom.radius * 0.75f));
    }
    return actor;
}

} // namespace

MeasurementController::MeasurementController(core::scene::SceneState& scene, MeasurementStore& store)
    : scene_(scene)
    , store_(store) {
}

MeasurementController::~MeasurementController() {
    ClearPickVisuals();
}

void MeasurementController::SubscribeEvents() {
    if (subscribedEvents_) {
        return;
    }
    subscribedEvents_ = true;

    scene_.events.onAtomPicked.Subscribe([this](const core::scene::AtomPickedEvent& event) {
        HandleAtomPicked(event);
    });
    scene_.events.onEmptyClick.Subscribe([this](const core::scene::EmptyClickEvent& event) {
        HandleEmptyClick(event);
    });
    scene_.events.onDragSelection.Subscribe([this](const core::scene::DragSelectionEvent& event) {
        HandleDragSelection(event);
    });
    scene_.events.onAtomsChanged.Subscribe([this](const core::scene::AtomsChangedEvent& event) {
        HandleAtomsChanged(event);
    });
    scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
        HandleStructureRemoved(event);
    });
    scene_.events.onSelectionChanged.Subscribe([this](const core::scene::SelectionChangedEvent&) {
        if (IsActive()) {
            SyncPickVisuals();
        }
    });
}

void MeasurementController::Subscribe(core::vtk::MouseInteractor& mouseInteractor) {
    mouseInteractor_ = &mouseInteractor;
    mouseInteractor_->SetEventBus(&scene_.events);
    mouseInteractor_->SetRenderRequestHandler([]() {
        core::vtk::VtkViewer::Instance().RequestRender();
    });
    mouseInteractor_->SetActiveStructureId(ActiveStructureId());
}

void MeasurementController::EnterMode(MeasurementMode mode) {
    if (mode == MeasurementMode::None) {
        ExitMode();
        return;
    }

    if (mode_ == mode) {
        ClearPickedAtoms();
        return;
    }

    mode_ = mode;
    pickedAtomIds_.clear();
    ClearPickVisuals();
    if (mouseInteractor_ != nullptr) {
        mouseInteractor_->SetActiveStructureId(ActiveStructureId());
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

void MeasurementController::ExitMode() {
    if (mode_ == MeasurementMode::None && pickedAtomIds_.empty()) {
        return;
    }
    mode_ = MeasurementMode::None;
    pickedAtomIds_.clear();
    ClearPickVisuals();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void MeasurementController::ClearPickedAtoms() {
    pickedAtomIds_.clear();
    ClearPickVisuals();
    core::vtk::VtkViewer::Instance().RequestRender();
}

bool MeasurementController::ApplyCenterMeasurement() {
    if (!IsCenterMode(mode_)) {
        return false;
    }
    const bool created = CreateMeasurementFromPickedAtoms();
    if (created) {
        ClearPickedAtoms();
    }
    return created;
}

void MeasurementController::HandleAtomPicked(const core::scene::AtomPickedEvent& event) {
    if (!IsActive()) {
        return;
    }

    int32_t structureId = -1;
    const core::scene::AtomRecord* atom = ResolvePickedAtom(event, structureId);
    if (atom == nullptr || structureId < 0) {
        HandleEmptyClick(core::scene::EmptyClickEvent{event.structureId, event.screenX, event.screenY});
        return;
    }

    AddPickedAtom(structureId, atom->id);
}

void MeasurementController::HandleEmptyClick(const core::scene::EmptyClickEvent& /*event*/) {
    if (!IsActive()) {
        return;
    }
    if (!pickedAtomIds_.empty()) {
        ClearPickedAtoms();
    }
}

void MeasurementController::HandleDragSelection(const core::scene::DragSelectionEvent& event) {
    if (!IsCenterMode(mode_)) {
        return;
    }

    const std::vector<uint32_t> atomIds = CollectAtomsInRect(event);
    if (!event.additive) {
        pickedAtomIds_.clear();
    }

    int32_t activeStructure = event.structureId >= 0 ? event.structureId : ActiveStructureId();
    for (uint32_t atomId : atomIds) {
        AddPickedAtom(activeStructure, atomId);
    }
    SyncPickVisuals();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void MeasurementController::HandleAtomsChanged(const core::scene::AtomsChangedEvent& event) {
    if (!IsActive() || pickedAtomIds_.empty()) {
        return;
    }

    const int32_t sid = event.structureId >= 0 ? event.structureId : ActiveStructureId();
    if (sid != ActiveStructureId()) {
        return;
    }
    pickedAtomIds_.erase(
        std::remove_if(
            pickedAtomIds_.begin(),
            pickedAtomIds_.end(),
            [this, sid](uint32_t atomId) {
                return store_.FindAtom(sid, atomId) == nullptr;
            }),
        pickedAtomIds_.end());
    SyncPickVisuals();
}

void MeasurementController::HandleStructureRemoved(const core::scene::StructureRemovedEvent& event) {
    if (event.structureId == scene_.currentStructureId) {
        ExitMode();
    }
}

int32_t MeasurementController::ActiveStructureId() const {
    if (scene_.currentStructureId < 0 || !scene_.structures.Exists(scene_.currentStructureId)) {
        return -1;
    }
    return scene_.currentStructureId;
}

const core::scene::AtomRecord* MeasurementController::ResolvePickedAtom(
    const core::scene::AtomPickedEvent& event,
    int32_t& structureId) const {
    const int32_t sid = event.structureId >= 0 ? event.structureId : ActiveStructureId();
    structureId = sid;
    if (sid < 0 || !scene_.structures.IsVisible(sid)) {
        return nullptr;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return nullptr;
    }

    const core::scene::AtomRecord* bestAtom = nullptr;
    double bestDist2 = 0.0;
    for (const core::scene::AtomRecord& atom : recordIt->second.atoms) {
        if (!atom.visible) {
            continue;
        }
        const double dist2 = DistanceSquared(event.pickPosition, atom.cartesian);
        const double threshold = std::max(0.25, static_cast<double>(atom.radius) * 1.5);
        if (dist2 > threshold * threshold) {
            continue;
        }
        if (bestAtom == nullptr || dist2 < bestDist2) {
            bestAtom = &atom;
            bestDist2 = dist2;
        }
    }
    return bestAtom;
}

std::vector<uint32_t> MeasurementController::CollectAtomsInRect(
    const core::scene::DragSelectionEvent& event) const {
    std::vector<uint32_t> atomIds;
    const int32_t sid = event.structureId >= 0 ? event.structureId : ActiveStructureId();
    if (sid < 0 || !scene_.structures.IsVisible(sid)) {
        return atomIds;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return atomIds;
    }

    vtkRenderer* renderer = core::vtk::VtkViewer::Instance().GetRenderer();
    if (renderer == nullptr) {
        return atomIds;
    }

    const int minX = std::min(event.x0, event.x1);
    const int maxX = std::max(event.x0, event.x1);
    const int minY = std::min(event.y0, event.y1);
    const int maxY = std::max(event.y0, event.y1);

    std::unordered_set<uint32_t> uniqueIds;
    for (const core::scene::AtomRecord& atom : recordIt->second.atoms) {
        if (!atom.visible || atom.id == 0) {
            continue;
        }

        renderer->SetWorldPoint(atom.cartesian[0], atom.cartesian[1], atom.cartesian[2], 1.0);
        renderer->WorldToDisplay();
        const double* display = renderer->GetDisplayPoint();
        if (display == nullptr || !std::isfinite(display[0]) || !std::isfinite(display[1]) ||
            !std::isfinite(display[2]) || display[2] < 0.0 || display[2] > 1.0) {
            continue;
        }

        if (display[0] < static_cast<double>(minX) || display[0] > static_cast<double>(maxX) ||
            display[1] < static_cast<double>(minY) || display[1] > static_cast<double>(maxY)) {
            continue;
        }

        if (uniqueIds.insert(atom.id).second) {
            atomIds.push_back(atom.id);
        }
    }
    return atomIds;
}

void MeasurementController::AddPickedAtom(int32_t structureId, uint32_t atomId) {
    if (!IsActive() || atomId == 0 || store_.FindAtom(structureId, atomId) == nullptr) {
        return;
    }

    const bool centerMode = IsCenterMode(mode_);
    const size_t targetCount = TargetPickCount(mode_);
    if (!centerMode && targetCount == 0) {
        return;
    }

    if (!centerMode && pickedAtomIds_.size() >= targetCount) {
        pickedAtomIds_.clear();
    }

    if (!pickedAtomIds_.empty()) {
        const uint32_t firstId = pickedAtomIds_.front();
        if (store_.FindAtom(structureId, firstId) == nullptr) {
            pickedAtomIds_.clear();
        }
    }

    if (std::find(pickedAtomIds_.begin(), pickedAtomIds_.end(), atomId) != pickedAtomIds_.end()) {
        SyncPickVisuals();
        return;
    }

    if (centerMode || pickedAtomIds_.size() < targetCount) {
        pickedAtomIds_.push_back(atomId);
    }

    SyncPickVisuals();
    if (!centerMode && pickedAtomIds_.size() == targetCount) {
        if (CreateMeasurementFromPickedAtoms()) {
            pickedAtomIds_.clear();
            ClearPickVisuals();
        }
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

bool MeasurementController::CreateMeasurementFromPickedAtoms() {
    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return false;
    }

    switch (mode_) {
    case MeasurementMode::Distance:
        return pickedAtomIds_.size() == 2 &&
            store_.AddDistance(sid, pickedAtomIds_[0], pickedAtomIds_[1]) != 0;
    case MeasurementMode::Angle:
        return pickedAtomIds_.size() == 3 &&
            store_.AddAngle(sid, pickedAtomIds_[0], pickedAtomIds_[1], pickedAtomIds_[2]) != 0;
    case MeasurementMode::Dihedral:
        return pickedAtomIds_.size() == 4 &&
            store_.AddDihedral(
                sid,
                pickedAtomIds_[0],
                pickedAtomIds_[1],
                pickedAtomIds_[2],
                pickedAtomIds_[3]) != 0;
    case MeasurementMode::GeometricCenter:
    case MeasurementMode::CenterOfMass:
        return pickedAtomIds_.size() >= 2 &&
            store_.AddCenter(sid, TypeFromMode(mode_), pickedAtomIds_) != 0;
    case MeasurementMode::None:
    default:
        return false;
    }
}

void MeasurementController::ClearPickVisuals() {
    for (PickVisual& visual : pickVisuals_) {
        if (visual.shellActor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(visual.shellActor);
        }
        if (visual.labelActor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor2D(visual.labelActor);
        }
    }
    pickVisuals_.clear();
}

void MeasurementController::SyncPickVisuals() {
    ClearPickVisuals();
    if (!IsActive()) {
        return;
    }

    const int32_t sid = ActiveStructureId();
    if (sid < 0) {
        return;
    }

    std::vector<uint32_t> validIds;
    validIds.reserve(pickedAtomIds_.size());
    for (size_t index = 0; index < pickedAtomIds_.size(); ++index) {
        const core::scene::AtomRecord* atom = store_.FindAtom(sid, pickedAtomIds_[index]);
        if (atom == nullptr || !atom->visible) {
            continue;
        }
        validIds.push_back(atom->id);

        PickVisual visual;
        visual.atomId = atom->id;
        visual.shellActor = CreatePickShellActor(*atom, index + 1);
        visual.labelActor = IsCenterMode(mode_) ? nullptr : CreatePickLabelActor(*atom, index + 1);
        if (visual.shellActor != nullptr) {
            core::vtk::VtkViewer::Instance().AddActor(visual.shellActor);
        }
        if (visual.labelActor != nullptr) {
            core::vtk::VtkViewer::Instance().AddActor2D(visual.labelActor);
        }
        pickVisuals_.push_back(std::move(visual));
    }
    pickedAtomIds_ = std::move(validIds);
}

} // namespace features::measurement
