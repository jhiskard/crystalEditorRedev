# Phase 3.4.2 — Edit / Atoms 이식 (Phase 3.4 의 2/3 — 가장 무거운 단계) 세부계획서

> 분할 컨텍스트: **Phase 3.4 의 2/3** ([`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) Option A 채택 — Cell → **Atoms** → Bonds 의존성 순서)
> 모체 계획서:    [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)
> 선행 sub-phase: [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md)
> 상위 문서:      [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + §6.0 공통 지침
> 메뉴 매핑:      [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
> UI 이식 지침:   [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01~UI-12)
> 작성일:        2026-05-07
> 대상 브랜치:    `refactor/menu-aligned`
> 단위 PR:       1 개
> 예상 소요:      3~4 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-07 | 초안 작성 (Phase 3.4 모체 계획서를 분할안 Option A 에 따라 세분화) |

---

## 0. 한 줄 요약

> Phase 3.4 의 *세 sub-phase 중 두 번째이자 가장 무거운* 단계로, Edit 메뉴의 *Atoms 항목* 을 이식한다. `features/edit/atoms/` 신설 + `edit_menu` Atoms 항목 추가 + InitOnce 시그니처에 `MouseInteractor&` 추가. legacy `atom_manager.{cpp,h}` (704 줄) + `surrounding_atom_manager.{cpp,h}` (409 줄) + `atom_editor_ui.{cpp,h}` (942 줄) + `vtk_renderer.cpp` 의 *atom 부분* (~700 줄) 을 이식. **`core::scene::EventBus::onAtomsChanged` + `onStructureAdded` 첫 구독자** + **`core::vtk::MouseInteractor` 첫 구독자** 도착으로 Phase 2 인프라의 *세 종 동시 검증*. **Phase 3.3 의 Add atom + Bravais Apply 의 atom 추가가 비로소 화면에 시각적으로 보이는 *결정적 보상* 시점**.
> 본 sub-phase 라인수 약 **1,900 줄, 10 파일** — Phase 3.3 (1,883) 와 거의 동일 규모. Phase 3.4 의 가장 무거운 단계지만 분할로 *Phase 평균 (2,200) 이내* 로 정렬.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `features/edit/atoms/` sub-folder 신설 + 10 파일 작성 (atom_manager + surrounding_atom_manager + atom_renderer + atoms_controller + atom_editor_ui). (b) `edit_menu` 갱신 — Atoms 항목 추가 + `InitOnce(scene, MouseInteractor&)` 로 시그니처 확장. (c) 메뉴 `Edit / Atoms` 클릭 시 Created Atoms 윈도우 표시. (d) **legacy `vtk_renderer.cpp` 의 *atom 부분 ~700 줄* 분할 이식** — `atom_renderer.{cpp,h}` 신설 (vtk_renderer 분할 2/3). (e) **`core::scene::EventBus::onAtomsChanged` + `onStructureAdded` 의 첫 구독자** 도착 — Phase 3.3 의 PeriodicTableController + BravaisController emit 자와 결합. (f) **`core::vtk::MouseInteractor` 의 첫 구독자** 도착 — atom 픽킹 / hover / 드래그 셀렉션 이벤트. (g) *Phase 3.3 의 onStructureAdded emit 보강* (§Phase 3.3 평가서 §1.6.4) — 본 단계 PR 안에서 함께 처리. (h) `npm run build-wasm:debug` + `:release` 통과 + 런타임에서 Edit / Atoms 메뉴 동작 + **Phase 3.3 의 atom 추가가 시각적으로 보임 + 시나리오 S1/S2/S5/S6 통과**. (i) **legacy 의 Created Atoms UI 동작이 1:1 보존됨** (§1.4.1 — Created Atoms 테이블의 컬럼 순서 / 셀 편집 응답 / 픽킹 색상 등) |
| **비목표** | Edit / Bonds 메뉴 항목 (Phase 3.4.3 진행), bond_renderer 분할 (3.4.3 의 영역), element_database 두 번째 사용자 — covalentRadius (3.4.3 의 영역), `menu_router` 정식 도입 (Phase 4), Cell Apply 시 atom 좌표 재배치의 *완전한 회귀* — 본 단계는 *cell_controller 의 OnCellChanged 콜백 구독* 까지만. *atom_renderer 의 모든 legacy 기능* (예: 라벨 편집 모드의 매우 세부 옵션) 회귀 — Phase 6 마무리, **legacy UI 의 *재설계* — Created Atoms 의 컬럼 순서 / 그룹 드롭다운 / Apply 응답 패턴 등은 변경 금지** |

> Phase 3.4.2 의 미덕: *"검토자가 git diff 를 보고 `features/edit/atoms/` 의 신규 10 파일 + `features/edit/edit_menu.{cpp,h}` 의 Atoms 항목 추가 + `features/build/bravais/bravais_controller.cpp` 의 onStructureAdded emit 1 줄 + `app/app.cpp` 의 InitOnce 시그니처 확장 + `CMakeLists.txt` 의 source 추가 외에 의심할 게 없고, side-by-side 스크린샷에서 legacy 와 새 트리의 Created Atoms 윈도우가 시각적으로 동일하며, **Phase 3.3 의 Add atom 이 본 단계에서 비로소 화면에 sphere 로 그려지고, atom 클릭 → 테이블 행 강조의 picking 흐름이 정상 작동한다**"*.

### 1.1 Phase 3.4.2 의 의의 — *3 인프라 동시 검증 + 결정적 시각 보상*

본 sub-phase 는 Phase 3.4 의 3 분할 중 **가장 무거운 (~1,900 줄)** 단계로서, Phase 2 인프라의 *세 종을 단일 sub-phase 안에서 동시 검증* 한다.

| Phase 2 인프라 | 본 sub-phase 의 첫 사용 |
|---|---|
| `core/scene/EventBus::onAtomsChanged` (Subscribe) | atom_renderer 가 첫 구독자 — Phase 3.3 의 PeriodicTableController.AddAtom + BravaisController.Apply 의 emit 자와 결합 |
| `core/scene/EventBus::onStructureAdded` (Subscribe) | atoms_controller 가 첫 구독자 — Phase 3.3 평가서 §1.6.4 의 미발신 보강과 *동시 도착* |
| `core/vtk/MouseInteractor` 픽킹 이벤트 | atoms_controller 가 첫 구독자 (`OnHoverAtom`, `OnSelectAtom`, `OnDragSelection`, `OnEmptyClick`) — Phase 3.5 measurement 가 두 번째 구독자 예정 |
| `core/scene/Selection / Hover` | atom_renderer 가 첫 표시 책임자 — 선택 강조 / hover 색상 변경 |
| `core/scene/SceneState::structureRecords::atoms` | atom_manager 가 *최초의 변경자* — Add/Remove/Move/SetVisible/Select |

→ 본 sub-phase 는 **Phase 3.3 의 emit 자 (3 곳) 가 비로소 *듣는 자를 만나는* 결정적 시점**. 시나리오 S1 *"Periodic Table → C → Add atom → Created Atoms 테이블에 1 행 + 화면에 sphere 표시"* 가 본 단계에서 비로소 통과.

### 1.2 회색지대 정책 인계

본 계획서에서도 네 회색지대 정책을 그대로 적용 (Phase 3.4.1 §1.2 와 동일).

### 1.2.1 Phase 3.3 onStructureAdded 보강 — 본 sub-phase PR 에 포함

Phase 3.3 평가서 §1.6.4 의 권장 사항 — `BravaisController.Apply` 가 새 구조 생성 시 `onStructureAdded.Emit` 미발신 — 을 본 sub-phase PR 에서 함께 처리. *구독자가 도착하는 본 단계가 자연스러운 보강 시점*.

```cpp
// features/build/bravais/bravais_controller.cpp 보강
// Apply 안에서 새 구조 생성 직후
if (newStructureCreated) {
    scene_.events.onStructureAdded.Emit(core::scene::StructureAddedEvent{structureId});
}
```

→ 본 보강은 *추가만* 하고 기존 `scene_.structures.Register` 호출은 보존. PR 안에서 Phase 3.3 시나리오 S3/S4 도 *재검증* 하여 회귀 부재 확인.

### 1.3 라인수 압축 정책

Phase 3.1/3.2/3.3 의 압축 패턴을 본 단계에서도 적용. 단 atom_editor_ui (942 줄) + vtk_renderer atom 분할 (~700 줄) + surrounding_atom_manager (409 줄) 의 *알고리즘 + UI 데이터* 비중이 크므로 **약 70% 수준** 으로 보수적.

**보존 필수 (압축 시에도 변경 금지)**:

- `atom_manager.cpp` 의 atom 데이터 변경 메서드 (Add/Remove/Move/SetVisible/Select) 의 *호출 sequence + EventBus emit 시점*
- `surrounding_atom_manager.cpp` 의 PBC image atom 생성 알고리즘 (cell matrix + atom 좌표 → image atom 좌표 8/26 종)
- vtk_renderer 의 *atom 부분* (`initializeAtomGroupVTK`, `updateAtomGroupVTK`, `clearAtomGroupVTK`, `setAtomGroupVisible`, `syncAtomLabelActors`) 의 *vtkSphereSource → vtkGlyph3D → mapper → actor* 시퀀스
- `atom_editor_ui.cpp` 의 Created Atoms 테이블 *컬럼 순서 + 정렬 키 + Apply 응답 패턴* (§1.4.1)
- §1.4 의 모든 UI 보존 항목

### 1.4 legacy UI 작동방식 1:1 보존

본 sub-phase 의 보존 대상은 **Created Atoms 윈도우 1 종**.

#### 1.4.1 Created Atoms 윈도우 보존 대상

`legacy/atoms/ui/atom_editor_ui.cpp` (896 줄) 가 정의하는 다음 UI 동작은 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| 테이블 컬럼 | Index / Symbol / X / Y / Z / Selected / Visible / Group 등 — legacy 의 정확한 *컬럼 순서와 너비* |
| 정렬 / 필터 | 컬럼 헤더 클릭으로 정렬 (legacy 가 가졌다면) / 검색 박스 |
| 선택 / Visibility | 행 클릭 시 atom 선택 + scene 의 picking actor 강조 / 체크박스로 visibility 토글 |
| Edit 동작 | 좌표 셀의 *직접 편집 가능 여부* — legacy 의 InputFloat 응답 패턴 (Enter 시 적용? 즉시 적용?) |
| Group 관리 | atomGroup 컬럼의 드롭다운 — 그룹 추가/제거 동작 |
| Add/Remove 버튼 | 개별 atom 추가/제거 (Periodic Table 윈도우 의 "Add atom" 과 다른 *table 안* 의 빠른 추가) |
| Selected/Hover 색상 | atom_renderer 의 강조 색상 — legacy 의 RGBA 그대로 |

#### 1.4.2 보존 검증 절차

PR 작성자는 본 PR 본문에 다음 검증을 첨부:

1. **Side-by-side 스크린샷** — legacy Created Atoms vs 새 트리의 같은 윈도우.
2. **사용자 시나리오 정합성** — 다음 4 시나리오를 legacy 와 새 트리에서 차례로 실행해 동일 결과 확인:
   - **(S1)** Periodic Table → "C" (Carbon) → "Add atom" → **Created Atoms 테이블에 C 1 행 + 화면에 sphere 표시** *(Phase 3.3 의 atom 추가가 본 sub-phase 에서 비로소 시각화)*
   - **(S2)** Created Atoms 테이블 → C 행의 X 셀 클릭 → 좌표 변경 → Enter → atom 위치 갱신 (legacy 와 동일 응답 시점)
   - **(S5)** atom 클릭 (mouse_interactor) → Created Atoms 테이블 행 자동 선택 + atom 강조 색상
   - **(S6)** atom 드래그 셀렉션 → 다중 atom 선택 + Created Atoms 테이블 행 다중 강조
3. **시나리오 S3-atoms** — Phase 3.4.1 의 S3-cell 을 atom 측까지 확장: Bravais Lattice Templates → "FCC", a=3.5 → Apply → **Cell + atom 4 개 모두 갱신**. *atom 측 시각화가 본 단계에서 비로소 가능*.
4. **Phase 3.3 시나리오 S3/S4 재검증** — onStructureAdded 보강 후 Phase 3.3 의 시나리오가 회귀 없이 동작.
5. **Intentional UI deviation 사유서** — 의도적인 UI 변경 발생 시 PR 본문 명기 (없으면 *"None"*).

---

## 2. 전제 — Phase 3.4.1 완료 상태

본 계획서는 다음이 충족된 상태에서 시작.

- [ ] Phase 3.4.1 commit 머지
- [ ] `webassembly/src/features/edit/cell/` 8 파일 + `edit_menu.{cpp,h}` skeleton 정상 동작
- [ ] `core::scene::EventBus::onCellChanged` 첫 구독자 동작 확인
- [ ] `npm run build-wasm:debug` + `:release` exit 0
- [ ] Edit / Cell 메뉴 항목 동작 + 시나리오 S3-cell 통과
- [ ] **Phase 3.3 의 onStructureAdded 미발신 항목** — Phase 3.3 평가서 §1.6.4 — *본 sub-phase 안에서 보강* (§1.2.1)

---

## 3. legacy 참조 인벤토리

| legacy 파일 | 라인 수 | 역할 | 새 트리 행선지 |
|---|---|---|---|
| `legacy/atoms/domain/atom_manager.{cpp,h}` | 454 + 250 | atom 데이터 변경 (`Add/Remove/Move/SetVisible/Select`) | `features/edit/atoms/atom_manager.{cpp,h}` |
| `legacy/atoms/domain/surrounding_atom_manager.{cpp,h}` | 389 + 20 | PBC image atom 생성 — *주기 경계 표시* | `features/edit/atoms/surrounding_atom_manager.{cpp,h}` |
| `legacy/atoms/ui/atom_editor_ui.{cpp,h}` | 896 + 46 | Created Atoms 테이블 ImGui | `features/edit/atoms/atom_editor_ui.{cpp,h}` |
| `legacy/atoms/infrastructure/vtk_renderer.{cpp,h}` (atom 부분) | (1,792 의 ~700) | atom group + label sync | `features/edit/atoms/atom_renderer.{cpp,h}` |
| **(신규)** | — | `atoms_controller` (mouse_interactor + onStructureAdded 구독자) | 신규 작성 |

**총 legacy 참조: ~2,755 줄.** 압축 후 약 **~1,900 줄** 예상 (Phase 3.3 의 61.2% 수준).

### 3.1 외부 의존 사전 분석

| legacy 파일 | 외부 의존 | 새 트리 처리 |
|---|---|---|
| `atom_manager.{cpp,h}` | `class AtomsTemplate` (parent), SceneState (간접) | controller 로 분기 + SceneState 직접 접근. **EventBus::onAtomsChanged emit 추가** |
| `surrounding_atom_manager.{cpp,h}` | `cell_manager.h` (Phase 3.4.1 도입), atom_manager | namespace 정리 + cell_manager 의존 그대로 (Phase 3.4.1 의 인터페이스 사용) |
| `atom_editor_ui.{cpp,h}` | `class AtomsTemplate` (forward), atom_manager | controller 로 분기 |
| `vtk_renderer.{cpp,h}` (atom 부분) | vtk* 다수, atom_manager | `core::vtk::VtkViewer::Instance().GetRenderer()->AddActor(...)` 직접 |

### 3.2 메뉴 트리 매핑

| 메뉴 항목 | 새 진입점 | 윈도우 |
|---|---|---|
| `Edit / Atoms` | `features::edit::atoms::Show()` | Created Atoms |

→ Phase 3.4.1 의 *Cell 항목만* 에서 본 단계의 *Cell + Atoms* 로 확장.

### 3.3 mouse_interactor 의존 분석 (첫 구독자)

`features/edit/atoms` 가 `core/vtk/mouse_interactor.h` 의 다음 이벤트를 구독:

| 이벤트 | 구독 위치 | 처리 |
|---|---|---|
| `OnHoverAtom(int atomId)` | atoms_controller | hover 색상 표시 + tooltip |
| `OnSelectAtom(int atomId)` | atoms_controller | selection 갱신 + Created Atoms 테이블 행 강조 |
| `OnDragSelection(rect)` | atoms_controller | 사각 영역 안의 atoms 다중 선택 |
| `OnEmptyClick()` | atoms_controller | selection 해제 |

→ Phase 3.5 (measurement) 가 같은 이벤트의 *두 번째 구독자*. 본 단계에서 *publisher 1 명, subscriber 1 명* 의 깨끗한 1:1 관계 확립.

### 3.4 vtk_renderer atom 부분 분할 정책

Phase 3.4 §1.2.1 의 분할 정책 적용 (Phase 3.4.1 의 cell 분할 패턴 그대로 확장).

| 영역 | 분할 대상 vtk* 호출 | 새 모듈 |
|---|---|---|
| Atom group 액터 | `vtkSphereSource`, `vtkGlyph3D`, `vtkPolyDataMapper`, `vtkActor` | `atom_renderer.cpp` |
| 기능 | `initializeAtomGroupVTK`, `updateAtomGroupVTK`, `clearAtomGroupVTK`, `clearAllAtomGroupsVTK`, `isAtomGroupInitialized`, `setAtomGroupVisible`, `setAllAtomGroupsVisible`, `syncAtomLabelActors`, `clearAllAtomLabelActors` | 9 메서드 그대로 보존 |
| Label sync | `vtkBillboardTextActor3D` 또는 텍스트 액터 | atom_renderer 안에 흡수 |

> 분할 시 보존 필수: 각 vtk* 호출의 *호출 순서 + 인자 + Modified 호출 시점* + *AtomGroupVTKData 캐시 정책* 은 legacy 와 1:1.

---

## 4. 작업 절차 (단계별)

### Step 1 — `features/edit/atoms/` 폴더 신설

```bash
cd webassembly/src
mkdir -p features/edit/atoms
```

### Step 2 — Phase 3.3 onStructureAdded emit 보강

§1.2.1 의 보강. Phase 3.3 의 BravaisController.Apply 안에서 emit 추가.

```cpp
// features/build/bravais/bravais_controller.cpp 보강
void BravaisController::Apply(...) {
    int32_t structureId = EnsureCurrentStructure();
    bool newStructureCreated = ...;   // 새 구조 생성 여부 플래그

    // 기존 코드 유지 (scene_.structures.Register 등)
    ...

    if (newStructureCreated) {
        scene_.events.onStructureAdded.Emit(core::scene::StructureAddedEvent{structureId});  // ← 신규
    }
    scene_.events.onCellChanged.Emit(core::scene::CellChangedEvent{structureId});
    scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});
}
```

### Step 3 — `atom_manager.{cpp,h}` 도메인 이식

legacy 704 줄을 namespace 정리 + EventBus emit 추가하며 이식.

```cpp
/**
 * @file features/edit/atoms/atom_manager.h
 * @brief Atom 데이터 변경자 — Add/Remove/Move/SetVisible/Select.
 */
