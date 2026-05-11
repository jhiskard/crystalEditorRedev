#include "bond_renderer.h"

#include "core/data/element_database.h"
#include "core/vtk/vtk_viewer.h"

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCellData.h>
#include <vtkCylinderSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnsignedCharArray.h>

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace features::edit::bonds {
namespace {

vtkSmartPointer<vtkPolyData> CreateCylinderGeometry(float radius) {
    vtkSmartPointer<vtkCylinderSource> cylinder = vtkSmartPointer<vtkCylinderSource>::New();
    cylinder->SetRadius(radius);
    cylinder->SetHeight(1.0);
    cylinder->SetResolution(20);
    cylinder->SetCenter(0.0, 0.0, 0.0);
    cylinder->Update();
    return cylinder->GetOutput();
}

vtkSmartPointer<vtkTransform> BuildSegmentTransform(
    const std::array<float, 3>& pointA,
    const std::array<float, 3>& pointB) {
    const double ax = static_cast<double>(pointA[0]);
    const double ay = static_cast<double>(pointA[1]);
    const double az = static_cast<double>(pointA[2]);
    const double bx = static_cast<double>(pointB[0]);
    const double by = static_cast<double>(pointB[1]);
    const double bz = static_cast<double>(pointB[2]);

    const double dx = bx - ax;
    const double dy = by - ay;
    const double dz = bz - az;
    const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (length < 1e-6) {
        return nullptr;
    }

    const double dirX = dx / length;
    const double dirY = dy / length;
    const double dirZ = dz / length;

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->PostMultiply();
    transform->Identity();
    transform->Translate((ax + bx) * 0.5, (ay + by) * 0.5, (az + bz) * 0.5);

    const double dot = std::clamp(dirY, -1.0, 1.0);
    const double angleDeg = std::acos(dot) * 180.0 / 3.14159265358979323846;

    double axisX = dirZ;
    double axisY = 0.0;
    double axisZ = -dirX;
    const double axisLen = std::sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);

    if (axisLen > 1e-6) {
        axisX /= axisLen;
        axisY /= axisLen;
        axisZ /= axisLen;
        transform->RotateWXYZ(angleDeg, axisX, axisY, axisZ);
    } else if (dot < 0.0) {
        transform->RotateWXYZ(180.0, 1.0, 0.0, 0.0);
    }

    transform->Scale(1.0, length, 1.0);
    return transform;
}

std::array<float, 3> ResolveColor(const std::string& symbol) {
    const core::data::ElementInfo* info = core::data::ElementDatabase::getInstance().getElementInfo(symbol);
    if (info == nullptr) {
        return {0.7f, 0.7f, 0.7f};
    }
    return {
        std::clamp(info->defaultColor.r, 0.0f, 1.0f),
        std::clamp(info->defaultColor.g, 0.0f, 1.0f),
        std::clamp(info->defaultColor.b, 0.0f, 1.0f),
    };
}

} // namespace

BondRenderer::BondRenderer(core::scene::SceneState& scene)
    : scene_(scene) {
}

BondRenderer::~BondRenderer() {
    ClearAllBondGroups();
}

bool BondRenderer::BondGroupVTKData::IsInitialized() const {
    return baseGeometry != nullptr && appender1 != nullptr && appender2 != nullptr && mapper1 != nullptr &&
           mapper2 != nullptr && actor1 != nullptr && actor2 != nullptr;
}

void BondRenderer::Subscribe() {
    if (subscribed_) {
        return;
    }
    subscribed_ = true;

    scene_.events.onBondsChanged.Subscribe([this](const core::scene::BondsChangedEvent& event) {
        OnBondsChanged(event.structureId);
    });
    scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
        ClearStructure(event.structureId);
    });
}

void BondRenderer::InitializeBondGroup(const std::string& bondTypeKey, float radius) {
    if (scene_.currentStructureId < 0) {
        return;
    }
    EnsureGroupInitialized(BuildGroupKey(scene_.currentStructureId, bondTypeKey), radius);
}

