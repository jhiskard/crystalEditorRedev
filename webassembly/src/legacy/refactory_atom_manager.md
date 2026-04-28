# AtomManager 클래스 분리 대상 상세 목록

## 1. 데이터 구조 분리 대상

### 1.1 핵심 데이터 구조체 (🔄 수정 - 기존 구조 유지)
**파일**: `atoms_template.cpp`에서 `atoms/core/atoms_types.h`로 이동

```cpp
// 🔄 수정: AtomInfo 구조체 (기존 형태 완전 유지)
struct AtomInfo {
    std::string symbol;           // 원소 기호
    ImVec4 color;                // 색상
    float radius;                // 반지름
    float position[3];           // 카르테시안 좌표
    float fracPosition[3];       // 분수 좌표
    
    // 편집 관련
    std::string tempSymbol;      // 임시 원소 기호
    float tempPosition[3];       // 임시 카르테시안 좌표  
    float tempFracPosition[3];   // 임시 분수 좌표
    float tempRadius;            // 임시 반지름
    bool modified;               // 수정 여부
    bool selected;               // 선택 여부
    
    // 통합 렌더링 시스템
    uint32_t id;                 // 고유 ID
    AtomType atomType;           // ORIGINAL/SURROUNDING
    bool isInstanced;            // 항상 true (통합 시스템)
    size_t instanceIndex;        // 그룹 내 인스턴스 인덱스
    uint32_t originalAtomId;     // SURROUNDING 원자의 원본 ID
    
    // 🔄 수정: 기존 생성자들 완전 유지
    AtomInfo() = default;
    AtomInfo(const std::string& sym, const ImVec4& col, float rad, 
             const float pos[3], AtomType type = AtomType::ORIGINAL);
};
```

### 1.2 원자 그룹 데이터 구조체 (🔄 수정 - 기존 구조 유지)
**파일**: `atoms_template.cpp`에서 `atoms/core/atoms_types.h`로 이동

```cpp
// 🔄 수정: AtomGroupInfo 구조체 (기존 형태 완전 유지)
struct AtomGroupInfo {
    std::string element;                                   // 원소 기호
    float baseRadius;                                     // 기본 반지름
    
    // 통합 그룹 데이터 (원본 + 주변 통합)
    std::vector<vtkSmartPointer<vtkTransform>> transforms; // 위치/크기 변환
    std::vector<ImVec4> colors;                           // 색상 (타입별 조정됨)
    std::vector<uint32_t> atomIds;                        // 고유 ID 목록
    std::vector<AtomType> atomTypes;                      // 원본/주변 타입
    std::vector<size_t> originalIndices;                  // createdAtoms/surroundingAtoms 인덱스
    
    // VTK 렌더링 파이프라인 (레거시 대체)
    vtkSmartPointer<vtkPolyData> baseGeometry;            // 기본 구 기하학적 데이터
    vtkSmartPointer<vtkAppendPolyData> appender;          // 인스턴스 결합기
    vtkSmartPointer<vtkPolyDataMapper> mapper;            // VTK 매퍼
    vtkSmartPointer<vtkActor> actor;                      // VTK 액터
    
    // 🔄 수정: 기존 생성자들 완전 유지
    AtomGroupInfo();
    AtomGroupInfo(const std::string& elem, float radius);
};
```

### 1.3 전역 데이터 벡터 (🔄 수정 - AtomManager 멤버로 이동)

```cpp
// ❌ 삭제: atoms_template.cpp의 전역 변수들
static std::vector<AtomInfo> createdAtoms;        // → m_createdAtoms
static std::vector<AtomInfo> surroundingAtoms;    // → m_surroundingAtoms
static std::unordered_map<std::string, AtomGroupInfo> atomGroups; // → m_atomGroups
static uint32_t nextAtomId = 1;                   // → m_nextAtomId
```

## 2. 함수 분리 대상

### 2.1 원자 생성 함수들 (🔄 수정 - 로직 완전 유지)