#pragma once
#include "core/scene/scene_state.h"

#include <array>
#include <string>

namespace features::edit::atoms {

class AtomManager {
public:
    explicit AtomManager(core::scene::SceneState& scene);

    int  Add(int32_t structureId, const std::string& symbol,
             const std::array<float, 3>& position);
    void Remove(int32_t structureId, int atomId);
    void Move(int32_t structureId, int atomId, const std::array<float, 3>& position);
    void SetVisible(int32_t structureId, int atomId, bool visible);

    /// @brief Cell 변경 시 atom 좌표 갱신 콜백 — Phase 3.4.1 의 cell_controller 가 호출.
    void OnCellChanged(int32_t structureId, bool preserveFractional);

private:
    core::scene::SceneState& scene_;
};

} // namespace features::edit::atoms
```

> **EventBus emit**: 모든 변경 메서드 끝에서 `scene_.events.onAtomsChanged.Emit(...)`. *batch* 변경 시 1 회만 emit (Phase 3.4 §6.7 리스크 완화).

### Step 4 — `surrounding_atom_manager.{cpp,h}` 도메인 이식

legacy 409 줄을 namespace 정리. *cell_manager 의존 그대로* (Phase 3.4.1 에서 도입된 `features::edit::cell::CellManager` 인터페이스 사용).

```cpp
/**
 * @file features/edit/atoms/surrounding_atom_manager.h
 * @brief PBC image atom 생성 — 주기 경계 표시.
 */
