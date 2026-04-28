# AtomsTemplate 리팩토링 상세 작업목록

> **📋 작업 참고사항**: 이 문서는 Claude가 단계별로 리팩토링을 수행할 수 있도록 구체적인 작업 내용과 구현 가이드를 제공합니다.

---

## 🎯 **전체 작업 개요**

### **현재 상태 분석 (2024년 기준)**
- **메인 파일**: `atoms_template.h` (3000+ 라인), `atoms_template.cpp`
- **확장 파일**: `atoms_template_periodic_table.cpp`, `atoms_template_bravais_lattice.cpp`
- **주요 문제**: 단일 클래스에 85개+ 함수, UI/비즈니스/인프라 로직 혼재
- **의존성**: VTK, ImGui, spdlog, 각종 전역 변수/함수

### **목표 상태**
- **계층별 분리**: UI → Application → Domain → Infrastructure
- **파일 개수**: 10개 → 120개 (모듈화)
- **클래스 책임**: 다중 → 단일 책임 원칙
- **확장성**: 새로운 파일 형식, UI 컴포넌트, 렌더링 백엔드 지원

---

## 📅 **Phase 1: Infrastructure Layer (2-3주)**
**목표**: 외부 의존성(VTK, 파일 I/O, 배치 시스템) 분리

### 1.1 VTK 렌더링 시스템 분리 (1주)

#### **Task 1.1.1**: 기본 인프라 구조 생성
```bash
# 파일 생성 목록
src/infrastructure/rendering/vtk_renderer.h
src/infrastructure/rendering/vtk_renderer.cpp
src/infrastructure/rendering/atom_group_renderer.h  
src/infrastructure/rendering/atom_group_renderer.cpp
src/infrastructure/rendering/bond_group_renderer.h
src/infrastructure/rendering/bond_group_renderer.cpp
src/infrastructure/rendering/cell_renderer.h
src/infrastructure/rendering/cell_renderer.cpp
src/infrastructure/rendering/rendering_utils.h
src/infrastructure/rendering/rendering_utils.cpp
```

**구현 가이드**:
```cpp
// vtk_renderer.h - 메인 VTK 렌더링 시스템
class VTKRenderer {
public:
    void initialize();
    void shutdown();
    
    // 원자 렌더링 - 기존 함수들을 여기로 이동
    void addAtomGroup(const std::string& symbol, const AtomGroupData& data);
    void updateAtomGroup(const std::string& symbol); // ← updateUnifiedAtomGroupVTK에서 이동
    void removeAtomGroup(const std::string& symbol);
    
    // 결합 렌더링 - 기존 함수들을 여기로 이동  
    void addBondGroup(const std::string& bondKey, const BondGroupData& data);
    void updateBondGroup(const std::string& bondKey); // ← updateBondGroup에서 이동
    void removeBondGroup(const std::string& bondKey);
    
    // 격자 렌더링 - 기존 함수들을 여기로 이동
    void setCellVectors(const float matrix[3][3]); // ← createUnitCell에서 이동
    void setCellVisible(bool visible); // ← cellVisible 변수 관리를 여기로
    
    void render();
    float calculateMemoryUsage() const; // ← calculateAtomGroupMemory에서 이동
    
private:
    std::map<std::string, std::unique_ptr<AtomGroupRenderer>> atomGroups;
    std::map<std::string, std::unique_ptr<BondGroupRenderer>> bondGroups;
    std::unique_ptr<CellRenderer> cellRenderer;
};
```

**이동할 기존 함수들**:
- `addAtomToGroup()` → `VTKRenderer::addAtomGroup()`
- `updateUnifiedAtomGroupVTK()` → `AtomGroupRenderer::update()`
- `updateBondGroup()` → `BondGroupRenderer::update()`
- `createUnitCell()` → `CellRenderer::setCellVectors()`
- `clearUnitCell()` → `CellRenderer::clear()`

#### **Task 1.1.2**: 전역 VTK 함수들을 클래스로 이동
```cpp
// rendering_utils.h - 전역 함수들을 static 메서드로 변환
class RenderingUtils {
public:
    // 기존: vtkSmartPointer<vtkPolyData> createBaseBondGeometry(float radius, float height)
    static vtkSmartPointer<vtkPolyData> createBaseBondGeometry(float radius, float height);
    
    // 기존: void setupBondActorProperties(vtkSmartPointer<vtkActor> actor)  
    static void setupBondActorProperties(vtkSmartPointer<vtkActor> actor);
    
    // 기존: std::string generateBondTypeKey(const std::string& element1, const std::string& element2)
    static std::string generateBondTypeKey(const std::string& element1, const std::string& element2);
    
    // 기존: std::pair<...> calculateBondTransforms2Color(...)
    static BondTransformPair calculateBondTransforms2Color(const AtomInfo& atom1, const AtomInfo& atom2);
};
```

**마이그레이션 체크리스트**:
- [ ] `createBaseBondGeometry()` 전역 함수 → `RenderingUtils::createBaseBondGeometry()`
- [ ] `setupBondActorProperties()` 전역 함수 → `RenderingUtils::setupBondActorProperties()`
- [ ] `generateBondTypeKey()` 전역 함수 → `RenderingUtils::generateBondTypeKey()`
- [ ] `calculateBondTransforms2Color()` 전역 함수 → `RenderingUtils::calculateBondTransforms2Color()`

### 1.2 파일 I/O 시스템 분리 (1주)

#### **Task 1.2.1**: 파일 I/O 기본 구조 생성
```bash
# 파일 생성 목록
src/infrastructure/io/file_io_manager.h
src/infrastructure/io/file_io_manager.cpp
src/infrastructure/io/file_reader.h
src/infrastructure/io/xsf_reader.h
src/infrastructure/io/xsf_reader.cpp
src/infrastructure/io/structure_data.h
src/infrastructure/io/structure_data.cpp
```

**구현 가이드**:
```cpp
// file_reader.h - 추상 인터페이스
class FileReader {
public:
    virtual ~FileReader() = default;
    virtual bool read(const std::string& filepath, StructureData& data) = 0;
    virtual std::string getFormatName() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

// xsf_reader.h - XSF 파일 전용 리더
class XSFReader : public FileReader {
public:
    bool read(const std::string& filepath, StructureData& data) override;
    std::string getFormatName() const override { return "XCrySDen Structure File"; }
    std::vector<std::string> getSupportedExtensions() const override { return {".xsf"}; }
    
private:
    // 기존: bool ParseXSFFile(const std::string& filePath) → 여기로 이동
    bool parseXSFFile(std::ifstream& file, StructureData& data);
};

// structure_data.h - 통합 구조 데이터
struct StructureData {
    std::vector<AtomData> atoms;
    float cellMatrix[3][3];
    std::string title;
    std::map<std::string, std::string> metadata;
    
    bool isValid() const;
    void clear();
};
```

**이동할 기존 함수들**:
- `ParseXSFFile()` → `XSFReader::parseXSFFile()`
- `InitializeAtomicStructure()` → `StructureData::initialize()` 또는 별도 processor
- `AtomData` 구조체 → `StructureData` 내부로 이동

#### **Task 1.2.2**: 확장 가능한 파일 형식 지원
```cpp
// file_io_manager.h - Strategy 패턴 적용
class FileIOManager {
public:
    bool loadStructure(const std::string& filepath, StructureData& data);
    void registerReader(const std::string& extension, std::unique_ptr<FileReader> reader);
    std::vector<std::string> getSupportedFormats() const;
    
private:
    std::map<std::string, std::unique_ptr<FileReader>> readers;
    std::string getFileExtension(const std::string& filepath);
};
```

**확장 파일 형식 추가 준비**:
- [ ] CIF Reader 인터페이스 설계
- [ ] POSCAR Reader 인터페이스 설계  
- [ ] PDB Reader 인터페이스 설계

### 1.3 배치 업데이트 시스템 분리 (0.5주)

#### **Task 1.3.1**: 배치 시스템 독립화
```bash
# 파일 생성 목록
src/infrastructure/batch/batch_update_system.h
src/infrastructure/batch/batch_update_system.cpp
src/infrastructure/batch/performance_monitor.h
src/infrastructure/batch/performance_monitor.cpp
```

**구현 가이드**:
```cpp
// batch_update_system.h
class BatchUpdateSystem {
public:
    void beginBatch();
    void endBatch(); // ← AtomsTemplate::endBatch()에서 로직 이동
    bool isBatchActive() const;
    
    void scheduleAtomUpdate(const std::string& symbol);
    void scheduleBondUpdate(const std::string& bondKey);
    
    // BatchGuard는 그대로 유지하되, 이 클래스를 참조하도록 수정
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
    
    // 기존: void updatePerformanceStats(float duration) → 여기로 이동
    void updatePerformanceStats(float duration);
};
```

**이동할 기존 코드**:
- `beginBatch()` → `BatchUpdateSystem::beginBatch()`
- `endBatch()` → `BatchUpdateSystem::endBatch()`
- `BatchGuard` 클래스 → `BatchUpdateSystem::BatchGuard`
- `updatePerformanceStats()` → `BatchUpdateSystem::updatePerformanceStats()`

---

