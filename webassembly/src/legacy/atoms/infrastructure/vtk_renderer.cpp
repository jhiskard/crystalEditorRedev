// atoms/infrastructure/vtk_renderer.cpp
#include "vtk_renderer.h"
#include "../domain/special_points.h"
#include "../../config/log_config.h"
#include <vtkSphereSource.h>
#include <vtkCylinderSource.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>

// 기존 include 목록에 추가
#include "bz_plot_layer.h"
#include <vtkPolyLine.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCoordinate.h>
#include <vtkArrowSource.h>
#include <vtkVectorText.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3D.h>
#include <vtkPoints.h>

namespace atoms {
namespace infrastructure {

namespace {
void updateLabelActor2D(vtkSmartPointer<vtkActor2D> actor, const VTKRenderer::LabelActorSpec& label) {
    if (!actor) {
        return;
    }

    if (vtkTextActor* textActor = vtkTextActor::SafeDownCast(actor)) {
        textActor->SetInput(label.text.c_str());
        if (vtkCoordinate* coord = textActor->GetPositionCoordinate()) {
            coord->SetValue(label.worldPosition[0], label.worldPosition[1], label.worldPosition[2]);
        }
        textActor->SetVisibility(label.visible ? 1 : 0);
        textActor->Modified();
        return;
    }

    actor->SetVisibility(label.visible ? 1 : 0);
    actor->Modified();
}
} // namespace

// ============================================================================
// 생성자 / 소멸자
// ============================================================================

VTKRenderer::VTKRenderer() 
    : m_sphereResolution(20)
    , m_bondThickness(1.0f)
    , m_bondOpacity(1.0f)
    , m_bzPlotVisible(false) {
    SPDLOG_DEBUG("VTKRenderer initialized");
}

VTKRenderer::~VTKRenderer() {
    SPDLOG_DEBUG("VTKRenderer destructor - cleaning up all VTK resources");
    
    try {
        clearAllAtomGroupsVTK();
        clearAllBondGroups();
        clearUnitCell();
        clearAllStructureLabelActors();
        clearBZPlot(); 
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error during VTKRenderer cleanup: {}", e.what());
    }
    
    SPDLOG_DEBUG("VTKRenderer destroyed");
}

// ============================================================================
// 원자 그룹 렌더링
// ============================================================================

void VTKRenderer::initializeAtomGroupVTK(const std::string& symbol, float radius) {
    if (m_atomGroups.find(symbol) != m_atomGroups.end() && 
        m_atomGroups[symbol].isInitialized()) {
        SPDLOG_DEBUG("Atom group {} already initialized", symbol);
        return;
    }
    
    SPDLOG_DEBUG("Initializing VTK pipeline for atom group: {}", symbol);
    
    try {
        AtomGroupVTK& group = m_atomGroups[symbol];
        group.baseRadius = radius;
        
        // 기본 구 geometry 생성
        group.baseGeometry = createSphereGeometry(0.5f); // 기본 반지름 0.5
        
        // Appender 초기화
        group.appender = vtkSmartPointer<vtkAppendPolyData>::New();
        vtkSmartPointer<vtkPolyData> emptyPolyData = vtkSmartPointer<vtkPolyData>::New();
        group.appender->AddInputData(emptyPolyData);
        
        // Mapper 초기화
        group.mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        group.mapper->SetInputConnection(group.appender->GetOutputPort());
        
        // Actor 초기화
        group.actor = vtkSmartPointer<vtkActor>::New();
        group.actor->SetMapper(group.mapper);
        setupActorProperties(group.actor);
        
        // 렌더러에 추가 (편집 동작 중 카메라 시점 유지)
        VtkViewer::Instance().AddActor(group.actor, false);
        
        SPDLOG_DEBUG("Successfully initialized VTK pipeline for atom group: {}", symbol);
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error initializing VTK pipeline for atom group {}: {}", symbol, e.what());
        throw;
    }
}

void VTKRenderer::updateAtomGroupVTK(
    const std::string& symbol,
    const std::vector<vtkSmartPointer<vtkTransform>>& transforms,
    // const std::vector<ImVec4>& colors
    const std::vector<atoms::domain::Color4f>& colors
) {
    
    if (m_atomGroups.find(symbol) == m_atomGroups.end()) {
        SPDLOG_ERROR("Atom group {} not found", symbol);
        return;
    }
    
    auto& group = m_atomGroups[symbol];
    
    if (!group.isInitialized()) {
        SPDLOG_WARN("Atom group {} not initialized, initializing now", symbol);
        initializeAtomGroupVTK(symbol, group.baseRadius);
    }
    
    if (transforms.size() != colors.size()) {
        SPDLOG_ERROR("Size mismatch for atom group {}: transforms={}, colors={}", 
                    symbol, transforms.size(), colors.size());
        return;
    }
    
    try {
        // Appender 재설정
        group.appender->RemoveAllInputs();
        
        if (transforms.empty()) {
            // 빈 그룹 처리
            vtkSmartPointer<vtkPolyData> emptyPolyData = vtkSmartPointer<vtkPolyData>::New();
            group.appender->AddInputData(emptyPolyData);
            group.appender->Update();
            
            if (group.mapper) group.mapper->Update();
            if (group.actor) group.actor->Modified();
            
            SPDLOG_DEBUG("Updated empty atom group: {}", symbol);
            return;
        }
        
        // 색상 배열 준비
        vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
        colorArray->SetNumberOfComponents(3);
        colorArray->SetName((symbol + "_colors").c_str());
        
        size_t validInstances = 0;
        
        // 각 인스턴스 처리
        for (size_t i = 0; i < transforms.size(); i++) {
            if (!transforms[i]) {
                SPDLOG_WARN("Transform {} is null for atom group: {}", i, symbol);
                continue;
            }
            
            // Transform 적용
            vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = 
                vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            transformFilter->SetInputData(group.baseGeometry);
            transformFilter->SetTransform(transforms[i]);
            transformFilter->Update();
            
            vtkSmartPointer<vtkPolyData> transformedData = transformFilter->GetOutput();
            if (transformedData && transformedData->GetNumberOfPoints() > 0) {
                group.appender->AddInputData(transformedData);
                
                // 색상 데이터 추가
                // ImVec4 color = colors[i];
                atoms::domain::Color4f color = colors[i];
                // color.x = std::max(0.0f, std::min(1.0f, color.x));
                // color.y = std::max(0.0f, std::min(1.0f, color.y));
                // color.z = std::max(0.0f, std::min(1.0f, color.z));
                color.r = std::max(0.0f, std::min(1.0f, color.r));
                color.g = std::max(0.0f, std::min(1.0f, color.g));
                color.b = std::max(0.0f, std::min(1.0f, color.b));

                // unsigned char r = static_cast<unsigned char>(color.x * 255.0f);
                // unsigned char g = static_cast<unsigned char>(color.y * 255.0f);
                // unsigned char b = static_cast<unsigned char>(color.z * 255.0f);
                unsigned char r = static_cast<unsigned char>(color.r * 255.0f);
                unsigned char g = static_cast<unsigned char>(color.g * 255.0f);
                unsigned char b = static_cast<unsigned char>(color.b * 255.0f);
                
                vtkIdType numCells = transformedData->GetNumberOfCells();
                for (vtkIdType j = 0; j < numCells; j++) {
                    colorArray->InsertNextTuple3(r, g, b);
                }
                
                validInstances++;
            }
        }
        
        // 파이프라인 업데이트
        group.appender->Update();
        
        // 색상 배열 설정
        vtkPolyData* output = group.appender->GetOutput();
        if (output && colorArray->GetNumberOfTuples() > 0) {
            output->GetCellData()->SetScalars(colorArray);
        }
        
        if (group.mapper) group.mapper->Update();
        if (group.actor) group.actor->Modified();
        
        SPDLOG_DEBUG("Updated atom group {} with {}/{} valid instances", 
                    symbol, validInstances, transforms.size());
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error updating VTK pipeline for atom group {}: {}", symbol, e.what());
    }
}

void VTKRenderer::clearAtomGroupVTK(const std::string& symbol) {
    if (m_atomGroups.find(symbol) == m_atomGroups.end()) {
        SPDLOG_DEBUG("Atom group {} not found for clearing", symbol);
        return;
    }
    
    SPDLOG_DEBUG("Clearing VTK pipeline for atom group: {}", symbol);
    
    auto& group = m_atomGroups[symbol];
    
    try {
        if (group.actor) {
            VtkViewer::Instance().RemoveActor(group.actor);
            group.actor = nullptr;
        }
        
        if (group.appender) {
            group.appender->RemoveAllInputs();
            group.appender = nullptr;
        }
        
        group.mapper = nullptr;
        group.baseGeometry = nullptr;
        
        m_atomGroups.erase(symbol);
        
        SPDLOG_DEBUG("Successfully cleared VTK pipeline for atom group: {}", symbol);
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error clearing VTK pipeline for atom group {}: {}", symbol, e.what());
    }
}

void VTKRenderer::clearAllAtomGroupsVTK() {
    SPDLOG_INFO("Clearing all atom group VTK pipelines");
    
    for (auto& [symbol, group] : m_atomGroups) {
        try {
            if (group.actor) {
                VtkViewer::Instance().RemoveActor(group.actor);
            }
            if (group.appender) {
                group.appender->RemoveAllInputs();
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Error clearing atom group {}: {}", symbol, e.what());
        }
    }
    
    m_atomGroups.clear();
    clearAllAtomLabelActors();
    
    SPDLOG_INFO("All atom group VTK pipelines cleared");
}

bool VTKRenderer::isAtomGroupInitialized(const std::string& symbol) const {
    auto it = m_atomGroups.find(symbol);
    return it != m_atomGroups.end() && it->second.isInitialized();
}

// ============================================================================
// 결합 그룹 렌더링
// ============================================================================

void VTKRenderer::initializeBondGroup(const std::string& bondTypeKey, float radius) {
    if (m_bondGroups.find(bondTypeKey) != m_bondGroups.end() && 
        m_bondGroups[bondTypeKey].isInitialized()) {
        SPDLOG_DEBUG("Bond group {} already initialized", bondTypeKey);
        return;
    }
    
    SPDLOG_DEBUG("Initializing 2-color bond group for key: {}", bondTypeKey);
    
    try {
        BondGroupVTK& group = m_bondGroups[bondTypeKey];
        group.baseRadius = radius;
        
        // 기본 실린더 geometry 생성
        float adjustedRadius = radius * m_bondThickness;
        group.baseGeometry = createCylinderGeometry(adjustedRadius, 1.0f);
        
        // 첫 번째 색상 파이프라인
        group.appender1 = vtkSmartPointer<vtkAppendPolyData>::New();
        vtkSmartPointer<vtkPolyData> emptyPolyData1 = vtkSmartPointer<vtkPolyData>::New();
        group.appender1->AddInputData(emptyPolyData1);
        
        group.mapper1 = vtkSmartPointer<vtkPolyDataMapper>::New();
        group.mapper1->SetInputConnection(group.appender1->GetOutputPort());
        
        group.actor1 = vtkSmartPointer<vtkActor>::New();
        group.actor1->SetMapper(group.mapper1);
        group.actor1->SetPickable(false);  // ← 추가
        setupActorProperties(group.actor1, true);
        
        // 두 번째 색상 파이프라인
        group.appender2 = vtkSmartPointer<vtkAppendPolyData>::New();
        vtkSmartPointer<vtkPolyData> emptyPolyData2 = vtkSmartPointer<vtkPolyData>::New();
        group.appender2->AddInputData(emptyPolyData2);
        
        group.mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
        group.mapper2->SetInputConnection(group.appender2->GetOutputPort());
        
        group.actor2 = vtkSmartPointer<vtkActor>::New();
        group.actor2->SetMapper(group.mapper2);
        group.actor2->SetPickable(false);  // ← 추가
        setupActorProperties(group.actor2, true);
        
        // 렌더러에 추가 (편집 동작 중 카메라 시점 유지)
        VtkViewer::Instance().AddActor(group.actor1, false);
        VtkViewer::Instance().AddActor(group.actor2, false);
        
        SPDLOG_DEBUG("Successfully initialized 2-color bond group: {}", bondTypeKey);
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error initializing bond group {}: {}", bondTypeKey, e.what());
        throw;
    }
}

void VTKRenderer::updateBondGroup(
    const std::string& bondTypeKey,
    const std::vector<vtkSmartPointer<vtkTransform>>& transforms1,
    const std::vector<vtkSmartPointer<vtkTransform>>& transforms2,
    // const std::vector<ImVec4>& colors1, const std::vector<ImVec4>& colors2
    const std::vector<atoms::domain::Color4f>& colors1, 
    const std::vector<atoms::domain::Color4f>& colors2
) {
    
    if (m_bondGroups.find(bondTypeKey) == m_bondGroups.end()) {
        SPDLOG_ERROR("Bond group {} not found", bondTypeKey);
        return;
    }
    
    auto& group = m_bondGroups[bondTypeKey];
    
    if (!group.isInitialized()) {
        SPDLOG_WARN("Bond group {} not initialized, initializing now", bondTypeKey);
        initializeBondGroup(bondTypeKey, group.baseRadius);
    }
    
    if (transforms1.size() != transforms2.size() || 
        transforms1.size() != colors1.size() ||
        transforms1.size() != colors2.size()) {
        SPDLOG_ERROR("Size mismatch for bond group {}: t1={}, t2={}, c1={}, c2={}", 
                    bondTypeKey, transforms1.size(), transforms2.size(), 
                    colors1.size(), colors2.size());
        return;
    }
    
    try {
        // Appender 재설정
        group.appender1->RemoveAllInputs();
        group.appender2->RemoveAllInputs();
        
        if (transforms1.empty()) {
            // 빈 그룹 처리
            vtkSmartPointer<vtkPolyData> emptyPolyData1 = vtkSmartPointer<vtkPolyData>::New();
            vtkSmartPointer<vtkPolyData> emptyPolyData2 = vtkSmartPointer<vtkPolyData>::New();
            group.appender1->AddInputData(emptyPolyData1);
            group.appender2->AddInputData(emptyPolyData2);
            group.appender1->Update();
            group.appender2->Update();
            
            if (group.mapper1) group.mapper1->Update();
            if (group.mapper2) group.mapper2->Update();
            if (group.actor1) group.actor1->Modified();
            if (group.actor2) group.actor2->Modified();
            
            SPDLOG_DEBUG("Updated empty bond group: {}", bondTypeKey);
            return;
        }
        
        // 색상 배열 준비
        vtkSmartPointer<vtkUnsignedCharArray> colorArray1 = vtkSmartPointer<vtkUnsignedCharArray>::New();
        colorArray1->SetNumberOfComponents(3);
        colorArray1->SetName((bondTypeKey + "_colors1").c_str());
        
        vtkSmartPointer<vtkUnsignedCharArray> colorArray2 = vtkSmartPointer<vtkUnsignedCharArray>::New();
        colorArray2->SetNumberOfComponents(3);
        colorArray2->SetName((bondTypeKey + "_colors2").c_str());
        
        size_t validInstances = 0;
        
        // 각 인스턴스 처리
        for (size_t i = 0; i < transforms1.size(); i++) {
            if (!transforms1[i] || !transforms2[i]) {
                SPDLOG_WARN("Transform {} is null for bond group: {}", i, bondTypeKey);
                continue;
            }
            
            // 첫 번째 색상 transform
            vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter1 = 
                vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            transformFilter1->SetInputData(group.baseGeometry);
            transformFilter1->SetTransform(transforms1[i]);
            transformFilter1->Update();
            
            // 두 번째 색상 transform
            vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter2 = 
                vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            transformFilter2->SetInputData(group.baseGeometry);
            transformFilter2->SetTransform(transforms2[i]);
            transformFilter2->Update();
            
            vtkSmartPointer<vtkPolyData> transformedData1 = transformFilter1->GetOutput();
            vtkSmartPointer<vtkPolyData> transformedData2 = transformFilter2->GetOutput();
            
            if (transformedData1 && transformedData1->GetNumberOfPoints() > 0 &&
                transformedData2 && transformedData2->GetNumberOfPoints() > 0) {
                
                group.appender1->AddInputData(transformedData1);
                group.appender2->AddInputData(transformedData2);
                
                // 색상 데이터 추가
                // ImVec4 color1 = colors1[i];
                atoms::domain::Color4f color1 = colors1[i];
                // color1.x = std::max(0.0f, std::min(1.0f, color1.x));
                // color1.y = std::max(0.0f, std::min(1.0f, color1.y));
                // color1.z = std::max(0.0f, std::min(1.0f, color1.z));
                color1.r = std::max(0.0f, std::min(1.0f, color1.r));
                color1.g = std::max(0.0f, std::min(1.0f, color1.g));
                color1.b = std::max(0.0f, std::min(1.0f, color1.b));
                
                // unsigned char r1 = static_cast<unsigned char>(color1.x * 255.0f);
                // unsigned char g1 = static_cast<unsigned char>(color1.y * 255.0f);
                // unsigned char b1 = static_cast<unsigned char>(color1.z * 255.0f);
                unsigned char r1 = static_cast<unsigned char>(color1.r * 255.0f);
                unsigned char g1 = static_cast<unsigned char>(color1.g * 255.0f);
                unsigned char b1 = static_cast<unsigned char>(color1.b * 255.0f);
                
                vtkIdType numCells1 = transformedData1->GetNumberOfCells();
                for (vtkIdType j = 0; j < numCells1; j++) {
                    colorArray1->InsertNextTuple3(r1, g1, b1);
                }
                
                // ImVec4 color2 = colors2[i];
                atoms::domain::Color4f color2 = colors2[i];
                // color2.x = std::max(0.0f, std::min(1.0f, color2.x));
                // color2.y = std::max(0.0f, std::min(1.0f, color2.y));
                // color2.z = std::max(0.0f, std::min(1.0f, color2.z));
                color2.r = std::max(0.0f, std::min(1.0f, color2.r));
                color2.g = std::max(0.0f, std::min(1.0f, color2.g));
                color2.b = std::max(0.0f, std::min(1.0f, color2.b));
                
                // unsigned char r2 = static_cast<unsigned char>(color2.x * 255.0f);
                // unsigned char g2 = static_cast<unsigned char>(color2.y * 255.0f);
                // unsigned char b2 = static_cast<unsigned char>(color2.z * 255.0f);
                unsigned char r2 = static_cast<unsigned char>(color2.r * 255.0f);
                unsigned char g2 = static_cast<unsigned char>(color2.g * 255.0f);
                unsigned char b2 = static_cast<unsigned char>(color2.b * 255.0f);
                
                vtkIdType numCells2 = transformedData2->GetNumberOfCells();
                for (vtkIdType j = 0; j < numCells2; j++) {
                    colorArray2->InsertNextTuple3(r2, g2, b2);
                }
                
                validInstances++;
            }
        }
        
        // 파이프라인 업데이트
        group.appender1->Update();
        group.appender2->Update();
        
        // 색상 배열 설정
        vtkPolyData* output1 = group.appender1->GetOutput();
        vtkPolyData* output2 = group.appender2->GetOutput();
        
        if (output1 && colorArray1->GetNumberOfTuples() > 0) {
            output1->GetCellData()->SetScalars(colorArray1);
        }
        if (output2 && colorArray2->GetNumberOfTuples() > 0) {
            output2->GetCellData()->SetScalars(colorArray2);
        }
        
        if (group.mapper1) group.mapper1->Update();
        if (group.mapper2) group.mapper2->Update();
        if (group.actor1) group.actor1->Modified();
        if (group.actor2) group.actor2->Modified();
        
        SPDLOG_DEBUG("Updated 2-color bond group {} with {}/{} valid instances", 
                    bondTypeKey, validInstances, transforms1.size());
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error updating bond group {}: {}", bondTypeKey, e.what());
    }
}

void VTKRenderer::clearBondGroup(const std::string& bondTypeKey) {
    if (m_bondGroups.find(bondTypeKey) == m_bondGroups.end()) {
        SPDLOG_DEBUG("Bond group {} not found for clearing", bondTypeKey);
        return;
    }
    
    SPDLOG_DEBUG("Clearing bond group: {}", bondTypeKey);
    
    auto& group = m_bondGroups[bondTypeKey];
    
    try {
        if (group.actor1) {
            VtkViewer::Instance().RemoveActor(group.actor1);
            group.actor1 = nullptr;
        }
        
        if (group.actor2) {
            VtkViewer::Instance().RemoveActor(group.actor2);
            group.actor2 = nullptr;
        }
        
        if (group.appender1) {
            group.appender1->RemoveAllInputs();
            group.appender1 = nullptr;
        }
        
        if (group.appender2) {
            group.appender2->RemoveAllInputs();
            group.appender2 = nullptr;
        }
        
        group.mapper1 = nullptr;
        group.mapper2 = nullptr;
        group.baseGeometry = nullptr;
        
        m_bondGroups.erase(bondTypeKey);
        
        SPDLOG_DEBUG("Successfully cleared bond group: {}", bondTypeKey);
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error clearing bond group {}: {}", bondTypeKey, e.what());
    }
}

void VTKRenderer::clearAllBondGroups() {
    SPDLOG_INFO("Clearing all bond groups");
    
    for (auto& [bondTypeKey, group] : m_bondGroups) {
        try {
            if (group.actor1) {
                VtkViewer::Instance().RemoveActor(group.actor1);
            }
            if (group.actor2) {
                VtkViewer::Instance().RemoveActor(group.actor2);
            }
            if (group.appender1) {
                group.appender1->RemoveAllInputs();
            }
            if (group.appender2) {
                group.appender2->RemoveAllInputs();
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Error clearing bond group {}: {}", bondTypeKey, e.what());
        }
    }
    
    m_bondGroups.clear();
    clearAllBondLabelActors();
    
    SPDLOG_INFO("All bond groups cleared");
}

void VTKRenderer::updateAllBondGroupThickness(float thickness) {
    m_bondThickness = thickness;
    
    SPDLOG_DEBUG("Updating bond thickness for all groups to {:.1f}", thickness);
    
    std::vector<std::string> groupsToUpdate;
    
    for (auto& [bondTypeKey, group] : m_bondGroups) {
        if (group.isInitialized()) {
            // 새로운 geometry 생성
            float newRadius = group.baseRadius * thickness;
            group.baseGeometry = createCylinderGeometry(newRadius, 1.0f);
            
            groupsToUpdate.push_back(bondTypeKey);
            
            SPDLOG_DEBUG("Updated thickness for bond group: {} (radius: {:.3f})", 
                        bondTypeKey, newRadius);
        }
    }
    
    SPDLOG_DEBUG("Marked {} bond groups for geometry update", groupsToUpdate.size());
}

void VTKRenderer::updateAllBondGroupOpacity(float opacity) {
    m_bondOpacity = opacity;
    
    SPDLOG_DEBUG("Updating bond opacity for all groups to {:.1f}", opacity);
    
    for (auto& [bondTypeKey, group] : m_bondGroups) {
        if (group.actor1) {
            vtkProperty* property1 = group.actor1->GetProperty();
            if (property1) {
                property1->SetOpacity(opacity);
                
                if (opacity < 1.0f) {
                    property1->SetRenderLinesAsTubes(true);
                    group.actor1->ForceTranslucentOn();
                } else {
                    group.actor1->ForceOpaqueOn();
                }
                
                group.actor1->Modified();
            }
        }
        
        if (group.actor2) {
            vtkProperty* property2 = group.actor2->GetProperty();
            if (property2) {
                property2->SetOpacity(opacity);
                
                if (opacity < 1.0f) {
                    property2->SetRenderLinesAsTubes(true);
                    group.actor2->ForceTranslucentOn();
                } else {
                    group.actor2->ForceOpaqueOn();
                }
                
                group.actor2->Modified();
            }
        }
        
        SPDLOG_DEBUG("Updated opacity for bond group: {} (opacity: {:.1f})", 
                    bondTypeKey, opacity);
    }
}

// ============================================================================
// 유닛 셀 렌더링
// ============================================================================

void VTKRenderer::createUnitCell(const float matrix[3][3]) {
    createUnitCell(-1, matrix);
}

void VTKRenderer::createUnitCell(int32_t structureId, const float matrix[3][3]) {
    clearUnitCell(structureId);
    
    SPDLOG_DEBUG("Creating unit cell visualization");
    
    // 8개의 꼭지점 계산
    float vertices[8][3] = {
        {0.0f, 0.0f, 0.0f},  // 원점 (0,0,0)
        {0.0f, 0.0f, 0.0f},  // (1,0,0)
        {0.0f, 0.0f, 0.0f},  // (0,1,0)
        {0.0f, 0.0f, 0.0f},  // (1,1,0)
        {0.0f, 0.0f, 0.0f},  // (0,0,1)
        {0.0f, 0.0f, 0.0f},  // (1,0,1)
        {0.0f, 0.0f, 0.0f},  // (0,1,1)
        {0.0f, 0.0f, 0.0f}   // (1,1,1)
    };
    
    // 꼭지점 위치 계산
    vertices[1][0] = matrix[0][0];
    vertices[1][1] = matrix[0][1];
    vertices[1][2] = matrix[0][2];
    
    vertices[2][0] = matrix[1][0];
    vertices[2][1] = matrix[1][1];
    vertices[2][2] = matrix[1][2];
    
    vertices[3][0] = matrix[0][0] + matrix[1][0];
    vertices[3][1] = matrix[0][1] + matrix[1][1];
    vertices[3][2] = matrix[0][2] + matrix[1][2];
    
    vertices[4][0] = matrix[2][0];
    vertices[4][1] = matrix[2][1];
    vertices[4][2] = matrix[2][2];
    
    vertices[5][0] = matrix[0][0] + matrix[2][0];
    vertices[5][1] = matrix[0][1] + matrix[2][1];
    vertices[5][2] = matrix[0][2] + matrix[2][2];
    
    vertices[6][0] = matrix[1][0] + matrix[2][0];
    vertices[6][1] = matrix[1][1] + matrix[2][1];
    vertices[6][2] = matrix[1][2] + matrix[2][2];
    
    vertices[7][0] = matrix[0][0] + matrix[1][0] + matrix[2][0];
    vertices[7][1] = matrix[0][1] + matrix[1][1] + matrix[2][1];
    vertices[7][2] = matrix[0][2] + matrix[1][2] + matrix[2][2];
    
    // 12개의 모서리 정의
    int edges[12][2] = {
        {0, 1}, {0, 2}, {0, 4},  // 원점에서 나가는 3개 모서리
        {1, 3}, {1, 5},          // v1에서 나가는 2개 모서리
        {2, 3}, {2, 6},          // v2에서 나가는 2개 모서리
        {3, 7},                  // v1+v2에서 나가는 1개 모서리
        {4, 5}, {4, 6},          // v3에서 나가는 2개 모서리
        {5, 7}, {6, 7}           // 나머지 2개 모서리
    };
    
    bool storedVisible = true;
    auto itVisible = m_unitCellVisibleByStructure.find(structureId);
    if (itVisible != m_unitCellVisibleByStructure.end()) {
        storedVisible = itVisible->second;
    }
    bool effectiveVisible = storedVisible && !m_unitCellGlobalHidden;

    auto& actors = m_cellEdgeActorsByStructure[structureId];
    actors.clear();

    // 각 모서리를 선으로 생성
    for (int i = 0; i < 12; i++) {
        int startIdx = edges[i][0];
        int endIdx = edges[i][1];
        
        vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
        lineSource->SetPoint1(vertices[startIdx]);
        lineSource->SetPoint2(vertices[endIdx]);
        lineSource->Update();
        
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(lineSource->GetOutputPort());
        
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetPickable(false);

        // 선 속성 설정
        actor->GetProperty()->SetColor(1.0, 1.0, 1.0);  // 흰색
        actor->GetProperty()->SetLineWidth(2.0);
        actor->SetVisibility(effectiveVisible ? 1 : 0);
        
        // 렌더러에 추가
        VtkViewer::Instance().AddActor(actor);
        
        actors.push_back(actor);
    }
    
    m_unitCellVisibleByStructure[structureId] = storedVisible;
    
    SPDLOG_DEBUG("Created unit cell with {} edges", actors.size());
}

void VTKRenderer::clearUnitCell() {
    SPDLOG_DEBUG("Clearing unit cell visualization");
    
    for (auto& [structureId, actors] : m_cellEdgeActorsByStructure) {
        (void)structureId;
        for (auto& actor : actors) {
            if (actor) {
                VtkViewer::Instance().RemoveActor(actor);
            }
        }
    }
    
    m_cellEdgeActorsByStructure.clear();
    m_unitCellVisibleByStructure.clear();
    
    SPDLOG_DEBUG("Unit cell cleared");
}

void VTKRenderer::clearUnitCell(int32_t structureId) {
    auto it = m_cellEdgeActorsByStructure.find(structureId);
    if (it == m_cellEdgeActorsByStructure.end()) {
        return;
    }
    
    for (auto& actor : it->second) {
        if (actor) {
            VtkViewer::Instance().RemoveActor(actor);
        }
    }
    
    m_cellEdgeActorsByStructure.erase(it);
    m_unitCellVisibleByStructure.erase(structureId);
}

void VTKRenderer::setUnitCellVisible(bool visible) {
    m_unitCellGlobalHidden = !visible;
    
    for (auto& [structureId, actors] : m_cellEdgeActorsByStructure) {
        bool structureVisible = true;
        auto itVisible = m_unitCellVisibleByStructure.find(structureId);
        if (itVisible != m_unitCellVisibleByStructure.end()) {
            structureVisible = itVisible->second;
        }
        bool effectiveVisible = visible && structureVisible;
        for (auto& actor : actors) {
            if (actor) {
                actor->SetVisibility(effectiveVisible ? 1 : 0);
            }
        }
    }
    
    SPDLOG_DEBUG("Unit cell visibility set to: {}", visible);
}

void VTKRenderer::setUnitCellVisible(int32_t structureId, bool visible) {
    m_unitCellVisibleByStructure[structureId] = visible;
    auto it = m_cellEdgeActorsByStructure.find(structureId);
    if (it == m_cellEdgeActorsByStructure.end()) {
        return;
    }

    bool effectiveVisible = visible && !m_unitCellGlobalHidden;
    for (auto& actor : it->second) {
        if (actor) {
            actor->SetVisibility(effectiveVisible ? 1 : 0);
        }
    }
}

bool VTKRenderer::hasUnitCell(int32_t structureId) const {
    auto it = m_cellEdgeActorsByStructure.find(structureId);
    return it != m_cellEdgeActorsByStructure.end() && !it->second.empty();
}

bool VTKRenderer::isUnitCellVisible(int32_t structureId) const {
    auto it = m_unitCellVisibleByStructure.find(structureId);
    if (it == m_unitCellVisibleByStructure.end()) {
        return false;
    }
    return it->second;
}

// ============================================================================
// 내부 헬퍼 함수
// ============================================================================

vtkSmartPointer<vtkPolyData> VTKRenderer::createSphereGeometry(float radius) {
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(radius);
    sphereSource->SetThetaResolution(m_sphereResolution);
    sphereSource->SetPhiResolution(m_sphereResolution);
    sphereSource->SetCenter(0, 0, 0);
    sphereSource->Update();
    
    return sphereSource->GetOutput();
}

vtkSmartPointer<vtkPolyData> VTKRenderer::createCylinderGeometry(float radius, float height) {
    vtkSmartPointer<vtkCylinderSource> cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
    cylinderSource->SetRadius(radius);
    cylinderSource->SetHeight(height);
    cylinderSource->SetResolution(m_sphereResolution);
    cylinderSource->SetCenter(0, 0, 0);
    cylinderSource->Update();
    
    return cylinderSource->GetOutput();
}

void VTKRenderer::setupActorProperties(vtkSmartPointer<vtkActor> actor, bool isBond) {
    if (!actor) {
        return;
    }

    // ========== Pickable 설정 추가 ==========
    actor->SetPickable(true);
    // ========================================

    vtkProperty* property = actor->GetProperty();
    if (property) {
        property->SetEdgeVisibility(false);
        property->SetAmbient(0.3);
        property->SetDiffuse(0.7);
        property->SetSpecular(0.1);
        property->SetSpecularPower(10);
        
        if (isBond) {
            property->SetOpacity(m_bondOpacity);
            
            if (m_bondOpacity < 1.0f) {
                property->SetRenderLinesAsTubes(true);
                actor->ForceTranslucentOn();
            } else {
                actor->ForceOpaqueOn();
            }
        }
        
        actor->Modified();
    }
}

// ============================================================================
// BZ Plot 렌더링 구현
// ============================================================================

void VTKRenderer::createBZPlot(const domain::BZVerticesResult& bzData) {
    clearBZPlot();
    
    if (!bzData.success || bzData.facets.empty()) {
        SPDLOG_WARN("Invalid BZ data, cannot create BZ plot");
        return;
    }
    
    SPDLOG_INFO("Creating BZ Plot with {} facets", bzData.facets.size());
    
    // 각 facet을 선으로 렌더링
    for (size_t facetIdx = 0; facetIdx < bzData.facets.size(); facetIdx++) {
        const auto& facet = bzData.facets[facetIdx];
        
        if (facet.vertices.empty()) {
            SPDLOG_WARN("Facet {} has no vertices, skipping", facetIdx);
            continue;
        }
        
        size_t numVertices = facet.vertices.size();
        for (size_t i = 0; i < numVertices; i++) {
            size_t nextIdx = (i + 1) % numVertices;
            
            const auto& v1 = facet.vertices[i];
            const auto& v2 = facet.vertices[nextIdx];
            
            vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
            lineSource->SetPoint1(v1[0], v1[1], v1[2]);
            lineSource->SetPoint2(v2[0], v2[1], v2[2]);
            lineSource->Update();
            
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(lineSource->GetOutputPort());
            
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            
            // 선 속성 설정 (검은색 선)
            actor->GetProperty()->SetColor(0.0, 0.0, 0.0);
            actor->GetProperty()->SetLineWidth(3.0);
            
            // 렌더러에 추가
            VtkViewer::Instance().AddActor(actor);
            
            // Actor 저장
            m_bzPlotActors.push_back(actor);
        }
    }
    
    m_bzPlotVisible = true;
    
    SPDLOG_INFO("Created BZ Plot with {} line segments", m_bzPlotActors.size());
}

void VTKRenderer::clearBZPlot() {
    SPDLOG_DEBUG("Clearing BZ Plot visualization");
    
    for (auto& actor : m_bzPlotActors) {
        if (actor) {
            VtkViewer::Instance().RemoveActor(actor);
        }
    }

    for (auto& actor : m_bzPlotActors2D) {
        if (actor) {
            VtkViewer::Instance().RemoveActor2D(actor);
        }
    }
    
    m_bzPlotActors.clear();
    m_bzPlotActors2D.clear();
    m_bzPlotVisible = false;
    
    SPDLOG_DEBUG("BZ Plot cleared");
}

void VTKRenderer::setBZPlotVisible(bool visible) {
    if (m_bzPlotVisible == visible) {
        return;
    }
    
    for (auto& actor : m_bzPlotActors) {
        if (actor) {
            actor->SetVisibility(visible ? 1 : 0);
        }
    }

    for (auto& actor : m_bzPlotActors2D) {
        if (actor) {
            actor->SetVisibility(visible ? 1 : 0);
        }
    }
    
    m_bzPlotVisible = visible;
    
    SPDLOG_DEBUG("BZ Plot visibility set to: {}", visible);
}

bool VTKRenderer::isBZPlotVisible() const {
    return m_bzPlotVisible;
}

// ============================================================================
// ⭐ 완전한 BZ Plot 생성 (모든 요소 통합)
// ============================================================================

void VTKRenderer::createCompleteBZPlot(
    const domain::BZVerticesResult& bzData,
    const double cell[3][3], 
    const double icell[3][3],
    bool showVectors,
    bool showLabels,
    const std::string& pathInput,
    int npoints)
{
    clearBZPlot();
    
    if (!bzData.success || bzData.facets.empty()) {
        SPDLOG_WARN("Invalid BZ data, cannot create complete BZ plot");
        return;
    }
    
    // ⭐ 스케일 팩터 계산
    double maxLength = calculateMaxReciprocalVectorLength(icell);
    if (maxLength < 1e-10) {
        SPDLOG_ERROR("Invalid reciprocal lattice vectors (max length too small)");
        return;
    }
    
    // 기준 길이를 5.0으로 정규화 (더 큰 값으로 수정)
    m_bzScaleFactor = 1.0 * maxLength;
    
    SPDLOG_INFO("Creating complete BZ Plot with {} facets (scale factor: {:.3f})", 
                bzData.facets.size(), m_bzScaleFactor);
    
    try {
        // 1. IBZ 경계선 렌더링
        for (size_t facetIdx = 0; facetIdx < bzData.facets.size(); facetIdx++) {
            const auto& facet = bzData.facets[facetIdx];
            
            if (facet.vertices.empty()) continue;
            
            size_t numVertices = facet.vertices.size();
            for (size_t i = 0; i < numVertices; i++) {
                size_t nextIdx = (i + 1) % numVertices;
                
                const auto& v1 = facet.vertices[i];
                const auto& v2 = facet.vertices[nextIdx];
                
                vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
                lineSource->SetPoint1(v1[0], v1[1], v1[2]);
                lineSource->SetPoint2(v2[0], v2[1], v2[2]);
                lineSource->Update();
                
                vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                mapper->SetInputConnection(lineSource->GetOutputPort());
                
                vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
                actor->SetMapper(mapper);
                actor->GetProperty()->SetColor(0.0, 0.0, 0.0);
                
                // ⚠️ 선 두께 고정값 사용
                double lineWidth = 3.0;
                actor->GetProperty()->SetLineWidth(lineWidth);
                
                VtkViewer::Instance().AddActor(actor);
                m_bzPlotActors.push_back(actor);
            }
        }
        
        // 2. 역격자 벡터 렌더링
        if (showVectors) {
            renderReciprocalVectors(icell);
        }
        
        // 3. Bandpath 및 특수점 라벨 렌더링
        if (!pathInput.empty() && npoints > 0) {
            SPDLOG_INFO("========================================");
            SPDLOG_INFO("Bandpath Rendering - Step 3");
            SPDLOG_INFO("========================================");
            SPDLOG_INFO("Input path string: '{}'", pathInput);
            SPDLOG_INFO("Requested npoints: {}", npoints);
            SPDLOG_INFO("");
            
            try {
                // 3-1. Path 파싱
                SPDLOG_INFO("Step 3-1: Parsing path string...");
                auto pathSegments = atoms::domain::PathParser::parse(pathInput);
                
                if (!pathSegments.empty()) {
                    SPDLOG_INFO("✓ Successfully parsed {} path segment(s)", pathSegments.size());
                    
                    // 파싱된 세그먼트 상세 출력
                    for (size_t i = 0; i < pathSegments.size(); i++) {
                        std::string segmentStr = "";
                        for (size_t j = 0; j < pathSegments[i].size(); j++) {
                            segmentStr += pathSegments[i][j];
                            if (j < pathSegments[i].size() - 1) {
                                segmentStr += " → ";
                            }
                        }
                        SPDLOG_INFO("  Segment {}: {}", i + 1, segmentStr);
                    }
                    SPDLOG_INFO("");
                    
                    // 3-2. 격자 타입 결정 및 특수점 로드
                    SPDLOG_INFO("Step 3-2: Detecting lattice type and loading special points...");

                    // ⭐ 격자 타입 자동 감지
                    std::string latticeType = atoms::domain::SpecialPointsDatabase::detectLatticeType(cell);
                    SPDLOG_INFO("  Detected lattice type: {}", latticeType);

                    // 감지된 격자 타입에 해당하는 특수점 로드
                    auto specialPointsFrac = atoms::domain::SpecialPointsDatabase::getSpecialPoints(latticeType);
                    SPDLOG_INFO("  Loaded {} special points (fractional coordinates)", specialPointsFrac.size());

                    // 분수 좌표 출력 (디버그 레벨)
                    SPDLOG_DEBUG("  Special points in fractional coordinates:");
                    for (const auto& [label, fracCoord] : specialPointsFrac) {
                        SPDLOG_DEBUG("    {}: ({:.6f}, {:.6f}, {:.6f})", 
                                    label, fracCoord[0], fracCoord[1], fracCoord[2]);
                    }
                    SPDLOG_INFO("");
                    
                    // 3-3. 분수 좌표 → 카르테시안 좌표 변환
                    SPDLOG_INFO("Step 3-3: Converting special points to Cartesian coordinates...");
                    std::map<std::string, std::array<double, 3>> specialPointsCart;
                    
                    for (const auto& [label, fracCoord] : specialPointsFrac) {
                        auto cartCoord = atoms::domain::SpecialPointsDatabase::fractionalToCartesian(
                            fracCoord, icell);
                        specialPointsCart[label] = cartCoord;
                        
                        SPDLOG_DEBUG("  {} (frac): ({:.6f}, {:.6f}, {:.6f}) → (cart): ({:.6f}, {:.6f}, {:.6f})",
                                    label,
                                    fracCoord[0], fracCoord[1], fracCoord[2],
                                    cartCoord[0], cartCoord[1], cartCoord[2]);
                    }
                    SPDLOG_INFO("✓ Converted {} special points to Cartesian coordinates", specialPointsCart.size());
                    SPDLOG_INFO("");
                    
                    // 3-4. K-points 생성 (경로 보간)
                    SPDLOG_INFO("Step 3-4: Generating k-points along bandpath...");
                    SPDLOG_INFO("  Target total points: {}", npoints);
                    
                    auto kpoints = atoms::domain::KpointsInterpolator::generateKpoints(
                        pathSegments, specialPointsCart, npoints);
                    
                    if (!kpoints.empty()) {
                        SPDLOG_INFO("✓ Successfully generated {} k-points", kpoints.size());
                        
                        // K-points 통계
                        if (kpoints.size() > 0) {
                            SPDLOG_INFO("  First k-point: ({:.6f}, {:.6f}, {:.6f})",
                                    kpoints[0][0], kpoints[0][1], kpoints[0][2]);
                            SPDLOG_INFO("  Last k-point:  ({:.6f}, {:.6f}, {:.6f})",
                                    kpoints[kpoints.size()-1][0], 
                                    kpoints[kpoints.size()-1][1], 
                                    kpoints[kpoints.size()-1][2]);
                        }
                        SPDLOG_INFO("");
                        
                        // 3-5. Bandpath 렌더링 (빨간색 선)
                        SPDLOG_INFO("Step 3-5: Rendering bandpath (red lines)...");
                        renderBandpath(kpoints);
                        SPDLOG_INFO("✓ Bandpath rendered successfully");
                        SPDLOG_INFO("");
                        
                        // 3-6. K-points 렌더링 (빨간색 점)
                        SPDLOG_INFO("Step 3-6: Rendering k-points (red spheres)...");
                        double kpointRadius = 0.02 * m_bzScaleFactor;
                        SPDLOG_INFO("  K-point radius: {:.6f} (scale factor: {:.3f})", 
                                kpointRadius, m_bzScaleFactor);
                        
                        renderKpoints(kpoints, kpointRadius);
                        SPDLOG_INFO("✓ Rendered {} k-point spheres", kpoints.size());
                        SPDLOG_INFO("");
                        
                        // 3-7. 특수점 라벨 렌더링
                        if (showLabels) {
                            SPDLOG_INFO("Step 3-7: Rendering special point labels...");
                            
                            // 경로에 포함된 특수점만 필터링
                            std::map<std::string, std::array<double, 3>> labelsToShow;
                            for (const auto& segment : pathSegments) {
                                for (const auto& label : segment) {
                                    auto it = specialPointsCart.find(label);
                                    if (it != specialPointsCart.end()) {
                                        labelsToShow[label] = it->second;
                                    }
                                }
                            }
                            
                            if (!labelsToShow.empty()) {
                                SPDLOG_INFO("  Labels to render: {}", labelsToShow.size());
                                
                                // 각 라벨 정보 출력
                                for (const auto& [label, cartCoord] : labelsToShow) {
                                    SPDLOG_DEBUG("    {}: ({:.6f}, {:.6f}, {:.6f})",
                                                label, cartCoord[0], cartCoord[1], cartCoord[2]);
                                }
                                
                                renderSpecialPointLabels(labelsToShow);
                                SPDLOG_INFO("✓ Rendered {} special point labels", labelsToShow.size());
                            } else {
                                SPDLOG_WARN("  No labels to render (empty labelsToShow map)");
                            }
                        } else {
                            SPDLOG_INFO("Step 3-7: Skipping label rendering (showLabels = false)");
                        }
                        SPDLOG_INFO("");
                        
                    } else {
                        SPDLOG_ERROR("✗ Failed to generate k-points (empty result)");
                    }
                    
                } else {
                    SPDLOG_ERROR("✗ Path parsing failed (empty pathSegments)");
                }
                
                SPDLOG_INFO("========================================");
                SPDLOG_INFO("Bandpath Rendering Completed");
                SPDLOG_INFO("========================================");
                SPDLOG_INFO("");
                
            } catch (const std::exception& e) {
                SPDLOG_ERROR("========================================");
                SPDLOG_ERROR("Error in Bandpath Rendering");
                SPDLOG_ERROR("========================================");
                SPDLOG_ERROR("Path string: '{}'", pathInput);
                SPDLOG_ERROR("Exception: {}", e.what());
                SPDLOG_ERROR("========================================");
                SPDLOG_ERROR("");
            }
        }
                
        m_bzPlotVisible = true;
        
        SPDLOG_INFO("Complete BZ Plot created with {} actors", m_bzPlotActors.size());
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error creating complete BZ plot: {}", e.what());
    }
}

// ============================================================================
// ⭐ 역격자 벡터 렌더링 (b1, b2, b3)
// ============================================================================

void VTKRenderer::renderReciprocalVectors(const double icell[3][3]) {
    SPDLOG_DEBUG("Rendering reciprocal vectors with scale factor: {:.3f}", m_bzScaleFactor);
    
    double origin[3] = {0.0, 0.0, 0.0};
    double black[3] = {0.0, 0.0, 0.0};
    
    const char* labels[3] = {"b1", "b2", "b3"};
    
    for (int i = 0; i < 3; i++) {
        double end[3] = {icell[i][0], icell[i][1], icell[i][2]};
        
        // ⭐ 화살표 머리 크기는 고정 (world 단위), 길이만 벡터 길이를 따른다.
        double tipLength = 0.5 * m_bzScaleFactor;
        double shaftRadius = 0.01 * m_bzScaleFactor;
        
        vtkSmartPointer<vtkActor> arrowActor = createArrowActor(
            origin, end, black, tipLength, shaftRadius);
        
        VtkViewer::Instance().AddActor(arrowActor);
        m_bzPlotActors.push_back(arrowActor);
        
        // 라벨 위치 (화살표 끝에서 약간 더 멀리)
        double labelPos[3] = {
            end[0] * 1.01,
            end[1] * 1.01,
            end[2] * 1.01
        };
        
        int fontSize = 64;
        
        vtkSmartPointer<vtkActor2D> labelActor = createTextActor2D(
            labels[i], labelPos, fontSize, black);
        
        VtkViewer::Instance().AddActor2D(labelActor, false);
        m_bzPlotActors2D.push_back(labelActor);
        
        SPDLOG_DEBUG("Rendered reciprocal vector {}: ({:.3f}, {:.3f}, {:.3f})", 
                    labels[i], end[0], end[1], end[2]);
    }
}

// ============================================================================
// ⭐ Bandpath 렌더링 (빨간색 선)
// ============================================================================

void VTKRenderer::renderBandpath(const std::vector<std::array<double, 3>>& kpoints) {
    if (kpoints.size() < 2) {
        SPDLOG_WARN("Need at least 2 k-points to render bandpath");
        return;
    }
    
    SPDLOG_DEBUG("Rendering bandpath with {} k-points", kpoints.size());
    
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (const auto& kp : kpoints) {
        points->InsertNextPoint(kp[0], kp[1], kp[2]);
        
        // ⚠️ 디버깅: 첫 번째와 마지막 k-point 출력
        if (points->GetNumberOfPoints() == 1 || 
            points->GetNumberOfPoints() == kpoints.size()) {
            SPDLOG_DEBUG("K-point {}: ({:.6f}, {:.6f}, {:.6f})", 
                        points->GetNumberOfPoints() - 1,
                        kp[0], kp[1], kp[2]);
        }
    }
    
    vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(kpoints.size());
    for (size_t i = 0; i < kpoints.size(); i++) {
        polyLine->GetPointIds()->SetId(i, i);
    }
    
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);
    
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(cells);
    
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);
    
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0);  // 빨간색
    
