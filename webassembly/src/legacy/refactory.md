# AtomsTemplate 리팩토링 계획서

## 1. 현재 상태 분석

### 주요 문제점
- **거대한 단일 클래스**: AtomsTemplate이 모든 기능을 담당 (3000+ 라인)
- **높은 결합도**: UI, 데이터 구조, 렌더링 로직이 모두 혼재
- **전역 변수 남용**: static 변수들이 캡슐화되지 않음
- **복잡한 의존성**: VTK, ImGui, 데이터 구조가 긴밀하게 연결
- **유지보수 어려움**: 새로운 기능 추가 시 기존 코드에 미치는 영향 예측 어려움

### 현재 클래스 책임
```cpp
class AtomsTemplate {
    // UI 렌더링 (ImGui)
    // 원자 생성/관리
    // 결합 생성/관리
    // 격자 구조 관리
    // 파일 I/O (XSF)
    // VTK 렌더링 관리
    // 배치 업데이트 시스템
    // 메모리 관리
    // 성능 통계
};
```

## 2. 리팩토링 목표

### 아키텍처 원칙
- **단일 책임 원칙**: 각 클래스는 하나의 명확한 역할
- **의존성 역전**: 인터페이스를 통한 느슨한 결합
- **확장성**: 새로운 파일 형식, 렌더링 백엔드 추가 용이
- **테스트 가능성**: 각 컴포넌트 독립적 테스트 가능
- **재사용성**: 컴포넌트 단위 재사용 가능

## 3. 제안하는 새로운 아키텍처

### 3.1 계층별 분리

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

### 3.2 주요 클래스 설계

#### Core Domain Classes

```cpp
// 원자 관리 전담
class AtomManager {
public:
    void addAtom(const AtomInfo& atom);
    void removeAtom(uint32_t atomId);
    void updateAtom(uint32_t atomId, const AtomInfo& newInfo);
    std::vector<AtomInfo> getAtoms() const;
    AtomInfo* findAtom(uint32_t atomId);
    
private:
    std::vector<AtomInfo> originalAtoms;
    std::vector<AtomInfo> surroundingAtoms;
    uint32_t nextAtomId = 1;
};

// 결합 관리 전담
class BondManager {
public:
    void createBond(uint32_t atom1Id, uint32_t atom2Id, BondType type);
    void removeBond(uint32_t bondId);
    void removeAllBonds();
    std::vector<BondInfo> getBonds() const;
    bool shouldCreateBond(const AtomInfo& atom1, const AtomInfo& atom2);
    
private:
    std::vector<BondInfo> bonds;
    uint32_t nextBondId = 1;
    float bondScalingFactor = 1.0f;
};

// 격자 구조 관리 전담
class CellManager {
public:
    void setCellMatrix(const float matrix[3][3]);
    void getCellMatrix(float matrix[3][3]) const;
    void cartesianToFractional(const float cart[3], float frac[3]) const;
    void fractionalToCartesian(const float frac[3], float cart[3]) const;
    bool isNearBoundary(const float fracCoords[3], float threshold = 0.1f) const;
    
private:
    float cellMatrix[3][3];
    float invMatrix[3][3];
    void calculateInverseMatrix();
};

// 원소 데이터베이스
class ElementDatabase {
public:
    const Element* getElement(const std::string& symbol) const;
    ImVec4 getElementColor(const std::string& symbol) const;
    float getCovalentRadius(const std::string& symbol) const;
    std::vector<Element> getAllElements() const;
    
private:
    std::map<std::string, Element> elements;
    void initializeElements();
};
```

#### Infrastructure Classes

