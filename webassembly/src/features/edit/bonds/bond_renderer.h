/**
 * @file features/edit/bonds/bond_renderer.h
 * @brief Bond actor renderer for edit feature scenes.
 */
#pragma once

#include "core/scene/scene_state.h"

#include <vtkSmartPointer.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class vtkActor;
class vtkAppendPolyData;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkTransform;

namespace features::edit::bonds {

class BondRenderer {
public:
    explicit BondRenderer(core::scene::SceneState& scene);
    ~BondRenderer();

    void Subscribe();

    void InitializeBondGroup(const std::string& bondTypeKey, float radius);
    void UpdateBondGroup(int32_t structureId, const std::string& bondTypeKey);
    void ClearBondGroup(const std::string& bondTypeKey);
    void ClearAllBondGroups();
    void SetBondGroupVisible(const std::string& bondTypeKey, bool visible);
    void SetAllBondGroupsVisible(bool visible);
    void UpdateAllBondGroupThickness(float thickness);
    void UpdateAllBondGroupOpacity(float opacity);

private:
    struct BondGroupVTKData {
        vtkSmartPointer<vtkPolyData> baseGeometry;
        vtkSmartPointer<vtkAppendPolyData> appender1;
        vtkSmartPointer<vtkAppendPolyData> appender2;
        vtkSmartPointer<vtkPolyDataMapper> mapper1;
        vtkSmartPointer<vtkPolyDataMapper> mapper2;
        vtkSmartPointer<vtkActor> actor1;
        vtkSmartPointer<vtkActor> actor2;
        float baseRadius = 0.1f;
        bool visible = true;

        bool IsInitialized() const;
    };

    struct BondGeometryInput {
        vtkSmartPointer<vtkTransform> transform1;
        vtkSmartPointer<vtkTransform> transform2;
        std::array<float, 3> color1 = {0.7f, 0.7f, 0.7f};
        std::array<float, 3> color2 = {0.7f, 0.7f, 0.7f};
    };

    static std::string BuildGroupKey(int32_t structureId, const std::string& bondTypeKey);
    static std::pair<int32_t, std::string> ParseGroupKey(const std::string& groupKey);
    static std::pair<vtkSmartPointer<vtkTransform>, vtkSmartPointer<vtkTransform>> BuildHalfBondTransforms(
        const std::array<float, 3>& pointA,
        const std::array<float, 3>& pointB,
        float radius1,
        float radius2);

    const core::scene::AtomRecord* FindAtomById(const core::scene::StructureRecord& record, uint32_t atomId) const;
    void OnBondsChanged(int32_t structureId);
    void RebuildStructure(int32_t structureId);
    void ClearStructure(int32_t structureId);
    void EnsureGroupInitialized(const std::string& groupKey, float radius);
    void UpdateGroupData(const std::string& groupKey, const std::vector<BondGeometryInput>& geometry);

    core::scene::SceneState& scene_;
    std::unordered_map<std::string, BondGroupVTKData> bondGroups_;
    float globalThickness_ = 1.0f;
    float globalOpacity_ = 1.0f;
    bool subscribed_ = false;
};

} // namespace features::edit::bonds
