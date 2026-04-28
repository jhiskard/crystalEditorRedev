#pragma once

#include "macro/singleton_macro.h"
#include "mesh_manager.h"  // ✅ 추가: VolumeDisplaySettings 사용

// Standard library
#include <cstdint>
#include <functional>


class MeshDetail {
    DECLARE_SINGLETON(MeshDetail)

public:
    void Render(int32_t meshId, bool* openWindow = nullptr);
    void RenderVolumeControls(int32_t meshId, const char* tableId, bool showRenderMode = true);

    // ========================================================================
    // Getters
    // ========================================================================
    bool GetUiEdgeMeshVisibility() const { return m_UiEdgeMeshVisibility; }
    bool GetUiFaceMeshVisibility() const { return m_UiFaceMeshVisibility; }
    bool GetUiVolumeMeshVisibility() const { return m_UiVolumeMeshVisibility; }
    
    // ✅ 추가: Volume Rendering Getters
    int GetUiVolumeRenderMode() const { return m_UiVolumeRenderMode; }
    int GetUiVolumeColorPreset() const { return m_UiVolumeColorPreset; }

    // ========================================================================
    // Edge Mesh Setters
    // ========================================================================
    void SetUiEdgeMeshColor(const double* color);
    void SetUiEdgeMeshOpacity(double opacity);
    void SetUiEdgeMeshVisibility(bool visibility) { m_UiEdgeMeshVisibility = visibility; }

    // ========================================================================
    // Face Mesh Setters
    // ========================================================================
    void SetUiFaceMeshColor(const double* color);
    void SetUiFaceMeshOpacity(double opacity);
    void SetUiFaceMeshEdgeColor(const double* color);
    void SetUiFaceMeshVisibility(bool visibility) { m_UiFaceMeshVisibility = visibility; }

    // ========================================================================
    // Volume Mesh Setters
    // ========================================================================
    void SetUiVolumeMeshColor(const double* color);
    void SetUiVolumeMeshOpacity(double opacity);
    void SetUiVolumeMeshEdgeColor(const double* color);
    void SetUiVolumeMeshVisibility(bool visibility) { m_UiVolumeMeshVisibility = visibility; }
    
    // ✅ 추가: Volume Rendering Setters
    void SetUiVolumeWindowLevel(double window, double level);
    void SetUiVolumeRenderOpacityMin(double opacity);
    void SetUiVolumeRenderOpacity(double opacity);
    void SetUiVolumeRenderMode(int mode) { m_UiVolumeRenderMode = mode; }
    void SetUiVolumeColorPreset(int preset) { m_UiVolumeColorPreset = preset; }
    void SetUiVolumeColorCurve(double midpoint, double sharpness);
    void SetUiVolumeSampleDistanceIndex(int index) { m_UiVolumeSampleDistanceIndex = index; }
    void SetUiVolumeQuality(int quality) { m_UiVolumeQuality = quality; }

    // ========================================================================
    // Changed Flags Setters
    // ========================================================================
    void SetHasEdgeMeshColorChanged(bool hasChanged) { s_HasEdgeMeshColorChanged = hasChanged; }
    void SetHasEdgeMeshOpacityChanged(bool hasChanged) { s_HasEdgeMeshOpacityChanged = hasChanged; }

    void SetHasFaceMeshColorChanged(bool hasChanged) { s_HasFaceMeshColorChanged = hasChanged; }
    void SetHasFaceMeshOpacityChanged(bool hasChanged) { s_HasFaceMeshOpacityChanged = hasChanged; }
    void SetHasFaceMeshEdgeColorChanged(bool hasChanged) { s_HasFaceMeshEdgeColorChanged = hasChanged; }

    void SetHasVolumeMeshColorChanged(bool hasChanged) { s_HasVolumeMeshColorChanged = hasChanged; }
    void SetHasVolumeMeshOpacityChanged(bool hasChanged) { s_HasVolumeMeshOpacityChanged = hasChanged; }
    void SetHasVolumeMeshEdgeColorChanged(bool hasChanged) { s_HasVolumeMeshEdgeColorChanged = hasChanged; }
    
    // ✅ 추가: Volume Rendering Changed Flags
    void SetHasVolumeRenderOpacityMinChanged(bool hasChanged) { s_HasVolumeRenderOpacityMinChanged = hasChanged; }
    void SetHasVolumeRenderOpacityMaxChanged(bool hasChanged) { s_HasVolumeRenderOpacityMaxChanged = hasChanged; }
    void SetHasVolumeWindowLevelChanged(bool hasChanged) { s_HasVolumeWindowLevelChanged = hasChanged; }
    void SetHasVolumeColorCurveChanged(bool hasChanged) { s_HasVolumeColorCurveChanged = hasChanged; }
    void SetHasVolumeSampleDistanceChanged(bool hasChanged) { s_HasVolumeSampleDistanceChanged = hasChanged; }

private:
    void forEachTargetVolumeMesh(int32_t meshId, const std::function<void(Mesh*)>& func);
    void syncVolumeUiFromMesh(const Mesh& mesh);