    // ⚠️ 선 두께
    double lineWidth = 5.0;
    actor->GetProperty()->SetLineWidth(lineWidth);
    
    VtkViewer::Instance().AddActor(actor);
    m_bzPlotActors.push_back(actor);
    
    SPDLOG_DEBUG("Bandpath rendered successfully with line width {:.1f}", lineWidth);
}

// ============================================================================
// ⭐ K-points 렌더링 (빨간색 점)
// ============================================================================

void VTKRenderer::renderKpoints(
    const std::vector<std::array<double, 3>>& kpoints,
    double radius)
{
    if (kpoints.empty()) {
        SPDLOG_WARN("No k-points to render");
        return;
    }
    
    SPDLOG_DEBUG("Rendering {} k-points with radius {:.6f}", kpoints.size(), radius);
    
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (const auto& kp : kpoints) {
        points->InsertNextPoint(kp[0], kp[1], kp[2]);
    }
    
    vtkSmartPointer<vtkPolyData> pointsPolyData = vtkSmartPointer<vtkPolyData>::New();
    pointsPolyData->SetPoints(points);
    
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(radius);
    sphereSource->SetThetaResolution(16);
    sphereSource->SetPhiResolution(16);
    
    SPDLOG_DEBUG("Sphere source radius set to: {:.6f}", radius);
    
    vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
    glyph3D->SetInputData(pointsPolyData);
    glyph3D->SetSourceConnection(sphereSource->GetOutputPort());
    glyph3D->Update();
    
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(glyph3D->GetOutputPort());
    
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0);  // 빨간색
    
