# Phase 3.4.1 — Edit / Cell 이식 (Phase 3.4 의 1/3 — 의존성 루트) 세부계획서

> 분할 컨텍스트: **Phase 3.4 의 1/3** ([`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) Option A 채택 — Cell → Atoms → Bonds 의존성 순서)
> 모체 계획서:    [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)
> 상위 문서:      [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + §6.0 공통 지침
> 선행 평가서:    [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
> 메뉴 매핑:      [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
> UI 이식 지침:   [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01~UI-12)
> 작성일:        2026-05-07
> 대상 브랜치:    `refactor/menu-aligned`
> 단위 PR:       1 개
> 예상 소요:      1.5~2 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-07 | 초안 작성 (Phase 3.4 모체 계획서를 분할안 Option A 에 따라 세분화) |

---

## 0. 한 줄 요약

> Phase 3.4 의 *세 sub-phase 중 첫 번째* 로, Edit 메뉴의 *Cell 항목만* 이식한다. `features/edit/cell/` 신설 + `edit_menu.{cpp,h}` *skeleton* (Cell 항목만 wiring) 작성. legacy `cell_manager.{cpp,h}` (182 줄) + `cell_info_ui.{cpp,h}` (144 줄) + `vtk_renderer.cpp` 의 *cell 부분* (~300 줄) 을 이식. **`core/scene/EventBus::onCellChanged` 의 첫 구독자** 도착으로 Phase 3.3 의 emit 자 (BravaisController.Apply) 와 결합 — Phase 3.4 의 *4 중 인프라 검증* 중 *EventBus::onCellChanged 한 종* 의 검증 완료.
> Phase 3.4 모체 계획서가 *4,400 줄, 22 파일* 의 거대 단위였던 것을 본 sub-phase 가 **~700 줄, 10 파일** 의 가벼운 단위로 시작 — Phase 3.7 viewer (예상 ~600) 수준. 의존성 루트로서 후속 3.4.2/3.4.3 의 *분할 패턴 정립* 역할.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `features/edit/cell/` sub-folder 신설 + 8 파일 작성 + `features/edit/edit_menu.{cpp,h}` *skeleton* 2 파일 (Cell 항목만 wiring). (b) 메뉴 `Edit / Cell` 클릭 시 Cell Information 윈도우 표시. (c) **legacy `vtk_renderer.cpp` 의 *cell 부분 ~300 줄* 분할 이식** — `cell_renderer.{cpp,h}` 신설. (d) `core/scene/EventBus::onCellChanged` 의 첫 구독자 — Phase 3.3 의 BravaisController.Apply 의 emit 자와 결합. (e) `npm run build-wasm:debug` + `:release` 통과 + 런타임에서 Edit / Cell 메뉴 동작 + Bravais Apply 시 lattice matrix 갱신 시각 확인. (f) **legacy 의 Cell Information UI 동작이 1:1 보존됨** (§1.4 — 본 단계는 Cell Information 윈도우 1 종만) |
| **비목표** | Edit / Atoms / Bonds 메뉴 항목 (Phase 3.4.2 / 3.4.3 진행), atom_renderer / bond_renderer 분할 (각 sub-phase 가 자기 부분만 분할), mouse_interactor 첫 구독자 (Phase 3.4.2 의 영역), element_database 두 번째 사용자 (Phase 3.4.3 의 영역), `menu_router` 정식 도입 (Phase 4), Cell Align (X/Y/Z) 의 *toolbar 측 UI* (Phase 3.7), **legacy UI 의 *재설계* — Cell Information 윈도우의 lattice matrix 표시 형식 / 행 순서 / 소수점 자릿수 등은 변경 금지** |

> Phase 3.4.1 의 미덕: *"검토자가 git diff 를 보고 `features/edit/cell/` 의 신규 8 파일 + `features/edit/edit_menu.{cpp,h}` 의 skeleton + `app/app.cpp` 의 메뉴 hook 1 줄 + `CMakeLists.txt` 의 source 추가 외에 의심할 게 없고, side-by-side 스크린샷에서 legacy 와 새 트리의 Cell Information 윈도우가 시각적으로 동일하며, **Phase 3.3 의 Bravais Apply 후 lattice matrix 가 새 트리의 Cell Information 윈도우에 갱신되어 보인다**"*.

### 1.1 Phase 3.4.1 의 의의 — *분할 패턴 정립 + onCellChanged 첫 구독자*

본 sub-phase 는 Phase 3.4 의 3 분할 중 **가장 가벼운 (~700 줄)** 단계로서, 다음 *템플릿* 역할을 한다:

| 정립할 패턴 | 본 sub-phase 의 첫 사례 | 후속 sub-phase 의 적용 |
|---|---|---|
| `features/edit/edit_menu.{cpp,h}` *skeleton* + 점진 항목 추가 | Cell 항목만 wiring | 3.4.2 가 Atoms 항목 추가, 3.4.3 가 Bonds 항목 추가 |
| `vtk_renderer.cpp` *부분 분할* (cell ~300 줄) | cell_renderer.{cpp,h} | 3.4.2 atom_renderer (~700), 3.4.3 bond_renderer (~700) |
| `core::scene::EventBus` *N 종 구독자 도착* | onCellChanged 1 종 | 3.4.2 onAtomsChanged + onStructureAdded + mouse_interactor, 3.4.3 onBondsChanged emit+구독 |
| `core::scene::SceneState::structureRecords::cell` *직접 변경자* | cell_manager 가 lattice matrix / 파라미터 변경 | 3.4.2 atom_manager, 3.4.3 bond_manager |
| `*_controller` 두께 결정 | cell_controller 가 *얇은 흐름* (Phase 3.3 의 BravaisController 105 줄 패턴) | Atoms/Bonds 의 controller 도 동일 *외부 모듈 위임률* 패턴 적용 |

→ Phase 3.3 평가서 §1.6.2 의 *"controller 두께는 외부 모듈 위임률에 반비례"* 가설을 **본 단계가 첫 검증**.

### 1.2 회색지대 정책 인계 (Phase 0 §1.1 + Phase 1 §1.2 + Phase 3.1 §1.3 + Phase 3.4 §1.2.1)

본 계획서에서도 네 회색지대 정책을 그대로 적용.

| 회색지대 출처 | 적용 범위 (Phase 3.4.1) |
|---|---|
| Phase 0 §1.1 — typo build-fix | legacy/ 내부의 1~5 줄 typo |
| Phase 1 §1.2 — 빌드 호환성 shim | 새 트리 측의 얇은 redirect 파일 |
| Phase 3.1 §1.3 — 임시 메뉴 hook | `app/app.cpp::renderDockSpace` 에 `features::edit::DrawMenu()` 추가 |
| Phase 3.4 §1.2.1 — vtk_renderer 분할 | legacy 측 vtk_renderer 는 동결 유지. *cell 부분 ~300 줄을 새 트리에 복사 + namespace 정리* |

### 1.3 라인수 압축 정책 (Phase 3.1/3.2/3.3 의 학습 적용)

Phase 3.1 (53%) / 3.2 (57.5%) / 3.3 (61.2%) 의 압축 패턴을 본 단계에서도 적용. 단 cell 도메인은 *수학적 변환 (lattice matrix ↔ 파라미터)* + *vtk_renderer 의 cell 부분 분할* 로 압축 여지가 작아 **약 70% 수준** 으로 보수적 예상.

**보존 필수 (압축 시에도 변경 금지)**:

- `cell_manager.cpp` 의 lattice matrix ↔ 파라미터 (a/b/c/α/β/γ) 변환 알고리즘
- `cell_info_ui.cpp` 의 lattice matrix 표시 *행 순서 + 소수점 자릿수* (§1.4.1)
- vtk_renderer 의 *cell 부분* (`createUnitCell`, `clearUnitCell`, `setUnitCellVisible` 등) 의 vtk 호출 시퀀스 (§1.2.1)
- §1.4 의 모든 UI 보존 항목

### 1.4 legacy UI 작동방식 1:1 보존 (상위 §6.0.1 의 본 단계 적용)

상위 계획서 §6.0.1 + UI 이식 공통 지침 [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) 의 UI-01~UI-12 가 본 단계에 그대로 적용. 본 sub-phase 의 보존 대상은 **Cell Information 윈도우 1 종**.

#### 1.4.1 Cell Information 윈도우 보존 대상

`legacy/atoms/ui/cell_info_ui.cpp` (101 줄) 가 정의하는 다음 UI 동작은 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| Lattice matrix 표시 | 3×3 매트릭스 (a/b/c 행) — legacy 의 *행 순서* 와 *소수점 자릿수* (legacy 가 `%.4f` 또는 `%.6f` 사용했다면 동일) |
| Lattice 파라미터 | a / b / c / α / β / γ 의 *읽기 전용 표시 vs 편집 가능* 여부 (legacy 의 `ImGui::InputFloat` vs `ImGui::Text` 선택 보존) |
| Volume / Density | 자동 계산된 *셀 볼륨 + 원자 밀도* 의 표시 형식 |
| Apply 버튼 | matrix 또는 파라미터 변경 후 Apply — *기존 atom 위치 처리* 정책 (fractional 보존 vs cartesian 보존). **본 단계는 atom 측이 미도착이므로 controller 인터페이스만 노출**, atom 좌표 갱신은 Phase 3.4.2 진입 후 비로소 가능 |
| Cell Align (X/Y/Z) | toolbar 의 inline 버튼 — *Phase 3.7 viewer/toolbar* 가 본 cell_controller 의 콜백을 호출. **본 단계는 cell_controller 의 공개 콜백만 노출**, UI 자체는 toolbar 가 그림 (Phase 3.7) |

#### 1.4.2 보존 검증 절차 (상위 §6.0.2 의 본 단계 적용)

PR 작성자는 본 PR 본문에 다음 검증을 첨부:

1. **Side-by-side 스크린샷** — legacy Cell Information vs 새 트리의 같은 윈도우.
2. **사용자 시나리오 정합성** — 다음 1 시나리오를 legacy 와 새 트리에서 차례로 실행해 동일 결과 확인:
   - **(S3-cell)** Bravais Lattice Templates → "FCC (Face-Centered Cubic)" → a=3.5 → Apply → **Cell Information 윈도우의 lattice matrix 가 갱신** + cell 외곽선 액터 갱신 *(atom 측은 Phase 3.4.2 후에야 시각화 가능)*
3. **Intentional UI deviation 사유서** — 의도적인 UI 변경 발생 시 PR 본문 명기 (없으면 *"None"*).

> 모체 계획서의 시나리오 S3 *"FCC + Triclinic 적용 → unit cell 매트릭스 + atom 4 개 추가"* 중 *cell 부분만* 본 단계에서 검증. atom 시각화는 3.4.2 에서 *S3-atoms* 로 추가 검증.

#### 1.4.3 압축과 UI 보존의 조화 (예시)

| 압축 가능 ✓ | 보존 필수 ✗ (변경 금지) |
|---|---|
| 한국어 깨진 인코딩 주석 → Doxygen | Lattice matrix 의 *행 순서* + *소수점 자릿수* |
| `m_parent->X()` → `controller_.X()` | Apply 시 *fractional 보존 vs cartesian 보존* 정책 (controller 인터페이스에 보존) |
| `printf` 디버그 제거 | Volume / Density 표시 형식 |
| 미사용 멤버 제거 | unitCell 액터의 `Modified` 호출 시점 (vtk_renderer 분할 시 보존) |

---

## 2. 전제 — Phase 3.3 완료 상태

본 계획서는 다음이 충족된 상태에서 시작.

- [ ] Phase 3.3 commit 머지 ([`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md) §4.3.A 통과)
- [ ] `webassembly/src/features/build/{periodic_table, bravais}/` 16 파일 + 임시 메뉴 hook 정상 동작
- [ ] `core/{scene, io, data, vtk, render, ui}` 인프라 빌드 가능
- [ ] `npm run build-wasm:debug` + `:release` exit 0
- [ ] Build 메뉴 2 항목 동작 (Phase 3.3 §5 #15-23 통과)
- [ ] **`core::scene::EventBus::onCellChanged.Emit` 호출 확인됨** (Phase 3.3 의 BravaisController.Apply:97) — 본 단계에서 *구독자가 도착* 할 emit 자.

---

## 3. legacy 참조 인벤토리

| legacy 파일 | 라인 수 | 역할 | 새 트리 행선지 |
|---|---|---|---|
| `legacy/atoms/domain/cell_manager.{cpp,h}` | 137 + 45 | unit cell + lattice 매트릭스 / 파라미터 변환 | `features/edit/cell/cell_manager.{cpp,h}` |
| `legacy/atoms/ui/cell_info_ui.{cpp,h}` | 101 + 43 | Cell Information 윈도우 ImGui | `features/edit/cell/cell_info_ui.{cpp,h}` |
| `legacy/atoms/infrastructure/vtk_renderer.{cpp,h}` | 1,792 + 435 | atom + bond + cell 액터 — *본 sub-phase 는 cell 부분 ~300 줄만 분할* | `features/edit/cell/cell_renderer.{cpp,h}` (`createUnitCell` / `clearUnitCell` / `setUnitCellVisible` 등) |
| **(신규)** | — | `cell_controller`, `edit_menu` skeleton | 신규 작성 |

**총 legacy 참조: ~626 줄.** 압축 후 약 **~700 줄** 예상 (cell_renderer 가 신규 분할이라 약간 풍부).

### 3.1 외부 의존 사전 분석

| legacy 파일 | 외부 의존 | 새 트리 처리 |
|---|---|---|
| `cell_manager.{cpp,h}` | STL 만 + 자체 매트릭스 연산 | namespace 만 변경 |
| `cell_info_ui.{cpp,h}` | cell_manager, `class AtomsTemplate` (forward) | controller 로 분기 |
| `vtk_renderer.{cpp,h}` (cell 부분) | vtk* 다수, cell_manager | `core::vtk::VtkViewer::Instance().GetRenderer()->AddActor(...)` 직접. cell_manager 의존은 그대로 |

### 3.2 메뉴 트리 매핑 (04 §4 Edit 참조)

| 메뉴 항목 | 새 진입점 | 윈도우 |
|---|---|---|
| `Edit / Cell` | `features::edit::cell::Show()` | Cell Information |

→ 본 sub-phase 는 *1 메뉴 항목만* 활성. Atoms / Bonds 항목은 후속 sub-phase 에서 추가.

### 3.3 vtk_renderer cell 부분 분할 정책

Phase 3.4 §1.2.1 의 분할 정책을 본 단계에서 처음 적용.

| 영역 | 분할 대상 vtk* 호출 | 새 모듈 |
|---|---|---|
| Unit cell 액터 | `vtkOutlineSource`, `vtkAxesActor`, `vtkPolyDataMapper`, `vtkActor` | `cell_renderer.cpp` |
| 기능 | `createUnitCell(matrix)`, `createUnitCell(structureId, matrix)`, `clearUnitCell()`, `clearUnitCell(structureId)`, `setUnitCellVisible(bool)`, `setUnitCellVisible(structureId, bool)`, `hasUnitCell(structureId)`, `isUnitCellVisible(structureId)` | 8 메서드 그대로 보존 |

> 분할 시 보존 필수: 각 vtk* 호출의 *호출 순서 + 인자 + Modified 호출 시점* 은 legacy 와 1:1.

---

## 4. 작업 절차 (단계별)

### Step 1 — `features/edit/cell/` 폴더 신설

```bash
cd webassembly/src
mkdir -p features/edit/cell
```

### Step 2 — `cell_manager.{cpp,h}` 도메인 이식 (가장 가벼움)

legacy 182 줄을 namespace 정리만 하며 이식.

```cpp
/**
 * @file features/edit/cell/cell_manager.h
 * @brief Unit cell + lattice 매트릭스 / 파라미터 변환.
 */
#pragma once
#include "core/scene/scene_state.h"

#include <array>

namespace features::edit::cell {

class CellManager {
public:
    explicit CellManager(core::scene::SceneState& scene);

    void SetMatrix(int32_t structureId, const std::array<std::array<float, 3>, 3>& m);
    void SetParameters(int32_t structureId,
                       float a, float b, float c,
                       float alpha, float beta, float gamma);

    std::array<std::array<float, 3>, 3> GetMatrix(int32_t structureId) const;
    void GetParameters(int32_t structureId,
                       float& a, float& b, float& c,
                       float& alpha, float& beta, float& gamma) const;

    float Volume(int32_t structureId) const;

    /// @brief Cell Align (X/Y/Z) — toolbar (Phase 3.7) 가 호출할 콜백.
    void AlignAxis(int32_t structureId, int axis);

private:
    core::scene::SceneState& scene_;
};

} // namespace features::edit::cell
```

> **EventBus emit**: `SetMatrix` / `SetParameters` / `AlignAxis` 끝에서 `scene_.events.onCellChanged.Emit(...)` 호출. *본 단계에서는 Phase 3.3 의 emit 자 (BravaisController.Apply) 와 동일한 이벤트를 cell_manager 도 emit*.

### Step 3 — `cell_renderer.{cpp,h}` 신설 (vtk_renderer cell 부분 분할)

legacy `vtk_renderer.cpp` 의 cell 관련 ~300 줄을 *복사 + namespace 정리* 형태로 이식.

```cpp
/**
 * @file features/edit/cell/cell_renderer.h
 * @brief Unit cell 액터 그룹 — Phase 3.4.1 의 vtk_renderer 분할 (1/3).
 */
#pragma once
#include "core/scene/scene_state.h"
#include "core/vtk/vtk_viewer.h"

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <unordered_map>

namespace features::edit::cell {

class CellRenderer {
public:
    explicit CellRenderer(core::scene::SceneState& scene);
    ~CellRenderer();

    /// @brief EventBus::onCellChanged 구독 — Phase 3.3 의 BravaisController.Apply 와 결합.
    void Subscribe();

    void CreateUnitCell(int32_t structureId,
                        const std::array<std::array<float, 3>, 3>& matrix);
    void ClearUnitCell(int32_t structureId);
    void SetUnitCellVisible(int32_t structureId, bool visible);
    bool HasUnitCell(int32_t structureId) const;
    bool IsUnitCellVisible(int32_t structureId) const;

private:
    core::scene::SceneState&                                  scene_;
    std::unordered_map<int32_t, vtkSmartPointer<vtkActor>>    cellActors_;
    bool                                                       globalHidden_ = false;
};

} // namespace features::edit::cell
```

> **§1.2.1 분할 검증**: legacy vtk_renderer 의 `createUnitCell` / `clearUnitCell` / `setUnitCellVisible` 호출 시 *vtkOutlineSource → mapper → actor → AddActor* 시퀀스가 새 트리에서도 *완전 동일* — §5 #8 grep diff 로 검증.
>
> **§1.1 첫 구독자 도착**: `Subscribe()` 안에서 `scene_.events.onCellChanged.Subscribe(...)` — Phase 3.3 의 emit 자와 결합. 본 단계의 *결정적 인프라 검증*.

### Step 4 — `cell_controller.{cpp,h}` (얇은 흐름)

Phase 3.3 의 BravaisController (105 줄) 패턴을 따른 *얇은 controller*.

```cpp
/**
 * @file features/edit/cell/cell_controller.h
 * @brief Cell Information 의 통합 진입점 — UI ↔ 도메인.
 */
#pragma once
#include "cell_manager.h"
#include "cell_renderer.h"

namespace features::edit::cell {

class CellController {
public:
    explicit CellController(core::scene::SceneState& scene);

    CellManager&  Mgr()  { return manager_; }
    CellRenderer& Rend() { return renderer_; }

    /// @brief Cell Align (X/Y/Z) — Phase 3.7 toolbar 가 호출.
    void AlignAxis(int axis);

private:
    core::scene::SceneState& scene_;
    CellManager              manager_;
    CellRenderer             renderer_;
};

} // namespace features::edit::cell
```

### Step 5 — `cell_info_ui.{cpp,h}` UI 이식 (§1.4.1 보존)

legacy 144 줄을 namespace 정리하며 이식. ImGui 위젯 흐름 1:1 보존.

```cpp
/**
 * @file features/edit/cell/cell_info_ui.h
 * @brief Cell Information 윈도우 ImGui — §1.4.1 보존.
 */
#pragma once
#include <imgui.h>

namespace features::edit::cell {

class CellController;

class CellInfoUI {
public:
    explicit CellInfoUI(CellController& controller);
    void Render(bool* open);

private:
    CellController& controller_;
    // ★ §1.4.1 보존: lattice matrix 표시 / 파라미터 / Volume·Density / Apply 응답 패턴 그대로
};

} // namespace features::edit::cell
```

### Step 6 — `edit_menu.{cpp,h}` *skeleton* 작성

Phase 3.3 의 `build_menu` 패턴. **본 단계는 Cell 항목만 wiring** (3.4.2 / 3.4.3 가 점진 추가).

```cpp
/**
 * @file features/edit/edit_menu.h
 * @brief Edit 메뉴 — 3 항목 (Atoms / Bonds / Cell) 진입점.
 *
 * @note Phase 3.4.1 에서 Cell 항목만 wiring. Atoms 는 3.4.2, Bonds 는 3.4.3 에서 추가.
 */
#pragma once
namespace core::scene { struct SceneState; }
namespace core::vtk   { class MouseInteractor; }

namespace features::edit {

void DrawMenu();
void RenderWindows();
void Tick(float dt);
void Shutdown();
void InitOnce(core::scene::SceneState& scene);
// 3.4.2 진입 시 InitOnce 시그니처에 MouseInteractor& 추가 예정.

} // namespace features::edit
```

```cpp
// features/edit/edit_menu.cpp
namespace features::edit {

namespace {
    bool g_showCellInfo = false;
    cell::CellController* g_cellCtrl = nullptr;
    cell::CellInfoUI*     g_cellUI   = nullptr;
}

void DrawMenu() {
    if (ImGui::BeginMenu("Edit")) {
        // Phase 3.4.2 에서 Atoms 항목 추가 예정.
        // Phase 3.4.3 에서 Bonds 항목 추가 예정.
        if (ImGui::MenuItem("Cell")) {
            g_showCellInfo = true;
        }
        ImGui::EndMenu();
    }
}

void RenderWindows() {
    if (g_showCellInfo && g_cellUI) g_cellUI->Render(&g_showCellInfo);
}

void InitOnce(core::scene::SceneState& scene) {
    static cell::CellController cellCtrl(scene);
    static cell::CellInfoUI     cellUI(cellCtrl);
    cellCtrl.Rend().Subscribe();   // ★ onCellChanged 첫 구독자

    g_cellCtrl = &cellCtrl;
    g_cellUI   = &cellUI;
}

} // namespace features::edit
```

### Step 7 — `app/app.cpp` 임시 메뉴 hook

```cpp
#include "../features/edit/edit_menu.h"   // 신규

void App::renderDockSpace() {
    if (ImGui::BeginMenuBar()) {
        ...
        features::utilities::bz::DrawMenu();
        features::data::DrawMenu();
        features::build::DrawMenu();
        features::edit::DrawMenu();                     // ← 신규 Phase 3.4.1
        ImGui::EndMenuBar();
    }
    features::utilities::bz::RenderWindows(nullptr);
    features::data::RenderWindows();
    features::build::RenderWindows();
    features::edit::RenderWindows();                     // ← 신규
}

int App::Init() {
    static core::scene::SceneState scene;
    features::utilities::bz::InitOnce(scene);
    features::data::InitOnce(scene);
    features::build::InitOnce(scene);
    features::edit::InitOnce(scene);                     // ← 신규
}
```

### Step 8 — `CMakeLists.txt` SOURCES_FEATURES 확장

```cmake
set(SOURCES_FEATURES
    # Phase 3.1~3.3
    ...
    # Phase 3.4.1 신규
    webassembly/src/features/edit/edit_menu.cpp
    webassembly/src/features/edit/edit_menu.h
    webassembly/src/features/edit/cell/cell_manager.cpp
    webassembly/src/features/edit/cell/cell_manager.h
    webassembly/src/features/edit/cell/cell_renderer.cpp
    webassembly/src/features/edit/cell/cell_renderer.h
    webassembly/src/features/edit/cell/cell_controller.cpp
    webassembly/src/features/edit/cell/cell_controller.h
    webassembly/src/features/edit/cell/cell_info_ui.cpp
    webassembly/src/features/edit/cell/cell_info_ui.h
)
```

총 10 신규 파일.

### Step 9 — 정적 검증

```bash
cd webassembly/src

# legacy 호출 0
grep -rnE "AtomsTemplate::|atoms::domain::|atoms::infrastructure::|atoms::ui::" \
  features/edit/ | grep -v "//" || echo "OK"

# legacy include 0
grep -rnE '#include\s+"(\.\./)*legacy/' features/edit/ | grep -v "//" || echo "OK"

# **onCellChanged 구독 1+ 곳**
grep -rnE "onCellChanged.Subscribe|onCellChanged\.Subscribe" features/edit/cell/ | head -5

# **vtk_renderer cell 부분 분할 검증** — vtkOutlineSource / vtkAxesActor 호출 보존
grep -nE "vtkOutlineSource|vtkAxesActor|createUnitCell|CreateUnitCell" \
  features/edit/cell/cell_renderer.cpp | head -10

# legacy vtk_renderer 의존 0
grep -rnE "atoms::infrastructure::VTKRenderer" features/edit/ | grep -v "//" || echo "OK"

# namespace 일관성
grep -rnE "^namespace features::edit" features/edit/

# **§1.4 UI 보존 — ImGui 위젯 인자 grep diff (cell_info_ui)**
grep -nE 'ImGui::(InputFloat|Text|Button)' \
  legacy/atoms/ui/cell_info_ui.cpp \
  features/edit/cell/cell_info_ui.cpp 2>/dev/null
```

### Step 10 — 빌드 + 런타임 검증 (Windows 측)

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release
npm run dev
```

**§1.4 보존 검증**:

- [ ] Side-by-side 스크린샷 (legacy Cell Information vs 새 트리)
- [ ] 시나리오 S3-cell — Bravais Apply → lattice matrix 갱신 + cell 외곽선 액터 갱신 (atom 측 미시각화는 정상)
- [ ] Intentional UI deviation 사유서

### Step 11 — 커밋 & PR

```powershell
git commit -m "Phase 3.4.1 (1/3): features/edit/cell — first sub-phase

This PR is 1/3 of Phase 3.4. It introduces the cell sub-folder, the edit_menu
skeleton with the Cell item only, and splits the vtk_renderer cell portion.

- New folder: features/edit/cell/ (8 files) + edit_menu.{cpp,h} skeleton (2 files).
- Menu bar: Edit (skeleton) / Cell.
- core/scene::EventBus::onCellChanged first subscriber — pairs with Phase 3.3 emit
  (BravaisController.Apply).
- vtk_renderer.cpp 1,792-line split (1/3) — cell portion (~300 lines) ported to
  cell_renderer.
- legacy Cell Information UI preserved 1:1 per §6.0.1.

Side-by-side screenshot (Cell Information) and S3-cell scenario test attached.
Intentional UI deviation: None.

Reference:
  - webassembly/docs/phases/phase3_4_1_edit_cell.md (this sub-phase)
  - webassembly/docs/phases/phase3_4_split_proposal.md (split rationale)
  - webassembly/docs/phases/phase3_4_edit_atoms_bonds_cell.md (master plan)
"
```

---

## 5. 검증 매트릭스 (14 항목)

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `features/edit/cell/` + `features/edit/edit_menu.{cpp,h}` 존재 | `ls features/edit/` | cell 폴더 + edit_menu 2 파일 | 정적 |
| 2 | cell 파일 수 | `ls features/edit/cell/` | 8 (.cpp 4 + .h 4) | 정적 |
| 3 | edit_menu skeleton | grep `MenuItem.*Cell` features/edit/edit_menu.cpp | 1 hit | 정적 |
| 4 | namespace 일관성 (`features::edit::cell::*`) | grep | 모든 .cpp/.h | 정적 |
| 5 | legacy 호출 0 | grep | 0 hit | 정적 |
| 6 | `#include "../legacy/"` 0 | grep | 0 hit | 정적 |
| 7 | legacy `vtk_renderer` 의존 0 | grep `atoms::infrastructure::VTKRenderer` | 0 hit | 정적 |
| 8 | **`core::scene::EventBus::onCellChanged.Subscribe` 호출 1+** | grep | 1+ hit (cell_renderer 또는 cell_controller) | 정적 — *핵심* |
| 9 | **vtk_renderer cell 분할 — vtkOutlineSource / vtkAxesActor / createUnitCell 호출 보존** | grep `vtkOutlineSource\|vtkAxesActor` | 다수 hit (legacy 와 비슷한 갯수) | 정적 — *핵심* |
| 10 | cell_manager 의 lattice matrix ↔ 파라미터 변환 보존 | grep `SetMatrix\|SetParameters\|GetMatrix\|GetParameters\|Volume` | 5 hit | 정적 |
| 11 | **§1.4.1 UI 보존 — ImGui 위젯 인자 비교** | legacy 와 새 트리 grep diff | 일치 | 정적 |
| 12 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 13 | Release 빌드 | `npm run build-wasm:release` | exit 0 | 동적 |
| 14 | **§1.4 — 시나리오 S3-cell** (Bravais Apply → lattice matrix 갱신) | 메뉴 클릭 + 시연 | legacy 와 동일 | 동적 — *핵심* |

> 추가 검증 (Side-by-side 스크린샷 + 콘솔 에러 0) 은 §4 Step 10 의 *§1.4 보존 검증* 으로 별도 첨부.

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | vtk_renderer 의 cell 부분 ~300 줄 분할 시 *액터 캐싱 정책* 누락 | 셀 외곽선이 그려지지 않거나 메모리 누수 | (a) §5 #9 의 vtk* 호출 갯수 비교. (b) legacy 의 `m_unitCellGlobalHidden` / `m_cellActors` 같은 캐시 키 grep 으로 1:1 매핑 |
| 6.2 | onCellChanged 구독 시 *Phase 3.3 의 emit 자가 무한 루프* — controller 가 emit + 자기 구독 | 무한 루프 | Subscribe 위치를 *renderer 만* 으로 한정. controller / cell_manager 는 emit 만 |
| 6.3 | cell_info_ui 의 lattice matrix 표시 *행 순서* 가 압축 중 변경 | §1.4.1 violation | §5 #11 의 ImGui 위젯 grep diff + Side-by-side 스크린샷 |
| 6.4 | Cell Apply 시 *fractional 보존 vs cartesian 보존* 정책 — *atom 측이 미도착* 이라 본 단계에서는 *controller 인터페이스만* 노출. atom 도착 (3.4.2) 후 *실제 atom 좌표 갱신* 흐름 검증 필요 | Phase 3.4.2 진입 시점에 회귀 가능 | controller 의 `OnCellChanged(int32_t structureId, bool preserveFractional)` 콜백 시그니처를 본 단계에서 *명시적으로 정의* + Phase 3.4.2 의 atoms_controller 가 본 콜백을 구독 |
| 6.5 | edit_menu skeleton 이 *Cell 만* wiring — Atoms / Bonds 항목 부재 | 사용자가 *"Edit 메뉴가 빈약하다"* 로 오해 | PR 본문에 *"Atoms 는 3.4.2, Bonds 는 3.4.3 에서 추가 예정"* 명시. ImGui::BeginMenu 안에 주석 형태로 *placeholder* 표시 가능 |
| 6.6 | cell_manager 의 매트릭스 ↔ 파라미터 변환 알고리즘 압축 중 *수치 오차* 도입 | 셀 시각화 정확도 차이 | legacy 의 변환 함수를 *비트 동등 복사* + namespace 만 변경 |

---

## 7. 롤백 절차

```bash
git restore --staged .
git restore .
git clean -fd webassembly/src/features/edit
```

원인 진단 우선순위:

1. **vtk_renderer cell 분할 누락** — §6.1 의 grep 검증 실패 → legacy 측 행 분포 재추정.
2. **§1.4.1 UI 보존 위반** — §6.3 의 즉시 복원 또는 사유서.
3. **EventBus 무한 루프** — §6.2 의 emit/subscribe 분리 검토.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] §2 의 Phase 3.3 전제 충족
- [ ] §5 검증 매트릭스 14 항목 통과
- [ ] features/edit/ 안에서 legacy 호출 0 + legacy include 0 + vtk_renderer 의존 0
- [ ] **`core::scene::EventBus::onCellChanged.Subscribe` 호출 1+ 곳** (인프라 검증)
- [ ] **vtk_renderer cell 분할 — vtkOutlineSource / vtkAxesActor 호출 갯수 legacy 와 비교 (±10% 범위)**
- [ ] **§1.4.1 UI 보존 — Side-by-side 스크린샷 (Cell Information) 첨부**
- [ ] **시나리오 S3-cell 통과 — Bravais Apply 후 lattice matrix 갱신 확인**
- [ ] **Intentional UI deviation 사유서 작성** (없으면 *"None"*)
- [ ] `legacy/` 한 글자도 변경 없음 (vtk_renderer 분할은 *복사 + 새 트리 정리* 로만 진행)
- [ ] PR 본문에 *"3.4.2 Atoms / 3.4.3 Bonds 후속 진행 예정"* 명시
- [ ] PR 본문에 본 문서 + split_proposal.md + 모체 plan 링크

