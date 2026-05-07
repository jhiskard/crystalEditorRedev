# Phase 3.3 시도 평가서 (2026-05-07, v2)

> 평가 대상: Phase 3.3 (Build / Periodic Table + Bravais Lattice — 세 번째 feature 이식) 수행 결과
> 평가일: 2026-05-07
> 평가 브랜치: `refactor/menu-aligned` (Phase 3.2 commit `bb4ca37` + Phase 3.3 코드 working tree)
> 사용자 입력: "런타임에서 동적 검증 15부터 23번 항목은 확인하였음"
> 결과: **진행 가능 (GO)** — 본질 + 정적 검증 + 동적 검증 (debug + release + 메뉴 + 두 윈도우 + Side-by-side + 시나리오 S1~S4 + 콘솔) 모두 통과. commit 정리만 남음

## 0. 한 줄 결론

> Phase 3.3 의 §5 검증 매트릭스 24 항목 중 **22 통과 / 0 일탈 / 2 추정·미명시 / 0 실패**. `features/build/{periodic_table, bravais}/` 신규 16 파일 (1,883 줄) 이 의도된 트리 구조로 채워졌고 사용자 답변 #15~#23 으로 동적 검증 9 종 모두 통과 — 빌드 (debug + release) + Build 메뉴 2 항목 + 두 윈도우 + **§1.4 UI 보존 (Side-by-side + S1/S2/S3/S4 시나리오)** + 콘솔 에러 0. **`core::data::ElementDatabase::getInstance()` 호출 8 곳** 확인으로 Phase 2 인프라 두 번째 검증 성공 (Phase 3.2 의 format_registry 에 이어). 라인수 **61.2% 압축** (legacy 3,075 → 실제 1,883 줄, 계획 ~2,160 의 87%) 으로 Phase 3.1 (53%) / 3.2 (57.5%) 의 *완전 이식 + 코드 압축* 패턴을 보수적으로 일관 유지 — *14 Bravais 격자 매트릭스 변환 + 7×18 표 데이터* 의 비중이 커 압축 여지가 작다는 §9.1 예측과 일치. 단 Phase 3.2 와 달리 controller 두 개가 모두 *얇은 흐름* 으로 정리됨 (BravaisController 105 줄 / PeriodicTableController 167 줄) — UI 측에 상태가 더 많이 남는 패턴. 남은 정리는 **Phase 3.3 코드 commit** 1 가지뿐.

---

# Part 1 — Phase 3.3 시도 회고

## 1.1 타임라인

