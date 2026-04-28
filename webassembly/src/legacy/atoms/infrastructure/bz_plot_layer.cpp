// atoms/infrastructure/bz_plot_layer.cpp
#include "bz_plot_layer.h"
#include "../../vtk_viewer.h"

namespace atoms {
namespace infrastructure {

// ============================================================================
// ActorGroup Implementation
// ============================================================================

void BZPlotLayer::ActorGroup::setVisibility(bool vis) {
    visible = vis;
    for (auto& actor : actors) {
        if (actor) {
            actor->SetVisibility(vis);
        }
    }
}

void BZPlotLayer::ActorGroup::clear() {
    // VtkViewer에서 Actor 제거
    for (auto& actor : actors) {
        if (actor) {
            VtkViewer::Instance().RemoveActor(actor);
        }
    }
    
    // 벡터 정리
    actors.clear();
    visible = true; // 기본값으로 리셋
}

// ============================================================================
// BZPlotLayer Implementation
// ============================================================================

BZPlotLayer::~BZPlotLayer() {
    clear();
}

// 전체 레이어 표시/숨김
void BZPlotLayer::show() {
    m_isVisible = true;
    
    // 각 카테고리의 가시성 상태에 따라 표시
    m_ibzLines.setVisibility(m_ibzLines.visible);
    m_reciprocalVectors.setVisibility(m_reciprocalVectors.visible);
    m_bandpaths.setVisibility(m_bandpaths.visible);
    m_kpoints.setVisibility(m_kpoints.visible);
    m_labels.setVisibility(m_labels.visible);
}

void BZPlotLayer::hide() {
    m_isVisible = false;
    
    // 모든 카테고리 숨김 (가시성 상태는 유지)
    for (auto& actor : m_ibzLines.actors) {
        if (actor) actor->SetVisibility(false);
    }
    for (auto& actor : m_reciprocalVectors.actors) {
        if (actor) actor->SetVisibility(false);
    }
    for (auto& actor : m_bandpaths.actors) {
        if (actor) actor->SetVisibility(false);
    }
    for (auto& actor : m_kpoints.actors) {
        if (actor) actor->SetVisibility(false);
    }
    for (auto& actor : m_labels.actors) {
        if (actor) actor->SetVisibility(false);
    }
}

// 카테고리별 가시성 설정
void BZPlotLayer::setIBZLinesVisible(bool visible) {
    m_ibzLines.visible = visible;
    if (m_isVisible) {
        m_ibzLines.setVisibility(visible);
    }
}

void BZPlotLayer::setReciprocalVectorsVisible(bool visible) {
    m_reciprocalVectors.visible = visible;
    if (m_isVisible) {
        m_reciprocalVectors.setVisibility(visible);
    }
}

void BZPlotLayer::setBandpathVisible(bool visible) {
    m_bandpaths.visible = visible;
    if (m_isVisible) {
        m_bandpaths.setVisibility(visible);
    }
}

void BZPlotLayer::setKpointsVisible(bool visible) {
    m_kpoints.visible = visible;
    if (m_isVisible) {
        m_kpoints.setVisibility(visible);
    }
}

void BZPlotLayer::setLabelsVisible(bool visible) {
    m_labels.visible = visible;
    if (m_isVisible) {
        m_labels.setVisibility(visible);
    }
}

// Actor 추가 메서드
void BZPlotLayer::addIBZLineActor(vtkSmartPointer<vtkActor> actor) {
    if (!actor) return;
    
    // VtkViewer에 추가
    VtkViewer::Instance().AddActor(actor);
    
    // 그룹에 저장
    m_ibzLines.actors.push_back(actor);
    
    // 현재 가시성 상태 적용
    actor->SetVisibility(m_isVisible && m_ibzLines.visible);
}

void BZPlotLayer::addReciprocalVectorActor(vtkSmartPointer<vtkActor> actor) {
    if (!actor) return;
    
    VtkViewer::Instance().AddActor(actor);
    
    m_reciprocalVectors.actors.push_back(actor);
    actor->SetVisibility(m_isVisible && m_reciprocalVectors.visible);
}

void BZPlotLayer::addBandpathActor(vtkSmartPointer<vtkActor> actor) {
    if (!actor) return;
    
    VtkViewer::Instance().AddActor(actor);
    
    m_bandpaths.actors.push_back(actor);
    actor->SetVisibility(m_isVisible && m_bandpaths.visible);
}

void BZPlotLayer::addKpointActor(vtkSmartPointer<vtkActor> actor) {
    if (!actor) return;
    
    VtkViewer::Instance().AddActor(actor);
    
    m_kpoints.actors.push_back(actor);
    actor->SetVisibility(m_isVisible && m_kpoints.visible);
}

void BZPlotLayer::addLabelActor(vtkSmartPointer<vtkActor> actor) {
    if (!actor) return;
    
    VtkViewer::Instance().AddActor(actor);
    
    m_labels.actors.push_back(actor);
    actor->SetVisibility(m_isVisible && m_labels.visible);
}

// 전체 정리
void BZPlotLayer::clear() {
    m_ibzLines.clear();
    m_reciprocalVectors.clear();
    m_bandpaths.clear();
    m_kpoints.clear();
    m_labels.clear();
    
    m_isVisible = false;
}

// 통계
size_t BZPlotLayer::getTotalActorCount() const {
    return m_ibzLines.size() + 
           m_reciprocalVectors.size() + 
           m_bandpaths.size() + 
           m_kpoints.size() + 
           m_labels.size();
}

} // namespace infrastructure
} // namespace atoms