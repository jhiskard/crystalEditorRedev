# AtomsTemplate 리팩토링 계획 v2.0 (네임스페이스 유지 방식)
## (안정성 우선, 점진적 개선 전략)

## 1. 프로젝트 개요

현재 `atoms_template.h/cpp` 파일은 약 3,500줄의 단일체(Monolithic) 구조입니다. **네임스페이스 유지 방식**을 적용하여 기존 코드의 안정성을 보장하면서 점진적으로 리팩토링을 진행합니다.

### 1.1 변경된 전략
- **기존 계획**: 클래스 기반 AtomManager, BondManager, CellManager 도입
- **새로운 계획**: 네임스페이스 기반 함수 분리, 전역 변수 유지
- **핵심 원칙**: 기존 코드 최대한 보존, 점진적 함수 이동

### 1.2 목표 아키텍처

```
┌─────────────────────────────────────┐
│           UI Layer                  │
│  - renderCreatedAtomsSection()      │
│  - renderBondsManagementTools()     │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│        Application Layer            │
│  - AtomsTemplate (Controller)       │
│  - 기존 비즈니스 로직 점진적 위임          │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│    Domain Layer (네임스페이스 방식)     │
│  - atoms::domain::*                 │
│  - 전역 변수 유지 + 함수 분리            │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│    Infrastructure Layer             │
│  - BatchUpdateSystem ← 완료          │
│  - VTKRenderer ← 완료                │
│  - FileIOManager ← 완료              │
└─────────────────────────────────────┘
```

## 2. 리팩토링 단계별 계획 (업데이트)

### Phase 1: Infrastructure 분리 (✅ 완료)
- BatchUpdateSystem, VTKRenderer, FileIOManager 분리 완료

### Phase 2: Domain 네임스페이스 함수 분리 (2-3일) 🎯 **핵심 작업**
1. AtomManager 함수 분리 (네임스페이스 기반)
2. BondManager 함수 분리 (네임스페이스 기반)
3. CellManager 함수 분리 (네임스페이스 기반)
4. ElementDatabase 통합 (기존 완료)

### Phase 3: UI 레이어 분리 (2일)
1. UI 렌더링 함수 추출
2. 이벤트 핸들링 분리

### Phase 4: 통합 및 정리 (1일)
1. 위임 래퍼 구현
2. 통합 테스트
3. 문서화

## 3. 상세 작업 목록 (네임스페이스 방식)

### 3.1 디렉토리 구조 (변경사항 없음)

```bash
webassembly/src/atoms/
├── infrastructure/              # ✅ 완료
│   ├── batch_update_system.h/cpp
│   ├── vtk_renderer.h/cpp
│   └── file_io_manager.h/cpp
├── domain/                      # 🎯 네임스페이스 방식으로 확장
│   ├── atom_manager.h/cpp       # 전역변수 + 네임스페이스 함수
│   ├── bond_manager.h/cpp       # 전역변수 + 네임스페이스 함수
│   ├── cell_manager.h/cpp       # 전역변수 + 네임스페이스 함수
│   └── element_database.h/cpp   # ✅ 완료 (클래스 방식 유지)
├── ui/                          # Phase 3에서 생성
│   └── atoms_template_ui.h/cpp
└── atoms_template.h/cpp        # 위임 래퍼로 점진적 수정
```

### 3.2 Phase 2.1: AtomManager 네임스페이스 함수 분리 (1일)