#pragma once
#include "core/scene/scene_state.h"
#include "../cell/cell_manager.h"

namespace features::edit::atoms {

class SurroundingAtomManager {
public:
    SurroundingAtomManager(core::scene::SceneState& scene,
                           cell::CellManager& cellManager);

    void Recompute(int32_t structureId);
    void SetEnabled(bool enabled);

private:
    core::scene::SceneState& scene_;
    cell::CellManager&       cellManager_;
};

} // namespace features::edit::atoms
```

### Step 5 — `atom_renderer.{cpp,h}` 신설 (vtk_renderer atom 분할)

legacy `vtk_renderer.cpp` 의 atom 관련 ~700 줄을 *복사 + namespace 정리* 형태로 이식.

```cpp
/**
 * @file features/edit/atoms/atom_renderer.h
 * @brief Atom 액터 그룹 — Phase 3.4.2 의 vtk_renderer 분할 (2/3).
 */
#pragma once
#include "core/scene/scene_state.h"
#include "core/vtk/vtk_viewer.h"

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <unordered_map>

namespace features::edit::atoms {

class AtomRenderer {
public:
    explicit AtomRenderer(core::scene::SceneState& scene);
    ~AtomRenderer();

    /// @brief EventBus::onAtomsChanged 구독 — Phase 3.3 의 emit 자와 결합.
    void Subscribe();

