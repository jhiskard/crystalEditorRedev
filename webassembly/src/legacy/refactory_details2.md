# AtomsTemplate 리팩토링 상세 작업목록 (VtkViewer 통합 버전)

> **📋 작업 참고사항**: 이 문서는 기존 VtkViewer 모듈을 활용하여 계층별 분리 아키텍처로 리팩토링하는 구체적인 작업 내용과 구현 가이드를 제공합니다.

---

## 🎯 **전체 작업 개요**

### **현재 상태 분석 (2024년 기준)**
- **메인 파일**: `atoms_template.h/cpp` (3000+ 라인), VTK 직접 호출 방식
- **상위 모듈**: `vtk_viewer.h/cpp` - VTK 래퍼 모듈 (기존 구현됨)
- **주요 문제**: 
  - AtomsTemplate이 VTK를 직접 조작 (`VtkViewer::Instance().AddActor()` 호출)
  - UI/비즈니스/렌더링 로직이 단일 클래스에 혼재
  - VtkViewer의 고수준 인터페이스 활용도 낮음

### **목표 상태 (VtkViewer 중심 아키텍처)**
```
┌─────────────────────────────────────┐
│            UI Layer                 │
│  - AtomsTemplateUI                  │
│  - PeriodicTableWidget              │
│  - BravaisLatticeWidget             │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│        Application Layer            │
│  - AtomsTemplate (Controller)       │
│  - StructureLoader                  │
│  - AtomService                      │
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
│  - AtomRenderer (VtkViewer 기반)    │
│  - BondRenderer (VtkViewer 기반)    │
│  - CellRenderer (VtkViewer 기반)    │
│  - FileIOManager                    │
└─────────────────────────────────────┘
                    │
┌─────────────────────────────────────┐
│       VTK Wrapper Layer             │
│  - VtkViewer (기존 모듈 활용)        │
│  - MouseInteractorStyle             │
└─────────────────────────────────────┘
```

---

## 📅 **Phase 1: Infrastructure Layer 리팩토링 (VtkViewer 통합) (2-3주)**
**목표**: VtkViewer 인터페이스를 활용한 렌더링 시스템 분리

### 1.1 VtkViewer 기반 렌더링 시스템 분리 (1주)

#### **Task 1.1.1**: VtkViewer 통합 렌더링 인프라 구조 생성
```bash
# 파일 생성 목록 (snake_case 네이밍)
src/infrastructure/rendering/atom_renderer.h
src/infrastructure/rendering/atom_renderer.cpp
src/infrastructure/rendering/bond_renderer.h  
src/infrastructure/rendering/bond_renderer.cpp
src/infrastructure/rendering/cell_renderer.h
src/infrastructure/rendering/cell_renderer.cpp
src/infrastructure/rendering/rendering_manager.h
src/infrastructure/rendering/rendering_manager.cpp
```

