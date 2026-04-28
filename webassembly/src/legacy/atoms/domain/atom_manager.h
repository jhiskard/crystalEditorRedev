#pragma once

#include "../../config/log_config.h"
#include "color.h"

#include <string>
// #include <imgui.h> 
#include <vector>
#include <cstdint>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>


class AtomsTemplate;

namespace atoms {
namespace domain {

class ElementDatabase;

enum class AtomType {
    ORIGINAL,      // 원본 원자 (기본값)
    SURROUNDING    // 주변 원자 (반투명)
};

/**
 * @brief 원자 정보를 저장하는 구조체 (레거시 제거 완료)
 * 
 * 각 원자의 기본 속성과 통합 렌더링을 위한 데이터를 포함합니다.
 * 모든 원자는 CPU 인스턴싱 모드로만 렌더링됩니다.
 * 
 * 좌표 시스템:
 * - position/fracPosition: 현재 확정된 좌표 (카르테시안/분수)
 * - tempPosition/tempFracPosition: 편집 중인 임시 좌표
 * 
 * 통합 렌더링 시스템:
 * - AtomType으로 원본/주변 구분
 * - isInstanced = true (항상 CPU 인스턴싱 사용)
 * - VTK 개별 객체는 완전 제거
 */
struct AtomInfo {
    // ========================================================================
    // CORE PROPERTIES
    // ========================================================================
    std::string symbol;
    // ImVec4 color;
    Color4f color;
    float radius;          // 뷰어 구 반경(표시 전용)
    float bondRadius = 0.0f; // 결합 판단 전용 내부 반경
    float position[3];       // Cartesian 좌표
    float fracPosition[3];   // Fractional 좌표
    
    // ========================================================================
    // EDITING SYSTEM
    // ========================================================================
    std::string tempSymbol;
    float tempPosition[3];     // 임시 Cartesian 좌표
    float tempFracPosition[3]; // 임시 Fractional 좌표
    float tempRadius;
    bool modified = false;     // 수정 여부 플래그
    bool selected = false;     // 선택 여부 플래그
    
    // ========================================================================
    // UNIFIED RENDERING SYSTEM ONLY
    // ========================================================================
    uint32_t id = 0;           // 고유 ID
    int32_t structureId = -1;  // 구조 ID (XSF/CHGCAR 항목)
    AtomType atomType = AtomType::ORIGINAL; // 원본/주변 타입 구분
    bool isInstanced = true;   // 🔧 항상 true (CPU 인스턴싱만 사용)
    size_t instanceIndex = SIZE_MAX; // 그룹 내 인스턴스 인덱스
    uint32_t originalAtomId = 0;  // SURROUNDING 원자의 경우 원본 ORIGINAL 원자의 ID
    
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    
    // 기본 생성자
    AtomInfo() = default;
    
    // 통합 생성자 (레거시 VTK 초기화 제거)
    AtomInfo(const std::string& sym, const Color4f& col, float rad, 
             const float pos[3], AtomType type = AtomType::ORIGINAL)
        : symbol(sym), color(col), radius(rad), bondRadius(rad), atomType(type), isInstanced(true) {
        position[0] = pos[0]; 
        position[1] = pos[1]; 
        position[2] = pos[2];
        
        // 분수 좌표는 나중에 설정
        fracPosition[0] = fracPosition[1] = fracPosition[2] = 0.0f;
        
        // 임시 값들 초기화
        tempSymbol = sym;
        tempPosition[0] = pos[0]; tempPosition[1] = pos[1]; tempPosition[2] = pos[2];
        tempFracPosition[0] = tempFracPosition[1] = tempFracPosition[2] = 0.0f;
        tempRadius = rad;
        
        // 🔧 VTK 개별 객체 완전 제거 - 더 이상 초기화하지 않음
        // ❌ 제거됨: actor, sphereSource, mapper 멤버 자체가 삭제
    }
};

/**
 * @brief 통합 원자 그룹 정보 구조체 (VTK 파이프라인 포함)
 * 
 * 레거시 시스템 제거 후 모든 VTK 렌더링 파이프라인을 
 * 통합 관리하도록 구조체를 확장합니다.
 */
struct AtomGroupInfo {
    std::string element;      // 원소 기호 (예: "C", "H", "O")
    float baseRadius;         // 기본 반지름
    
    // ========================================================================
    // UNIFIED GROUP DATA (원본 + 주변 통합)
    // ========================================================================
    std::vector<vtkSmartPointer<vtkTransform>> transforms;  // 위치/크기 변환
    // std::vector<ImVec4> colors;                            // 색상 (타입별 조정됨)
    std::vector<Color4f> colors;
    std::vector<uint32_t> atomIds;                         // 고유 ID 목록
    std::vector<AtomType> atomTypes;                       // 원본/주변 타입
    std::vector<size_t> originalIndices;                   // createdAtoms/surroundingAtoms 인덱스
    