    void InitializeAtomGroup(const std::string& symbol, float radius);
    void UpdateAtomGroup(int32_t structureId, const std::string& symbol);
    void ClearAtomGroup(const std::string& symbol);
    void ClearAllAtomGroups();
    void SetAtomGroupVisible(const std::string& symbol, bool visible);
    void SetAllAtomGroupsVisible(bool visible);

    void SyncAtomLabelActors(const std::vector<LabelActorSpec>& labels);
    void ClearAllAtomLabelActors();

private:
    core::scene::SceneState& scene_;
    // legacy 의 AtomGroupVTKData 캐시 보존
};

} // namespace features::edit::atoms
```

> **§1.1 첫 구독자 도착**: `Subscribe()` 안에서 `scene_.events.onAtomsChanged.Subscribe(...)` — Phase 3.3 의 emit 자와 결합. **본 단계의 결정적 인프라 검증**.
>
> **§1.2.1 분할 검증**: legacy vtk_renderer 의 `initializeAtomGroupVTK` / `updateAtomGroupVTK` 호출 시 *vtkSphereSource → vtkGlyph3D → mapper → actor* 시퀀스가 새 트리에서도 *완전 동일* — §5 #11 grep diff 로 검증.

### Step 6 — `atoms_controller.{cpp,h}` (mouse_interactor 첫 구독자)

```cpp
/**
 * @file features/edit/atoms/atoms_controller.h
 * @brief Atoms 의 통합 진입점 — UI ↔ 도메인 + mouse_interactor 첫 구독자.
 */
#pragma once
#include "atom_manager.h"
#include "surrounding_atom_manager.h"
#include "atom_renderer.h"
#include "core/vtk/mouse_interactor.h"

namespace features::edit::atoms {

class AtomsController {
public:
    AtomsController(core::scene::SceneState& scene,
                    cell::CellManager& cellManager);

    /// @brief mouse_interactor 픽킹 이벤트 첫 구독자.
    void Subscribe(core::vtk::MouseInteractor& mouseInteractor);

    /// @brief onStructureAdded 첫 구독자.
    void SubscribeEvents();