#### Task 2.1.1: atom_manager.h 확장
```cpp
// atoms/domain/atom_manager.h
#pragma once

#include "../../config/log_config.h"
#include <string>
#include <imgui.h>
#include <vector>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

namespace atoms {
namespace domain {

// ========================================================================
// EXISTING DATA STRUCTURES (기존 유지)
// ========================================================================
enum class AtomType {
    ORIGINAL,      // 원본 원자 (기본값)
    SURROUNDING    // 주변 원자 (반투명)
};

struct AtomInfo {
    // 기존 구조체 내용 그대로 유지
    std::string symbol;
    ImVec4 color;
    float radius;
    float position[3];
    float fracPosition[3];
    // ... 기존 필드들 모두 유지
};

struct AtomGroupInfo {
    // 기존 구조체 내용 그대로 유지
    std::string element;
    float baseRadius;
    std::vector<vtkSmartPointer<vtkTransform>> transforms;
    std::vector<ImVec4> colors;
    // ... 기존 필드들 모두 유지
};

// ========================================================================
// GLOBAL VARIABLES (기존 유지)
// ========================================================================
extern std::vector<atoms::domain::AtomInfo> createdAtoms;
extern std::vector<atoms::domain::AtomInfo> surroundingAtoms;
extern std::map<std::string, atoms::domain::AtomGroupInfo> atomGroups;

// ========================================================================
// ATOM MANAGEMENT FUNCTIONS (atoms_template.cpp에서 이동)
// ========================================================================

/**
 * @brief 원자 구체 생성 함수 (atoms_template.cpp에서 이동)
 */
void createAtomSphere(const char* symbol, 
                      const ImVec4& color, 
                      float radius,
                      const float position[3], 
                      AtomType atomType = AtomType::ORIGINAL);

/**
 * @brief 원자 그룹 초기화 (atoms_template.cpp에서 이동)
 */
void initializeAtomGroup(const std::string& symbol, float radius);

/**
 * @brief 고유 원자 ID 생성 (atoms_template.cpp에서 이동)
 */
uint32_t generateUniqueAtomId();

/**
 * @brief 원자 그룹에 원자 추가 (atoms_template.cpp에서 이동)
 */
void addAtomToGroup(const std::string& symbol,
                    uint32_t atomId,
                    vtkSmartPointer<vtkTransform> transform,
                    const ImVec4& color,
                    AtomType atomType,
                    size_t originalIndex);

/**
 * @brief 통합 원자 그룹 VTK 업데이트 (atoms_template.cpp에서 이동)
 */
void updateUnifiedAtomGroupVTK(const std::string& symbol);

/**
 * @brief 주변 원자 생성 (atoms_template.cpp에서 이동)
 */
void createSurroundingAtoms(const float cellMatrix[3][3]);

/**
 * @brief 원자 제거 (atoms_template.cpp에서 이동)
 */
void removeAtom(uint32_t atomId, std::vector<AtomInfo>& atomList);

/**
 * @brief 선택된 원자들 삭제 (atoms_template.cpp에서 이동)
 */
void deleteSelectedAtoms();

/**
 * @brief 모든 원자 삭제 (atoms_template.cpp에서 이동)
 */
void clearAllAtoms();

/**
 * @brief 원자 위치 업데이트 (atoms_template.cpp에서 이동)
 */
void updateAtomPosition(uint32_t atomId, const float position[3]);

/**
 * @brief 원자 찾기 (atoms_template.cpp에서 이동)
 */
AtomInfo* findAtomById(uint32_t atomId, std::vector<AtomInfo>& atomList);

// ========================================================================
// UTILITY FUNCTIONS
// ========================================================================

/**
 * @brief 원자 개수 조회
 */
size_t getAtomCount(AtomType type = AtomType::ORIGINAL);

/**
 * @brief 원자 그룹 개수 조회
 */
size_t getAtomGroupCount();

/**
 * @brief 메모리 사용량 계산
 */
float calculateAtomMemoryUsage();

} // namespace domain
} // namespace atoms
```

