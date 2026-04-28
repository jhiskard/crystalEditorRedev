#include "mesh.h"
#include "app.h"
#include "vtk_viewer.h"
#include "toolbar.h"

// 표준 라이브러리
#include <algorithm>

// VTK
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkMath.h>
#include <vtkPointSource.h>
#include <vtkColorTransferFunction.h>
#include <vtkStreamTracer.h>
#include <vtkLineSource.h>
#include <vtkScalarBarActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>

namespace {
struct VolumeWindowLevelRange {
    double rangeMin;
    double rangeMax;
    double window;
    double level;
};

VolumeWindowLevelRange resolveVolumeWindowLevelRange(double dataMin, double dataMax, double window, double level) {
    if (dataMin == dataMax) {
        dataMax = dataMin + 1.0;
    }

    if (window <= 0.0) {
        window = dataMax - dataMin;
    }
    if (window <= 0.0) {
        window = 1.0;
    }

    double minValue = level - window * 0.5;
    double maxValue = level + window * 0.5;
    if (minValue < dataMin) {
        minValue = dataMin;
    }
    if (maxValue > dataMax) {
        maxValue = dataMax;
    }
    if (minValue >= maxValue) {
        minValue = dataMin;
        maxValue = dataMax;
    }

    VolumeWindowLevelRange resolved;
    resolved.rangeMin = minValue;
    resolved.rangeMax = maxValue;
    resolved.window = maxValue - minValue;
    resolved.level = (minValue + maxValue) * 0.5;
    return resolved;
}
}  // namespace


int32_t Mesh::m_LastId = -1;

MeshUPtr Mesh::New(const char* name,
    vtkSmartPointer<vtkDataSet> edgeDataSet,
    vtkSmartPointer<vtkDataSet> faceDataSet,
    vtkSmartPointer<vtkDataSet> volumeDataSet) {
    MeshUPtr mesh = MeshUPtr(new Mesh());
    if (name == nullptr) {
        return nullptr;
    }

    mesh->m_Id = GenerateNewId();
    mesh->m_Name = name;

    // Set data set
    if (edgeDataSet) {
        mesh->m_EdgeDataSet = edgeDataSet;
    }
    if (faceDataSet) {
        mesh->m_FaceDataSet = faceDataSet;
    }
    if (volumeDataSet) {
        mesh->m_VolumeDataSet = volumeDataSet;
        mesh->m_VolumeMeshQualityDataSets[static_cast<int>(VolumeQuality::High)] = volumeDataSet;
        mesh->m_VolumeRenderQualityDataSets[static_cast<int>(VolumeQuality::High)] = volumeDataSet;
    }

    // Create mesh mapper and actor
    mesh->createMeshMapper();
    mesh->createMeshActor();

    // // Create stream line mapper and actor
    // mesh->createStreamLineMapper();
    // mesh->createStreamLineActor();

    // // Create scalar bar actor
    // mesh->createScalarBarActor();

    return std::move(mesh);
}

Mesh::Mesh() {
}

Mesh::~Mesh() {
    SPDLOG_DEBUG("Mesh ({}) destroyed.", m_Name);

    // Remove vtk actors from the VtkViewer
    VtkViewer& vtkViewer = VtkViewer::Instance();   
    vtkViewer.RemoveActor(m_EdgeMeshActor);
    vtkViewer.RemoveActor(m_FaceMeshActor);
    vtkViewer.RemoveActor(m_VolumeMeshActor);
    vtkViewer.RemoveVolume(m_VolumeRenderActor);
    vtkViewer.RemoveActor(m_StreamLineActor);
    vtkViewer.RemoveActor2D(m_ScalarBarActor);
}

void Mesh::createMeshMapper() {
    if (m_EdgeDataSet == nullptr &&
        m_FaceDataSet == nullptr &&
        m_VolumeDataSet == nullptr) {
        return;
    }

    // Set edge data set to the mapper
    if (m_EdgeDataSet != nullptr) {
        if (m_EdgeMeshMapper == nullptr) {
            m_EdgeMeshMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        }
        m_EdgeMeshMapper->SetInputData(m_EdgeDataSet);
        m_EdgeMeshMapper->Update();
    }
    
    // Set face data set to the mapper
    if (m_FaceDataSet != nullptr) {
        if (m_FaceMeshMapper == nullptr) {
            m_FaceMeshMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        }
        m_FaceMeshMapper->SetInputData(m_FaceDataSet);
        m_FaceMeshMapper->Update();
    }

    // Set volume data set to the mapper
    if (m_VolumeDataSet != nullptr) {
        if (m_VolumeMeshMapper == nullptr) {
            m_VolumeMeshMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        }
        m_VolumeMeshMapper->SetInputData(m_VolumeDataSet);
        m_VolumeMeshMapper->Update();
        updateVolumeDataRange(m_VolumeDataSet);
        updateVolumeColorTransfer();
    }
}