#### 2.1.1 핵심 원자 생성 함수
**원본**: `atoms_template.cpp::addAtom(...)`
**대상**: `AtomManager::addAtom(...)`

```cpp
// 🔄 수정: 기존 로직 완전 보존하여 이관
uint32_t AtomManager::addAtom(const std::string& symbol, float x, float y, float z, 
                              AtomType type = AtomType::ORIGINAL) {
    // ✅ 유지: 기존 로직 완전 동일
    // - generateUniqueAtomId() 호출
    // - ElementDatabase에서 색상/반지름 조회 
    // - AtomInfo 생성 및 초기화
    // - addAtomToGroup() 호출
    // - createdAtoms/surroundingAtoms에 추가
    // - scheduleAtomGroupUpdate() 호출
}
```

#### 2.1.2 그룹 추가 함수
**원본**: `atoms_template.cpp::addAtomToGroup(...)`
**대상**: `AtomManager::addAtomToGroup(...)`

```cpp
// 🔄 수정: 기존 로직 완전 보존하여 이관
void AtomManager::addAtomToGroup(const std::string& symbol, 
                                vtkSmartPointer<vtkTransform> transform,
                                const ImVec4& color, uint32_t atomId, 
                                AtomType atomType, size_t originalIndex) {
    // ✅ 유지: 기존 로직 완전 동일
    // - atomGroups 접근 및 데이터 추가
    // - transforms, colors, atomIds, atomTypes, originalIndices 업데이트
}
```

### 2.2 원자 삭제 함수들 (🔄 수정 - 로직 완전 유지)

#### 2.2.1 그룹에서 원자 제거
**원본**: `atoms_template.cpp::removeAtomFromGroup(...)`  
**대상**: `AtomManager::removeAtomFromGroup(...)`

```cpp
// 🔄 수정: 기존 로직 완전 보존하여 이관
void AtomManager::removeAtomFromGroup(const std::string& symbol, uint32_t atomId) {
    // ✅ 유지: 기존 로직 완전 동일
    // - atomGroups에서 atomId 찾기
    // - 해당 인덱스의 모든 데이터 제거
    // - scheduleAtomGroupUpdate() 호출
}
```

#### 2.2.2 전체 원자 삭제
**원본**: `atoms_template.cpp::clearAllAtoms()`
**대상**: `AtomManager::clearAllAtoms()`

```cpp
// 🔄 수정: 기존 로직 완전 보존하여 이관  
void AtomManager::clearAllAtoms() {
    // ✅ 유지: 기존 로직 완전 동일
    // - BatchGuard 생성
    // - createdAtoms.clear()
    // - surroundingAtoms.clear() 
    // - atomGroups.clear()
    // - VTK 그룹 정리 호출
}
```

### 2.3 원자 그룹 관리 함수들 (🔄 수정 - 로직 완전 유지)

#### 2.3.1 그룹 초기화
**원본**: `atoms_template.cpp::initializeAtomGroup(...)`
**대상**: `AtomManager::initializeAtomGroup(...)`

```cpp
// 🔄 수정: 기존 로직 완전 보존하여 이관
void AtomManager::initializeAtomGroup(const std::string& symbol, float radius) {
    // ✅ 유지: 기존 로직 완전 동일
    // - 중복 체크
    // - AtomGroupInfo 생성
    // - VTK 파이프라인 초기화 호출
}
```

### 2.4 주변 원자 관리 함수들 (🔄 수정 - 로직 완전 유지)

#### 2.4.1 주변 원자 생성
**원본**: `atoms_template.cpp::createSurroundingAtoms(...)`
**대상**: `AtomManager::createSurroundingAtoms(...)`

#### 2.4.2 주변 원자 숨김/표시
**원본**: `atoms_template.cpp::hideSurroundingAtoms()`
**대상**: `AtomManager::hideSurroundingAtoms()`