```cpp
// VTK 렌더링 전담
class VTKRenderer {
public:
    void initialize();
    void addAtomGroup(const std::string& symbol, const std::vector<AtomRenderData>& atoms);
    void addBondGroup(const std::string& bondKey, const std::vector<BondRenderData>& bonds);
    void updateAtomGroup(const std::string& symbol);
    void updateBondGroup(const std::string& bondKey);
    void removeAtomGroup(const std::string& symbol);
    void removeBondGroup(const std::string& bondKey);
    void render();
    
private:
    std::map<std::string, AtomGroupRenderer> atomGroups;
    std::map<std::string, BondGroupRenderer> bondGroups;
};

// 파일 I/O 전담
class FileIOManager {
public:
    bool loadXSF(const std::string& filepath, StructureData& data);
    bool saveCIF(const std::string& filepath, const StructureData& data);
    bool loadPOSCAR(const std::string& filepath, StructureData& data);
    
private:
    std::unique_ptr<FileReader> createReader(const std::string& extension);
};

// 배치 업데이트 시스템
class BatchUpdateSystem {
public:
    void beginBatch();
    void endBatch();
    void scheduleAtomUpdate(const std::string& symbol);
    void scheduleBondUpdate(const std::string& bondKey);
    bool isBatchMode() const;
    
private:
    bool batchActive = false;
    std::set<std::string> pendingAtomUpdates;
    std::set<std::string> pendingBondUpdates;
    std::unique_ptr<VTKRenderer> renderer;
};
```

#### UI Layer

```cpp
// UI 컨트롤러
class AtomsTemplateUI {
public:
    AtomsTemplateUI(AtomsTemplate* controller);
    void render(bool* openWindow);
    
private:
    void renderCreatedAtomsSection();
    void renderBondsManagementTools();
    void renderCellInformation();
    bool renderAtomTable(bool editMode, bool useFractionalCoords);
    
    AtomsTemplate* controller;
    UIState uiState;
};

// 재사용 가능한 UI 컴포넌트
class UIComponents {
public:
    static bool atomTable(const std::vector<AtomInfo>& atoms, AtomTableConfig& config);
    static bool cellMatrixEditor(float matrix[3][3], bool& editMode);
    static bool elementPicker(std::string& selectedElement);
    static void performanceStats(const PerformanceData& stats);
};
```

#### Main Controller (리팩토링된 AtomsTemplate)

```cpp
class AtomsTemplate {
    DECLARE_SINGLETON(AtomsTemplate)
    
public:
    // High-level operations
    void loadStructure(const std::string& filepath);
    void saveStructure(const std::string& filepath);
    void addAtom(const std::string& symbol, const float position[3]);
    void removeAtom(uint32_t atomId);
    void createAllBonds();
    void clearAllBonds();
    void showSurroundingAtoms(bool show);
    
    // UI Integration
    void render(bool* openWindow = nullptr);
    
    // Getters for UI
    const std::vector<AtomInfo>& getAtoms() const;
    const std::vector<BondInfo>& getBonds() const;
    const CellManager& getCellManager() const;
    
private:
    // Core managers
    std::unique_ptr<AtomManager> atomManager;
    std::unique_ptr<BondManager> bondManager;
    std::unique_ptr<CellManager> cellManager;
    std::unique_ptr<ElementDatabase> elementDB;
    
    // Infrastructure
    std::unique_ptr<VTKRenderer> renderer;
    std::unique_ptr<FileIOManager> fileManager;
    std::unique_ptr<BatchUpdateSystem> batchSystem;
    
    // UI
    std::unique_ptr<AtomsTemplateUI> ui;
    
    // Event handlers
    void onAtomChanged(uint32_t atomId);
    void onBondChanged(uint32_t bondId);
    void onCellChanged();
};
```

## 4. 단계별 리팩토링 계획

### Phase 1: 데이터 구조 분리 (2-3주)
**목표**: 원자, 결합, 격자 관련 구조체와 유틸리티 분리

#### 1.1 Core Data Structures
```cpp
// atoms/atom_info.h
struct AtomInfo {
    uint32_t id;
    std::string symbol;
    float position[3];
    float fracPosition[3];
    float radius;
    ImVec4 color;
    AtomType type;
    // ... 기타 필드
};

// bonds/bond_info.h  
struct BondInfo {
    uint32_t id;
    uint32_t atom1Id, atom2Id;
    BondType type;
    std::string groupKey;
    // ... 기타 필드
};

// cell/cell_info.h
struct CellInfo {
    float matrix[3][3];
    float invMatrix[3][3];
    bool isValid() const;
};
```