## 🎯 **Phase 2: Domain Layer (3-4주)**  
**목표**: 핵심 비즈니스 로직 분리 (가장 중요한 단계)

### 2.1 ElementDatabase 구축 (1주)

#### **Task 2.1.1**: 기본 ElementDatabase 구조 생성
```bash
# 파일 생성 목록
src/domain/elements/element_database.h
src/domain/elements/element_database.cpp
src/domain/elements/element.h
src/domain/elements/element.cpp
src/domain/elements/periodic_table_data.h
src/domain/elements/periodic_table_data.cpp
```

**구현 가이드**:
```cpp
// element_database.h - 싱글톤 패턴
class ElementDatabase {
public:
    static ElementDatabase& getInstance();
    
    // 기존 함수들을 여기로 이동
    const Element* getElement(const std::string& symbol) const;
    const Element* getElement(int atomicNumber) const;
    ImVec4 getElementColor(const std::string& symbol) const; // ← findElementColor()에서 이동
    float getCovalentRadius(const std::string& symbol) const;
    std::string getElementName(const std::string& symbol) const;
    std::string getSymbolFromAtomicNumber(int atomicNumber) const; // ← 전역 함수에서 이동
    
    // 확장 기능
    std::vector<Element> getAllElements() const;
    std::vector<Element> getElementsByClassification(ElementClassification classification) const;
    
private:
    ElementDatabase();
    void initializeElements(); // ← 전역 배열 데이터를 여기서 초기화
    
    std::map<std::string, Element> elementsBySymbol;
    std::map<int, Element> elementsByNumber;
};
```

**이동할 기존 데이터**:
- `static std::vector<Element> elements` → `ElementDatabase` 멤버
- `chemical_symbols[]` 배열 → `ElementDatabase` 내부 초기화
- `atomic_names[]` 배열 → `ElementDatabase` 내부 초기화
- `atomic_masses[]` 배열 → `ElementDatabase` 내부 초기화
- `covalent_radii[]` 배열 → `ElementDatabase` 내부 초기화
- `jmol_colors[]` 배열 → `ElementDatabase` 내부 초기화

#### **Task 2.1.2**: 확장된 원소 분류 시스템
```cpp
// element.h - 확장된 Element 구조체
struct Element {
    // 기존 필드들
    std::string symbol;
    std::string name;
    int atomicNumber;
    float atomicMass;
    float covalentRadius;
    ImVec4 color;
    int period;
    int group;
    
    // 새로 추가될 필드들 (atoms_template_periodic_table.cpp 분석 기반)
    ElementClassification classification;
    float atomicRadius;
    float electronegativity;
    std::vector<int> commonOxidationStates;
    bool isRadioactive;
    
    bool isValid() const;
};

enum class ElementClassification {
    All = 0,
    NonMetals = 1,           // ← atoms_template_periodic_table.cpp의 category 매핑
    AlkaliMetals = 2,
    AlkalineEarthMetals = 3,
    TransitionMetals = 4,
    PostTransitionMetals = 5,
    Metalloids = 6,
    Halogens = 7,
    NobleGases = 8,
    Lanthanides = 9,
    Actinides = 10
};
```

### 2.2 AtomManager 구축 (1주)

#### **Task 2.2.1**: 원자 관리 시스템 생성
```bash
# 파일 생성 목록
src/domain/atoms/atom_manager.h
src/domain/atoms/atom_manager.cpp
src/domain/atoms/atom_info.h
src/domain/atoms/atom_info.cpp
src/domain/atoms/surrounding_atoms.h
src/domain/atoms/surrounding_atoms.cpp
```

**구현 가이드**:
```cpp
// atom_manager.h
class AtomManager {
public:
    // CRUD Operations - 기존 함수들을 여기로 이동
    uint32_t addAtom(const AtomCreateRequest& request); // ← createAtomSphere()에서 로직 이동
    bool removeAtom(uint32_t atomId);
    bool updateAtom(uint32_t atomId, const AtomUpdateRequest& request);
    
    // Query Operations - 기존 함수들을 여기로 이동
    AtomInfo* findAtom(uint32_t atomId); // ← findAtomById()에서 이동
    const AtomInfo* findAtom(uint32_t atomId) const;
    std::vector<AtomInfo> getAtoms(AtomType type = AtomType::ORIGINAL) const;
    int findAtomIndex(const AtomInfo& atom) const; // ← findAtomIndex()에서 이동
    
    // Surrounding Atoms - 기존 함수들을 여기로 이동
    void createSurroundingAtoms(const CellManager& cellManager); // ← createSurroundingAtoms()에서 이동
    void clearSurroundingAtoms(); // ← hideSurroundingAtoms()에서 이동
    bool areSurroundingAtomsVisible() const;
    
    // Statistics
    size_t getAtomCount(AtomType type = AtomType::ORIGINAL) const;
    std::map<std::string, int> getElementCounts() const;
    
    // Memory Management - 기존 함수에서 이동
    float calculateMemoryUsage() const; // ← calculateAtomGroupMemory()에서 이동
    
private:
    std::vector<AtomInfo> originalAtoms;
    std::vector<AtomInfo> surroundingAtoms;
    uint32_t nextAtomId = 1;
    bool surroundingVisible = false;
    
    uint32_t generateAtomId(); // ← generateUniqueAtomId()에서 이동
};
```

**이동할 기존 코드**:
- `createAtomSphere()` → `AtomManager::addAtom()` (로직 변환)
- `findAtomById()` → `AtomManager::findAtom()`
- `findAtomIndex()` → `AtomManager::findAtomIndex()`
- `createSurroundingAtoms()` → `AtomManager::createSurroundingAtoms()`
- `hideSurroundingAtoms()` → `AtomManager::clearSurroundingAtoms()`
- `calculateAtomGroupMemory()` → `AtomManager::calculateMemoryUsage()`

#### **Task 2.2.2**: AtomInfo 구조체 정리
```cpp
// atom_info.h - 기존 AtomInfo 구조체를 정리하고 확장
struct AtomInfo {
    // Core Properties (기존 유지)
    uint32_t id = 0;
    std::string symbol;
    ImVec4 color;
    float radius;
    float position[3];
    float fracPosition[3];
    
    // Type and Rendering (기존 유지) 
    AtomType atomType = AtomType::ORIGINAL;
    bool isInstanced = true;
    size_t instanceIndex = SIZE_MAX;
    uint32_t originalAtomId = 0; // SURROUNDING 원자의 경우 원본 ID
    
    // Editing System (기존 유지)
    std::string tempSymbol;
    float tempPosition[3];
    float tempFracPosition[3];
    float tempRadius;
    bool modified = false;
    bool selected = false;
    
    // Validation
    bool isValid() const;
    void syncTempValues(); // temp → actual 값 동기화
};
```

### 2.3 BondManager 구축 (1주)

#### **Task 2.3.1**: 결합 관리 시스템 생성
```bash  
# 파일 생성 목록
src/domain/bonds/bond_manager.h
src/domain/bonds/bond_manager.cpp
src/domain/bonds/bond_info.h
src/domain/bonds/bond_info.cpp
src/domain/bonds/bond_calculator.h
src/domain/bonds/bond_calculator.cpp
```

**구현 가이드**:
```cpp
// bond_manager.h
class BondManager {
public:
    // Bond Creation - 기존 함수들을 여기로 이동
    uint32_t createBond(uint32_t atom1Id, uint32_t atom2Id, BondType type = BondType::ORIGINAL); // ← createBond()에서 이동
    bool removeBond(uint32_t bondId);
    void removeAllBonds(BondType type); // ← clearAllBonds()에서 로직 이동
    
    // Automatic Bond Generation - 기존 함수들을 여기로 이동  
    void createAllBonds(const AtomManager& atomManager); // ← createAllBonds()에서 이동
    void createBondsForAtoms(const std::vector<uint32_t>& atomIds, const AtomManager& atomManager); // ← createBondsForAtoms()에서 이동
    
    // Bond Validation - 기존 함수에서 이동
    bool shouldCreateBond(const AtomInfo& atom1, const AtomInfo& atom2) const; // ← shouldCreateBond()에서 이동
    
    // Bond Properties - 기존 함수들에서 이동
    void setBondThickness(float thickness); // ← updateAllBondGroupThickness() 관련 로직
    void setBondOpacity(float opacity); // ← updateAllBondGroupOpacity() 관련 로직
    void setBondScalingFactor(float factor);
    
    // Query Operations
    std::vector<BondInfo> getBonds(BondType type = BondType::ORIGINAL) const;
    std::vector<BondInfo> getBondsForAtom(uint32_t atomId) const;
    BondInfo* findBond(uint32_t bondId);
    
    // Statistics - 기존 함수에서 이동
    size_t getBondCount(BondType type = BondType::ORIGINAL) const; // ← getTotalBondCount()에서 이동  
    float calculateMemoryUsage() const; // ← calculateBondGroupMemory()에서 이동
    
private:
    std::vector<BondInfo> bonds;
    uint32_t nextBondId = 1;
    float bondScalingFactor = 1.0f;
    float bondThickness = 1.0f;
    float bondOpacity = 1.0f;
    
    uint32_t generateBondId(); // ← generateUniqueBondId()에서 이동
};
```