    AtomManager&             AtomMgr()        { return atomManager_; }
    SurroundingAtomManager&  SurroundingMgr() { return surroundingManager_; }
    AtomRenderer&            Rend()           { return atomRenderer_; }

private:
    core::scene::SceneState&  scene_;
    AtomManager               atomManager_;
    SurroundingAtomManager    surroundingManager_;
    AtomRenderer              atomRenderer_;

    void HandleHoverAtom(int atomId);
    void HandleSelectAtom(int atomId);
    void HandleDragSelection(const Rect& rect);
    void HandleEmptyClick();
};

} // namespace features::edit::atoms
```

> Phase 3.3 평가서 §1.6.2 의 *얇은 controller* 패턴 그대로 적용 — 외부 모듈 (atom_manager / surrounding / atom_renderer / mouse_interactor) 가 책임 흡수.

### Step 7 — `atom_editor_ui.{cpp,h}` UI 이식 (§1.4.1 보존 핵심)

legacy `atom_editor_ui.cpp` (896 줄) 의 ImGui 흐름을 *위젯 단위 1:1 보존* 하며 이식. 본 단계의 **가장 큰 UI 파일** + **가장 큰 §1.4 보존 부담**.

```cpp
/**
 * @file features/edit/atoms/atom_editor_ui.h
 * @brief Created Atoms 윈도우 ImGui — §1.4.1 보존 핵심.
 */
#pragma once
#include <imgui.h>

namespace features::edit::atoms {

class AtomsController;

class AtomEditorUI {
public:
    explicit AtomEditorUI(AtomsController& controller);
    void Render(bool* open);

private:
    AtomsController& controller_;
    char             searchBuffer_[64] = {0};
    int              sortColumn_       = 0;
    // ★ §1.4.1 보존: 컬럼 순서 (Index/Symbol/X/Y/Z/Selected/Visible/Group) +
    //                정렬 + 셀 편집 응답 + Add/Remove 버튼 + 그룹 드롭다운
};

} // namespace features::edit::atoms
```

### Step 8 — `edit_menu` 갱신 — Atoms 항목 추가

Phase 3.4.1 의 skeleton 에 Atoms 항목 wiring 추가 + InitOnce 시그니처 확장.

```cpp
// features/edit/edit_menu.h 갱신
namespace features::edit {

void DrawMenu();
void RenderWindows();
void Tick(float dt);
void Shutdown();
void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mi);  // ← 시그니처 확장

}
```

```cpp
// features/edit/edit_menu.cpp 갱신
namespace features::edit {

namespace {
    bool g_showCellInfo = false;
    bool g_showCreatedAtoms = false;   // ← 신규
    cell::CellController*    g_cellCtrl  = nullptr;
    cell::CellInfoUI*        g_cellUI    = nullptr;
    atoms::AtomsController*  g_atomsCtrl = nullptr;     // ← 신규
    atoms::AtomEditorUI*     g_atomsUI   = nullptr;     // ← 신규
}

void DrawMenu() {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Atoms")) g_showCreatedAtoms = true;   // ← 신규
        // 3.4.3 에서 Bonds 항목 추가 예정.
        if (ImGui::MenuItem("Cell"))  g_showCellInfo = true;
        ImGui::EndMenu();
    }
}

void RenderWindows() {
    if (g_showCreatedAtoms && g_atomsUI) g_atomsUI->Render(&g_showCreatedAtoms);   // ← 신규
    if (g_showCellInfo     && g_cellUI)  g_cellUI->Render(&g_showCellInfo);
}

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mi) {
    static cell::CellController   cellCtrl(scene);
    static cell::CellInfoUI       cellUI(cellCtrl);
    cellCtrl.Rend().Subscribe();

    static atoms::AtomsController atomsCtrl(scene, cellCtrl.Mgr());   // ← 신규
    static atoms::AtomEditorUI    atomsUI(atomsCtrl);                  // ← 신규
    atomsCtrl.Rend().Subscribe();          // onAtomsChanged
    atomsCtrl.SubscribeEvents();           // onStructureAdded
    atomsCtrl.Subscribe(mi);               // mouse_interactor

    g_cellCtrl  = &cellCtrl;  g_cellUI  = &cellUI;
    g_atomsCtrl = &atomsCtrl; g_atomsUI = &atomsUI;
}

}
```

### Step 9 — `app/app.cpp` InitOnce 시그니처 확장

```cpp
int App::Init() {
    static core::scene::SceneState scene;
    static core::vtk::MouseInteractor mouseInteractor(scene);   // ← 신규 (또는 기존 인스턴스 재사용)
    features::utilities::bz::InitOnce(scene);
    features::data::InitOnce(scene);
    features::build::InitOnce(scene);
    features::edit::InitOnce(scene, mouseInteractor);            // ← 시그니처 확장
}
```

### Step 10 — `CMakeLists.txt` SOURCES_FEATURES 확장

```cmake
set(SOURCES_FEATURES
    # Phase 3.1~3.3 + 3.4.1
    ...
    # Phase 3.4.2 신규
    webassembly/src/features/edit/atoms/atom_manager.cpp
    webassembly/src/features/edit/atoms/atom_manager.h
    webassembly/src/features/edit/atoms/surrounding_atom_manager.cpp
    webassembly/src/features/edit/atoms/surrounding_atom_manager.h
    webassembly/src/features/edit/atoms/atom_renderer.cpp
    webassembly/src/features/edit/atoms/atom_renderer.h
    webassembly/src/features/edit/atoms/atoms_controller.cpp
    webassembly/src/features/edit/atoms/atoms_controller.h
    webassembly/src/features/edit/atoms/atom_editor_ui.cpp
    webassembly/src/features/edit/atoms/atom_editor_ui.h
)
```

총 10 신규 파일.

### Step 11 — 정적 검증

```bash
cd webassembly/src

# legacy 호출 0
grep -rnE "AtomsTemplate::|atoms::domain::|atoms::infrastructure::|atoms::ui::" \
  features/edit/atoms/ | grep -v "//" || echo "OK"

