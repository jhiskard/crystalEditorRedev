/**
 * @file features/edit/atoms/atom_renderer.h
 * @brief Atom actor renderer split from legacy vtk renderer.
 */
#pragma once

#include "core/scene/scene_state.h"
#include "core/data/color.h"

#include <vtkSmartPointer.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class vtkActor;
class vtkActor2D;
class vtkGlyph3D;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkSphereSource;

namespace features::edit::atoms {

struct LabelActorSpec {
    uint32_t id = 0;
    std::string text;
    std::array<double, 3> worldPosition = {0.0, 0.0, 0.0};
    bool visible = true;
};

class AtomRenderer {
public:
    explicit AtomRenderer(core::scene::SceneState& scene);
    ~AtomRenderer();

    void Subscribe();

    void InitializeAtomGroup(const std::string& symbol, float radius);
    void UpdateAtomGroup(int32_t structureId, const std::string& symbol);
    void ClearAtomGroup(const std::string& symbol);
    void ClearAllAtomGroups();
    void SetAtomGroupVisible(const std::string& symbol, bool visible);
    void SetAllAtomGroupsVisible(bool visible);

    void SyncAtomLabelActors(const std::vector<LabelActorSpec>& labels);
    void ClearAllAtomLabelActors();

private:
    struct AtomGroupVTKData {
        vtkSmartPointer<vtkSphereSource> sphereSource;
        vtkSmartPointer<vtkPolyData> inputData;
        vtkSmartPointer<vtkGlyph3D> glyph;
        vtkSmartPointer<vtkPolyDataMapper> mapper;
        vtkSmartPointer<vtkActor> actor;
        float baseRadius = 1.0f;
        bool visible = true;

        bool IsInitialized() const;
    };

    static std::string BuildGroupKey(int32_t structureId, const std::string& symbol);
    static std::pair<int32_t, std::string> ParseGroupKey(const std::string& key);

    void OnAtomsChanged(int32_t structureId);
    void RebuildStructure(int32_t structureId);
    void ClearStructure(int32_t structureId);
    void EnsureGroupInitialized(const std::string& groupKey, float radius);
    void UpdateGroupData(
        const std::string& groupKey,
        const std::vector<const core::scene::AtomRecord*>& atoms,
        const core::data::Color4f& color);

    core::scene::SceneState& scene_;
    std::unordered_map<std::string, AtomGroupVTKData> atomGroups_;
    std::unordered_map<uint32_t, vtkSmartPointer<vtkActor2D>> atomLabelActors_;
    bool subscribed_ = false;
};

} // namespace features::edit::atoms