**이동할 기존 코드**:
- `createBond()` → `BondManager::createBond()`
- `shouldCreateBond()` → `BondManager::shouldCreateBond()`
- `createAllBonds()` → `BondManager::createAllBonds()`
- `createBondsForAtoms()` → `BondManager::createBondsForAtoms()`
- `clearAllBonds()` → `BondManager::removeAllBonds()`
- `getTotalBondCount()` → `BondManager::getBondCount()`
- `calculateBondGroupMemory()` → `BondManager::calculateMemoryUsage()`

#### **Task 2.3.2**: 결합 계산 로직 분리
```cpp
// bond_calculator.h - 수학적 계산 로직만 분리
class BondCalculator {
public:
    // 기존 함수들을 static 메서드로 이동
    static float calculateBondRadius(const AtomInfo& atom1, const AtomInfo& atom2); // ← calculateBondRadius()에서 이동
    static double calculateBondDistance(const AtomInfo& atom1, const AtomInfo& atom2);
    static bool isValidBondDistance(double distance, const AtomInfo& atom1, const AtomInfo& atom2, float scalingFactor);
    
    // Transform 계산 - 기존 함수에서 이동
    static BondTransformPair calculateBondTransforms(const AtomInfo& atom1, const AtomInfo& atom2); // ← calculateBondTransforms2Color()에서 이동
    
private:
    static bool getAtomPositionAndRadius(const AtomInfo& atom, double position[3], double& radius); // ← getAtomPositionAndRadius()에서 이동
};
```

### 2.4 CellManager 구축 (0.5주)

#### **Task 2.4.1**: 격자 관리 시스템 생성
```bash
# 파일 생성 목록
src/domain/cell/cell_manager.h
src/domain/cell/cell_manager.cpp
src/domain/cell/coordinate_converter.h
src/domain/cell/coordinate_converter.cpp
src/domain/cell/bravais_lattice.h
src/domain/cell/bravais_lattice.cpp
```

**구현 가이드**:
```cpp
// cell_manager.h
class CellManager {
public:
    // Cell Definition - 기존 데이터를 여기로 이동
    void setCellMatrix(const float matrix[3][3]);
    void getCellMatrix(float matrix[3][3]) const;
    bool isCellDefined() const; // ← cellEdgeActors.empty() 체크 로직을 여기로
    
    // Coordinate Conversion - 기존 전역 함수들을 여기로 이동
    void cartesianToFractional(const float cart[3], float frac[3]) const; // ← cartesianToFractional()에서 이동
    void fractionalToCartesian(const float frac[3], float cart[3]) const; // ← fractionalToCartesian()에서 이동
    
    // Boundary Detection - 기존 createSurroundingAtoms() 로직에서 분리
    bool isNearBoundary(const float fracCoords[3], float threshold = 0.1f) const;
    std::vector<std::array<float, 3>> getBoundaryTranslations(const float fracCoords[3], float threshold = 0.1f) const;
    
    // Cell Properties
    float getCellVolume() const;
    std::array<float, 3> getCellLengths() const;
    std::array<float, 3> getCellAngles() const;
    
    // Visualization
    void setVisible(bool visible); // ← cellVisible 변수 관리
    bool isVisible() const;
    
private:
    float cellMatrix[3][3];
    float invMatrix[3][3];
    bool matrixDefined = false;
    bool visible = false;
    
    void calculateInverseMatrix(); // ← calculateInverseMatrix()에서 이동
};
```

**이동할 기존 코드**:
- `calculateInverseMatrix()` 전역 함수 → `CellManager::calculateInverseMatrix()`
- `cartesianToFractional()` 전역 함수 → `CellManager::cartesianToFractional()`
- `fractionalToCartesian()` 전역 함수 → `CellManager::fractionalToCartesian()`
- `cellInfo` 전역 변수 → `CellManager` 멤버 변수들
- `cellVisible` 전역 변수 → `CellManager::visible`

### 2.5 Bravais Lattice 시스템 확장 (0.5주)

#### **Task 2.5.1**: atoms_template_bravais_lattice.cpp 통합
```cpp
// bravais_lattice.h - atoms_template_bravais_lattice.cpp의 로직을 클래스화
class BravaisLatticeTemplate {
public:
    static std::vector<BravaisLatticeTemplate> getAllTemplates();
    static BravaisLatticeTemplate getTemplate(BravaisLatticeType type); // ← renderCrystalTemplate() 로직에서 추출
    
    // Template properties - atoms_template_bravais_lattice.cpp의 latticeNames 배열 활용
    std::string getName() const;
    BravaisLatticeType getType() const;
    CrystalSystem getCrystalSystem() const;
    
    // Matrix calculation - atoms_template_bravais_lattice.cpp의 setBravaisLattice() 로직 활용
    void calculateCellMatrix(const LatticeParameters& params, float matrix[3][3]) const;
    bool validateParameters(const LatticeParameters& params) const;
    
    // Description - atoms_template_bravais_lattice.cpp의 TreeNode 설명들 활용
    std::string getDescription() const;
    
private:
    BravaisLatticeType latticeType;
    CrystalSystem crystalSystem;
    std::string name;
    std::string description;
};

// Lattice Parameters - atoms_template_bravais_lattice.cpp의 bravaisParams 배열을 구조체화
struct LatticeParameters {
    float a = 1.0f;
    float b = 1.0f; 
    float c = 1.0f;
    float alpha = 90.0f;
    float beta = 90.0f;
    float gamma = 90.0f;
    
    bool isValid() const;
};
```

---

## 🎮 **Phase 3: Application Layer (2주)**
**목표**: 고수준 애플리케이션 로직 및 컨트롤러 구성

### 3.1 AtomsTemplate 컨트롤러화 (1주)

#### **Task 3.1.1**: AtomsTemplate 슬림화
```cpp
// atoms_template.h (리팩토링 후)
class AtomsTemplate {
    DECLARE_SINGLETON(AtomsTemplate)
    
public:
    // Initialization
    bool initialize();
    void shutdown();
    
    // High-level Operations - 기존 고수준 함수들만 유지
    void loadStructure(const std::string& filepath); // ← LoadXSFFile()에서 이동
    void saveStructure(const std::string& filepath);
    void clearAll();
    
    // Atom Operations - Manager에 위임
    uint32_t addAtom(const std::string& symbol, const float position[3]);
    bool removeAtom(uint32_t atomId);
    void showSurroundingAtoms(bool show);
    
    // Bond Operations - Manager에 위임
    void createAllBonds(); // ← 기존 함수는 유지하되 BondManager에 위임
    void clearAllBonds(); // ← 기존 함수는 유지하되 BondManager에 위임
    void setBondProperties(float thickness, float opacity, float distanceFactor);
    
    // Cell Operations - Manager에 위임
    void setCellMatrix(const float matrix[3][3]);
    void showCell(bool show);
    
    // UI Integration
    void render(bool* openWindow = nullptr); // ← UI로 위임할 예정 (Phase 4)
    
    // Manager Access (UI에서 사용)
    const AtomManager& getAtomManager() const { return *atomManager; }
    const BondManager& getBondManager() const { return *bondManager; }
    const CellManager& getCellManager() const { return *cellManager; }
    const ElementDatabase& getElementDatabase() const { return *elementDB; }
    
    // Batch System Access - 기존 함수들을 여기서 위임  
    void beginBatch() { batchSystem->beginBatch(); }
    void endBatch() { batchSystem->endBatch(); }
    bool isBatchMode() const { return batchSystem->isBatchActive(); }
    BatchGuard createBatchGuard() { return batchSystem->createBatchGuard(); }
    
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
    
    bool initialized = false;
    
    // Component Integration
    void setupComponentConnections(); // Manager들 간의 연결 설정
    void updateFractionalCoordinates(); // 격자 변경 시 모든 원자 분수 좌표 재계산
};
```

**제거할 기존 함수들** (Manager로 이동):
- ✅ `createAtomSphere()` → `AtomManager::addAtom()`
- ✅ `createBond()` → `BondManager::createBond()`
- ✅ `shouldCreateBond()` → `BondManager::shouldCreateBond()` 
- ✅ `findAtomById()` → `AtomManager::findAtom()`
- ✅ `calculateBondRadius()` → `BondCalculator::calculateBondRadius()`
- ✅ `cartesianToFractional()` → `CellManager::cartesianToFractional()`

#### **Task 3.1.2**: Manager 간 통신 인터페이스 구축
```cpp
// atoms_template.cpp에서 Manager들 간의 연결 설정
void AtomsTemplate::setupComponentConnections() {
    // AtomManager가 원자 변경 시 BondManager에 알림
    atomManager->onAtomAdded = [this](uint32_t atomId) {
        // 새로운 원자와 기존 원자들 간의 결합 생성
        bondManager->createBondsForAtom(atomId, *atomManager);
        renderer->scheduleAtomUpdate(atomManager->findAtom(atomId)->symbol);
    };
    
    // BondManager가 결합 변경 시 VTKRenderer에 알림
    bondManager->onBondCreated = [this](uint32_t bondId) {
        auto bond = bondManager->findBond(bondId);
        renderer->scheduleBondUpdate(bond->bondGroupKey);
    };
    
    // CellManager가 격자 변경 시 모든 Manager에 알림
    cellManager->onCellChanged = [this]() {
        updateFractionalCoordinates();
        if (atomManager->areSurroundingAtomsVisible()) {
            atomManager->clearSurroundingAtoms();
            atomManager->createSurroundingAtoms(*cellManager);
        }
    };
}
```

