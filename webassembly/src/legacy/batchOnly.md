# AtomsTemplate 레거시 모드 제거 계획

## 1. 전체 개요

현재 코드는 **통합 시스템(CPU 인스턴싱)**과 **레거시 시스템**이 혼재되어 있으며, 배치 업데이트 시스템도 복잡하게 구성되어 있습니다. 목표는 다음 두 시스템만 남기는 것입니다:

- ✅ **배치 업데이트 시스템** (BatchGuard, beginBatch/endBatch)
- ✅ **CPU 인스턴싱 모드** (atomGroups, bondGroups 통합 시스템)

## 2. 제거 대상 시스템들

### 2.1 레거시 원자 렌더링 시스템
```cpp
// 제거 대상: 개별 VTK 액터 기반 원자 렌더링
static std::map<std::string, vtkSmartPointer<vtkPolyData>> elementBaseGeometries;
static std::map<std::string, vtkSmartPointer<vtkActor>> elementGroupActors;
static std::map<std::string, vtkSmartPointer<vtkPolyDataMapper>> elementGroupMappers;
static std::map<std::string, vtkSmartPointer<vtkAppendPolyData>> elementAppenders;
static std::map<std::string, std::vector<vtkSmartPointer<vtkTransform>>> elementTransforms;
static std::map<std::string, std::vector<ImVec4>> elementColors;
static std::map<std::string, std::vector<size_t>> elementAtomIndices;
```

### 2.2 AtomInfo의 레거시 VTK 객체들
```cpp
// AtomInfo 구조체에서 제거할 멤버들
vtkSmartPointer<vtkActor> actor;
vtkSmartPointer<vtkSphereSource> sphereSource;
vtkSmartPointer<vtkPolyDataMapper> mapper;
```

### 2.3 BondInfo의 레거시 VTK 객체들
```cpp
// BondInfo 구조체에서 제거할 멤버들
vtkSmartPointer<vtkActor> actor1;
vtkSmartPointer<vtkActor> actor2;
```

### 2.4 복잡한 배치 모드들
- `beginAtomUpdates/endAtomUpdates`
- `beginBondUpdates/endBondUpdates`
- `beginAtomOnlyUpdates/endAtomOnlyUpdates`
- `beginRenderingUpdates/endRenderingUpdates` (간소화)

## 3. 단계별 작업 계획

### 🎯 Phase 1: 레거시 함수 제거 및 통합 (High Priority)

#### 3.1 원자 관리 함수 통합
```cpp
// 제거할 함수들
- initializeElementGroup() ❌
- updateElementGroup() ❌
- updateElementGroupBatchAware() ❌
- removeAtomFromGroup(symbol, instanceIndex) ❌
- updateAtomInGroup() ❌
- clearAllAtomGroups() ❌

// 남길 통합 함수들 (atomGroups 기반)
- initializeAtomGroup() ✅
- updateAtomGroup() ✅
- addAtomToGroup() ✅
- removeAtomFromGroup(symbol, atomId) ✅
```

#### 3.2 배치 시스템 간소화
```cpp
// 제거할 배치 관련 멤버/함수들
- beginRenderingUpdates() ❌
- endRenderingUpdates() ❌
- 복잡한 batchUpdateMode 로직 ❌

// 남길 핵심 배치 시스템
- beginBatch() ✅
- endBatch() ✅
- BatchGuard ✅
- isBatchMode() ✅
```

#### 3.3 AtomInfo/BondInfo 구조체 정리
```cpp
// AtomInfo에서 제거
struct AtomInfo {
    // 제거 대상
    vtkSmartPointer<vtkActor> actor;           // ❌
    vtkSmartPointer<vtkSphereSource> sphereSource; // ❌
    vtkSmartPointer<vtkPolyDataMapper> mapper; // ❌
    
    // 유지 대상
    uint32_t id = 0;           // ✅
    AtomType atomType = AtomType::ORIGINAL; // ✅
    bool isInstanced = true;   // ✅ (항상 true로 설정)
    size_t instanceIndex = SIZE_MAX; // ✅
    // ... 기타 핵심 데이터들
};

// BondInfo에서 제거
struct BondInfo {
    // 제거 대상
    vtkSmartPointer<vtkActor> actor1;  // ❌
    vtkSmartPointer<vtkActor> actor2;  // ❌
    
    // 유지 대상
    uint32_t id = 0;                   // ✅
    bool isInstanced = true;           // ✅ (항상 true로 설정)
    std::string bondGroupKey;          // ✅
    size_t instanceIndex = SIZE_MAX;   // ✅
    BondType bondType = BondType::ORIGINAL; // ✅
    // ... 기타 핵심 데이터들
};
```