void BondRenderer::UpdateBondGroup(int32_t structureId, const std::string& bondTypeKey) {
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

    std::vector<BondGeometryInput> geometry;
    float groupRadius = 0.1f;
    for (const auto& bond : recordIt->second.bonds) {
        if (bond.typeKey != bondTypeKey || !bond.visible) {
            continue;
        }

        const core::scene::AtomRecord* atom1 = FindAtomById(recordIt->second, bond.atomId1);
        const core::scene::AtomRecord* atom2 = FindAtomById(recordIt->second, bond.atomId2);
        if (atom1 == nullptr || atom2 == nullptr) {
            continue;
        }

        auto transforms = BuildHalfBondTransforms(atom1->cartesian, atom2->cartesian);
        if (transforms.first == nullptr || transforms.second == nullptr) {
            continue;
        }

        BondGeometryInput input;
        input.transform1 = transforms.first;
        input.transform2 = transforms.second;
        input.color1 = ResolveColor(atom1->symbol);
        input.color2 = ResolveColor(atom2->symbol);
        geometry.push_back(input);
        groupRadius = std::max(groupRadius, bond.radius);
    }

    const std::string groupKey = BuildGroupKey(sid, bondTypeKey);
    EnsureGroupInitialized(groupKey, groupRadius);
    UpdateGroupData(groupKey, geometry);
    core::vtk::VtkViewer::Instance().RequestRender();
}

void BondRenderer::ClearBondGroup(const std::string& bondTypeKey) {
    if (scene_.currentStructureId < 0) {
        return;
    }
    const std::string groupKey = BuildGroupKey(scene_.currentStructureId, bondTypeKey);
    auto it = bondGroups_.find(groupKey);
    if (it == bondGroups_.end()) {
        return;
    }

    if (it->second.actor1 != nullptr) {
        core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor1);
    }
    if (it->second.actor2 != nullptr) {
        core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor2);
    }
    bondGroups_.erase(it);
    core::vtk::VtkViewer::Instance().RequestRender();
}

void BondRenderer::ClearAllBondGroups() {
    for (auto& [groupKey, group] : bondGroups_) {
        (void)groupKey;
        if (group.actor1 != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(group.actor1);
        }
        if (group.actor2 != nullptr) {
            core::vtk::VtkViewer::Instance().RemoveActor(group.actor2);
        }
    }
    bondGroups_.clear();
    core::vtk::VtkViewer::Instance().RequestRender();
}

