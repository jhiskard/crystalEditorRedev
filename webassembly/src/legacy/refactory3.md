# AtomsTemplate 리팩토링 계획 및 상세 작업목록

## 1. 프로젝트 개요

현재 `atoms_template.h/cpp` 파일은 약 3,000줄 이상의 코드로 구성된 단일체(Monolithic) 구조입니다. 이를 계층적 아키텍처로 분리하여 유지보수성과 확장성을 향상시키고자 합니다.

### 1.1 현재 상태 분석
- **총 코드 라인**: ~3,500 줄 (헤더 + 구현)
- **주요 책임**: 원자/결합/셀 관리, UI 렌더링, 파일 I/O, VTK 렌더링
- **의존성**: VTK, ImGui, spdlog
- **문제점**: 
  - 단일 클래스에 너무 많은 책임 집중
  - 테스트 어려움
  - 코드 재사용성 낮음
  - 새 기능 추가 시 기존 코드 영향도 높음

### 1.2 목표 아키텍처

```
┌─────────────────────────────────────┐
│           UI Layer                  │
│  - AtomsTemplateUI                  │
│  - UIComponents                     │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│        Application Layer            │
│  - AtomsTemplate (Controller)       │
│  - CommandManager                   │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│         Domain Layer                │
│  - AtomManager                      │
│  - BondManager                      │
│  - CellManager                      │
│  - ElementDatabase                  │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│      Infrastructure Layer           │
│  - VTKRenderer                      │
│  - FileIOManager                    │
│  - BatchUpdateSystem                │
└─────────────────────────────────────┘
```

## 2. 리팩토링 단계별 계획

### Phase 1: 준비 및 기반 작업 (1-2일)
1. 백업 및 버전 관리 설정
2. 테스트 환경 구축
3. 기존 기능 문서화
4. 디렉토리 구조 생성

### Phase 2: 데이터 구조 분리 (2-3일)
1. 공통 데이터 구조 추출
2. 유틸리티 함수 분리
3. 상수 및 설정값 분리

### Phase 3: 도메인 레이어 구축 (3-4일)
1. AtomManager 클래스 구현
2. BondManager 클래스 구현
3. CellManager 클래스 구현
4. ElementDatabase 클래스 구현

### Phase 4: 인프라 레이어 구축 (2-3일)
1. VTKRenderer 분리
2. FileIOManager 구현
3. BatchUpdateSystem 최적화

### Phase 5: UI 레이어 분리 (2-3일)
1. AtomsTemplateUI 클래스 생성
2. UI 컴포넌트 모듈화
3. 이벤트 핸들링 분리

### Phase 6: 통합 및 최적화 (2일)
1. 레이어 간 통합 테스트
2. 성능 최적화
3. 메모리 관리 개선
4. 문서화 완료

## 3. 상세 작업 목록

### 3.1 디렉토리 구조 생성

```bash
webassembly/src/atoms/
├── core/
│   ├── atoms_types.h           # 기본 타입 정의
│   ├── atoms_constants.h       # 상수 정의
│   └── atoms_utils.h/cpp       # 유틸리티 함수
├── domain/
│   ├── atom_manager.h/cpp      # 원자 관리
│   ├── bond_manager.h/cpp      # 결합 관리
│   ├── cell_manager.h/cpp      # 셀 관리
│   └── element_database.h/cpp  # 원소 데이터베이스
├── infrastructure/
│   ├── vtk_renderer.h/cpp      # VTK 렌더링
│   ├── file_io_manager.h/cpp   # 파일 입출력
│   └── batch_update_system.h/cpp # 배치 업데이트
├── ui/
│   ├── atoms_template_ui.h/cpp # UI 메인
│   ├── atom_table_ui.h/cpp     # 원자 테이블
│   ├── bond_ui.h/cpp            # 결합 UI
│   └── cell_ui.h/cpp           # 셀 UI
└── atoms_template.h/cpp        # 메인 컨트롤러
```

### 3.2 Phase 1: 준비 작업 상세

#### Task 1.1: 프로젝트 백업
```bash
# 백업 스크립트 생성
cp -r webassembly/src/atoms_template.* backup/
git tag pre-refactoring-v1.0
```