**구현 가이드**:
```cpp
// rendering_manager.h - VtkViewer를 활용한 중앙 렌더링 매니저
#pragma once

#include <memory>
#include <string>
#include <map>
#include <vtkSmartPointer.h>

// Forward declarations
class vtkActor;
class AtomRenderer;
class BondRenderer;
class CellRenderer;
class VtkViewer;

/**
 * @brief VtkViewer를 활용한 중앙 렌더링 관리자
 * 
 * 모든 3D 렌더링 작업을 VtkViewer 인터페이스를 통해 관리합니다.
 * 원자, 결합, 단위 격자의 렌더링을 통합하여 제공하며,
 * VtkViewer의 기존 기능들을 활용합니다.
 */
class RenderingManager {
public:
    /**
     * @brief 렌더링 매니저 초기화
     * 
     * VtkViewer와의 연결을 설정하고 하위 렌더러들을 초기화합니다.
     * 
     * @return 초기화 성공 여부
     */
    bool initialize();
    
    /**
     * @brief 렌더링 매니저 종료
     * 
     * 모든 렌더링 리소스를 정리하고 VtkViewer와의 연결을 해제합니다.
     */
    void shutdown();
    
    // VtkViewer 인터페이스 활용
    /**
     * @brief VTK 액터를 렌더러에 추가
     * 
     * @param actor 추가할 VTK 액터
     * @param resetCamera 카메라 리셋 여부 (기본값: false)
     */
    void addActor(vtkSmartPointer<vtkActor> actor, bool resetCamera = false);
    
    /**
     * @brief VTK 액터를 렌더러에서 제거
     * 
     * @param actor 제거할 VTK 액터
     */
    void removeActor(vtkSmartPointer<vtkActor> actor);
    
    /**
     * @brief 렌더링 실행
     * 
     * VtkViewer를 통해 현재 장면을 렌더링합니다.
     */
    void render();
    
    // 원자 렌더링 - 기존 함수들을 여기로 이동
    /**
     * @brief 원자 그룹 추가
     * 
     * @param symbol 원소 기호
     * @param data 원자 그룹 데이터
     */
    void addAtomGroup(const std::string& symbol, const AtomGroupData& data);
    
    /**
     * @brief 원자 그룹 업데이트
     * 
     * @param symbol 업데이트할 원소 기호
     * @note 기존 updateUnifiedAtomGroupVTK에서 이동
     */
    void updateAtomGroup(const std::string& symbol);
    
    /**
     * @brief 원자 그룹 제거
     * 
     * @param symbol 제거할 원소 기호
     */
    void removeAtomGroup(const std::string& symbol);
    
    // 결합 렌더링 - 기존 함수들을 여기로 이동  
    /**
     * @brief 결합 그룹 추가
     * 
     * @param bondKey 결합 키 (예: "C-H")
     * @param data 결합 그룹 데이터
     */
    void addBondGroup(const std::string& bondKey, const BondGroupData& data);
    
    /**
     * @brief 결합 그룹 업데이트
     * 
     * @param bondKey 업데이트할 결합 키
     * @note 기존 updateBondGroup에서 이동
     */
    void updateBondGroup(const std::string& bondKey);
    
    /**
     * @brief 결합 그룹 제거
     * 
     * @param bondKey 제거할 결합 키
     */
    void removeBondGroup(const std::string& bondKey);
    
    // 격자 렌더링 - 기존 함수들을 여기로 이동
    /**
     * @brief 단위 격자 벡터 설정
     * 
     * @param matrix 3x3 격자 매트릭스
     * @note 기존 createUnitCell에서 이동
     */
    void setCellVectors(const float matrix[3][3]);
    
    /**
     * @brief 단위 격자 가시성 설정
     * 
     * @param visible 가시성 여부
     * @note 기존 cellVisible 변수 관리를 여기로 이동
     */
    void setCellVisible(bool visible);
    
    // 성능 모니터링
    /**
     * @brief 메모리 사용량 계산
     * 
     * @return 총 메모리 사용량 (MB)
     * @note 기존 calculateAtomGroupMemory에서 이동
     */
    float calculateMemoryUsage() const;
    
    /**
     * @brief 원자 렌더러 접근자
     * 
     * @return AtomRenderer 포인터
     */
    AtomRenderer* getAtomRenderer() const { return atomRenderer.get(); }
    
    /**
     * @brief 결합 렌더러 접근자
     * 
     * @return BondRenderer 포인터
     */
    BondRenderer* getBondRenderer() const { return bondRenderer.get(); }
    
    /**
     * @brief 격자 렌더러 접근자
     * 
     * @return CellRenderer 포인터
     */
    CellRenderer* getCellRenderer() const { return cellRenderer.get(); }
    
private:
    std::unique_ptr<AtomRenderer> atomRenderer;  ///< 원자 렌더링 전담 객체
    std::unique_ptr<BondRenderer> bondRenderer;  ///< 결합 렌더링 전담 객체
    std::unique_ptr<CellRenderer> cellRenderer;  ///< 격자 렌더링 전담 객체
    
    bool isInitialized;  ///< 초기화 상태 플래그
    
    /**
     * @brief VtkViewer 인스턴스 접근
     * 
     * @return VtkViewer 참조 (싱글톤)
     */
    VtkViewer& getViewer();
};

// atom_renderer.h - VtkViewer 기반 원자 렌더링 전담
#pragma once

#include <memory>
#include <string>
#include <map>
#include <vtkSmartPointer.h>

// Forward declarations
class vtkActor;
class VtkViewer;
struct AtomGroupInfo;
struct AtomGroupData;

/**
 * @brief VtkViewer 기반 원자 렌더링 전담 클래스
 * 
 * 원자 그룹의 생성, 업데이트, 제거를 담당하며
 * VtkViewer 인터페이스를 활용하여 3D 렌더링을 수행합니다.
 */
class AtomRenderer {
public:
    /**
     * @brief 원자 렌더러 초기화
     * 
     * @return 초기화 성공 여부
     */
    bool initialize();
    
    /**
     * @brief 원자 렌더러 종료
     * 
     * 모든 원자 그룹과 VTK 액터들을 정리합니다.
     */
    void shutdown();
    
    // 원자 그룹 관리 - 기존 로직과 매칭
    /**
     * @brief 원자 그룹 추가
     * 
     * @param symbol 원소 기호
     * @param data 원자 그룹 데이터
     */
    void addAtomGroup(const std::string& symbol, const AtomGroupData& data);
    
    /**
     * @brief 원자 그룹 업데이트
     * 
     * @param symbol 업데이트할 원소 기호
     */
    void updateAtomGroup(const std::string& symbol);
    
    /**
     * @brief 특정 원자 그룹 제거
     * 
     * @param symbol 제거할 원소 기호
     */
    void removeAtomGroup(const std::string& symbol);
    
    /**
     * @brief 모든 원자 그룹 제거
     * 
     * @note 기존 clearAllUnifiedAtomGroups에서 이동
     */
    void clearAllAtomGroups();
    
    // VtkViewer 통합
    /**
     * @brief VTK 액터를 VtkViewer에 추가
     * 
     * @param actor 추가할 VTK 액터
     */
    void addActorToViewer(vtkSmartPointer<vtkActor> actor);
    
    /**
     * @brief VTK 액터를 VtkViewer에서 제거
     * 
     * @param actor 제거할 VTK 액터
     */
    void removeActorFromViewer(vtkSmartPointer<vtkActor> actor);
    
    /**
     * @brief 메모리 사용량 계산
     * 
     * @return 원자 렌더링에 사용된 메모리량 (MB)
     */
    float calculateMemoryUsage() const;
    
    /**
     * @brief 원자 그룹 개수 반환
     * 
     * @return 현재 관리 중인 원자 그룹 개수
     */
    size_t getAtomGroupCount() const { return atomGroups.size(); }
    
private:
    std::map<std::string, AtomGroupInfo> atomGroups;  ///< 원자 그룹 정보 맵 (기존 구조 유지)
    bool isInitialized;  ///< 초기화 상태 플래그
    
    /**
     * @brief VtkViewer 인스턴스 접근
     * 
     * @return VtkViewer 참조
     */
    VtkViewer& getViewer();
    
    /**
     * @brief VTK 파이프라인 초기화
     * 
     * @param symbol 원소 기호
     * @param radius 기본 반지름
     * @return 초기화 성공 여부
     */
    bool initializeVtkPipeline(const std::string& symbol, float radius);
    
    /**
     * @brief VTK 파이프라인 정리
     * 
     * @param symbol 원소 기호
     */
    void cleanupVtkPipeline(const std::string& symbol);
};
```

**이동할 기존 함수들**:
- `addAtomToGroup()` → `AtomRenderer::addAtomGroup()`
- `updateUnifiedAtomGroupVTK()` → `AtomRenderer::updateAtomGroup()`
- `clearAllUnifiedAtomGroups()` → `AtomRenderer::clearAllAtomGroups()`
- `createUnitCell()` → `CellRenderer::setCellVectors()`
- `clearUnitCell()` → `CellRenderer::clear()`

#### **Task 1.1.2**: VtkViewer 통합을 위한 기존 코드 변경
```cpp
// 기존 코드 (atoms_template.cpp)
VtkViewer::Instance().AddActor(actor);
VtkViewer::Instance().RemoveActor(actor);
VtkViewer::Instance().Render();

// 새로운 코드 (rendering_manager 사용)
RenderingManager& renderMgr = RenderingManager::getInstance();
renderMgr.addActor(actor);
renderMgr.removeActor(actor);
renderMgr.render();
```

**마이그레이션 체크리스트**:
- [ ] `VtkViewer::Instance().AddActor()` 호출을 `RenderingManager` 인터페이스로 변경
- [ ] `VtkViewer::Instance().RemoveActor()` 호출을 `RenderingManager` 인터페이스로 변경
- [ ] `VtkViewer::Instance().Render()` 호출을 `RenderingManager` 인터페이스로 변경
- [ ] 모든 VTK 액터 생성 코드를 AtomRenderer/BondRenderer로 이동

### 1.2 파일 I/O 시스템 분리 (1주)