void BondRenderer::SetBondGroupVisible(const std::string& bondTypeKey, bool visible) {
    for (auto& [groupKey, group] : bondGroups_) {
        const auto [sid, key] = ParseGroupKey(groupKey);
        (void)sid;
        if (key != bondTypeKey) {
            continue;
        }
        group.visible = visible;
        if (group.actor1 != nullptr) {
            group.actor1->SetVisibility(visible ? 1 : 0);
        }
        if (group.actor2 != nullptr) {
            group.actor2->SetVisibility(visible ? 1 : 0);
        }
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

void BondRenderer::SetAllBondGroupsVisible(bool visible) {
    for (auto& [groupKey, group] : bondGroups_) {
        (void)groupKey;
        group.visible = visible;
        if (group.actor1 != nullptr) {
            group.actor1->SetVisibility(visible ? 1 : 0);
        }
        if (group.actor2 != nullptr) {
            group.actor2->SetVisibility(visible ? 1 : 0);
        }
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

void BondRenderer::UpdateAllBondGroupThickness(float thickness) {
    globalThickness_ = std::max(0.05f, thickness);
    for (auto& [groupKey, group] : bondGroups_) {
        (void)groupKey;
        if (!group.IsInitialized()) {
            continue;
        }
        group.baseGeometry = CreateCylinderGeometry(group.baseRadius * globalThickness_);
    }

    for (const auto& [sid, record] : scene_.structureRecords) {
        (void)record;
        RebuildStructure(sid);
    }
}

void BondRenderer::UpdateAllBondGroupOpacity(float opacity) {
    globalOpacity_ = std::clamp(opacity, 0.05f, 1.0f);

    for (auto& [groupKey, group] : bondGroups_) {
        (void)groupKey;
        if (group.actor1 != nullptr) {
            group.actor1->GetProperty()->SetOpacity(globalOpacity_);
            group.actor1->Modified();
        }
        if (group.actor2 != nullptr) {
            group.actor2->GetProperty()->SetOpacity(globalOpacity_);
            group.actor2->Modified();
        }
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

std::string BondRenderer::BuildGroupKey(int32_t structureId, const std::string& bondTypeKey) {
    return std::to_string(structureId) + ":" + bondTypeKey;
}

std::pair<int32_t, std::string> BondRenderer::ParseGroupKey(const std::string& groupKey) {
    const size_t pos = groupKey.find(':');
    if (pos == std::string::npos) {
        return std::make_pair(-1, std::string());
    }
    return std::make_pair(std::stoi(groupKey.substr(0, pos)), groupKey.substr(pos + 1));
}

std::pair<vtkSmartPointer<vtkTransform>, vtkSmartPointer<vtkTransform>> BondRenderer::BuildHalfBondTransforms(
    const std::array<float, 3>& pointA,
    const std::array<float, 3>& pointB) {
    const std::array<float, 3> mid = {
        (pointA[0] + pointB[0]) * 0.5f,
        (pointA[1] + pointB[1]) * 0.5f,
        (pointA[2] + pointB[2]) * 0.5f,
    };

    vtkSmartPointer<vtkTransform> t1 = BuildSegmentTransform(pointA, mid);
    vtkSmartPointer<vtkTransform> t2 = BuildSegmentTransform(mid, pointB);
    return std::make_pair(t1, t2);
}

const core::scene::AtomRecord* BondRenderer::FindAtomById(
    const core::scene::StructureRecord& record,
    uint32_t atomId) const {
    auto it = std::find_if(record.atoms.begin(), record.atoms.end(), [atomId](const core::scene::AtomRecord& atom) {
        return atom.id == atomId;
    });
    if (it == record.atoms.end()) {
        return nullptr;
    }
    return &(*it);
}

void BondRenderer::OnBondsChanged(int32_t structureId) {
    if (structureId < 0) {
        for (const auto& [sid, record] : scene_.structureRecords) {
            (void)record;
            RebuildStructure(sid);
        }
        return;
    }
    RebuildStructure(structureId);
}

void BondRenderer::RebuildStructure(int32_t structureId) {
    auto recordIt = scene_.structureRecords.find(structureId);
    if (recordIt == scene_.structureRecords.end()) {
        ClearStructure(structureId);
        return;
    }

    std::unordered_map<std::string, std::vector<BondGeometryInput>> groupedGeometry;
    std::unordered_map<std::string, float> groupRadii;
    groupedGeometry.reserve(recordIt->second.bonds.size());
    groupRadii.reserve(recordIt->second.bonds.size());

    for (const auto& bond : recordIt->second.bonds) {
        const std::string& bondTypeKey = bond.typeKey;
        if (bondTypeKey.empty() || !bond.visible) {
            continue;
        }

        const core::scene::AtomRecord* atom1 = FindAtomById(recordIt->second, bond.atomId1);
        const core::scene::AtomRecord* atom2 = FindAtomById(recordIt->second, bond.atomId2);
        if (atom1 == nullptr || atom2 == nullptr) {
            continue;
        }

        auto transforms = BuildHalfBondTransforms(atom1->cartesian, atom2->cartesian);
        if (transforms.first == nullptr || transforms.second == nullptr) {
            continue;
        }

        BondGeometryInput input;
        input.transform1 = transforms.first;
        input.transform2 = transforms.second;
        input.color1 = ResolveColor(atom1->symbol);
        input.color2 = ResolveColor(atom2->symbol);

        groupedGeometry[bondTypeKey].push_back(input);
        groupRadii[bondTypeKey] = std::max(groupRadii[bondTypeKey], std::max(0.001f, bond.radius));
    }

    std::unordered_set<std::string> expectedKeys;
    expectedKeys.reserve(groupedGeometry.size());
    for (auto& [bondTypeKey, geometry] : groupedGeometry) {
        const std::string groupKey = BuildGroupKey(structureId, bondTypeKey);
        expectedKeys.insert(groupKey);
        const float radius = std::max(0.001f, groupRadii[bondTypeKey]);
        EnsureGroupInitialized(groupKey, radius);
        UpdateGroupData(groupKey, geometry);
    }

    for (auto it = bondGroups_.begin(); it != bondGroups_.end();) {
        const auto [sid, key] = ParseGroupKey(it->first);
        (void)key;
        if (sid == structureId && expectedKeys.find(it->first) == expectedKeys.end()) {
            if (it->second.actor1 != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor1);
            }
            if (it->second.actor2 != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor2);
            }
            it = bondGroups_.erase(it);
        } else {
            ++it;
        }
    }

    core::vtk::VtkViewer::Instance().RequestRender();
}

void BondRenderer::ClearStructure(int32_t structureId) {
    for (auto it = bondGroups_.begin(); it != bondGroups_.end();) {
        const auto [sid, key] = ParseGroupKey(it->first);
        (void)key;
        if (sid == structureId) {
            if (it->second.actor1 != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor1);
            }
            if (it->second.actor2 != nullptr) {
                core::vtk::VtkViewer::Instance().RemoveActor(it->second.actor2);
            }
            it = bondGroups_.erase(it);
        } else {
            ++it;
        }
    }
    core::vtk::VtkViewer::Instance().RequestRender();
}

void BondRenderer::EnsureGroupInitialized(const std::string& groupKey, float radius) {
    auto& group = bondGroups_[groupKey];
    group.baseRadius = std::max(0.001f, radius);

    if (group.IsInitialized()) {
        return;
    }

    group.baseGeometry = CreateCylinderGeometry(group.baseRadius * globalThickness_);
    group.appender1 = vtkSmartPointer<vtkAppendPolyData>::New();
    group.appender2 = vtkSmartPointer<vtkAppendPolyData>::New();

    vtkSmartPointer<vtkPolyData> empty1 = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPolyData> empty2 = vtkSmartPointer<vtkPolyData>::New();
    group.appender1->AddInputData(empty1);
    group.appender2->AddInputData(empty2);

    group.mapper1 = vtkSmartPointer<vtkPolyDataMapper>::New();
    group.mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    group.mapper1->SetInputConnection(group.appender1->GetOutputPort());
    group.mapper2->SetInputConnection(group.appender2->GetOutputPort());
    group.mapper1->SetScalarModeToUseCellData();
    group.mapper2->SetScalarModeToUseCellData();
    group.mapper1->ScalarVisibilityOn();
    group.mapper2->ScalarVisibilityOn();

    group.actor1 = vtkSmartPointer<vtkActor>::New();
    group.actor2 = vtkSmartPointer<vtkActor>::New();
    group.actor1->SetMapper(group.mapper1);
    group.actor2->SetMapper(group.mapper2);
    group.actor1->SetPickable(false);
    group.actor2->SetPickable(false);
    group.actor1->GetProperty()->SetOpacity(globalOpacity_);
    group.actor2->GetProperty()->SetOpacity(globalOpacity_);
    group.actor1->SetVisibility(group.visible ? 1 : 0);
    group.actor2->SetVisibility(group.visible ? 1 : 0);

    core::vtk::VtkViewer::Instance().AddActor(group.actor1);
    core::vtk::VtkViewer::Instance().AddActor(group.actor2);
}

void BondRenderer::UpdateGroupData(const std::string& groupKey, const std::vector<BondGeometryInput>& geometry) {
    auto groupIt = bondGroups_.find(groupKey);
    if (groupIt == bondGroups_.end() || !groupIt->second.IsInitialized()) {
        return;
    }

    BondGroupVTKData& group = groupIt->second;
    group.appender1->RemoveAllInputs();
    group.appender2->RemoveAllInputs();

    vtkSmartPointer<vtkUnsignedCharArray> colors1 = vtkSmartPointer<vtkUnsignedCharArray>::New();
    vtkSmartPointer<vtkUnsignedCharArray> colors2 = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors1->SetNumberOfComponents(3);
    colors2->SetNumberOfComponents(3);
    colors1->SetName((groupKey + "_colors1").c_str());
    colors2->SetName((groupKey + "_colors2").c_str());

    if (geometry.empty()) {
        vtkSmartPointer<vtkPolyData> empty1 = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPolyData> empty2 = vtkSmartPointer<vtkPolyData>::New();
        group.appender1->AddInputData(empty1);
        group.appender2->AddInputData(empty2);
    } else {
        for (const BondGeometryInput& input : geometry) {
            if (input.transform1 == nullptr || input.transform2 == nullptr) {
                continue;
            }

            vtkSmartPointer<vtkTransformPolyDataFilter> filter1 = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            filter1->SetInputData(group.baseGeometry);
            filter1->SetTransform(input.transform1);
            filter1->Update();

            vtkSmartPointer<vtkTransformPolyDataFilter> filter2 = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            filter2->SetInputData(group.baseGeometry);
            filter2->SetTransform(input.transform2);
            filter2->Update();

            vtkSmartPointer<vtkPolyData> transformed1 = filter1->GetOutput();
            vtkSmartPointer<vtkPolyData> transformed2 = filter2->GetOutput();
            if (transformed1 == nullptr || transformed2 == nullptr) {
                continue;
            }

            group.appender1->AddInputData(transformed1);
            group.appender2->AddInputData(transformed2);

            const unsigned char r1 = static_cast<unsigned char>(std::clamp(input.color1[0], 0.0f, 1.0f) * 255.0f);
            const unsigned char g1 = static_cast<unsigned char>(std::clamp(input.color1[1], 0.0f, 1.0f) * 255.0f);
            const unsigned char b1 = static_cast<unsigned char>(std::clamp(input.color1[2], 0.0f, 1.0f) * 255.0f);
            const unsigned char r2 = static_cast<unsigned char>(std::clamp(input.color2[0], 0.0f, 1.0f) * 255.0f);
            const unsigned char g2 = static_cast<unsigned char>(std::clamp(input.color2[1], 0.0f, 1.0f) * 255.0f);
            const unsigned char b2 = static_cast<unsigned char>(std::clamp(input.color2[2], 0.0f, 1.0f) * 255.0f);

            const vtkIdType cells1 = transformed1->GetNumberOfCells();
            const vtkIdType cells2 = transformed2->GetNumberOfCells();
            for (vtkIdType i = 0; i < cells1; ++i) {
                colors1->InsertNextTuple3(r1, g1, b1);
            }
            for (vtkIdType i = 0; i < cells2; ++i) {
                colors2->InsertNextTuple3(r2, g2, b2);
            }
        }
    }

    group.appender1->Update();
    group.appender2->Update();

    if (vtkPolyData* output1 = group.appender1->GetOutput(); output1 != nullptr && colors1->GetNumberOfTuples() > 0) {
        output1->GetCellData()->SetScalars(colors1);
    }
    if (vtkPolyData* output2 = group.appender2->GetOutput(); output2 != nullptr && colors2->GetNumberOfTuples() > 0) {
        output2->GetCellData()->SetScalars(colors2);
    }

    group.mapper1->Update();
    group.mapper2->Update();
    group.actor1->GetProperty()->SetOpacity(globalOpacity_);
    group.actor2->GetProperty()->SetOpacity(globalOpacity_);
    group.actor1->SetVisibility(group.visible ? 1 : 0);
    group.actor2->SetVisibility(group.visible ? 1 : 0);
    group.actor1->Modified();
    group.actor2->Modified();
}

} // namespace features::edit::bonds