### 3.2 워크플로우 및 서비스 레이어 (1주)

#### **Task 3.2.1**: 구조 로딩 워크플로우
```bash
# 파일 생성 목록  
src/application/workflows/structure_loader.h
src/application/workflows/structure_loader.cpp
src/application/services/atom_service.h
src/application/services/atom_service.cpp
```

**구현 가이드**:
```cpp
// structure_loader.h - 복잡한 로딩 프로세스를 워크플로우로 관리
class StructureLoader {
public:
    StructureLoader(FileIOManager& fileManager, AtomManager& atomManager, 
                   CellManager& cellManager, BondManager& bondManager);
    
    bool loadStructure(const std::string& filepath); // ← LoadXSFFile()에서 로직 이동
    
private:
    FileIOManager& fileManager;
    AtomManager& atomManager;
    CellManager& cellManager;
    BondManager& bondManager;
    
    bool validateStructureData(const StructureData& data);
    void clearExistingStructure(); // ← LoadXSFFile()의 정리 로직
    void createAtomsFromData(const StructureData& data); // ← InitializeAtomicStructure()에서 로직 이동
    void setupCellFromData(const StructureData& data);
    void generateBonds(); // ← createAllBonds() 호출
};
```

**이동할 기존 로직**:
- `LoadXSFFile()` → `StructureLoader::loadStructure()` (전체 워크플로우)
- `InitializeAtomicStructure()` → `StructureLoader::createAtomsFromData()` (일부 로직)

#### **Task 3.2.2**: 편집 시스템 서비스화
```cpp
// atom_service.h - 복잡한 원자 편집 로직을 서비스화
class AtomService {
public:
    AtomService(AtomManager& atomManager, BondManager& bondManager, 
               CellManager& cellManager, VTKRenderer& renderer);
    
    // 기존 applyAtomChanges() 로직을 여기로 이동
    void applyChanges(const std::vector<AtomEditRequest>& requests);
    void validateEdit(const AtomEditRequest& request);
    
private:
    AtomManager& atomManager;
    BondManager& bondManager;
    CellManager& cellManager;
    VTKRenderer& renderer;
    
    void handleSymbolChange(uint32_t atomId, const std::string& newSymbol);
    void handlePositionChange(uint32_t atomId, const float newPosition[3]);
    void regenerateBondsForAtom(uint32_t atomId); // ← applyAtomChanges()의 결합 재생성 로직
};
```

---

## 🖼️ **Phase 4: UI Layer (3-4주)**
**목표**: 사용자 인터페이스 완전 분리 및 확장 기능 추가

### 4.1 기본 UI 시스템 분리 (1주)

#### **Task 4.1.1**: AtomsTemplateUI 생성
```bash
# 파일 생성 목록
src/ui/atoms_template_ui.h
src/ui/atoms_template_ui.cpp
src/ui/components/ui_components.h
src/ui/components/ui_components.cpp
src/ui/components/atom_table.h  
src/ui/components/atom_table.cpp
```

**구현 가이드**:
```cpp
// atoms_template_ui.h  
class AtomsTemplateUI {
public:
    explicit AtomsTemplateUI(AtomsTemplate* controller);
    void render(bool* openWindow); // ← AtomsTemplate::Render()에서 이동
    
private:
    // 기존 UI 함수들을 여기로 이동
    void renderCreatedAtomsSection(); // ← AtomsTemplate::renderCreatedAtomsSection()에서 이동
    void renderBondsManagementTools(); // ← AtomsTemplate::renderBondsManagementTools()에서 이동
    void renderCellInformation(); // ← AtomsTemplate::renderCellInformation()에서 이동
    bool renderAtomTable(bool editMode, bool useFractionalCoords); // ← AtomsTemplate::renderAtomTable()에서 이동
    
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
```

**이동할 기존 UI 함수들**:
- `Render()` → `AtomsTemplateUI::render()`
- `renderCreatedAtomsSection()` → `AtomsTemplateUI::renderCreatedAtomsSection()`
- `renderBondsManagementTools()` → `AtomsTemplateUI::renderBondsManagementTools()`
- `renderCellInformation()` → `AtomsTemplateUI::renderCellInformation()`
- `renderAtomTable()` → `AtomsTemplateUI::renderAtomTable()`
- `applyAtomChanges()` → `AtomService::applyChanges()` (UI에서 서비스 호출)

### 4.2 확장 UI 컴포넌트 (atoms_template_periodic_table.cpp 통합) (1주)

#### **Task 4.2.1**: PeriodicTableWidget 생성
```bash
# 파일 생성 목록
src/ui/widgets/periodic_table_widget.h
src/ui/widgets/periodic_table_widget.cpp
```

**구현 가이드**:
```cpp
// periodic_table_widget.h - atoms_template_periodic_table.cpp의 renderPeriodicTable() 로직을 클래스화
class PeriodicTableWidget {
public:
    explicit PeriodicTableWidget(const ElementDatabase& elementDB, const CellManager& cellManager);
    
    void render(); // ← renderPeriodicTable()에서 이동
    
    // Selection management - atoms_template_periodic_table.cpp의 selectedElement 변수들
    void setSelectedElement(int elementIndex);
    int getSelectedElement() const;
    
    // Classification filtering - atoms_template_periodic_table.cpp의 category 변수
    void setClassificationFilter(ElementClassification filter);
    ElementClassification getClassificationFilter() const;
    
    // Callbacks
    std::function<void(const std::string& symbol, const float position[3])> onAtomAddRequested;
    
private:
    const ElementDatabase& elementDB;
    const CellManager& cellManager;
    int selectedElement = -1;
    ElementClassification classificationFilter = ElementClassification::All;
    
    // UI state - atoms_template_periodic_table.cpp의 static 변수들을 멤버로
    float atomPosition[3] = {0.0f, 0.0f, 0.0f};
    float fracPosition[3] = {0.0f, 0.0f, 0.0f};
    
    // Rendering helpers - atoms_template_periodic_table.cpp의 로직들을 메서드화
    void renderMainPeriodicTable();
    void renderLanthanides();
    void renderActinides();
    void renderElement(const Element& element, int elementIndex, const ImVec2& size);
    void renderElementTooltip(const Element& element);
    void renderClassificationFilter(); // ← category 선택 ComboBox
    void renderPositionInput(); // ← atomPosition/fracPosition 입력
    void renderSelectedElementInfo();
    
    // Style management - atoms_template_periodic_table.cpp의 스타일 관리 개선
    void setupElementButtonStyle(const Element& element, bool isSelected);
    ImVec4 calculateHoveredColor(const ImVec4& baseColor);
    ImVec4 calculateActiveColor(const ImVec4& baseColor);
};
```

**이동할 atoms_template_periodic_table.cpp 코드**:
- `renderPeriodicTable()` 전체 함수 → `PeriodicTableWidget::render()`
- `selectedElement` static 변수 → `PeriodicTableWidget::selectedElement` 멤버
- `category` static 변수 → `PeriodicTableWidget::classificationFilter` 멤버
- `atomPosition` static 배열 → `PeriodicTableWidget::atomPosition` 멤버
- 주기율표 렌더링 루프 → `PeriodicTableWidget::renderMainPeriodicTable()` 등

#### **Task 4.2.2**: 현재 버전 API 호환성 업데이트
```cpp
// periodic_table_widget.cpp - 현재 AtomsTemplate API와 연동
void PeriodicTableWidget::render() {
    // ... 기존 UI 렌더링 로직 ...
    
    if (ImGui::Button("Add to Structure")) {
        if (onAtomAddRequested && selectedElement >= 0) {
            const Element* element = elementDB.getElement(selectedElement);
            if (element) {
                float finalPosition[3];
                
                if (useFractionalCoords && cellManager.isCellDefined()) {
                    cellManager.fractionalToCartesian(fracPosition, finalPosition);
                } else {
                    finalPosition[0] = atomPosition[0];
                    finalPosition[1] = atomPosition[1]; 
                    finalPosition[2] = atomPosition[2];
                }
                
                // 콜백을 통해 AtomsTemplate에 원자 추가 요청
                onAtomAddRequested(element->symbol, finalPosition);
            }
        }
    }
}
```

### 4.3 Bravais Lattice Widget (atoms_template_bravais_lattice.cpp 통합) (1주)

#### **Task 4.3.1**: BravaisLatticeWidget 생성
```bash
# 파일 생성 목록
src/ui/widgets/bravais_lattice_widget.h
src/ui/widgets/bravais_lattice_widget.cpp
```