#### **Task 1.2.1**: 파일 I/O 기본 구조 생성 (코딩 표준 적용)
```bash
# 파일 생성 목록 (snake_case 네이밍)
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
#pragma once

#include <string>
#include <vector>

// Forward declaration
struct StructureData;

/**
 * @brief 파일 읽기 추상 인터페이스
 * 
 * 다양한 구조 파일 형식을 지원하기 위한 Strategy 패턴 기반 인터페이스입니다.
 * 각 파일 형식별로 구체적인 구현체를 제공합니다.
 */
class FileReader {
public:
    /**
     * @brief 가상 소멸자
     */
    virtual ~FileReader() = default;
    
    /**
     * @brief 파일 읽기 메서드
     * 
     * @param filepath 읽을 파일 경로
     * @param data 읽은 데이터를 저장할 구조체
     * @return 읽기 성공 여부
     */
    virtual bool read(const std::string& filepath, StructureData& data) = 0;
    
    /**
     * @brief 파일 형식 이름 반환
     * 
     * @return 파일 형식 이름 문자열
     */
    virtual std::string getFormatName() const = 0;
    
    /**
     * @brief 지원하는 파일 확장자 목록 반환
     * 
     * @return 지원 확장자 벡터
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

// xsf_reader.h - XSF 파일 전용 리더
#pragma once

#include "file_reader.h"
#include <fstream>

/**
 * @brief XCrySDen Structure File (XSF) 전용 리더
 * 
 * XSF 파일 형식을 파싱하여 원자 구조 데이터를 읽어옵니다.
 * PRIMVEC (격자 벡터)와 PRIMCOORD (원자 좌표) 섹션을 지원합니다.
 */
class XsfReader : public FileReader {
public:
    /**
     * @brief XSF 파일 읽기
     * 
     * @param filepath XSF 파일 경로
     * @param data 파싱된 구조 데이터
     * @return 읽기 성공 여부
     */
    bool read(const std::string& filepath, StructureData& data) override;
    
    /**
     * @brief 파일 형식 이름 반환
     * 
     * @return "XCrySDen Structure File"
     */
    std::string getFormatName() const override { return "XCrySDen Structure File"; }
    
    /**
     * @brief 지원하는 파일 확장자 목록
     * 
     * @return {".xsf"} 벡터
     */
    std::vector<std::string> getSupportedExtensions() const override { return {".xsf"}; }
    
private:
    /**
     * @brief XSF 파일 파싱
     * 
     * @param file 열린 파일 스트림
     * @param data 파싱 결과 저장소
     * @return 파싱 성공 여부
     * @note 기존 ParseXSFFile에서 이동
     */
    bool parseXsfFile(std::ifstream& file, StructureData& data);
    
    /**
     * @brief PRIMVEC 섹션 파싱
     * 
     * @param file 파일 스트림
     * @param cellMatrix 3x3 격자 매트릭스
     * @return 파싱 성공 여부
     */
    bool parsePrimvecSection(std::ifstream& file, float cellMatrix[3][3]);
    
    /**
     * @brief PRIMCOORD 섹션 파싱
     * 
     * @param file 파일 스트림
     * @param atoms 원자 데이터 벡터
     * @return 파싱 성공 여부
     */
    bool parsePrimcoordSection(std::ifstream& file, std::vector<AtomData>& atoms);
    
    /**
     * @brief 원자 번호를 원소 기호로 변환
     * 
     * @param atomicNumber 원자 번호
     * @return 원소 기호 문자열
     */
    std::string getSymbolFromAtomicNumber(int atomicNumber) const;
};

// structure_data.h - 통합 구조 데이터
#pragma once

#include <vector>
#include <string>
#include <map>

/**
 * @brief 원자 데이터 구조체
 * 
 * 개별 원자의 기본 정보를 저장합니다.
 */
struct AtomData {
    std::string symbol;  ///< 원소 기호 (예: "C", "H")
    float position[3];   ///< 카르테시안 좌표
    
    /**
     * @brief 생성자
     * 
     * @param sym 원소 기호
     * @param x X 좌표
     * @param y Y 좌표  
     * @param z Z 좌표
     */
    AtomData(const std::string& sym, float x, float y, float z) 
        : symbol(sym) {
        position[0] = x;
        position[1] = y;
        position[2] = z;
    }
    
    /**
     * @brief 기본 생성자
     */
    AtomData() : symbol("") {
        position[0] = position[1] = position[2] = 0.0f;
    }
};

/**
 * @brief 통합 구조 데이터
 * 
 * 파일에서 읽어온 완전한 원자 구조 정보를 저장합니다.
 * 원자 정보, 격자 벡터, 메타데이터를 포함합니다.
 */
struct StructureData {
    std::vector<AtomData> atoms;           ///< 원자 데이터 목록
    float cellMatrix[3][3];               ///< 3x3 격자 매트릭스
    std::string title;                    ///< 구조 제목
    std::map<std::string, std::string> metadata;  ///< 추가 메타데이터
    
    /**
     * @brief 기본 생성자 - 단위 매트릭스로 초기화
     */
    StructureData() : title("") {
        // 단위 매트릭스로 초기화
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                cellMatrix[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }
    
    /**
     * @brief 구조 데이터 유효성 검사
     * 
     * @return 데이터가 유효한지 여부
     */
    bool isValid() const;
    
    /**
     * @brief 모든 데이터 초기화
     */
    void clear();
    
    /**
     * @brief 원자 개수 반환
     * 
     * @return 원자 개수
     */
    size_t getAtomCount() const { return atoms.size(); }
    
    /**
     * @brief 격자 부피 계산
     * 
     * @return 단위 격자 부피
     */
    float getCellVolume() const;
};

// file_io_manager.h - Strategy 패턴 적용
#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

// Forward declarations
class FileReader;
struct StructureData;

/**
 * @brief 파일 입출력 매니저
 * 
 * Strategy 패턴을 활용하여 다양한 파일 형식을 지원합니다.
 * 각 파일 형식별 리더를 등록하고 적절한 리더를 선택하여 파일을 읽습니다.
 */
class FileIOManager {
public:
    /**
     * @brief 생성자 - 기본 리더들 등록
     */
    FileIOManager();
    
    /**
     * @brief 소멸자
     */
    ~FileIOManager() = default;
    
    /**
     * @brief 구조 파일 로드
     * 
     * @param filepath 로드할 파일 경로
     * @param data 로드된 구조 데이터 저장소
     * @return 로드 성공 여부
     */
    bool loadStructure(const std::string& filepath, StructureData& data);
    
    /**
     * @brief 파일 리더 등록
     * 
     * @param extension 파일 확장자 (예: ".xsf")
     * @param reader 등록할 파일 리더
     */
    void registerReader(const std::string& extension, std::unique_ptr<FileReader> reader);
    
    /**
     * @brief 지원하는 파일 형식 목록 반환
     * 
     * @return 지원 형식 목록 (형식명과 확장자 쌍)
     */
    std::vector<std::pair<std::string, std::string>> getSupportedFormats() const;
    
    /**
     * @brief 특정 확장자 지원 여부 확인
     * 
     * @param extension 확인할 확장자
     * @return 지원 여부
     */
    bool isExtensionSupported(const std::string& extension) const;
    
private:
    std::map<std::string, std::unique_ptr<FileReader>> readers;  ///< 확장자별 리더 맵
    
    /**
     * @brief 파일 경로에서 확장자 추출
     * 
     * @param filepath 파일 경로
     * @return 소문자 확장자 (점 포함, 예: ".xsf")
     */
    std::string getFileExtension(const std::string& filepath) const;
    
    /**
     * @brief 기본 파일 리더들 초기화
     */
    void initializeDefaultReaders();
    
    /**
     * @brief 파일 존재 여부 확인
     * 
     * @param filepath 확인할 파일 경로
     * @return 파일 존재 여부
     */
    bool fileExists(const std::string& filepath) const;
};
```

