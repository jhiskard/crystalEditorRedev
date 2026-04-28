// atoms/infrastructure/vtk_renderer.h
#pragma once

#include "../../vtk_viewer.h"
#include "bz_plot_layer.h"      // 추가
#include "../domain/bz_plot.h"  // 추가
#include "../domain/color.h" 

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransform.h>
#include <vtkLineSource.h>

#include <string>
#include <vector>
#include <map>
#include <memory>  // 추가 (unique_ptr용)
#include <unordered_map>
#include <array>
#include <cstdint>
// #include <imgui.h>

// Forward declarations
class vtkPolyData;
class vtkActor;
class vtkAppendPolyData;
class vtkPolyDataMapper;
class vtkTransform;
class vtkLineSource;

namespace atoms {

namespace domain {
struct BZVerticesResult;  // ⭐ Forward declaration
}

namespace infrastructure {

// Forward declarations for data types
enum class AtomType;
enum class BondType;

/**
 * @brief VTK 렌더링 시스템 - 모든 VTK 관련 렌더링 로직을 관리
 * 
 * AtomsTemplate에서 VTK 렌더링 관련 코드를 분리하여 관리합니다.
 * 원자 그룹, 결합 그룹, 유닛 셀 렌더링을 담당합니다.
 */
class VTKRenderer {
public:

    // ========================================================================
    // 원자 그룹 렌더링
    // ========================================================================
    /**
     * @brief 원자 그룹 VTK 파이프라인 구조체 // pick 때문에 public으로 옮기기
     */
    struct AtomGroupVTK {
        vtkSmartPointer<vtkPolyData> baseGeometry;           // 기본 구 기하학적 데이터
        vtkSmartPointer<vtkAppendPolyData> appender;         // 인스턴스 결합기
        vtkSmartPointer<vtkPolyDataMapper> mapper;           // VTK 매퍼
        vtkSmartPointer<vtkActor> actor;                     // VTK 액터
        float baseRadius;                                     // 기본 반지름
        
        AtomGroupVTK() : baseRadius(0.5f) {}
        
        bool isInitialized() const {
            return baseGeometry && appender && mapper && actor;
        }
    };
    VTKRenderer();
    ~VTKRenderer();
            
    /**
     * @brief 통합 원자 그룹 VTK 파이프라인 초기화
     */
    void initializeAtomGroupVTK(const std::string& symbol, float radius);
    
    /**
     * @brief 통합 원자 그룹 VTK 업데이트
     */
    void updateAtomGroupVTK(
        const std::string& symbol,
        const std::vector<vtkSmartPointer<vtkTransform>>& transforms,
        // const std::vector<ImVec4>& colors
        const std::vector<atoms::domain::Color4f>& colors
    );
    
    /**
     * @brief 특정 원자 그룹 VTK 파이프라인 정리
     */
    void clearAtomGroupVTK(const std::string& symbol);
    
    /**
     * @brief 모든 원자 그룹 VTK 파이프라인 정리
     */
    void clearAllAtomGroupsVTK();
    
    /**
     * @brief 원자 그룹 VTK 파이프라인 초기화 상태 확인
     */
    bool isAtomGroupInitialized(const std::string& symbol) const;
    
    // ⭐ 원자 그룹 가시성 제어 (추가)
    void setAtomGroupVisible(const std::string& symbol, bool visible);
    void setAllAtomGroupsVisible(bool visible);
    void setAllBondGroupsVisible(bool visible);

    struct LabelActorSpec {
        uint32_t id = 0;
        std::string text;
        std::array<double, 3> worldPosition { 0.0, 0.0, 0.0 };
        bool visible = true;
    };

    void syncAtomLabelActors(const std::vector<LabelActorSpec>& labels);
    void syncBondLabelActors(const std::vector<LabelActorSpec>& labels);
    void clearAllAtomLabelActors();
    void clearAllBondLabelActors();
    void clearAllStructureLabelActors();

    // ========================================================================
    // 결합 그룹 렌더링
    // ========================================================================
    
    /**
     * @brief 결합 그룹 초기화
     */
    void initializeBondGroup(const std::string& bondTypeKey, float radius);
    
    /**
     * @brief 결합 그룹 업데이트 (2-color 시스템)
     */
    void updateBondGroup(
        const std::string& bondTypeKey,
        const std::vector<vtkSmartPointer<vtkTransform>>& transforms1,
        const std::vector<vtkSmartPointer<vtkTransform>>& transforms2,
        // const std::vector<ImVec4>& colors1, const std::vector<ImVec4>& colors2
        const std::vector<atoms::domain::Color4f>& colors1, 
        const std::vector<atoms::domain::Color4f>& colors2
    );
    