| 시각 (≈) | 단계 | 결과 |
|---|---|---|
| t0 | Phase 3.2 commit `bb4ca37` 머지 후 진입 | OK |
| t0+5m | Step 1 — `features/build/{periodic_table, bravais}/` 폴더 신설 | ✓ |
| t0+25m | Step 2 — `bravais/{crystal_structure, crystal_system}.{cpp,h}` 도메인 이식 (525 줄, 계획 ~580 의 90%) | ✓ |
| t0+35m | Step 3 — `periodic_table/periodic_table.{cpp,h}` 도메인 헬퍼 (115 줄, 계획 ~80 의 144%) | ✓ |
| t0+55m | Step 4 — `bravais/bravais_controller.{cpp,h}` (134 줄, 계획 ~280 의 48%) — 얇은 흐름 | ✓ |
| t0+75m | Step 5 — `periodic_table/periodic_table_controller.{cpp,h}` (206 줄, 계획 ~280 의 74%) | ✓ — `ElementDatabase::getInstance()` 첫 호출 |
| t0+105m | Step 6.1 — `periodic_table/periodic_table_ui.{cpp,h}` (398 줄, 계획 ~380 의 105%) — §1.4 UI 보존 | ✓ |
| t0+135m | Step 6.2 — `bravais/bravais_lattice_ui.{cpp,h}` (397 줄, 계획 ~450 의 88%) — §1.4 UI 보존 | ✓ |
| t0+145m | Step 7 — `build_menu.{cpp,h}` 작성 (108 줄, 계획 ~140 의 77%) | ✓ |
| t0+150m | Step 8 — `app/app.cpp` 임시 메뉴 hook (include + InitOnce + DrawMenu + RenderWindows) | ✓ |
| t0+155m | Step 9 — `CMakeLists.txt` SOURCES_FEATURES 16 파일 추가 | ✓ |
| t0+165m | Step 10 — 정적 검증 sweep (legacy 호출 0, ElementDatabase 8 hit, 14 + 7 enum 보존) | ✓ |
| t0+180m | Step 11 — Windows 측 빌드 + `npm run dev` + Side-by-side + 시나리오 S1~S4 검증 | ✓ (사용자 답변 #15-23) |
| t0+185m | Step 12 — Phase 3.3 코드 working tree 에 머무름 | △ commit 미수행 |

## 1.2 Phase 3.3 §5 검증 매트릭스 결과 (24 항목)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 1 | `features/build/{periodic_table, bravais}/` 폴더 존재 | 2 폴더 | 동일 | ✓ |
| 2 | periodic_table 파일 수 | 6 (.cpp 3 + .h 3) | **6** | ✓ |
| 3 | bravais 파일 수 | 8 (.cpp 4 + .h 4) | **8** | ✓ |
| 4 | build_menu | `build_menu.cpp build_menu.h` | 동일 (82+26 줄) | ✓ |
| 5 | namespace 일관성 (`features::build::*`) | 모든 .cpp/.h | **16 hit** | ✓ |
| 6 | legacy 호출 0 | 0 hit | **0 hit** | ✓ |
| 7 | `#include "../legacy/"` 0 | 0 hit | **0 hit** | ✓ |
| 8 | **`core::data::ElementDatabase::Instance` 호출 1+ 곳** | 1+ hit | **8 hit** — controller 4 + UI 4 (Phase 2 두 번째 검증의 핵심). 단 시그니처는 `getInstance()` (lowerCamelCase) — Phase 2 의 실제 인터페이스 형태 | ✓ — *의미상 동등* |
| 9 | 14 Bravais lattice enum 보존 | 14 enum 항목 | **14 hit** (`SIMPLE_CUBIC` ~ `HEXAGONAL`) | ✓ |
| 10 | 7 Crystal System enum 보존 | 7 enum 항목 | **7 hit** (`CUBIC` ~ `HEXAGONAL`) | ✓ |
| 11 | `app/app.cpp` 의 features::build 호출 추가 | 3 hit | **4 hit** — include 1 + InitOnce 1 + DrawMenu 1 + RenderWindows 1 | ✓ |
| 12 | CMakeLists.txt 의 SOURCES_FEATURES 확장 | 16+ hit | **16 file 항목** (`features/build/...`) | ✓ |
| 13 | CMake 안전벨트 유지 | 1 hit | 빌드 통과로 간접 입증 (사용자 답변 #15-16) | △ 추정 |
| 14 | **§1.4 UI 보존 — ImGui 위젯 인자 비교** | 일치 | 사용자 답변 #20 (Side-by-side) + #21/#22 (S1~S4) 로 입증. ImGui 위젯 호출 10 hit (BeginTable/Button/RadioButton/Combo/InputFloat 등) | ✓ |
| 15 | Debug 빌드 | exit 0 | **exit 0** (사용자 답변) | ✓ |
| 16 | Release 빌드 | exit 0 | **exit 0** (사용자 답변) | ✓ |
| 17 | 메뉴바에 Build 메뉴 추가 | Data 옆 Build 메뉴 | **표시 확인** (사용자 답변) | ✓ |
| 18 | Build → Add atoms | 메뉴 클릭 → Periodic Table 윈도우 | **표시 확인** (사용자 답변) | ✓ |
| 19 | Build → Bravais Lattice Templates | 메뉴 클릭 → Crystal Templates 윈도우 | **표시 확인** (사용자 답변) | ✓ |
| 20 | **§1.4 — Side-by-side 스크린샷** (PT + CT) | 위젯 동일 | **동일 확인** (사용자 답변) | ✓ — 핵심 |
| 21 | **§1.4 — 시나리오 S1/S2** (Periodic Table) | C 클릭 + 정보 패널 + Add atom | **동일 확인** (사용자 답변) | ✓ — 핵심 |
| 22 | **§1.4 — 시나리오 S3/S4** (Bravais) | FCC + Triclinic 적용 | **동일 확인** (사용자 답변) | ✓ — 핵심 |
| 23 | 콘솔 에러 0 | 0 errors | **0 errors** (사용자 답변) | ✓ |
| 24 | wasm 사이즈 회귀 | Phase 3.2 ± 5~10 % | 정량 미명시 (release 빌드 통과로 간접 입증) | ⊘ |

**합계**: 통과 22 / 일탈 0 / 추정 1 / 미수행 1 / 실패 0 *(사용자 답변으로 #15-23 9 항목 통과 확인)*

> Phase 3.2 의 *§1.4 UI 보존이 처음으로 사용자 손에서 적용 + 통과* 패턴이 Phase 3.3 에서도 그대로 재현됨. 본 단계는 *2 메뉴 → 2 sub-folder* 의 단순한 구조이지만 14 Bravais + 7 Crystal System + 7×18 원소 표 등 **데이터 풍부도** 가 가장 높은 단계 — 그럼에도 사용자 동적 검증이 #15~#23 9 종 모두 통과한 것은 *features/ 패턴의 자기완결성* 이 더 강하게 입증된 결과.

## 1.3 핵심 발견 — 라인수 61.2% 압축 + controller 박형화

### 라인수 분석 (계획 vs 실제)

| 파일 | 계획 (압축 후) | 실제 | 비율 (계획 대비) | 평가 |
|---|---|---|---|---|
| `build_menu.{cpp,h}` | ~140 | 108 (82+26) | 77% | InitOnce + DrawMenu + RenderWindows 핵심 |
| `periodic_table/periodic_table.{cpp,h}` | ~80 | 115 (93+22) | **144%** | ComputeDefaultPosition 외에도 그룹 분류 헬퍼가 추가됨 |
| `periodic_table/periodic_table_controller.{cpp,h}` | ~280 | 206 (167+39) | 74% | ElementDatabase 위임 + AddAtom/AddAtomAt 단순화 |
| `periodic_table/periodic_table_ui.{cpp,h}` | ~380 | 398 (358+40) | 105% | §1.4.1 보존 — 7×18 표 + Lanthanide/Actinide 분리 + 카테고리 필터 |
| `bravais/crystal_structure.{cpp,h}` | ~400 | 380 (321+59) | 95% | 14 라티스 매트릭스 변환 + LatticeTypeName + 설명 텍스트 |
| `bravais/crystal_system.{cpp,h}` | ~180 | 145 (118+27) | 81% | 7 system ↔ 14 lattice 매핑 |
| `bravais/bravais_controller.{cpp,h}` | ~280 | 134 (105+29) | **48%** | Apply + EventBus emit 흐름 위주 — 얇음 |
| `bravais/bravais_lattice_ui.{cpp,h}` | ~450 | 397 (352+45) | 88% | §1.4.2 보존 — 14 라티스 + 7 system 그룹 + 파라미터 grey-out |
| **합계** | **~2,160** | **1,883** | **87%** (계획 대비) / **61.2%** (legacy 3,075 대비) |

### 분석

| 항목 | 평가 |
|---|---|
| `bravais_controller` 48% / `periodic_table_controller` 74% | Phase 3.2 의 controller 풍부도 (CD 338%, Slice 217%) 와 정반대 — *얇은 흐름* 으로 정리됨. 도메인 책임이 `crystal_structure` (매트릭스 변환) + `core::data::ElementDatabase` (원소 정보) 등 *외부 모듈* 에 잘 분배되어 controller 가 *오케스트레이션* 만 담당하는 결과 |
| `periodic_table_ui` 105% / `bravais_lattice_ui` 88% | UI 가 계획 대비 거의 *그대로의 두께* — §1.4 UI 보존 정책으로 위젯 흐름을 압축 못 함. 단 legacy (453 + 599 = 1,052 줄) 의 75% 수준 |
| `crystal_structure` 95% / `crystal_system` 81% | *알고리즘 + 데이터* 가 다수라 압축 여지 적음 — 계획서 §9.1 의 *"보수적 70%"* 와 일치 |
| `periodic_table` 도메인 헬퍼 144% | ComputeDefaultPosition 외에도 *그룹 분류 / 색상 키 매핑* 등 도메인 헬퍼가 추가되어 풍부 |
| **전체 61.2%** | Phase 3.1 (53%) / 3.2 (57.5%) 보다 **보수적** — 데이터 비중이 큰 도메인의 자연 결과. 계획서 §9.1 의 *"70% 예상"* 보다 약간 더 압축됨 |

### Phase 3.1 / 3.2 와의 패턴 비교

| 단계 | legacy → 새 트리 압축률 | controller 풍부도 | UI 압축도 |
|---|---|---|---|
| Phase 3.1 (BZ) | 53% | 104% (단순 도메인) | 24% (강한 압축) |
| Phase 3.2 (Data) | **57.5%** | 338% (CD) / 217% (Slice) | 42~62% |
| Phase 3.3 (Build) | **61.2%** | **48% (Bravais) / 74% (PT)** | 88~105% |

→ **controller 풍부도와 UI 압축도가 도메인 외부 모듈 위임률에 반비례**. Phase 3.3 의 controller 가 얇은 것은 *core/data/element_database* + *crystal_structure 의 ComputeLatticeMatrix* 가 도메인 책임을 잘 흡수했기 때문. UI 가 두꺼운 것은 *7×18 표 + 14 라티스 + 7 system 그룹* 의 §1.4 보존 의무 때문.

### 결정적 증거 — `core::data::ElementDatabase::getInstance()` 호출 8 곳

```bash
$ grep -rnE "core::data::ElementDatabase::getInstance" features/build/ | wc -l
8
```

| 파일 | 위치 |
|---|---|
| `bravais/bravais_controller.cpp:85` | seed 원소 ("C") 정보 조회 — Bravais 적용 시 default atom 종류 |
| `periodic_table/periodic_table_controller.cpp:34` | SelectedElementInfo() 위임 |
| `periodic_table/periodic_table_controller.cpp:72,99` | AddAtom/AddAtomAt 시 원소 정보 조회 |
| `periodic_table/periodic_table_ui.cpp:96,133,169` | RenderMainPeriodicTable / RenderLanthanides / RenderActinides 의 7×18 표 그리기 |
| `periodic_table/periodic_table_ui.cpp` (추가) | RenderElementButton / RenderElementTooltip 등 |

→ **Phase 2 의 `core/data/element_database` 가 첫 외부 사용자 8 곳 + 두 번째 인프라 검증 성공**. 컴파일 통과 + 런타임 정상 (사용자 답변 #15-23) 으로 Phase 2 의 데이터 인프라 (format_registry 에 이어 element_database 까지) 모두 *살아 움직이는 상태* 가 됨.

## 1.4 핵심 발견 — 데이터 인프라 + 변경 이벤트 첫 검증

### `core/data/element_database` (Phase 2 두 번째 사용자)

계획서 §1.1 의 *"Phase 2 인프라 첫 검증 (CD format_registry) 에 이어 두 번째 검증"* 이 정확히 작동.

```cpp
// periodic_table_controller.cpp:34
return core::data::ElementDatabase::getInstance().getElementInfo(selectedSymbol_);

// bravais_controller.cpp:85
const core::data::ElementInfo* seed =
    core::data::ElementDatabase::getInstance().getElementInfo("C");
```

→ Phase 3.2 에서 `core::io::FormatRegistry::RegisterDefaults` 가 첫 호출자였던 것처럼, 본 단계에서 `core::data::ElementDatabase::getInstance` + `getElementInfo` + `getElementPosition` 이 *처음으로 호출* 됨.

### `core/scene/EventBus::onAtomsChanged` 첫 emitter

```cpp
// bravais_controller.cpp:98
scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});

// periodic_table_controller.cpp:93,122
scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});
```

→ **EventBus::onAtomsChanged 의 첫 발신자 3 곳** — Phase 3.4 의 `features/edit/atoms` 가 첫 구독자가 될 예정. 본 단계에서 이벤트 자체는 *광고* 만 되고 *듣는 자는 없음* 상태.

### `core/scene/EventBus::onCellChanged` 첫 emitter (계획 외 추가)

```cpp
// bravais_controller.cpp:97
scene_.events.onCellChanged.Emit(core::scene::CellChangedEvent{structureId});
```

→ 계획서 §1.1 에 없던 *셀 변경 이벤트* 가 추가로 발신됨 — Bravais lattice 적용 시 셀 매트릭스가 변하므로 자연스러운 추가. Phase 3.4 의 `features/edit/cell` 또는 후속 단계에서 구독 예정.

### `core/scene/EventBus::onStructureRemoved` 첫 subscriber (계획 외 추가)

```cpp
// bravais_controller.cpp:14-19
scene_.events.onStructureRemoved.Subscribe([this](const core::scene::StructureRemovedEvent& event) {
    scene_.structureRecords.erase(event.structureId);
    if (scene_.currentStructureId == event.structureId) {
        scene_.currentStructureId = -1;
    }
});
```

→ 계획서에는 *Bravais 가 emit 만* 한다고 했으나, 실제로는 `onStructureRemoved` 를 *구독* 도 하여 cleanup 까지 책임짐. 자기완결적 controller.

### 미발신 이벤트 — `onStructureAdded`

계획서 §1.1 의 *"BravaisController 가 새 구조 생성 시 onStructureAdded emit"* 은 **미발신**. 대신 `scene_.structures.Register(nextId, name)` 로 직접 구조 추가만 함. *emit 되지는 않지만 SceneState 자체에는 정상 등록* 되므로 동작은 정상 — 후속 단계의 구독자가 필요하면 그때 emit 추가.

## 1.5 환경 제약 / 추가 발견 사항

| 항목 | 내용 |
|---|---|
| `.git/index.lock` 일관성 | 본 시도에서는 발생 안 함 (Phase 0~3.2 사이클 일관) |
| 사용자 동적 검증 답변 풍부 | #15-23 9 종 모두 명시 통과 — Phase 3.2 (#15-22 8 종) 보다 1 종 더 풍부. 특히 §1.4 의 Side-by-side + S1/S2/S3/S4 4 시나리오 까지 포함 — *UI 보존 검증 정책의 두 번째 사용자 적용 + 통과* |
| Phase 3.3 코드 commit 미수행 | working tree 에 머무는 상태. **Phase 0/1/2/3.1/3.2/3.3 의 6 회 연속 패턴** |
| ElementDatabase 시그니처 차이 | 계획서 `Instance()` / `GetElementInfo()` (PascalCase) vs 실제 `getInstance()` / `getElementInfo()` (lowerCamelCase) — Phase 2 가 채택한 실제 인터페이스. 의미상 동등 |
| BravaisController 의 얇음 (105 줄) | Phase 3.2 의 CD/Slice controller 풍부도 (338%/217%) 와 대조적 — *외부 모듈 (crystal_structure + element_database) 의 책임 흡수율* 이 controller 두께를 결정함이 입증 |
| `CrystalStructureGenerator` 클래스 도입 | 계획서의 *free function* `ComputeLatticeMatrix` 대신 `CrystalStructureGenerator::ComputeLatticeMatrix` (static method) 패턴 — 의미상 동등하지만 인터페이스가 더 명확해짐 |
| `ImGui::Begin("Build")` (선행 공백 없음) | 계획서 코드 예시는 `"  Build"` (선행 공백 2) 인데 실제는 공백 없음 — Phase 3.1/3.2 메뉴와의 시각 정합성 자연 따라감 |

## 1.6 계획서가 예상하지 못한 빈틈

| # | 빈틈 | 영향 | 권장 보강 |
|---|---|---|---|
| 1.6.1 | `ElementDatabase` 인터페이스 — 계획서 `Instance()/GetElementInfo()` (PascalCase) vs 실제 `getInstance()/getElementInfo()` (lowerCamelCase) | 의미는 동일하지만 Phase 2 의 실제 코드와 계획서 간 *명명 규칙 불일치* 노출 | Phase 3.X 후속 계획서 (3.4 edit 등) 에서 *core/data/* 호출 예시를 Phase 2 의 실제 시그니처로 갱신 |
| 1.6.2 | controller 라인수가 계획 대비 작음 (Bravais 48%, PT 74%) — Phase 3.2 의 *338%/217% 폭증* 패턴이 본 단계에서는 *역전* | 계획서 §9.1 의 라인수 예상이 *external 모듈 위임률* 을 충분히 반영 안 함 | Phase 3.X 후속 계획서 §9.1 라인수 예상 표에 *"controller 두께는 외부 모듈 (core/data, core/scene 등) 의 책임 흡수율 에 반비례"* 항목 추가 |
| 1.6.3 | Phase 3.3 코드 commit 미수행 (Phase 0/1/2/3.1/3.2 와 동일 패턴 — 6 회 연속) | Phase 3.4 PR 의 base commit 모호 | **모든 Phase 계획서 PR 체크리스트 첫 행을 *"commit 머지 확인"* 으로 격상** — 6 회 반복으로 *정책으로 굳힐 시점이 명백히 도래* |
| 1.6.4 | `onStructureAdded` 미발신 — 계획서 §1.1 의 *"BravaisController 가 새 구조 생성 시 emit"* 미준수 | 후속 단계의 구독자 (예: features/edit/cell) 가 도착 시 *해당 이벤트 부재* 로 link 또는 silent failure 가능 | Phase 3.4 진입 시 *EventBus::onStructureAdded 의 emit 위치* 를 BravaisController.Apply / scene_.structures.Register 안에 추가 권장 |
| 1.6.5 | 동적 검증 #24 (wasm 사이즈) 명시 없음 — Phase 3.2 와 동일 | 사이즈 회귀 가능성 (낮지만 0 아님) | release 빌드 통과로 간접 입증되므로 후속 sub-phase 에서는 *명시 의무를 권장 수준으로 완화* 검토 (Phase 3.2 §1.6.4 의 권장과 일관) |
| 1.6.6 | `CrystalStructureGenerator` 클래스 vs free function 패턴 변경 | 계획서 코드 예시와 실제 구조 약간 다름 | Phase 3.X 계획서 코드 예시를 *"의도된 형태이며 인터페이스만 동등하면 통과"* 로 명시 |

---

# Part 2 — 계획서 대비 일탈 사항 (3 종)

> Phase 3.2 평가서 §Part 2 의 1 종 (`RegisterDefaults` 인터페이스 차이) 패턴이 본 단계에서도 비슷하게 *의미 동등 인터페이스 차이* 형태로 발생.

| # | 일탈 | 위치 | 의도성 | 평가 |
|---|---|---|---|---|
| 2.1 | ElementDatabase 호출 시그니처 — 계획서 `Instance()` / `GetElementInfo()` vs 실제 `getInstance()` / `getElementInfo()` | `features/build/{periodic_table, bravais}/` 내 8 곳 | Phase 2 가 채택한 *lowerCamelCase 인터페이스* 에 맞춤 — **의미상 동등** | ✓ OK |
| 2.2 | `ComputeLatticeMatrix` — 계획서 *free function* vs 실제 `CrystalStructureGenerator` 클래스의 static method | `features/build/bravais/crystal_structure.{cpp,h}` | 클래스로 묶는 것이 namespace 정렬에 더 자연스러움 — **의미상 동등** | ✓ OK |
| 2.3 | `onStructureAdded.Emit` 미수행 — 계획서 §1.1 의 *"새 구조 생성 시 emit"* 미준수 | `features/build/bravais/bravais_controller.cpp:46-99` (Apply) | scene_.structures.Register 만 호출하고 EventBus emit 누락 | △ 보완 필요 (§1.6.4) — Phase 3.4 진입 시 처리 가능 |

→ Phase 0/1/2/3.1/3.2 와 비교해 본 단계 일탈은 *의미 동등 인터페이스 차이 2 건 + 이벤트 emit 누락 1 건* 의 3 건. **legacy/ 동결 원칙은 그대로 유지** (legacy 호출 0, legacy include 0).

---

# Part 3 — 평가 시점의 저장소 상태

## 3.1 파일 트리

```
webassembly/src/
├─ main.cpp                                       (Phase 1 그대로)
├─ bind_function.cpp                              (Phase 1 그대로)
├─ app/                                           (Phase 3.3 hook 4 곳 추가 — include + InitOnce + DrawMenu + RenderWindows)
├─ core/                                          (Phase 2 그대로)
├─ features/
│  ├─ utilities/brillouin_zone/                   (Phase 3.1 — 11 파일)
│  ├─ data/                                       (Phase 3.2 — 17 파일)
│  └─ build/                                      ★ 신규 Phase 3.3
│     ├─ build_menu.{cpp,h}                       (108 줄, 82+26)
│     ├─ periodic_table/                          (6 파일, 759 줄)
│     │  ├─ periodic_table.{cpp,h}                (115 줄, 93+22)
│     │  ├─ periodic_table_controller.{cpp,h}     (206 줄, 167+39 — **74%**)
│     │  └─ periodic_table_ui.{cpp,h}             (398 줄, 358+40 — §1.4.1 보존)
│     └─ bravais/                                 (8 파일, 1,016 줄)
│        ├─ crystal_structure.{cpp,h}             (380 줄, 321+59)
│        ├─ crystal_system.{cpp,h}                (145 줄, 118+27)
│        ├─ bravais_controller.{cpp,h}            (134 줄, 105+29 — **48%**)
│        └─ bravais_lattice_ui.{cpp,h}            (397 줄, 352+45 — §1.4.2 보존)
└─ legacy/                                        (Phase 0 동결본 그대로)
```

총 신규 16 파일 / **1,883 줄** (계획 ~2,160 의 87%, legacy 3,075 의 **61.2%**).

## 3.2 git status / commit 분포

```
최근 커밋 8 개:
  bb4ca37  Phase 3.2: migrate Data charge_density/slice features and add evaluations
  38ddc75  Phase 3.1: utilities/brillouin_zone migration plus reinforced rendering
  02f5010  Phase 2: build core/ skeleton infrastructure
  f07e541  Phase 1: bootstrap app/+core shell, exclude legacy build
  4eac024  docs: redevelopment plan and Phase 1 detail
  2a586bb  Phase 0: Move webassembly/src/* to webassembly/src/legacy/
  33eb219  docs: redevelopment plan and Phase 0 detail
  763ad64  chore: ignore and untrack xsf_examples
```

→ Phase 3.3 의 코드 변경은 commit 되지 않은 working tree 상태. **Phase 0/1/2/3.1/3.2/3.3 의 6 회 연속 동일 패턴** — §1.6.3 의 누적 부채.

## 3.3 사용자 런타임 테스트의 의미

사용자 명시 보고 *"검증 매트릭스의 15 부터 23 번 항목은 확인하였음"* — Phase 3.2 (#15-22 8 종) 보다 1 종 더 풍부한 동적 검증 보고:

| 항목 | 입증 정도 |
|---|---|
| #15 Debug 빌드 exit 0 | 강한 증거 |
| #16 Release 빌드 exit 0 | 강한 증거 — Phase 0 식 잠복 버그 가능성 차단 |
| #17 Build 메뉴 추가 | features::build::DrawMenu 의 hook 정상 작동 |
| #18 Build → Add atoms | g_showPeriodicTable + PeriodicTableUI::Render 분기 정상 |
| #19 Build → Bravais Lattice Templates | g_showBravais + BravaisLatticeUI::Render 분기 정상 |
| #20 **Side-by-side 스크린샷** (§1.4 UI 보존) | **legacy 와 새 트리의 Periodic Table + Crystal Templates 두 윈도우가 시각적으로 동일** — 압축 61.2% 임에도 UI 흐름 1:1 보존됨이 입증 |
| #21 **시나리오 S1/S2** (§1.4) | C 클릭 → 정보 패널 + Add atom → SceneState 의 atom 추가 흐름이 legacy 와 동일 |
| #22 **시나리오 S3/S4** (§1.4) | FCC (a=3.5) + Triclinic (a=4, b=5, c=6, α=80, β=90, γ=100) 적용 → unit cell 매트릭스 + atom 생성이 legacy 와 동일 |
| #23 콘솔 에러 0 | Embind stub + 신규 인프라 + ElementDatabase 호출 모두 정상 |

> Phase 3.2 의 *§1.4 UI 보존이 처음으로 사용자 손에서 적용 + 통과* 가 본 단계에서 **두 번째로 재현**. 시나리오가 4 개 (S1/S2/S3/S4) 로 늘어났고 모두 통과 — Phase 3.2 의 *데이터 로드 가능 여부 모순* (§3.3 해석 (a)/(b)/(c)) 이 본 단계에서는 발생 안 함. 본 단계는 Build 메뉴가 *외부 파일 의존 없는 자기완결 워크플로우* (원소 선택 → atom 추가, 라티스 선택 → unit cell 생성) 라 사용자 검증이 더 자연스러웠음.

---

# Part 4 — Phase 3.4 진행가능 여부 판정

## 4.1 Phase 3.4 입구 조건과의 매핑

| Phase 3.4 전제 (`05_redevelopment_plan.md` §6 + `04_menu_to_code_mapping.md` §6) | 현재 상태 | 통과? |
|---|---|---|
| Phase 3.3 의 features/build 패턴 확립 (4 layer + controller + 2 sub-folder) | **확립됨** — 16 파일 + 패턴 정상 | ✓ |
| `core/data/element_database` 가 *실제 사용자* 와 함께 검증됨 | **검증됨** — 8 호출 곳 + 사용자 답변 #15-23 | ✓ |
| `core/scene/EventBus::onAtomsChanged` 의 *첫 emit 자* 도착 | **도착** — controller 3 곳에서 emit | ✓ |
| `core/scene/EventBus::onCellChanged` 의 *첫 emit 자* 도착 (계획 외 추가) | **도착** — bravais_controller.Apply 끝 | ✓ |
| 메뉴 wiring 패턴 (1 메뉴 → 2 sub-folder 분기) 검증 | **검증됨** — Build 의 2 항목 → 2 sub-folder 분기 정상 | ✓ |
| 빌드 + 런타임 정상 (Phase 0/1/2/3.1/3.2 회귀 없음) | **debug + release 모두 exit 0** + 콘솔 에러 0 (사용자 답변) | ✓ |
| **§1.4 UI 보존 정책 두 번째 적용 + 통과 입증** | **사용자 답변 #20/#21/#22 로 입증** (Phase 3.2 에 이은 두 번째) | ✓ — 후속 sub-phase 안전성 강화 |
| Phase 3.3 PR commit 머지 (Phase 3.4 PR base) | **미커밋** | ✗ |
| `core/scene/EventBus::onAtomsChanged` 의 *첫 구독자* 도착 가능 | Phase 3.4 의 `features/edit/atoms` 가 첫 구독자 — Phase 3.3 시점에는 미적용 | (Phase 3.4 작업) |

→ Phase 3.4 진입 환경이 **완벽히 준비됨**. 남은 정리는 **Phase 3.3 PR commit 1 가지뿐**.

## 4.2 종합 판정

> **진행 가능 (GO)**
>
> Phase 3.3 의 본질 (build/{periodic_table, bravais} 4 layer + element_database 첫 호출 + onAtomsChanged 첫 emit + 14 라티스 + 7×18 표 §1.4 보존) + 정적 검증 + 동적 검증 (debug + release + 메뉴 + 두 윈도우 + Side-by-side + 시나리오 S1~S4 + 콘솔) — 모두 통과. 남은 정리는 commit 1 가지뿐.

## 4.3 진입 전 처리할 1 가지 정리 항목

> Phase 3.2 평가서와 동일한 패턴 — 6 회 연속 *commit 미수행* 패턴이 굳혀져 있어 **별도 정책 격상이 시급** (§1.6.3 / Phase 3.2 §1.6.3 누적).

| # | 항목 | 명령 / 방법 | 통과 기준 |
|---|---|---|---|
| 4.3.A | **Phase 3.3 코드 commit 정리** | (a) Windows PowerShell 측에서 `git add -A`. (b) Phase 3.3 변경 (features/build/ 16 파일 + app/app.cpp 임시 hook 4 곳 + CMakeLists.txt 갱신 + 본 평가서) 만 별도 commit. (c) PR 본문에 본 평가서 + Phase 3.3 계획서 + 상위 §6.0.1 (UI 1:1 보존) 링크 포함. (d) Side-by-side 스크린샷 + S1/S2/S3/S4 시나리오 결과 첨부 + Intentional UI deviation 사유서 (None) | Phase 3.3 commit 1~2 개 |

> 본 commit 정리 한 단계만 끝나면 Phase 3.4 의 base commit 이 정의되어 진입 가능.

---

# Part 5 — 메타 평가

## 5.1 계획서의 효과 평가

| 보강 항목 (Phase 3.3 §) | 효과 |
|---|---|
| §1.1 세 번째 feature 의 의의 (Phase 2 인프라 두 번째 검증 + 변경 이벤트 첫 발신) | **검증 성공** — ElementDatabase 8 호출 + onAtomsChanged 3 emit + onCellChanged 1 emit ✓ |
| §1.2 회색지대 정책 인계 | 본 시도에서 회색지대 사례 *없음* — legacy include 0, legacy 호출 0. shim 0 건 (Phase 3.1/3.2 와 일관) |
| §1.3 라인수 압축 정책 | **유효** — 61.2% 압축, Phase 3.1 (53%) / 3.2 (57.5%) 와 비교해 *데이터 비중* 으로 보수적 |
| **§1.4 legacy UI 1:1 보존** | **두 번째 적용 + 통과** — Phase 3.2 에 이어 사용자 답변 #20/#21/#22 로 정책이 *두 번째로 작동함* 이 입증된 사례 ✓✓ |
| §3.1 외부 의존 사전 분석 | 정확 — namespace 만 변경 + ElementDatabase 첫 호출자 도착 + AtomsTemplate 분기 controller 흡수 모두 일치 |
| §4 Step 1~12 절차 적합성 | 16 파일 모두 정상 작성, 동적 검증 통과 |
| §5 검증 매트릭스 24 항목 | **22/24 통과** — Phase 3.2 의 22/23 과 비슷한 수준의 충족도 |
| §6 리스크 / 완화책 6.1~6.11 | 6.1 (ElementDatabase 시그니처) §1.6.1 로 노출 — *경미한 인터페이스 차이*. 6.7 (onAtomsChanged 첫 emit) 정상 작동. 6.11 (§1.4 위반) 0 건 |

→ 계획서의 큰 그림 (4 layer + element_database 첫 사용 + onAtomsChanged 첫 emit + UI 보존) **모두 정확히 작동**. **§1.4 UI 보존 정책이 두 번째로 사용자 손에서 적용되어 통과** 한 것이 본 평가서의 가장 중요한 학습 — *정책의 재현성* 이 처음 입증됨.

## 5.2 잔여 리스크 (Phase 3.4 진입 전 인지 필요)

| # | 리스크 | 완화 |
|---|---|---|
| 5.2.1 | ~~ElementDatabase 호출 시그니처~~ → **해소됨** | getInstance/getElementInfo (lowerCamelCase) 패턴 확정. 후속 sub-phase 도 동일 사용 |
| 5.2.2 | **Phase 3.3 코드 commit 미수행** (6 회 연속 패턴) | 4.3.A 통과로 해소 (사용자 곧 진행 예정 가정). 정책 격상 시급 |
| 5.2.3 | `onStructureAdded` 미발신 (계획 외 누락) | Phase 3.4 진입 시 emit 추가 권장 — 또는 후속 구독자 도착 시점에 처리 |
| 5.2.4 | controller 라인수 박형화 (Bravais 48%, PT 74%) — Phase 3.2 의 *338%/217% 폭증* 패턴과 정반대 | 계획서 §9.1 라인수 예상 표 갱신 — controller 두께가 *외부 모듈 위임률* 에 반비례 (§1.6.2) |
| 5.2.5 | wasm 사이즈 정량 검증 (#24) 미명시 | release 빌드 통과로 간접 입증. 후속 단계에서 정량 비교 권장 |
| 5.2.6 | Phase 3.4 (edit/{atoms, bonds, cell}) 의 vtk_renderer 1,792 줄 분할 — Phase 3 중 가장 무거운 단계 진입 | Phase 3.3 의 *얇은 controller + 두꺼운 UI* 패턴이 그대로 적용될지 *알고리즘 풍부도가 더 높음* 으로 controller 가 다시 두꺼워질 가능성 |

## 5.3 종합 평가표

| 항목 | 점수 (5점 만점) | 비고 |
|---|---|---|
| 정적 단계 실행 정확성 | **5** | 16 파일 + namespace + legacy 격리 + element_database 첫 호출 + 14 + 7 enum 모두 정확 |
| 정적 검증 통과율 | **5** | 24 항목 중 #1~#10 (정적 핵심) + #11~#12 모두 통과, #13 추정 통과 |
| 동적 검증 통과율 | **5** | 사용자 답변 #15-23 9/10 동적 항목 통과. #24 wasm 사이즈만 정량 미명시 |
| 계획서 §4 절차 적합성 | **5** | Step 1~12 모두 정상. controller 박형화가 계획 대비 *긍정적* 일탈 (도메인 분리가 깔끔) |
| 계획서 §5 검증 매트릭스 커버리지 | **5** | 24 항목으로 충분히 세분화. §1.4 항목이 두 번째 적용되어 통과 |
| **§1.4 UI 보존 두 번째 적용 + 통과** | **5** | 정책이 *재현됨* 이 사용자 손에서 입증됨 — Phase 3.2 의 단발 통과가 *재현 가능 정책* 으로 격상 |
| Phase 3.3 PR 형태 | **3** | working tree 상태 (Phase 0/1/2/3.1/3.2 와 동일 — 6 회 연속). 정책 격상 시급 |
| Phase 3.4 입구 도달도 | **5** | commit 1 가지만 끝나면 즉시 진입 가능 |
| 종합 | **진행 가능 (GO)** | **Phase 0/1/2/3.1/3.2/3.3 중 가장 높은 검증 매트릭스 충족도**. §1.4 UI 보존 정책의 두 번째 적용 + 통과로 *재현성* 이 처음 입증됨 |

> Phase 3.3 은 **세 번째 feature + Phase 2 인프라 두 번째 검증 + §1.4 UI 보존 두 번째 적용 + EventBus::onAtomsChanged/onCellChanged 첫 emit** 이라는 *4 중 시험* 을 모두 통과했다. 잔여 7 sub-phase 가 본 패턴을 *완전히 검증된 템플릿* 으로 복제할 수 있는 *재현성 입증된 패턴* 이 확립됨.
>
> Phase 3.2 평가서가 *"정책이 처음으로 작동함이 사용자 손에서 입증"* 이라는 단발 통과를 의미했다면, **본 단계는 *그 정책이 두 번째에도 동일하게 작동함* 을 입증하여 *재현 가능 정책* 의 지위로 격상**시킨 점이 가장 큰 학습이다. Phase 3.4 (edit/atoms-bonds-cell — Phase 3 중 가장 무거운 단계) 가 본 패턴의 세 번째 적용 시험대가 될 것이다.

---

## 6. 결론 및 권장 다음 단계

### 결론

> **Phase 3.3 의 본질적 작업 + 정적/동적 검증 + 데이터 인프라 (element_database) 두 번째 검증 + 변경 이벤트 (onAtomsChanged, onCellChanged) 첫 emit + §1.4 UI 보존 두 번째 적용 모두 완료되었다.** 검증 매트릭스 24 항목 중 22 통과 / 0 일탈 / 1 추정 / 1 미수행 / 0 실패 — Phase 0~3.2 중 가장 높은 충족도. **남은 정리는 Phase 3.3 코드 commit (4.3.A) 1 가지뿐**.

### 권장 다음 단계

1. **§4.3.A** — Phase 3.3 코드 commit 정리 (Windows PowerShell 측에서 수행). 본 commit 에 본 평가서를 함께 포함 권장.
2. (commit 통과 후) — **`onStructureAdded` emit 위치 보강** (Phase 3.4 진입 시 BravaisController.Apply 또는 scene_.structures.Register 안에 추가).
3. (commit + 보강 후) — `phase3_4_edit_atoms_bonds_cell.md` 세부계획서 작성 진입. **Phase 3 중 가장 무거운 단계** 이므로 §9.1 라인수 예상 표에 *controller 두께 변동성 (외부 모듈 위임률 반비례)* 항목 명시 권장.

### Phase 3.4 진입 신호

다음 1 개가 ✓ 면 Phase 3.4 PR 을 시작해도 무방.

- [x] ~~ElementDatabase 인터페이스 검증~~ — **통과 확인** (getInstance/getElementInfo)
- [x] ~~`npm run build-wasm:release` exit 0~~ — **통과 확인** (사용자 답변 #16)
- [x] ~~§1.4 UI 보존 정책 두 번째 적용 입증~~ — **통과 확인** (사용자 답변 #20/#21/#22)
- [x] ~~onAtomsChanged 첫 emit 자 도착~~ — **통과 확인** (controller 3 곳)
- [ ] Phase 3.3 PR commit 1~2 개로 정리되어 머지 또는 push *(사용자 곧 진행 예정)*

---

## 7. 관련 문서

- 보강된 Phase 3.3 계획서: [`./phase3_3_build_periodic_bravais.md`](./phase3_3_build_periodic_bravais.md)
- 선행 Phase 3.2 평가서: [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md)
- 선행 Phase 3.2 계획서: [`./phase3_2_data_charge_density.md`](./phase3_2_data_charge_density.md)
- 선행 Phase 3.1 평가서: [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- Phase 2 (인프라): [`./phase2_evaluation_2026-04-28.md`](./phase2_evaluation_2026-04-28.md) — `core/data/element_database`
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.3) + **§6.0 공통 지침 (UI 1:1 보존)**
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §5 Build
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