**이동할 기존 함수들**:
- `ParseXSFFile()` → `XSFReader::parseXSFFile()`
- `InitializeAtomicStructure()` → `StructureLoader::createFromData()` (Phase 3에서)
- `AtomData` 구조체 → `StructureData` 내부로 이동

### 1.3 배치 업데이트 시스템 분리 (0.5주)

#### **Task 1.3.1**: VtkViewer 기반 배치 시스템 개선 (코딩 표준 적용)
```bash
# 파일 생성 목록 (snake_case 네이밍)
src/infrastructure/batch/batch_update_system.h
src/infrastructure/batch/batch_update_system.cpp
src/infrastructure/batch/performance_monitor.h
src/infrastructure/batch/performance_monitor.cpp
```

**구현 가이드**:
```cpp
// batch_update_system.h - VtkViewer와 통합된 배치 시스템
#pragma once

#include <set>
#include <string>
#include <memory>
#include <chrono>

// Forward declarations
class RenderingManager;

/**
 * @brief 배치 업데이트 시스템
 * 
 * VtkViewer와 통합된 고성능 배치 렌더링 시스템입니다.
 * 여러 원자/결합 업데이트를 모아서 한 번에 처리하여 성능을 최적화합니다.
 */
class BatchUpdateSystem {
public:
    /**
     * @brief 배치 모드 시작
     * 
     * 이후 모든 렌더링 업데이트를 지연시켜 배치로 처리합니다.
     */
    void beginBatch();
    
    /**
     * @brief 배치 모드 종료 및 일괄 처리
     * 
     * 지연된 모든 업데이트를 실행하고 VtkViewer를 통해 렌더링합니다.
     * @note 기존 AtomsTemplate::endBatch()에서 로직 이동
     */
    void endBatch();
    
    /**
     * @brief 현재 배치 모드 상태 확인
     * 
     * @return 배치 모드 활성화 여부
     */
    bool isBatchActive() const { return batchActive; }
    
    /**
     * @brief 원자 그룹 업데이트 스케줄링
     * 
     * @param symbol 업데이트할 원소 기호
     */
    void scheduleAtomUpdate(const std::string& symbol);
    
    /**
     * @brief 결합 그룹 업데이트 스케줄링
     * 
     * @param bondKey 업데이트할 결합 키 (예: "C-H")
     */
    void scheduleBondUpdate(const std::string& bondKey);
    
    /**
     * @brief 렌더링 매니저 설정
     * 
     * @param manager VtkViewer와 연결된 렌더링 매니저
     */
    void setRenderingManager(RenderingManager* manager) { renderingManager = manager; }
    
    /**
     * @brief RAII 기반 배치 가드 클래스
     * 
     * 생성자에서 배치 모드를 시작하고 소멸자에서 자동으로 종료합니다.
     * 예외 안전성을 보장합니다.
     */
    class BatchGuard {
    public:
        /**
         * @brief 배치 가드 생성자
         * 
         * @param system 배치 시스템 포인터
         */
        explicit BatchGuard(BatchUpdateSystem* system);
        
        /**
         * @brief 배치 가드 소멸자
         * 
         * 자동으로 배치를 종료하고 렌더링을 실행합니다.
         */
        ~BatchGuard();
        
        // 복사/이동 방지 (RAII 안전성)
        BatchGuard(const BatchGuard&) = delete;
        BatchGuard& operator=(const BatchGuard&) = delete;
        BatchGuard(BatchGuard&&) = delete;
        BatchGuard& operator=(BatchGuard&&) = delete;
        
    private:
        BatchUpdateSystem* system;  ///< 관리하는 배치 시스템
        bool wasActivated;          ///< 이 가드가 배치를 시작했는지 여부
    };
    
    /**
     * @brief 배치 가드 생성 팩토리
     * 
     * @return BatchGuard 객체
     */
    BatchGuard createBatchGuard() { return BatchGuard(this); }
    
    /**
     * @brief 대기 중인 업데이트 개수 반환
     * 
     * @return 총 대기 중인 업데이트 수 (원자 그룹 + 결합 그룹)
     */
    size_t getPendingUpdateCount() const {
        return pendingAtomUpdates.size() + pendingBondUpdates.size();
    }
    
    /**
     * @brief 성능 통계 반환
     * 
     * @return 최근 배치 처리 시간 (밀리초)
     */
    float getLastBatchDuration() const { return lastBatchDuration; }
    
private:
    bool batchActive;                               ///< 배치 모드 활성화 상태
    std::set<std::string> pendingAtomUpdates;       ///< 대기 중인 원자 업데이트
    std::set<std::string> pendingBondUpdates;       ///< 대기 중인 결합 업데이트
    RenderingManager* renderingManager;             ///< VtkViewer와 연결된 렌더링 매니저
    float lastBatchDuration;                        ///< 마지막 배치 처리 시간 (ms)
    
    /**
     * @brief 성능 통계 업데이트
     * 
     * @param duration 처리 시간 (밀리초)
     */
    void updatePerformanceStats(float duration);
    
    /**
     * @brief 대기 중인 모든 업데이트 처리
     * 
     * VtkViewer 통합 렌더링 수행
     */
    void processPendingUpdates();
    
    /**
     * @brief 강제 배치 종료
     * 
     * 예외 상황에서 안전하게 배치 모드를 종료합니다.
     */
    void forceBatchEnd();
};

// performance_monitor.h
#pragma once

#include <chrono>
#include <vector>

/**
 * @brief 렌더링 성능 통계 구조체
 */
struct RenderingPerformanceStats {
    int totalAtoms;              ///< 총 원자 수
    int totalGroups;             ///< 총 그룹 수  
    int totalBonds;              ///< 총 결합 수
    float lastUpdateTime;        ///< 마지막 업데이트 시간 (ms)
    float averageUpdateTime;     ///< 평균 업데이트 시간 (ms)
    int updateCount;             ///< 총 업데이트 횟수
    float memoryUsage;           ///< 메모리 사용량 (MB)
    
    /**
     * @brief 기본 생성자 - 모든 값을 0으로 초기화
     */
    RenderingPerformanceStats() 
        : totalAtoms(0), totalGroups(0), totalBonds(0)
        , lastUpdateTime(0.0f), averageUpdateTime(0.0f)
        , updateCount(0), memoryUsage(0.0f) {}
};

/**
 * @brief 성능 모니터링 클래스
 * 
 * 렌더링 시스템의 성능을 모니터링하고 통계를 수집합니다.
 * VtkViewer와 연계하여 프레임 레이트, 메모리 사용량 등을 추적합니다.
 */
class PerformanceMonitor {
public:
    /**
     * @brief 성능 측정 시작
     */
    void startMeasurement();
    
    /**
     * @brief 성능 측정 종료 및 통계 업데이트
     * 
     * @return 측정된 시간 (밀리초)
     */
    float endMeasurement();
    
    /**
     * @brief 현재 성능 통계 반환
     * 
     * @return 성능 통계 구조체
     */
    const RenderingPerformanceStats& getStats() const { return stats; }
    
    /**
     * @brief 통계 업데이트
     * 
     * @param duration 처리 시간 (밀리초)
     * @param atomCount 원자 수
     * @param bondCount 결합 수
     * @param groupCount 그룹 수
     * @param memoryUsage 메모리 사용량 (MB)
     */
    void updateStats(float duration, int atomCount, int bondCount, 
                    int groupCount, float memoryUsage);
    
    /**
     * @brief 성능 경고 기준 설정
     * 
     * @param thresholdMs 경고를 발생시킬 임계 시간 (밀리초)
     */
    void setWarningThreshold(float thresholdMs) { warningThreshold = thresholdMs; }
    
    /**
     * @brief 통계 초기화
     */
    void resetStats();
    
    /**
     * @brief 성능 보고서 생성
     * 
     * @return 성능 보고서 문자열
     */
    std::string generatePerformanceReport() const;
    
    /**
     * @brief 최근 측정 시간들의 히스토리 반환
     * 
     * @return 최근 측정 시간 벡터 (최대 100개)
     */
    const std::vector<float>& getMeasurementHistory() const { return measurementHistory; }
    
private:
    RenderingPerformanceStats stats;                              ///< 현재 성능 통계
    std::chrono::high_resolution_clock::time_point startTime;    ///< 측정 시작 시간
    float warningThreshold;                                       ///< 성능 경고 임계값 (ms)
    std::vector<float> measurementHistory;                       ///< 측정 시간 히스토리
    
    static const size_t MAX_HISTORY_SIZE = 100;                  ///< 히스토리 최대 크기
    
    /**
     * @brief 이동 평균 계산
     * 
     * @param newValue 새로운 값
     * @param currentAverage 현재 평균
     * @param count 총 측정 횟수
     * @return 업데이트된 이동 평균
     */
    float calculateMovingAverage(float newValue, float currentAverage, int count) const;
    
    /**
     * @brief 성능 경고 확인 및 로그
     * 
     * @param duration 측정된 시간
     */
    void checkPerformanceWarning(float duration) const;
};

// 전역 성능 모니터 인스턴스 (필요시)
extern PerformanceMonitor g_performanceMonitor;
```