#### Task 2.1.2: atom_manager.cpp 구현
```cpp
// atoms/domain/atom_manager.cpp
#include "atom_manager.h"
#include "../infrastructure/vtk_renderer.h"
#include "../infrastructure/batch_update_system.h"
#include "element_database.h"
#include "../../vtk_viewer.h"

namespace atoms {
namespace domain {

// ========================================================================
// GLOBAL VARIABLES DEFINITION (기존 유지)
// ========================================================================
std::vector<atoms::domain::AtomInfo> createdAtoms;
std::vector<atoms::domain::AtomInfo> surroundingAtoms;
std::map<std::string, atoms::domain::AtomGroupInfo> atomGroups;

// ========================================================================
// ATOM CREATION IMPLEMENTATION (atoms_template.cpp에서 이동)
// ========================================================================

void createAtomSphere(const char* symbol, const ImVec4& color, float radius, 
                      const float position[3], AtomType atomType) {
    
    // [atoms_template.cpp의 createAtomSphere 함수 내용을 그대로 복사]
    
    std::string symbolStr(symbol);
    
    // 통합 원자 그룹 초기화
    initializeAtomGroup(symbolStr, radius);
    
    float adjustedRadius = radius * 0.5f;
    
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    transform->Translate(position[0], position[1], position[2]);
    
    float scaleFactor = adjustedRadius / 0.5f;
    transform->Scale(scaleFactor, scaleFactor, scaleFactor);
    
    // AtomType에 따른 색상 조정
    ImVec4 adjustedColor = color;
    if (atomType == AtomType::SURROUNDING) {
        float colorMultiplier = 200.0f / 255.0f;
        adjustedColor.x *= colorMultiplier;
        adjustedColor.y *= colorMultiplier;
        adjustedColor.z *= colorMultiplier;
    }
    
    // 고유 ID 생성
    uint32_t atomId = generateUniqueAtomId();
    
    // 통합 그룹 시스템에만 추가
    size_t originalIndex = (atomType == AtomType::ORIGINAL) ?
        createdAtoms.size() : surroundingAtoms.size();
    
    addAtomToGroup(symbolStr, atomId, transform, adjustedColor, atomType, originalIndex);
    
    // AtomInfo 생성 및 추가
    AtomInfo newAtom(symbolStr, adjustedColor, radius, position, atomType);
    newAtom.id = atomId;
    newAtom.instanceIndex = atomGroups[symbolStr].transforms.size() - 1;
    
    if (atomType == AtomType::ORIGINAL) {
        createdAtoms.push_back(newAtom);
    } else {
        surroundingAtoms.push_back(newAtom);
    }
    
    // VTK 업데이트 스케줄링
    // BatchUpdateSystem을 통해 스케줄링 (Infrastructure 레이어 활용)
    SPDLOG_DEBUG("Created {} atom: {} at ({:.3f}, {:.3f}, {:.3f})", 
                (atomType == AtomType::ORIGINAL ? "ORIGINAL" : "SURROUNDING"),
                symbolStr, position[0], position[1], position[2]);
}

void initializeAtomGroup(const std::string& symbol, float radius) {
    // [atoms_template.cpp의 initializeAtomGroup 함수 내용을 그대로 복사]
    
    if (atomGroups.find(symbol) != atomGroups.end()) {
        return; // 이미 초기화됨
    }
    
    atomGroups[symbol] = AtomGroupInfo(symbol, radius);
    atomGroups[symbol].initializeVTKPipeline();
    
    SPDLOG_DEBUG("Initialized atom group: {} with radius {:.3f}", symbol, radius);
}

uint32_t generateUniqueAtomId() {
    // [atoms_template.cpp의 generateUniqueAtomId 함수 내용을 그대로 복사]
    static uint32_t nextAtomId = 1;
    return nextAtomId++;
}

void addAtomToGroup(const std::string& symbol,
                    uint32_t atomId,
                    vtkSmartPointer<vtkTransform> transform,
                    const ImVec4& color,
                    AtomType atomType,
                    size_t originalIndex) {
    
    // [atoms_template.cpp의 addAtomToGroup 함수 내용을 그대로 복사]
    
    auto& group = atomGroups[symbol];
    group.transforms.push_back(transform);
    group.colors.push_back(color);
    group.atomIds.push_back(atomId);
    group.atomTypes.push_back(atomType);
    group.originalIndices.push_back(originalIndex);
    
    SPDLOG_DEBUG("Added atom {} to group {}, total atoms in group: {}", 
                atomId, symbol, group.transforms.size());
}

// [다른 함수들도 동일하게 atoms_template.cpp에서 이동]
// updateUnifiedAtomGroupVTK, createSurroundingAtoms, removeAtom 등...

} // namespace domain
} // namespace atoms
```

#### Task 2.1.3: atoms_template.cpp 수정 (위임 구현)
```cpp
// atoms_template.cpp
#include "domain/atom_manager.h"

// 기존 함수를 위임으로 변경
void AtomsTemplate::createAtomSphere(const char* symbol, const ImVec4& color, float radius, 
                                    const float position[3], atoms::domain::AtomType atomType) {
    
    // 🎯 AtomManager 네임스페이스 함수로 위임
    atoms::domain::createAtomSphere(symbol, color, radius, position, atomType);
}

void AtomsTemplate::initializeAtomGroup(const std::string& symbol, float radius) {
    // 🎯 AtomManager 네임스페이스 함수로 위임
    atoms::domain::initializeAtomGroup(symbol, radius);
}

uint32_t AtomsTemplate::generateUniqueAtomId() {
    // 🎯 AtomManager 네임스페이스 함수로 위임
    return atoms::domain::generateUniqueAtomId();
}

// 전역 변수 접근은 네임스페이스를 통해 계속 사용
void AtomsTemplate::someFunction() {
    // 기존 코드 그대로 유지 가능
    for (auto& atom : atoms::domain::createdAtoms) {
        // 기존 로직 그대로
    }
    
    // 원자 그룹 접근도 그대로
    atoms::domain::atomGroups["C"].transforms.push_back(transform);
}
```

### 3.3 Phase 2.2: BondManager 네임스페이스 함수 분리 (1일)