    // ========================================================================
    // Edge Mesh UI State
    // ========================================================================
    float m_UiEdgeMeshColor[3] { 0.0f, 0.0f, 0.0f };
    float m_UiEdgeMeshOpacity { 0.0f };
    bool m_UiEdgeMeshVisibility { false };

    // ========================================================================
    // Face Mesh UI State
    // ========================================================================
    float m_UiFaceMeshColor[3] { 0.0f, 0.0f, 0.0f };
    float m_UiFaceMeshOpacity { 0.0f };
    float m_UiFaceMeshEdgeColor[3] { 0.0f, 0.0f, 0.0f };
    bool m_UiFaceMeshVisibility { false };

    // ========================================================================
    // Volume Mesh UI State
    // ========================================================================
    float m_UiVolumeMeshColor[3] { 0.0f, 0.0f, 0.0f };
    float m_UiVolumeMeshOpacity { 0.0f };
    float m_UiVolumeMeshEdgeColor[3] { 0.0f, 0.0f, 0.0f };
    bool m_UiVolumeMeshVisibility { true };
    bool m_UiVolumeDirectApply { true };
    bool m_VolumeUseSharedSettings { true };
    int32_t m_LastVolumeControlMeshId { -1 };
    
    // ✅ 추가: Volume Rendering UI State
    float m_UiVolumeWindow { 1.0f };
    float m_UiVolumeLevel { 0.5f };
    float m_UiVolumeRenderOpacityMin { 0.0f };
    float m_UiVolumeRenderOpacity { 0.0f };
    int m_UiVolumeRenderMode { 0 };
    int m_UiVolumeColorPreset { static_cast<int>(Mesh::VolumeColorPreset::Rainbow) };
    float m_UiVolumeColorMidpoint { 0.5f };
    float m_UiVolumeColorSharpness { 0.0f };
    int m_UiVolumeSampleDistanceIndex { 2 };
    int m_UiVolumeQuality { 2 };

    // ========================================================================
    // Static Changed Flags
    // ========================================================================
    static bool s_HasEdgeMeshColorChanged;
    static bool s_HasEdgeMeshOpacityChanged;

    static bool s_HasFaceMeshColorChanged;
    static bool s_HasFaceMeshOpacityChanged;
    static bool s_HasFaceMeshEdgeColorChanged;

    static bool s_HasVolumeMeshColorChanged;
    static bool s_HasVolumeMeshOpacityChanged;
    static bool s_HasVolumeMeshEdgeColorChanged;
    
    // ✅ 추가: Volume Rendering Changed Flags
    static bool s_HasVolumeRenderOpacityMinChanged;
    static bool s_HasVolumeRenderOpacityMaxChanged;
    static bool s_HasVolumeWindowLevelChanged;
    static bool s_HasVolumeColorCurveChanged;
    static bool s_HasVolumeSampleDistanceChanged;

    // ========================================================================
    // Basic Table Row Rendering
    // ========================================================================
    void renderTableRow(const char* item, int32_t value);
    void renderTableRow(const char* item, const char* value = nullptr);
    void renderTableRow(const char* item, double value);

    // ========================================================================
    // Edge Mesh Table Row Rendering
    // ========================================================================
    void renderTableRowEdgeMeshColor(int32_t meshId, const double* color);
    void renderTableRowEdgeMeshOpacity(int32_t meshId, double opacity);
    void renderTableRowEdgeMeshVisibility(int32_t meshId, bool visibility);

    // ========================================================================
    // Face Mesh Table Row Rendering
    // ========================================================================
    void renderTableRowFaceMeshColor(int32_t meshId, const double* color);
    void renderTableRowFaceMeshOpacity(int32_t meshId, double opacity);
    void renderTableRowFaceMeshEdgeColor(int32_t meshId, const double* color);
    void renderTableRowFaceMeshVisibility(int32_t meshId, bool visibility);

    // ========================================================================
    // Volume Mesh Table Row Rendering
    // ========================================================================
    void renderTableRowVolumeMeshColor(int32_t meshId, const double* color);
    void renderTableRowVolumeMeshOpacity(int32_t meshId, double opacity);
    void renderTableRowVolumeMeshEdgeColor(int32_t meshId, const double* color);
    void renderTableRowVolumeMeshVisibility(int32_t meshId, bool visibility, bool linkToTree);
    void renderTableRowVolumeDirectApply();
    
    // ✅ 추가: Volume Rendering Table Row Rendering
    void renderTableRowVolumeRenderMode(int32_t meshId, bool volumeAvailable);
    void renderTableRowVolumeWindowLevel(int32_t meshId);
    void renderTableRowVolumeRenderOpacityMin(int32_t meshId, double opacity);
    void renderTableRowVolumeRenderOpacity(int32_t meshId, double opacity);
    void renderTableRowVolumeColorPreset(int32_t meshId, bool volumeAvailable);
    void renderTableRowVolumeColorCurve(int32_t meshId);
    void renderTableRowVolumeSampleDistance(int32_t meshId);
    void renderTableRowVolumeQuality(int32_t meshId);

    // ✅ 추가: Shared Volume Settings Sync
    void syncSharedVolumeUiFromSettings(const MeshManager::VolumeDisplaySettings& settings);
};
