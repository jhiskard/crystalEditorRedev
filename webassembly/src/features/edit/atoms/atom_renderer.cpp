#include "atom_renderer.h"

#include "core/data/element_database.h"
#include "core/vtk/vtk_viewer.h"

#include <vtkActor.h>
#include <vtkCoordinate.h>
#include <vtkFloatArray.h>
#include <vtkGlyph3D.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace features::edit::atoms {
namespace {

void ApplyLegacyAtomShading(vtkProperty* property) {
    if (property == nullptr) {
        return;
    }

    property->SetAmbient(0.3);
    property->SetDiffuse(0.7);
    property->SetSpecular(0.1);
    property->SetSpecularPower(10.0);
    property->SetEdgeVisibility(false);
}

} // namespace

AtomRenderer::AtomRenderer(core::scene::SceneState& scene)
    : scene_(scene) {
}

AtomRenderer::~AtomRenderer() {
    ClearAllAtomGroups();
    ClearAllSelectionShellActors();
    ClearAllAtomLabelActors();
}

bool AtomRenderer::AtomGroupVTKData::IsInitialized() const {
    return sphereSource != nullptr && inputData != nullptr && glyph != nullptr && mapper != nullptr && actor != nullptr;
}

void AtomRenderer::Subscribe() {
    if (subscribed_) {
        return;
    }
    subscribed_ = true;

    scene_.events.onAtomsChanged.Subscribe([this](const core::scene::AtomsChangedEvent& event) {
        OnAtomsChanged(event.structureId);
    });
    scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
        ClearStructure(event.structureId);
    });
    scene_.events.onSelectionChanged.Subscribe([this](const core::scene::SelectionChangedEvent&) {
        SyncSelectionShellActors();
        core::vtk::VtkViewer::Instance().RequestRender();
    });
}

void AtomRenderer::InitializeAtomGroup(const std::string& symbol, float radius) {
    if (scene_.currentStructureId < 0) {
        return;
    }
    EnsureGroupInitialized(BuildGroupKey(scene_.currentStructureId, symbol), radius);
}

void AtomRenderer::UpdateAtomGroup(int32_t structureId, const std::string& symbol) {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0) {
        return;
    }

    auto recordIt = scene_.structureRecords.find(sid);
    if (recordIt == scene_.structureRecords.end()) {
        return;
    }

    std::vector<const core::scene::AtomRecord*> atoms;
    atoms.reserve(recordIt->second.atoms.size());
    float baseRadius = 1.0f;
    for (const auto& atom : recordIt->second.atoms) {
        if (atom.symbol != symbol || !atom.visible) {
            continue;
        }
        baseRadius = std::max(baseRadius, atom.radius);
        atoms.push_back(&atom);
    }

    const std::string groupKey = BuildGroupKey(sid, symbol);
    if (atoms.empty()) {
        auto it = atomGroups_.find(groupKey);
        if (it != atomGroups_.end()) {
            if (it->second.actor != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor);
            }
            atomGroups_.erase(it);
            SyncSelectionShellActors();
        }
        return;
    }

    const core::data::ElementInfo* element = core::data::ElementDatabase::getInstance().getElementInfo(symbol);
    const core::data::Color4f baseColor =
        (element != nullptr) ? element->defaultColor : core::data::Color4f(0.7f, 0.7f, 0.7f, 1.0f);

    EnsureGroupInitialized(groupKey, baseRadius);
    UpdateGroupData(groupKey, atoms, baseColor);
}

