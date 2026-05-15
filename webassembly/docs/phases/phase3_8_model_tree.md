# Phase 3.8 - Model Tree (Windows / Model Tree) 이식 세부계획서

> 상위 문서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.8) + §6.0 공통 지침
> 대상 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §2 features/model_tree + §3 5-진입점 컨벤션
> 메뉴 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §10 Windows + §12 메뉴 외 진입점 (Model Tree 우클릭)
> 메뉴 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §3, §5 (Windows / Model Tree)
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01 ~ UI-12)
> 선행 계획서: [`./phase3_7_viewer_v2.md`](./phase3_7_viewer_v2.md) v2.1.1
> **선행 평가서 (entry gate baseline)**: [`./phase3_7_evaluation_2026-05-15.md`](./phase3_7_evaluation_2026-05-15.md)
> 작성일: 2026-05-15
> 대상 브랜치: `refactor/menu-aligned2` 후속 작업 브랜치 (예: `refactor/model-tree`)
> 단위 PR: 1 개, 단일 sub-folder 중심 (`features/model_tree/`) + 필요 최소 core/edit/measurement/data 얇은 public API 추가
> 예상 소요: 2 ~ 3 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-15 | 초안 작성 — Phase 3.7 (2026-05-15) 평가서의 GO 판정과 4 종 DEVIATION (D1 ~ D4), 1 종 PARTIAL (D5 Distance factor 슬라이더), 인계 의무 (D1 사후 확정 / D3 viewer→measurement 호출 정리 / D5 후속 회귀 PR) 을 Phase 3.8 의 entry gate / 비목표로 반영. legacy 출처: `legacy/model_tree.{cpp,h}`, `legacy/atoms/atoms_template.cpp` 의 GetStructures / IsStructureVisible / SetStructureVisible / IsAtomVisibleById / SetAtomVisibilityForIds / IsAtomLabelVisibleById / SetAtomLabelVisibilityForIds / RemoveStructure / RemoveMeasurementsByStructure / SetCurrentStructureId 등 |

---

## 0. 한 줄 요약

> Phase 3.8 은 **`features/model_tree/`** 를 신설해 legacy 의 `Windows / Model Tree` 창을 복구한다. 본 단계는 Phase 3.7 까지 누적된 `core::scene::SceneState`, `StructureRegistry`, `SelectionSet`, `HoverInfo`, EventBus 의 *읽기 사용자* 가 되어 (a) 구조 트리, (b) 구조별 atom / atom-group / measurement / charge density 자식 노드 트리, (c) `Show` (visibility) 토글, (d) `Label` 가시성 토글, (e) `Count` 표시, (f) `Remove` 액션, (g) 삭제 확인 / 측정 클리어 확인 modal 을 재현한다.
>
> Phase 3.7 의 entry gate 인계 사항을 본 단계에서 *사후 확정 / 모니터링* 한다: (D1) Viewer 의 drag-rectangle 색상 (노란색 `IM_COL32(255, 216, 64, ...)`) deviation 을 *intentional UI deviation* 으로 확정 또는 파란색 복귀. (D2) `vtkWebAssembly*` + GLFW 직접 의존성을 의존성 다이어그램으로 추적. (D3) `features/viewer` → `features/measurement::RenderViewerOverlay` 직접 호출 경로는 Phase 4 인계 유지. (D5) `Distance factor` 슬라이더 `-50% ~ +50%` ↔ legacy `0.1 ~ 2.0` 차이는 *본 phase 범위 외* 로 분리하고 후속 회귀 PR 발의 권장.
>
> Mesh 트리 (`Mesh Tree section` 의 `Mesh` 인스턴스) 는 **Phase 3.9** `features/mesh` 범위로 분리한다. 본 단계의 Mesh Tree section 은 *placeholder / 빈 노드* 또는 *deferred* 안내로 유지한다.

---

# Part 1 - 목표 / 비목표 / Phase 3.7 entry gate 인계

## 1.1 목표

| # | 구분 | 항목 |
|---:|---|---|
| G1 | Windows / Model Tree 메뉴 | `Windows / Model Tree` 토글로 `Model Tree` ImGui 창 표시 / 숨김. close 버튼 ↔ 메뉴 체크박스 양방향 동기화 |
| G2 | Crystal Structure 트리 | `CollapsingHeader("Crystal Structure")` + `BeginTable` 5 컬럼 (Name / Show / Label / Count / Remove) 복원 |
| G3 | 구조 노드 | 각 structure 가 root 트리 노드 (`ICON_FA6_ATOM Name`) 로 표시. Show 컬럼의 eye icon 토글로 `core::scene::structures.SetVisible(id, bool)` 호출 |
| G4 | Atoms 자식 노드 | 각 structure 의 *Atoms* 노드 (`ICON_FA6_ATOM Atoms`) 가 펼쳐지면 *원소별 group* (`ICON_FA6_CIRCLE Symbol`) → *개별 atom row* 트리 표시 |
| G5 | 3-state 가시성 토글 | Atoms / 원소 group / 개별 atom 각각의 Show 컬럼은 *모두 가시 / 일부 가시 / 모두 비가시* 3-state (`ICON_FA6_EYE` 100% / 65% / `ICON_FA6_EYE_SLASH` 40%). 클릭 시 그룹 전체 토글 |
| G6 | Label 컬럼 | `IsAtomLabelVisibleById` / `SetAtomLabelVisibilityForIds` quick API 와 동등한 새 API 를 `features/edit/atoms` 가 노출 → 트리에서 토글 |
| G7 | Count 컬럼 | 구조 / 원소 group / 단일 atom 의 atom count 표시 |
| G8 | Remove 액션 | 구조 row 의 Remove 버튼 → "Delete Confirmation" modal (legacy 동일 문구) → `core::scene::structures.Remove(id)` + `StructureRemovedEvent` emit |
| G9 | Measurements 자식 노드 | 구조 안에 누적된 측정 (Distance / Angle / Dihedral / GeometricCenter / CenterOfMass) 을 type 별 sub-folder 로 표시. 개별 visibility 토글 (`MeasurementStore::SetVisible`) + Clear-all-by-structure 확인 modal |
| G10 | Charge Density / Slice 자식 노드 | `features::data::HasChargeDensity()` 가 true 이고 `boundStructureId` 가 매칭되면 Isosurface / Surface / Volumetric / Plane 항목 표시. 클릭 시 해당 Data 창 열기 (`Show(Mode)` 호출) |
| G11 | Selection 동기화 | 트리에서 atom row 클릭 시 `SelectAtom` event 또는 `SelectionSet.SelectByIds`. 반대로 `SelectionChangedEvent` 구독 → 트리 row highlight 갱신 |
| G12 | EventBus 구독 | `onStructureAdded` / `onStructureRemoved` / `onStructureVisibilityChanged` / `onAtomsChanged` / `onSelectionChanged` 구독으로 자동 갱신 |
| G13 | legacy 격리 유지 | `webassembly/src/legacy/` 미수정. 신규 파일에서 `legacy/` include 0 |
| G14 | 빌드 검증 | `npm run build-wasm:debug` + `npm run build-wasm:release` PASS |
| G15 | wasm size 추적 | 직전 baseline 15,266,894 B (Phase 3.7, 2026-05-15) 대비 delta 기록 |
| G16 | Phase 3.7 entry gate 처리 | D1 drag rectangle 색상 사후 확정, D3 viewer→measurement 호출 경로 *모니터링* (수정 없음, Phase 4 인계), D5 Distance factor 슬라이더는 비목표 (후속 PR) |

## 1.2 비목표

| 항목 | 사유 / 후속 phase |
|---|---|
| Mesh Tree section | `features/mesh` 가 Phase 3.9. 본 단계는 placeholder / deferred 안내 |
| Mesh actor 트리 자식 노드 | Phase 3.9 |
| Mesh Display Mode 트리 연결 | Phase 3.9 |
| `D5 Distance factor 슬라이더` 범위 복원 | 본 phase 외. 별도 Phase 3.4 추가 회귀 PR 또는 Phase 4 UI 정비 PR |
| `D3 features/viewer → features/measurement` 직접 호출 경로 정리 | Phase 4 MenuRouter 도입 시 *overlay slot registry* 또는 layer 의존성 결정. 본 phase 는 *모니터링* 만 |
| Bonds 자식 노드 | legacy 도 bonds 를 tree 에 *직접* 노출하지 않음 (Bonds Management 윈도우만 사용). 본 phase 도 동일하게 트리 외부 — Edit / Bonds 창에서 처리 |
| Cell 자식 노드 | legacy 도 cell 가시성을 tree 에 별도 노드로 표시하지 않음 (구조 자체의 visibility 와 묶임). 본 phase 동일 |
| drag-and-drop reordering | legacy 미지원 |
| 트리 다중 선택 / 다중 작업 | legacy 미지원. 단일 selection 만 |
| `MenuRouter` 정식 도입 | Phase 4 범위. 본 phase 는 `app/app.cpp` 임시 hook |
| `Settings` / 다른 `Windows` 메뉴 항목 | Phase 4 (`app/settings`, `app/layout_manager`) |
| Phase 3.4 / 3.5 / 3.7 의 코드 수정 | 원칙 금지. 단 (a) edit / measurement / data 의 *얇은 public quick API* 추가 (예: `IsAtomLabelVisible(id)`, `SetAtomLabelVisibility(ids, bool)`, `MeasurementsByStructure(id)`, `OpenChargeDensityViewer(structureId, mode)`) 는 허용. (b) measurement store 의 `RemoveByStructure(id)` 는 이미 존재 — 직접 사용 가능 |