# legacy include 0
grep -rnE '#include\s+"(\.\./)*legacy/' features/edit/atoms/ | grep -v "//" || echo "OK"

# **EventBus 구독자 — onAtomsChanged + onStructureAdded**
grep -rnE "onAtomsChanged.Subscribe|onStructureAdded.Subscribe" features/edit/atoms/ | head -5

# **mouse_interactor 첫 구독자**
grep -rnE "core::vtk::MouseInteractor|MouseInteractor::|OnHoverAtom|OnSelectAtom|OnDragSelection|OnEmptyClick" \
  features/edit/atoms/ | head -10

# legacy vtk_renderer 의존 0
grep -rnE "atoms::infrastructure::VTKRenderer" features/edit/atoms/ | grep -v "//" || echo "OK"

# **vtk_renderer atom 분할 — vtkSphereSource / vtkGlyph3D 호출 보존**
grep -nE "vtkSphereSource|vtkGlyph3D|InitializeAtomGroup|UpdateAtomGroup" \
  features/edit/atoms/atom_renderer.cpp | head -10

# **Phase 3.3 onStructureAdded emit 보강 확인**
grep -nE "onStructureAdded.Emit" features/build/bravais/bravais_controller.cpp

# **§1.4.1 UI 보존 — atom_editor_ui ImGui 위젯 grep diff**
grep -nE 'ImGui::(BeginTable|TableNextRow|TableSetupColumn|InputFloat|Checkbox|Combo|Button)' \
  legacy/atoms/ui/atom_editor_ui.cpp \
  features/edit/atoms/atom_editor_ui.cpp 2>/dev/null
```

### Step 12 — 빌드 + 런타임 검증 (Windows 측)

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release
npm run dev
```

**§1.4 보존 검증**:

- [ ] Side-by-side 스크린샷 (legacy Created Atoms vs 새 트리)
- [ ] **시나리오 S1 — Phase 3.3 의 Add atom 이 본 단계에서 시각화** *(결정적 보상 시점)*
- [ ] 시나리오 S2 — Created Atoms 셀 편집
- [ ] **시나리오 S3-atoms — Bravais Apply 후 cell + atom 4 개 모두 갱신**
- [ ] 시나리오 S5/S6 — mouse_interactor 픽킹 + 드래그 셀렉션
- [ ] **Phase 3.3 시나리오 S3/S4 재검증** (onStructureAdded 보강 후 회귀 부재)
- [ ] Intentional UI deviation 사유서

### Step 13 — 커밋 & PR

```powershell
git commit -m "Phase 3.4.2 (2/3): features/edit/atoms — second sub-phase, heaviest

This PR is 2/3 of Phase 3.4. It introduces the atoms sub-folder, splits the
vtk_renderer atom portion (~700 lines), brings the first subscribers of
onAtomsChanged + onStructureAdded + MouseInteractor.

Phase 3.3 §1.6.4 backfill: BravaisController.Apply now emits onStructureAdded.

- New folder: features/edit/atoms/ (10 files).
- Menu bar: Edit / Atoms (added). edit_menu InitOnce signature now takes MouseInteractor&.
- core/scene::EventBus::onAtomsChanged first subscriber.
- core/scene::EventBus::onStructureAdded first subscriber + Phase 3.3 emit backfill.
- core/vtk::MouseInteractor first subscriber (HoverAtom / SelectAtom / DragSelection).
- vtk_renderer.cpp 1,792-line split (2/3) — atom portion ported to atom_renderer.
- legacy Created Atoms UI preserved 1:1 per §6.0.1.

**Visual reward: Phase 3.3 Add atom + Bravais Apply now render on screen.**

Side-by-side screenshot (Created Atoms) and S1/S2/S3-atoms/S5/S6 + Phase 3.3 S3/S4
re-verified scenario tests attached.
Intentional UI deviation: None.

Reference:
  - webassembly/docs/phases/phase3_4_2_edit_atoms.md (this sub-phase)
  - webassembly/docs/phases/phase3_4_split_proposal.md (split rationale)
  - webassembly/docs/phases/phase3_4_edit_atoms_bonds_cell.md (master plan)
"
```

---

## 5. 검증 매트릭스 (14 항목)

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `features/edit/atoms/` 폴더 + edit_menu 갱신 | `ls features/edit/atoms/` + grep `MenuItem.*Atoms` | atoms 폴더 + Atoms 메뉴 항목 1 hit | 정적 |
| 2 | atoms 파일 수 | `ls features/edit/atoms/` | 10 (.cpp 5 + .h 5) | 정적 |
| 3 | namespace 일관성 (`features::edit::atoms::*`) | grep | 모든 .cpp/.h | 정적 |
| 4 | legacy 호출 0 + legacy include 0 + vtk_renderer 의존 0 | grep 3 종 | 0 hit 모두 | 정적 |
| 5 | **`onAtomsChanged.Subscribe` 호출 1+** | grep | 1+ hit (atom_renderer) | 정적 — *핵심* |
| 6 | **`onStructureAdded.Subscribe` 호출 1+** | grep | 1+ hit (atoms_controller) | 정적 — *핵심* |
| 7 | **`onStructureAdded.Emit` 보강 확인** (Phase 3.3) | grep `features/build/bravais/bravais_controller.cpp` | 1+ hit | 정적 — *§1.2.1* |
| 8 | **`core::vtk::MouseInteractor` 첫 구독자** | grep | 1+ hit (atoms_controller) | 정적 — *핵심* |
| 9 | surrounding_atom_manager PBC 알고리즘 보존 | grep `Recompute\|imageAtoms\|periodicImage` | 다수 hit | 정적 |
| 10 | atom_manager 의 5 변경 메서드 보존 | grep `Add\|Remove\|Move\|SetVisible\|Select` | 5+ hit | 정적 |
| 11 | **vtk_renderer atom 분할 — vtkSphereSource / vtkGlyph3D 호출 보존** | grep | 다수 hit (legacy 와 비슷한 갯수) | 정적 — *핵심* |
| 12 | **§1.4.1 UI 보존 — ImGui 위젯 인자 비교** (atom_editor_ui) | legacy 와 새 트리 grep diff | 일치 | 정적 |
| 13 | Debug + Release 빌드 | `npm run build-wasm:*` | exit 0 | 동적 |
| 14 | **§1.4 — 시나리오 S1/S2/S3-atoms/S5/S6 + Phase 3.3 S3/S4 재검증** | 메뉴 클릭 + 시연 | legacy 와 동일 + 회귀 부재 | 동적 — *핵심* |