**구현 가이드**:
```cpp
// bravais_lattice_widget.h - atoms_template_bravais_lattice.cpp의 renderCrystalTemplate() 로직을 클래스화
class BravaisLatticeWidget {
public:
    explicit BravaisLatticeWidget(CellManager& cellManager);
    
    void render(); // ← renderCrystalTemplate()에서 이동
    
    // Lattice management
    void setBravaisLattice(BravaisLatticeType type); // ← setBravaisLattice()에서 이동
    BravaisLatticeType getSelectedLatticeType() const;
    
    // Crystal system filtering - atoms_template_bravais_lattice.cpp의 필터 체크박스들
    void setCrystalSystemFilter(const std::set<CrystalSystem>& systems);
    std::set<CrystalSystem> getCrystalSystemFilter() const;
    
    // Callbacks
    std::function<void(const float matrix[3][3])> onLatticeApplied;
    
private:
    CellManager& cellManager;
    int selectedBravaisType = -1; // ← selectedBravaisType static 변수를 멤버로
    std::array<LatticeParameters, 14> bravaisParams; // ← bravaisParams 배열을 멤버로
    std::set<CrystalSystem> activeFilters;
    
    // UI layout - atoms_template_bravais_lattice.cpp의 레이아웃 변수들
    float buttonWidth = 0.25f;
    float paramWidth = 0.65f;
    float inputWidth = 60.0f;
    
    // Rendering components - atoms_template_bravais_lattice.cpp의 로직들을 메서드화
    void renderCrystalSystemFilters(); // ← 필터 체크박스들
    void renderLatticeList(); // ← BravaisLatticeTable 
    void renderLatticeParameters(int latticeIndex); // ← switch-case 매개변수 입력들
    void renderLatticeDescription(); // ← TreeNode 설명
    
    // Parameter input helpers - atoms_template_bravais_lattice.cpp의 각 격자별 입력 로직
    void renderCubicParameters(int latticeIndex);
    void renderTetragonalParameters(int latticeIndex);
    void renderOrthorhombicParameters(int latticeIndex);
    void renderMonoclinicParameters(int latticeIndex);
    void renderTriclinicParameters(int latticeIndex);
    void renderRhombohedralParameters(int latticeIndex);
    void renderHexagonalParameters(int latticeIndex);
    
    // Utility functions
    void calculateCellMatrix(BravaisLatticeType type, const LatticeParameters& params, float matrix[3][3]);
    bool isLatticeVisible(int latticeIndex) const; // ← latticeVisible 배열 로직
    std::string getLatticeDescription(BravaisLatticeType type) const;
};
```

**이동할 atoms_template_bravais_lattice.cpp 코드**:
- `renderCrystalTemplate()` 전체 함수 → `BravaisLatticeWidget::render()`
- `selectedBravaisType` static 변수 → `BravaisLatticeWidget::selectedBravaisType` 멤버
- `bravaisParams` static 배열 → `BravaisLatticeWidget::bravaisParams` 멤버
- `setBravaisLattice()` 함수 → `BravaisLatticeWidget::setBravaisLattice()` 
- 각 격자별 매개변수 입력 switch-case → 개별 render 메서드들

### 4.4 UI 통합 및 탭 시스템 (1주)

#### **Task 4.4.1**: TabSystem 구현
```bash
# 파일 생성 목록
src/ui/layouts/tab_system.h
src/ui/layouts/tab_system.cpp
```

**구현 가이드**:
```cpp
// tab_system.h
class TabSystem {
public:
    enum class Tab {
        Atoms = 0,
        Bonds = 1,
        Cell = 2,
        PeriodicTable = 3,        // atoms_template_periodic_table.cpp 기능
        BravaisLattice = 4,       // atoms_template_bravais_lattice.cpp 기능
        Analysis = 5,
        Settings = 6
    };
    
    void render(AtomsTemplate* controller);
    void setActiveTab(Tab tab);
    Tab getActiveTab() const;
    
private:
    Tab activeTab = Tab::Atoms;
    
    void renderTabBar();
    void renderTabContent(Tab tab, AtomsTemplate* controller);
};
```

#### **Task 4.4.2**: AtomsTemplateUI 최종 통합
```cpp
// atoms_template_ui.cpp - 모든 UI 컴포넌트 통합
void AtomsTemplateUI::render(bool* openWindow) {
    if (openWindow != nullptr) {
        if (!*openWindow) return;
        
        if (ImGui::Begin("Atomistic model builder", openWindow)) {
            // 탭 시스템 사용
            if (ImGui::BeginTabBar("MainTabs")) {
                
                if (ImGui::BeginTabItem("Atoms")) {
                    renderCreatedAtomsSection(); // 기존 함수
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Periodic Table")) {
                    if (!periodicTableWidget) {
                        periodicTableWidget = std::make_unique<PeriodicTableWidget>(
                            controller->getElementDatabase(), 
                            controller->getCellManager()
                        );
                        // 콜백 설정
                        periodicTableWidget->onAtomAddRequested = [this](const std::string& symbol, const float position[3]) {
                            controller->addAtom(symbol, position);
                        };
                    }
                    periodicTableWidget->render();
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Bravais Lattice")) {
                    if (!bravaisLatticeWidget) {
                        bravaisLatticeWidget = std::make_unique<BravaisLatticeWidget>(
                            controller->getCellManager()
                        );
                        // 콜백 설정
                        bravaisLatticeWidget->onLatticeApplied = [this](const float matrix[3][3]) {
                            controller->setCellMatrix(matrix);
                        };
                    }
                    bravaisLatticeWidget->render();
                    ImGui::EndTabItem();
                }
                
                // 다른 탭들...
                
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}
```

---

## 🧪 **테스트 및 검증 (각 Phase별 병행)**

### **단위 테스트 작성 가이드**
```cpp
// tests/unit/domain/atom_manager_test.cpp 예시
#include <gtest/gtest.h>
#include "domain/atoms/atom_manager.h"

class AtomManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        atomManager = std::make_unique<AtomManager>();
    }
    
    std::unique_ptr<AtomManager> atomManager;
};

TEST_F(AtomManagerTest, AddAtom_ValidRequest_ReturnsNonZeroId) {
    AtomCreateRequest request;
    request.symbol = "C";
    request.position = {0.0f, 0.0f, 0.0f};
    request.radius = 0.76f;
    
    uint32_t atomId = atomManager->addAtom(request);
    
    EXPECT_NE(atomId, 0);
    EXPECT_EQ(atomManager->getAtomCount(), 1);
    
    const AtomInfo* atom = atomManager->findAtom(atomId);
    ASSERT_NE(atom, nullptr);
    EXPECT_EQ(atom->symbol, "C");
}

TEST_F(AtomManagerTest, RemoveAtom_ExistingId_RemovesSuccessfully) {
    AtomCreateRequest request;
    request.symbol = "H";
    request.position = {1.0f, 0.0f, 0.0f};
    
    uint32_t atomId = atomManager->addAtom(request);
    EXPECT_TRUE(atomManager->removeAtom(atomId));
    EXPECT_EQ(atomManager->getAtomCount(), 0);
    EXPECT_EQ(atomManager->findAtom(atomId), nullptr);
}
```

### **통합 테스트 가이드**
```cpp
// tests/integration/structure_loading_test.cpp 예시
TEST(StructureLoadingTest, LoadXSFFile_ValidFile_LoadsCorrectly) {
    AtomsTemplate atomsTemplate;
    atomsTemplate.initialize();
    
    std::string testFile = "tests/fixtures/test_structures/simple_cubic.xsf";
    atomsTemplate.loadStructure(testFile);
    
    EXPECT_GT(atomsTemplate.getAtomManager().getAtomCount(), 0);
    EXPECT_TRUE(atomsTemplate.getCellManager().isCellDefined());
    
    // 구조가 올바르게 로드되었는지 검증
    auto atoms = atomsTemplate.getAtomManager().getAtoms();
    EXPECT_FALSE(atoms.empty());
}
```

---

## 📋 **Phase별 완료 체크리스트**

### **Phase 1 완료 체크리스트** ✅
- [ ] VTKRenderer 클래스 생성 및 기본 기능 구현
- [ ] AtomGroupRenderer, BondGroupRenderer, CellRenderer 분리
- [ ] 전역 VTK 함수들을 RenderingUtils로 이동
- [ ] FileIOManager 및 XSFReader 구현 
- [ ] BatchUpdateSystem 독립 클래스화
- [ ] 기존 코드에서 새로운 클래스들로의 호출 변경
- [ ] Phase 1 단위 테스트 통과

### **Phase 2 완료 체크리스트** ✅  
- [ ] ElementDatabase 싱글톤 클래스 구현
- [ ] 전역 원소 데이터를 ElementDatabase로 이전
- [ ] AtomManager 클래스 구현 및 기존 원자 관련 함수들 이동
- [ ] BondManager 클래스 구현 및 기존 결합 관련 함수들 이동
- [ ] CellManager 클래스 구현 및 기존 격자 관련 함수들 이동
- [ ] BravaisLatticeTemplate 시스템 구현
- [ ] 좌표 변환 함수들을 적절한 클래스로 이동
- [ ] Phase 2 단위 테스트 통과