void Mesh::createMeshActor() {
    if (m_EdgeMeshMapper == nullptr &&
        m_FaceMeshMapper == nullptr &&
        m_VolumeMeshMapper == nullptr) {
        return;
    }

    // Set edge mesh actor
    if (m_EdgeMeshMapper != nullptr) {
        if (m_EdgeMeshActor == nullptr) {
            m_EdgeMeshActor = vtkSmartPointer<vtkActor>::New();
        }
        m_EdgeMeshActor->SetMapper(m_EdgeMeshMapper);
    }

    // Set face mesh actor
    if (m_FaceMeshMapper != nullptr) {
        if (m_FaceMeshActor == nullptr) {
            m_FaceMeshActor = vtkSmartPointer<vtkActor>::New();
        }
        m_FaceMeshActor->SetMapper(m_FaceMeshMapper);
    }

    // Set volume mesh actor
    if (m_VolumeMeshMapper != nullptr) {
        if (m_VolumeMeshActor == nullptr) {
            m_VolumeMeshActor = vtkSmartPointer<vtkActor>::New();
        }
        m_VolumeMeshActor->SetMapper(m_VolumeMeshMapper);
    }

    // Set default properties - color, opacity, edge color
    SetEdgeMeshColor(0.95, 0.95, 0.95);   // Light gray
    SetEdgeMeshOpacity(1.0);
    SetEdgeMeshVisibility(false);  // Hide by default

    SetFaceMeshColor(0.0, 0.67, 1.0);
    SetFaceMeshOpacity(1.0);
    SetFaceMeshEdgeColor(0.0, 0.0, 0.0);
    SetFaceMeshVisibility(false);  // Hide by default

    SetVolumeMeshColor(0.95, 0.95, 0.95);   // Light gray
    SetVolumeMeshOpacity(1.0);
    SetVolumeMeshEdgeColor(0.0, 0.0, 0.0);  // Black
    SetVolumeMeshVisibility(true);  // Show by default

    SetDisplayMode(Toolbar::Instance().GetMeshDisplayMode());

    if (m_VolumeDataSet != nullptr) {
        vtkSmartPointer<vtkDataSet> volumeData = m_VolumeRenderQualityDataSets[static_cast<int>(m_VolumeQuality)];
        if (!volumeData) {
            volumeData = m_VolumeDataSet;
        }
        ensureVolumeRenderPipeline(volumeData);
    }
}

void Mesh::createStreamLineMapper() {
    if (m_VolumeDataSet == nullptr) {
        return;
    }

    vtkPointData* pointData = m_VolumeDataSet->GetPointData();
    if (pointData == nullptr) {
        return;
    }

    vtkDataArray* velocity = pointData->GetArray("U");
    if (velocity == nullptr) {
        return;
    }

    vtkNew<vtkFloatArray> velocityMag;
    velocityMag->SetName("Umag");
    velocityMag->SetNumberOfComponents(1);

    vtkIdType tupleCount = velocity->GetNumberOfTuples();
    velocityMag->SetNumberOfTuples(tupleCount);

    for (vtkIdType i = 0; i < tupleCount; ++i) {
        double vel[3];
        velocity->GetTuple(i, vel);
        double mag = vtkMath::Norm(vel);
        velocityMag->SetTuple1(i, mag);
    }
    m_VolumeDataSet->GetPointData()->AddArray(velocityMag);
    m_VolumeDataSet->GetPointData()->SetActiveScalars("Umag");

    // Set color transfer function       
    double minRange = velocityMag->GetRange()[0];
    double maxRange = velocityMag->GetRange()[1];
    double midRange = (minRange + maxRange) / 2.0;

    // Adding colors at specified points
    m_ColorTransferFunction->AddRGBPoint(minRange, 0.231373, 0.298039, 0.752941); // Hex #3b4cc0
    m_ColorTransferFunction->AddRGBPoint(midRange, 0.865003, 0.865003, 0.865003); // Hex #dfdbd9
    m_ColorTransferFunction->AddRGBPoint(maxRange, 0.705882, 0.0156863, 0.14902); // Hex #b40426

    m_VolumeMeshMapper->SetLookupTable(m_ColorTransferFunction);
    m_VolumeMeshMapper->SetColorModeToMapScalars();
    m_VolumeMeshMapper->ScalarVisibilityOn();
    m_VolumeMeshMapper->SetScalarRange(minRange, maxRange);

    // TODO: Line and sphere sources should be moved to VtkViewer class

    // Stream tracer setup with point cloud
    // int sourceCount = 200;
    // double Xc = 0.0;  double Yc = -0.3;  double Zc = 0.0;
    // double radius = 0.3;

    // vtkNew<vtkPointSource> sphereSource;
    // sphereSource->SetNumberOfPoints(sourceCount);
    // sphereSource->SetCenter(Xc, Yc, Zc);
    // sphereSource->SetRadius(radius);

    // Stream tracer setup with line source
    int sourceCount = 100;
    double X1 = -0.5;  double Y1 = 0.0;  double Z1 = 0.5;
    double X2 = -0.5;  double Y2 = 4.0;  double Z2 = 0.5;
    vtkNew<vtkLineSource> lineSource;
    lineSource->SetPoint1(X1, Y1, Z1);
    lineSource->SetPoint2(X2, Y2, Z2);
    lineSource->SetResolution(sourceCount);

    // Calculate streamlines
    vtkNew<vtkStreamTracer> streamTracer;
    streamTracer->SetInputData(m_VolumeDataSet);
    // streamTracer->SetSourceConnection(sphereSource->GetOutputPort());
    streamTracer->SetSourceConnection(lineSource->GetOutputPort());
    
    // Set vector field
    streamTracer->SetInputArrayToProcess(0, 0, 0, 
        vtkDataObject::FIELD_ASSOCIATION_POINTS, velocity->GetName());

    streamTracer->SetMaximumPropagation(3.2);
    streamTracer->SetInitialIntegrationStep(0.1);
    streamTracer->SetIntegrationStepUnit(2);  // 2: Cell length
    streamTracer->SetMinimumIntegrationStep(0.01);
    streamTracer->SetMaximumIntegrationStep(0.5);
    streamTracer->SetIntegrationDirectionToBoth();
    streamTracer->SetComputeVorticity(false);
    streamTracer->SetIntegratorTypeToRungeKutta45();
    streamTracer->SetMaximumNumberOfSteps(2000);

    m_StreamLineMapper->SetInputConnection(streamTracer->GetOutputPort());
    m_StreamLineMapper->SetLookupTable(m_ColorTransferFunction);
    m_StreamLineMapper->SetScalarRange(minRange, maxRange);
}