> 추가 검증 (Side-by-side 스크린샷 + 콘솔 에러 0) 은 §4 Step 12 의 *§1.4 보존 검증* 으로 별도 첨부.

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | vtk_renderer 의 atom 부분 ~700 줄 분할 시 *AtomGroupVTKData 캐시 정책* 일부 누락 | atom 이 그려지지 않거나 메모리 누수 | (a) §5 #11 의 vtk* 호출 갯수 비교. (b) legacy 의 `AtomGroupVTKData::isInitialized()` 같은 캐시 키 grep 으로 1:1 매핑 |
| 6.2 | `onAtomsChanged` 첫 구독자 도착 시 *Phase 3.3 의 emit 자가 무한 루프* — controller 가 emit + 자기 구독 | 무한 루프 | Subscribe 위치를 *atom_renderer 만* 으로 한정. controller / atom_manager 는 emit 만 |
| 6.3 | `onStructureAdded` Phase 3.3 보강 시 BravaisController.Apply 의 호출 sequence 변경 | Phase 3.3 회귀 — 시나리오 S3/S4 반복 실패 | §1.2.1 의 보강은 *추가만* + Phase 3.3 시나리오 S3/S4 재검증 (§5 #14) |
| 6.4 | mouse_interactor 의 픽킹 이벤트 시그니처 — Phase 2 의 `core/vtk/mouse_interactor.h` 가 legacy `mouse_interactor_style.cpp` 와 다를 가능성 | link error 또는 silent failure | Phase 2 인터페이스 사전 확인 + legacy 측 mouse_interactor_style 의 *5 종 picking 메서드* 와 매핑 |
| 6.5 | atom_editor_ui 의 896 줄 압축 중 *컬럼 순서 / 정렬 / 셀 편집 응답 패턴* 누락 | §1.4.1 violation | §5 #12 의 ImGui 위젯 grep diff + Side-by-side 스크린샷 + 시나리오 S2 |
| 6.6 | Bravais Apply 시 atom 4 개 추가 → onAtomsChanged emit → atom_renderer 가 *4 번 reflow* | 성능 부하 | Bravais Apply 가 *batch* 로 처리하도록 Apply 끝에서 1 회만 emit (Phase 3.3 의 emit 자 확인) |
| 6.7 | Cell Apply 시 *fractional 보존 vs cartesian 보존* 정책 — Phase 3.4.1 의 `OnCellChanged` 콜백을 atoms 가 구독해야 함 | atom 좌표 미갱신 | atom_manager 의 `OnCellChanged(structureId, preserveFractional)` 시그니처 확정 + cell_controller 가 본 콜백 호출 |
| 6.8 | surrounding_atom_manager 의 PBC image atom 알고리즘 압축 중 *image 종 (8/26)* 변경 | image atom 표시 차이 — §1.4 violation | legacy 의 *image 종 결정 알고리즘* 을 단위 함수로 분리 + 단위 테스트 |
| 6.9 | edit_menu InitOnce 시그니처 확장 — Phase 3.4.1 호출자가 갱신 안 됨 | link error | app/app.cpp 의 호출 사이트 갱신 (§4 Step 9) — sub-phase PR 안에서 완결 |
| 6.10 | controller 풍부도 — atoms_controller 가 mouse_interactor + onStructureAdded + atom_manager 등 *책임 다수* 흡수해 두꺼워질 가능성 | 라인수 예상 폭 | §9.1 의 *얇은 controller (~200) vs 풍부 controller (~450)* 두 시나리오 명시. Phase 3.3 §1.6.2 의 학습 — 외부 모듈 위임률에 반비례 |

---

## 7. 롤백 절차

```bash
git restore --staged .
git restore .
git clean -fd webassembly/src/features/edit/atoms
git checkout -- webassembly/src/features/build/bravais/bravais_controller.cpp   # Phase 3.3 보강 롤백
git checkout -- webassembly/src/features/edit/edit_menu.cpp webassembly/src/features/edit/edit_menu.h
git checkout -- webassembly/src/app/app.cpp
```

원인 진단 우선순위:

1. **vtk_renderer atom 분할 누락** — §6.1 의 grep 검증 실패.
2. **§1.4.1 UI 보존 위반** — §6.5 의 즉시 복원 또는 사유서.
3. **mouse_interactor 시그니처 불일치** — §6.4 의 Phase 2 인터페이스 점검.
4. **EventBus 무한 루프** — §6.2 의 emit/subscribe 분리.
5. **Phase 3.3 회귀** — §6.3 의 시나리오 S3/S4 재검증.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] §2 의 Phase 3.4.1 전제 충족
- [ ] §5 검증 매트릭스 14 항목 통과
- [ ] features/edit/atoms/ 안에서 legacy 호출 0 + legacy include 0 + vtk_renderer 의존 0
- [ ] **`onAtomsChanged.Subscribe` + `onStructureAdded.Subscribe` 호출 1+ 곳씩**
- [ ] **Phase 3.3 의 onStructureAdded emit 보강 확인** (§1.2.1)
- [ ] **`core::vtk::MouseInteractor` 첫 구독자 도착 확인**
- [ ] **vtk_renderer atom 분할 — vtk* 호출 갯수 legacy 와 비교 (±10% 범위)**
- [ ] **§1.4.1 UI 보존 — Side-by-side 스크린샷 (Created Atoms) 첨부**
- [ ] **시나리오 S1/S2/S3-atoms/S5/S6 통과** (특히 *S1 의 시각화 핵심 보상*)
- [ ] **Phase 3.3 시나리오 S3/S4 재검증** (회귀 부재)
- [ ] **Intentional UI deviation 사유서 작성** (없으면 *"None"*)
- [ ] `legacy/` 한 글자도 변경 없음
- [ ] PR 본문에 *"3.4.3 Bonds 후속 진행 예정"* 명시
- [ ] PR 본문에 본 문서 + split_proposal.md + 모체 plan + Phase 3.4.1 commit hash 링크