#### 1.2 Utility Functions
```cpp
// utils/coordinate_utils.h
namespace CoordinateUtils {
    void cartesianToFractional(const float cart[3], float frac[3], const float matrix[3][3]);
    void fractionalToCartesian(const float frac[3], float cart[3], const float matrix[3][3]);
    void calculateInverseMatrix(const float matrix[3][3], float inverse[3][3]);
}

// utils/color_utils.h
namespace ColorUtils {
    ImVec4 getContrastTextColor(const ImVec4& bgColor);
    ImVec4 adjustColorForType(const ImVec4& baseColor, AtomType type);
}
```

**작업 항목**:
- [ ] atom_info.h/cpp 생성 및 AtomInfo 구조체 이동
- [ ] bond_info.h/cpp 생성 및 BondInfo 구조체 이동  
- [ ] cell_info.h/cpp 생성 및 CellInfo 구조체 이동
- [ ] coordinate_utils.h/cpp 생성 및 좌표 변환 함수 이동
- [ ] color_utils.h/cpp 생성 및 색상 유틸리티 이동
- [ ] 기존 코드에서 새로운 헤더 include로 변경
- [ ] 컴파일 오류 수정 및 테스트

### Phase 2: ElementDatabase 분리 (1-2주)  
**목표**: 원소 관련 데이터와 주기율표 기능 분리

#### 2.1 ElementDatabase 클래스
```cpp
// elements/element_database.h
class ElementDatabase {
public:
    static ElementDatabase& getInstance();
    
    const Element* getElement(const std::string& symbol) const;
    const Element* getElement(int atomicNumber) const;
    ImVec4 getElementColor(const std::string& symbol) const;
    float getCovalentRadius(const std::string& symbol) const;
    float getAtomicMass(const std::string& symbol) const;
    std::string getElementName(const std::string& symbol) const;
    
    std::vector<Element> getAllElements() const;
    std::vector<Element> getElementsByPeriod(int period) const;
    std::vector<Element> getElementsByGroup(int group) const;
    
    void addCustomElement(const Element& element);
    
private:
    ElementDatabase();
    void initializeElements();
    
    std::map<std::string, Element> elementsBySymbol;
    std::map<int, Element> elementsByNumber;
};

// elements/element.h
struct Element {
    std::string symbol;
    std::string name;
    int atomicNumber;
    float atomicMass;
    float covalentRadius;
    ImVec4 color;
    int period;
    int group;
    std::string classification;
};
```

**작업 항목**:
- [ ] Element 구조체 정의 및 데이터 정리
- [ ] ElementDatabase 클래스 구현
- [ ] 주기율표 데이터 초기화 로직 구현  
- [ ] 주기/족별 조회 기능 구현
- [ ] 사용자 정의 원소 추가 기능 구현
- [ ] 기존 전역 배열들을 ElementDatabase로 대체
- [ ] 단위 테스트 작성

### Phase 3: Manager 클래스들 분리 (3-4주)
**목표**: 핵심 도메인 로직을 담당하는 Manager 클래스들 구현