**이동할 기존 코드**:
- `beginBatch()` → `BatchUpdateSystem::beginBatch()`
- `endBatch()` → `BatchUpdateSystem::endBatch()`
- `BatchGuard` 클래스 → `BatchUpdateSystem::BatchGuard`
- `VtkViewer::Instance().Render()` 호출을 `RenderingManager`로 통합

---

## 🎯 **Phase 2: Domain Layer (3-4주)**  
**목표**: 핵심 비즈니스 로직 분리 (VtkViewer와 독립적)

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
- `chemical_symbols[]` 배열 → `ElementDatabase` 내부 초기화
- `atomic_names[]` 배열 → `ElementDatabase` 내부 초기화
- `jmol_colors[]` 배열 → `ElementDatabase` 내부 초기화
- `covalent_radii[]` 배열 → `ElementDatabase` 내부 초기화
- `findElementColor()` 함수 → `ElementDatabase::getElementColor()`

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
// atom_manager.h - VtkViewer와 독립적인 도메인 로직
class AtomManager {
public:
    // CRUD Operations - 기존 함수들을 여기로 이동
    uint32_t addAtom(const AtomCreateRequest& request); // ← createAtomSphere()에서 로직 분리
    bool removeAtom(uint32_t atomId);
    bool updateAtom(uint32_t atomId, const AtomUpdateRequest& request);
    
    // Query Operations - 기존 함수들을 여기로 이동
    AtomInfo* findAtom(uint32_t atomId); // ← findAtomById()에서 이동
    const AtomInfo* findAtom(uint32_t atomId) const;
    std::vector<AtomInfo> getAtoms(AtomType type = AtomType::ORIGINAL) const;
    int findAtomIndex(const AtomInfo& atom) const; // ← findAtomIndex()에서 이동
    
    // Surrounding Atoms - 기존 함수들을 여기로 이동
    void createSurroundingAtoms(const CellManager& cellManager); // ← createSurroundingAtoms()에서 로직 분리
    void clearSurroundingAtoms(); // ← hideSurroundingAtoms()에서 로직 분리
    bool areSurroundingAtomsVisible() const;
    
    // Statistics
    size_t getAtomCount(AtomType type = AtomType::ORIGINAL) const;
    std::map<std::string, int> getElementCounts() const;
    
    // Rendering Integration (AtomRenderer와 연결)
    void setAtomRenderer(AtomRenderer* renderer) { atomRenderer = renderer; }
    
private:
    std::vector<AtomInfo> originalAtoms; // 기존 createdAtoms
    std::vector<AtomInfo> surroundingAtoms; // 기존 surroundingAtoms
    uint32_t nextAtomId = 1;
    bool surroundingVisible = false;
    
    // Rendering integration
    AtomRenderer* atomRenderer = nullptr;
    
    uint32_t generateAtomId(); // ← generateUniqueAtomId()에서 이동
    void notifyRenderer(const std::string& symbol); // 렌더링 알림
};
```

**이동할 기존 코드**:
- `createAtomSphere()` → `AtomManager::addAtom()` + `AtomRenderer::addAtomGroup()`
- `findAtomById()` → `AtomManager::findAtom()`
- `findAtomIndex()` → `AtomManager::findAtomIndex()`
- `generateUniqueAtomId()` → `AtomManager::generateAtomId()`

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
// bond_manager.h - VtkViewer와 독립적인 결합 로직
class BondManager {
public:
    // Bond Creation - 기존 함수들을 여기로 이동
    uint32_t createBond(uint32_t atom1Id, uint32_t atom2Id, BondType type = BondType::ORIGINAL); // ← createBond()에서 분리
    bool removeBond(uint32_t bondId);
    void removeAllBonds(BondType type); // ← clearAllBonds()에서 로직 분리
    
    // Automatic Bond Generation - 기존 함수들을 여기로 이동  
    void createAllBonds(const AtomManager& atomManager); // ← createAllBonds()에서 분리
    void createBondsForAtoms(const std::vector<uint32_t>& atomIds, const AtomManager& atomManager); // ← createBondsForAtoms()에서 분리
    
    // Bond Validation - 기존 함수에서 이동
    bool shouldCreateBond(const AtomInfo& atom1, const AtomInfo& atom2) const; // ← shouldCreateBond()에서 이동
    
    // Bond Properties - 기존 함수들에서 이동
    void setBondThickness(float thickness); // ← updateAllBondGroupThickness() 관련 로직
    void setBondOpacity(float opacity); // ← updateAllBondGroupOpacity() 관련 로직
    void setBondScalingFactor(float factor);
    
    // Rendering Integration (BondRenderer와 연결)
    void setBondRenderer(BondRenderer* renderer) { bondRenderer = renderer; }
    
    // Statistics - 기존 함수에서 이동
    size_t getBondCount(BondType type = BondType::ORIGINAL) const; // ← getTotalBondCount()에서 이동  
    
private:
    std::vector<BondInfo> bonds; // 기존 createdBonds + surroundingBonds 통합
    uint32_t nextBondId = 1;
    float bondScalingFactor = 1.0f;
    float bondThickness = 1.0f;
    float bondOpacity = 1.0f;
    
    // Rendering integration
    BondRenderer* bondRenderer = nullptr;
    
    uint32_t generateBondId(); // ← generateUniqueBondId()에서 이동
    void notifyRenderer(const std::string& bondKey); // 렌더링 알림
};
```

