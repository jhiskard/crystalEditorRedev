#pragma once

#include "../../config/log_config.h"
#include "atom_manager.h"
#include "element_database.h"
#include "color.h"

#include <string>
#include <vector>
#include <set>
#include <utility>

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkAppendPolyData.h>
#include <vtkMatrix4x4.h>

class AtomsTemplate;


namespace atoms {
namespace domain {

enum class BondType {
    ORIGINAL,      // 원본 결합 (불투명)
    SURROUNDING    // 주변 결합 (반투명)
};

/**
 * @brief 결합 정보를 저장하는 구조체 (레거시 제거 완료)
 * 
 * 모든 결합은 CPU 인스턴싱 모드로만 렌더링되며,
 * 2-color 시스템을 통해 두 원자의 색상을 구분 표시합니다.
 */
struct BondInfo {
    // ========================================================================
    // ATOM INDICES
    // ========================================================================
    int atom1Index;                    // 첫 번째 원자의 인덱스
    int atom2Index;                    // 두 번째 원자의 인덱스
    uint32_t atom1Id = 0;              // 첫 번째 원자의 고유 ID (인덱스 해석 모호성 방지)
    uint32_t atom2Id = 0;              // 두 번째 원자의 고유 ID (인덱스 해석 모호성 방지)
    
    // ========================================================================
    // UNIFIED RENDERING SYSTEM ONLY
    // ========================================================================
    uint32_t id = 0;                   // 고유 ID
    int32_t structureId = -1;          // 구조 ID (XSF/CHGCAR 항목)
    bool isInstanced = true;           // 🔧 항상 true (CPU 인스턴싱만 사용)
    std::string bondGroupKey;          // 결합 그룹 키 (예: "C-H")
    size_t instanceIndex = SIZE_MAX;   // 그룹 내 인스턴스 인덱스
    BondType bondType = BondType::ORIGINAL; // 결합 타입 (원본/주변)
    
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    BondInfo() = default;
    
    // 🔧 레거시 생성자 제거 - VTK 개별 액터 기반 생성자 삭제
    // ❌ 제거됨: BondInfo(vtkSmartPointer<vtkActor> a1, vtkSmartPointer<vtkActor> a2, ...)
    
    // 통합 인스턴싱 생성자만 유지
    BondInfo(uint32_t bondId, int idx1, int idx2, const std::string& groupKey, 
             size_t instIdx, BondType type = BondType::ORIGINAL)
        : id(bondId), atom1Index(idx1), atom2Index(idx2), isInstanced(true),
          bondGroupKey(groupKey), instanceIndex(instIdx), bondType(type) {
        // 🔧 VTK 개별 객체 완전 제거 - 더 이상 초기화하지 않음
        // ❌ 제거됨: actor1, actor2 멤버 자체가 삭제
    }
};

extern std::vector<atoms::domain::BondInfo> createdBonds;
extern std::vector<atoms::domain::BondInfo> surroundingBonds; // 주변 원자들의 결합


struct BondGroupInfo {
    std::string element1;
    std::string element2;
    float radius;

    std::string key;
    float baseRadius = 0.0f;
    
    std::vector<vtkSmartPointer<vtkTransform>> transforms1;  // 첫 번째 색상 실린더
    std::vector<vtkSmartPointer<vtkTransform>> transforms2;  // 두 번째 색상 실린더
    // std::vector<ImVec4> colors1;  // 첫 번째 색상들
    // std::vector<ImVec4> colors2;  // 두 번째 색상들
    std::vector<Color4f> colors1;
    std::vector<Color4f> colors2;
    std::vector<uint32_t> bondIds;
    
    BondGroupInfo() : radius(0.1f) {}
    
    BondGroupInfo(const std::string& elem1, const std::string& elem2, float r) 
        : element1(elem1), element2(elem2), radius(r) {}
};

extern std::map<std::string, atoms::domain::BondGroupInfo> bondGroups;
// extern std::set<std::string> bondGroupsToUpdate;

// Phase 1: 도메인 API
bool hasBondGroup(const std::string& key);
BondGroupInfo* findBondGroup(const std::string& key);
void initializeBondGroup(const std::string& key, float baseRadius);
void clearAllBondGroups();
const std::map<std::string, BondGroupInfo>& getBondGroups();
void clearAllBonds(AtomsTemplate* parent);
std::string generateBondTypeKey(const std::string& element1, const std::string& element2);
float calculateBondRadius(const AtomInfo& atom1, const AtomInfo& atom2);
int findAtomIndex(const AtomInfo& atom, const std::vector<AtomInfo>& atomList);
AtomInfo* findAtomById(uint32_t atomId, std::vector<AtomInfo>& atomList);
std::pair<vtkSmartPointer<vtkTransform>, vtkSmartPointer<vtkTransform>>
calculateBondTransforms2Color(const AtomInfo& atom1, const AtomInfo& atom2);
void addBondToGroup2Color(
    const std::string& bondTypeKey,
    vtkSmartPointer<vtkTransform> transform1,
    vtkSmartPointer<vtkTransform> transform2,
    const Color4f& color1,
    const Color4f& color2,
    uint32_t bondId
);
bool removeBondFromGroup(const std::string& bondTypeKey, size_t instanceIndex);
void createBond(
    AtomsTemplate* parent,
    const AtomInfo& atom1,
    const AtomInfo& atom2,
    BondType bondType,
    float bondScalingFactor,
    const ElementDatabase& elementDB
);
void createBondsForAtoms(
    AtomsTemplate* parent,
    const std::vector<uint32_t>& atomIds,
    bool includeOriginal,
    bool includeSurrounding,
    bool clearExisting,
    float bondScalingFactor,
    const ElementDatabase& elementDB
);
void addBondsToGroups(
    AtomsTemplate* parent,
    int atomIndex,
    float bondScalingFactor,
    const ElementDatabase& elementDB
);
void updateAllBondGroupThickness(AtomsTemplate* parent, float thickness);
void updateAllBondGroupOpacity(AtomsTemplate* parent, float opacity);

/**
 * @brief 두 원자 사이에 결합을 생성할지 판단 (도메인 로직)
 *
 * 기존 AtomsTemplate::shouldCreateBond 로직을 유지하되,
 * - AtomInfo 데이터 + ElementDatabase + 스케일 팩터만 입력으로 받도록 정리.
 */
bool shouldCreateBond(
    const AtomInfo& atom1,
    const AtomInfo& atom2,
    const ElementDatabase& elementDB,
    float bondScalingFactor
);

} //atoms
} // domain 