#### Task 1.2: CMakeLists.txt 업데이트 준비
```cmake
# 새 소스 파일 추가를 위한 변수 정의
set(ATOMS_CORE_SOURCES
    webassembly/src/atoms/core/atoms_utils.cpp
)
set(ATOMS_DOMAIN_SOURCES
    webassembly/src/atoms/domain/atom_manager.cpp
    webassembly/src/atoms/domain/bond_manager.cpp
    webassembly/src/atoms/domain/cell_manager.cpp
    webassembly/src/atoms/domain/element_database.cpp
)
# ... 추가 소스 정의
```

#### Task 1.3: 테스트 파일 생성
```cpp
// test/test_atoms_refactoring.cpp
void testAtomCreation();
void testBondCalculation();
void testFileIO();
void testBatchUpdate();
```

### 3.3 Phase 2: 데이터 구조 분리 상세

#### Task 2.1: atoms_types.h 생성
```cpp
// atoms/core/atoms_types.h
#pragma once
#include <imgui.h>
#include <string>
#include <vector>

namespace atoms {

enum class AtomType {
    ORIGINAL,
    SURROUNDING
};

enum class BondType {
    ORIGINAL,
    SURROUNDING
};

struct AtomInfo {
    uint32_t id;
    std::string symbol;
    float position[3];
    float radius;
    ImVec4 color;
    AtomType type;
    bool selected;
    bool modified;
    // ... 추가 멤버
};

struct BondInfo {
    uint32_t id;
    uint32_t atom1Id;
    uint32_t atom2Id;
    BondType type;
    float thickness;
    // ... 추가 멤버
};

struct CellInfo {
    float matrix[3][3];
    float invmatrix[3][3];
    bool modified;
    bool visible;
};

} // namespace atoms
```

#### Task 2.2: atoms_constants.h 생성
```cpp
// atoms/core/atoms_constants.h
#pragma once

namespace atoms {
namespace constants {

// 원자 데이터
extern const char* chemical_symbols[];
extern const char* atomic_names[];
extern const float atomic_masses[];
extern const float covalent_radii[];
extern const float jmol_colors[][3];

// 렌더링 설정
constexpr int DEFAULT_RESOLUTION = 20;
constexpr float DEFAULT_BOND_THICKNESS = 0.1f;
constexpr float BOND_DISTANCE_SCALE = 1.25f;

} // namespace constants
} // namespace atoms
```

#### Task 2.3: atoms_utils.h/cpp 생성
```cpp
// atoms/core/atoms_utils.h
#pragma once
#include <imgui.h>

namespace atoms {
namespace utils {

ImVec4 getContrastTextColor(const ImVec4& bgColor);
void calculateInverseMatrix(const float matrix[3][3], float inverse[3][3]);
void cartesianToFractional(const float cartesian[3], float fractional[3], 
                           const float invmatrix[3][3]);
void fractionalToCartesian(const float fractional[3], float cartesian[3], 
                           const float matrix[3][3]);
float calculateDistance(const float pos1[3], const float pos2[3]);

} // namespace utils
} // namespace atoms
```

### 3.4 Phase 3: 도메인 레이어 구축 상세

#### Task 3.1: AtomManager 클래스
```cpp
// atoms/domain/atom_manager.h
#pragma once
#include "atoms/core/atoms_types.h"
#include <memory>
#include <unordered_map>

namespace atoms {

class AtomManager {
public:
    // 원자 생성/삭제
    uint32_t createAtom(const std::string& symbol, const float position[3], 
                       float radius, const ImVec4& color, AtomType type);
    void removeAtom(uint32_t atomId);
    void clearAllAtoms();
    
    // 원자 검색
    AtomInfo* findAtom(uint32_t atomId);
    std::vector<AtomInfo*> findAtomsBySymbol(const std::string& symbol);
    std::vector<AtomInfo*> getAtomsInRadius(const float center[3], float radius);
    
    // 원자 수정
    void updateAtomPosition(uint32_t atomId, const float position[3]);
    void updateAtomSymbol(uint32_t atomId, const std::string& symbol);
    void applyModifications();
    
    // 주변 원자 관리
    void createSurroundingAtoms(const CellInfo& cellInfo);
    void hideSurroundingAtoms();
    
    // 통계
    size_t getAtomCount(AtomType type = AtomType::ORIGINAL) const;
    float calculateMemoryUsage() const;
    
private:
    std::unordered_map<uint32_t, AtomInfo> m_atoms;
    std::unordered_map<uint32_t, AtomInfo> m_surroundingAtoms;
    uint32_t m_nextAtomId = 1;
    
    uint32_t generateUniqueId();
};

} // namespace atoms
```