### **Phase 3 완료 체크리스트** ✅
- [ ] AtomsTemplate을 경량 컨트롤러로 변환
- [ ] Manager들 간의 통신 인터페이스 구현
- [ ] StructureLoader 워크플로우 클래스 구현
- [ ] AtomService 등 서비스 레이어 구현
- [ ] 기존 고수준 함수들의 Manager 위임 구조 구현
- [ ] Phase 3 통합 테스트 통과

### **Phase 4 완료 체크리스트** ✅
- [ ] AtomsTemplateUI 클래스 구현
- [ ] 기존 UI 함수들을 AtomsTemplateUI로 이동
- [ ] PeriodicTableWidget 구현 (atoms_template_periodic_table.cpp 통합)
- [ ] BravaisLatticeWidget 구현 (atoms_template_bravais_lattice.cpp 통합)
- [ ] TabSystem 구현 및 UI 컴포넌트 통합
- [ ] UI와 비즈니스 로직 완전 분리 검증
- [ ] 확장 UI 기능 동작 확인

---

## 🔧 **개발 환경 및 도구**

### **권장 개발 도구**
- **IDE**: Visual Studio, CLion, 또는 VSCode
- **빌드 시스템**: CMake 3.16+
- **테스트 프레임워크**: Google Test
- **문서 생성**: Doxygen
- **코드 포맷팅**: clang-format
- **정적 분석**: clang-tidy, PVS-Studio

### **의존성 라이브러리**
```cmake
# CMakeLists.txt에서 관리할 의존성들
find_package(VTK REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(IMGUI REQUIRED imgui)
find_package(spdlog REQUIRED)

# 새로 추가될 의존성 (선택사항)
find_package(Catch2 REQUIRED) # 또는 Google Test
find_package(Boost REQUIRED)  # 유틸리티용
```

### **코딩 표준**
- **네이밍**: PascalCase (클래스), camelCase (함수/변수), UPPER_CASE (상수)
- **파일명**: snake_case.h/cpp
- **헤더 가드**: `#pragma once` 사용
- **문서화**: 모든 public 함수에 Doxygen 주석
- **예외 처리**: RAII 패턴 적극 활용

---

## 📈 **성공 지표 및 검증 방법**

### **코드 품질 지표**
- **복잡도 감소**: McCabe 복잡도 10 이하 유지
- **파일 크기**: 평균 파일당 300 라인 이하
- **함수 크기**: 함수당 50 라인 이하 권장
- **테스트 커버리지**: 80% 이상 달성

### **성능 지표**
- **메모리 사용량**: 현재 대비 30% 이상 최적화
- **렌더링 성능**: 배치 업데이트로 50% 이상 향상
- **로딩 시간**: 대용량 구조 파일 로딩 속도 유지

### **사용성 지표**
- **기능 추가 시간**: 새로운 파일 형식 지원 1일 내 구현 가능
- **버그 수정 시간**: 격리된 버그 1시간 내 수정 가능
- **UI 확장성**: 새로운 UI 컴포넌트 2시간 내 추가 가능

---

## 🎯 **최종 완료 후 기대 효과**

### **개발자 경험 개선**
- ✅ **명확한 책임 분리**: 각 클래스의 역할이 명확
- ✅ **독립적 개발**: 여러 개발자가 동시에 다른 모듈 작업 가능
- ✅ **쉬운 테스트**: 각 컴포넌트 단위별 테스트 가능
- ✅ **빠른 디버깅**: 문제 발생 시 영향 범위 명확

### **사용자 경험 개선** 
- ✅ **향상된 UI**: 주기율표, Bravais 격자 위젯 등 전문 기능
- ✅ **더 나은 성능**: 배치 업데이트로 렌더링 최적화
- ✅ **확장된 기능**: 다양한 파일 형식 지원
- ✅ **안정성 증대**: 체계적인 테스트로 버그 감소

### **장기적 유지보수성**
- ✅ **확장 가능한 아키텍처**: 새로운 기능 추가 시 기존 코드 영향 최소화
- ✅ **모듈화된 구조**: 특정 기능 변경 시 해당 모듈만 수정
- ✅ **표준화된 인터페이스**: 일관된 API 설계로 학습 비용 절감
- ✅ **문서화 완비**: 각 컴포넌트별 명확한 사용법 및 예제

---

## 🚀 **실제 구현 시 주의사항**

### **마이그레이션 중 호환성 유지**
```cpp
// migration/legacy_compatibility.h - 기존 코드 호환성 유지
class LegacyCompatibilityLayer {
public:
    // 기존 전역 함수들을 임시로 유지 (deprecated 마킹)
    [[deprecated("Use ElementDatabase::getInstance().getElementColor() instead")]]
    static ImVec4 findElementColor(const char* symbol) {
        return ElementDatabase::getInstance().getElementColor(std::string(symbol));
    }
    
    [[deprecated("Use CellManager::cartesianToFractional() instead")]]
    static void cartesianToFractional(const float cartesian[3], float fractional[3], const float invmatrix[3][3]) {
        // CellManager로 위임하되 경고 로그 출력
        SPDLOG_WARN("Using deprecated cartesianToFractional function. Please update to use CellManager.");
        // 임시 구현...
    }
};
```

### **메모리 관리 및 성능 최적화**
```cpp
// infrastructure/memory/smart_pointer_utils.h - 스마트 포인터 관리
template<typename T>
class ComponentManager {
public:
    template<typename... Args>
    std::shared_ptr<T> create(Args&&... args) {
        auto component = std::make_shared<T>(std::forward<Args>(args)...);
        activeComponents.push_back(component);
        return component;
    }
    
    void cleanup() {
        // 약한 참조들 정리
        activeComponents.erase(
            std::remove_if(activeComponents.begin(), activeComponents.end(),
                [](const std::weak_ptr<T>& ptr) { return ptr.expired(); }),
            activeComponents.end()
        );
    }
    
private:
    std::vector<std::weak_ptr<T>> activeComponents;
};
```

### **에러 처리 및 복구 전략**
```cpp
// utils/error_handling.h - 체계적인 에러 처리
enum class ErrorCode {
    Success = 0,
    FileNotFound,
    InvalidFormat,
    MemoryAllocation,
    VTKInitialization,
    InvalidAtomData,
    BondCreationFailed
};

class Result {
public:
    template<typename T>
    static Result success(T&& value) {
        Result result;
        result.errorCode = ErrorCode::Success;
        result.data = std::forward<T>(value);
        return result;
    }
    
    static Result failure(ErrorCode code, const std::string& message) {
        Result result;
        result.errorCode = code;
        result.errorMessage = message;
        return result;
    }
    
    bool isSuccess() const { return errorCode == ErrorCode::Success; }
    ErrorCode getErrorCode() const { return errorCode; }
    const std::string& getErrorMessage() const { return errorMessage; }
    
private:
    ErrorCode errorCode;
    std::string errorMessage;
    std::any data; // C++17의 std::any 사용
};

// 사용 예시
Result loadStructureFile(const std::string& filepath) {
    try {
        StructureData data;
        if (!FileIOManager::loadStructure(filepath, data)) {
            return Result::failure(ErrorCode::InvalidFormat, "Failed to parse structure file");
        }
        return Result::success(std::move(data));
    } catch (const std::exception& e) {
        return Result::failure(ErrorCode::FileNotFound, e.what());
    }
}
```

---

## 📚 **Phase별 상세 구현 가이드**

### **Phase 1 상세 구현 순서**

#### **Week 1: VTK 렌더링 시스템**
**Day 1-2**: 기본 인프라 구조
```bash
# 작업 순서
1. src/infrastructure/rendering/ 디렉토리 생성
2. vtk_renderer.h 헤더 정의 (인터페이스만)
3. atom_group_renderer.h, bond_group_renderer.h 헤더 정의
4. CMakeLists.txt에 새로운 소스 파일들 추가
5. 컴파일 오류 해결을 위한 최소 구현
```

**Day 3-4**: VTK 함수 이동
```cpp
// 이동 우선순위 (의존성 순서)
1. createBaseBondGeometry() → RenderingUtils::createBaseBondGeometry()
2. setupBondActorProperties() → RenderingUtils::setupBondActorProperties()
3. generateBondTypeKey() → RenderingUtils::generateBondTypeKey()
4. updateBondGroup() → BondGroupRenderer::update()
5. updateUnifiedAtomGroupVTK() → AtomGroupRenderer::update()
```

**Day 5**: 통합 테스트 및 버그 수정

#### **Week 2: 파일 I/O 시스템**
**Day 1-2**: 기본 구조 및 인터페이스
```cpp
// 구현 순서
1. FileReader 추상 클래스 정의
2. StructureData 구조체 설계
3. XSFReader 기본 구현 (ParseXSFFile 로직 이동)
4. FileIOManager Strategy 패턴 구현
```

**Day 3-4**: XSF 파일 파싱 로직 이동
```cpp
// ParseXSFFile() → XSFReader::read() 이동 시 고려사항
1. 전역 변수 접근을 매개변수로 변환
2. 에러 처리를 Result 패턴으로 개선
3. AtomData 구조체를 StructureData 내부로 이동
4. 파일 경로 유효성 검사 강화
```

**Day 5**: 확장 파일 형식 준비 및 테스트

### **Phase 2 상세 구현 순서**