검토자 — 머지 전:

- [ ] diff 가 (a) features/edit/cell/ 신규 8 파일, (b) features/edit/edit_menu skeleton 2 파일, (c) app/app.cpp 임시 hook 4 곳, (d) CMakeLists.txt source 추가 — 4 가지로만 구성?
- [ ] **첨부 스크린샷에서 legacy 와 새 트리의 Cell Information 윈도우가 시각적으로 동일?**
- [ ] **시나리오 S3-cell 본인 환경에서도 동일 결과 재현?**
- [ ] vtk_renderer cell 분할 — vtk* 호출 갯수가 legacy 와 거의 동일?
- [ ] `legacy/` 0 changed lines
- [ ] CI 빌드 (debug + release) 통과

---

## 9. 부록

### 9.1 features/edit/cell/ + edit_menu skeleton 10 파일 요약 + 라인수 예상

| 파일 | 역할 | 라인 (legacy) | 압축 후 예상 |
|---|---|---|---|
| `edit_menu.{cpp,h}` | Edit 메뉴 skeleton — Cell 항목만 (신규) | — | ~80 |
| `cell/cell_manager.{cpp,h}` | matrix / 파라미터 변환 | 182 | ~130 |
| `cell/cell_renderer.{cpp,h}` | vtk_renderer cell 분할 (신규) | (1,792 의 ~300) | ~250 |
| `cell/cell_controller.{cpp,h}` | 통합 진입점 (신규, 얇은 controller) | — | ~120 |
| `cell/cell_info_ui.{cpp,h}` | Cell Information (§1.4.1 보존) | 144 | ~100 |
| **합계** | | **~626** *(vtk_renderer 의 ~300 만 포함)* | **~680** |