#### Task 2.2.1: bond_manager.h 확장
```cpp
// atoms/domain/bond_manager.h
#pragma once

#include <string>
#include <imgui.h>
#include <vector>
#include <set>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkAppendPolyData.h>

namespace atoms {
namespace domain {

// ========================================================================
// EXISTING DATA STRUCTURES (기존 유지)
// ========================================================================
enum class BondType {
    ORIGINAL,      // 원본 결합 (불투명)
    SURROUNDING    // 주변 결합 (반투명)
};

struct BondInfo {
    // 기존 구조체 내용 그대로 유지
    int atom1Index;
    int atom2Index;
    uint32_t id = 0;
    bool isInstanced = true;
    std::string bondGroupKey;
    size_t instanceIndex = SIZE_MAX;
    BondType bondType = BondType::ORIGINAL;
    // ... 기존 필드들 모두 유지
};

struct BondGroupInfo {
    // 기존 구조체 내용 그대로 유지
    std::string element1;
    std::string element2;
    float radius;
    std::vector<vtkSmartPointer<vtkTransform>> transforms1;
    std::vector<vtkSmartPointer<vtkTransform>> transforms2;
    std::vector<ImVec4> colors1;
    std::vector<ImVec4> colors2;
    std::vector<uint32_t> bondIds;
    // ... 기존 필드들 모두 유지
};

// ========================================================================
// GLOBAL VARIABLES (기존 유지)
// ========================================================================
extern std::vector<atoms::domain::BondInfo> createdBonds;
extern std::vector<atoms::domain::BondInfo> surroundingBonds;
extern std::map<std::string, atoms::domain::BondGroupInfo> bondGroups;
extern std::map<std::string, vtkSmartPointer<vtkPolyData>> bondBaseGeometries;

// VTK 파이프라인 전역 변수들 (기존 유지)
extern std::map<std::string, vtkSmartPointer<vtkActor>> bondGroupActors1;
extern std::map<std::string, vtkSmartPointer<vtkActor>> bondGroupActors2;
extern std::map<std::string, vtkSmartPointer<vtkPolyDataMapper>> bondGroupMappers1;
extern std::map<std::string, vtkSmartPointer<vtkPolyDataMapper>> bondGroupMappers2;
extern std::map<std::string, vtkSmartPointer<vtkAppendPolyData>> bondGroupAppenders1;
extern std::map<std::string, vtkSmartPointer<vtkAppendPolyData>> bondGroupAppenders2;
extern std::set<std::string> bondGroupsToUpdate;

// ========================================================================
// BOND MANAGEMENT FUNCTIONS (atoms_template.cpp에서 이동)
// ========================================================================

/**
 * @brief 모든 원자 간 결합 생성 (atoms_template.cpp에서 이동)
 */
void createAllBonds();

/**
 * @brief 특정 원자들에 대한 결합 생성 (atoms_template.cpp에서 이동)
 */
void createBondsForAtoms(const std::vector<uint32_t>& atomIds);

/**
 * @brief 결합 생성 여부 판단 (atoms_template.cpp에서 이동)
 */
bool shouldCreateBond(const AtomInfo& atom1, const AtomInfo& atom2);

/**
 * @brief 결합 그룹 초기화 (atoms_template.cpp에서 이동)
 */
void initializeBondGroup(const std::string& bondTypeKey);

/**
 * @brief 결합 그룹 업데이트 (atoms_template.cpp에서 이동)
 */
void updateBondGroup(const std::string& bondTypeKey);

/**
 * @brief 모든 결합 삭제 (atoms_template.cpp에서 이동)
 */
void clearAllBonds();

/**
 * @brief 특정 원자의 결합 제거 (atoms_template.cpp에서 이동)
 */
void removeBondsForAtom(uint32_t atomId);

/**
 * @brief 결합 두께 업데이트 (atoms_template.cpp에서 이동)
 */
void updateAllBondGroupThickness(float thickness);

/**
 * @brief 결합 투명도 업데이트 (atoms_template.cpp에서 이동)
 */
void updateAllBondGroupOpacity(float opacity);

// ========================================================================
// UTILITY FUNCTIONS
// ========================================================================

/**
 * @brief 결합 개수 조회
 */
size_t getBondCount(BondType type = BondType::ORIGINAL);

/**
 * @brief 결합 그룹 개수 조회
 */
size_t getBondGroupCount();

/**
 * @brief 메모리 사용량 계산
 */
float calculateBondMemoryUsage();

} // namespace domain
} // namespace atoms
```