#### Task 3.2: BondManager 클래스
```cpp
// atoms/domain/bond_manager.h
#pragma once
#include "atoms/core/atoms_types.h"
#include <memory>
#include <unordered_map>

namespace atoms {

class AtomManager; // Forward declaration

class BondManager {
public:
    BondManager(AtomManager* atomManager);
    
    // 결합 생성/삭제
    void createAllBonds();
    void createBondsForAtoms(const std::vector<uint32_t>& atomIds);
    void clearAllBonds();
    void removeBondsForAtom(uint32_t atomId);
    
    // 결합 판단
    bool shouldCreateBond(const AtomInfo& atom1, const AtomInfo& atom2);
    
    // 결합 검색
    std::vector<BondInfo*> getBondsForAtom(uint32_t atomId);
    
    // 결합 속성
    void updateBondThickness(float thickness);
    void updateBondOpacity(float opacity);
    
    // 통계
    size_t getBondCount() const;
    float calculateMemoryUsage() const;
    
private:
    AtomManager* m_atomManager;
    std::unordered_map<uint32_t, BondInfo> m_bonds;
    uint32_t m_nextBondId = 1;
    
    // 공간 최적화를 위한 그리드
    struct SpatialGrid {
        std::unordered_map<std::string, std::vector<uint32_t>> cells;
        float cellSize;
    };
    SpatialGrid m_spatialGrid;
    
    void buildSpatialGrid();
    std::string getCellKey(const float position[3]);
};

} // namespace atoms
```

#### Task 3.3: CellManager 클래스
```cpp
// atoms/domain/cell_manager.h
#pragma once
#include "atoms/core/atoms_types.h"

namespace atoms {

class CellManager {
public:
    // 셀 생성/제거
    void createUnitCell(const float matrix[3][3]);
    void clearUnitCell();
    
    // 셀 속성
    void setCellMatrix(const float matrix[3][3]);
    const float* getCellMatrix() const;
    const float* getInverseMatrix() const;
    
    // 좌표 변환
    void cartesianToFractional(const float cartesian[3], float fractional[3]) const;
    void fractionalToCartesian(const float fractional[3], float cartesian[3]) const;
    
    // Bravais lattice 템플릿 (향후 확장)
    void applyBravaisTemplate(const std::string& templateName);
    
    // 가시성
    void setVisible(bool visible);
    bool isVisible() const;
    
private:
    CellInfo m_cellInfo;
    bool m_visible = false;
    
    void calculateInverseMatrix();
};

} // namespace atoms
```

#### Task 3.4: ElementDatabase 클래스
```cpp
// atoms/domain/element_database.h
#pragma once
#include <string>
#include <imgui.h>
#include <unordered_map>

namespace atoms {

struct Element {
    std::string symbol;
    std::string name;
    int atomicNumber;
    float mass;
    float covalentRadius;
    ImVec4 color;
    int group;
    int period;
};

class ElementDatabase {
public:
    static ElementDatabase& getInstance();
    
    // 원소 정보 조회
    const Element* getElement(const std::string& symbol) const;
    const Element* getElementByNumber(int atomicNumber) const;
    
    // 원소 속성
    ImVec4 getElementColor(const std::string& symbol) const;
    float getCovalentRadius(const std::string& symbol) const;
    float getAtomicMass(const std::string& symbol) const;
    
    // 주기율표 정보
    std::vector<Element> getElementsByPeriod(int period) const;
    std::vector<Element> getElementsByGroup(int group) const;
    
    // 사용자 정의 원소 (향후 확장)
    void addCustomElement(const Element& element);
    
private:
    ElementDatabase();
    std::unordered_map<std::string, Element> m_elements;
    std::unordered_map<int, Element*> m_elementsByNumber;
    
    void initializeElements();
};

} // namespace atoms
```

### 3.5 Phase 4: 인프라 레이어 구축 상세