> Phase 3.3 의 61.2% 보다 보수적 — *vtk_renderer cell 분할 + §1.4.1 보존* 으로 압축 여지 작음.

### 9.2 사후 점검: Phase 3.4.1 머지 직후 트리

```
webassembly/src/
├─ ... (Phase 0-2 + 3.1 + 3.2 + 3.3 유지)
├─ features/
│  ├─ utilities/brillouin_zone/  (Phase 3.1)
│  ├─ data/                       (Phase 3.2)
│  ├─ build/                      (Phase 3.3)
│  └─ edit/                       ★ 신규 Phase 3.4.1
│     ├─ edit_menu.{cpp,h}        ★ skeleton — Cell 항목만 wiring
│     └─ cell/                    (8 파일)
│        ├─ cell_manager.{cpp,h}
│        ├─ cell_renderer.{cpp,h}    ★ vtk_renderer 분할 결과 (1/3)
│        ├─ cell_controller.{cpp,h}  ★ onCellChanged 첫 구독자
│        └─ cell_info_ui.{cpp,h}     ★ §1.4.1 보존
└─ legacy/                        (동결 — vtk_renderer 도 그대로)
```

### 9.3 Phase 3.3 평가서의 학습 반영

| Phase 3.3 평가서 권장 | 본 sub-phase 적용 위치 |
|---|---|
| §1.6.1 — `core/data` 시그니처 lowerCamelCase | 본 단계는 core/data 미사용 — 3.4.3 에서 검증 |
| §1.6.2 — controller 두께 변동성 | §1.1 의 *얇은 controller 패턴 첫 검증* + §9.1 의 라인수 예상 |
| §1.6.3 — commit 미수행 정책 | §2 전제 항목 + §8 PR 체크리스트 첫 행 |
| §1.6.4 — `onStructureAdded` 미발신 보강 | 본 단계에서는 *구독자 미도착* — 3.4.2 에서 보강 + 구독 |
| §1.6.6 — 클래스 vs free function 패턴 | §4 Step 2~5 코드 예시는 *의도된 형태* 임 명시 |