#### Task 2.2.2: bond_manager.cpp 구현
```cpp
// atoms/domain/bond_manager.cpp
#include "bond_manager.h"
#include "atom_manager.h"  // AtomInfo 접근
#include "../infrastructure/vtk_renderer.h"
#include "../infrastructure/batch_update_system.h"

namespace atoms {
namespace domain {

// ========================================================================
// GLOBAL VARIABLES DEFINITION (기존 유지)
// ========================================================================
std::vector<atoms::domain::BondInfo> createdBonds;
std::vector<atoms::domain::BondInfo> surroundingBonds;
std::map<std::string, atoms::domain::BondGroupInfo> bondGroups;
std::map<std::string, vtkSmartPointer<vtkPolyData>> bondBaseGeometries;

std::map<std::string, vtkSmartPointer<vtkActor>> bondGroupActors1;
std::map<std::string, vtkSmartPointer<vtkActor>> bondGroupActors2;
std::map<std::string, vtkSmartPointer<vtkPolyDataMapper>> bondGroupMappers1;
std::map<std::string, vtkSmartPointer<vtkPolyDataMapper>> bondGroupMappers2;
std::map<std::string, vtkSmartPointer<vtkAppendPolyData>> bondGroupAppenders1;
std::map<std::string, vtkSmartPointer<vtkAppendPolyData>> bondGroupAppenders2;

std::set<std::string> bondGroupsToUpdate;

// ========================================================================
// BOND MANAGEMENT IMPLEMENTATION (atoms_template.cpp에서 이동)
// ========================================================================

void createAllBonds() {
    // [atoms_template.cpp의 createAllBonds 함수 내용을 그대로 복사]
    
    SPDLOG_INFO("Creating bonds for all atoms");
    
    clearAllBonds();  // 기존 결합 삭제
    
    const auto& atoms = createdAtoms;  // 전역 변수 직접 사용
    
    for (size_t i = 0; i < atoms.size(); ++i) {
        for (size_t j = i + 1; j < atoms.size(); ++j) {
            if (shouldCreateBond(atoms[i], atoms[j])) {
                // 결합 생성 로직
                std::string bondKey = atoms[i].symbol + "-" + atoms[j].symbol;
                initializeBondGroup(bondKey);
                
                // BondInfo 생성 및 추가
                BondInfo bond;
                bond.atom1Index = i;
                bond.atom2Index = j;
                bond.id = generateUniqueBondId();
                bond.bondGroupKey = bondKey;
                bond.bondType = BondType::ORIGINAL;
                
                createdBonds.push_back(bond);
                
                // 결합 그룹에 추가
                addBondToGroup(bondKey, bond);
            }
        }
    }
    
    // 모든 결합 그룹 업데이트
    for (const auto& pair : bondGroups) {
        updateBondGroup(pair.first);
    }
    
    SPDLOG_INFO("Created {} bonds in {} groups", createdBonds.size(), bondGroups.size());
}

bool shouldCreateBond(const AtomInfo& atom1, const AtomInfo& atom2) {
    // [atoms_template.cpp의 shouldCreateBond 함수 내용을 그대로 복사]
    
    // 두 원자 간 거리 계산
    float dx = atom1.position[0] - atom2.position[0];
    float dy = atom1.position[1] - atom2.position[1];
    float dz = atom1.position[2] - atom2.position[2];
    float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // 공유결합 반지름 합계로 판단
    float bondThreshold = atom1.radius + atom2.radius;
    bondThreshold *= 1.2f;  // 여유값
    
    return distance < bondThreshold;
}

// [다른 함수들도 동일하게 atoms_template.cpp에서 이동]
// initializeBondGroup, updateBondGroup, clearAllBonds 등...

} // namespace domain
} // namespace atoms
```

### 3.4 Phase 2.3: CellManager 네임스페이스 함수 분리 (1일)

#### Task 2.3.1: cell_manager.h 확장
```cpp
// atoms/domain/cell_manager.h
#pragma once

#include <vector>
#include <vtkSmartPointer.h>
#include <vtkActor.h>

// ========================================================================
// EXISTING DATA STRUCTURES (기존 유지)
// ========================================================================
struct CellInfo {
    float matrix[3][3];    // 3x3 matrix for lattice vectors
    float invmatrix[3][3]; // 3x3 inverse matrix for converting cartesian to fractional coordinates
    bool modified;         // Flag to track modifications
    
    CellInfo() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                matrix[i][j] = (i == j) ? 1.0f : 0.0f;
                invmatrix[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
        modified = false;
    }
};

// ========================================================================
// GLOBAL VARIABLES (기존 유지)
// ========================================================================
extern CellInfo cellInfo;
extern std::vector<vtkSmartPointer<vtkActor>> cellEdgeActors;

// ========================================================================
// CELL MANAGEMENT FUNCTIONS (atoms_template.cpp에서 이동)
// ========================================================================

/**
 * @brief 역행렬 계산 (기존 유지)
 */
void calculateInverseMatrix(const float matrix[3][3], float inverse[3][3]);

/**
 * @brief 카르테시안 좌표를 분수 좌표로 변환 (기존 유지)
 */
void cartesianToFractional(const float cartesian[3], float fractional[3], const float invmatrix[3][3]);

/**
 * @brief 분수 좌표를 카르테시안 좌표로 변환 (기존 유지)
 */
void fractionalToCartesian(const float fractional[3], float cartesian[3], const float matrix[3][3]);

/**
 * @brief 단위 셀 생성 (atoms_template.cpp에서 이동)
 */
void createUnitCell(const float matrix[3][3]);

/**
 * @brief 단위 셀 제거 (atoms_template.cpp에서 이동)
 */
void clearUnitCell();

/**
 * @brief 단위 셀 가시성 설정 (atoms_template.cpp에서 이동)
 */
void setUnitCellVisible(bool visible);

/**
 * @brief 셀 정보 업데이트 (atoms_template.cpp에서 이동)
 */
void updateCellInfo(const float matrix[3][3]);

// ========================================================================
// UTILITY FUNCTIONS
// ========================================================================

/**
 * @brief 현재 셀 정보 조회
 */
const CellInfo& getCellInfo();

/**
 * @brief 셀 부피 계산
 */
float calculateCellVolume();

/**
 * @brief 셀 가시성 상태 조회
 */
bool isUnitCellVisible();
```