## 1.3 Phase 3.7 평가서 인계 표

| Phase 3.7 항목 | 상태 | Phase 3.8 반영 |
|---|---|---|
| #14 ~ #32 런타임 PASS | 기 PASS | imported XSF / CHGCAR 위에서 트리 표시 / 토글 시나리오를 동일 입력으로 검증 |
| #33 ~ #45 입력 계약 PASS | 기 PASS | Ctrl + 클릭 선택이 트리 row highlight 와 동기화되는지 추가 검증 |
| #46 ~ #52 렌더 스타일 회귀 PASS | 기 PASS | 트리 토글로 visibility 변경 시 sphere / cylinder 가시화 변동을 시각 확인 |
| #53 `Distance factor` 슬라이더 PARTIAL | 미해소 | 본 phase 비목표. 후속 PR 인계 |
| D1 drag rectangle 색상 노란색 deviation | DEVIATION | **사후 확정 의무**: 본 phase 의 Part 5 UI deviation 표에 *intentional UI deviation* 으로 등록 또는 파란색 복귀 결정. 결정 결과는 Phase 3.8 평가서에 기록 |
| D2 `vtkWebAssembly*` + GLFW 의존성 | DEVIATION | 본 phase 는 의존성 신규 추가 *금지*. 정적 검증으로 비악화 확인 |
| D3 viewer → measurement 호출 | DEVIATION | 본 phase 도 *동일한 우회 패턴* 을 적용 가능 (예: model_tree 가 `features::data::Show(Mode)` 또는 `features::measurement::*` quick API 직접 호출). Phase 4 MenuRouter 도입 시 일괄 정리. 정적 검증 #10 으로 비악화 확인 |
| D4 EventBus 필드 확장 | DEVIATION | 본 phase 는 EventBus 구조체 *추가 확장 금지* 원칙. 새 event 가 필요하면 *deviation 등록* 후 명시 |
| D6 measurement overlay 위치 보강 | 의도된 보강 | 본 phase 는 동일 패턴으로 measurement overlay 의 안정성 유지 |

---

# Part 2 - 현재 상태와 gap 분석

## 2.1 현재 코드 상태 (2026-05-15 동결 시점)

| 영역 | 현재 상태 | gap |
|---|---|---|
| `core/scene/structure_registry` | `Register`, `Remove`, `Exists`, `IsVisible`, `SetVisible`, `List`, `Empty`, `Clear` 모두 존재 + `StructureAddedEvent` / `StructureRemovedEvent` / `StructureVisibilityChangedEvent` emit | 충분. 본 phase 는 *읽기 + Visibility 쓰기* 사용 |
| `core/scene/scene_state` | `currentStructureId`, `structures`, `structureRecords` map, `nextAtomId`, `nextBondId`, `selection`, `hover`, `events` 모두 존재 | 충분 |
| `core/scene/selection` | `SelectByIds`, `ContainsAtom` 등 | 충분 |
| `features/edit/atoms/atom_manager` | atom group 단위 가시성 제어가 *renderer level* 에 있음. label 가시성 API 는 *없음* | **gap**: `SetAtomVisibility(atomIds, visible)`, `IsAtomVisible(atomId)`, `SetAtomLabelVisibility(atomIds, visible)`, `IsAtomLabelVisible(atomId)` quick API 신설 필요 |
| `features/edit/atoms/atom_renderer` | label actor (`atomLabelActors_`) + `SyncAtomLabelActors` 가 존재 | label 데이터 source 가 *없음* — 본 phase 에서 `atom_manager` 에 label visibility flag 추가 (또는 `core::scene::AtomRecord` 에 `labelVisible` 필드 추가) |
| `features/measurement/measurement_store` | `MeasurementsForStructure(structureId)`, `Records()`, `SetVisible(measurementId, bool)`, `Remove(id)`, `RemoveByStructure(structureId)` 모두 존재 | 충분 |
| `features/data/data_menu` | `HasChargeDensity()`, `GetActiveChargeDensityName()`, quick animation API 모두 존재 | **gap**: `OpenViewer(Mode mode, int32_t structureId)` quick wrapper 추가 또는 `data_menu.cpp` 의 기존 `Request` 분기로 충분 |
| `features/file` (Phase 3.6) | imported structure 가 자동으로 `StructureRegistry::Register` + `structureRecords` 추가 | 충분. 본 phase 는 변경 0 |
| `features/viewer` (Phase 3.7) | Viewer texture 가시화 + Settings/Windows menu hook | 본 phase 는 *Windows 메뉴에 Model Tree 추가* 만 필요 (viewer_menu.cpp 의 `Windows` 메뉴에 한 행 추가) |
| `app/app.cpp` | Phase 3.7 의 viewer 임시 hook 패턴이 동일하게 적용 가능 | `features::model_tree::InitOnce/DrawMenuEntry/RenderWindows/Shutdown` hook 추가 |

## 2.2 legacy 참조 범위

| legacy 파일 | 참조할 내용 | 신규 위치 |
|---|---|---|
| `legacy/model_tree.{cpp,h}` | `Render` 진입점, `renderXsfStructureTable`, `renderDeleteConfirmPopup`, `renderClearMeasurementsConfirmPopup`, 3-state visibility icon (`renderLabelState`), 5 컬럼 layout, atom group / measurement / charge density sub-tree | `features/model_tree/model_tree.{cpp,h}` + `features/model_tree/model_tree_actions.{cpp,h}` |
| `legacy/atoms/atoms_template.cpp` | `GetStructures`, `IsStructureVisible`, `SetStructureVisible`, `RemoveStructure`, `IsAtomVisibleById`, `SetAtomVisibilityForIds`, `IsAtomLabelVisibleById`, `SetAtomLabelVisibilityForIds`, `RemoveMeasurementsByStructure`, `SetCurrentStructureId`, `SetChargeDensityStructureId`, `HasChargeDensity`, `GetChargeDensityStructureId`, `GetAtomCountForStructure` | `core::scene::StructureRegistry` (read), `core::scene::SceneState::structureRecords` (read), 신규 `features::edit::atoms::SetAtomVisibility(ids, bool)` 등, 신규 `features::data::OpenViewer(structureId, Mode)` 또는 직접 controller 호출 |
| `legacy/atoms/ui/atom_editor_ui.cpp` 의 *label 관련 부분* | label visibility 정책 | `features::edit::atoms::atom_manager` 에 `labelVisible` 추가 |
| `legacy/macro/singleton_macro.h` | DECLARE_SINGLETON | 사용 금지. 본 phase 는 정적 인스턴스 또는 features 의 InitOnce 패턴 사용 |
| `legacy/icon/...` | `ICON_FA6_EYE`, `ICON_FA6_EYE_SLASH`, `ICON_FA6_FOLDER_TREE`, `ICON_FA6_ATOM`, `ICON_FA6_CIRCLE`, `ICON_FA6_TRASH` 등 | 신규 트리의 `core/ui/icons/` 에 동일 매크로가 이미 존재. 직접 사용 |

## 2.3 핵심 설계 판단

| 판단 | 내용 |
|---|---|
| Model Tree 는 *순수 read-mostly UI* | atom / measurement / charge density 의 *상태 변경* 은 각 feature 의 *얇은 quick API* 를 호출. Model Tree 가 직접 atom 의 `selected` flag 나 measurement actor 를 만지지 않는다 |
| Window 메뉴 hook 은 *viewer_menu 와 분리* | Phase 4 MenuRouter 도입 시 `app/layout_manager` 가 통합 Windows 메뉴를 그리게 됨. 그 때까지 `viewer_menu` 의 Windows 메뉴에 *Model Tree* 항목을 한 행 추가 또는 `model_tree_menu` 가 별도 메뉴 entry 그리기. 본 phase 는 후자 (분리) 채택 — feature 단위 응집 유지 |
| Mesh Tree section 은 *deferred placeholder* | legacy 의 Mesh Tree section 코드 (`renderMeshTable`, `renderMeshTree`) 는 본 phase 에서 *복구하지 않음*. 빈 CollapsingHeader 또는 미표시. Phase 3.9 에서 `features/mesh` 가 자체 sub-tree 를 그리도록 *옵셔널 callback* hook 만 준비 |
| Selection 동기화 정책 | (a) 트리 atom row 클릭 → `EventBus::onAtomPicked` 와 동등한 event 직접 emit 은 *금지*. 대신 `features::edit::atoms::SelectAtomById(structureId, atomId, bool toggleMode)` quick wrapper 호출. (b) 반대 방향: `SelectionChangedEvent` 구독 → tree row 의 *visual highlight* 만 동기화 (트리 self-state 보유 금지). 단일 source of truth = `SceneState::selection` |
| EventBus 확장 금지 | Phase 3.7 의 D4 (event 필드 확장) 는 의도된 보강이지만 Phase 3.8 은 *추가 확장 없음*. 새 event 가 필요하면 deviation 등록 후 명시 |
| `D1` drag rectangle 색상 결정 | 본 phase 의 Part 5 § 5.4 *Intentional UI deviation 사전 등록* 표에 (a) **노란색 채택 — 사후 확정** 또는 (b) **파란색 복귀** 둘 중 하나로 명시. 권장: 사후 확정 (atom selection 색과의 시각 일관성 + 워크플로우 안정) |