### 2.5 원자 편집 관련 함수들 (🔄 수정 - 로직 완전 유지)

#### 2.5.1 원자 변경사항 적용
**원본**: `atoms_template.cpp::applyAtomChanges()`
**대상**: `AtomManager::applyAtomChanges()`

```cpp
// 🔄 수정: 기존 로직 완전 보존하여 이관
void AtomManager::applyAtomChanges() {
    // ✅ 유지: 기존 로직 완전 동일
    // - BatchGuard 생성
    // - modified 플래그 체크
    // - 원자 속성 업데이트 
    // - 결합 재생성 (BondManager 연동)
    // - VTK 업데이트
}
```

### 2.6 유틸리티 함수들 (🔄 수정 - 로직 완전 유지)

#### 2.6.1 ID 생성
**원본**: `atoms_template.cpp::generateUniqueAtomId()`
**대상**: `AtomManager::generateUniqueAtomId()`

## 3. AtomManager 클래스 완전 설계

### 3.1 헤더 파일 설계
**파일**: `atoms/domain/atom_manager.h`

```cpp
#pragma once
#include "atoms/core/atoms_types.h"
#include "atoms/domain/element_database.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace atoms {
namespace infrastructure {
    class VTKRenderer;      // Forward declaration
    class BatchUpdateSystem; // Forward declaration
}

namespace domain {

/**
 * @brief 원자 관리자 클래스 - 모든 원자 관련 비즈니스 로직 담당
 * 
 * 기존 atoms_template.cpp의 원자 관련 로직을 완전히 이관하여 관리합니다.
 * 원본 로직의 형태와 입출력을 완전히 보존합니다.
 */
class AtomManager {
public:
    explicit AtomManager(infrastructure::VTKRenderer* vtkRenderer,
                        infrastructure::BatchUpdateSystem* batchSystem);
    ~AtomManager();
    
    // ========================================================================
    // 원자 생성/삭제 (기존 atoms_template.cpp 함수들 완전 이관)
    // ========================================================================
    
    /**
     * @brief 원자 추가 (기존 addAtom 함수 완전 이관)
     * @return 생성된 원자의 고유 ID
     */
    uint32_t addAtom(const std::string& symbol, float x, float y, float z, 
                     AtomType type = AtomType::ORIGINAL);
    
    /**
     * @brief 모든 원자 삭제 (기존 clearAllAtoms 함수 완전 이관)
     */
    void clearAllAtoms();
    
    // ========================================================================
    // 원자 그룹 관리 (기존 atoms_template.cpp 함수들 완전 이관)
    // ========================================================================
    
    /**
     * @brief 원자 그룹에 추가 (기존 addAtomToGroup 함수 완전 이관)
     */
    void addAtomToGroup(const std::string& symbol, 
                       vtkSmartPointer<vtkTransform> transform,
                       const ImVec4& color, uint32_t atomId, 
                       AtomType atomType, size_t originalIndex);
    
    /**
     * @brief 원자 그룹에서 제거 (기존 removeAtomFromGroup 함수 완전 이관)
     */
    void removeAtomFromGroup(const std::string& symbol, uint32_t atomId);
    
    /**
     * @brief 원자 그룹 초기화 (기존 initializeAtomGroup 함수 완전 이관)
     */
    void initializeAtomGroup(const std::string& symbol, float radius);
    
    // ========================================================================
    // 주변 원자 관리 (기존 atoms_template.cpp 함수들 완전 이관)
    // ========================================================================
    
    /**
     * @brief 주변 원자 생성 (기존 createSurroundingAtoms 함수 완전 이관)
     */
    void createSurroundingAtoms(const CellInfo& cellInfo, float maxDistance);
    
    /**
     * @brief 주변 원자 숨김 (기존 hideSurroundingAtoms 함수 완전 이관)
     */
    void hideSurroundingAtoms();
    
    /**
     * @brief 주변 원자 가시성 설정 (기존 setSurroundingsVisible 함수 완전 이관)
     */
    void setSurroundingsVisible(bool visible);
    bool getSurroundingsVisible() const { return m_surroundingsVisible; }
    
    // ========================================================================
    // 원자 편집 관리 (기존 atoms_template.cpp 함수들 완전 이관)  
    // ========================================================================
    
    /**
     * @brief 원자 변경사항 적용 (기존 applyAtomChanges 함수 완전 이관)
     */
    void applyAtomChanges();
    
    // ========================================================================
    // 데이터 접근자 (기존 전역 변수들에 대한 접근 제공)
    // ========================================================================
    
    /**
     * @brief 생성된 원자 목록 접근 (기존 createdAtoms 전역변수 대체)
     */
    std::vector<AtomInfo>& getCreatedAtoms() { return m_createdAtoms; }
    const std::vector<AtomInfo>& getCreatedAtoms() const { return m_createdAtoms; }
    
    /**
     * @brief 주변 원자 목록 접근 (기존 surroundingAtoms 전역변수 대체)
     */
    std::vector<AtomInfo>& getSurroundingAtoms() { return m_surroundingAtoms; }
    const std::vector<AtomInfo>& getSurroundingAtoms() const { return m_surroundingAtoms; }
    
    /**
     * @brief 원자 그룹 맵 접근 (기존 atomGroups 전역변수 대체)  
     */
    std::unordered_map<std::string, AtomGroupInfo>& getAtomGroups() { return m_atomGroups; }
    const std::unordered_map<std::string, AtomGroupInfo>& getAtomGroups() const { return m_atomGroups; }
    
    /**
     * @brief 원자 통계
     */
    size_t getCreatedAtomCount() const { return m_createdAtoms.size(); }
    size_t getSurroundingAtomCount() const { return m_surroundingAtoms.size(); }
    size_t getTotalAtomCount() const { return m_createdAtoms.size() + m_surroundingAtoms.size(); }
    
private:
    // ========================================================================
    // 데이터 멤버 (기존 전역 변수들 이관)
    // ========================================================================
    std::vector<AtomInfo> m_createdAtoms;                              // 기존 createdAtoms
    std::vector<AtomInfo> m_surroundingAtoms;                          // 기존 surroundingAtoms
    std::unordered_map<std::string, AtomGroupInfo> m_atomGroups;       // 기존 atomGroups
    uint32_t m_nextAtomId;                                            // 기존 nextAtomId
    bool m_surroundingsVisible;                                       // 기존 surroundingsVisible
    
    // ========================================================================
    // 외부 의존성 (인프라 레이어 참조)
    // ========================================================================
    infrastructure::VTKRenderer* m_vtkRenderer;           // VTK 렌더링 위임
    infrastructure::BatchUpdateSystem* m_batchSystem;     // 배치 업데이트 위임
    ElementDatabase* m_elementDB;                         // 원소 정보 조회
    
    // ========================================================================
    // 내부 유틸리티 함수들 (기존 atoms_template.cpp 함수들 완전 이관)
    // ========================================================================
    
    /**
     * @brief 고유 ID 생성 (기존 generateUniqueAtomId 함수 완전 이관)
     */
    uint32_t generateUniqueAtomId();
};

} // namespace domain  
} // namespace atoms
```