#### Task 2.3.2: cell_manager.cpp 구현
```cpp
// atoms/domain/cell_manager.cpp
#include "cell_manager.h"
#include "../infrastructure/vtk_renderer.h"
#include "../../config/log_config.h"

// ========================================================================
// GLOBAL VARIABLES DEFINITION (기존 유지)
// ========================================================================
CellInfo cellInfo;
std::vector<vtkSmartPointer<vtkActor>> cellEdgeActors;

// ========================================================================
// CELL MANAGEMENT IMPLEMENTATION (기존 + atoms_template.cpp에서 이동)
// ========================================================================

void calculateInverseMatrix(const float matrix[3][3], float inverse[3][3]) {
    // [기존 cell_manager.cpp의 함수 내용 그대로 유지]
    
    float det = matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1])
              - matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0])
              + matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    
    if (std::abs(det) < 1e-6f) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                inverse[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
        SPDLOG_WARN("Matrix is singular or near-singular. Inverse calculation failed.");
        return;
    }
    
    float invDet = 1.0f / det;
    
    inverse[0][0] = invDet * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]);
    inverse[0][1] = invDet * (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]);
    inverse[0][2] = invDet * (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]);
    
    inverse[1][0] = invDet * (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]);
    inverse[1][1] = invDet * (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]);
    inverse[1][2] = invDet * (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]);
    
    inverse[2][0] = invDet * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    inverse[2][1] = invDet * (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]);
    inverse[2][2] = invDet * (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]);
}

void cartesianToFractional(const float cartesian[3], float fractional[3], const float invmatrix[3][3]) {
    // [기존 cell_manager.cpp의 함수 내용 그대로 유지]
    
    for (int i = 0; i < 3; i++) {
        fractional[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            fractional[i] += cartesian[j] * invmatrix[j][i];
        }
    }
}

void fractionalToCartesian(const float fractional[3], float cartesian[3], const float matrix[3][3]) {
    // [기존 cell_manager.cpp의 함수 내용 그대로 유지]
    
    for (int i = 0; i < 3; i++) {
        cartesian[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            cartesian[i] += fractional[j] * matrix[j][i];
        }
    }
}

void createUnitCell(const float matrix[3][3]) {
    // [atoms_template.cpp의 createUnitCell 함수 내용을 이동]
    
    clearUnitCell();  // 기존 셀 제거
    
    // VTKRenderer를 통해 단위 셀 생성
    // Infrastructure 레이어 활용
    SPDLOG_DEBUG("Creating unit cell with matrix");
    
    // 셀 정보 업데이트
    updateCellInfo(matrix);
}

void clearUnitCell() {
    // [atoms_template.cpp의 clearUnitCell 함수 내용을 이동]
    
    // VTKRenderer를 통해 단위 셀 제거
    // Infrastructure 레이어 활용
    cellEdgeActors.clear();
    
    SPDLOG_DEBUG("Unit cell cleared");
}

void updateCellInfo(const float matrix[3][3]) {
    // 셀 행렬 업데이트
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cellInfo.matrix[i][j] = matrix[i][j];
        }
    }
    
    // 역행렬 계산
    calculateInverseMatrix(cellInfo.matrix, cellInfo.invmatrix);
    cellInfo.modified = true;
    
    SPDLOG_DEBUG("Cell info updated");
}

// [다른 함수들 구현...]

} // namespace atoms
} // namespace domain
```

### 3.5 Phase 3: UI 레이어 분리 (2일)