    VtkViewer::Instance().AddActor(actor);
    m_bzPlotActors.push_back(actor);
    
    SPDLOG_DEBUG("K-points rendered successfully (actor added to m_bzPlotActors)");
}

// ============================================================================
// ⭐ 특수점 라벨 렌더링 (옵션 3: 일반 vtkActor 사용)
// ============================================================================

void VTKRenderer::renderSpecialPointLabels(
    const std::map<std::string, std::array<double, 3>>& specialPoints)
{
    SPDLOG_DEBUG("Rendering {} special point labels", specialPoints.size());
    
    double black[3] = {0.0, 0.0, 0.0};
    
    for (const auto& [name, position] : specialPoints) {
        // ⚠️ 라벨 오프셋 크게 수정 (고정값)
        double labelPos[3] = {
            position[0] * 1.01,
            position[1] * 1.01,
            position[2] * 1.01
        };
        
        std::string displayName = name;
        if (name == "G" || name == "Gamma") {
            displayName = "G";  // UTF-8 Gamma
        }
        
        int fontSize = 64;
        
        vtkSmartPointer<vtkActor2D> labelActor = createTextActor2D(
            displayName, labelPos, fontSize, black);
        
        VtkViewer::Instance().AddActor2D(labelActor, false);
        m_bzPlotActors2D.push_back(labelActor);
        
        SPDLOG_DEBUG("Rendered label '{}' at ({:.3f}, {:.3f}, {:.3f}) with fontSize {}", 
                    displayName, position[0], position[1], position[2], fontSize);
    }
}