#### 3.1 AtomManager
```cpp
// atoms/atom_manager.h  
class AtomManager {
public:
    // CRUD Operations
    uint32_t addAtom(const AtomCreateRequest& request);
    bool removeAtom(uint32_t atomId);
    bool updateAtom(uint32_t atomId, const AtomUpdateRequest& request);
    
    // Query Operations
    AtomInfo* findAtom(uint32_t atomId);
    const AtomInfo* findAtom(uint32_t atomId) const;
    std::vector<AtomInfo> getAtoms(AtomType type = AtomType::ORIGINAL) const;
    std::vector<AtomInfo> getAtomsByElement(const std::string& symbol) const;
    
    // Batch Operations
    void clearAllAtoms(AtomType type);
    void updateAllAtoms(const std::function<void(AtomInfo&)>& updater);
    
    // Surrounding Atoms
    void createSurroundingAtoms(const CellManager& cellManager);
    void clearSurroundingAtoms();
    bool areSurroundingAtomsVisible() const;
    
    // Statistics
    size_t getAtomCount(AtomType type = AtomType::ORIGINAL) const;
    std::map<std::string, int> getElementCounts() const;
    
    // Events
    void onAtomAdded(uint32_t atomId);
    void onAtomRemoved(uint32_t atomId);
    void onAtomUpdated(uint32_t atomId);
    
private:
    std::vector<AtomInfo> originalAtoms;
    std::vector<AtomInfo> surroundingAtoms;
    uint32_t nextAtomId = 1;
    bool surroundingVisible = false;
    
    uint32_t generateAtomId();
    void validateAtom(const AtomInfo& atom);
};
```

#### 3.2 BondManager
```cpp
// bonds/bond_manager.h
class BondManager {
public:
    // Bond Creation
    uint32_t createBond(uint32_t atom1Id, uint32_t atom2Id, BondType type = BondType::ORIGINAL);
    bool removeBond(uint32_t bondId);
    void removeAllBonds(BondType type);
    
    // Automatic Bond Generation  
    void createAllBonds(const AtomManager& atomManager);
    void createBondsForAtom(uint32_t atomId, const AtomManager& atomManager);
    void removeBondsForAtom(uint32_t atomId);
    
    // Query Operations
    std::vector<BondInfo> getBonds(BondType type = BondType::ORIGINAL) const;
    std::vector<BondInfo> getBondsForAtom(uint32_t atomId) const;
    BondInfo* findBond(uint32_t bondId);
    
    // Bond Criteria
    bool shouldCreateBond(const AtomInfo& atom1, const AtomInfo& atom2) const;
    void setBondScalingFactor(float factor);
    float getBondScalingFactor() const;
    
    // Bond Properties
    void setBondThickness(float thickness);
    void setBondOpacity(float opacity);
    float getBondThickness() const;
    float getBondOpacity() const;
    
    // Statistics
    size_t getBondCount(BondType type = BondType::ORIGINAL) const;
    std::map<std::string, int> getBondTypeCounts() const;
    
private:
    std::vector<BondInfo> bonds;
    uint32_t nextBondId = 1;
    float bondScalingFactor = 1.0f;
    float bondThickness = 1.0f;  
    float bondOpacity = 1.0f;
    
    uint32_t generateBondId();
    std::string generateBondGroupKey(const std::string& elem1, const std::string& elem2);
    float calculateBondRadius(const AtomInfo& atom1, const AtomInfo& atom2);
};
```

#### 3.3 CellManager  
```cpp
// cell/cell_manager.h
class CellManager {
public:
    // Cell Definition
    void setCellMatrix(const float matrix[3][3]);
    void getCellMatrix(float matrix[3][3]) const;
    bool isCellDefined() const;
    
    // Coordinate Conversion
    void cartesianToFractional(const float cart[3], float frac[3]) const;
    void fractionalToCartesian(const float frac[3], float cart[3]) const;
    
    // Boundary Detection
    bool isNearBoundary(const float fracCoords[3], float threshold = 0.1f) const;
    std::vector<std::array<float, 3>> getBoundaryTranslations(const float fracCoords[3], float threshold = 0.1f) const;
    
    // Cell Properties
    float getCellVolume() const;
    std::array<float, 3> getCellLengths() const;
    std::array<float, 3> getCellAngles() const;
    
    // Cell Visualization
    void setVisible(bool visible);
    bool isVisible() const;
    
private:
    float cellMatrix[3][3];
    float invMatrix[3][3];
    bool matrixDefined = false;
    bool visible = false;
    
    void calculateInverseMatrix();
    void validateMatrix() const;
};
```