    // ========================================================================
    // VTK RENDERING PIPELINE (레거시 대체)
    // ========================================================================
    vtkSmartPointer<vtkPolyData> baseGeometry;           // 기본 구 기하학적 데이터
    vtkSmartPointer<vtkAppendPolyData> appender;         // 인스턴스 결합기
    vtkSmartPointer<vtkPolyDataMapper> mapper;           // VTK 매퍼
    vtkSmartPointer<vtkActor> actor;                     // VTK 액터
    
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    AtomGroupInfo() : baseRadius(0.5f) {
        // VTK 파이프라인 초기화
        baseGeometry = nullptr;
        appender = nullptr;
        mapper = nullptr;
        actor = nullptr;
    }
    
    AtomGroupInfo(const std::string& elem, float radius) 
        : element(elem), baseRadius(radius) {
        // VTK 파이프라인 초기화
        baseGeometry = nullptr;
        appender = nullptr;
        mapper = nullptr;
        actor = nullptr;
    }
    
    // ========================================================================
    // VTK PIPELINE MANAGEMENT
    // ========================================================================
    
    // [수정] VTK 직접 관리 제거
    void initializeVTKPipeline() {
        // [삭제] 기존 VTK 파이프라인 초기화 코드
        // baseGeometry = vtkSmartPointer<vtkPolyData>::New();
        // appender = vtkSmartPointer<vtkAppendPolyData>::New();
        // mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        // actor = vtkSmartPointer<vtkActor>::New();
        // VtkViewer::Instance().AddActor(actor);
        
        // [추가] 로그만 출력 (실제 VTK는 VTKRenderer가 관리)
        SPDLOG_DEBUG("AtomGroupInfo initialized for element: {} (VTK managed by VTKRenderer)", element);
    }

    // [수정] VTK 직접 관리 제거
    void cleanupVTKPipeline() {
        // [삭제] 기존 VTK 정리 코드
        // if (actor) {
        //     VtkViewer::Instance().RemoveActor(actor);
        //     actor = nullptr;
        // }
        // if (appender) {
        //     appender->RemoveAllInputs();
        //     appender = nullptr;
        // }
        
        // [유지] 데이터 정리
        transforms.clear();
        colors.clear();
        atomIds.clear();
        atomTypes.clear();
        originalIndices.clear();
        
        // [추가] 로그 메시지 변경
        SPDLOG_DEBUG("AtomGroupInfo cleaned up for element: {} (VTK managed by VTKRenderer)", element);
    }

    // [수정] 판단 기준 변경
    bool isVTKPipelineInitialized() const {
        // [삭제] 기존 VTK 객체 확인
        // return (baseGeometry && appender && mapper && actor);
        
        // [추가] 데이터 존재 여부로 판단
        return !transforms.empty();
    }
};

// 전역 원자 벡터 선언 (extern)
extern std::vector<atoms::domain::AtomInfo> createdAtoms;
extern std::vector<atoms::domain::AtomInfo> surroundingAtoms; 
extern std::map<std::string, atoms::domain::AtomGroupInfo> atomGroups;

/// 심볼 기준 그룹 초기화 (없으면 생성, 있으면 baseRadius 갱신)
void initializeAtomGroup(const std::string& symbol, float radius);

/// 그룹 존재 여부 확인
bool hasAtomGroup(const std::string& symbol);

/// 그룹 포인터 조회 (없으면 nullptr)
AtomGroupInfo* findAtomGroup(const std::string& symbol);

/// 그룹에 원자 추가 (순수 도메인 조작)
void addAtomToGroup(const std::string& symbol,
                    vtkSmartPointer<vtkTransform> transform,
                    // const ImVec4& color,
                    const Color4f& color,
                    uint32_t atomId,
                    AtomType atomType,
                    size_t originalIndex);

/// 그룹에서 atomId 기준으로 원자 제거, 제거했으면 true
bool removeAtomFromGroup(const std::string& symbol, uint32_t atomId);

/// 모든 atomGroups 클리어
void clearAllAtomGroups();

/// read-only 조회용
const std::map<std::string, AtomGroupInfo>& getAtomGroups();

bool getAtomPositionAndRadius(
    const atoms::domain::AtomInfo& atom, double position[3], double& radius
);

/// Atom ID 생성기 (ORIGINAL/SURROUNDING 공용)
uint32_t generateUniqueAtomId();
bool isSurroundingsVisible();
void setSurroundingsVisible(bool visible);

void applyAtomChanges(::AtomsTemplate* parent,
                      float bondScalingFactor,
                      const ElementDatabase& elementDB);

} //atoms
} // domain 