    /**
     * @brief 특정 결합 그룹 제거
     */
    void clearBondGroup(const std::string& bondTypeKey);
    
    /**
     * @brief 모든 결합 그룹 제거
     */
    void clearAllBondGroups();
    
    /**
     * @brief 결합 두께 전체 업데이트
     */
    void updateAllBondGroupThickness(float thickness);
    
    /**
     * @brief 결합 투명도 전체 업데이트
     */
    void updateAllBondGroupOpacity(float opacity);
    
    // ========================================================================
    // 유닛 셀 렌더링
    // ========================================================================
    
    /**
     * @brief 유닛 셀 생성
     */
    void createUnitCell(const float matrix[3][3]);

    /**
     * @brief 구조별 유닛 셀 생성
     */
    void createUnitCell(int32_t structureId, const float matrix[3][3]);
    
    /**
     * @brief 유닛 셀 제거
     */
    void clearUnitCell();

    /**
     * @brief 구조별 유닛 셀 제거
     */
    void clearUnitCell(int32_t structureId);
    
    /**
     * @brief 유닛 셀 가시성 설정
     */
    void setUnitCellVisible(bool visible);

    /**
     * @brief 구조별 유닛 셀 가시성 설정
     */
    void setUnitCellVisible(int32_t structureId, bool visible);

    /**
     * @brief 구조별 유닛 셀 존재 여부 확인
     */
    bool hasUnitCell(int32_t structureId) const;

    /**
     * @brief 구조별 유닛 셀 가시성 조회
     */
    bool isUnitCellVisible(int32_t structureId) const;

    // ========================================================================
    // ⭐ BZ Plot 렌더링 (신규 추가)
    // ========================================================================
    
    /**
     * @brief BZ Plot 생성
     * @param bzData BZ 계산 결과 데이터
     */
    void createBZPlot(const domain::BZVerticesResult& bzData);
    
    /**
     * @brief BZ Plot 제거
     */
    void clearBZPlot();
    
    /**
     * @brief BZ Plot 가시성 설정
     */
    void setBZPlotVisible(bool visible);
    
    /**
     * @brief BZ Plot 가시성 조회
     */
    bool isBZPlotVisible() const;

    /**
     * @brief 완전한 BZ Plot 생성 (모든 요소 포함)
     * @param bzData BZ 계산 결과
     * @param cell 
     * @param icell 역격자 벡터 3x3 행렬
     * @param showVectors 역격자 벡터 표시 여부
     * @param showLabels 특수점 라벨 표시 여부
     * @param pathInput 밴드 경로 문자열 (예: "GXMGRX")
     * @param npoints 보간 점 개수
     */
    void createCompleteBZPlot(
        const domain::BZVerticesResult& bzData,
        const double cell[3][3], 
        const double icell[3][3],
        bool showVectors = true,
        bool showLabels = true,
        const std::string& pathInput = "",
        int npoints = 50
    );
    
    /**
     * @brief 역격자 벡터 렌더링 (b1, b2, b3)
     * @param icell 역격자 벡터 3x3 행렬
     */
    void renderReciprocalVectors(const double icell[3][3]);
    
    /**
     * @brief 밴드 경로 렌더링
     * @param kpoints k-점 좌표 배열
     */
    void renderBandpath(const std::vector<std::array<double, 3>>& kpoints);
    
    /**
     * @brief K-points 렌더링 (빨간색 점)
     * @param kpoints k-점 좌표 배열
     * @param radius 점 반지름
     */
    void renderKpoints(
        const std::vector<std::array<double, 3>>& kpoints,
        double radius = 0.05
    );
    
    /**
     * @brief 특수점 라벨 렌더링
     * @param specialPoints 특수점 이름과 좌표 맵
     */
    void renderSpecialPointLabels(
        const std::map<std::string, std::array<double, 3>>& specialPoints
    );
    
    // ========================================================================
    // 렌더링 설정
    // ========================================================================
    
    /**
     * @brief 구 해상도 설정
     */
    void setSphereResolution(int resolution) { m_sphereResolution = resolution; }
    
    /**
     * @brief 구 해상도 조회
     */
    int getSphereResolution() const { return m_sphereResolution; }

    // ========== Getter 추가 ==========
    const std::map<std::string, AtomGroupVTK>& getAtomGroups() const { 
        return m_atomGroups; 
    }
    // =================================    
    
private:
    // ========================================================================
    // 내부 데이터 구조
    // ========================================================================
    