#### Task 4.1: VTKRenderer 클래스
```cpp
// atoms/infrastructure/vtk_renderer.h
#pragma once
#include "atoms/core/atoms_types.h"
#include <vtkSmartPointer.h>
#include <unordered_map>

class vtkActor;
class vtkPolyData;
class vtkTransform;

namespace atoms {

class VTKRenderer {
public:
    // 원자 렌더링
    void initializeAtomGroup(const std::string& symbol, float radius);
    void updateAtomGroup(const std::string& symbol, 
                        const std::vector<vtkSmartPointer<vtkTransform>>& transforms,
                        const std::vector<ImVec4>& colors);
    void clearAtomGroups();
    
    // 결합 렌더링
    void initializeBondGroup(const std::string& bondTypeKey);
    void updateBondGroup(const std::string& bondTypeKey,
                        const std::vector<vtkSmartPointer<vtkTransform>>& transforms,
                        const std::vector<ImVec4>& colors);
    void clearBondGroups();
    
    // 셀 렌더링
    void renderUnitCell(const float matrix[3][3], bool visible);
    
    // 성능 최적화
    void beginBatchUpdate();
    void endBatchUpdate();
    
    // 통계
    size_t getRenderedAtomCount() const;
    size_t getRenderedBondCount() const;
    float getLastRenderTime() const;
    
private:
    struct AtomGroupVTK {
        vtkSmartPointer<vtkPolyData> sphereTemplate;
        vtkSmartPointer<vtkActor> actor;
        std::vector<vtkSmartPointer<vtkTransform>> transforms;
        std::vector<ImVec4> colors;
    };
    
    struct BondGroupVTK {
        vtkSmartPointer<vtkPolyData> cylinderTemplate;
        vtkSmartPointer<vtkActor> actor;
        std::vector<vtkSmartPointer<vtkTransform>> transforms;
        std::vector<ImVec4> colors;
    };
    
    std::unordered_map<std::string, AtomGroupVTK> m_atomGroups;
    std::unordered_map<std::string, BondGroupVTK> m_bondGroups;
    vtkSmartPointer<vtkActor> m_cellActor;
    
    bool m_batchMode = false;
    std::set<std::string> m_pendingAtomUpdates;
    std::set<std::string> m_pendingBondUpdates;
};

} // namespace atoms
```

#### Task 4.2: FileIOManager 클래스
```cpp
// atoms/infrastructure/file_io_manager.h
#pragma once
#include "atoms/core/atoms_types.h"
#include <string>
#include <vector>

namespace atoms {

class AtomManager;
class CellManager;

class FileIOManager {
public:
    FileIOManager(AtomManager* atomManager, CellManager* cellManager);
    
    // XSF 파일 지원
    bool loadXSFFile(const std::string& filePath);
    bool saveXSFFile(const std::string& filePath);
    
    // 향후 확장 예정 포맷
    bool loadCIFFile(const std::string& filePath);
    bool loadPOSCARFile(const std::string& filePath);
    bool loadXYZFile(const std::string& filePath);
    
    // 파일 유효성 검사
    bool validateFile(const std::string& filePath);
    std::string getFileFormat(const std::string& filePath);
    
    // 에러 처리
    std::string getLastError() const;
    
private:
    AtomManager* m_atomManager;
    CellManager* m_cellManager;
    std::string m_lastError;
    
    // XSF 파싱
    struct AtomData {
        std::string symbol;
        float position[3];
    };
    
    bool parseXSFFile(const std::string& filePath, 
                     float cellVectors[3][3],
                     std::vector<AtomData>& atoms);
};

} // namespace atoms
```

#### Task 4.3: BatchUpdateSystem 클래스
```cpp
// atoms/infrastructure/batch_update_system.h
#pragma once
#include <functional>
#include <queue>
#include <set>

namespace atoms {

class BatchUpdateSystem {
public:
    static BatchUpdateSystem& getInstance();
    
    // 배치 모드 제어
    void beginBatch();
    void endBatch();
    bool isBatchMode() const;
    
    // 업데이트 스케줄링
    void scheduleAtomGroupUpdate(const std::string& symbol);
    void scheduleBondGroupUpdate(const std::string& bondTypeKey);
    void scheduleRender();
    
    // 배치 가드 (RAII)
    class BatchGuard {
    public:
        explicit BatchGuard(BatchUpdateSystem* system);
        ~BatchGuard();
        BatchGuard(const BatchGuard&) = delete;
        BatchGuard& operator=(const BatchGuard&) = delete;
    private:
        BatchUpdateSystem* m_system;
    };
    
    // 통계
    size_t getPendingUpdateCount() const;
    float getLastBatchDuration() const;
    
private:
    BatchUpdateSystem() = default;
    
    bool m_batchMode = false;
    std::set<std::string> m_pendingAtomGroups;
    std::set<std::string> m_pendingBondGroups;
    bool m_renderScheduled = false;
    
    std::chrono::high_resolution_clock::time_point m_batchStartTime;
    float m_lastBatchDuration = 0.0f;
    
    void processPendingUpdates();
};

} // namespace atoms
```