#### **Week 1: ElementDatabase 구축**
**Day 1-2**: 기본 데이터베이스 구조
```cpp
// 구현 단계
1. Element 구조체 재정의 (확장 필드 추가)
2. ElementDatabase 싱글톤 클래스 기본 구조
3. 기존 전역 배열 데이터를 초기화 함수로 이동
4. 기본 쿼리 함수들 구현 (getElement, getElementColor 등)
```

**Day 3-4**: 확장 기능 및 분류 시스템
```cpp
// atoms_template_periodic_table.cpp 분석 기반 구현
1. ElementClassification enum 정의
2. 분류별 원소 필터링 함수 구현
3. 확장 원소 속성 (전기음성도, 산화수 등) 추가
4. 색상 스킴 시스템 구현 (Jmol, CPK 등)
```

**Day 5**: 기존 코드에서 ElementDatabase 사용으로 변경

#### **Week 2-3: Manager 클래스들 구축**
**AtomManager 구현 (Week 2)**:
```cpp
// Day 1-2: 기본 CRUD 연산
1. AtomInfo 구조체 정리 및 검증 함수 추가
2. addAtom(), removeAtom(), findAtom() 구현
3. createAtomSphere() 로직을 addAtom()으로 변환
4. 기존 전역 원자 벡터를 AtomManager 멤버로 이동

// Day 3-4: 주변 원자 시스템
1. SurroundingAtoms 클래스 분리 검토
2. createSurroundingAtoms() 로직을 AtomManager로 이동
3. 경계 감지 알고리즘 개선
4. 원자 그룹별 메모리 사용량 계산 이동

// Day 5: 통합 테스트 및 성능 검증
```

**BondManager 구현 (Week 3)**:
```cpp
// Day 1-2: 결합 생성 로직
1. BondInfo 구조체 정리
2. createBond(), shouldCreateBond() 로직 이동
3. 결합 유효성 검사 알고리즘 개선
4. 결합 그룹 관리 시스템 통합

// Day 3-4: 자동 결합 생성
1. createAllBonds() 로직을 BondManager로 이동
2. 공간 격자 최적화 알고리즘 개선
3. 배치 처리 시스템과의 통합
4. 결합 속성 관리 (두께, 투명도 등)

// Day 5: 성능 최적화 및 메모리 누수 검사
```

### **Phase 3 상세 구현 순서**

#### **Week 1: AtomsTemplate 컨트롤러화**
**Day 1-3**: Manager 통합 및 의존성 주입
```cpp
// 구현 순서
1. AtomsTemplate 생성자에서 Manager 인스턴스 생성
2. 기존 함수들을 Manager 위임으로 변경
3. Manager 간 통신 인터페이스 구현
4. setupComponentConnections() 메서드 구현
```

**Day 4-5**: 고수준 워크플로우 구현
```cpp
// StructureLoader 구현
1. LoadXSFFile() 로직을 단계별로 분해
2. 오류 처리 및 롤백 시스템 구현
3. 진행 상황 콜백 시스템 추가
4. 대용량 파일 처리 최적화
```

#### **Week 2: 서비스 레이어 구현**
**Day 1-3**: AtomService 구현
```cpp
// applyAtomChanges() 로직 서비스화
1. 편집 요청 검증 시스템
2. 원자 속성 변경 처리 (기호, 위치, 반지름)
3. 결합 재생성 최적화
4. 주변 원자 동기화 시스템
```

**Day 4-5**: 명령 패턴 및 실행 취소 시스템
```cpp
// Command Pattern 구현
1. ICommand 인터페이스 정의
2. AtomCommands (AddAtom, RemoveAtom, UpdateAtom)
3. BondCommands (CreateBond, RemoveBond)
4. CommandManager (실행, 취소, 재실행)
```

### **Phase 4 상세 구현 순서**

#### **Week 1: 기본 UI 분리**
**Day 1-3**: AtomsTemplateUI 구현
```cpp
// UI 함수 이동 순서
1. Render() → AtomsTemplateUI::render()
2. renderCreatedAtomsSection() 이동
3. renderBondsManagementTools() 이동
4. renderCellInformation() 이동
5. 상태 관리 시스템 구현 (UIState)
```

**Day 4-5**: UI 컴포넌트 모듈화
```cpp
// UIComponents 네임스페이스 구현
1. AtomTable 컴포넌트 분리
2. BondControls 컴포넌트 분리
3. CellEditor 컴포넌트 분리
4. 재사용 가능한 UI 유틸리티 함수들
```

#### **Week 2-3: 확장 UI 위젯**
**PeriodicTableWidget 구현 (Week 2)**:
```cpp
// atoms_template_periodic_table.cpp 통합
Day 1-2: 기본 구조
1. renderPeriodicTable() 로직을 render() 메서드로 이동
2. static 변수들을 멤버 변수로 변환
3. 원소 선택 및 필터링 시스템 구현

Day 3-4: 확장 기능
1. 분류별 필터링 개선
2. 좌표 입력 시스템 (직교/분수)
3. ElementDatabase와의 통합
4. 현재 API 호환성 콜백 구현

Day 5: 스타일 개선 및 사용성 향상
1. ImGui 스타일 스택 관리 개선
2. 툴팁 시스템 개선
3. 반응형 레이아웃 구현
```

**BravaisLatticeWidget 구현 (Week 3)**:
```cpp
// atoms_template_bravais_lattice.cpp 통합
Day 1-2: 기본 구조
1. renderCrystalTemplate() 로직을 render() 메서드로 이동
2. 격자 매개변수 관리 시스템
3. 결정계별 필터링 구현

Day 3-4: 격자 계산 통합
1. setBravaisLattice() 로직 이동
2. BravaisLatticeTemplate과의 통합
3. 실시간 매개변수 검증
4. 배치 업데이트 시스템 통합

Day 5: UI 개선 및 검증
1. 매개변수 입력 UI 개선
2. 격자 설명 시스템 구현
3. 에러 처리 및 사용자 피드백
```

#### **Week 4: UI 통합 및 마무리**
**Day 1-3**: TabSystem 및 레이아웃
```cpp
// 통합 탭 시스템
1. TabSystem 클래스 구현
2. 동적 탭 추가/제거 시스템
3. 탭별 상태 관리
4. 키보드 단축키 지원
```

**Day 4-5**: 최종 통합 및 테스트
```cpp
// 전체 UI 시스템 검증
1. 모든 UI 컴포넌트 통합 테스트
2. 메모리 누수 검사
3. 성능 최적화 (렌더링 속도)
4. 사용성 테스트 및 개선
```

---

## 🧪 **테스트 전략 상세 가이드**

### **단위 테스트 구현 가이드**

#### **Mock 객체 사용 예시**
```cpp
// tests/mocks/mock_vtk_renderer.h
class MockVTKRenderer : public VTKRenderer {
public:
    MOCK_METHOD(void, addAtomGroup, (const std::string& symbol, const AtomGroupData& data));
    MOCK_METHOD(void, updateAtomGroup, (const std::string& symbol));
    MOCK_METHOD(void, removeAtomGroup, (const std::string& symbol));
    MOCK_METHOD(void, render, ());
};

// tests/unit/domain/atom_manager_test.cpp
class AtomManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockRenderer = std::make_shared<MockVTKRenderer>();
        atomManager = std::make_unique<AtomManager>();
        atomManager->setRenderer(mockRenderer);
    }
    
    std::shared_ptr<MockVTKRenderer> mockRenderer;
    std::unique_ptr<AtomManager> atomManager;
};

TEST_F(AtomManagerTest, AddAtom_CallsRendererUpdate) {
    EXPECT_CALL(*mockRenderer, addAtomGroup(::testing::_, ::testing::_))
        .Times(1);
    
    AtomCreateRequest request{"C", {0, 0, 0}, 0.76f};
    uint32_t atomId = atomManager->addAtom(request);
    
    EXPECT_NE(atomId, 0);
}
```

#### **통합 테스트 시나리오**
```cpp
// tests/integration/full_workflow_test.cpp
TEST(FullWorkflowTest, CompleteStructureWorkflow) {
    // Given: 빈 AtomsTemplate
    AtomsTemplate atomsTemplate;
    atomsTemplate.initialize();
    
    // When: XSF 파일 로드
    std::string testFile = "tests/fixtures/diamond.xsf";
    ASSERT_TRUE(atomsTemplate.loadStructure(testFile));
    
    // Then: 구조가 올바르게 로드됨
    EXPECT_EQ(atomsTemplate.getAtomManager().getAtomCount(), 8); // 다이아몬드 구조
    EXPECT_TRUE(atomsTemplate.getCellManager().isCellDefined());
    
    // When: 결합 생성
    atomsTemplate.createAllBonds();
    
    // Then: 적절한 결합 수 생성
    EXPECT_GT(atomsTemplate.getBondManager().getBondCount(), 0);
    
    // When: 주변 원자 표시
    atomsTemplate.showSurroundingAtoms(true);
    
    // Then: 주변 원자들 생성됨
    EXPECT_GT(atomsTemplate.getAtomManager().getAtomCount(AtomType::SURROUNDING), 0);
    
    // When: 새로운 원자 추가
    uint32_t newAtomId = atomsTemplate.addAtom("H", {5.0f, 5.0f, 5.0f});
    
    // Then: 원자가 추가되고 결합이 업데이트됨
    EXPECT_NE(newAtomId, 0);
    auto newBondCount = atomsTemplate.getBondManager().getBondCount();
    // 새로운 결합들이 생성되었는지 확인 (거리에 따라)
    
    // When: 구조 저장 (확장 기능)
    std::string outputFile = "tests/output/modified_diamond.xsf";
    EXPECT_TRUE(atomsTemplate.saveStructure(outputFile));
}
```