---

# Part 3 - 대상 파일 구조

## 3.1 신규 파일 계획

| 파일 | 역할 | 예상 라인 |
|---|---|---:|
| `webassembly/src/features/model_tree/model_tree_menu.h` | Phase 3.8 public entrypoint 선언 (`InitOnce`, `DrawMenu`, `HandleRequest`, `RenderWindows`, `Tick`, `Shutdown`) | 35 ~ 50 |
| `webassembly/src/features/model_tree/model_tree_menu.cpp` | `Windows / Model Tree` 메뉴 hook + show/hide flag 관리 + `ModelTreePanel` 라이프사이클 | 100 ~ 140 |
| `webassembly/src/features/model_tree/model_tree_panel.h` | `ModelTreePanel::Render(bool* open)` 선언 | 30 ~ 50 |
| `webassembly/src/features/model_tree/model_tree_panel.cpp` | `ImGui::Begin("Model Tree")` + `BeginTable` 5 컬럼 + Crystal Structure section + Mesh placeholder + 삭제/측정 클리어 modal | 350 ~ 500 |
| `webassembly/src/features/model_tree/model_tree_actions.h` | tree action 캡슐: ToggleStructure, ToggleAtomGroup, ToggleAtom, ToggleLabel, RemoveStructure, OpenChargeDensityViewer, ClearMeasurements, SelectAtom | 50 ~ 70 |
| `webassembly/src/features/model_tree/model_tree_actions.cpp` | action 구현 — 각 quick API 호출 wrapper | 200 ~ 280 |
| `webassembly/src/features/model_tree/tree_state.h` | TreeNodeOpenState (구조 root / Atoms / 원소 group 등 펼침 상태 보존), PendingDelete / PendingClearMeasurements state, 3-state visibility helper | 60 ~ 90 |
| **합계 (신규)** | **7 파일** | **약 825 ~ 1,180** |

## 3.2 보강 대상 파일

| 파일 | 변경 내용 |
|---|---|
| `webassembly/src/features/edit/atoms/atom_manager.{h,cpp}` | `AtomRecord` 에 `labelVisible` 추가 또는 `atom_manager` 가 `std::unordered_set<uint32_t> labelVisibleAtomIds_` 보관. `IsAtomLabelVisible(atomId)`, `SetAtomLabelVisibility(structureId, atomIds, bool)` quick API. atom visibility 의 `IsAtomVisible(atomId)`, `SetAtomVisibility(structureId, atomIds, bool)` 도 신설. 기본값은 `visible=true`, `labelVisible=false` (legacy 동일) |
| `webassembly/src/features/edit/edit_menu.{h,cpp}` | 위 4 종 quick API public 노출 |
| `webassembly/src/features/edit/atoms/atom_renderer.cpp` | `SyncAtomLabelActors` 의 source 가 `atom_manager` 의 labelVisible flag 를 따르도록 갱신 (현재 source 가 명확하지 않으면 최소 보강) |
| `webassembly/src/features/measurement/measurement_menu.{h,cpp}` | quick API `MeasurementsByStructure(structureId)`, `SetMeasurementVisible(id, bool)`, `RemoveMeasurement(id)`, `ClearMeasurementsByStructure(structureId)` 노출 (existing `measurement_store` 를 wrap) |
| `webassembly/src/features/data/data_menu.{h,cpp}` | `OpenChargeDensityViewerForStructure(structureId, Mode)`, `IsChargeDensityBoundTo(structureId)` quick API 노출. mode 는 Isosurface / Surface / Volumetric / Plane |
| `webassembly/src/features/viewer/viewer_menu.cpp` | **변경 금지 우선**. 단 Windows 메뉴 안에 Model Tree 항목을 *추가하지 않음* (model_tree 가 자체 menu entry 그림) |
| `webassembly/src/app/app.cpp` | `features/model_tree` include, `InitOnce`, `DrawMenu`, `RenderWindows` hook 추가. *Phase 3.7 의 viewer hook 순서 이후* 에 배치 |
| `CMakeLists.txt` | 신규 7 파일 등록 |

## 3.3 예상 규모

| 구분 | 예상 |
|---|---:|
| 신규 `features/model_tree` 파일 | 7 |
| 보강 파일 (edit / measurement / data / app / CMake) | 8 ~ 12 |
| 신규 / 수정 라인 수 | 1,100 ~ 1,500 (atoms_template.cpp 의 ModelTree 호출자 부분이 신규 트리에서 lean 하게 재구성됨) |
| legacy 직접 참조 / include | 0 |
| `features/viewer` / `features/file` / `core/io` / `core/vtk` 변경 | 0 (Phase 3.6 / 3.7 비침범) |

---

# Part 4 - 아키텍처 세부 계획

## 4.1 호출 흐름

```text
app::App::Init()
  -> core::vtk::VtkViewer::Instance().Init()
  -> features::file::InitOnce(scene)
  -> features::utilities::bz::InitOnce(scene)
  -> features::data::InitOnce(scene)
  -> features::build::InitOnce(scene)
  -> features::edit::InitOnce(scene, mouseInteractor)
  -> features::measurement::InitOnce(scene, mouseInteractor)
  -> features::viewer::InitOnce(scene, mouseInteractor)
  -> features::model_tree::InitOnce(scene)       // ★ 신규

app::App::renderDockSpaceAndMenu()
  -> 메뉴바: Crystal Viewer | File | Edit | Build | Measurement | Data | Utilities | Settings | Windows
  -> features::viewer::DrawMenus()       // Settings / Viewer FPS Overlay + Windows / Viewer
  -> features::model_tree::DrawMenu()    // ★ 신규 — Windows / Model Tree 항목 추가
  -> ... 기타 기존 메뉴 ...

app::App::renderFrame()
  -> features::viewer::RenderWindows()
  -> features::model_tree::RenderWindows() // ★ 신규
  -> features::file::RenderWindows()
  -> ... 기타 기존 windows ...
```

> `viewer_menu` 의 `Windows` 메뉴와 `model_tree_menu` 의 `Windows` 메뉴는 ImGui 가 *같은 이름의 BeginMenu 를 자동 병합* 한다. 단 두 곳 모두 같은 frame 에서 `BeginMenu("Windows")` 를 호출하면 *최초 호출자만 메뉴 본체* 를 그린다. 따라서 본 phase 는 두 가지 옵션 중 하나 채택:
> - **옵션 A (권장)**: `viewer_menu::DrawMenus()` 안의 `Windows` 메뉴에 `Model Tree` 항목을 추가. 단 `viewer_menu` 가 *model_tree 의 show flag 와 toggle API* 를 알아야 함 → feature ↔ feature 직접 호출 deviation (Phase 3.7 의 D3 와 유사 패턴)
> - **옵션 B**: `model_tree_menu::DrawMenu()` 가 *별도의 top-level `Windows` 메뉴* 를 시도. ImGui 의 메뉴 병합은 시도되나 *분리 entry* 로 표시될 위험
> - **옵션 C (최종 권장)**: `viewer_menu::DrawMenus()` 가 *`features::model_tree::IsVisible() / SetVisible(bool)` 의 함수 포인터를 InitOnce 시 등록받는* registry 패턴. Phase 4 MenuRouter 가 등장할 때까지의 임시 hook. *옵션 A 의 진화* 형 — 메뉴 응집 유지 + feature 호출 명시
>
> 본 phase 는 **옵션 C** 를 채택. `viewer_menu` 에 `RegisterWindowMenuEntry(label, getter, setter)` 가 신설되어 `model_tree` 가 self-register 하는 형태. *명시적 deviation 으로 PR 본문 등록*.

## 4.2 책임 분리