#### Task 3.1: atoms_template_ui.h/cpp 생성
```cpp
// atoms/ui/atoms_template_ui.h
#pragma once

namespace atoms {
namespace ui {

/**
 * @brief UI 렌더링 함수들 (atoms_template.cpp에서 분리)
 */

/**
 * @brief 생성된 원자 섹션 렌더링 (atoms_template.cpp에서 이동)
 */
void renderCreatedAtomsSection();

/**
 * @brief 결합 관리 도구 렌더링 (atoms_template.cpp에서 이동)
 */
void renderBondsManagementTools();

/**
 * @brief 셀 설정 UI 렌더링 (atoms_template.cpp에서 이동)
 */
void renderCellSettings();

/**
 * @brief 파일 다이얼로그 렌더링 (atoms_template.cpp에서 이동)
 */
void renderFileDialog();

/**
 * @brief 설정 다이얼로그 렌더링 (atoms_template.cpp에서 이동)
 */
void renderSettingsDialog();

/**
 * @brief 메뉴바 렌더링 (atoms_template.cpp에서 이동)
 */
void renderMenuBar();

/**
 * @brief 툴바 렌더링 (atoms_template.cpp에서 이동)
 */
void renderToolbar();

/**
 * @brief 상태바 렌더링 (atoms_template.cpp에서 이동)
 */
void renderStatusBar();

} // namespace ui
} // namespace atoms
```

### 3.6 Phase 4: 통합 및 정리 (1일)

#### Task 4.1: AtomsTemplate 통합 컨트롤러
```cpp
// atoms_template.h (최종 형태)
#pragma once

#include "../macro/singleton_macro.h"
#include "infrastructure/batch_update_system.h"
#include "infrastructure/vtk_renderer.h"
#include "infrastructure/file_io_manager.h"
#include "domain/atom_manager.h"
#include "domain/bond_manager.h"
#include "domain/cell_manager.h"
#include "domain/element_database.h"

class AtomsTemplate {
    DECLARE_SINGLETON(AtomsTemplate)
    
public:
    // ========================================================================
    // DELEGATION WRAPPERS (기존 인터페이스 유지)
    // ========================================================================
    
    // Atom 관련 위임
    void createAtomSphere(const char* symbol, const ImVec4& color, float radius, 
                         const float position[3], atoms::domain::AtomType atomType) {
        atoms::domain::createAtomSphere(symbol, color, radius, position, atomType);
    }
    
    void initializeAtomGroup(const std::string& symbol, float radius) {
        atoms::domain::initializeAtomGroup(symbol, radius);
    }
    
    uint32_t generateUniqueAtomId() {
        return atoms::domain::generateUniqueAtomId();
    }
    
    // Bond 관련 위임
    void createAllBonds() {
        atoms::domain::createAllBonds();
    }
    
    void createBondsForAtoms(const std::vector<uint32_t>& atomIds) {
        atoms::domain::createBondsForAtoms(atomIds);
    }
    
    void clearAllBonds() {
        atoms::domain::clearAllBonds();
    }
    
    // Cell 관련 위임
    void createUnitCell(const float matrix[3][3]) {
        atoms::domain::createUnitCell(matrix);
    }
    
    void clearUnitCell() {
        atoms::domain::clearUnitCell();
    }
    
    // Infrastructure 위임 (기존 유지)
    void beginBatch() { m_batchSystem->beginBatch(); }
    void endBatch() { m_batchSystem->endBatch(); }
    bool isBatchMode() const { return m_batchSystem->isBatchMode(); }
    
    // ========================================================================
    // MAIN INTERFACE
    // ========================================================================
    void render(bool* openWindow = nullptr);
    void initialize();
    void shutdown();
    
    // ========================================================================
    // GLOBAL DATA ACCESS (네임스페이스 통해 접근)
    // ========================================================================
    
    // 직접 전역 변수 접근 (기존 방식 유지)
    std::vector<atoms::domain::AtomInfo>& getCreatedAtoms() {
        return atoms::domain::createdAtoms;
    }
    
    std::vector<atoms::domain::BondInfo>& getCreatedBonds() {
        return atoms::domain::createdBonds;
    }
    
    CellInfo& getCellInfo() {
        return atoms::domain::cellInfo;
    }
    
private:
    // Infrastructure 컴포넌트들
    std::unique_ptr<atoms::infrastructure::BatchUpdateSystem> m_batchSystem;
    std::unique_ptr<atoms::infrastructure::VTKRenderer> m_vtkRenderer;
    std::unique_ptr<atoms::infrastructure::FileIOManager> m_fileIOManager;
    
    // ElementDatabase는 싱글톤으로 유지 (기존 완료)
    
    void initializeInfrastructure();
};
```

## 4. 마이그레이션 전략 (네임스페이스 방식)