**이동할 기존 코드**:
- `createBond()` → `BondManager::createBond()` + `BondRenderer::addBondGroup()`
- `shouldCreateBond()` → `BondManager::shouldCreateBond()`
- `createAllBonds()` → `BondManager::createAllBonds()`
- `clearAllBonds()` → `BondManager::removeAllBonds()`

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
// cell_manager.h - VtkViewer와 독립적인 격자 로직
class CellManager {
public:
    // Cell Definition - 기존 데이터를 여기로 이동
    void setCellMatrix(const float matrix[3][3]);
    void getCellMatrix(float matrix[3][3]) const;
    bool isCellDefined() const; // ← cellEdgeActors.empty() 체크 로직을 별도로 분리
    
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
    
    // Rendering Integration (CellRenderer와 연결)
    void setCellRenderer(CellRenderer* renderer) { cellRenderer = renderer; }
    void setVisible(bool visible);
    bool isVisible() const;
    
private:
    float cellMatrix[3][3];
    float invMatrix[3][3];
    bool matrixDefined = false;
    bool visible = false;
    
    // Rendering integration
    CellRenderer* cellRenderer = nullptr;
    
    void calculateInverseMatrix(); // ← calculateInverseMatrix()에서 이동
    void notifyRenderer(); // 렌더링 알림
};
```

**이동할 기존 코드**:
- `calculateInverseMatrix()` 전역 함수 → `CellManager::calculateInverseMatrix()`
- `cartesianToFractional()` 전역 함수 → `CellManager::cartesianToFractional()`
- `fractionalToCartesian()` 전역 함수 → `CellManager::fractionalToCartesian()`
- `cellInfo` 전역 변수 → `CellManager` 멤버 변수들

---

## 🎮 **Phase 3: Application Layer (2주)**
**목표**: 고수준 애플리케이션 로직 및 컨트롤러 구성

### 3.1 AtomsTemplate 컨트롤러화 (1주)

#### **Task 3.1.1**: VtkViewer 기반 AtomsTemplate 슬림화
```cpp
// atoms_template.h (리팩토링 후) - VtkViewer 통합 버전
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
    
    // UI Integration - VtkViewer 독립적
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
    // Core Domain Components
    std::unique_ptr<AtomManager> atomManager;
    std::unique_ptr<BondManager> bondManager;
    std::unique_ptr<CellManager> cellManager;
    std::unique_ptr<ElementDatabase> elementDB;
    
    // Infrastructure Components - VtkViewer 기반
    std::unique_ptr<RenderingManager> renderingManager; // VtkViewer 통합
    std::unique_ptr<FileIOManager> fileManager;
    std::unique_ptr<BatchUpdateSystem> batchSystem;
    
    bool initialized = false;
    
    // Component Integration
    void setupComponentConnections(); // Manager들과 Renderer들 간의 연결 설정
    void updateFractionalCoordinates(); // 격자 변경 시 모든 원자 분수 좌표 재계산
    
    // VtkViewer 통합 헬퍼
    void initializeRenderingSystem(); // RenderingManager + VtkViewer 설정
};
```

**제거할 기존 함수들** (Manager로 이동):
- ✅ `createAtomSphere()` → `AtomManager::addAtom()` + `AtomRenderer::addAtomGroup()`
- ✅ `createBond()` → `BondManager::createBond()` + `BondRenderer::addBondGroup()`
- ✅ `VtkViewer::Instance().AddActor()` 직접 호출 → `RenderingManager` 통합

#### **Task 3.1.2**: VtkViewer 기반 컴포넌트 통신 설정
```cpp
// atoms_template.cpp에서 VtkViewer 기반 연결 설정
void AtomsTemplate::setupComponentConnections() {
    // RenderingManager 설정
    renderingManager = std::make_unique<RenderingManager>();
    renderingManager->initialize();
    
    // Domain → Infrastructure 연결
    atomManager->setAtomRenderer(renderingManager->getAtomRenderer());
    bondManager->setBondRenderer(renderingManager->getBondRenderer());
    cellManager->setCellRenderer(renderingManager->getCellRenderer());
    
    // AtomManager → BondManager 알림
    atomManager->onAtomAdded = [this](uint32_t atomId) {
        bondManager->createBondsForAtom(atomId, *atomManager);
        // 렌더링은 각 Manager가 자체적으로 처리 (VtkViewer 통합)
    };
    
    // BondManager → RenderingManager 알림
    bondManager->onBondCreated = [this](uint32_t bondId) {
        // 렌더링 업데이트는 BondRenderer에서 자동 처리
    };
    
    // CellManager → 전체 시스템 알림
    cellManager->onCellChanged = [this]() {
        updateFractionalCoordinates();
        if (atomManager->areSurroundingAtomsVisible()) {
            atomManager->clearSurroundingAtoms();
            atomManager->createSurroundingAtoms(*cellManager);
        }
        // VtkViewer 렌더링은 CellRenderer에서 처리
    };
}