### 🎯 Phase 2: 렌더링 시스템 통합 (Medium Priority)

#### 3.4 원자 렌더링 시스템 통합
```cpp
// atoms_template.cpp에서 제거할 전역 변수들
static std::map<std::string, vtkSmartPointer<vtkPolyData>> elementBaseGeometries; // ❌
static std::map<std::string, vtkSmartPointer<vtkActor>> elementGroupActors; // ❌
static std::map<std::string, vtkSmartPointer<vtkPolyDataMapper>> elementGroupMappers; // ❌
static std::map<std::string, vtkSmartPointer<vtkAppendPolyData>> elementAppenders; // ❌
static std::map<std::string, std::vector<vtkSmartPointer<vtkTransform>>> elementTransforms; // ❌
static std::map<std::string, std::vector<ImVec4>> elementColors; // ❌
static std::map<std::string, std::vector<size_t>> elementAtomIndices; // ❌

// 유지할 통합 시스템
static std::map<std::string, AtomGroupInfo> atomGroups; // ✅
```

#### 3.5 createAtomSphere() 함수 간소화
```cpp
void AtomsTemplate::createAtomSphere(const char* symbol, const ImVec4& color, float radius, 
    const float position[3], AtomType atomType) {

    // 🔧 간소화: 레거시 elementTransforms 제거
    // ❌ elementTransforms[symbolStr].push_back(transform);
    // ❌ elementColors[symbolStr].push_back(adjustedColor);
    // ❌ elementAtomIndices[symbolStr].push_back(...);

    // ✅ 통합 시스템만 사용
    initializeAtomGroup(symbolStr, radius);
    addAtomToGroup(symbolStr, transform, adjustedColor, atomId, atomType, originalIndex);
    
    // ✅ AtomInfo는 항상 인스턴스 렌더링
    AtomInfo atomInfo(symbolStr, adjustedColor, radius, position, atomType);
    atomInfo.isInstanced = true;  // 항상 true
    atomInfo.actor = nullptr;     // 항상 null
    atomInfo.sphereSource = nullptr; // 항상 null
    atomInfo.mapper = nullptr;    // 항상 null
}
```

#### 3.6 createBond() 함수 간소화
```cpp
void AtomsTemplate::createBond(const AtomInfo& atom1, const AtomInfo& atom2, BondType bondType) {
    // ✅ 통합 시스템만 사용
    addBondToGroup2Color(bondTypeKey, transforms.first, transforms.second, 
                        adjustedColor1, adjustedColor2, bondId);
    
    BondInfo bondInfo;
    bondInfo.isInstanced = true;  // 항상 true
    bondInfo.actor1 = nullptr;    // 항상 null  
    bondInfo.actor2 = nullptr;    // 항상 null
    // ... 나머지 로직
}
```

### 🎯 Phase 3: 코드 클린업 (Low Priority)

#### 3.7 헤더 파일 정리 (atoms_template.h)
```cpp
// 제거할 deprecated 함수들
void beginAtomUpdates() ❌
void endAtomUpdates() ❌
void beginBondUpdates() ❌
void endBondUpdates() ❌
void beginAtomOnlyUpdates() ❌
void endAtomOnlyUpdates() ❌
void beginRenderingUpdates() ❌
void endRenderingUpdates() ❌
void forceBatchUpdateEnd() ❌
bool isBatchUpdateMode() ❌

// 제거할 레거시 전역 함수들
void initializeElementGroup(const std::string& symbol, float radius) ❌
void updateElementGroup(const std::string& symbol) ❌
void updateElementGroupBatchAware(const std::string& symbol) ❌
void removeAtomFromGroup(const std::string& symbol, size_t instanceIndex) ❌
void updateAtomInGroup(...) ❌
void clearAllAtomGroups() ❌
```