**작업 항목**:
- [ ] AtomManager 클래스 구현 및 테스트
- [ ] BondManager 클래스 구현 및 테스트  
- [ ] CellManager 클래스 구현 및 테스트
- [ ] Manager 클래스 간 상호작용 인터페이스 정의
- [ ] 기존 AtomsTemplate의 해당 로직을 Manager로 이동
- [ ] 단위 테스트 및 통합 테스트 작성

### Phase 4: 렌더링 시스템 분리 (2-3주)
**목표**: VTK 렌더링 로직을 별도 클래스로 분리

#### 4.1 VTKRenderer 
```cpp
// rendering/vtk_renderer.h
class VTKRenderer {
public:
    void initialize();
    void shutdown();
    
    // Atom Rendering
    void addAtomGroup(const std::string& symbol, const AtomGroupData& data);
    void updateAtomGroup(const std::string& symbol, const AtomGroupData& data);
    void removeAtomGroup(const std::string& symbol);
    void clearAllAtomGroups();
    
    // Bond Rendering  
    void addBondGroup(const std::string& bondKey, const BondGroupData& data);
    void updateBondGroup(const std::string& bondKey, const BondGroupData& data);
    void removeBondGroup(const std::string& bondKey);
    void clearAllBondGroups();
    
    // Cell Rendering
    void setCellVectors(const float matrix[3][3]);
    void setCellVisible(bool visible);
    
    // Rendering Control
    void render();
    void setResolution(int resolution);
    int getResolution() const;
    
    // Memory Management
    float calculateMemoryUsage() const;
    void optimizeMemory();
    
private:
    std::map<std::string, std::unique_ptr<AtomGroupRenderer>> atomGroups;
    std::map<std::string, std::unique_ptr<BondGroupRenderer>> bondGroups;
    std::unique_ptr<CellRenderer> cellRenderer;
    int resolution = 20;
    
    void setupRenderingPipeline();
};

// rendering/atom_group_renderer.h
class AtomGroupRenderer {
public:
    void initialize(const std::string& symbol, float baseRadius);
    void update(const AtomGroupData& data);
    void cleanup();
    
private:
    vtkSmartPointer<vtkPolyData> baseGeometry;
    vtkSmartPointer<vtkAppendPolyData> appender;
    vtkSmartPointer<vtkPolyDataMapper> mapper;
    vtkSmartPointer<vtkActor> actor;
};
```

**작업 항목**:
- [ ] VTKRenderer 기본 인터페이스 설계
- [ ] AtomGroupRenderer 구현
- [ ] BondGroupRenderer 구현 (2-color 지원)
- [ ] CellRenderer 구현
- [ ] 기존 VTK 코드를 새로운 클래스들로 이동
- [ ] 렌더링 성능 최적화
- [ ] 메모리 관리 개선

### Phase 5: 파일 I/O 시스템 분리 (2-3주)
**목표**: 파일 입출력을 확장 가능한 시스템으로 리팩토링

#### 5.1 FileIOManager와 Strategy 패턴
```cpp
// io/file_io_manager.h
class FileIOManager {
public:
    bool loadStructure(const std::string& filepath, StructureData& data);
    bool saveStructure(const std::string& filepath, const StructureData& data);
    
    void registerReader(const std::string& extension, std::unique_ptr<FileReader> reader);
    void registerWriter(const std::string& extension, std::unique_ptr<FileWriter> writer);
    
    std::vector<std::string> getSupportedReadFormats() const;
    std::vector<std::string> getSupportedWriteFormats() const;
    
private:
    std::map<std::string, std::unique_ptr<FileReader>> readers;
    std::map<std::string, std::unique_ptr<FileWriter>> writers;
    
    std::string getFileExtension(const std::string& filepath);
};

// io/file_reader.h
class FileReader {
public:
    virtual ~FileReader() = default;
    virtual bool read(const std::string& filepath, StructureData& data) = 0;
    virtual std::string getFormatName() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

// io/xsf_reader.h
class XSFReader : public FileReader {
public:
    bool read(const std::string& filepath, StructureData& data) override;
    std::string getFormatName() const override { return "XCrySDen Structure File"; }
    std::vector<std::string> getSupportedExtensions() const override { return {".xsf"}; }
    
private:
    bool parseXSFFile(std::ifstream& file, StructureData& data);
};

// io/structure_data.h
struct StructureData {
    std::vector<AtomData> atoms;
    float cellMatrix[3][3];
    std::string title;
    std::map<std::string, std::string> metadata;
    
    bool isValid() const;
    void clear();
};
```

