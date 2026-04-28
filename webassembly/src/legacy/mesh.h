#pragma once

#include "macro/ptr_macro.h"
#include "enum/viewer_enums.h"
#include "mesh_group.h"
#include "common/colormap.h"

// Standard library
#include <cstdint>
#include <memory>
#include <vector>

// VTK
#include <vtkDataSet.h>

class vtkDataSetMapper;
class vtkActor;
class vtkPolyDataMapper;
class vtkScalarBarActor;
class vtkColorTransferFunction;
class vtkVolume;
class vtkSmartVolumeMapper;
class vtkVolumeProperty;
class vtkPiecewiseFunction;


DECLARE_PTR(Mesh)
class Mesh {
public:
    enum class VolumeRenderMode {
        Surface = 0,
        Volume = 1
    };

    using VolumeColorPreset = common::ColorMapPreset;

    enum class VolumeQuality {
        Low = 0,
        Medium = 1,
        High = 2
    };

    static MeshUPtr New(const char* name,
        vtkSmartPointer<vtkDataSet> edgeDataSet = nullptr,
        vtkSmartPointer<vtkDataSet> faceDataSet = nullptr,
        vtkSmartPointer<vtkDataSet> volumeDataSet = nullptr);
    ~Mesh();

    // Getters
    int32_t GetId() const { return m_Id; }
    const char* GetName() const { return m_Name.c_str(); }
    vtkSmartPointer<vtkActor> GetEdgeMeshActor() const { return m_EdgeMeshActor; }
    vtkSmartPointer<vtkActor> GetFaceMeshActor() const { return m_FaceMeshActor; }
    vtkSmartPointer<vtkActor> GetVolumeMeshActor() const { return m_VolumeMeshActor; }
    vtkSmartPointer<vtkVolume> GetVolumeRenderActor() const { return m_VolumeRenderActor; }
    void MarkVolumeRenderAdded() { m_VolumeRenderAdded = true; }
    vtkSmartPointer<vtkActor> GetStreamLineActor() const { return m_StreamLineActor; }
    vtkSmartPointer<vtkScalarBarActor> GetScalarBarActor() const { return m_ScalarBarActor; }

    // Get edge element properties
    const double* GetEdgeMeshColor() const;
    double GetEdgeMeshOpacity() const;
    bool GetEdgeMeshVisibility() const;

    // Get face element properties
    const double* GetFaceMeshColor() const;
    double GetFaceMeshOpacity() const;
    const double* GetFaceMeshEdgeColor() const;
    bool GetFaceMeshVisibility() const;

    // Get volume element properties
    const double* GetVolumeMeshColor() const;
    double GetVolumeMeshOpacity() const;
    const double* GetVolumeMeshEdgeColor() const;
    bool GetVolumeMeshVisibility() const;
    bool GetVolumeSurfaceVisibility() const { return m_VolumeSurfaceVisibility; }
    bool GetVolumeRenderVisibility() const { return m_VolumeRenderVisibility; }

    bool IsVolumeRenderingAvailable() const { return m_VolumeRenderActor != nullptr; }
    VolumeRenderMode GetVolumeRenderMode() const { return m_VolumeRenderMode; }
    VolumeColorPreset GetVolumeColorPreset() const { return m_VolumeColorPreset; }
    double GetVolumeColorMidpoint() const { return m_VolumeColorMidpoint; }
    double GetVolumeColorSharpness() const { return m_VolumeColorSharpness; }
    double GetVolumeWindow() const { return m_VolumeWindow; }
    double GetVolumeLevel() const { return m_VolumeLevel; }
    double GetVolumeRenderOpacityMin() const { return m_VolumeRenderOpacityMin; }
    double GetVolumeRenderOpacity() const { return m_VolumeRenderOpacityMax; }
    int GetVolumeSampleDistanceIndex() const { return m_VolumeSampleDistanceIndex; }
    VolumeQuality GetVolumeQuality() const { return m_VolumeQuality; }
    const double* GetVolumeDataRange() const { return m_VolumeDataRange; }
    bool IsVolumeAutoAdjustSampleDistances() const;
    bool IsVolumeInteractiveAdjustSampleDistances() const;

    const std::vector<MeshGroupUPtr>& GetMeshGroups() const { return m_MeshGroups; }
    size_t GetMeshGroupCount() const { return m_MeshGroups.size(); }

    const MeshGroup* GetMeshGroupById(int32_t groupId) const;
    MeshGroup* GetMeshGroupByIdMutable(int32_t groupId);

    // Set edge element properties
    void SetEdgeMeshColor(double r, double g, double b);
    void SetEdgeMeshOpacity(double opacity);
    void SetEdgeMeshVisibility(bool visibility);

    // Set face element properties
    void SetFaceMeshColor(double r, double g, double b);
    void SetFaceMeshOpacity(double opacity);
    void SetFaceMeshEdgeColor(double r, double g, double b);
    void SetFaceMeshVisibility(bool visibility);

    // Set volume element properties
    void SetVolumeMeshColor(double r, double g, double b);
    void SetVolumeMeshOpacity(double opacity);
    void SetVolumeMeshEdgeColor(double r, double g, double b);
    void SetVolumeMeshVisibility(bool visibility);
    void SetVolumeSurfaceVisibility(bool visibility);
    void SetVolumeRenderVisibility(bool visibility);
    void SetVolumeMeshSuppressed(bool suppressed);
    bool IsVolumeMeshSuppressed() const { return m_VolumeMeshSuppressed; }