---

## 10. 후속 단계 연결 — Phase 3.4.2 (Atoms)

Phase 3.4.1 머지 후 Phase 3.4.2 (`features/edit/atoms`) 진입.

1. `features/edit/atoms/` 신설 + 10 파일 (atom_manager + surrounding_atom_manager + atom_renderer + atoms_controller + atom_editor_ui).
2. `edit_menu` 갱신 — Atoms 항목 추가 + InitOnce 시그니처에 `MouseInteractor&` 추가.
3. **`core::scene::EventBus::onAtomsChanged` + `onStructureAdded` 첫 구독자** 도착.
4. **`core::vtk::MouseInteractor` 첫 구독자** 도착 (atom 픽킹).
5. **vtk_renderer atom 부분 ~700 줄 분할** (2/3).
6. *Phase 3.3 의 onStructureAdded emit 보강* (§3.3 평가서 §1.6.4) — 본 단계 또는 Phase 3.4.2 PR 안에서 처리.
7. 시나리오 S1 (Add atom 시각화 — 핵심 보상), S2 (셀 편집), S5/S6 (mouse_interactor) 검증.

Phase 3.4.2 세부계획서: [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md).

---

## 11. 관련 문서

- 분할 컨텍스트: [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) §3.1 (Option A) + §5.1 (3.4.1 검증 매트릭스 14 항목)
- 모체 계획서: [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md) (전체 그림 + EventBus 5 종 + 시나리오 S1~S6)
- 후속 sub-phase: [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md), [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md)
- 선행 평가서: [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + **§6.0 공통 지침 (UI 1:1 보존)** + §11 (vtk_renderer 분할 리스크) + §13 (UI 이식 공통 지침)
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
- UI 이식 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
