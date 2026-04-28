# AtomsTemplate 리팩토링 계획 및 상세 작업목록
## (인프라 레이어 우선 분리 전략 적용)

## 1. 프로젝트 개요

현재 `atoms_template.h/cpp` 파일은 약 3,500줄의 단일체(Monolithic) 구조입니다. 인프라 레이어를 우선 분리하여 안정적인 리팩토링을 진행합니다.

### 1.1 현재 상태 분석
- **총 코드 라인**: ~3,500 줄 (헤더 + 구현)
- **핵심 인프라**: VTK 렌더링, 배치 시스템, 파일 I/O
- **의존성**: VTK, ImGui, spdlog
- **리팩토링 전략**: 인프라 우선 분리로 기존 코드 최소 변경

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
│  - 기존 비즈니스 로직 유지          │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│         Domain Layer                │
│  - 원자/결합/셀 관리 로직          │
│  - Element 데이터                   │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│    Infrastructure Layer (우선분리)   │
│  - BatchUpdateSystem ← Phase 1      │
│  - VTKRenderer ← Phase 2           │
│  - FileIOManager ← Phase 3         │
└─────────────────────────────────────┘
```

## 2. 리팩토링 단계별 계획 (수정)

### Phase 1: BatchUpdateSystem 분리 (1일) 🎯 최우선
1. 배치 시스템 로직 추출
2. BatchGuard RAII 이동
3. 스케줄링 메커니즘 분리
4. 기존 인터페이스 래퍼 구현

### Phase 2: VTKRenderer 분리 (2일)
1. VTK 렌더링 파이프라인 추출
2. 원자/결합 그룹 렌더링 분리
3. 셀 렌더링 로직 이동
4. 기존 함수 래퍼로 변경

### Phase 3: FileIOManager 분리 (1일)
1. XSF 파싱 로직 추출
2. 파일 I/O 인터페이스 정의
3. 데이터 구조 분리

### Phase 4: 도메인 레이어 구축 (3일)
1. AtomManager 구현
2. BondManager 구현
3. CellManager 구현
4. ElementDatabase 구현

### Phase 5: UI 레이어 분리 (2일)
1. UI 렌더링 함수 추출
2. 이벤트 핸들링 분리

### Phase 6: 통합 및 정리 (1일)
1. 레거시 코드 제거
2. 통합 테스트
3. 문서화

## 3. 상세 작업 목록 (인프라 우선)

### 3.1 디렉토리 구조 생성

```bash
webassembly/src/atoms/
├── infrastructure/              # Phase 1-3: 우선 생성
│   ├── batch_update_system.h/cpp
│   ├── vtk_renderer.h/cpp
│   └── file_io_manager.h/cpp
├── core/                        # Phase 4: 이후 생성
│   ├── atoms_types.h
│   ├── atoms_constants.h
│   └── atoms_utils.h/cpp
├── domain/                      # Phase 4: 이후 생성
│   ├── atom_manager.h/cpp
│   ├── bond_manager.h/cpp
│   ├── cell_manager.h/cpp
│   └── element_database.h/cpp
├── ui/                          # Phase 5: 이후 생성
│   └── atoms_template_ui.h/cpp
└── atoms_template.h/cpp        # 메인 컨트롤러 (점진적 수정)
```

### 3.2 Phase 1: BatchUpdateSystem 분리 (1일)

#### Task 1.1: batch_update_system.h 생성
```cpp
// atoms/infrastructure/batch_update_system.h
#pragma once
#include <set>
#include <string>
#include <chrono>

class AtomsTemplate; // Forward declaration

namespace atoms {
namespace infrastructure {

class BatchUpdateSystem {
public:
    explicit BatchUpdateSystem(AtomsTemplate* parent);
    
    // 기존 AtomsTemplate의 배치 함수들 이동
    void beginBatch();
    void endBatch();
    bool isBatchMode() const { return batchMode; }
    void forceBatchEnd();
    
