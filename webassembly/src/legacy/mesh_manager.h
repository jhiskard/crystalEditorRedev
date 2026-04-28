// Manage mesh tree and the whole mesh data
#pragma once

#include "macro/singleton_macro.h"
#include "lcrs_tree.h"
#include "mesh.h"

// Standard library
#include <unordered_map>
#include <vector>

class LcrsTreUPtr;
class vtkDataSet;


class MeshManager {
    DECLARE_SINGLETON(MeshManager)

public:
    // ========================================================================
    // ✅ 추가: Volume Display Settings 구조체
    // ========================================================================
    struct VolumeDisplaySettings {
        Mesh::VolumeRenderMode renderMode { Mesh::VolumeRenderMode::Surface };
        bool volumeVisibility { true };
        Mesh::VolumeColorPreset colorPreset { Mesh::VolumeColorPreset::Rainbow };
        double colorMidpoint { 0.5 };
        double colorSharpness { 0.0 };
        double window { 1.0 };
        double level { 0.5 };
        double opacityMin { 0.0 };
        double opacityMax { 0.8 };
        int sampleDistanceIndex { 2 };
        Mesh::VolumeQuality quality { Mesh::VolumeQuality::High };
        double meshColor[3] { 0.0, 0.0, 0.0 };
        double meshOpacity { 1.0 };
        double meshEdgeColor[3] { 0.0, 0.0, 0.0 };
    };

    // ========================================================================
    // Mesh Management
    // ========================================================================
    Mesh* InsertMesh(const char* name, vtkSmartPointer<vtkDataSet> edgeDataSet = nullptr,
        vtkSmartPointer<vtkDataSet> faceDataSet = nullptr, vtkSmartPointer<vtkDataSet> volumeDataSet = nullptr,
        int32_t parentId = 0);
    const LcrsTreeUPtr& GetMeshTree() const { return m_MeshTree; }
    const Mesh* GetMeshById(int32_t id) const;
    Mesh* GetMeshByIdMutable(int32_t id);

    void ShowMesh(int32_t id);
    void HideMesh(int32_t id);
    void DeleteMesh(int32_t id);
    void SetDisplayMode(int32_t id, MeshDisplayMode mode);
    void SetAllDisplayMode(MeshDisplayMode mode);

    // ========================================================================
    // ✅ 추가: Volume Display Settings Management
    // ========================================================================
    bool HasSharedVolumeDisplaySettings() const { return m_HasSharedVolumeDisplaySettings; }
    const VolumeDisplaySettings& GetSharedVolumeDisplaySettings() const { return m_SharedVolumeDisplaySettings; }
    void SetSharedVolumeDisplaySettings(const VolumeDisplaySettings& settings);
    void EnsureSharedVolumeDisplaySettingsFromMesh(const Mesh& mesh);
    void ApplySharedVolumeDisplaySettingsToAllMeshes();
    bool GetGlobalVolumeDataRange(double& minOut, double& maxOut) const;

    // ========================================================================
    // Tree Utilities
    // ========================================================================
    IconState GetChildrenVisibility(const TreeNode* parentNode) const;
    static void SetParentIconState(TreeNode* node);

    // ========================================================================
    // XSF Structure Management
    // ========================================================================
    int32_t RegisterXsfStructure(const std::string& fileName);
    void DeleteXsfStructure(int32_t id);
    void DeleteAllXsfStructures();
    bool HasXsfStructures() const { return !m_XsfStructureIds.empty(); }

#ifdef DEBUG_BUILD
    static void PrintMeshTree();
#endif

private:
    LcrsTreeUPtr m_MeshTree;
    std::vector<int32_t> m_XsfStructureIds;  // 로드된 XSF 구조 ID 목록

    // Map of mesh id to mesh pointer
    // This map has the only ownership of the mesh data.
    std::unordered_map<int32_t, MeshUPtr> m_IdToMeshMap;

    // ✅ 추가: Volume Display Settings
    bool m_HasSharedVolumeDisplaySettings { false };
    VolumeDisplaySettings m_SharedVolumeDisplaySettings;

#ifdef DEBUG_BUILD
    void createTestMeshItems();
#endif

    // ✅ 추가: Helper function
    void applyVolumeDisplaySettingsToMesh(Mesh* mesh, const VolumeDisplaySettings& settings);
};