| 계층 | 책임 | 금지 |
|---|---|---|
| `features/model_tree/model_tree_menu` | `Windows / Model Tree` 메뉴 hook (옵션 C registry 등록), show/hide flag 관리, panel 라이프사이클 | tree row 직접 렌더 금지 |
| `features/model_tree/model_tree_panel` | `Model Tree` ImGui 창 + `BeginTable` 5 컬럼 + Crystal Structure section + Mesh placeholder + 삭제 / 측정 클리어 modal | 데이터 직접 수정 금지 (모두 actions 경유) |
| `features/model_tree/model_tree_actions` | quick API 호출 wrapper (`features::edit::SetAtomVisibility`, `features::measurement::Remove`, `features::data::OpenViewer` 등) | feature 의 내부 객체 직접 접근 금지 |
| `features/model_tree/tree_state` | TreeNodeOpenState, PendingDelete, PendingClearMeasurements 등 *transient UI state* | scene state 보관 금지 (selection 등은 `SceneState` 단일 source 가 진실) |
| `core/scene` | 변경 없음 | (D4 EventBus 확장 금지 원칙) |
| `features/edit/atoms` | label visibility flag 추가 + quick API 노출 | tree UI 코드 금지 |
| `features/measurement` | quick API 노출 | tree UI 코드 금지 |
| `features/data` | quick API 노출 | tree UI 코드 금지 |
| `features/viewer` (Phase 3.7) | **변경 최소**. 단 옵션 C 의 registry helper 추가 | tree 직접 렌더 금지 |
| `features/file` / `core/io` / `core/vtk` (Phase 3.6 / 3.7) | **변경 0** | 정적 검증 항목 |

## 4.3 옵션 C registry helper 명세

```cpp
// features/viewer/viewer_menu.h 에 신규 추가
namespace features::viewer {

struct WindowsMenuEntry {
    std::string label;
    std::function<bool()> getter;   // 현재 표시 상태
    std::function<void(bool)> setter; // 표시 상태 변경
};

void RegisterWindowsMenuEntry(WindowsMenuEntry entry);

} // namespace features::viewer
```

`viewer_menu::DrawMenus()` 의 `Windows` 메뉴 안에서 *등록된 모든 entry* 를 순회하며 `MenuItem(label, nullptr, getter())` 처리. setter 호출.

`features/model_tree::InitOnce` 가 `features::viewer::RegisterWindowsMenuEntry({"Model Tree", [](){ return ...; }, [](bool v){ ... }})` 호출.

## 4.4 신규 quick API 명세

### 4.4.1 `features::edit` (atoms / label visibility)

```cpp
namespace features::edit {

bool IsAtomVisible(int32_t structureId, uint32_t atomId);
void SetAtomVisibility(int32_t structureId, const std::vector<uint32_t>& atomIds, bool visible);
bool IsAtomLabelVisible(int32_t structureId, uint32_t atomId);
void SetAtomLabelVisibility(int32_t structureId, const std::vector<uint32_t>& atomIds, bool visible);

void SelectAtomById(int32_t structureId, uint32_t atomId, bool toggle); // toggle=false → exclusive
void ClearStructureSelection(int32_t structureId);

} // namespace features::edit
```

### 4.4.2 `features::measurement`

```cpp
namespace features::measurement {

std::vector<MeasurementListItem> MeasurementsByStructure(int32_t structureId);
bool SetMeasurementVisible(uint32_t measurementId, bool visible);
bool RemoveMeasurement(uint32_t measurementId);
int  ClearMeasurementsByStructure(int32_t structureId);

} // namespace features::measurement
```

### 4.4.3 `features::data`

```cpp
namespace features::data {

bool IsChargeDensityBoundTo(int32_t structureId);
void OpenChargeDensityViewerForStructure(int32_t structureId, charge_density::Mode mode);
void OpenSliceViewerForStructure(int32_t structureId);

} // namespace features::data
```

## 4.5 EventBus 구독 정책

`features::model_tree::InitOnce` 가 다음 구독을 등록:

| Event | 핸들러 의도 |
|---|---|
| `onStructureAdded` | tree 갱신 (다음 frame 자동 반영) |
| `onStructureRemoved` | tree 갱신 + `PendingDelete` 상태 클리어 |
| `onStructureVisibilityChanged` | tree row 의 eye icon 갱신 |
| `onAtomsChanged` | atom group / count 갱신 |
| `onSelectionChanged` | row highlight 갱신 |

> Model Tree 는 *immediate-mode ImGui* 라서 매 frame 새로 그린다 — 구독은 필요 시 *show flag 강제 redraw* 또는 tree node open state 보존 정도에만 사용.

---

# Part 5 - Model Tree UI 계약 (legacy 1:1 보존 필수)

## 5.1 Model Tree 창 계약

| 항목 | legacy 기준 | Phase 3.8 v1 요구 |
|---|---|---|
| 창 title | `ICON_FA6_FOLDER_TREE  Model Tree` | 동일 |
| 메뉴 위치 | `Windows / Model Tree` | 동일 |
| 메뉴 라벨 | `Model Tree` | 동일 |
| close 버튼 동작 | close 시 메뉴 체크박스 해제 | 동일 (옵션 C registry 의 getter/setter 양방향 동기화) |
| 창 default 표시 | 사용자 설정 / layout 에 따라 다름 | 본 phase default = *hidden* (사용자가 명시적으로 열어야 함). legacy 와 동일 |
| 창 flags | `ImGuiWindowFlags_NoScrollbar` | 동일 |

## 5.2 Crystal Structure 섹션 계약

| 항목 | legacy 기준 | Phase 3.8 요구 |
|---|---|---|
| 섹션 헤더 | `CollapsingHeader("Crystal Structure", ImGuiTreeNodeFlags_DefaultOpen)` | 동일 |
| 표시 조건 | `structures` 가 비어 있지 않거나 `HasChargeDensity()` 인 경우 | 동일 |
| 테이블 | 5 컬럼: Name / Show / Label / Count / Remove | 동일 |
| 테이블 flags | `Resizable \| NoBordersInBodyUntilResize \| Reorderable \| PadOuterX \| BordersOuter \| SizingFixedFit` | 동일 |
| 컬럼 flags | Name = WidthStretch, 나머지 4 = WidthFixed | 동일 |
| 헤더 row | `TableHeader` (4 PushID 충돌 회피 100 offset) | 동일 |

## 5.3 트리 노드 계층

```text
Crystal Structure (CollapsingHeader)
├─ ICON_FA6_ATOM  Structure 1 (TreeNode root, OpenOnArrow + OpenOnDoubleClick + DefaultOpen + SpanFullWidth)
│  ├─ ICON_FA6_ATOM  Atoms (TreeNode, OpenOnArrow + OpenOnDoubleClick + DefaultOpen + SpanFullWidth)
│  │  ├─ ICON_FA6_CIRCLE  Si (TreeNode, OpenOnArrow + OpenOnDoubleClick + SpanFullWidth)
│  │  │  ├─ Si #1
│  │  │  ├─ Si #2
│  │  │  └─ ...
│  │  └─ ICON_FA6_CIRCLE  O (TreeNode)
│  │     └─ O #1, O #2, ...
│  ├─ Measurements (TreeNode, 측정이 있을 때만)
│  │  ├─ Distance (1)
│  │  │  └─ d#1: Si#1 ~ O#1 = 1.602 Å
│  │  ├─ Angle (1)
│  │  ├─ Dihedral (0)
│  │  ├─ GeometricCenter (0)
│  │  └─ CenterOfMass (0)
│  └─ Charge Density (HasChargeDensity && boundTo(structureId))
│     ├─ Isosurface
│     ├─ Surface
│     ├─ Volumetric
│     └─ Plane (2D Slice)
├─ ICON_FA6_ATOM  Structure 2
└─ ...
Mesh (CollapsingHeader, placeholder)
└─ (deferred to Phase 3.9)
```

## 5.4 Show / Label 컬럼 3-state 아이콘 정책

| 상태 | 아이콘 + 색상 | 클릭 시 |
|---|---|---|
| All visible | `ICON_FA6_EYE` 100% opacity | 전부 hidden 으로 토글 |
| Some visible | `ICON_FA6_EYE` 65% opacity | 전부 visible 로 토글 |
| None visible | `ICON_FA6_EYE_SLASH` 40% opacity | 전부 visible 로 토글 |

> Label 컬럼도 동일 정책. 단 group / structure 의 children 이 *모두 label 없음* 인 초기 상태에는 `EYE_SLASH` 40%.

## 5.5 Remove 컬럼 정책

| Row 유형 | Remove 아이콘 | 클릭 시 |
|---|---|---|
| Structure root | `ICON_FA6_TRASH` (또는 legacy 동일 텍스트) | `Delete Confirmation` modal → Yes 시 `structures.Remove(id)` 호출 |
| Atoms / 원소 group / 개별 atom | `--` (TextDisabled) | 무동작 (legacy 동일) |
| Measurement type folder | `ICON_FA6_TRASH` | `Clear Measurements Confirmation` modal → Yes 시 `ClearMeasurementsByStructure(id, type)` |
| Individual measurement | `ICON_FA6_TRASH` | 즉시 `RemoveMeasurement(measurementId)` (legacy 동일 — 확인 없음) |
| Charge Density 자식 | `--` | 무동작 |

## 5.6 Delete / Clear Measurements 확인 modal