    // 스케줄링 함수들 이동
    void scheduleAtomGroupUpdate(const std::string& symbol);
    void scheduleBondGroupUpdate(const std::string& bondKey);
    
    // BatchGuard 이동 (기존 코드 그대로)
    class BatchGuard {
    private:
        BatchUpdateSystem* system;
        bool wasActivated;
    public:
        explicit BatchGuard(BatchUpdateSystem* sys);
        ~BatchGuard();
        BatchGuard(const BatchGuard&) = delete;
        BatchGuard& operator=(const BatchGuard&) = delete;
    };
    
private:
    AtomsTemplate* parent;
    bool batchMode;
    std::set<std::string> pendingAtomGroups;
    std::set<std::string> pendingBondGroups;
    std::chrono::high_resolution_clock::time_point batchStartTime;
    
    void updatePerformanceStats(float duration);
};

} // namespace infrastructure
} // namespace atoms
```

#### Task 1.2: AtomsTemplate 수정 (최소 변경)
```cpp
// atoms_template.h 수정
class AtomsTemplate {
private:
    std::unique_ptr<atoms::infrastructure::BatchUpdateSystem> m_batchSystem;
    
public:
    // 기존 인터페이스 유지 (단순 위임)
    void beginBatch() { m_batchSystem->beginBatch(); }
    void endBatch() { m_batchSystem->endBatch(); }
    bool isBatchMode() const { return m_batchSystem->isBatchMode(); }
    void scheduleAtomGroupUpdate(const std::string& s) { 
        m_batchSystem->scheduleAtomGroupUpdate(s); 
    }
    void scheduleBondGroupUpdate(const std::string& s) { 
        m_batchSystem->scheduleBondGroupUpdate(s); 
    }
    
    // BatchGuard 타입 별칭 (기존 코드 호환성)
    using BatchGuard = atoms::infrastructure::BatchUpdateSystem::BatchGuard;
    BatchGuard createBatchGuard() { 
        return BatchGuard(m_batchSystem.get()); 
    }
};
```

### 3.3 Phase 2: VTKRenderer 분리 (2일)

#### Task 2.1: vtk_renderer.h 생성
```cpp
// atoms/infrastructure/vtk_renderer.h
#pragma once
#include <vtkSmartPointer.h>
#include <unordered_map>
#include <vector>
#include <imgui.h>

class vtkPolyData;
class vtkActor;
class vtkAppendPolyData;
class vtkPolyDataMapper;
class vtkTransform;

namespace atoms {
namespace infrastructure {

class VTKRenderer {
public:
    // 기존 원자 그룹 VTK 함수들 이동
    void initializeUnifiedAtomGroupVTK(const std::string& symbol, float radius);
    void updateUnifiedAtomGroupVTK(const std::string& symbol);
    void clearUnifiedAtomGroupVTK();
    void clearAllUnifiedAtomGroups();
    
    // 기존 결합 그룹 VTK 함수들 이동
    void initializeBondGroup(const std::string& bondTypeKey);
    void updateBondGroup(const std::string& bondTypeKey);
    void clearBondGroup(const std::string& bondTypeKey);
    void clearAllBondGroups();
    
    // 기존 셀 렌더링 함수들 이동
    void createUnitCell(float matrix[3][3]);
    void clearUnitCell();
    
    // 데이터 접근 (AtomsTemplate에서 전달받음)
    void setAtomGroupData(const std::string& symbol, 
                          const std::vector<vtkSmartPointer<vtkTransform>>& transforms,
                          const std::vector<ImVec4>& colors);
    void setBondGroupData(const std::string& bondTypeKey,
                          const std::vector<vtkSmartPointer<vtkTransform>>& transforms1,
                          const std::vector<vtkSmartPointer<vtkTransform>>& transforms2,
                          const std::vector<ImVec4>& colors1,
                          const std::vector<ImVec4>& colors2);
    
private:
    // 기존 atomGroups 구조체 이동
    struct UnifiedAtomGroupVTK {
        vtkSmartPointer<vtkPolyData> sphereTemplate;
        vtkSmartPointer<vtkAppendPolyData> appender;
        vtkSmartPointer<vtkPolyDataMapper> mapper;
        vtkSmartPointer<vtkActor> actor;
        std::vector<vtkSmartPointer<vtkTransform>> transforms;
        std::vector<ImVec4> colors;
    };
    