### 4.1 점진적 함수 이동
1. **함수별 이동**: createAtomSphere → initializeAtomGroup → ... (단계별)
2. **즉시 검증**: 각 함수 이동 후 컴파일 및 기능 테스트
3. **롤백 준비**: 문제 발생 시 이전 상태로 쉬운 복원

### 4.2 전역 변수 유지 전략
```cpp
// 네임스페이스 안에서 전역 변수 유지
namespace atoms { namespace domain {
    // 기존 전역 변수들 그대로 유지
    std::vector<AtomInfo> createdAtoms;
    std::vector<BondInfo> createdBonds;
    std::map<std::string, AtomGroupInfo> atomGroups;
}}

// atoms_template.cpp에서는 네임스페이스 통해 접근
void AtomsTemplate::someFunction() {
    atoms::domain::createdAtoms.push_back(newAtom);  // 기존과 거의 동일
}
```

### 4.3 호환성 보장 전략
```cpp
// 기존 코드에서 사용하던 방식 그대로 유지 가능
for (auto& atom : atoms::domain::createdAtoms) {
    // 기존 로직 변경 없음
    updateAtomPosition(atom);
}

// 네임스페이스 함수 호출도 간단
atoms::domain::createAtomSphere("C", color, radius, position);
```

## 5. 예상 결과 (네임스페이스 방식)

### 5.1 장점
- **빠른 구현**: 각 Phase 1-2일로 총 6-8일 완료
- **안정성**: 기존 코드 최대한 보존으로 버그 위험 최소화
- **호환성**: 기존 API 100% 유지
- **성능**: 기존과 동일한 성능 (오버헤드 없음)
- **점진적**: 함수별 단계적 이동으로 위험 분산

### 5.2 개선 효과
- **모듈화**: 논리적 책임 분리 (atoms::domain, atoms::infrastructure)
- **유지보수성**: 관련 함수들의 물리적 응집
- **확장성**: 새 함수 추가 시 적절한 네임스페이스에 배치
- **테스트**: 네임스페이스별 단위 테스트 가능

### 5.3 한계 및 향후 개선
- **전역 상태**: 전역 변수는 여전히 존재 (향후 클래스화 고려)
- **캡슐화**: 완전한 캡슐화는 달성하지 못함 (점진적 개선 필요)

## 6. 실행 체크리스트 (업데이트)

### Phase 2.1: AtomManager 함수 분리
- [ ] atom_manager.h 함수 선언 추가
- [ ] atom_manager.cpp 함수 구현 (atoms_template.cpp에서 복사)
- [ ] atoms_template.cpp 위임 래퍼 구현
- [ ] 컴파일 테스트
- [ ] 기능 테스트 (createAtomSphere 동작 확인)

### Phase 2.2: BondManager 함수 분리
- [ ] bond_manager.h 함수 선언 추가
- [ ] bond_manager.cpp 함수 구현
- [ ] atoms_template.cpp 위임 래퍼 구현
- [ ] 컴파일 테스트
- [ ] 기능 테스트 (결합 생성 동작 확인)

### Phase 2.3: CellManager 함수 분리
- [ ] cell_manager.h 함수 선언 추가
- [ ] cell_manager.cpp 함수 구현
- [ ] atoms_template.cpp 위임 래퍼 구현
- [ ] 컴파일 테스트
- [ ] 기능 테스트 (셀 생성 동작 확인)

### Phase 3: UI 레이어 분리
- [ ] atoms_template_ui.h/cpp 생성
- [ ] UI 렌더링 함수들 이동
- [ ] atoms_template.cpp UI 호출부 위임
- [ ] 컴파일 테스트
- [ ] UI 기능 테스트

### Phase 4: 통합 및 정리
- [ ] 모든 위임 래퍼 점검
- [ ] 전체 기능 통합 테스트
- [ ] 성능 테스트 (기존 대비)
- [ ] 메모리 사용량 확인
- [ ] 문서화 업데이트

## 7. 예상 일정 (네임스페이스 방식)

총 소요 기간: **6-8일**

- **Week 1**: Phase 2 (Domain 함수 분리) - 3-4일
- **Week 2**: Phase 3-4 (UI 분리 및 통합) - 3-4일

## 8. 성공 지표 (조정)

1. **구현 속도**: 6-8일 내 완료
2. **안정성**: 기존 기능 100% 동작
3. **성능**: 기존 대비 성능 저하 0%
4. **코드 품질**: 각 네임스페이스별 500줄 이하
5. **호환성**: 기존 API 100% 호환

---

**결론**: 네임스페이스 유지 방식은 **안정성과 속도**를 우선시하는 실용적 접근법입니다. 클래스 기반 설계의 장점은 포기하되, **즉시 실행 가능하고 위험이 낮은** 리팩토링을 통해 프로젝트를 안전하게 개선할 수 있습니다.