| 항목 | legacy 기준 | Phase 3.8 요구 |
|---|---|---|
| Delete modal title | `Delete Confirmation` | 동일 |
| Delete modal body | `<name> will be removed.` + `Are you sure?` | 동일 |
| Delete buttons | `Yes` / `No`, width 120 px, 가운데 정렬 | 동일 |
| Yes 동작 | structure remove + selectedMeshId 정리 | 동일 (atomsTemplate 호출 → 새 트리 `structures.Remove(id)`) |
| Clear Measurements modal title | `Clear Measurements?` (또는 legacy 동일 문구) | legacy 동일 문구 |
| Clear Measurements body | 측정 전체 수 표시 후 확인 | legacy 와 동일 문구 |
| Clear Measurements 동작 | `RemoveMeasurementsByStructure(id)` | 동일 |

## 5.7 Charge Density / Slice 자식 노드 동작

| 노드 | 클릭 시 | API |
|---|---|---|
| `Isosurface` | Data 창 + Isosurface 모드 표시 | `features::data::OpenChargeDensityViewerForStructure(id, Mode::Isosurface)` |
| `Surface` | 동일 + Surface | 동일 |
| `Volumetric` | 동일 + Volumetric | 동일 |
| `Plane` | 2D Slice Viewer 창 표시 | `features::data::OpenSliceViewerForStructure(id)` |

## 5.8 Intentional UI deviation 사전 등록

| 항목 | legacy 동작 | Phase 3.8 v1 계획 | 사유 | 후속 |
|---|---|---|---|---|
| Mesh Tree section | 별도 Mesh 노드 트리 + 가시성 / 삭제 | placeholder *deferred* CollapsingHeader 또는 미표시 | Phase 3.9 `features/mesh` 미도착 | Phase 3.9 에서 `features/mesh::DrawModelTreeSection` 또는 등록형 callback 으로 확장 |
| `Windows` 메뉴 항목 등록 방식 | legacy 는 `App` 이 모든 항목을 한 곳에서 그림 | Phase 3.8 은 *옵션 C registry* 채택 (`features::viewer::RegisterWindowsMenuEntry`). `Windows / Viewer` 와 `Windows / Model Tree` 가 같은 ImGui 메뉴에 묶이도록 함 | feature 단위 응집 + Phase 4 MenuRouter 전 임시 hook | Phase 4 `app/layout_manager` 가 정식 inventory 로 흡수 |
| `Distance factor` 슬라이더 | legacy 0.1 ~ 2.0 (배수) | 현 신규 트리 `-50% ~ +50%` (퍼센트) — *본 phase 미수정* | Phase 3.7 평가서의 D5 인계 | 별도 회귀 PR (Phase 3.4 추가 회귀 또는 Phase 4 UI 정비) |
| `D1 drag rectangle 색상` | legacy `IM_COL32(90, 170, 255, ...)` 파란색 | *사후 확정*: **노란색 `IM_COL32(255, 216, 64, ...)` 유지** 권장 — atom selection 색 일관성 / 워크플로우 안정 우선 | 사용자-visible 색 차이는 *기능 영향 없음*. 본 phase 가 확정 권한 | 평가서에 *intentional UI deviation* 으로 final 기록 |

## 5.9 UI 지침 매트릭스 매핑 (UI-01 ~ UI-12)

| UI ID | Phase 3.8 적용 |
|---|---|
| UI-01 옵션명 / 표시순서 | Crystal Structure 컬럼 순서 (Name / Show / Label / Count / Remove), 자식 노드 순서 (Atoms / Measurements / Charge Density) |
| UI-02 기본값 / 범위 | structure 가시성 default true, atom label default false, tree node DefaultOpen 정책 |
| UI-03 입력 방법 | `IsItemClicked`, `CollapsingHeader`, `TreeNodeEx`, `Button`, `BeginPopupModal` 그대로 |
| UI-04 조건부 표시 규칙 | `structures.Empty() && !HasChargeDensity()` 시 섹션 미표시, Charge Density 자식은 `HasChargeDensity && boundTo` 일 때만, Measurements 자식은 `MeasurementsByStructure.empty()` 시 노드 자체 표시하나 sub-folder 가 비어 있음 |
| UI-05 활성 / 비활성 규칙 | structureVisible 가 false 면 자식 `BeginDisabled` |
| UI-06 Apply 시점 | 모든 토글 즉시 반영 (`ImGui::IsItemClicked` 안의 set 호출 + 다음 frame 자동 redraw) |
| UI-07 공통옵션 공유 연동 | `core::scene::structures.SetVisible` 의 emit 이 viewer renderer 와 동기화. `data_menu` 의 mode 가 `data_ui` 와 공유 |
| UI-08 색상 도구 | 본 phase 신규 색상 도구 없음 (Charge Density 색은 Data UI 에서) |
| UI-09 다중 데이터 선택 UI | XSF Grid selector 는 Data UI 가 보유 — 본 phase 미영향 |
| UI-10 ImGui ID 충돌 | tree node ID = `##XsfRoot<sid>`, `##AtomsRoot_<sid>`, `##Atom_<symbol><sid>`, `##Measurements_<sid>_<type>`, `##ChargeDensity_<sid>_<mode>`. column header PushID(iCol + 100) |
| UI-11 실제 파일 로드 렌더 | imported XSF / CHGCAR 가 트리에 자동 표시 |
| UI-12 빈 데이터 테스트 훅 | structure 0 + density 0 → 섹션 자체 미표시. structure 1 + density 0 → Charge Density sub-folder 만 미표시 |

---

# Part 6 - 구현 순서

## 6.1 Step 0 - 정적 조사 / 기준 캡처

| 작업 | 산출물 |
|---|---|
| legacy `model_tree.cpp` 의 `renderXsfStructureTable` / `renderDeleteConfirmPopup` / `renderClearMeasurementsConfirmPopup` 의 라벨 / 컬럼 / 동작 추출 | 본 계획서 Part 5 와 대조 |
| `core::scene::StructureRegistry` / `SceneState::structureRecords` / `measurement_store::MeasurementsForStructure` / `charge_density_controller` 의 read API 정리 | 본 계획서 Part 4.4 quick API 명세와 대조 |
| Phase 3.7 평가서 D1 ~ D5 인계 의무 추출 | Part 1.3 |
| 직전 baseline wasm size 측정 | `Get-Item public\\wasm\\VTK-Workbench.wasm` → 15,266,894 B 인계 |

## 6.2 Step 1 - quick API 보강

| 작업 | 산출물 |
|---|---|
| `features/edit/atoms/atom_manager` 에 `labelVisibleAtomIds_` 보관 + `IsAtomLabelVisible/SetAtomLabelVisibility` 메서드 추가 | atom label flag |
| `features/edit/atoms/atom_manager` 에 atom visibility 의 `IsAtomVisible/SetAtomVisibility` 도 추가 (legacy 의 atom 단위 visibility 보존) | 또는 기존 `AtomRecord::visible` 직접 사용 — 본 phase 는 *후자* 채택 + `SetAtomVisibility(ids, bool)` quick wrapper |
| `features/edit/edit_menu` 에 `IsAtomVisible`, `SetAtomVisibility`, `IsAtomLabelVisible`, `SetAtomLabelVisibility`, `SelectAtomById`, `ClearStructureSelection` 추가 | 6 종 신규 quick API |
| `features/measurement/measurement_menu` 에 `MeasurementsByStructure`, `SetMeasurementVisible`, `RemoveMeasurement`, `ClearMeasurementsByStructure` 추가 | 4 종 신규 quick API |
| `features/data/data_menu` 에 `IsChargeDensityBoundTo`, `OpenChargeDensityViewerForStructure`, `OpenSliceViewerForStructure` 추가 | 3 종 신규 quick API |
| `features/viewer/viewer_menu` 에 `RegisterWindowsMenuEntry(WindowsMenuEntry)` registry 신설 (옵션 C) | viewer_menu 보강 |
| Atom renderer label sync | `atom_renderer::SyncAtomLabelActors` 가 `atom_manager` 의 label flag 를 source 로 사용 |

## 6.3 Step 2 - `features/model_tree` skeleton 추가

| 작업 | 산출물 |
|---|---|
| `model_tree_menu.{h,cpp}` 추가 — `InitOnce`, `DrawMenu`, `HandleRequest`, `RenderWindows`, `Tick`, `Shutdown` | 5 진입점 |
| `model_tree_panel.{h,cpp}` 추가 — `ModelTreePanel::Render(bool*)` | tree 본체 |
| `model_tree_actions.{h,cpp}` 추가 — quick API 호출 wrapper | action 단일 진입점 |
| `tree_state.h` 추가 — `TreeNodeOpenStateMap` (`std::unordered_map<std::string, bool>`), `PendingDelete`, `PendingClearMeasurements` 등 | UI transient state |
| `CMakeLists.txt` 등록 | build source list 갱신 |
| `app/app.cpp` hook 추가 | InitOnce / DrawMenu / RenderWindows |

## 6.4 Step 3 - Crystal Structure section 구현