    void SetVolumeRenderMode(VolumeRenderMode mode);
    void SetVolumeColorPreset(VolumeColorPreset preset);
    void SetVolumeColorCurve(double midpoint, double sharpness);
    void SetVolumeWindowLevel(double window, double level);
    void SetVolumeRenderOpacityMin(double opacity);
    void SetVolumeRenderOpacity(double opacity);
    void SetVolumeSampleDistanceIndex(int index);
    void SetVolumeSampleDistanceAutoMode(bool autoAdjust, bool interactiveAdjust);
    void SetVolumeQuality(VolumeQuality quality);
    void SetVolumeQualityDataSets(vtkSmartPointer<vtkDataSet> highSurface,
        vtkSmartPointer<vtkDataSet> mediumSurface,
        vtkSmartPointer<vtkDataSet> lowSurface,
        vtkSmartPointer<vtkDataSet> highVolume,
        vtkSmartPointer<vtkDataSet> mediumVolume,
        vtkSmartPointer<vtkDataSet> lowVolume);

    void SetDisplayMode(MeshDisplayMode mode);

    void SetMeshGroups(std::vector<MeshGroupUPtr>&& meshGroups);
    void DeleteMeshGroup(int32_t groupId);

    bool IsXsfStructure() const { return m_IsXsfStructure; }
    void SetIsXsfStructure(bool isXsf) { m_IsXsfStructure = isXsf; }

    static int32_t GenerateNewId() { return ++m_LastId; }

private:
    int32_t m_Id { -1 };
    std::string m_Name;
    bool m_IsXsfStructure = false;  // XSF 구조인지 여부

    vtkSmartPointer<vtkDataSet> m_EdgeDataSet { nullptr };
    vtkSmartPointer<vtkDataSetMapper> m_EdgeMeshMapper { nullptr };
    vtkSmartPointer<vtkActor> m_EdgeMeshActor { nullptr };

    vtkSmartPointer<vtkDataSet> m_FaceDataSet { nullptr };
    vtkSmartPointer<vtkDataSetMapper> m_FaceMeshMapper { nullptr };
    vtkSmartPointer<vtkActor> m_FaceMeshActor { nullptr };

    vtkSmartPointer<vtkDataSet> m_VolumeDataSet { nullptr };
    vtkSmartPointer<vtkDataSetMapper> m_VolumeMeshMapper { nullptr };
    vtkSmartPointer<vtkActor> m_VolumeMeshActor { nullptr };
    vtkSmartPointer<vtkDataSet> m_VolumeMeshQualityDataSets[3] { nullptr, nullptr, nullptr };
    vtkSmartPointer<vtkDataSet> m_VolumeRenderQualityDataSets[3] { nullptr, nullptr, nullptr };
    vtkSmartPointer<vtkSmartVolumeMapper> m_VolumeRenderMapper { nullptr };
    vtkSmartPointer<vtkVolumeProperty> m_VolumeRenderProperty { nullptr };
    vtkSmartPointer<vtkColorTransferFunction> m_VolumeRenderColor { nullptr };
    vtkSmartPointer<vtkPiecewiseFunction> m_VolumeRenderOpacity { nullptr };
    vtkSmartPointer<vtkVolume> m_VolumeRenderActor { nullptr };
    bool m_VolumeRenderAdded { false };

    VolumeRenderMode m_VolumeRenderMode { VolumeRenderMode::Surface };
    VolumeColorPreset m_VolumeColorPreset { VolumeColorPreset::Rainbow };
    VolumeQuality m_VolumeQuality { VolumeQuality::High };
    double m_VolumeColorMidpoint { 0.5 };
    double m_VolumeColorSharpness { 0.0 };
    double m_VolumeWindow { 1.0 };
    double m_VolumeLevel { 0.5 };
    bool m_VolumeWindowLevelAuto { true };
    double m_VolumeRenderOpacityMin { 0.0 };
    double m_VolumeRenderOpacityMax { 0.8 };
    int m_VolumeSampleDistanceIndex { 2 };
    double m_VolumeDataRange[2] { 0.0, 1.0 };
    bool m_VolumeMeshVisibility { true };
    bool m_VolumeSurfaceVisibility { true };
    bool m_VolumeRenderVisibility { false };
    bool m_VolumeMeshSuppressed { false };

    vtkSmartPointer<vtkPolyDataMapper> m_StreamLineMapper { nullptr };
    vtkSmartPointer<vtkActor> m_StreamLineActor { nullptr };
    vtkSmartPointer<vtkScalarBarActor> m_ScalarBarActor { nullptr };
    vtkSmartPointer<vtkColorTransferFunction> m_ColorTransferFunction { nullptr };

    std::vector<MeshGroupUPtr> m_MeshGroups;

    static int32_t m_LastId;

    Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    void createMeshMapper();
    void createMeshActor();
    void createStreamLineMapper();
    void createStreamLineActor();
    void createScalarBarActor();

    void ensureVolumeMeshPipeline();
    void ensureVolumeRenderPipeline(vtkSmartPointer<vtkDataSet> dataSet);
    void updateVolumeDataRange(vtkSmartPointer<vtkDataSet> dataSet);
    void updateVolumeColorTransfer();
    void updateVolumeOpacityFunction();
    void updateVolumeSampleDistance();
    void updateVolumeRenderVisibility();
};