## 4. 전역 함수 래퍼 유지

### 4.1 기존 전역 함수 호환성 유지 (🔄 수정 - 완전 유지)
**파일**: `atoms_template.cpp` (AtomManager 연결 후에도 유지)

```cpp
// ✅ 유지: 기존 전역 함수들을 AtomManager 위임으로 변경
void addAtomToGroup(const std::string& symbol, 
                   vtkSmartPointer<vtkTransform> transform,
                   const ImVec4& color, uint32_t atomId, 
                   AtomType atomType, size_t originalIndex) {
    // 🔄 수정: AtomManager 위임
    AtomsTemplate::Instance().getAtomManager()->addAtomToGroup(
        symbol, transform, color, atomId, atomType, originalIndex);
}

void removeAtomFromGroup(const std::string& symbol, uint32_t atomId) {
    // 🔄 수정: AtomManager 위임  
    AtomsTemplate::Instance().getAtomManager()->removeAtomFromGroup(symbol, atomId);
}

void initializeAtomGroup(const std::string& symbol, float radius) {
    // 🔄 수정: AtomManager 위임
    AtomsTemplate::Instance().getAtomManager()->initializeAtomGroup(symbol, radius);
}
```

## 5. AtomsTemplate 클래스 수정 사항

### 5.1 멤버 변수 추가 (🆕 추가)
**파일**: `atoms_template.h`