| 작업 | 세부 |
|---|---|
| `CollapsingHeader("Crystal Structure", DefaultOpen)` 그리기 | legacy 와 동일 |
| `BeginTable("XsfStructure_Table", 5, xsfTableFlags)` + 컬럼 setup + Headers row | legacy 와 동일 |
| structure 순회 → root 트리 row | structure visibility 토글 + tree open state 보존 |
| Atoms 자식 노드 → 원소 group → 개별 atom row | 3-state visibility + label visibility |
| Measurements 자식 노드 (조건부) | type 별 sub-folder + 개별 measurement row |
| Charge Density 자식 노드 (조건부) | Isosurface / Surface / Volumetric / Plane 4 행 |
| Mesh placeholder section | `CollapsingHeader("Mesh", DefaultOpen=false)` + `TextDisabled("Mesh content will be restored in Phase 3.9.")` |

## 6.5 Step 4 - 삭제 / 측정 클리어 modal

| 작업 | 세부 |
|---|---|
| `Delete Confirmation` modal | legacy 와 동일 문구 / 버튼 / 동작 |
| `Clear Measurements?` modal | legacy 와 동일 문구 |

## 6.6 Step 5 - EventBus 구독

| 작업 | 세부 |
|---|---|
| `onStructureRemoved` 구독 | `PendingDelete` 상태 클리어 |
| `onSelectionChanged` 구독 | tree row highlight 갱신 trigger |
| `onAtomsChanged` / `onStructureVisibilityChanged` | 다음 frame 자동 redraw |

## 6.7 Step 6 - D1 사후 확정

| 작업 | 세부 |
|---|---|
| Phase 3.7 D1 drag rectangle 색상 결정 | 본 phase 권장: 노란색 유지 + Part 5.8 의 *Intentional UI deviation* 표에 final 기록 |
| Phase 3.8 평가서에 D1 final 결정 명시 | 평가서 Part 5 에 결정 사유 + 대안 옵션 비교 표 |

## 6.8 Step 7 - 빌드 / size / 평가서 작성

| 작업 | 세부 |
|---|---|
| `npm run build-wasm:debug` | exit 0 |
| `npm run build-wasm:release` | exit 0 |
| release `VTK-Workbench.wasm` size 측정 | baseline 15,266,894 B (2026-05-15) 대비 delta 기록 |
| 평가서 작성 | `phase3_8_evaluation_YYYY-MM-DD.md`. Phase 3.7 entry gate 처리 섹션 포함 |

---

# Part 7 - 검증 매트릭스

## 7.1 정적 / 구조 검증 (#1 ~ #14)

| # | 검증 항목 | 기대값 | 방법 | 판정 기준 |
|---:|---|---|---|---|
| 1 | 신규 model_tree 파일 | `features/model_tree/` 7 개 | `rg --files webassembly/src/features/model_tree` | PASS / FAIL |
| 2 | legacy 수정 0 | `webassembly/src/legacy/` 기능 변경 없음 | `git diff -- webassembly/src/legacy` | PASS / FAIL |
| 3 | legacy include 0 | 신규 / 보강 코드에서 `legacy/` include 0 | `rg -n "legacy\|src/legacy\|#include.*legacy" webassembly/src/features/model_tree webassembly/src/features/edit webassembly/src/features/measurement webassembly/src/features/data` | PASS / FAIL |
| 4 | app hook | `InitOnce` / `DrawMenu` / `RenderWindows` 호출 존재 | `rg -n "features::model_tree" webassembly/src/app/app.cpp` | PASS / FAIL |
| 5 | CMake 등록 | 신규 7 파일 등록 | `rg -n "features/model_tree" CMakeLists.txt` | PASS / FAIL |
| 6 | Windows menu registry | `RegisterWindowsMenuEntry` 신설 + `model_tree` 가 호출 | `rg -n "RegisterWindowsMenuEntry" webassembly/src/features` | PASS / FAIL |
| 7 | edit quick API | `IsAtomVisible`, `SetAtomVisibility`, `IsAtomLabelVisible`, `SetAtomLabelVisibility`, `SelectAtomById`, `ClearStructureSelection` 존재 | `rg -n "IsAtomVisible\|SetAtomVisibility\|IsAtomLabelVisible\|SetAtomLabelVisibility\|SelectAtomById\|ClearStructureSelection" webassembly/src/features/edit/edit_menu.h` | PASS / FAIL |
| 8 | measurement quick API | `MeasurementsByStructure`, `SetMeasurementVisible`, `RemoveMeasurement`, `ClearMeasurementsByStructure` 존재 | `rg -n "MeasurementsByStructure\|SetMeasurementVisible\|RemoveMeasurement\|ClearMeasurementsByStructure" webassembly/src/features/measurement/measurement_menu.h` | PASS / FAIL |
| 9 | data quick API | `IsChargeDensityBoundTo`, `OpenChargeDensityViewerForStructure`, `OpenSliceViewerForStructure` 존재 | `rg -n "IsChargeDensityBoundTo\|OpenChargeDensityViewerForStructure\|OpenSliceViewerForStructure" webassembly/src/features/data/data_menu.h` | PASS / FAIL |
| 10 | Phase 3.6 / 3.7 비침범 | `features/file/`, `core/io/`, `core/vtk/`, `features/viewer/` (except `viewer_menu` registry hook) 변경 0 | `git diff --stat -- webassembly/src/features/file webassembly/src/core/io webassembly/src/core/vtk webassembly/src/features/viewer` | PASS (변경 0 또는 registry hook 만) / FAIL |
| 11 | EventBus 미확장 | `core/scene/events.h` 신규 event 또는 필드 0 | `git diff -- webassembly/src/core/scene/events.h` | PASS (변경 0) / DEVIATION (명시 등록) |
| 12 | label flag source | `atom_manager` 의 label visibility flag + `atom_renderer::SyncAtomLabelActors` 가 동일 source 참조 | 코드 trace | PASS / FAIL |
| 13 | 3-state visibility helper | `tree_state.h` 또는 `model_tree_panel.cpp` 에 *all / some / none* 판정 helper 존재 | `rg -n "AllVisible\|AnyVisible\|3State\|threeState" webassembly/src/features/model_tree` | PASS / FAIL |
| 14 | Mesh placeholder | Mesh section 이 `CollapsingHeader` + `TextDisabled` deferred 안내 | `rg -n "Phase 3.9\|deferred" webassembly/src/features/model_tree` | PASS / FAIL |

## 7.2 빌드 / 정량 검증 (#15 ~ #17)

| # | 검증 항목 | 기대값 | 명령 | 판정 |
|---:|---|---|---|---|
| 15 | debug build | exit 0 | `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"` | PASS / FAIL |
| 16 | release build | exit 0 | `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"` | PASS / FAIL |
| 17 | wasm size 기록 | baseline 15,266,894 B (Phase 3.7, 2026-05-15) 대비 정상 범위 | `Get-Item public\wasm\VTK-Workbench.wasm` | PASS / WATCH (>+3% 시 분석) |

## 7.3 런타임 / 시각 검증 (#18 ~ #44)

| # | 검증 항목 | 기대값 | 판정 |
|---:|---|---|---|
| 18 | Windows / Model Tree 메뉴 표시 | 메뉴 항목 존재 + 체크박스 동작 | PASS / FAIL |
| 19 | Model Tree 창 표시 / 숨김 | 메뉴 토글 + close 버튼 양방향 동기화 | PASS / FAIL |
| 20 | 빈 scene 상태 | Crystal Structure section 미표시. Mesh section 만 deferred 안내 | PASS / FAIL |
| 21 | XSF 1 개 import 후 | structure 1 행 + Atoms 자식 펼침 + 원소 group → 개별 atom row | PASS / FAIL |
| 22 | structure visibility 토글 | eye icon 클릭 → Viewer 의 atom / cell actor 가시화 변경 | PASS / FAIL |
| 23 | atom group 3-state | 원소 group 의 *모두 / 일부 / 모두 비가시* 아이콘 정확 표시 | PASS / FAIL |
| 24 | 개별 atom visibility 토글 | atom row 의 eye icon 클릭 → 해당 atom 만 hide / show | PASS / FAIL |
| 25 | label visibility 토글 | Label 컬럼 eye icon 클릭 → 해당 원자 label 표시 / 숨김 | PASS / FAIL |
| 26 | Count 컬럼 | structure root / Atoms / 원소 group 의 atom 개수 정확 표시 | PASS / FAIL |
| 27 | structure remove 액션 | Remove 클릭 → Delete Confirmation modal → Yes 시 structure 제거 + Viewer 갱신 | PASS / FAIL |
| 28 | Delete Confirmation 문구 | `<name> will be removed.` + `Are you sure?` + Yes / No 버튼 | PASS / FAIL |
| 29 | tree row → atom selection | atom row 클릭 → 해당 atom 만 selected, Viewer 의 노란 shell 표시 | PASS / FAIL |
| 30 | atom selection → tree row highlight | Viewer 에서 Ctrl + 클릭 → 트리의 해당 atom row highlight | PASS / FAIL |
| 31 | Measurements 자식 노드 표시 | 측정이 있으면 Distance / Angle / Dihedral / GeometricCenter / CenterOfMass type 폴더 표시 | PASS / FAIL |
| 32 | Measurement visibility 토글 | 개별 measurement row 의 eye icon → Viewer measurement actor 가시성 토글 | PASS / FAIL |
| 33 | Measurement type clear | type 폴더의 trash icon → Clear Measurements modal → Yes 시 해당 type 측정 모두 삭제 | PASS / FAIL |
| 34 | Individual measurement remove | 개별 row 의 trash icon → 즉시 삭제 (확인 없음, legacy 동일) | PASS / FAIL |
| 35 | Charge Density 자식 노드 표시 | `HasChargeDensity` && `boundTo` 일 때만 표시 | PASS / FAIL |
| 36 | Isosurface 노드 클릭 | Charge Density Viewer 창 열림 + Isosurface 모드 활성 | PASS / FAIL |
| 37 | Surface 노드 클릭 | 동일 + Surface 모드 | PASS / FAIL |
| 38 | Volumetric 노드 클릭 | 동일 + Volumetric 모드 | PASS / FAIL |
| 39 | Plane 노드 클릭 | 2D Slice Viewer 창 열림 | PASS / FAIL |
| 40 | Mesh placeholder | deferred 안내 텍스트 표시 | PASS / FAIL |
| 41 | 다중 structure 공존 | 2 개 이상 structure 동시 표시 + 독립 토글 | PASS / FAIL |
| 42 | repeat open/close | 창 close/open 반복 후 tree state 안정 (확장 / 축소 상태 보존) | PASS / FAIL |
| 43 | console error 0 | runtime JS / C++ error 0 | PASS / FAIL |
| 44 | 다른 메뉴 영향 0 | File / Edit / Build / Measurement / Data / Utilities / Viewer 동시 동작 정상 | PASS / FAIL |