// ============================================================================
// ⭐ 내부 헬퍼 함수: 화살표 Actor 생성
// ============================================================================

vtkSmartPointer<vtkActor> VTKRenderer::createArrowActor(
    const double start[3],
    const double end[3],
    const double color[3],
    double tipLength,
    double shaftRadius)
{
    // 방향 벡터 계산
    double direction[3] = {
        end[0] - start[0],
        end[1] - start[1],
        end[2] - start[2]
    };
    
    double length = std::sqrt(
        direction[0] * direction[0] +
        direction[1] * direction[1] +
        direction[2] * direction[2]
    );
    
    double tipLengthNormalized = 0.1;

    // ArrowSource 생성
    vtkSmartPointer<vtkArrowSource> arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    arrowSource->SetTipLength(tipLengthNormalized);
    arrowSource->SetTipRadius(shaftRadius * 3.0);
    arrowSource->SetShaftRadius(shaftRadius);
    arrowSource->SetShaftResolution(16);
    arrowSource->SetTipResolution(16);
    
    // Transform 생성 (회전 및 스케일)
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(start[0], start[1], start[2]);
    
    // 화살표 방향 설정 (기본 X축 -> direction 벡터)
    if (length > 1e-6) {
        double xAxis[3] = {1.0, 0.0, 0.0};
        double normalizedDir[3] = {
            direction[0] / length,
            direction[1] / length,
            direction[2] / length
        };
        
        // 회전축 계산 (외적)
        double rotationAxis[3] = {
            xAxis[1] * normalizedDir[2] - xAxis[2] * normalizedDir[1],
            xAxis[2] * normalizedDir[0] - xAxis[0] * normalizedDir[2],
            xAxis[0] * normalizedDir[1] - xAxis[1] * normalizedDir[0]
        };
        
        double rotationAxisLength = std::sqrt(
            rotationAxis[0] * rotationAxis[0] +
            rotationAxis[1] * rotationAxis[1] +
            rotationAxis[2] * rotationAxis[2]
        );

        // 회전각 계산 (내적)
        double dotProduct = 
            xAxis[0] * normalizedDir[0] +
            xAxis[1] * normalizedDir[1] +
            xAxis[2] * normalizedDir[2];
        
        if (rotationAxisLength > 1e-6) {           
            double angle = std::acos(std::max(-1.0, std::min(1.0, dotProduct)));
            angle = angle * 180.0 / M_PI;  // 라디안 -> 도
            
            // 회전 적용
            transform->RotateWXYZ(
                angle,
                rotationAxis[0] / rotationAxisLength,
                rotationAxis[1] / rotationAxisLength,
                rotationAxis[2] / rotationAxisLength
            );

        } else if (dotProduct < 0) {
            // 180도 회전 (반대 방향)
            transform->RotateY(180.0);
        }
        
        // 길이 스케일
        transform->Scale(length, 1.0, 1.0);
    }
    
    // TransformPolyDataFilter 적용
    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = 
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(arrowSource->GetOutputPort());
    transformFilter->SetTransform(transform);
    transformFilter->Update();
    
    // Mapper 및 Actor
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());
    
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    
    // 색상 설정
    if (color != nullptr) {
        actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    } else {
        actor->GetProperty()->SetColor(0.0, 0.0, 0.0);
    }
    
    return actor;
}