**작업 항목**:
- [ ] FileReader/FileWriter 인터페이스 설계
- [ ] StructureData 통합 데이터 구조 정의
- [ ] XSFReader 구현 (기존 로직 이동)
- [ ] 추가 파일 형식 지원 (CIF, POSCAR, PDB)
- [ ] 파일 형식 자동 감지 기능
- [ ] 에러 처리 및 검증 강화

### Phase 6: 배치 업데이트 시스템 분리 (1-2주)
**목표**: 성능 최적화를 위한 배치 시스템 모듈화

#### 6.1 BatchUpdateSystem
```cpp
// updates/batch_update_system.h
class BatchUpdateSystem {
public:
    void beginBatch();
    void endBatch();
    bool isBatchActive() const;
    
    void scheduleAtomUpdate(const std::string& symbol);
    void scheduleBondUpdate(const std::string& bondKey);
    
    void setRenderer(VTKRenderer* renderer);
    
    class BatchGuard {
    public:
        explicit BatchGuard(BatchUpdateSystem* system);
        ~BatchGuard();
        
    private:
        BatchUpdateSystem* system;
        bool wasActivated;
    };
    
    BatchGuard createBatchGuard();
    
private:
    bool batchActive = false;
    std::set<std::string> pendingAtomUpdates;
    std::set<std::string> pendingBondUpdates;
    VTKRenderer* renderer = nullptr;
    
    void processPendingUpdates();
    void updatePerformanceStats(float duration);
};
```

**작업 항목**:
- [ ] BatchUpdateSystem 구현
- [ ] BatchGuard RAII 패턴 구현
- [ ] 성능 통계 수집 기능
- [ ] VTKRenderer와의 통합
- [ ] 기존 배치 로직 이동 및 개선

### Phase 7: UI 분리 (2-3주)  
**목표**: UI 로직을 별도 클래스로 분리하여 재사용성 향상

#### 7.1 AtomsTemplateUI
```cpp
// ui/atoms_template_ui.h
class AtomsTemplateUI {
public:
    explicit AtomsTemplateUI(AtomsTemplate* controller);
    void render(bool* openWindow);
    
private:
    void renderCreatedAtomsSection();
    void renderBondsManagementTools(); 
    void renderCellInformation();
    bool renderAtomTable(bool editMode, bool useFractionalCoords);
    void renderPerformanceStats();
    
    AtomsTemplate* controller;
    UIState uiState;
    
    struct UIState {
        bool editMode = false;
        bool useFractionalCoords = false;
        bool surroundingsVisible = false;
        bool cellVisible = false;
        std::set<uint32_t> selectedAtoms;
    };
};

// ui/ui_components.h
namespace UIComponents {
    bool atomTable(
        const std::vector<AtomInfo>& atoms,
        const AtomTableConfig& config,
        AtomTableState& state
    );
    
    bool cellMatrixEditor(
        float matrix[3][3], 
        bool& editMode,
        const CellEditorConfig& config = {}
    );
    
    bool elementPicker(
        std::string& selectedElement,
        const ElementDatabase& database
    );
    
    void performanceStats(const PerformanceData& stats);
    
    bool bondControls(
        float& thickness,
        float& opacity, 
        float& distanceFactor
    );
}
```

**작업 항목**:
- [ ] AtomsTemplateUI 클래스 구현
- [ ] 재사용 가능한 UI 컴포넌트 라이브러리 구현
- [ ] UI 상태 관리 시스템 구현
- [ ] 기존 UI 로직을 새로운 클래스로 이동
- [ ] UI와 비즈니스 로직 분리 검증

