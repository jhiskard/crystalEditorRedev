// atoms/infrastructure/bz_plot_layer.h
#pragma once

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vector>

namespace atoms {
namespace infrastructure {

/**
 * @brief BZ Plot 전용 레이어 시스템
 * 
 * Brillouin Zone 시각화를 위한 독립적인 레이어를 관리합니다.
 * 각 시각화 요소(IBZ lines, reciprocal vectors 등)를 카테고리별로 관리하며,
 * 개별적으로 표시/숨김을 제어할 수 있습니다.
 */
class BZPlotLayer {
public:
    /**
     * @brief Actor 카테고리별 그룹
     * 
     * 동일한 시각화 목적을 가진 Actor들을 묶어서 관리합니다.
     */
    struct ActorGroup {
        std::vector<vtkSmartPointer<vtkActor>> actors;
        bool visible = true;
        
        /**
         * @brief 그룹 내 모든 Actor의 가시성 설정
         */
        void setVisibility(bool vis);
        
        /**
         * @brief 그룹 내 모든 Actor 제거
         */
        void clear();
        
        /**
         * @brief Actor 개수 반환
         */
        size_t size() const { return actors.size(); }
    };
    
private:
    ActorGroup m_ibzLines;           // IBZ 경계선 (검은색 점선)
    ActorGroup m_reciprocalVectors;  // 역격자 벡터 (검은색 화살표)
    ActorGroup m_bandpaths;          // 밴드 경로 (빨간색 선)
    ActorGroup m_kpoints;            // K-points (빨간색 점)
    ActorGroup m_labels;             // 텍스트 레이블
    
    bool m_isVisible = false;
    
public:
    BZPlotLayer() = default;
    ~BZPlotLayer();
    
    // 전체 레이어 표시/숨김
    /**
     * @brief 전체 BZ Plot 레이어 표시
     */
    void show();
    
    /**
     * @brief 전체 BZ Plot 레이어 숨김
     */
    void hide();
    
    /**
     * @brief 레이어 가시성 상태 반환
     */
    bool isVisible() const { return m_isVisible; }
    
    // 카테고리별 토글
    /**
     * @brief IBZ 경계선 가시성 설정
     */
    void setIBZLinesVisible(bool visible);
    
    /**
     * @brief 역격자 벡터 가시성 설정
     */
    void setReciprocalVectorsVisible(bool visible);
    
    /**
     * @brief 밴드 경로 가시성 설정
     */
    void setBandpathVisible(bool visible);
    
    /**
     * @brief K-points 가시성 설정
     */
    void setKpointsVisible(bool visible);
    
    /**
     * @brief 텍스트 레이블 가시성 설정
     */
    void setLabelsVisible(bool visible);
    
    // 카테고리별 가시성 조회
    bool isIBZLinesVisible() const { return m_ibzLines.visible; }
    bool isReciprocalVectorsVisible() const { return m_reciprocalVectors.visible; }
    bool isBandpathVisible() const { return m_bandpaths.visible; }
    bool isKpointsVisible() const { return m_kpoints.visible; }
    bool isLabelsVisible() const { return m_labels.visible; }
    
    // Actor 추가 메서드
    /**
     * @brief IBZ 경계선 Actor 추가
     */
    void addIBZLineActor(vtkSmartPointer<vtkActor> actor);
    
    /**
     * @brief 역격자 벡터 Actor 추가
     */
    void addReciprocalVectorActor(vtkSmartPointer<vtkActor> actor);
    
    /**
     * @brief 밴드 경로 Actor 추가
     */
    void addBandpathActor(vtkSmartPointer<vtkActor> actor);
    
    /**
     * @brief K-point Actor 추가
     */
    void addKpointActor(vtkSmartPointer<vtkActor> actor);
    
    /**
     * @brief 텍스트 레이블 Actor 추가
     */
    void addLabelActor(vtkSmartPointer<vtkActor> actor);
    
    // 전체 정리
    /**
     * @brief 모든 Actor 제거 및 초기화
     */
    void clear();
    
    // 통계
    /**
     * @brief 전체 Actor 개수 반환
     */
    size_t getTotalActorCount() const;
    
    // 카테고리별 Actor 개수
    size_t getIBZLineCount() const { return m_ibzLines.size(); }
    size_t getReciprocalVectorCount() const { return m_reciprocalVectors.size(); }
    size_t getBandpathCount() const { return m_bandpaths.size(); }
    size_t getKpointCount() const { return m_kpoints.size(); }
    size_t getLabelCount() const { return m_labels.size(); }
};

} // namespace infrastructure
} // namespace atoms