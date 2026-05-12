/**
 * @file features/measurement/measurement_store.h
 * @brief Measurement lifecycle, visibility, and actor ownership.
 */
#pragma once

#include "measurement_mode.h"

#include "core/scene/scene_state.h"

#include <vtkSmartPointer.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class vtkActor;
class vtkActor2D;

namespace features::measurement {

struct MeasurementListItem {
    uint32_t id = 0;
    int32_t structureId = -1;
    MeasurementType type = MeasurementType::Distance;
    std::string displayName;
    bool visible = true;
};

struct MeasurementRecord {
    uint32_t id = 0;
    int32_t structureId = -1;
    MeasurementType type = MeasurementType::Distance;
    std::vector<uint32_t> atomIds;
    std::string displayName;
    double value = 0.0;
    std::array<double, 3> center = {0.0, 0.0, 0.0};
    bool visible = true;
    std::vector<vtkSmartPointer<vtkActor>> actors;
    std::vector<vtkSmartPointer<vtkActor2D>> textActors;
};

class MeasurementStore {
public:
    explicit MeasurementStore(core::scene::SceneState& scene);
    ~MeasurementStore();

    void Subscribe();

    uint32_t AddDistance(int32_t structureId, uint32_t atomId1, uint32_t atomId2);
    uint32_t AddAngle(int32_t structureId, uint32_t atomId1, uint32_t atomId2, uint32_t atomId3);
    uint32_t AddDihedral(
        int32_t structureId,
        uint32_t atomId1,
        uint32_t atomId2,
        uint32_t atomId3,
        uint32_t atomId4);
    uint32_t AddCenter(int32_t structureId, MeasurementType type, const std::vector<uint32_t>& atomIds);

    bool SetVisible(uint32_t measurementId, bool visible);
    bool Remove(uint32_t measurementId);
    int RemoveByStructure(int32_t structureId);
    void Clear();

    std::vector<MeasurementListItem> MeasurementsForStructure(int32_t structureId) const;
    const std::vector<MeasurementRecord>& Records() const { return records_; }

    const core::scene::AtomRecord* FindAtom(int32_t structureId, uint32_t atomId) const;

private:
    int32_t ResolveStructureId(int32_t structureId) const;
    bool BuildActors(MeasurementRecord& record);
    bool HasAllAtoms(const MeasurementRecord& record) const;
    bool EffectiveVisible(const MeasurementRecord& record) const;
    void ApplyVisibility(MeasurementRecord& record);
    void AttachActors(MeasurementRecord& record);
    void DetachActors(MeasurementRecord& record);
    void RefreshStructure(int32_t structureId);
    std::string BuildAtomLabel(int32_t structureId, uint32_t atomId) const;
    std::string BuildCenterDisplayName(
        MeasurementType type,
        int32_t structureId,
        const std::vector<uint32_t>& atomIds,
        const std::array<double, 3>& center) const;

    core::scene::SceneState& scene_;
    std::vector<MeasurementRecord> records_;
    uint32_t nextMeasurementId_ = 1;
    bool subscribed_ = false;
};

} // namespace features::measurement