#### 3.8 함수별 간소화 작업
```cpp
// updateAtomGroup() - 레거시 동기화 로직 제거
void updateAtomGroup(const std::string& symbol) {
    // ❌ 제거: elementTransforms 동기화
    // ❌ 제거: updateElementGroup() fallback
    
    // ✅ 유지: atomGroups 기반 렌더링만
    if (atomGroups.find(symbol) != atomGroups.end()) {
        const auto& group = atomGroups[symbol];
        // 통합 렌더링 로직만...
    }
}

// clearAllBonds() - 개별 액터 제거 로직 제거  
void AtomsTemplate::clearAllBonds() {
    // ❌ 제거: 개별 액터 제거 루프
    // for (const BondInfo& bond : createdBonds) {
    //     if (bond.actor1) VtkViewer::Instance().RemoveActor(bond.actor1);
    //     if (bond.actor2) VtkViewer::Instance().RemoveActor(bond.actor2);
    // }
    
    // ✅ 유지: 그룹 기반 정리만
    clearAllBondGroups();
    createdBonds.clear();
    surroundingBonds.clear();
}

// hideSurroundingAtoms() - 개별 액터 제거 로직 제거
void AtomsTemplate::hideSurroundingAtoms() {
    // ❌ 제거: 개별 액터 제거
    // if (atom.actor) { VtkViewer::Instance().RemoveActor(atom.actor); }
    
    // ✅ 유지: 통합 그룹에서만 제거
    for (const auto& atom : surroundingAtoms) {
        removeAtomFromGroup(atom.symbol, atom.id);
    }
}
```

## 4. 구체적 작업 순서

### 단계 1: 기본 정리 (1-2일)
1. **AtomInfo/BondInfo 구조체 정리**
   - VTK 개별 객체 멤버 제거
   - `isInstanced = true` 강제 설정
   - 생성자에서 VTK 객체 초기화 제거

2. **deprecated 함수들 제거**
   - `atoms_template.h`에서 deprecated 섹션 완전 삭제
   - 컴파일 에러 발생 시 호출 부분 수정

### 단계 2: 렌더링 시스템 통합 (2-3일)
3. **레거시 전역 변수 제거**
   - `elementTransforms`, `elementColors` 등 전역 맵 삭제
   - `elementGroupActors`, `elementAppenders` 등 VTK 객체 맵 삭제

4. **함수들 간소화**
   - `createAtomSphere()`: elementTransforms 동기화 제거
   - `updateAtomGroup()`: 레거시 시스템 fallback 제거
   - `clearAllBonds()`, `hideSurroundingAtoms()`: 개별 액터 처리 제거

### 단계 3: 배치 시스템 간소화 (1일)
5. **배치 API 간소화**
   - `beginRenderingUpdates/endRenderingUpdates` 제거
   - 복잡한 배치 모드 분기 로직 단순화
   - `BatchGuard`와 `beginBatch/endBatch`만 유지

### 단계 4: 테스트 및 검증 (1일)
6. **기능 검증**
   - XSF 파일 로딩 테스트
   - 원자 편집 모드 테스트
   - Boundary atoms 생성/제거 테스트
   - 결합 생성/제거 테스트
   - 배치 업데이트 성능 테스트

## 5. 예상 효과

### 5.1 코드 복잡성 감소
- **라인 수**: ~500-800 라인 감소 예상
- **전역 변수**: 7개 맵 → 2개 맵으로 축소
- **함수 수**: ~15개 레거시 함수 제거

### 5.2 성능 향상
- **메모리 사용량**: 중복 데이터 구조 제거로 ~30% 감소
- **렌더링 성능**: 단일 인스턴스 시스템으로 일관성 확보
- **배치 효율성**: 단순화된 배치 로직으로 오버헤드 감소

### 5.3 유지보수성 개선
- **단일 책임**: 각 시스템이 명확한 역할 분담
- **디버깅 용이성**: 이중 시스템 동기화 문제 해결
- **코드 가독성**: 복잡한 fallback 로직 제거

## 6. 리스크 및 대응방안

### 6.1 호환성 문제
- **리스크**: 기존 코드에서 레거시 함수 직접 호출
- **대응**: 컴파일 에러를 통한 점진적 수정

### 6.2 성능 회귀
- **리스크**: 일부 특수 상황에서 성능 저하 가능
- **대응**: 각 단계별 성능 측정 및 롤백 계획

### 6.3 렌더링 오류
- **리스크**: VTK 파이프라인 변경으로 인한 시각적 오류
- **대응**: 단계적 테스트 및 스크린샷 비교

## 7. 성공 지표

✅ **완료 기준**:
- [ ] 모든 레거시 전역 변수 제거 완료
- [ ] AtomInfo/BondInfo에서 VTK 개별 객체 제거 완료  
- [ ] 배치 시스템이 `beginBatch/endBatch` + `BatchGuard`만 사용
- [ ] 모든 원자/결합이 CPU 인스턴싱 모드로 렌더링
- [ ] 기존 기능 100% 동작 (XSF 로딩, 편집, Boundary atoms 등)
- [ ] 메모리 사용량 30% 이상 감소
- [ ] 코드 라인 수 500+ 라인 감소

이 계획을 통해 AtomsTemplate을 **현대적이고 유지보수 가능한** 단일 시스템으로 진화시킬 수 있습니다.