검토자 — 머지 전:

- [ ] diff 가 (a) features/edit/atoms/ 신규 10 파일, (b) features/edit/edit_menu 갱신 (Atoms 항목 + InitOnce 시그니처), (c) features/build/bravais/bravais_controller.cpp 의 onStructureAdded emit 1 줄, (d) app/app.cpp 의 InitOnce 호출 시그니처 갱신, (e) CMakeLists.txt source 추가 — 5 가지로만 구성?
- [ ] **첨부 스크린샷에서 legacy 와 새 트리의 Created Atoms 윈도우가 시각적으로 동일?**
- [ ] **시나리오 S1 — Phase 3.3 의 Add atom 이 본인 환경에서 시각화 확인?** *(결정적 보상)*
- [ ] **Phase 3.3 시나리오 S3/S4 본인 환경에서도 회귀 부재?**
- [ ] vtk_renderer atom 분할 — vtk* 호출 갯수가 legacy 와 거의 동일?
- [ ] `legacy/` 0 changed lines
- [ ] CI 빌드 (debug + release) 통과

---

## 9. 부록

### 9.1 features/edit/atoms/ 10 파일 요약 + 라인수 예상

| 파일 | 역할 | 라인 (legacy) | 압축 후 (얇은 controller) | 압축 후 (풍부 controller) |
|---|---|---|---|---|
| `atoms/atom_manager.{cpp,h}` | atom 데이터 변경 | 704 | ~480 | ~480 |
| `atoms/surrounding_atom_manager.{cpp,h}` | PBC image | 409 | ~280 | ~280 |
| `atoms/atom_renderer.{cpp,h}` | vtk_renderer atom 분할 (2/3) | (1,792 의 ~700) | ~500 | ~500 |
| `atoms/atoms_controller.{cpp,h}` | 통합 진입점 + mouse_interactor 구독 + onStructureAdded 구독 | — | ~200 | ~450 |
| `atoms/atom_editor_ui.{cpp,h}` | Created Atoms (§1.4.1) | 942 | ~600 | ~600 |
| **합계** | | **~2,755** | **~2,060** (≈ 75%) | **~2,310** (≈ 84%) |

> Phase 3.3 평가서 §1.6.2 의 *controller 두께 변동성* 반영 — 두 시나리오 모두 명시. 본 단계는 mouse_interactor + onStructureAdded + atom_manager 등 책임 다수라 *풍부 controller* 가능성 더 큼.

### 9.2 사후 점검: Phase 3.4.2 머지 직후 트리

```
webassembly/src/
├─ ... (Phase 0-2 + 3.1 + 3.2 + 3.3 + 3.4.1 유지)
├─ features/
│  └─ edit/
│     ├─ edit_menu.{cpp,h}             (★ 갱신: Atoms 항목 추가 + InitOnce 시그니처 확장)
│     ├─ cell/                          (Phase 3.4.1 — 8 파일)
│     └─ atoms/                         ★ 신규 Phase 3.4.2 (10 파일)
│        ├─ atom_manager.{cpp,h}
│        ├─ surrounding_atom_manager.{cpp,h}
│        ├─ atom_renderer.{cpp,h}        ★ vtk_renderer 분할 결과 (2/3)
│        ├─ atoms_controller.{cpp,h}     ★ mouse_interactor + onStructureAdded 첫 구독자
│        └─ atom_editor_ui.{cpp,h}       ★ §1.4.1 보존 핵심
└─ legacy/                              (동결)
```

### 9.3 Phase 3.3 평가서의 학습 반영

| Phase 3.3 평가서 권장 | 본 sub-phase 적용 위치 |
|---|---|
| §1.6.1 — `core/data` 시그니처 lowerCamelCase | 본 단계는 core/data 미사용 — 3.4.3 에서 검증 |
| §1.6.2 — controller 두께 변동성 | §9.1 의 *얇은/풍부 controller* 두 시나리오 + §6.10 |
| §1.6.3 — commit 미수행 정책 | §2 전제 항목 + §8 PR 체크리스트 |
| §1.6.4 — `onStructureAdded` 미발신 보강 | §1.2.1 의 *본 sub-phase 안 보강* + §4 Step 2 + §5 #7 + §6.3 |
| §1.6.6 — 클래스 vs free function 패턴 | §4 Step 3~7 코드 예시는 *의도된 형태* 임 명시 |

---

## 10. 후속 단계 연결 — Phase 3.4.3 (Bonds)

Phase 3.4.2 머지 후 Phase 3.4.3 (`features/edit/bonds`) 진입.

1. `features/edit/bonds/` 신설 + 8 파일 (bond_manager + bond_renderer + bonds_controller + bond_ui).
2. `edit_menu` 갱신 — Bonds 항목 추가.
3. **`core::scene::EventBus::onBondsChanged` 의 첫 emit + 첫 구독자 (단일 sub-phase 안 양방)** 도착.
4. **`core::data::ElementDatabase` 두 번째 사용자** — bond_manager 가 covalent radii 조회.
5. **vtk_renderer bond 부분 ~700 줄 분할** (3/3 — 분할 완료).
6. 시나리오 S4 (Bonds Recompute → cylinder 표시) 검증.

Phase 3.4.3 세부계획서: [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md).

---

## 11. 관련 문서

- 분할 컨텍스트: [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) §3.1 (Option A) + §5.2 (3.4.2 검증 매트릭스 14 항목)
- 모체 계획서: [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)
- 선행 sub-phase: [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md)
- 후속 sub-phase: [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md)
- 선행 평가서: [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + **§6.0 공통 지침 (UI 1:1 보존)** + §11 (vtk_renderer 분할 리스크) + §13 (UI 이식 공통 지침)
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
- UI 이식 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