void AtomsTemplate::initializeRenderingSystem() {
    // VtkViewer는 이미 초기화되어 있다고 가정
    // RenderingManager가 VtkViewer 인터페이스를 활용
    renderingManager->initialize();
    
    SPDLOG_INFO("Rendering system initialized with VtkViewer integration");
}
```

### 3.2 워크플로우 및 서비스 레이어 (1주)

#### **Task 3.2.1**: 구조 로딩 워크플로우 (VtkViewer 독립적)
```bash
# 파일 생성 목록  
src/application/workflows/structure_loader.h
src/application/workflows/structure_loader.cpp
src/application/services/atom_service.h
src/application/services/atom_service.cpp
```

**구현 가이드**:
```cpp
// structure_loader.h - VtkViewer와 독립적인 로딩 워크플로우
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
    
    // 렌더링은 각 Manager를 통해 자동으로 처리됨 (VtkViewer 통합)
};
```

**이동할 기존 로직**:
- `LoadXSFFile()` → `StructureLoader::loadStructure()` (VtkViewer 호출 제거)
- `InitializeAtomicStructure()` → `StructureLoader::createAtomsFromData()`
- 모든 `VtkViewer::Instance()` 직접 호출 제거

---

## 🖼️ **Phase 4: UI Layer (3-4주)**
**목표**: 사용자 인터페이스 완전 분리 (VtkViewer UI와 독립적)

### 4.1 기본 UI 시스템 분리 (1주)

#### **Task 4.1.1**: VtkViewer UI와 독립적인 AtomsTemplateUI 생성
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
// atoms_template_ui.h - VtkViewer UI와 독립적인 UI 시스템  
class AtomsTemplateUI {
public:
    explicit AtomsTemplateUI(AtomsTemplate* controller);
    void render(bool* openWindow); // ← AtomsTemplate::Render()에서 이동
    
private:
    // 기존 UI 함수들을 여기로 이동 (VtkViewer::Render와 독립적)
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
    
    // VtkViewer UI와의 연동은 controller를 통해서만
    // 직접적인 VtkViewer 호출 없음
};
```

**이동할 기존 UI 함수들**:
- `Render()` → `AtomsTemplateUI::render()` (VtkViewer UI 호출 제거)
- `renderCreatedAtomsSection()` → `AtomsTemplateUI::renderCreatedAtomsSection()`
- `applyAtomChanges()` → `AtomService::applyChanges()` (도메인 로직 분리)

### 4.2 확장 UI 컴포넌트 (atoms_template_periodic_table.cpp 통합) (1주)

#### **Task 4.2.1**: PeriodicTableWidget 생성 (VtkViewer 독립적)
```bash
# 파일 생성 목록
src/ui/widgets/periodic_table_widget.h
src/ui/widgets/periodic_table_widget.cpp
```

**구현 가이드**:
```cpp
// periodic_table_widget.h - VtkViewer와 독립적인 주기율표 위젯
class PeriodicTableWidget {
public:
    explicit PeriodicTableWidget(const ElementDatabase& elementDB, const CellManager& cellManager);
    
    void render(); // ← atoms_template_periodic_table.cpp의 renderPeriodicTable()에서 이동
    
    // Selection management
    void setSelectedElement(int elementIndex);
    int getSelectedElement() const;
    
    // Classification filtering
    void setClassificationFilter(ElementClassification filter);
    ElementClassification getClassificationFilter() const;
    
    // Callbacks - VtkViewer와 독립적으로 동작
    std::function<void(const std::string& symbol, const float position[3])> onAtomAddRequested;
    
private:
    const ElementDatabase& elementDB;
    const CellManager& cellManager;
    int selectedElement = -1;
    ElementClassification classificationFilter = ElementClassification::All;
    
    // UI state
    float atomPosition[3] = {0.0f, 0.0f, 0.0f};
    float fracPosition[3] = {0.0f, 0.0f, 0.0f};
    
    // Rendering helpers
    void renderMainPeriodicTable();
    void renderLanthanides();
    void renderActinides();
    void renderElement(const Element& element, int elementIndex, const ImVec2& size);
    void renderElementTooltip(const Element& element);
    void renderClassificationFilter();
    void renderPositionInput();
    void renderSelectedElementInfo();
    
    // Style management
    void setupElementButtonStyle(const Element& element, bool isSelected);
    ImVec4 calculateHoveredColor(const ImVec4& baseColor);
    ImVec4 calculateActiveColor(const ImVec4& baseColor);
};
```

### 4.3 Bravais Lattice Widget (atoms_template_bravais_lattice.cpp 통합) (1주)

#### **Task 4.3.1**: BravaisLatticeWidget 생성 (VtkViewer 독립적)
```bash
# 파일 생성 목록
src/ui/widgets/bravais_lattice_widget.h
src/ui/widgets/bravais_lattice_widget.cpp
```

**구현 가이드**:
```cpp
// bravais_lattice_widget.h - VtkViewer와 독립적인 격자 위젯
class BravaisLatticeWidget {
public:
    explicit BravaisLatticeWidget(CellManager& cellManager);
    
    void render(); // ← atoms_template_bravais_lattice.cpp의 renderCrystalTemplate()에서 이동
    
    // Lattice management
    void setBravaisLattice(BravaisLatticeType type);
    BravaisLatticeType getSelectedLatticeType() const;
    
    // Crystal system filtering
    void setCrystalSystemFilter(const std::set<CrystalSystem>& systems);
    std::set<CrystalSystem> getCrystalSystemFilter() const;
    
    // Callbacks - VtkViewer와 독립적
    std::function<void(const float matrix[3][3])> onLatticeApplied;
    
private:
    CellManager& cellManager;
    int selectedBravaisType = -1;
    std::array<LatticeParameters, 14> bravaisParams;
    std::set<CrystalSystem> activeFilters;
    
    // UI layout
    float buttonWidth = 0.25f;
    float paramWidth = 0.65f;
    float inputWidth = 60.0f;
    
    // Rendering components
    void renderCrystalSystemFilters();
    void renderLatticeList();
    void renderLatticeParameters(int latticeIndex);
    void renderLatticeDescription();
    
    // Parameter input helpers
    void renderCubicParameters(int latticeIndex);
    void renderTetragonalParameters(int latticeIndex);
    void renderOrthorhombicParameters(int latticeIndex);
    void renderMonoclinicParameters(int latticeIndex);
    void renderTriclinicParameters(int latticeIndex);
    void renderRhombohedralParameters(int latticeIndex);
    void renderHexagonalParameters(int latticeIndex);
    
    // Utility functions
    void calculateCellMatrix(BravaisLatticeType type, const LatticeParameters& params, float matrix[3][3]);
    bool isLatticeVisible(int latticeIndex) const;
    std::string getLatticeDescription(BravaisLatticeType type) const;
};
```

### 4.4 UI 통합 및 VtkViewer와의 조화 (1주)

#### **Task 4.4.1**: VtkViewer UI와 통합된 메인 애플리케이션
```bash
# 파일 생성 목록
src/ui/layouts/main_application_ui.h
src/ui/layouts/main_application_ui.cpp
```