// ============================================================================
// ⭐ 내부 헬퍼 함수: 텍스트 Actor 생성 (옵션 3: 고정 텍스트)
// ============================================================================

vtkSmartPointer<vtkActor> VTKRenderer::createTextActor(
    const std::string& text,
    const double position[3],
    int fontSize,
    const double color[3])
{
    // VectorText 생성
    vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
    textSource->SetText(text.c_str());
    
    // Mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(textSource->GetOutputPort());
    
    // ⚠️ 옵션 3: vtkFollower 대신 일반 vtkActor 사용
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPosition(position[0], position[1], position[2]);
    
    // 크기 조정
    double scale = fontSize / 50.0;
    actor->SetScale(scale, scale, scale);
    
    // 색상 설정
    if (color != nullptr) {
        actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    } else {
        actor->GetProperty()->SetColor(0.0, 0.0, 0.0);
    }
    
    SPDLOG_DEBUG("Created fixed text actor '{}' at ({:.3f}, {:.3f}, {:.3f}) with scale {:.3f}", 
                text, position[0], position[1], position[2], scale);
    
    return actor;
}

vtkSmartPointer<vtkActor2D> VTKRenderer::createTextActor2D(
    const std::string& text,
    const double position[3],
    int fontSize,
    const double color[3])
{
    vtkSmartPointer<vtkTextActor> actor = vtkSmartPointer<vtkTextActor>::New();
    actor->SetInput(text.c_str());

    vtkTextProperty* property = actor->GetTextProperty();
    if (property) {
        property->SetFontSize(fontSize*1.5);
        if (color != nullptr) {
            property->SetColor(color[0], color[1], color[2]);
        } else {
            property->SetColor(0.0, 0.0, 0.0);
        }
        property->SetBackgroundColor(1.0, 1.0, 1.0);
        property->SetBackgroundOpacity(0.7);
        property->SetFrame(true);
        property->SetFrameColor(0.0, 0.0, 0.0);
        property->SetJustificationToCentered();
        property->SetVerticalJustificationToCentered();
    }

    vtkCoordinate* coord = actor->GetPositionCoordinate();
    coord->SetCoordinateSystemToWorld();
    coord->SetValue(position[0], position[1], position[2]);

    SPDLOG_DEBUG("Created 2D text actor '{}' at ({:.3f}, {:.3f}, {:.3f})", 
                text, position[0], position[1], position[2]);

    return actor;
}