    /**
     * @brief 결합 그룹 VTK 파이프라인 구조체 (2-color 시스템)
     */
    struct BondGroupVTK {
        vtkSmartPointer<vtkPolyData> baseGeometry;           // 기본 실린더 기하학
        vtkSmartPointer<vtkAppendPolyData> appender1;        // 첫 번째 색상 결합기
        vtkSmartPointer<vtkAppendPolyData> appender2;        // 두 번째 색상 결합기
        vtkSmartPointer<vtkPolyDataMapper> mapper1;          // 첫 번째 색상 매퍼
        vtkSmartPointer<vtkPolyDataMapper> mapper2;          // 두 번째 색상 매퍼
        vtkSmartPointer<vtkActor> actor1;                    // 첫 번째 색상 액터
        vtkSmartPointer<vtkActor> actor2;                    // 두 번째 색상 액터
        float baseRadius;                                     // 기본 반지름
        
        BondGroupVTK() : baseRadius(0.1f) {}
        
        bool isInitialized() const {
            return baseGeometry && appender1 && appender2 && 
                   mapper1 && mapper2 && actor1 && actor2;
        }
    };
    
    // ========================================================================
    // 내부 헬퍼 함수
    // ========================================================================
    
    /**
     * @brief 기본 구 geometry 생성
     */
    vtkSmartPointer<vtkPolyData> createSphereGeometry(float radius);
    
    /**
     * @brief 기본 실린더 geometry 생성
     */
    vtkSmartPointer<vtkPolyData> createCylinderGeometry(float radius, float height);
    
    /**
     * @brief 액터 속성 설정
     */
    void setupActorProperties(vtkSmartPointer<vtkActor> actor, bool isBond = false);
    
    // ========================================================================
    // 멤버 변수
    // ========================================================================
    
    // 원자 그룹 맵
    std::map<std::string, AtomGroupVTK> m_atomGroups;
    
    // 결합 그룹 맵
    std::map<std::string, BondGroupVTK> m_bondGroups;
    
    // 구조별 유닛 셀 액터들
    std::unordered_map<int32_t, std::vector<vtkSmartPointer<vtkActor>>> m_cellEdgeActorsByStructure;
    std::unordered_map<int32_t, bool> m_unitCellVisibleByStructure;
    bool m_unitCellGlobalHidden = false;

    // BZ Plot 레이어 (신규 추가)
    std::vector<vtkSmartPointer<vtkActor>> m_bzPlotActors;
    std::vector<vtkSmartPointer<vtkActor2D>> m_bzPlotActors2D;
    bool m_bzPlotVisible;
    std::unordered_map<uint32_t, vtkSmartPointer<vtkActor2D>> m_atomLabelActors2D;
    std::unordered_map<uint32_t, vtkSmartPointer<vtkActor2D>> m_bondLabelActors2D;
    
    // 렌더링 설정
    int m_sphereResolution;
    float m_bondThickness;
    float m_bondOpacity;
    

    double m_bzScaleFactor = 0.1;  // BZ Plot 스케일 팩터

    /**
     * @brief 화살표 Actor 생성 (역격자 벡터용)
     * @param start 시작점
     * @param end 끝점
     * @param color 색상 (기본: 검은색)
     * @param tipLength 화살표 머리 길이 (world 단위)
     * @param shaftRadius 샤프트 반지름
     */
    vtkSmartPointer<vtkActor> createArrowActor(
        const double start[3],
        const double end[3],
        const double color[3] = nullptr,
        double tipLength = 0.2,
        double shaftRadius = 0.02
    );

    /**
     * @brief 텍스트 Actor 생성 (라벨용)
     * @param text 표시할 텍스트
     * @param position 위치
     * @param fontSize 폰트 크기
     * @param color 색상 (기본: 검은색)
     */
    vtkSmartPointer<vtkActor> createTextActor(
        const std::string& text,
        const double position[3],
        int fontSize = 18,
        const double color[3] = nullptr
    );

    /**
     * @brief 2D 텍스트 Actor 생성 (카메라 회전에 영향 받지 않음)
     * @param text 표시할 텍스트
     * @param position 월드 좌표 위치
     * @param fontSize 폰트 크기 (픽셀)
     * @param color 색상 (기본: 검은색)
     */
    vtkSmartPointer<vtkActor2D> createTextActor2D(
        const std::string& text,
        const double position[3],
        int fontSize = 14,
        const double color[3] = nullptr
    );

    /**
     * @brief 역격자 벡터의 최대 길이 계산
     * @param icell 역격자 벡터 3x3 행렬
     * @return 최대 벡터 길이
     */
    double calculateMaxReciprocalVectorLength(const double icell[3][3]);
};

} // namespace infrastructure
} // namespace atoms