```cpp
class AtomsTemplate {
private:
    // 🆕 추가: Domain 레이어 멤버
    std::unique_ptr<atoms::domain::AtomManager> m_atomManager;
    
public:
    // 🆕 추가: AtomManager 접근자
    atoms::domain::AtomManager* getAtomManager() { return m_atomManager.get(); }
    const atoms::domain::AtomManager* getAtomManager() const { return m_atomManager.get(); }
};
```

### 5.2 생성자 수정 (🔄 수정)
**파일**: `atoms_template.cpp`

```cpp
// 🔄 수정: 생성자에 AtomManager 초기화 추가
AtomsTemplate::AtomsTemplate() {
    // 기존 Infrastructure 초기화 
    m_batchSystem = std::make_unique<atoms::infrastructure::BatchUpdateSystem>(this);
    m_vtkRenderer = std::make_unique<atoms::infrastructure::VTKRenderer>();
    m_fileIOManager = std::make_unique<atoms::infrastructure::FileIOManager>(this);
    m_elementDB = &atoms::domain::ElementDatabase::getInstance();
    
    // 🆕 추가: AtomManager 초기화
    m_atomManager = std::make_unique<atoms::domain::AtomManager>(
        m_vtkRenderer.get(), m_batchSystem.get());
    
    // ✅ 유지: 기존 초기화 로직
    // ... 나머지 기존 생성자 코드 ...
}
```

### 5.3 기존 원자 관련 함수들 위임 (🔄 수정)
**파일**: `atoms_template.cpp`

```cpp
// 🔄 수정: 기존 AtomsTemplate 원자 함수들을 AtomManager 위임
uint32_t AtomsTemplate::addAtom(const std::string& symbol, float x, float y, float z) {
    return m_atomManager->addAtom(symbol, x, y, z, AtomType::ORIGINAL);
}

void AtomsTemplate::clearAllAtoms() {
    m_atomManager->clearAllAtoms();
}

void AtomsTemplate::createSurroundingAtoms(const CellInfo& cellInfo, float maxDistance) {
    m_atomManager->createSurroundingAtoms(cellInfo, maxDistance);
}

void AtomsTemplate::applyAtomChanges() {
    m_atomManager->applyAtomChanges();
}

// ✅ 유지: UI에서 사용하는 데이터 접근 함수들 (단순 위임)
std::vector<AtomInfo>& AtomsTemplate::getCreatedAtoms() {
    return m_atomManager->getCreatedAtoms();
}

const std::vector<AtomInfo>& AtomsTemplate::getCreatedAtoms() const {
    return m_atomManager->getCreatedAtoms();
}
```

## 6. 주의사항 및 마이그레이션 전략

### 6.1 데이터 호환성 유지
- **createdAtoms 전역변수** → `AtomManager::m_createdAtoms` 멤버로 이동하되, 기존 접근 방식 유지
- **atomGroups 전역변수** → `AtomManager::m_atomGroups` 멤버로 이동하되, 기존 접근 방식 유지
- 기존 UI 코드에서 `createdAtoms` 접근은 `getInstance().getAtomManager()->getCreatedAtoms()` 로 변경