void Mesh::createStreamLineActor() {
    m_StreamLineActor->SetMapper(m_StreamLineMapper);
    m_StreamLineActor->GetProperty()->SetLineWidth(3.0);    
}

void Mesh::createScalarBarActor() {
    if (m_ColorTransferFunction == nullptr) {
        return;
    }

    m_ScalarBarActor->SetLookupTable(m_ColorTransferFunction);
    m_ScalarBarActor->SetNumberOfLabels(8);
    m_ScalarBarActor->SetMaximumWidthInPixels(static_cast<int>(80 * App::DevicePixelRatio()));
    m_ScalarBarActor->SetMaximumHeightInPixels(static_cast<int>(240 * App::DevicePixelRatio()));
    m_ScalarBarActor->SetPosition(0.85, 0.1);
}

void Mesh::SetDisplayMode(MeshDisplayMode mode) {
    // Edge mesh actor is not affected by the display mode.
    if (m_FaceMeshActor == nullptr &&
        m_VolumeMeshActor == nullptr) {
        return;
    }

    if (m_FaceMeshActor != nullptr) {
        if (mode == MeshDisplayMode::WIREFRAME) {
            m_FaceMeshActor->GetProperty()->SetRepresentationToWireframe();
            m_FaceMeshActor->GetProperty()->SetEdgeVisibility(true);
        }
        else if (mode == MeshDisplayMode::SHADED) {
            m_FaceMeshActor->GetProperty()->SetRepresentationToSurface();
            m_FaceMeshActor->GetProperty()->SetEdgeVisibility(false);
        }
        else if (mode == MeshDisplayMode::WIRESHADED) {
            m_FaceMeshActor->GetProperty()->SetRepresentationToSurface();
            m_FaceMeshActor->GetProperty()->SetEdgeVisibility(true);
        }
    }

    if (m_VolumeMeshActor != nullptr) {
        if (mode == MeshDisplayMode::WIREFRAME) {
            m_VolumeMeshActor->GetProperty()->SetRepresentationToWireframe();
            m_VolumeMeshActor->GetProperty()->SetEdgeVisibility(true);
        }
        else if (mode == MeshDisplayMode::SHADED) {
            m_VolumeMeshActor->GetProperty()->SetRepresentationToSurface();
            m_VolumeMeshActor->GetProperty()->SetEdgeVisibility(false);
        }
        else if (mode == MeshDisplayMode::WIRESHADED) {
            m_VolumeMeshActor->GetProperty()->SetRepresentationToSurface();
            m_VolumeMeshActor->GetProperty()->SetEdgeVisibility(true);
        }   
    }

    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeMeshColor(double r, double g, double b) {
    if (m_VolumeMeshActor == nullptr) {
        return;
    }
    m_VolumeMeshActor->GetProperty()->SetColor(r, g, b);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeMeshOpacity(double opacity) {
    if (m_VolumeMeshActor == nullptr) {
        return;
    }
    m_VolumeMeshActor->GetProperty()->SetOpacity(opacity);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeMeshEdgeColor(double r, double g, double b) {
    if (m_VolumeMeshActor == nullptr) {
        return;
    }
    m_VolumeMeshActor->GetProperty()->SetEdgeColor(r, g, b);
    VtkViewer::Instance().RequestRender();
}

const double* Mesh::GetVolumeMeshColor() const {
    if (m_VolumeMeshActor == nullptr) {
        return nullptr;
    }
    return m_VolumeMeshActor->GetProperty()->GetColor();
}

double Mesh::GetVolumeMeshOpacity() const {
    if (m_VolumeMeshActor == nullptr) {
        return 1.0;
    }
    return m_VolumeMeshActor->GetProperty()->GetOpacity();
}

const double* Mesh::GetVolumeMeshEdgeColor() const {
    if (m_VolumeMeshActor == nullptr) {
        return nullptr;
    }
    return m_VolumeMeshActor->GetProperty()->GetEdgeColor();
}

const double* Mesh::GetEdgeMeshColor() const {
    if (m_EdgeMeshActor == nullptr) {
        return nullptr;
    }
    return m_EdgeMeshActor->GetProperty()->GetColor();
}

double Mesh::GetEdgeMeshOpacity() const {
    if (m_EdgeMeshActor == nullptr) {
        return 1.0;
    }
    return m_EdgeMeshActor->GetProperty()->GetOpacity();
}

void Mesh::SetEdgeMeshColor(double r, double g, double b) {
    if (m_EdgeMeshActor == nullptr) {
        return;
    }
    m_EdgeMeshActor->GetProperty()->SetColor(r, g, b);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetEdgeMeshOpacity(double opacity) {
    if (m_EdgeMeshActor == nullptr) {
        return;
    }
    m_EdgeMeshActor->GetProperty()->SetOpacity(opacity);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetEdgeMeshVisibility(bool visibility) {
    if (m_EdgeMeshActor == nullptr) {
        return;
    }
    m_EdgeMeshActor->SetVisibility(visibility);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeMeshVisibility(bool visibility) {
    m_VolumeMeshVisibility = visibility;
    updateVolumeColorTransfer();
    updateVolumeOpacityFunction();
    updateVolumeRenderVisibility();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeSurfaceVisibility(bool visibility) {
    m_VolumeSurfaceVisibility = visibility;
    updateVolumeRenderVisibility();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeRenderVisibility(bool visibility) {
    if (visibility && m_VolumeRenderActor == nullptr) {
        vtkSmartPointer<vtkDataSet> volumeData = m_VolumeRenderQualityDataSets[static_cast<int>(m_VolumeQuality)];
        if (!volumeData) {
            volumeData = m_VolumeDataSet;
        }
        ensureVolumeRenderPipeline(volumeData);
    }

    m_VolumeRenderVisibility = visibility;
    updateVolumeRenderVisibility();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeMeshSuppressed(bool suppressed) {
    m_VolumeMeshSuppressed = suppressed;
    updateVolumeRenderVisibility();
    VtkViewer::Instance().RequestRender();
}

const double* Mesh::GetFaceMeshColor() const {
    if (m_FaceMeshActor == nullptr) {
        return nullptr;
    }
    return m_FaceMeshActor->GetProperty()->GetColor();
}

double Mesh::GetFaceMeshOpacity() const {
    if (m_FaceMeshActor == nullptr) {
        return 1.0;
    }
    return m_FaceMeshActor->GetProperty()->GetOpacity();
}

const double* Mesh::GetFaceMeshEdgeColor() const {
    if (m_FaceMeshActor == nullptr) {
        return nullptr;
    }
    return m_FaceMeshActor->GetProperty()->GetEdgeColor();
}

void Mesh::SetFaceMeshColor(double r, double g, double b) {
    if (m_FaceMeshActor == nullptr) {
        return;
    }
    m_FaceMeshActor->GetProperty()->SetColor(r, g, b);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetFaceMeshOpacity(double opacity) {
    if (m_FaceMeshActor == nullptr) {
        return;
    }
    m_FaceMeshActor->GetProperty()->SetOpacity(opacity);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetFaceMeshEdgeColor(double r, double g, double b) {
    if (m_FaceMeshActor == nullptr) {
        return;
    }
    m_FaceMeshActor->GetProperty()->SetEdgeColor(r, g, b);
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetFaceMeshVisibility(bool visibility) {
    if (m_FaceMeshActor == nullptr) {
        return;
    }
    m_FaceMeshActor->SetVisibility(visibility);
    VtkViewer::Instance().RequestRender();
}

bool Mesh::GetEdgeMeshVisibility() const {
    if (m_EdgeMeshActor == nullptr) {
        return false;
    }
    return m_EdgeMeshActor->GetVisibility();
}

bool Mesh::GetFaceMeshVisibility() const {
    if (m_FaceMeshActor == nullptr) {
        return false;
    }
    return m_FaceMeshActor->GetVisibility();
}

bool Mesh::GetVolumeMeshVisibility() const {
    if (m_VolumeMeshActor == nullptr && m_VolumeRenderActor == nullptr) {
        return false;
    }
    return m_VolumeMeshVisibility;
}

bool Mesh::IsVolumeAutoAdjustSampleDistances() const {
    if (m_VolumeRenderMapper == nullptr) {
        return false;
    }
    return m_VolumeRenderMapper->GetAutoAdjustSampleDistances();
}

bool Mesh::IsVolumeInteractiveAdjustSampleDistances() const {
    if (m_VolumeRenderMapper == nullptr) {
        return false;
    }
    return m_VolumeRenderMapper->GetInteractiveAdjustSampleDistances();
}

void Mesh::SetVolumeRenderMode(VolumeRenderMode mode) {
    if (mode == VolumeRenderMode::Volume && m_VolumeRenderActor == nullptr) {
        vtkSmartPointer<vtkDataSet> volumeData = m_VolumeRenderQualityDataSets[static_cast<int>(m_VolumeQuality)];
        if (!volumeData) {
            volumeData = m_VolumeDataSet;
        }
        ensureVolumeRenderPipeline(volumeData);
    }
    if (mode == VolumeRenderMode::Volume && m_VolumeRenderActor == nullptr) {
        m_VolumeRenderMode = VolumeRenderMode::Surface;
    } else {
        m_VolumeRenderMode = mode;
    }

    // Keep legacy render-mode behavior for controls that explicitly change mode.
    if (m_VolumeRenderMode == VolumeRenderMode::Volume) {
        m_VolumeSurfaceVisibility = false;
        m_VolumeRenderVisibility = true;
    } else {
        m_VolumeSurfaceVisibility = true;
        m_VolumeRenderVisibility = false;
    }

    updateVolumeRenderVisibility();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeColorPreset(VolumeColorPreset preset) {
    m_VolumeColorPreset = preset;
    updateVolumeColorTransfer();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeColorCurve(double midpoint, double sharpness) {
    if (midpoint < 0.0) {
        midpoint = 0.0;
    } else if (midpoint > 1.0) {
        midpoint = 1.0;
    }

    if (sharpness < 0.0) {
        sharpness = 0.0;
    } else if (sharpness > 1.0) {
        sharpness = 1.0;
    }

    m_VolumeColorMidpoint = midpoint;
    m_VolumeColorSharpness = sharpness;
    updateVolumeColorTransfer();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeWindowLevel(double window, double level) {
    const VolumeWindowLevelRange resolved = resolveVolumeWindowLevelRange(
        m_VolumeDataRange[0], m_VolumeDataRange[1], window, level);
    m_VolumeWindow = resolved.window;
    m_VolumeLevel = resolved.level;
    m_VolumeWindowLevelAuto = false;
    updateVolumeColorTransfer();
    updateVolumeOpacityFunction();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeRenderOpacityMin(double opacity) {
    m_VolumeRenderOpacityMin = opacity;
    if (m_VolumeRenderOpacityMin > m_VolumeRenderOpacityMax) {
        m_VolumeRenderOpacityMin = m_VolumeRenderOpacityMax;
    }
    updateVolumeOpacityFunction();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeRenderOpacity(double opacity) {
    m_VolumeRenderOpacityMax = opacity;
    if (m_VolumeRenderOpacityMax < m_VolumeRenderOpacityMin) {
        m_VolumeRenderOpacityMax = m_VolumeRenderOpacityMin;
    }
    updateVolumeOpacityFunction();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeSampleDistanceIndex(int index) {
    if (index < 0) {
        index = 0;
    } else if (index > 5) {
        index = 5;
    }
    if (index == m_VolumeSampleDistanceIndex) {
        return;
    }
    m_VolumeSampleDistanceIndex = index;
    updateVolumeSampleDistance();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeSampleDistanceAutoMode(bool autoAdjust, bool interactiveAdjust) {
    if (m_VolumeRenderMapper == nullptr) {
        return;
    }
    const bool currentAutoAdjust = m_VolumeRenderMapper->GetAutoAdjustSampleDistances();
    const bool currentInteractiveAdjust = m_VolumeRenderMapper->GetInteractiveAdjustSampleDistances();
    if (currentAutoAdjust == autoAdjust && currentInteractiveAdjust == interactiveAdjust) {
        return;
    }
    m_VolumeRenderMapper->SetAutoAdjustSampleDistances(autoAdjust);
    m_VolumeRenderMapper->SetInteractiveAdjustSampleDistances(interactiveAdjust);
    if (!autoAdjust && !interactiveAdjust) {
        updateVolumeSampleDistance();
    }
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeQuality(VolumeQuality quality) {
    const int qualityIndex = static_cast<int>(quality);

    vtkSmartPointer<vtkDataSet> surfaceData = m_VolumeMeshQualityDataSets[qualityIndex];
    if (!surfaceData) {
        surfaceData = m_VolumeDataSet;
    }
    vtkSmartPointer<vtkDataSet> volumeData = m_VolumeRenderQualityDataSets[qualityIndex];
    if (!volumeData) {
        // 중저품질 데이터가 없으면 고품질 볼륨 데이터를 재사용해서 볼륨 모드를 유지
        volumeData = m_VolumeRenderQualityDataSets[static_cast<int>(VolumeQuality::High)];
    }
    if (!volumeData) {
        volumeData = surfaceData;
    }

    vtkDataSet* currentRenderInput = nullptr;
    if (m_VolumeRenderMapper) {
        currentRenderInput = vtkDataSet::SafeDownCast(m_VolumeRenderMapper->GetInput());
    }

    if (quality == m_VolumeQuality &&
        surfaceData == m_VolumeDataSet &&
        currentRenderInput == volumeData) {
        return;
    }

    m_VolumeQuality = quality;

    if (surfaceData) {
        if (!m_VolumeMeshMapper || !m_VolumeMeshActor) {
            m_VolumeDataSet = surfaceData;
            ensureVolumeMeshPipeline();
        } else if (m_VolumeDataSet != surfaceData) {
            m_VolumeDataSet = surfaceData;
            m_VolumeMeshMapper->SetInputData(m_VolumeDataSet);
            m_VolumeMeshMapper->Update();
            updateVolumeDataRange(m_VolumeDataSet);
        }
    }

    if (volumeData) {
        if (!m_VolumeRenderMapper || !m_VolumeRenderProperty || !m_VolumeRenderActor) {
            ensureVolumeRenderPipeline(volumeData);
        } else if (currentRenderInput != volumeData) {
            m_VolumeRenderMapper->SetInputData(volumeData);
            m_VolumeRenderMapper->Update();
            updateVolumeDataRange(volumeData);
            updateVolumeColorTransfer();
            updateVolumeOpacityFunction();
            updateVolumeSampleDistance();
        } else {
            updateVolumeSampleDistance();
        }
    }

    updateVolumeRenderVisibility();
    VtkViewer::Instance().RequestRender();
}

void Mesh::SetVolumeQualityDataSets(vtkSmartPointer<vtkDataSet> highSurface,
    vtkSmartPointer<vtkDataSet> mediumSurface,
    vtkSmartPointer<vtkDataSet> lowSurface,
    vtkSmartPointer<vtkDataSet> highVolume,
    vtkSmartPointer<vtkDataSet> mediumVolume,
    vtkSmartPointer<vtkDataSet> lowVolume) {
    m_VolumeMeshQualityDataSets[static_cast<int>(VolumeQuality::High)] = highSurface;
    m_VolumeMeshQualityDataSets[static_cast<int>(VolumeQuality::Medium)] = mediumSurface;
    m_VolumeMeshQualityDataSets[static_cast<int>(VolumeQuality::Low)] = lowSurface;
    m_VolumeRenderQualityDataSets[static_cast<int>(VolumeQuality::High)] = highVolume ? highVolume : highSurface;
    m_VolumeRenderQualityDataSets[static_cast<int>(VolumeQuality::Medium)] = mediumVolume ? mediumVolume : mediumSurface;
    m_VolumeRenderQualityDataSets[static_cast<int>(VolumeQuality::Low)] = lowVolume ? lowVolume : lowSurface;
    SetVolumeQuality(m_VolumeQuality);
}

void Mesh::SetMeshGroups(std::vector<MeshGroupUPtr>&& meshGroups) {
    if (meshGroups.empty()) {
        return;
    }

    m_MeshGroups = std::move(meshGroups);
    
    for (auto& meshGroup : m_MeshGroups) {
        VtkViewer::Instance().AddActor(meshGroup->GetGroupActor());
    }
}

void Mesh::DeleteMeshGroup(int32_t groupId) {
    auto it = std::remove_if(m_MeshGroups.begin(), m_MeshGroups.end(),
        [groupId](const MeshGroupUPtr& meshGroup) {
            return meshGroup->GetId() == groupId;
        });

    if (it != m_MeshGroups.end()) {
        VtkViewer::Instance().RemoveActor((*it)->GetGroupActor());
        m_MeshGroups.erase(it, m_MeshGroups.end());
    }
}

const MeshGroup* Mesh::GetMeshGroupById(int32_t groupId) const {
    for (const auto& meshGroup : m_MeshGroups) {
        if (meshGroup->GetId() == groupId) {
            return meshGroup.get();
        }
    }
    return nullptr;
}

MeshGroup* Mesh::GetMeshGroupByIdMutable(int32_t groupId) {
    for (auto& meshGroup : m_MeshGroups) {
        if (meshGroup->GetId() == groupId) {
            return meshGroup.get();
        }
    }
    return nullptr;
}

void Mesh::ensureVolumeMeshPipeline() {
    if (!m_VolumeDataSet) {
        return;
    }
    if (!m_VolumeMeshMapper) {
        m_VolumeMeshMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    }
    m_VolumeMeshMapper->SetInputData(m_VolumeDataSet);
    m_VolumeMeshMapper->Update();

    if (!m_VolumeMeshActor) {
        m_VolumeMeshActor = vtkSmartPointer<vtkActor>::New();
        m_VolumeMeshActor->SetMapper(m_VolumeMeshMapper);
        SetVolumeMeshColor(0.95, 0.95, 0.95);
        SetVolumeMeshOpacity(1.0);
        SetVolumeMeshEdgeColor(0.0, 0.0, 0.0);
        updateVolumeRenderVisibility();
        SetDisplayMode(Toolbar::Instance().GetMeshDisplayMode());
    } else {
        m_VolumeMeshActor->SetMapper(m_VolumeMeshMapper);
    }
    updateVolumeDataRange(m_VolumeDataSet);
}

void Mesh::ensureVolumeRenderPipeline(vtkSmartPointer<vtkDataSet> dataSet) {
    if (!dataSet) {
        return;
    }
    if (!vtkImageData::SafeDownCast(dataSet) &&
        !vtkRectilinearGrid::SafeDownCast(dataSet)) {
        SPDLOG_WARN("Volume rendering requires vtkImageData or vtkRectilinearGrid. Falling back to surface.");
        m_VolumeRenderMode = VolumeRenderMode::Surface;
        return;
    }
    if (!m_VolumeRenderMapper) {
        m_VolumeRenderMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
        m_VolumeRenderMapper->SetInteractiveAdjustSampleDistances(false);
        m_VolumeRenderMapper->SetAutoAdjustSampleDistances(false);
    }
    m_VolumeRenderMapper->SetInputData(dataSet);
    m_VolumeRenderMapper->Update();

    if (!m_VolumeRenderProperty) {
        m_VolumeRenderProperty = vtkSmartPointer<vtkVolumeProperty>::New();
        m_VolumeRenderProperty->ShadeOff();
        m_VolumeRenderProperty->SetInterpolationTypeToLinear();
    }
    if (!m_VolumeRenderColor) {
        m_VolumeRenderColor = vtkSmartPointer<vtkColorTransferFunction>::New();
    }
    if (!m_VolumeRenderOpacity) {
        m_VolumeRenderOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    }
    if (!m_VolumeRenderActor) {
        m_VolumeRenderActor = vtkSmartPointer<vtkVolume>::New();
        m_VolumeRenderActor->SetMapper(m_VolumeRenderMapper);
        m_VolumeRenderActor->SetProperty(m_VolumeRenderProperty);
    }
    if (!m_VolumeRenderAdded) {
        VtkViewer::Instance().AddVolume(m_VolumeRenderActor, false);
        m_VolumeRenderAdded = true;
    }

    updateVolumeDataRange(dataSet);
    updateVolumeColorTransfer();
    updateVolumeOpacityFunction();
    updateVolumeSampleDistance();
    updateVolumeRenderVisibility();
}

void Mesh::updateVolumeDataRange(vtkSmartPointer<vtkDataSet> dataSet) {
    if (!dataSet) {
        m_VolumeDataRange[0] = 0.0;
        m_VolumeDataRange[1] = 1.0;
        if (m_VolumeWindowLevelAuto) {
            m_VolumeWindow = 1.0;
            m_VolumeLevel = 0.5;
        }
        return;
    }

    double range[2] = { 0.0, 1.0 };
    vtkPointData* pointData = dataSet->GetPointData();
    if (pointData && pointData->GetScalars()) {
        pointData->GetScalars()->GetRange(range);
    } else {
        dataSet->GetScalarRange(range);
    }

    m_VolumeDataRange[0] = range[0];
    m_VolumeDataRange[1] = range[1];
    if (m_VolumeDataRange[0] == m_VolumeDataRange[1]) {
        m_VolumeDataRange[1] = m_VolumeDataRange[0] + 1.0;
    }

    if (m_VolumeWindowLevelAuto) {
        m_VolumeWindow = m_VolumeDataRange[1] - m_VolumeDataRange[0];
        m_VolumeLevel = (m_VolumeDataRange[0] + m_VolumeDataRange[1]) * 0.5;
        return;
    }

    const VolumeWindowLevelRange resolved = resolveVolumeWindowLevelRange(
        m_VolumeDataRange[0], m_VolumeDataRange[1], m_VolumeWindow, m_VolumeLevel);
    m_VolumeWindow = resolved.window;
    m_VolumeLevel = resolved.level;
}

void Mesh::updateVolumeColorTransfer() {
    if (!m_VolumeRenderColor) {
        m_VolumeRenderColor = vtkSmartPointer<vtkColorTransferFunction>::New();
    }

    const VolumeWindowLevelRange resolved = resolveVolumeWindowLevelRange(
        m_VolumeDataRange[0], m_VolumeDataRange[1], m_VolumeWindow, m_VolumeLevel);
    const double rangeMin = resolved.rangeMin;
    const double rangeMax = resolved.rangeMax;
    const double midValue = (rangeMin + rangeMax) * 0.5;

    common::ApplyColorMapToTransferFunction(
        m_VolumeRenderColor,
        m_VolumeColorPreset,
        rangeMin,
        midValue,
        rangeMax,
        m_VolumeColorMidpoint,
        m_VolumeColorSharpness);

    if (m_VolumeRenderProperty) {
        m_VolumeRenderProperty->SetColor(m_VolumeRenderColor);
    }
    if (m_VolumeMeshMapper) {
        m_VolumeMeshMapper->SetLookupTable(m_VolumeRenderColor);
        m_VolumeMeshMapper->SetColorModeToMapScalars();
        m_VolumeMeshMapper->ScalarVisibilityOn();
        m_VolumeMeshMapper->SetScalarRange(rangeMin, rangeMax);
    }
}

void Mesh::updateVolumeOpacityFunction() {
    if (!m_VolumeRenderOpacity || !m_VolumeRenderProperty) {
        return;
    }

    const VolumeWindowLevelRange resolved = resolveVolumeWindowLevelRange(
        m_VolumeDataRange[0], m_VolumeDataRange[1], m_VolumeWindow, m_VolumeLevel);
    const double rangeMin = resolved.rangeMin;
    const double rangeMax = resolved.rangeMax;

    double opacityMin = m_VolumeRenderOpacityMin;
    double opacityMax = m_VolumeRenderOpacityMax;
    if (opacityMin < 0.0) opacityMin = 0.0;
    if (opacityMin > 1.0) opacityMin = 1.0;
    if (opacityMax < 0.0) opacityMax = 0.0;
    if (opacityMax > 1.0) opacityMax = 1.0;

    m_VolumeRenderOpacity->RemoveAllPoints();
    m_VolumeRenderOpacity->AddPoint(rangeMin, opacityMin);
    m_VolumeRenderOpacity->AddPoint(rangeMax, opacityMax);
    m_VolumeRenderProperty->SetScalarOpacity(m_VolumeRenderOpacity);
}

void Mesh::updateVolumeSampleDistance() {
    if (!m_VolumeRenderMapper) {
        return;
    }
    static const double ratios[] = { 0.1, 0.5, 1.0, 2.0, 5.0, 10.0 };
    int index = m_VolumeSampleDistanceIndex;
    if (index < 0) index = 0;
    if (index > 5) index = 5;

    double baseDistance = 1.0;
    vtkSmartPointer<vtkDataSet> volumeData = m_VolumeRenderQualityDataSets[static_cast<int>(m_VolumeQuality)];
    if (!volumeData) {
        volumeData = m_VolumeDataSet;
    }
    vtkImageData* image = vtkImageData::SafeDownCast(volumeData);
    if (image) {
        double spacing[3] = { 1.0, 1.0, 1.0 };
        image->GetSpacing(spacing);
        baseDistance = std::min(spacing[0], std::min(spacing[1], spacing[2]));
        if (baseDistance <= 0.0) {
            baseDistance = 1.0;
        }
    }
    m_VolumeRenderMapper->SetSampleDistance(baseDistance * ratios[index]);
}

void Mesh::updateVolumeRenderVisibility() {
    if (m_VolumeMeshSuppressed) {
        if (m_VolumeMeshActor) {
            m_VolumeMeshActor->SetVisibility(false);
        }
        if (m_VolumeRenderActor) {
            m_VolumeRenderActor->SetVisibility(false);
        }
        return;
    }
    if (m_VolumeMeshActor) {
        m_VolumeMeshActor->SetVisibility(m_VolumeMeshVisibility && m_VolumeSurfaceVisibility);
    }
    if (m_VolumeRenderActor) {
        m_VolumeRenderActor->SetVisibility(m_VolumeMeshVisibility && m_VolumeRenderVisibility);
    }
}