## 7.4 UI 지침 매트릭스 (UI-01 ~ UI-12)

Part 5.9 표 참조. 모두 PASS 필요.

## 7.5 Phase 3.7 entry gate 처리 (#45 ~ #48)

| # | 항목 | 기대값 | 판정 |
|---:|---|---|---|
| 45 | **D1** drag rectangle 색상 final 결정 | 평가서 *Intentional UI deviation* 표에 final 기록 (노란색 유지 또는 파란색 복귀) | PASS / FAIL |
| 46 | **D2** `vtkWebAssembly*` + GLFW 의존성 비악화 | `core/vtk` 신규 의존성 추가 0 | PASS / FAIL |
| 47 | **D3** viewer → measurement 호출 경로 모니터링 | Phase 3.7 의 `viewer_panel.cpp:32` 호출 유지. 추가 위반 없음. Phase 4 인계 명시 | PASS / WATCH |
| 48 | **D5** Distance factor 슬라이더 | 본 phase 미수정 + 후속 PR 발의 의무 | PASS (deferred) |

---

# Part 8 - 완료 기준 (Definition of Done)

| DoD 항목 | 기준 |
|---|---|
| Model Tree 창 | `Windows / Model Tree` 토글 + close 버튼 동기화 + 실제 tree 렌더 |
| Crystal Structure 섹션 | structure / atoms / measurements / charge density 자식 트리 모두 표시 |
| 3-state visibility | Show / Label 컬럼의 *all / some / none* 아이콘 정확 |
| Remove 액션 | Delete Confirmation modal + structure remove |
| Measurement clear | Clear Measurements modal + type-별 / 개별 삭제 |
| Charge Density viewer | Isosurface / Surface / Volumetric / Plane 4 종 자식 클릭 시 Data 창 자동 표시 |
| Mesh placeholder | deferred 안내. Phase 3.9 에서 확장 |
| Selection 동기화 | 트리 ↔ Viewer 양방향 |
| EventBus 구독 | `onStructureAdded/Removed/VisibilityChanged`, `onAtomsChanged`, `onSelectionChanged` |
| legacy 격리 | legacy 변경 0, legacy include 0 |
| Phase 3.6 / 3.7 비침범 | File hot path + core/io + core/vtk + viewer 비변경 (registry hook 제외) |
| EventBus 미확장 | core/scene/events.h 변경 0 또는 deviation 등록 |
| build | debug / release 모두 PASS |
| runtime | console error 0 |
| size | wasm size delta 기록 |
| D1 사후 확정 | drag rectangle 색상 final 결정 + 평가서 기록 |
| 평가서 | `phase3_8_evaluation_YYYY-MM-DD.md` 작성 + Phase 3.7 entry gate 처리 섹션 |

---

# Part 9 - 리스크와 대응

| # | 리스크 | 영향 | 대응 |
|---:|---|---|---|
| R1 | label visibility flag 가 `core::scene::AtomRecord` 가 아닌 `atom_manager` 에 저장됨 → atom_renderer 의 label sync 가 source 를 헷갈릴 위험 | atom label 표시 누락 / 중복 | `atom_manager` 가 *유일한 label flag source*. `atom_renderer::SyncAtomLabelActors` 는 매번 `atom_manager` 의 flag 를 read. atom 삭제 시 flag 도 cleanup |
| R2 | Windows 메뉴 등록 옵션 C 의 `viewer_menu` 보강이 *Phase 3.7 비침범* 정적 검증과 충돌 | `viewer_menu.cpp` 의 변경이 정적 검증 #10 의 *features/viewer 변경 0* 위반 | 검증 #10 의 정의를 *registry helper 추가 외 변경 0* 으로 명문화. PR 본문에 명시 |
| R3 | tree row 클릭의 selection 경로가 `SceneState::selection` + `SelectionChangedEvent` 와 `SelectAtomById` 두 곳에 분산 | selection state divergence | `SelectAtomById` 가 내부적으로 `SelectionSet::SelectByIds` + `SelectionChangedEvent` emit 까지 일괄 수행. tree 는 *읽기 전용* 으로 selection set 만 참고 |
| R4 | 대형 structure (수천 atom) 에서 tree 렌더 비용 | FPS 저하 | (a) 원소 group 단계까지만 default open, 개별 atom row 는 user 가 펼칠 때만 렌더. (b) `ImGuiTableFlags_ScrollY` + `RowMinHeight` 로 virtualization 대체. (c) atom count > N (예: 500) 시 *원소 group 단위에서 truncate* 또는 lazy paginate. 본 phase 는 *(a) 기본 + (c) 후속* |
| R5 | Charge Density 가 1 개 structure 에만 bound 가능 (Phase 3.6 / 3.2 의 `boundStructureId_` 단일) — 그러나 트리는 multi-structure 표시 | bound structure 외의 tree 가 charge density 자식을 표시 안 함 | legacy 동일 동작. 사용자 혼란 우려 시 *not bound* 구조에는 charge density 노드 미표시 (current plan) 또는 grayed-out 표시 |
| R6 | Measurement type 폴더의 trash icon 이 *Clear-by-type-and-structure* 의미인데 modal 문구가 type 명시 안 하면 혼란 | 사용자 실수 | modal 문구에 type 명시 (`Clear all Distance measurements for <structure name>?`) |
| R7 | tree node open state 보존이 `model_tree_panel` 의 static map → re-create 시 state 손실 | UX 일관성 약화 | `tree_state.h` 의 `TreeNodeOpenStateMap` 을 namespace-static singleton 으로 보관. structure 삭제 시 해당 key cleanup |
| R8 | D1 색상 사후 확정이 *팀 합의* 필요 | PR 머지 지연 | 본 계획서 Part 5.8 에 *권장 결정* (노란색 유지) 미리 명시. PR 본문에서 변경 없음을 명시하고 평가서로 final 확정 |
| R9 | `data_menu::OpenChargeDensityViewerForStructure` 가 multi-structure charge density 를 지원하지 않으면 동작 미정 | 의도 외 결과 | 현재 controller 는 single bound. 본 phase 는 *active = 클릭한 structure* 로 rebind + `Show(Mode)` 호출. 미구현 시 deferred 표기 |
| R10 | wasm size 증가 | release artifact 비대화 | Phase 3.8 release build 후 size delta 기록. baseline 15,266,894 B 대비 +5% 이상 시 분석 |
| R11 | Mesh placeholder section 이 legacy 와 시각적으로 너무 다름 | UI 1:1 보존 위반 우려 | 명시적 *Intentional UI deviation* 으로 Part 5.8 에 등록 + Phase 3.9 에서 확장 약속 |
| R12 | Phase 3.7 의 D3 (viewer → measurement 직접 호출) 패턴을 Phase 3.8 가 *추가 확대* 할 가능성 (model_tree → measurement / data 직접 호출) | feature ↔ feature 의존 그래프 비대화 | quick API 만 사용 (직접 controller / store 접근 금지). PR 검토자가 의존 그래프 review |

---

# Part 10 - 평가서 작성 지침

Phase 3.8 완료 후 평가서는 `webassembly/docs/phases/phase3_8_evaluation_YYYY-MM-DD.md` 로 작성한다. 이전 phase 평가서 양식과 동일하게 유지한다.