    // 기존 bondGroups 구조체 이동
    struct BondGroupVTK {
        vtkSmartPointer<vtkAppendPolyData> appender;
        vtkSmartPointer<vtkPolyDataMapper> mapper;
        vtkSmartPointer<vtkActor> actor1;
        vtkSmartPointer<vtkActor> actor2;
        std::vector<vtkSmartPointer<vtkTransform>> transforms1;
        std::vector<vtkSmartPointer<vtkTransform>> transforms2;
        std::vector<ImVec4> colors1;
        std::vector<ImVec4> colors2;
    };
    
    std::unordered_map<std::string, UnifiedAtomGroupVTK> atomGroups;
    std::unordered_map<std::string, BondGroupVTK> bondGroups;
    std::vector<vtkSmartPointer<vtkActor>> cellEdgeActors;
};

} // namespace infrastructure
} // namespace atoms
```

#### Task 2.2: AtomsTemplate 수정 (VTK 함수 위임)
```cpp
// atoms_template.cpp 수정
void AtomsTemplate::updateUnifiedAtomGroupVTK(const std::string& symbol) {
    // 데이터 전달
    m_vtkRenderer->setAtomGroupData(symbol, 
                                    atomGroups[symbol].transforms,
                                    atomGroups[symbol].colors);
    // 렌더링 위임
    m_vtkRenderer->updateUnifiedAtomGroupVTK(symbol);
}

void AtomsTemplate::updateBondGroup(const std::string& bondTypeKey) {
    // 데이터 전달
    m_vtkRenderer->setBondGroupData(bondTypeKey,
                                    bondGroups[bondTypeKey].transforms1,
                                    bondGroups[bondTypeKey].transforms2,
                                    bondGroups[bondTypeKey].colors1,
                                    bondGroups[bondTypeKey].colors2);
    // 렌더링 위임
    m_vtkRenderer->updateBondGroup(bondTypeKey);
}
```

### 3.4 Phase 3: FileIOManager 분리 (1일)

#### Task 3.1: file_io_manager.h 생성
```cpp
// atoms/infrastructure/file_io_manager.h
#pragma once
#include <string>
#include <vector>

namespace atoms {
namespace infrastructure {

class FileIOManager {
public:
    // 기존 파싱 데이터 구조체 이동
    struct AtomData {
        std::string symbol;
        float position[3];
        
        AtomData(const std::string& sym, float x, float y, float z) 
            : symbol(sym) {
            position[0] = x;
            position[1] = y;
            position[2] = z;
        }
    };
    
    // 기존 ParseXSFFile 함수 이동
    bool ParseXSFFile(const std::string& filePath,
                     float cellVectors[3][3],
                     std::vector<AtomData>& atomsData);
    
    // 추후 확장용 인터페이스
    // bool ParseCIFFile(...);
    // bool ParsePOSCARFile(...);
};

} // namespace infrastructure
} // namespace atoms
```

#### Task 3.2: AtomsTemplate 수정 (파일 I/O 위임)
```cpp
// atoms_template.cpp 수정
bool AtomsTemplate::ParseXSFFile(const std::string& filePath) {
    float cellVectors[3][3];
    std::vector<atoms::infrastructure::FileIOManager::AtomData> atomsData;
    
    // 파싱 위임
    if (!m_fileIOManager->ParseXSFFile(filePath, cellVectors, atomsData)) {
        return false;
    }
    
    // 기존 InitializeAtomicStructure 호출
    InitializeAtomicStructure(cellVectors, atomsData);
    return true;
}
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