void AtomRenderer::ClearAtomGroup(const std::string& symbol) {
    if (scene_.currentStructureId < 0) {
        return;
    }
    const std::string groupKey = BuildGroupKey(scene_.currentStructureId, symbol);
    auto it = atomGroups_.find(groupKey);
    if (it == atomGroups_.end()) {
        return;
    }

    if (it->second.actor != nullptr) {
        core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor);
    }
    atomGroups_.erase(it);
    SyncSelectionShellActors();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void AtomRenderer::ClearAllAtomGroups() {
    for (auto& [key, group] : atomGroups_) {
        (void)key;
        if (group.actor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(group.actor);
        }
    }
    atomGroups_.clear();
    ClearAllSelectionShellActors();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void AtomRenderer::SetAtomGroupVisible(const std::string& symbol, bool visible) {
    if (scene_.currentStructureId < 0) {
        return;
    }
    const std::string groupKey = BuildGroupKey(scene_.currentStructureId, symbol);
    auto it = atomGroups_.find(groupKey);
    if (it == atomGroups_.end()) {
        return;
    }
    if (it->second.actor != nullptr) {
        it->second.actor->SetVisibility(visible ? 1 : 0);
        it->second.visible = visible;
        SyncSelectionShellActors();
        core::vtk::VtkViewer::Instance().RequestRender();
    }
}

void AtomRenderer::SetAllAtomGroupsVisible(bool visible) {
    for (auto& [key, group] : atomGroups_) {
        (void)key;
        group.visible = visible;
        if (group.actor != nullptr) {
            group.actor->SetVisibility(visible ? 1 : 0);
        }
    }
    SyncSelectionShellActors();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void AtomRenderer::SyncAtomLabelActors(const std::vector<LabelActorSpec>& labels) {
    std::unordered_map<uint32_t, const LabelActorSpec*> labelLookup;
    labelLookup.reserve(labels.size());
    for (const auto& label : labels) {
        if (label.id == 0) {
            continue;
        }
        labelLookup.emplace(label.id, &label);
    }

    for (auto it = atomLabelActors_.begin(); it != atomLabelActors_.end();) {
        if (labelLookup.find(it->first) == labelLookup.end()) {
            if (it->second != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor2D(it->second);
            }
            it = atomLabelActors_.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& [labelId, spec] : labelLookup) {
        auto actorIt = atomLabelActors_.find(labelId);
        if (actorIt == atomLabelActors_.end()) {
            vtkSmartPointer<vtkTextActor> actor = vtkSmartPointer<vtkTextActor>::New();
            actor->SetInput(spec->text.c_str());
            actor->SetVisibility(spec->visible ? 1 : 0);
            if (vtkTextProperty* property = actor->GetTextProperty(); property != nullptr) {
                property->SetFontSize(14);
                property->SetColor(0.0, 0.0, 0.0);
                property->SetBackgroundColor(1.0, 1.0, 1.0);
                property->SetBackgroundOpacity(0.6);
                property->SetFrame(true);
            }
            if (vtkCoordinate* coord = actor->GetPositionCoordinate(); coord != nullptr) {
                coord->SetCoordinateSystemToWorld();
                coord->SetValue(spec->worldPosition[0], spec->worldPosition[1], spec->worldPosition[2]);
            }
            core::vtk::VtkViewer::Instance().AddActor2D(actor);
            atomLabelActors_.emplace(labelId, actor);
        } else {
            vtkTextActor* textActor = vtkTextActor::SafeDownCast(actorIt->second.GetPointer());
            if (textActor != nullptr) {
                textActor->SetInput(spec->text.c_str());
                textActor->SetVisibility(spec->visible ? 1 : 0);
                if (vtkCoordinate* coord = textActor->GetPositionCoordinate(); coord != nullptr) {
                    coord->SetCoordinateSystemToWorld();
                    coord->SetValue(spec->worldPosition[0], spec->worldPosition[1], spec->worldPosition[2]);
                }
            }
        }
    }

    core::vtk::VtkViewer::Instance().RequestRender();
}

void AtomRenderer::ClearAllAtomLabelActors() {
    for (auto& [labelId, actor] : atomLabelActors_) {
        (void)labelId;
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor2D(actor);
        }
    }
    atomLabelActors_.clear();
}

std::string AtomRenderer::BuildGroupKey(int32_t structureId, const std::string& symbol) {
    return std::to_string(structureId) + ":" + symbol;
}

std::pair<int32_t, std::string> AtomRenderer::ParseGroupKey(const std::string& key) {
    const size_t pos = key.find(':');
    if (pos == std::string::npos) {
        return std::make_pair(-1, std::string());
    }
    return std::make_pair(std::stoi(key.substr(0, pos)), key.substr(pos + 1));
}

void AtomRenderer::OnAtomsChanged(int32_t structureId) {
    if (structureId < 0) {
        for (const auto& [sid, record] : scene_.structureRecords) {
            (void)record;
            RebuildStructure(sid);
        }
        return;
    }
    RebuildStructure(structureId);
}

void AtomRenderer::RebuildStructure(int32_t structureId) {
    auto recordIt = scene_.structureRecords.find(structureId);
    if (recordIt == scene_.structureRecords.end()) {
        ClearStructure(structureId);
        return;
    }

    std::unordered_map<std::string, std::vector<const core::scene::AtomRecord*>> atomsBySymbol;
    atomsBySymbol.reserve(recordIt->second.atoms.size());
    for (const auto& atom : recordIt->second.atoms) {
        if (!atom.visible) {
            continue;
        }
        atomsBySymbol[atom.symbol].push_back(&atom);
    }

    std::unordered_set<std::string> expectedKeys;
    expectedKeys.reserve(atomsBySymbol.size());
    for (const auto& [symbol, atoms] : atomsBySymbol) {
        if (atoms.empty()) {
            continue;
        }

        const std::string key = BuildGroupKey(structureId, symbol);
        expectedKeys.insert(key);

        const core::data::ElementInfo* element = core::data::ElementDatabase::getInstance().getElementInfo(symbol);
        const core::data::Color4f baseColor =
            (element != nullptr) ? element->defaultColor : core::data::Color4f(0.7f, 0.7f, 0.7f, 1.0f);

        float maxRadius = 1.0f;
        for (const auto* atom : atoms) {
            if (atom != nullptr) {
                maxRadius = std::max(maxRadius, atom->radius);
            }
        }

        EnsureGroupInitialized(key, maxRadius);
        UpdateGroupData(key, atoms, baseColor);
    }

    for (auto it = atomGroups_.begin(); it != atomGroups_.end();) {
        const auto [sid, symbol] = ParseGroupKey(it->first);
        (void)symbol;
        if (sid == structureId && expectedKeys.find(it->first) == expectedKeys.end()) {
            if (it->second.actor != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor);
            }
            it = atomGroups_.erase(it);
        } else {
            ++it;
        }
    }

    SyncSelectionShellActors();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void AtomRenderer::ClearStructure(int32_t structureId) {
    for (auto it = atomGroups_.begin(); it != atomGroups_.end();) {
        const auto [sid, symbol] = ParseGroupKey(it->first);
        (void)symbol;
        if (sid == structureId) {
            if (it->second.actor != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor);
            }
            it = atomGroups_.erase(it);
        } else {
            ++it;
        }
    }
    SyncSelectionShellActors();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void AtomRenderer::EnsureGroupInitialized(const std::string& groupKey, float radius) {
    auto& group = atomGroups_[groupKey];
    if (group.IsInitialized()) {
        group.baseRadius = radius;
        return;
    }

    group.baseRadius = radius;
    group.visible = true;

    group.sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    group.sphereSource->SetRadius(1.0);
    group.sphereSource->SetThetaResolution(20);
    group.sphereSource->SetPhiResolution(20);
    group.sphereSource->Update();

    group.inputData = vtkSmartPointer<vtkPolyData>::New();

    group.glyph = vtkSmartPointer<vtkGlyph3D>::New();
    group.glyph->SetSourceConnection(group.sphereSource->GetOutputPort());
    group.glyph->SetInputData(group.inputData);
    group.glyph->SetScaleModeToScaleByScalar();
    group.glyph->ScalingOn();
    group.glyph->OrientOff();

    group.mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    group.mapper->SetInputConnection(group.glyph->GetOutputPort());
    group.mapper->ScalarVisibilityOff();

    group.actor = vtkSmartPointer<vtkActor>::New();
    group.actor->SetMapper(group.mapper);
    group.actor->SetPickable(true);
    group.actor->SetVisibility(group.visible ? 1 : 0);
    group.actor->GetProperty()->SetOpacity(1.0);
    ApplyLegacyAtomShading(group.actor->GetProperty());

    core::vtk::VtkViewer::Instance().AddActor(group.actor);
}

void AtomRenderer::UpdateGroupData(
    const std::string& groupKey,
    const std::vector<const core::scene::AtomRecord*>& atoms,
    const core::data::Color4f& color) {
    auto groupIt = atomGroups_.find(groupKey);
    if (groupIt == atomGroups_.end() || !groupIt->second.IsInitialized()) {
        return;
    }

    auto& group = groupIt->second;
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkFloatArray> scales = vtkSmartPointer<vtkFloatArray>::New();
    scales->SetName("atom_scales");
    scales->SetNumberOfComponents(1);

    points->SetNumberOfPoints(static_cast<vtkIdType>(atoms.size()));
    for (size_t i = 0; i < atoms.size(); ++i) {
        const core::scene::AtomRecord* atom = atoms[i];
        if (atom == nullptr) {
            continue;
        }

        points->SetPoint(
            static_cast<vtkIdType>(i),
            static_cast<double>(atom->cartesian[0]),
            static_cast<double>(atom->cartesian[1]),
            static_cast<double>(atom->cartesian[2]));

        const float radiusScale = std::max(0.001f, atom->radius * 0.5f);
        scales->InsertNextValue(radiusScale);
    }

    group.inputData->SetPoints(points);
    group.inputData->GetPointData()->SetScalars(scales);
    group.inputData->Modified();

    group.glyph->SetScaleFactor(1.0);
    group.glyph->Update();
    group.mapper->Update();
    group.actor->Modified();

    float r = std::clamp(color.r, 0.0f, 1.0f);
    float g = std::clamp(color.g, 0.0f, 1.0f);
    float b = std::clamp(color.b, 0.0f, 1.0f);

    group.actor->GetProperty()->SetColor(r, g, b);
    ApplyLegacyAtomShading(group.actor->GetProperty());
}

vtkSmartPointer<vtkActor> AtomRenderer::MakeSelectionShellActor(const core::scene::AtomRecord& atom) const {
    const double shellRadius = static_cast<double>(std::max(0.01f, atom.radius * 0.5f + 0.01f));

    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetRadius(shellRadius);
    sphere->SetThetaResolution(24);
    sphere->SetPhiResolution(24);
    sphere->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphere->GetOutputPort());
    mapper->ScalarVisibilityOff();

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPosition(
        static_cast<double>(atom.cartesian[0]),
        static_cast<double>(atom.cartesian[1]),
        static_cast<double>(atom.cartesian[2]));
    actor->SetPickable(false);
    actor->SetVisibility(1);

    if (vtkProperty* property = actor->GetProperty(); property != nullptr) {
        property->SetRepresentationToWireframe();
        property->SetColor(1.0, 1.0, 0.0);
        property->SetLineWidth(2.0);
        property->SetAmbient(1.0);
        property->SetDiffuse(0.0);
        property->SetSpecular(0.0);
        property->SetEdgeVisibility(false);
    }

    return actor;
}

void AtomRenderer::SyncSelectionShellActors() {
    ClearAllSelectionShellActors();

    for (const auto& [structureId, record] : scene_.structureRecords) {
        for (const core::scene::AtomRecord& atom : record.atoms) {
            const bool selected = atom.selected || scene_.selection.ContainsAtom(atom.id);
            if (!selected || !atom.visible || !IsAtomGroupVisible(structureId, atom.symbol)) {
                continue;
            }

            vtkSmartPointer<vtkActor> shell = MakeSelectionShellActor(atom);
            if (shell == nullptr) {
                continue;
            }

            core::vtk::VtkViewer::Instance().AddActor(shell);
            selectionShells_.emplace(atom.id, shell);
        }
    }
}

void AtomRenderer::ClearAllSelectionShellActors() {
    for (auto& [atomId, actor] : selectionShells_) {
        (void)atomId;
        if (actor != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(actor);
        }
    }
    selectionShells_.clear();
}

bool AtomRenderer::IsAtomGroupVisible(int32_t structureId, const std::string& symbol) const {
    const std::string groupKey = BuildGroupKey(structureId, symbol);
    auto it = atomGroups_.find(groupKey);
    if (it == atomGroups_.end()) {
        return true;
    }
    return it->second.visible;
}

} // namespace features::edit::atoms