double VTKRenderer::calculateMaxReciprocalVectorLength(const double icell[3][3]) {
    double maxLength = 0.0;
    
    for (int i = 0; i < 3; i++) {
        double len = std::sqrt(
            icell[i][0] * icell[i][0] +
            icell[i][1] * icell[i][1] +
            icell[i][2] * icell[i][2]
        );
        maxLength = std::max(maxLength, len);
    }
    
    SPDLOG_DEBUG("Max reciprocal vector length: {:.3f}", maxLength);
    return maxLength;
}

// ============================================================================
// 원자/결합 그룹 가시성 제어
// ============================================================================

void VTKRenderer::setAtomGroupVisible(const std::string& symbol, bool visible) {
    auto it = m_atomGroups.find(symbol);
    if (it == m_atomGroups.end()) {
        SPDLOG_DEBUG("Atom group {} not found", symbol);
        return;
    }
    
    AtomGroupVTK& group = it->second;
    if (group.actor) {
        group.actor->SetVisibility(visible ? 1 : 0);
        SPDLOG_DEBUG("Set atom group {} visibility to {}", symbol, visible);
    }
}

void VTKRenderer::setAllAtomGroupsVisible(bool visible) {
    SPDLOG_INFO("Setting all atom groups visibility to: {}", visible);
    
    for (auto& [symbol, group] : m_atomGroups) {
        if (group.actor) {
            group.actor->SetVisibility(visible ? 1 : 0);
        }
    }
    
    SPDLOG_DEBUG("Updated visibility for {} atom groups", m_atomGroups.size());
}