| 평가 섹션 | 포함 내용 |
|---|---|
| 0. 집중 결론 | Model Tree 복구 여부 + Phase 3.7 entry gate 처리 결과 + GO / PARTIAL / BLOCKED 판단 |
| Part 1. 구현 개요 | 신규 파일 / 보강 파일 / 라인수 (`wc -l` 기준) / legacy 격리 |
| Part 2. 검증 매트릭스 | 본 계획서 Part 7 의 #1 ~ #48 결과 (정적 #1~#14 / 빌드 #15~#17 / 런타임 #18~#44 / Phase 3.7 entry gate #45~#48) |
| Part 3. 계획서 목표별 충족도 | G1 ~ G16 + 비목표 준수 |
| Part 4. 구현 품질 | 잘 된 점 + 구조적 편차 + Phase 3.7 entry gate 처리 결과 |
| Part 5. **D1 사후 확정** | drag rectangle 색상 final 결정 (노란색 / 파란색) + 사유 + 대안 비교 표 |
| Part 6. 리스크 | 미해결 항목 + 다음 phase 연결 |
| Part 7. 명령 요약 | build / test / static check / size 명령 |
| Part 8. 후속 phase 연결 | Phase 3.9 (mesh sub-tree 확장) + Phase 4 (MenuRouter / Windows 메뉴 정식 통합) |

---

# Part 11 - PR 체크리스트

## 작성자

### 정적 / 구조
- [ ] `features/model_tree/` 7 파일 추가 (`model_tree_menu.{h,cpp}`, `model_tree_panel.{h,cpp}`, `model_tree_actions.{h,cpp}`, `tree_state.h`)
- [ ] `features/edit/atoms/atom_manager` 에 label visibility flag + 6 종 quick API 추가
- [ ] `features/edit/edit_menu.{h,cpp}` 에 6 종 quick API 노출
- [ ] `features/measurement/measurement_menu.{h,cpp}` 에 4 종 quick API 노출
- [ ] `features/data/data_menu.{h,cpp}` 에 3 종 quick API 노출
- [ ] `features/viewer/viewer_menu.{h,cpp}` 에 `RegisterWindowsMenuEntry` registry 추가 (deviation 명시)
- [ ] `app/app.cpp` 에 `features::model_tree` include / hook 4 종 추가
- [ ] `CMakeLists.txt` 에 신규 7 파일 등록
- [ ] `webassembly/src/legacy/` 변경 0
- [ ] 신규 코드에서 `legacy/` include 0
- [ ] `features/file/` + `core/io/` + `core/vtk/` 변경 0 (Phase 3.6 / 3.7 비침범)
- [ ] `core/scene/events.h` 변경 0 (또는 deviation 등록)

### 빌드 / 정량
- [ ] `npm run build-wasm:debug` PASS
- [ ] `npm run build-wasm:release` PASS
- [ ] release `VTK-Workbench.wasm` size 기록 (baseline 15,266,894 B 대비 delta)

### 런타임
- [ ] Windows / Model Tree 토글
- [ ] Crystal Structure section 5 컬럼 / 자식 노드 트리
- [ ] 3-state Show / Label 토글
- [ ] structure remove + Delete Confirmation modal
- [ ] Measurements type 폴더 + 개별 row + Clear / Remove
- [ ] Charge Density 4 종 자식 노드 클릭 → Data 창
- [ ] Mesh placeholder + deferred 안내
- [ ] tree ↔ Viewer 양방향 selection 동기화
- [ ] 다중 structure 공존
- [ ] console error 0
- [ ] repeated open/close 안정

### Phase 3.7 entry gate
- [ ] **D1** drag rectangle 색상 final 결정 (노란색 유지 권장) 및 평가서 기록
- [ ] **D2** `vtkWebAssembly*` + GLFW 의존성 *비악화* 정적 확인
- [ ] **D3** viewer → measurement 호출 경로 *모니터링* (변경 0) + Phase 4 인계 재확인
- [ ] **D5** Distance factor 슬라이더 후속 PR 발의 의무 명시

### 기록
- [ ] PR 본문에 Phase 3.7 entry gate 처리 명시
- [ ] Intentional UI deviation 섹션 (Mesh placeholder, Windows registry, D1 색상)
- [ ] `phase3_8_evaluation_YYYY-MM-DD.md` 작성

## 검토자

- [ ] diff 가 `features/model_tree/` + 필요한 edit / measurement / data / viewer / app / CMake 보강으로 제한되어 있는가?
- [ ] `features/model_tree` 가 legacy 를 include 하지 않는가?
- [ ] `features/file/` + `core/io/` + `core/vtk/` 변경 0 인가?
- [ ] `features/viewer/` 변경이 *registry helper 추가* 외 0 인가?
- [ ] EventBus 가 변경 0 인가?
- [ ] tree row 클릭의 selection 경로가 `SelectAtomById` quick wrapper 만 사용하는가?
- [ ] Delete / Clear Measurements modal 문구가 legacy 와 동일한가?
- [ ] 3-state visibility 아이콘 / 색 정책이 정확한가?
- [ ] Mesh placeholder 가 deferred 안내로 표시되는가?
- [ ] D1 색상 final 결정이 평가서에 기록되는가?
- [ ] wasm size delta 가 정상 범위인가?

---

# Part 12 - 사용 명령 요약

```powershell
git status --short --branch
rg --files webassembly\src\features\model_tree
rg -n "legacy|src/legacy|#include.*legacy" webassembly\src\features\model_tree webassembly\src\features\edit webassembly\src\features\measurement webassembly\src\features\data
rg -n "features::model_tree" webassembly\src\app\app.cpp
rg -n "features/model_tree" CMakeLists.txt
rg -n "RegisterWindowsMenuEntry" webassembly\src\features
rg -n "IsAtomVisible|SetAtomVisibility|IsAtomLabelVisible|SetAtomLabelVisibility|SelectAtomById|ClearStructureSelection" webassembly\src\features\edit\edit_menu.h
rg -n "MeasurementsByStructure|SetMeasurementVisible|RemoveMeasurement|ClearMeasurementsByStructure" webassembly\src\features\measurement\measurement_menu.h
rg -n "IsChargeDensityBoundTo|OpenChargeDensityViewerForStructure|OpenSliceViewerForStructure" webassembly\src\features\data\data_menu.h
git diff --stat -- webassembly/src/features/file webassembly/src/core/io webassembly/src/core/vtk webassembly/src/features/viewer
git diff -- webassembly/src/core/scene/events.h
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"
Get-Item public\wasm\VTK-Workbench.wasm
# baseline (2026-05-15, Phase 3.7): 15,266,894 B
```

---

# Part 13 - 후속 Phase 연결

| 후속 phase | Phase 3.8 이 제공할 기반 | 후속 연결점 |
|---|---|---|
| Phase 3.9 Mesh | Mesh placeholder section + `RegisterModelTreeSection` 또는 callback hook 약속 | `features/mesh::DrawModelTreeSection` 가 placeholder 위치를 채우면 Mesh actor / Display Mode / 가시성 / 삭제 트리 완성 |
| Phase 4 MenuRouter | `features::viewer::RegisterWindowsMenuEntry` registry + `features::model_tree::DrawMenu` 임시 hook | `app/layout_manager` 가 Windows 메뉴 정식 inventory 로 흡수. `app/menu_router` 가 InvokeAction / OpenWindow / EnterMode / TogglePref 분리 |
| Phase 5 legacy 격리 | `features/model_tree` 가 legacy `model_tree.{cpp,h}` 참조 0 | legacy build 제외 시 충돌 없음 |
| Phase 6 마무리 | model_tree 의 quick API 정리 + EventBus fanout 검증 | tree ↔ Edit / Measurement / Data feature 의 event 일관성 보존 |

## 13.1 Phase 3.9 으로의 명시 인계 표

| 인계 항목 | 내용 | Phase 3.9 계획서 기재 위치 |
|---|---|---|
| 평가 baseline 시점 | 2026-05-15 / Phase 3.8 평가서 일자 | Part 1 / 변경 이력 |
| wasm size baseline | Phase 3.8 평가서의 final 값 | size delta 비교 baseline |
| Mesh placeholder 위치 | `features/model_tree/model_tree_panel` 의 `Mesh` CollapsingHeader 안 deferred 영역 | Mesh sub-tree 확장 진입점 |
| Mesh registry callback | Phase 3.8 에서 `RegisterModelTreeSection` 형식의 callback 등록 mechanism 권장 — Phase 3.8 가 *implementation 없는 hook only* 도 가능 | Phase 3.9 가 callback 구현 |

---

# Part 14 - 관련 문서

- 본 phase 의 선행 평가서: [`./phase3_7_evaluation_2026-05-15.md`](./phase3_7_evaluation_2026-05-15.md)
- 본 phase 의 선행 계획서: [`./phase3_7_viewer_v2.md`](./phase3_7_viewer_v2.md) v2.1.1
- 관련 회귀 PR 계획서: [`./phase3_4_renderer_regression.md`](./phase3_4_renderer_regression.md)
- 모 phase 3.4 / 3.5 / 3.6 계획서 및 평가서
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6.3.8
- 대상 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §2 features/model_tree + §3
- 메뉴 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §10 + §12
- 메뉴 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §5
- UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