### Phase 8: 메인 컨트롤러 리팩토링 (2주)
**목표**: AtomsTemplate을 가벼운 컨트롤러로 변환

#### 8.1 새로운 AtomsTemplate
```cpp
// atoms_template.h (리팩토링 후)
class AtomsTemplate {
    DECLARE_SINGLETON(AtomsTemplate)
    
public:
    // Initialization
    bool initialize();
    void shutdown();
    
    // High-level Operations  
    void loadStructure(const std::string& filepath);
    void saveStructure(const std::string& filepath);
    void clearAll();
    
    // Atom Operations
    uint32_t addAtom(const std::string& symbol, const float position[3]);
    bool removeAtom(uint32_t atomId);
    bool updateAtom(uint32_t atomId, const AtomInfo& newInfo);
    void showSurroundingAtoms(bool show);
    
    // Bond Operations
    void createAllBonds();
    void clearAllBonds();
    void setBondProperties(float thickness, float opacity, float distanceFactor);
    
    // Cell Operations
    void setCellMatrix(const float matrix[3][3]);
    void showCell(bool show);
    
    // UI Integration
    void render(bool* openWindow = nullptr);
    
    // Manager Access (for UI)
    const AtomManager& getAtomManager() const { return *atomManager; }
    const BondManager& getBondManager() const { return *bondManager; }
    const CellManager& getCellManager() const { return *cellManager; }
    const ElementDatabase& getElementDatabase() const { return *elementDB; }
    
private:
    // Core Components
    std::unique_ptr<AtomManager> atomManager;
    std::unique_ptr<BondManager> bondManager; 
    std::unique_ptr<CellManager> cellManager;
    std::unique_ptr<ElementDatabase> elementDB;
    
    // Infrastructure
    std::unique_ptr<VTKRenderer> renderer;
    std::unique_ptr<FileIOManager> fileManager;
    std::unique_ptr<BatchUpdateSystem> batchSystem;
    
    // UI
    std::unique_ptr<AtomsTemplateUI> ui;
    
    bool initialized = false;
    
    // Event Handlers
    void onAtomChanged(uint32_t atomId);
    void onBondChanged(uint32_t bondId);
    void onCellChanged();
    
    // Component Integration
    void setupComponentConnections();
};
```

**작업 항목**:
- [ ] 새로운 AtomsTemplate 클래스 구현
- [ ] 컴포넌트 간 통합 인터페이스 구현
- [ ] 이벤트 시스템 구현
- [ ] 의존성 주입 패턴 적용
- [ ] 전체 시스템 통합 테스트

## 5. 추가 개선 사항

### 5.1 확장성 개선
```cpp
// 플러그인 시스템
class PluginManager {
public:
    void loadPlugin(const std::string& path);
    void registerFileFormat(const std::string& ext, std::unique_ptr<FileReader> reader);
    void registerRenderingBackend(const std::string& name, std::unique_ptr<Renderer> backend);
};

// 다중 렌더링 백엔드 지원
class RenderingBackend {
public:
    virtual ~RenderingBackend() = default;
    virtual void initialize() = 0;
    virtual void render(const RenderData& data) = 0;
};

class VTKBackend : public RenderingBackend { /*...*/ };
class OpenGLBackend : public RenderingBackend { /*...*/ };
```

### 5.2 Bravais Lattice 템플릿
```cpp
// cell/bravais_lattice.h
enum class BravaisLattice {
    Cubic, Tetragonal, Orthorhombic, Hexagonal,
    Trigonal, Monoclinic, Triclinic
};

class LatticeTemplate {
public:
    static std::vector<LatticeTemplate> getAllTemplates();
    static LatticeTemplate getTemplate(BravaisLattice type);
    
    void applyCellMatrix(float matrix[3][3], float a, float b, float c, 
                        float alpha, float beta, float gamma) const;
    
    std::string getName() const;
    BravaisLattice getType() const;
    std::vector<std::string> getRequiredParameters() const;
    
private:
    BravaisLattice latticeType;
    std::string name;
    std::function<void(float[3][3], float, float, float, float, float, float)> matrixCalculator;
};
```