void VTKRenderer::setAllBondGroupsVisible(bool visible) {
    SPDLOG_INFO("Setting all bond groups visibility to: {}", visible);
    
    for (auto& [bondTypeKey, group] : m_bondGroups) {
        if (group.actor1) {
            group.actor1->SetVisibility(visible ? 1 : 0);
        }
        if (group.actor2) {
            group.actor2->SetVisibility(visible ? 1 : 0);
        }
    }
    
    SPDLOG_DEBUG("Updated visibility for {} bond groups", m_bondGroups.size());
}

void VTKRenderer::syncAtomLabelActors(const std::vector<LabelActorSpec>& labels) {
    std::unordered_map<uint32_t, const LabelActorSpec*> labelLookup;
    labelLookup.reserve(labels.size());
    for (const auto& label : labels) {
        if (label.id == 0) {
            continue;
        }
        labelLookup[label.id] = &label;
    }

    for (auto it = m_atomLabelActors2D.begin(); it != m_atomLabelActors2D.end();) {
        if (labelLookup.find(it->first) == labelLookup.end()) {
            if (it->second) {
                VtkViewer::Instance().RemoveActor2D(it->second);
            }
            it = m_atomLabelActors2D.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& [labelId, label] : labelLookup) {
        auto actorIt = m_atomLabelActors2D.find(labelId);
        if (actorIt == m_atomLabelActors2D.end()) {
            double position[3] = { label->worldPosition[0], label->worldPosition[1], label->worldPosition[2] };
            vtkSmartPointer<vtkActor2D> actor = createTextActor2D(label->text, position, 14, nullptr);
            VtkViewer::Instance().AddActor2D(actor, false);
            m_atomLabelActors2D.emplace(labelId, actor);
            updateLabelActor2D(actor, *label);
        } else {
            updateLabelActor2D(actorIt->second, *label);
        }
    }
}

void VTKRenderer::syncBondLabelActors(const std::vector<LabelActorSpec>& labels) {
    std::unordered_map<uint32_t, const LabelActorSpec*> labelLookup;
    labelLookup.reserve(labels.size());
    for (const auto& label : labels) {
        if (label.id == 0) {
            continue;
        }
        labelLookup[label.id] = &label;
    }

    for (auto it = m_bondLabelActors2D.begin(); it != m_bondLabelActors2D.end();) {
        if (labelLookup.find(it->first) == labelLookup.end()) {
            if (it->second) {
                VtkViewer::Instance().RemoveActor2D(it->second);
            }
            it = m_bondLabelActors2D.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& [labelId, label] : labelLookup) {
        auto actorIt = m_bondLabelActors2D.find(labelId);
        if (actorIt == m_bondLabelActors2D.end()) {
            double position[3] = { label->worldPosition[0], label->worldPosition[1], label->worldPosition[2] };
            vtkSmartPointer<vtkActor2D> actor = createTextActor2D(label->text, position, 14, nullptr);
            VtkViewer::Instance().AddActor2D(actor, false);
            m_bondLabelActors2D.emplace(labelId, actor);
            updateLabelActor2D(actor, *label);
        } else {
            updateLabelActor2D(actorIt->second, *label);
        }
    }
}

void VTKRenderer::clearAllAtomLabelActors() {
    for (auto& [labelId, actor] : m_atomLabelActors2D) {
        (void)labelId;
        if (actor) {
            VtkViewer::Instance().RemoveActor2D(actor);
        }
    }
    m_atomLabelActors2D.clear();
}

void VTKRenderer::clearAllBondLabelActors() {
    for (auto& [labelId, actor] : m_bondLabelActors2D) {
        (void)labelId;
        if (actor) {
            VtkViewer::Instance().RemoveActor2D(actor);
        }
    }
    m_bondLabelActors2D.clear();
}

void VTKRenderer::clearAllStructureLabelActors() {
    clearAllAtomLabelActors();
    clearAllBondLabelActors();
}

} // namespace infrastructure
} // namespace atoms