### 6.2 함수 시그니처 완전 유지
- 모든 기존 함수의 **매개변수, 반환값, 동작 로직** 완전 동일하게 유지
- 내부 구현만 AtomManager로 위임하는 방식
- 기존 호출 코드는 수정 불필요

### 6.3 VTK 연동 유지
- AtomManager는 VTKRenderer에 **의존성 주입**을 통해 연결
- 기존 VTK 관련 함수 호출 패턴 완전 유지
- `initializeUnifiedAtomGroupVTK()`, `updateUnifiedAtomGroupVTK()` 등의 호출 패턴 보존

## 7. 구현 순서

### 7.1 1단계: 데이터 구조 이동 
1. `AtomInfo` 구조체를 `atoms/core/atoms_types.h`로 이동
2. `AtomGroupInfo` 구조체를 `atoms/core/atoms_types.h`로 이동  
3. `AtomType` 열거형 정의 확인

### 7.2 2단계: AtomManager 클래스 생성
1. `atoms/domain/atom_manager.h` 헤더 생성
2. `atoms/domain/atom_manager.cpp` 구현 생성
3. 기존 atoms_template.cpp 함수들을 완전히 복사하여 이관

### 7.3 3단계: AtomsTemplate 연결
1. AtomsTemplate에 AtomManager 멤버 추가
2. 생성자에서 AtomManager 초기화
3. 기존 함수들을 AtomManager 위임으로 변경

### 7.4 4단계: 전역 변수 제거
1. `createdAtoms`, `surroundingAtoms`, `atomGroups` 전역변수 삭제  
2. 기존 접근 코드를 AtomManager 접근자로 변경
3. 컴파일 에러 해결 및 테스트

## 8. 예상되는 주요 변경 파일들

### 8.1 신규 생성 파일
- `atoms/domain/atom_manager.h` - AtomManager 클래스 헤더
- `atoms/domain/atom_manager.cpp` - AtomManager 클래스 구현

### 8.2 수정 대상 파일
- `atoms_template.h` - AtomManager 멤버 추가, 접근자 추가
- `atoms_template.cpp` - 원자 관련 함수들을 AtomManager 위임으로 변경
- 기존 UI 관련 함수들에서 `createdAtoms` 접근 방식 변경

### 8.3 데이터 마이그레이션 맵

| 기존 위치 | 이동 후 위치 | 변경 타입 |
|-----------|--------------|-----------|
| `static std::vector<AtomInfo> createdAtoms` | `AtomManager::m_createdAtoms` | 🔄 이동 |
| `static std::vector<AtomInfo> surroundingAtoms` | `AtomManager::m_surroundingAtoms` | 🔄 이동 |
| `static std::unordered_map<std::string, AtomGroupInfo> atomGroups` | `AtomManager::m_atomGroups` | 🔄 이동 |
| `static uint32_t nextAtomId` | `AtomManager::m_nextAtomId` | 🔄 이동 |
| `void addAtom(...)` | `AtomManager::addAtom(...)` | 🔄 이동 |
| `void addAtomToGroup(...)` | `AtomManager::addAtomToGroup(...)` | 🔄 이동 |
| `void removeAtomFromGroup(...)` | `AtomManager::removeAtomFromGroup(...)` | 🔄 이동 |
| `void clearAllAtoms()` | `AtomManager::clearAllAtoms()` | 🔄 이동 |
| `void initializeAtomGroup(...)` | `AtomManager::initializeAtomGroup(...)` | 🔄 이동 |
| `void createSurroundingAtoms(...)` | `AtomManager::createSurroundingAtoms(...)` | 🔄 이동 |
| `void applyAtomChanges()` | `AtomManager::applyAtomChanges()` | 🔄 이동 |
| `uint32_t generateUniqueAtomId()` | `AtomManager::generateUniqueAtomId()` | 🔄 이동 |

이 목록은 **원본 로직과 형태를 완전히 보존**하면서 AtomManager 클래스로 체계적으로 분리할 수 있는 구체적인 로드맵을 제공합니다.