### 5.3 주기율표 UI 컴포넌트
```cpp
// ui/periodic_table.h
class PeriodicTableWidget {
public:
    void render();
    void setSelectionMode(SelectionMode mode);
    void setSelectedElements(const std::set<std::string>& elements);
    std::set<std::string> getSelectedElements() const;
    
    // Callbacks
    std::function<void(const std::string&)> onElementSelected;
    std::function<void(const std::string&)> onElementHovered;
    
private:
    void renderElement(const Element& element, const ImVec2& position);
    void renderElementTooltip(const Element& element);
    
    SelectionMode selectionMode = SelectionMode::Single;
    std::set<std::string> selectedElements;
    const ElementDatabase& elementDB;
};
```

## 6. 마이그레이션 전략

### 6.1 점진적 마이그레이션
1. **기존 코드 유지**: 리팩토링 중에도 기존 기능 동작 보장
2. **인터페이스 호환성**: 기존 API 유지하면서 내부 구현만 변경
3. **단계별 검증**: 각 단계별로 테스트 및 검증 수행
4. **롤백 계획**: 문제 발생 시 이전 버전으로 롤백 가능

### 6.2 테스트 전략
```cpp
// 단위 테스트
TEST(AtomManagerTest, AddAtom) {
    AtomManager manager;
    AtomCreateRequest request{"C", {0, 0, 0}};
    uint32_t id = manager.addAtom(request);
    EXPECT_NE(id, 0);
    EXPECT_EQ(manager.getAtomCount(), 1);
}

// 통합 테스트
TEST(AtomsTemplateIntegrationTest, LoadXSFFile) {
    AtomsTemplate template;
    template.loadStructure("test.xsf");
    EXPECT_GT(template.getAtomManager().getAtomCount(), 0);
}

// 성능 테스트
TEST(PerformanceTest, LargeStructureLoading) {
    auto start = std::chrono::high_resolution_clock::now();
    // ... 대용량 파일 로딩
    auto duration = std::chrono::high_resolution_clock::now() - start;
    EXPECT_LT(duration.count(), MAX_LOADING_TIME_MS);
}
```

## 7. 예상 효과

### 7.1 코드 품질 향상
- **가독성**: 각 클래스의 책임이 명확해져 이해하기 쉬움
- **유지보수성**: 변경 영향 범위 최소화
- **테스트 용이성**: 각 컴포넌트 독립적 테스트 가능
- **확장성**: 새로운 기능 추가 시 기존 코드 영향 최소화

### 7.2 성능 개선
- **메모리 효율성**: 불필요한 데이터 중복 제거
- **렌더링 최적화**: 배치 업데이트 시스템 개선  
- **캐싱**: 자주 사용되는 계산 결과 캐싱

### 7.3 확장성 확보
- **파일 형식**: 새로운 파일 형식 지원 용이
- **렌더링 백엔드**: VTK 외 다른 렌더링 엔진 지원 가능
- **UI 테마**: 다양한 UI 테마 및 레이아웃 지원
- **플러그인**: 서드파티 확장 기능 지원

## 8. 리소스 요구사항

### 8.1 개발 시간
- **총 예상 시간**: 14-20주
- **핵심 개발자**: 2-3명
- **단계별 검증**: 각 단계마다 1주씩 추가

### 8.2 위험 요소 및 대응방안
- **호환성 문제**: 기존 API 유지 및 점진적 마이그레이션
- **성능 저하**: 각 단계별 성능 벤치마크 수행
- **복잡성 증가**: 명확한 인터페이스 설계 및 문서화
- **일정 지연**: 버퍼 시간 확보 및 우선순위 조정

이 리팩토링 계획을 통해 현재의 거대한 단일 클래스를 명확한 책임을 가진 여러 컴포넌트로 분리하여, 유지보수성과 확장성을 크게 향상시킬 수 있을 것입니다.