### 3.6 Phase 5: UI 레이어 분리 상세

#### Task 5.1: AtomsTemplateUI 메인 클래스
```cpp
// atoms/ui/atoms_template_ui.h
#pragma once
#include <memory>

namespace atoms {

class AtomManager;
class BondManager;
class CellManager;
class AtomTableUI;
class BondUI;
class CellUI;

class AtomsTemplateUI {
public:
    AtomsTemplateUI(AtomManager* atomManager, 
                   BondManager* bondManager,
                   CellManager* cellManager);
    
    // 메인 렌더링
    void render(bool* openWindow = nullptr);
    
    // UI 섹션
    void renderMenuBar();
    void renderToolbar();
    void renderMainContent();
    void renderStatusBar();
    
    // 모달 다이얼로그
    void renderFileDialog();
    void renderSettingsDialog();
    void renderAboutDialog();
    
private:
    AtomManager* m_atomManager;
    BondManager* m_bondManager;
    CellManager* m_cellManager;
    
    std::unique_ptr<AtomTableUI> m_atomTableUI;
    std::unique_ptr<BondUI> m_bondUI;
    std::unique_ptr<CellUI> m_cellUI;
    
    // UI 상태
    bool m_showFileDialog = false;
    bool m_showSettings = false;
    bool m_showAbout = false;
    bool m_editMode = false;
    bool m_useFractionalCoords = false;
};

} // namespace atoms
```

#### Task 5.2: AtomTableUI 클래스
```cpp
// atoms/ui/atom_table_ui.h
#pragma once
#include "atoms/core/atoms_types.h"
#include <vector>

namespace atoms {

class AtomManager;

class AtomTableUI {
public:
    explicit AtomTableUI(AtomManager* atomManager);
    
    // 테이블 렌더링
    void render(bool editMode, bool useFractionalCoords);
    
    // 선택 관리
    void selectAll();
    void deselectAll();
    std::vector<uint32_t> getSelectedAtoms() const;
    
    // 편집 관리
    bool hasModifications() const;
    void applyChanges();
    void discardChanges();
    
private:
    AtomManager* m_atomManager;
    
    // UI 상태
    std::vector<uint32_t> m_selectedAtoms;
    bool m_sortAscending = true;
    int m_sortColumn = 0;
    
    // 헬퍼 함수
    void renderTableHeader();
    void renderAtomRow(AtomInfo& atom, int index, bool editMode);
    void renderSelectionSummary();
};

} // namespace atoms
```

### 3.7 Phase 6: 통합 작업 상세

#### Task 6.1: 새로운 AtomsTemplate 컨트롤러
```cpp
// atoms/atoms_template.h (리팩토링 후)
#pragma once
#include "macro/singleton_macro.h"
#include <memory>
#include <string>

namespace atoms {

class AtomManager;
class BondManager;
class CellManager;
class ElementDatabase;
class VTKRenderer;
class FileIOManager;
class BatchUpdateSystem;
class AtomsTemplateUI;

class AtomsTemplate {
    DECLARE_SINGLETON(AtomsTemplate)
    
public:
    // 초기화/정리
    void initialize();
    void shutdown();
    
    // 메인 인터페이스
    void render(bool* openWindow = nullptr);
    void loadFile(const std::string& filePath);
    void saveFile(const std::string& filePath);
    
    // 매니저 접근자 (필요시)
    AtomManager* getAtomManager() { return m_atomManager.get(); }
    BondManager* getBondManager() { return m_bondManager.get(); }
    CellManager* getCellManager() { return m_cellManager.get(); }
    
private:
    // 도메인 레이어
    std::unique_ptr<AtomManager> m_atomManager;
    std::unique_ptr<BondManager> m_bondManager;
    std::unique_ptr<CellManager> m_cellManager;
    
    // 인프라 레이어
    std::unique_ptr<VTKRenderer> m_vtkRenderer;
    std::unique_ptr<FileIOManager> m_fileIOManager;
    
    // UI 레이어
    std::unique_ptr<AtomsTemplateUI> m_ui;
    
    // 초기화 헬퍼
    void initializeManagers();
    void initializeUI();
};

} // namespace atoms
```