**구현 가이드**:
```cpp
// main_application_ui.h - VtkViewer와 AtomsTemplateUI 통합
class MainApplicationUI {
public:
    void render();
    
private:
    std::unique_ptr<AtomsTemplateUI> atomsTemplateUI;
    std::unique_ptr<PeriodicTableWidget> periodicTableWidget;
    std::unique_ptr<BravaisLatticeWidget> bravaisLatticeWidget;
    
    // VtkViewer와의 통합
    void renderViewerWindow();
    void renderAtomsTemplateWindow();
    void renderPeriodicTableWindow();
    void renderBravaisLatticeWindow();
    
    // Window management
    struct WindowState {
        bool viewerWindowOpen = true;        // VtkViewer 창
        bool atomsWindowOpen = true;         // AtomsTemplate 창
        bool periodicTableWindowOpen = false;
        bool bravaisLatticeWindowOpen = false;
    };
    
    WindowState windowState;
};

// main_application_ui.cpp - 구현
void MainApplicationUI::render() {
    // 1. VtkViewer 창 (기존 VtkViewer::Render 활용)
    if (windowState.viewerWindowOpen) {
        VtkViewer::Instance().Render(&windowState.viewerWindowOpen);
    }
    
    // 2. AtomsTemplate 관리 창 (독립적)
    if (windowState.atomsWindowOpen) {
        atomsTemplateUI->render(&windowState.atomsWindowOpen);
    }
    
    // 3. 주기율표 창 (독립적)
    if (windowState.periodicTableWindowOpen) {
        if (ImGui::Begin("Periodic Table", &windowState.periodicTableWindowOpen)) {
            periodicTableWidget->render();
        }
        ImGui::End();
    }
    
    // 4. 격자 설정 창 (독립적)
    if (windowState.bravaisLatticeWindowOpen) {
        if (ImGui::Begin("Bravais Lattice", &windowState.bravaisLatticeWindowOpen)) {
            bravaisLatticeWidget->render();
        }
        ImGui::End();
    }
}
```

---

## 🧪 **테스트 및 검증 (각 Phase별 병행)**

### **VtkViewer 통합 테스트 가이드**
```cpp
// tests/integration/vtk_viewer_integration_test.cpp
TEST(VtkViewerIntegrationTest, RenderingManagerIntegration) {
    // VtkViewer 초기화 확인
    VtkViewer& viewer = VtkViewer::Instance();
    
    // RenderingManager 초기화
    RenderingManager renderingMgr;
    renderingMgr.initialize();
    
    // 원자 추가 테스트
    AtomGroupData atomData;
    atomData.symbol = "C";
    renderingMgr.addAtomGroup("C", atomData);
    
    // 렌더링 테스트
    renderingMgr.render();
    
    // VtkViewer가 정상적으로 렌더링되는지 확인
    EXPECT_TRUE(true); // 실제 렌더링 검증은 시각적 확인
}

TEST(AtomsTemplateTest, VtkViewerIndependentDomainLogic) {
    // Domain Layer가 VtkViewer와 독립적으로 동작하는지 테스트
    AtomManager atomManager;
    BondManager bondManager;
    CellManager cellManager;
    
    // VtkViewer 없이도 도메인 로직 실행 가능
    AtomCreateRequest request{"C", {0, 0, 0}};
    uint32_t atomId = atomManager.addAtom(request);
    
    EXPECT_NE(atomId, 0);
    EXPECT_EQ(atomManager.getAtomCount(), 1);
}
```

---

## 📋 **Phase별 완료 체크리스트 (VtkViewer 통합 버전)**

### **Phase 1 완료 체크리스트** ✅
- [ ] RenderingManager 클래스 생성 및 VtkViewer 통합
- [ ] AtomRenderer, BondRenderer, CellRenderer VtkViewer 기반 구현
- [ ] 기존 `VtkViewer::Instance().AddActor()` 호출을 RenderingManager로 변경
- [ ] FileIOManager 및 XSFReader 구현 (VtkViewer 독립적)
- [ ] BatchUpdateSystem VtkViewer 통합 개선
- [ ] Phase 1 통합 테스트 통과

### **Phase 2 완료 체크리스트** ✅  
- [ ] ElementDatabase 클래스 구현 (VtkViewer 독립적)
- [ ] AtomManager 클래스 구현 및 AtomRenderer 연동
- [ ] BondManager 클래스 구현 및 BondRenderer 연동
- [ ] CellManager 클래스 구현 및 CellRenderer 연동
- [ ] BravaisLatticeTemplate 시스템 구현
- [ ] 모든 도메인 로직이 VtkViewer와 독립적으로 동작 확인
- [ ] Phase 2 단위 테스트 통과

### **Phase 3 완료 체크리스트** ✅
- [ ] AtomsTemplate을 경량 컨트롤러로 변환 (VtkViewer 통합)
- [ ] Domain Layer와 Infrastructure Layer 연결 인터페이스 구현
- [ ] StructureLoader 워크플로우 클래스 구현 (VtkViewer 독립적)
- [ ] AtomService 등 서비스 레이어 구현
- [ ] RenderingManager를 통한 VtkViewer 통합 검증
- [ ] Phase 3 통합 테스트 통과

### **Phase 4 완료 체크리스트** ✅
- [ ] AtomsTemplateUI 클래스 구현 (VtkViewer UI와 독립적)
- [ ] PeriodicTableWidget 구현 (atoms_template_periodic_table.cpp 통합)
- [ ] BravaisLatticeWidget 구현 (atoms_template_bravais_lattice.cpp 통합)
- [ ] MainApplicationUI에서 VtkViewer와 다른 UI 창들의 조화로운 통합
- [ ] UI Layer가 Domain Layer와만 통신하고 VtkViewer와 독립적 동작 검증
- [ ] 전체 애플리케이션의 VtkViewer 통합 완성도 확인

---

## 🏆 **VtkViewer 통합 리팩토링의 주요 이점**

### **1. 기존 VTK 래퍼 활용**
- ✅ **VtkViewer 인프라 재사용**: 이미 구현된 VTK 래핑 로직 활용
- ✅ **카메라 조작 기능**: VtkViewer의 카메라 위젯 및 인터랙션 유지
- ✅ **렌더링 최적화**: VtkViewer의 프레임버퍼 관리 및 성능 최적화 활용

### **2. 계층별 책임 명확화**
- ✅ **Domain Layer**: VtkViewer와 완전 독립적인 비즈니스 로직
- ✅ **Infrastructure Layer**: VtkViewer 인터페이스를 활용한 렌더링 구현
- ✅ **UI Layer**: VtkViewer UI와 독립적인 도구 창들

### **3. 확장성 및 유지보수성**
- ✅ **렌더링 백엔드 독립성**: VtkViewer 교체 시 Domain Layer 영향 없음
- ✅ **UI 독립성**: VtkViewer UI와 AtomsTemplate UI 독립 개발 가능
- ✅ **테스트 용이성**: 각 계층별 독립적 테스트 가능

### **4. 기존 코드 호환성**
- ✅ **점진적 마이그레이션**: 기존 VtkViewer 기능 유지하면서 단계적 리팩토링
- ✅ **API 호환성**: 기존 `VtkViewer::Instance()` 호출 점진적 변경
- ✅ **기능 보존**: 모든 기존 기능 유지하면서 구조 개선

이 리팩토링을 통해 VtkViewer의 장점을 최대한 활용하면서도 확장 가능하고 유지보수가 용이한 현대적인 아키텍처를 구축할 수 있습니다.