### **성능 테스트 가이드**
```cpp
// tests/performance/large_structure_test.cpp
TEST(PerformanceTest, LargeStructureHandling) {
    const int LARGE_ATOM_COUNT = 10000;
    
    AtomsTemplate atomsTemplate;
    atomsTemplate.initialize();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 대량의 원자 추가
    {
        auto batchGuard = atomsTemplate.createBatchGuard(); // 배치 모드 사용
        
        for (int i = 0; i < LARGE_ATOM_COUNT; ++i) {
            float x = static_cast<float>(i % 100);
            float y = static_cast<float>((i / 100) % 100);
            float z = static_cast<float>(i / 10000);
            
            atomsTemplate.addAtom("C", {x, y, z});
        }
    } // 배치 모드 종료 시 일괄 업데이트
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(atomsTemplate.getAtomManager().getAtomCount(), LARGE_ATOM_COUNT);
    EXPECT_LT(duration.count(), 5000); // 5초 이내 완료
    
    // 메모리 사용량 검사
    float memoryUsage = atomsTemplate.getAtomManager().calculateMemoryUsage();
    EXPECT_LT(memoryUsage, 100.0f); // 100MB 이하
    
    // 결합 생성 성능 테스트
    startTime = std::chrono::high_resolution_clock::now();
    atomsTemplate.createAllBonds();
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_LT(duration.count(), 10000); // 10초 이내 완료
}
```

---

## 📖 **문서화 가이드라인**

### **코드 문서화 표준**
```cpp
/**
 * @brief 원자를 구조에 추가합니다.
 * 
 * 이 함수는 새로운 원자를 생성하여 현재 구조에 추가합니다.
 * 원자는 자동으로 고유 ID를 할당받으며, 필요시 다른 원자들과의
 * 결합이 자동으로 생성됩니다.
 * 
 * @param request 생성할 원자의 정보가 담긴 요청 객체
 * @return 생성된 원자의 고유 ID (0이면 실패)
 * 
 * @throws std::invalid_argument 잘못된 원소 기호나 위치가 주어진 경우
 * @throws std::runtime_error VTK 렌더링 오류가 발생한 경우
 * 
 * @example
 * ```cpp
 * AtomCreateRequest request;
 * request.symbol = "C";
 * request.position = {0.0f, 0.0f, 0.0f};
 * request.radius = 0.76f;
 * 
 * uint32_t atomId = atomManager.addAtom(request);
 * if (atomId != 0) {
 *     std::cout << "원자가 성공적으로 생성되었습니다. ID: " << atomId << std::endl;
 * }
 * ```
 * 
 * @see removeAtom(), findAtom(), AtomCreateRequest
 * @since 2.0.0
 * @author 개발자명
 */
uint32_t addAtom(const AtomCreateRequest& request);
```

### **README 및 사용자 가이드**
```markdown
# Atomistic Model Builder - 리팩토링된 버전

## 📋 개요
Atomistic Model Builder는 원자 구조를 시각화하고 편집할 수 있는
고성능 C++ 애플리케이션입니다. 이 버전은 확장성과 유지보수성을
크게 향상시킨 완전히 리팩토링된 아키텍처를 특징으로 합니다.

## 🏗️ 아키텍처 개요
```
UI Layer          ← 사용자 인터페이스 (ImGui)
Application Layer ← 비즈니스 로직 조정
Domain Layer      ← 핵심 도메인 모델 (원자, 결합, 격자)
Infrastructure    ← VTK 렌더링, 파일 I/O
```

## 🚀 주요 기능
- **다중 파일 형식 지원**: XSF, CIF, POSCAR (확장 가능)
- **대화형 주기율표**: 원소 선택 및 분류별 필터링
- **Bravais 격자 템플릿**: 14가지 격자 구조 지원
- **실시간 결합 생성**: 지능적인 결합 감지 알고리즘
- **배치 업데이트 시스템**: 대용량 구조 최적화 처리
- **확장 가능한 UI**: 모듈형 위젯 시스템

## 💻 빠른 시작
\```cpp
#include "atoms_template.h"

int main() {
    // 1. 애플리케이션 초기화
    AtomsTemplate& app = AtomsTemplate::getInstance();
    app.initialize();
    
    // 2. 구조 파일 로드
    app.loadStructure("example.xsf");
    
    // 3. 결합 생성
    app.createAllBonds();
    
    // 4. UI 실행
    bool isOpen = true;
    while (isOpen) {
        app.render(&isOpen);
    }
    
    return 0;
}
\```

## 🔧 개발자 가이드

### 새로운 파일 형식 추가
\```cpp
class MyFormatReader : public FileReader {
public:
    bool read(const std::string& filepath, StructureData& data) override {
        // 파일 파싱 로직 구현
        return true;
    }
    
    std::string getFormatName() const override {
        return "My Custom Format";
    }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {".myformat"};
    }
};

// 등록
fileManager.registerReader(".myformat", std::make_unique<MyFormatReader>());
\```

### 새로운 UI 위젯 추가
\```cpp
class MyCustomWidget {
public:
    void render() {
        if (ImGui::Begin("My Custom Widget")) {
            // 위젯 UI 로직
        }
        ImGui::End();
    }
};

// UI에 통합
void AtomsTemplateUI::render(bool* openWindow) {
    if (ImGui::BeginTabItem("My Widget")) {
        myCustomWidget.render();
        ImGui::EndTabItem();
    }
}
\```
```

---

## 🎯 **최종 검증 체크리스트**

### **기능적 요구사항 검증**
- [ ] **파일 로딩**: XSF, CIF, POSCAR 파일이 올바르게 로드됨
- [ ] **원자 관리**: 추가, 제거, 수정이 정상 동작함
- [ ] **결합 생성**: 자동 결합 감지 및 생성이 정확함
- [ ] **주변 원자**: 격자 경계의 주변 원자가 올바르게 표시됨
- [ ] **UI 응답성**: 모든 UI 컴포넌트가 즉시 반응함
- [ ] **배치 처리**: 대용량 데이터 처리 시 성능 최적화됨

### **비기능적 요구사항 검증**
- [ ] **성능**: 10,000개 원자 처리 시 5초 이내 완료
- [ ] **메모리**: 대용량 구조 로딩 시 100MB 이하 사용
- [ ] **안정성**: 24시간 연속 실행 시 메모리 누수 없음
- [ ] **확장성**: 새로운 파일 형식 추가 시 1일 이내 구현 가능
- [ ] **유지보수성**: 버그 수정 시 영향 범위가 명확히 제한됨

### **사용자 경험 검증**
- [ ] **직관적 UI**: 새로운 사용자가 5분 내 기본 기능 사용 가능
- [ ] **시각적 피드백**: 모든 작업에 대한 명확한 시각적 표시
- [ ] **에러 처리**: 사용자 친화적인 에러 메시지 및 복구 옵션
- [ ] **성능 표시**: 대용량 처리 시 진행 상황 표시
- [ ] **단축키 지원**: 주요 기능에 대한 키보드 단축키

### **개발자 경험 검증**
- [ ] **코드 가독성**: 각 클래스와 함수의 목적이 명확
- [ ] **테스트 용이성**: 모든 주요 컴포넌트의 단위 테스트 가능
- [ ] **문서화**: API 문서 및 사용 예제 완비
- [ ] **빌드 시스템**: 새로운 환경에서 10분 내 빌드 가능
- [ ] **디버깅**: 문제 발생 시 적절한 로그 정보 제공

---

## 🏁 **프로젝트 완료 후 제출물**

### **소스 코드**
- 리팩토링된 전체 소스 코드 (120+ 파일)
- CMakeLists.txt 및 빌드 스크립트
- 패키지 설정 파일 (vcpkg, conan 등)

### **문서화**
- API 레퍼런스 (Doxygen 생성)
- 사용자 매뉴얼
- 개발자 가이드
- 아키텍처 문서
- 마이그레이션 가이드

### **테스트**
- 단위 테스트 스위트 (80%+ 커버리지)
- 통합 테스트 시나리오
- 성능 벤치마크 결과
- 메모리 사용량 프로파일링 결과

### **예제 및 데모**
- 샘플 구조 파일들
- 사용 예제 코드
- 스크린샷 및 동영상 데모
- 성능 비교 데이터

이 상세한 작업 목록을 통해 Claude는 각 단계별로 체계적이고 일관된 리팩토링을 수행할 수 있으며, 최종적으로는 현재의 복잡한 단일 클래스를 확장 가능하고 유지보수가 용이한 현대적인 C++ 아키텍처로 완전히 변환할 수 있습니다.