## 4. 마이그레이션 전략

### 4.1 점진적 마이그레이션
1. 새 클래스를 생성하되, 기존 코드는 유지
2. 단위 테스트 작성 및 검증
3. 기능별로 점진적 이동
4. 이동 완료 후 기존 코드 제거

### 4.2 호환성 유지
```cpp
// 임시 호환성 레이어
namespace {
    // 기존 전역 함수를 새 시스템으로 리다이렉트
    void createAtomSphere(...) {
        atoms::AtomsTemplate::Instance().getAtomManager()->createAtom(...);
    }
}
```

### 4.3 테스트 계획
- 단위 테스트: 각 매니저 클래스별
- 통합 테스트: 레이어 간 상호작용
- 성능 테스트: 리팩토링 전후 비교
- UI 테스트: 사용자 인터페이스 동작 확인

## 5. 예상 결과

### 5.1 장점
- **유지보수성**: 각 클래스가 단일 책임 원칙 준수
- **테스트 용이성**: 모듈별 독립적 테스트 가능
- **확장성**: 새 파일 포맷, Bravais lattice 템플릿 쉽게 추가
- **재사용성**: 각 컴포넌트 독립적 사용 가능
- **성능**: 배치 시스템 최적화로 렌더링 성능 향상

### 5.2 위험 요소 및 대응
- **리스크**: 기존 기능 손상
  - **대응**: 점진적 마이그레이션, 충분한 테스트
- **리스크**: 성능 저하
  - **대응**: 프로파일링, 최적화
- **리스크**: 빌드 시스템 복잡도 증가
  - **대응**: CMake 모듈화

## 6. 실행 체크리스트

### Phase 1 체크리스트
- [ ] 기존 코드 백업 완료
- [ ] Git 브랜치 생성 (`refactoring/atoms-template`)
- [ ] 디렉토리 구조 생성
- [ ] CMakeLists.txt 준비
- [ ] 기본 테스트 환경 구축

### Phase 2 체크리스트
- [ ] atoms_types.h 생성 및 테스트
- [ ] atoms_constants.h 생성 및 데이터 이동
- [ ] atoms_utils.h/cpp 생성 및 함수 이동
- [ ] 컴파일 확인

### Phase 3 체크리스트
- [ ] AtomManager 클래스 구현
- [ ] BondManager 클래스 구현
- [ ] CellManager 클래스 구현
- [ ] ElementDatabase 클래스 구현
- [ ] 단위 테스트 작성

### Phase 4 체크리스트
- [ ] VTKRenderer 분리
- [ ] FileIOManager 구현
- [ ] BatchUpdateSystem 최적화
- [ ] 성능 테스트

### Phase 5 체크리스트
- [ ] AtomsTemplateUI 생성
- [ ] UI 컴포넌트 분리
- [ ] 이벤트 핸들링 구현
- [ ] UI 테스트

### Phase 6 체크리스트
- [ ] 모든 컴포넌트 통합
- [ ] 기존 코드 제거
- [ ] 전체 테스트 실행
- [ ] 문서화 완료
- [ ] 코드 리뷰

## 7. 예상 일정

총 소요 기간: **12-16일**

- Week 1: Phase 1-2 (준비 및 데이터 구조)
- Week 2: Phase 3-4 (도메인 및 인프라)
- Week 3: Phase 5-6 (UI 및 통합)

## 8. 성공 지표

1. **코드 라인 수**: 각 파일 500줄 이하
2. **테스트 커버리지**: 80% 이상
3. **빌드 시간**: 기존 대비 동등 이하
4. **런타임 성능**: 기존 대비 10% 이상 개선
5. **메모리 사용량**: 기존 대비 20% 감소

## 9. 다음 단계

이 계획이 승인되면:
1. Phase 1부터 순차적으로 실행
2. 각 Phase 완료 시 코드 리뷰
3. 문제 발생 시 계획 조정
4. 완료 후 팀 전체 교육

---

**Note**: 이 문서는 living document로, 진행 상황에 따라 지속적으로 업데이트됩니다.