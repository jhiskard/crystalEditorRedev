# Phase 3.4.3 — Edit / Bonds 이식 (Phase 3.4 의 3/3 — 분할 마무리) 세부계획서

> 분할 컨텍스트: **Phase 3.4 의 3/3** ([`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) Option A 채택 — Cell → Atoms → **Bonds** 의존성 순서)
> 모체 계획서:    [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)
> 선행 sub-phase: [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md)
> 상위 문서:      [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + §6.0 공통 지침
> 메뉴 매핑:      [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
> UI 이식 지침:   [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01~UI-12)
> 작성일:        2026-05-07
> 대상 브랜치:    `refactor/menu-aligned`
> 단위 PR:       1 개
> 예상 소요:      2~2.5 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-07 | 초안 작성 (Phase 3.4 모체 계획서를 분할안 Option A 에 따라 세분화) |

---

## 0. 한 줄 요약

> Phase 3.4 의 *세 sub-phase 중 마지막* 단계로, Edit 메뉴의 *Bonds 항목* 을 이식 — Phase 3.4 의 분할 마무리. `features/edit/bonds/` 신설 + `edit_menu` Bonds 항목 추가. legacy `bond_manager.{cpp,h}` (1,065 줄) + `bond_ui.{cpp,h}` (261 줄) + `vtk_renderer.cpp` 의 *bond 부분* (~700 줄) + 별도 `bond_renderer.{cpp,h}` (76 줄) 을 이식. **`core::scene::EventBus::onBondsChanged` 의 첫 emit + 첫 구독자 — *단일 sub-phase 안 양방 검증*** + **`core::data::ElementDatabase` 두 번째 사용자** (covalent radii 조회) 도착으로 Phase 2 인프라의 *최종 검증*. **vtk_renderer.cpp 1,792 줄 분할 완료 (3/3)** — Phase 3 의 가장 큰 기술 리스크 (상위 §11) 가 본 단계 머지로 해소.
> 본 sub-phase 라인수 약 **1,400 줄, 8 파일** — Phase 3.1 (1,426 ~ 2,131) 와 비슷한 규모. *onBondsChanged 양방 검증* + *element_database 두 번째 검증* 으로 Phase 2 인프라의 *동일 단계 안 다중 검증* 첫 사례.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `features/edit/bonds/` sub-folder 신설 + 8 파일 작성. (b) `edit_menu` 갱신 — *Bonds 항목 추가* (Cell + Atoms + Bonds 3 항목 모두 활성). (c) 메뉴 `Edit / Bonds` 클릭 시 Bonds Management 윈도우 표시. (d) **legacy `vtk_renderer.cpp` 의 bond 부분 (~700 줄) + `bond_renderer.cpp` (42 줄) 을 `bond_renderer.{cpp,h}` 로 통합** — *vtk_renderer 분할 완료 (3/3)*. (e) `core::scene::EventBus::onBondsChanged` 의 *첫 emit (bond_manager 의 RecomputeAll 끝)* + *첫 구독자 (bond_renderer)* — 단일 sub-phase 안 양방. (f) `core::data::ElementDatabase` *두 번째 사용자* — bond_manager 의 자동 본드 검출이 *covalent radii 합 + 마진* 알고리즘에서 호출. (g) `npm run build-wasm:debug` + `:release` 통과 + 런타임에서 Edit 메뉴 3 항목 모두 동작 + 시나리오 S4 (Recompute → cylinder 표시) + Phase 3.4.1/3.4.2 회귀 부재. (h) **legacy 의 Bonds Management UI 동작이 1:1 보존됨** (§1.4) |
| **비목표** | Cell / Atoms 항목 — 본 sub-phase 는 *Bonds 항목만 추가*. Phase 3.4.1 / 3.4.2 가 이미 wiring 완료. *측정 모드 (Measurement)* 의 mouse_interactor 사용 (Phase 3.5 의 영역). 18 항목 회귀 전체 통과. legacy/ 내부 코드 수정 (회색지대 §1.2~§1.3 예외, vtk_renderer 분할은 *복사 + 새 트리 정리* 로만). **legacy UI 의 *재설계* — Bonds Management 윈도우의 결합 종 알파벳 순서 / 거리 슬라이더 응답 패턴 변경 금지** |

> Phase 3.4.3 의 미덕: *"검토자가 git diff 를 보고 `features/edit/bonds/` 신규 8 파일 + `features/edit/edit_menu` 의 Bonds 항목 1 줄 + `app/app.cpp` 변동 0 (3.4.2 가 이미 시그니처 확장 완료) + `CMakeLists.txt` source 추가 외에 의심할 게 없고, side-by-side 스크린샷에서 legacy 와 새 트리의 Bonds Management 윈도우가 시각적으로 동일하며, **Recompute 클릭 후 H-O / C-N 등 자동 검출된 본드가 cylinder 로 표시** 된다"*.

### 1.1 Phase 3.4.3 의 *Phase 3.4 마무리* 의의

본 sub-phase 는 [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) §3 의 *의존성 자연 순서* 의 마지막 단계 — Bonds 가 atom 직접 의존이라 3.4.2 후 진입.

| Phase 2 인프라 | Phase 3.4.3 의 첫 사용 |
|---|---|
| `core/scene/EventBus::onBondsChanged` | bond_manager 의 RecomputeAll 끝에서 *첫 emit*, bond_renderer 가 *첫 구독자* — *단일 sub-phase 안 양방 검증* (Phase 3.3 의 emit-only 패턴, Phase 3.4.1/3.4.2 의 subscribe-only 패턴과 다른 *동일 단계 양방* 패턴) |
| `core/data/element_database` | bond_manager 의 자동 본드 검출이 *원소별 covalent radius* 조회 — Phase 3.3 의 PeriodicTable + Bravais 의 첫 사용에 이은 *두 번째 사용자*. Phase 3.3 평가서 §1.6.1 의 lowerCamelCase 시그니처 (`getInstance() / getElementInfo()`) 그대로 적용 |
| `core/scene/SceneState::structureRecords::bonds` | bond_manager 가 본 데이터를 *직접 변경* — *최초의 변경자* |
| vtk_renderer 분할 (3/3) | bond 부분 (~700 줄) 을 `bond_renderer.{cpp,h}` 로 흡수 + legacy `bond_renderer.cpp` (42 줄) thin helper 도 동일 모듈로 통합 — **vtk_renderer 1,792 줄 분할 100% 완료** |

→ Phase 3.4.1 (onCellChanged 구독) → 3.4.2 (onAtomsChanged + onStructureAdded + mouse_interactor 구독) → **3.4.3 (onBondsChanged emit + 구독 양방 + element_database 두 번째 + vtk_renderer 분할 마무리) — Phase 2 인프라 검증의 최종 단계**.

### 1.2 회색지대 정책 인계

[`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md) §3.1 의 회색지대 정책을 그대로 상속. *vtk_renderer 분할 회색지대* 의 마지막 적용 — bond 부분 ~700 줄 + bond_renderer.cpp 42 줄.

### 1.3 라인수 압축 정책

본 sub-phase 의 압축률 예상은 ~67% (legacy 2,102 → 새 트리 ~1,400). bond_manager 900 줄의 *자동 본드 검출 알고리즘 보존* 으로 도메인 측 압축은 보수적, *bond_renderer thin helper 흡수* 로 약간의 통합 효과.

**보존 필수**:

- `bond_manager.cpp` (900 줄) 의 *거리·각도 기반 자동 본드 검출 알고리즘* + 결합 종 (`H-O`, `C-N` 등) 결정 로직
- `bond_manager.cpp` 의 *covalent radii 합 + 마진* 기반 거리 임계값 default (예: `r_covalent(A) + r_covalent(B) + 0.4Å`)
- `bond_ui.cpp` (220 줄) 의 *결합 종 알파벳 순서* + *Recompute 버튼 응답* + *thickness/opacity 글로벌 슬라이더*
- legacy vtk_renderer 의 bond 부분 *그룹별 액터 캐싱* (`BondGroupVTKData`) + `vtkCylinderSource` / `vtkGlyph3D` / `vtkAppendPolyData` 호출 sequence
- §1.4 의 모든 UI 보존 항목

### 1.4 legacy UI 작동방식 1:1 보존 (상위 §6.0.1 의 본 sub-phase 적용)

#### 1.4.1 Bonds Management 윈도우 보존 대상

`legacy/atoms/ui/bond_ui.cpp` (220 줄) 가 정의하는 다음 UI 동작은 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| Bond 종 목록 | 자동 검출된 결합 종 (`H-O`, `C-N`, `C-C` 등) 의 *알파벳 순서* — legacy 의 표시 형식 |
| 거리 임계값 슬라이더 | Bond 종별 거리 슬라이더 — *기본값 (covalent radii 합 + 0.4Å 마진 등)* + 응답 패턴 (즉시 갱신 vs Apply) |
| Visibility 체크박스 | Bond 종별 체크박스 — legacy 의 default 상태 (모두 켜짐?) 보존 |
| Recompute 버튼 | 클릭 시 *전체 본드 재검출* sequence — legacy 의 시간 복잡도 보존 (atom × atom 거리 계산 + 임계값 비교) |
| Thickness 글로벌 슬라이더 | 모든 bond group 에 일괄 적용 — 슬라이더 범위 / 기본값 / 응답 시점 |
| Opacity 글로벌 슬라이더 | 동상 |
| 정보 패널 | 결합 종별 *카운트* (예: "H-O: 12 bonds") 표시 형식 |

#### 1.4.2 보존 검증 절차 (상위 §6.0.2)

PR 작성자는 본 PR 본문에 다음을 첨부:

1. **Side-by-side 스크린샷** — legacy Bonds Management vs 새 트리.
2. **사용자 시나리오** — 다음을 legacy 와 새 트리에서 차례로 실행:
   - **(S4)** Periodic Table → 여러 원자 추가 (예: H 4 + O 2) → Bonds Management → "Recompute" → 자동 검출된 H-O 본드의 *cylinder 표시* — legacy 와 동일한 본드 갯수 + 위치 + 색상
3. **Phase 3.4.1/3.4.2 회귀 부재** — Edit / Cell + Edit / Atoms 메뉴와 시나리오 S1~S6 가 본 PR 후에도 동일 결과
4. **Intentional UI deviation 사유서** — 없으면 *"None"* 명기.

#### 1.4.3 압축과 UI 보존의 조화

| 압축 가능 ✓ | 보존 필수 ✗ |
|---|---|
| 한국어 깨진 인코딩 주석 → Doxygen | 결합 종 *알파벳 순서* (legacy 가 std::sort 또는 std::map 으로 자동 정렬한 결과) |
| `m_parent->Y()` → `controller_.Y()` | "Recompute" 버튼 클릭 시 *재검출 알고리즘 시간 복잡도* (atom × atom 또는 spatial hash) |
| 미사용 `m_renderer` 멤버 제거 | covalent radii 합 + 마진 *기본값* (legacy 의 0.4Å 등) |
| `printf` 디버그 제거 | thickness/opacity 슬라이더 *변경 시 즉시 갱신 vs 다음 프레임 갱신* 정책 |

---

## 2. 전제 — Phase 3.4.2 완료 상태

본 계획서는 다음이 충족된 상태에서 시작한다.

- [ ] Phase 3.4.2 commit 머지 (`features/edit/atoms/` 10 파일 + edit_menu Atoms 항목)
- [ ] `core/{scene, io, data, vtk, render, ui}` 인프라 빌드 가능
- [ ] `npm run build-wasm:debug` + `:release` exit 0
- [ ] Edit 메뉴 2 항목 (Cell + Atoms) 동작 (Phase 3.4.1/3.4.2 §5 통과)
- [ ] **`core::scene::EventBus::onAtomsChanged` 첫 구독자 도착 (3.4.2)** — 본 sub-phase 의 bond_manager 가 *atom 변경 시 자동 재검출* 로 두 번째 구독자 가능
- [ ] **`core::vtk::MouseInteractor` 첫 구독자 도착 (3.4.2)** — 본 단계는 mouse_interactor 미사용
- [ ] **Phase 3.3 의 onStructureAdded emit 보강 완료 (3.4.2 안)** — 후속 단계 인프라 정합

---

## 3. legacy 참조 인벤토리

| legacy 파일 | 라인 수 | 역할 | 새 트리 행선지 |
|---|---|---|---|
| `legacy/atoms/domain/bond_manager.{cpp,h}` | 900 + 165 | 거리·각도 기반 자동 본드 검출 + 결합 종 결정 | `features/edit/bonds/bond_manager.{cpp,h}` |
| `legacy/atoms/ui/bond_ui.{cpp,h}` | 220 + 41 | Bonds Management 윈도우 ImGui | `features/edit/bonds/bond_ui.{cpp,h}` |
| `legacy/atoms/infrastructure/vtk_renderer.cpp` | (1,792 중 bond 부분 ~700) | `initializeBondGroup` / `updateBondGroup` / `clearBondGroup` / `updateAllBondGroupThickness/Opacity` 등 | `features/edit/bonds/bond_renderer.{cpp,h}` *(분할)* |
| `legacy/atoms/infrastructure/bond_renderer.{cpp,h}` | 42 + 34 | bond 액터 thin helper | `features/edit/bonds/bond_renderer.{cpp,h}` 안에 흡수 |
| **(신규)** | — | `bonds_controller`, edit_menu 갱신 | 신규 작성 / 갱신 |

**총 legacy 참조: 약 2,102 줄.** 압축 후 약 **1,400 줄** 예상 (Phase 3.1 1,426 와 비슷). bond_manager 900 줄의 알고리즘 보존이 압축 여지를 제한.

### 3.1 외부 의존 사전 분석

| legacy 파일 | 외부 의존 | 새 트리 처리 |
|---|---|---|
| `bond_manager.{cpp,h}` | atom_manager (atoms 좌표 / 종 조회), `core::data::ElementDatabase` (covalent radii) | **Phase 3.4.2 의 `features::edit::atoms::AtomManager` 직접 호출** + **`core::data::ElementDatabase::getInstance().getElementInfo(symbol)` 두 번째 사용자**. namespace 정리 |
| `bond_ui.{cpp,h}` | bond_manager, `class AtomsTemplate` (forward) | controller 로 분기 + AtomsTemplate forward 제거 |
| vtk_renderer.cpp 의 bond 부분 | vtk*, atom_manager (bond 양 끝점의 atom 좌표) | **분할** + atom 좌표는 *읽기 전용* 으로 SceneState::structureRecords::atoms 직접 |
| `bond_renderer.{cpp,h}` (legacy thin helper) | vtk_renderer | 본 단계의 bond_renderer 안에 통합 흡수 |

### 3.2 메뉴 트리 매핑 (04 §4 Edit 참조)

| 메뉴 항목 | 새 진입점 | 윈도우 |
|---|---|---|
| `Edit / Bonds` | `features::edit::bonds::Show()` | Bonds Management |

→ 본 sub-phase 머지 후 Edit 메뉴 3 항목 (Cell + Atoms + Bonds) 모두 활성. Phase 3.4 의 메뉴 wiring 완료.

### 3.3 vtk_renderer 분할 분석 (bond 부분 — 3/3 마무리)

legacy `vtk_renderer.h` / `vtk_renderer.cpp` 의 bond 관련 메서드:

| 메서드 | 역할 | 추정 라인 |
|---|---|---|
| `initializeBondGroup(const std::string& bondTypeKey, float radius)` | bond 종별 액터 그룹 초기화 | ~80 |
| `updateBondGroup(...)` (multiple) | bond 종별 액터 갱신 | ~250 |
| `clearBondGroup(const std::string& bondTypeKey)` | bond 종별 청산 | ~40 |
| `clearAllBondGroups()` | 모든 bond 청산 | ~30 |
| `updateAllBondGroupThickness(float thickness)` | thickness 일괄 적용 | ~50 |
| `updateAllBondGroupOpacity(float opacity)` | opacity 일괄 적용 | ~50 |
| `setAllBondGroupsVisible(bool visible)` | 일괄 visibility | ~30 |
| `syncBondLabelActors(...)` / `clearAllBondLabelActors()` | bond label sync | ~80 |
| 내부 BondGroupVTKData 캐시 + 헬퍼 | | ~90 |
| **합계** | | **~700** |

추가로 legacy `bond_renderer.cpp` (42 줄) thin helper 가 본 모듈에 통합. legacy 측 두 파일 모두 *동결본 유지* — 새 트리만 *복사 + 새 트리 정리*.

---

## 4. 작업 절차 (단계별)

### Step 1 — `features/edit/bonds/` 폴더 신설

```bash
cd webassembly/src
mkdir -p features/edit/bonds
```

### Step 2 — `bonds/bond_manager.{cpp,h}` 도메인 이식 (가장 무거움)

legacy 1,065 줄 (900 + 165) → 약 750 줄 예상. element_database 두 번째 사용자.

```cpp
/**
 * @file features/edit/bonds/bond_manager.h
 * @brief 자동 본드 검출 + 결합 종 결정 + thickness/opacity 글로벌 정책.
 */
#pragma once
#include "../../../core/scene/scene_state.h"
#include "../../../core/data/element_database.h"
#include <string>

namespace features::edit::bonds {

class BondManager {
public:
    explicit BondManager(core::scene::SceneState& scene);

    /// @brief 자동 본드 검출 — covalent radii 합 + 마진 기반.
    void RecomputeAll(int32_t structureId);

    /// @brief 결합 종 별 visibility / 거리 임계값.
    void SetBondTypeVisible(const std::string& bondTypeKey, bool visible);
    void SetBondDistanceThreshold(const std::string& bondTypeKey, float distance);
    float GetBondDistanceThreshold(const std::string& bondTypeKey) const;

    /// @brief 글로벌 thickness / opacity (legacy 의 동일 메서드 보존).
    void SetGlobalThickness(float thickness);
    void SetGlobalOpacity(float opacity);
    float GetGlobalThickness() const  { return globalThickness_; }
    float GetGlobalOpacity()   const  { return globalOpacity_; }

    /// @brief 결합 종 목록 (알파벳 순서 정렬).
    std::vector<std::string> ListBondTypes(int32_t structureId) const;

private:
    /// @brief 두 원소 간 default 거리 임계값 (covalent radii 합 + 마진).
    float ComputeDefaultThreshold(const std::string& symbolA,
                                  const std::string& symbolB) const;

    core::scene::SceneState& scene_;
    float globalThickness_ = 0.1f;
    float globalOpacity_   = 1.0f;
    // bondType -> threshold 캐시
};

} // namespace features::edit::bonds
```

> **§1.1 element_database 두 번째 사용자**: `ComputeDefaultThreshold` 안에서:
> ```cpp
> auto db = core::data::ElementDatabase::getInstance();
> const auto* infoA = db.getElementInfo(symbolA);
> const auto* infoB = db.getElementInfo(symbolB);
> return infoA->covalentRadius + infoB->covalentRadius + 0.4f;
> ```
> Phase 3.3 평가서 §1.6.1 의 lowerCamelCase 시그니처 그대로 적용.
>
> **§1.1 EventBus 첫 emit (단일 단계 안 양방)**: `RecomputeAll` 끝에서 `scene_.events.onBondsChanged.Emit(core::scene::BondsChangedEvent{structureId})`.

### Step 3 — `bonds/bond_renderer.{cpp,h}` (vtk_renderer 분할 결과 + thin helper 흡수)

legacy vtk_renderer.cpp 의 bond 부분 (~700 줄) + legacy bond_renderer.cpp (42 줄) 통합 흡수.

```cpp
/**
 * @file features/edit/bonds/bond_renderer.h
 * @brief Bond 액터 그룹 + label sync. Phase 3.4.3 의 vtk_renderer 분할 결과 (3/3).
 */
#pragma once
#include "../../../core/vtk/vtk_viewer.h"
#include "../../../core/scene/scene_state.h"
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <unordered_map>
#include <string>

namespace features::edit::bonds {

class BondRenderer {
public:
    explicit BondRenderer(core::scene::SceneState& scene);
    ~BondRenderer();

    /// @brief EventBus::onBondsChanged 구독 — 변경 시 재렌더 (단일 단계 안 양방).
    void Subscribe();

    void InitializeBondGroup(const std::string& bondTypeKey, float radius);
    void UpdateBondGroup(int32_t structureId, const std::string& bondTypeKey);
    void ClearBondGroup(const std::string& bondTypeKey);
    void ClearAllBondGroups();
    void SetBondGroupVisible(const std::string& bondTypeKey, bool visible);
    void UpdateAllBondGroupThickness(float thickness);
    void UpdateAllBondGroupOpacity(float opacity);

private:
    core::scene::SceneState& scene_;
    // legacy 의 BondGroupVTKData 캐시 보존
    std::unordered_map<std::string, vtkSmartPointer<vtkActor>> bondActors_;
};

} // namespace features::edit::bonds
```

> **§1.1 단일 단계 안 양방 패턴**: bond_manager.RecomputeAll() 끝에서 emit → bond_renderer.Subscribe() 안에서 구독. 무한 루프 방지 — bond_renderer 는 emit 하지 않음 (그리기만).
>
> **§1.2.1 분할 마무리 검증**: legacy vtk_renderer 의 `initializeBondGroup` / `updateBondGroup` 의 vtk* 호출 sequence (`vtkCylinderSource::SetRadius` → `vtkGlyph3D::SetSourceConnection` → `vtkAppendPolyData::AddInputData` → `mapper->Update()`) 가 새 트리에서도 *완전 동일* — §5 #11 grep diff.

### Step 4 — `bonds/bonds_controller.{cpp,h}` (얇은 통합 진입점)

Phase 3.4.1 의 *얇은 controller* 패턴 적용.

```cpp
/**
 * @file features/edit/bonds/bonds_controller.h
 * @brief Bonds Management 의 통합 진입점.
 */
#pragma once
#include "bond_manager.h"
#include "bond_renderer.h"
#include "../../../core/scene/scene_state.h"

namespace features::edit::bonds {

class BondsController {
public:
    explicit BondsController(core::scene::SceneState& scene);

    BondManager&  Manager()  { return bondManager_; }
    BondRenderer& Renderer() { return bondRenderer_; }

    /// @brief Phase 3.4.2 의 onAtomsChanged 두 번째 구독자 (선택) — atom 변경 시 자동 재검출.
    void SubscribeAtomChanges();

private:
    core::scene::SceneState& scene_;
    BondManager              bondManager_;
    BondRenderer             bondRenderer_;
};

} // namespace features::edit::bonds
```

> *(선택)* `SubscribeAtomChanges` — Phase 3.4.2 의 onAtomsChanged 의 *두 번째 구독자* 가 본 단계에 도착할 수 있음. atom 추가/삭제 시 자동 본드 재검출. UI 의 명시적 Recompute 와 별개로 *백그라운드 갱신 옵션*. 본 단계는 *옵션* 이며, 사용자가 *과도한 재계산* 을 우려하면 미구현 (Phase 6 마무리에서 옵션화).

### Step 5 — `bonds/bond_ui.{cpp,h}` (§1.4.1 보존)

legacy 261 줄 (220+41) → 약 180 줄 예상.

```cpp
/**
 * @file features/edit/bonds/bond_ui.h
 * @brief Bonds Management 윈도우 ImGui — §1.4.1 보존.
 */
#pragma once
#include <imgui.h>

namespace features::edit::bonds {

class BondsController;

class BondUI {
public:
    explicit BondUI(BondsController& controller);
    void Render(bool* open);

private:
    BondsController& controller_;
    // ★ §1.4.1 보존: 결합 종 알파벳 순서 + Recompute 응답 + thickness/opacity 응답 그대로
    float globalThickness_ = 0.1f;
    float globalOpacity_   = 1.0f;
};

} // namespace features::edit::bonds
```

### Step 6 — `edit_menu.{cpp,h}` 갱신 (Bonds 항목 추가)

Phase 3.4.1 의 skeleton + 3.4.2 의 Atoms 갱신을 잇는 마무리. *3 항목 모두 활성*.

```cpp
// features/edit/edit_menu.cpp (3.4.3 의 Bonds 추가)
#include "edit_menu.h"
#include "cell/cell_controller.h"
#include "cell/cell_info_ui.h"
#include "atoms/atoms_controller.h"
#include "atoms/atom_editor_ui.h"
#include "bonds/bonds_controller.h"      // ← 신규
#include "bonds/bond_ui.h"               // ← 신규

namespace features::edit {

namespace {
    bool g_showCellInfo        = false;
    bool g_showCreatedAtoms    = false;
    bool g_showBondsManagement = false;   // ← 신규
    cell::CellController*   g_cellCtrl  = nullptr;
    cell::CellInfoUI*       g_cellUI    = nullptr;
    atoms::AtomsController* g_atomsCtrl = nullptr;
    atoms::AtomEditorUI*    g_atomsUI   = nullptr;
    bonds::BondsController* g_bondsCtrl = nullptr;   // ← 신규
    bonds::BondUI*          g_bondsUI   = nullptr;   // ← 신규
}

void DrawMenu() {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Atoms")) g_showCreatedAtoms    = true;
        if (ImGui::MenuItem("Bonds")) g_showBondsManagement = true;   // ← 신규
        if (ImGui::MenuItem("Cell"))  g_showCellInfo        = true;
        ImGui::EndMenu();
    }
}

void RenderWindows() {
    if (g_showCellInfo        && g_cellUI)  g_cellUI->Render(&g_showCellInfo);
    if (g_showCreatedAtoms    && g_atomsUI) g_atomsUI->Render(&g_showCreatedAtoms);
    if (g_showBondsManagement && g_bondsUI) g_bondsUI->Render(&g_showBondsManagement);   // ← 신규
}

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mi) {
    static cell::CellController   cellCtrl(scene);
    static cell::CellInfoUI       cellUI(cellCtrl);
    static atoms::AtomsController atomsCtrl(scene);
    static atoms::AtomEditorUI    atomsUI(atomsCtrl);
    static bonds::BondsController bondsCtrl(scene);                    // ← 신규
    static bonds::BondUI          bondsUI(bondsCtrl);                  // ← 신규

    cellCtrl.Renderer().Subscribe();
    atomsCtrl.Subscribe(mi);
    bondsCtrl.Renderer().Subscribe();                                  // ← 신규 (onBondsChanged 첫 구독자)
    // (선택) bondsCtrl.SubscribeAtomChanges();   // onAtomsChanged 두 번째 구독자 옵션

    g_cellCtrl  = &cellCtrl;  g_cellUI  = &cellUI;
    g_atomsCtrl = &atomsCtrl; g_atomsUI = &atomsUI;
    g_bondsCtrl = &bondsCtrl; g_bondsUI = &bondsUI;                    // ← 신규
}

}
```

> **메뉴 항목 순서** — legacy 의 *Atoms / Bonds / Cell* 순서를 그대로 보존 (§1.4 정신).

### Step 7 — `app/app.cpp` 변동 *없음*

Phase 3.4.2 가 InitOnce 시그니처에 `MouseInteractor&` 를 이미 추가했으므로 본 sub-phase 는 app.cpp 측 *0 변동*. 이는 분할의 자연 결과 — 후속 sub-phase 가 *app.cpp 측 무변동* 이라는 점이 검토 부담 감소에 기여.

### Step 8 — `CMakeLists.txt` SOURCES_FEATURES 확장

```cmake
set(SOURCES_FEATURES
    # Phase 3.1~3.3 + 3.4.1 + 3.4.2
    ...
    # Phase 3.4.3 신규 (8 파일)
    webassembly/src/features/edit/bonds/bond_manager.cpp
    webassembly/src/features/edit/bonds/bond_manager.h
    webassembly/src/features/edit/bonds/bond_renderer.cpp
    webassembly/src/features/edit/bonds/bond_renderer.h
    webassembly/src/features/edit/bonds/bonds_controller.cpp
    webassembly/src/features/edit/bonds/bonds_controller.h
    webassembly/src/features/edit/bonds/bond_ui.cpp
    webassembly/src/features/edit/bonds/bond_ui.h
)
```

### Step 9 — 정적 검증

```bash
cd webassembly/src

# legacy 호출 0
grep -rnE "AtomsTemplate::|atoms::domain::|atoms::infrastructure::|atoms::ui::" \
  features/edit/bonds/ | grep -v "//" || echo "OK"

# legacy include 0
grep -rnE '#include\s+"(\.\./)*legacy/' features/edit/bonds/ | grep -v "//" || echo "OK"

# legacy vtk_renderer 의존 0
grep -rnE "atoms::infrastructure::VTKRenderer|atoms::infrastructure::vtk_renderer" \
  features/edit/bonds/ | grep -v "//" || echo "OK"

# **onBondsChanged 단일 단계 안 양방 검증**
grep -rnE "onBondsChanged\.Emit" features/edit/bonds/ | head -5    # bond_manager 안에 1+
grep -rnE "onBondsChanged\.Subscribe" features/edit/bonds/ | head -5  # bond_renderer 안에 1+

# **element_database 두 번째 사용자 — Phase 3.3 와 동일 시그니처**
grep -rnE "core::data::ElementDatabase::getInstance|getElementInfo" \
  features/edit/bonds/ | head -10

# **vtk_renderer bond 부분 분할 — vtk* 호출 보존 (3/3 마무리)**
grep -nE "vtkCylinderSource|vtkGlyph3D|vtkAppendPolyData|vtkActor" \
  features/edit/bonds/bond_renderer.cpp | wc -l   # legacy 의 bond 부분 vtk* 갯수 ±10%

# bond_manager 의 자동 검출 알고리즘 보존 (covalent radii 합 + 마진)
grep -nE "covalentRadius|getElementInfo|0\.4f|threshold" \
  features/edit/bonds/bond_manager.cpp | head -5

# **§1.4.1 UI 보존 — bond_ui ImGui 위젯 grep diff**
grep -nE 'ImGui::(SliderFloat|Checkbox|Button|Combo|InputFloat)' \
  legacy/atoms/ui/bond_ui.cpp \
  features/edit/bonds/bond_ui.cpp 2>/dev/null

# **vtk_renderer 분할 100% 완료 확인 — atom + bond + cell 모두 새 트리에 도착**
grep -rnE "vtkSphereSource|vtkCylinderSource|vtkOutlineSource|vtkAxesActor" \
  features/edit/ | wc -l   # 3 sub-phase 합계가 legacy vtk_renderer 의 vtk* 호출 갯수와 일치
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

- [ ] Side-by-side 스크린샷 (legacy Bonds Management vs 새 트리)
- [ ] 시나리오 S4 — 원자 다수 추가 → Recompute → cylinder 표시 (legacy 와 본드 갯수/위치 동일)
- [ ] **Phase 3.4.1 시나리오 S3-cell 회귀 부재**
- [ ] **Phase 3.4.2 시나리오 S1/S2/S5/S6 회귀 부재**
- [ ] Intentional UI deviation 사유서

### Step 11 — 커밋 & PR

```powershell
git commit -m "Phase 3.4.3 (3/3): features/edit/bonds — Phase 3.4 split completion

This PR is the third and final of three in Phase 3.4 (split per phase3_4_split_proposal.md).
After merge, Phase 3.4 (Edit / Atoms + Bonds + Cell) is complete and Phase 3.5 (measurement) becomes the next entry.

- New folder: features/edit/bonds/ (8 files).
- edit_menu update: Bonds item added — Edit menu now has 3 active items (Atoms / Bonds / Cell).
- core/scene::EventBus::onBondsChanged first emit (bond_manager) + first subscriber (bond_renderer)
  — single sub-phase bidirectional validation.
- core/data::ElementDatabase second user — bond_manager covalent radii lookup
  (Phase 3.3 evaluation §1.6.1 lowerCamelCase signature applied).
- vtk_renderer.cpp 1,792-line split (3/3) — bond portion (~700 lines) + legacy bond_renderer.cpp (42 lines)
  consolidated into bond_renderer. **vtk_renderer split 100% complete.**
- legacy Bonds Management UI preserved 1:1 per §6.0.1.

Side-by-side screenshot (Bonds Management) and S4 scenario test attached.
Phase 3.4.1/3.4.2 regression: None.
Intentional UI deviation: None.

Reference:
  - webassembly/docs/phases/phase3_4_edit_atoms_bonds_cell.md (parent index)
  - webassembly/docs/phases/phase3_4_split_proposal.md (split rationale)
  - webassembly/docs/phases/phase3_4_3_edit_bonds.md (this plan)
"
```

---

## 5. 검증 매트릭스 (14 항목)

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `features/edit/bonds/` 폴더 존재 | `ls features/edit/` | cell + atoms + bonds 3 폴더 | 정적 |
| 2 | bonds 파일 수 | `ls features/edit/bonds/` | 8 (.cpp 4 + .h 4) | 정적 |
| 3 | edit_menu Bonds 항목 추가 | grep `MenuItem.*Bonds` | 1 hit | 정적 |
| 4 | namespace 일관성 (`features::edit::bonds::*`) | grep | 모든 .cpp/.h | 정적 |
| 5 | legacy 호출 0 | grep | 0 hit | 정적 |
| 6 | `#include "../legacy/"` 0 | grep | 0 hit | 정적 |
| 7 | legacy `vtk_renderer` 의존 0 | grep | 0 hit | 정적 |
| 8 | **`onBondsChanged.Emit` 1+ 곳** (bond_manager) | grep | 1+ hit | 정적 — 핵심 |
| 9 | **`onBondsChanged.Subscribe` 1+ 곳** (bond_renderer) | grep | 1+ hit | 정적 — 핵심 (단일 단계 양방) |
| 10 | **`core::data::ElementDatabase::getInstance` 호출 1+ 곳** (bond_manager) | grep | 1+ hit | 정적 — Phase 2 두 번째 사용자 |
| 11 | **vtk_renderer bond 부분 분할 — vtk* 호출 갯수** | grep `vtkCylinderSource\|vtkGlyph3D\|vtkAppendPolyData` | legacy 의 bond 부분 ±10% | 정적 — 핵심 |
| 12 | **§1.4.1 UI 보존 — ImGui 위젯 인자 비교** | legacy 와 새 트리 grep diff | 일치 | 정적 |
| 13 | Debug 빌드 + Release 빌드 | `npm run build-wasm:debug` + `:release` | exit 0 | 동적 |
| 14 | 메뉴 3 항목 + 시나리오 S4 + Phase 3.4.1/3.4.2 회귀 부재 + Side-by-side + 콘솔 0 | 런타임 | 모두 통과 | 동적 — 핵심 묶음 |

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | bond_manager 의 900 줄 자동 검출 알고리즘 압축 중 *covalent radii 합 + 마진 default* 변경 | bond 갯수 / 위치 차이 — §1.4.1 violation | 압축 전 legacy 의 *기본 마진 (예: 0.4Å)* + *bond 임계값 알고리즘* 을 단위 함수로 분리하여 기능 동등 보존 + §5 #11 의 시나리오 S4 검증 |
| 6.2 | element_database 두 번째 사용자 — `covalentRadius` 필드 부재 또는 시그니처 차이 | link error 또는 silent failure | Phase 2 의 `core::data::ElementInfo` 구조체 사전 확인 — `covalentRadius` (또는 동등 필드) 존재 보장. 부재 시 *Phase 2 보강* (회색지대 §1.2 또는 element_database 확장 PR) |
| 6.3 | onBondsChanged emit + subscribe *단일 단계 양방* 시 무한 루프 — bond_renderer 가 emit 또 호출 가능 | 무한 루프 | bond_renderer 는 *그리기만*, emit 절대 호출 안 함. bond_manager 만 emit + bond_renderer 만 subscribe 의 일방향 흐름 강제 |
| 6.4 | vtk_renderer bond 부분 ~700 줄 분할 시 *그룹별 액터 캐싱 정책* (BondGroupVTKData) 누락 | bond 표시 안 됨 또는 메모리 누수 | (a) §5 #11 의 vtk* 호출 갯수 비교. (b) legacy 의 BondGroupVTKData::isInitialized 같은 캐시 키 grep 으로 1:1 매핑 |
| 6.5 | legacy `bond_renderer.cpp` (42 줄) thin helper 흡수 시 인터페이스 누락 | bond label sync 또는 thickness 일괄 적용 안 됨 | 별도 grep — legacy bond_renderer 의 모든 메서드가 새 트리 bond_renderer 의 인터페이스에 1:1 매핑됨 검증 |
| 6.6 | atom_manager 의존 — Phase 3.4.2 의 `features::edit::atoms::AtomManager` 가 *친구 클래스* 또는 *공개 API* 가 충분히 노출되지 않음 | bond_manager 가 atom 좌표 / 종 조회 불가 | 3.4.2 의 atom_manager 인터페이스에 `GetAtomsConst` 또는 `IterateAtoms` 같은 *읽기 전용* 메서드가 있는지 사전 확인. 부재 시 3.4.2 PR 보강 |
| 6.7 | 결합 종 *알파벳 순서* 가 std::map vs std::unordered_map 차이로 깨짐 | UI 표시 순서 차이 — §1.4.1 violation | bond_manager 의 ListBondTypes 가 std::sort 명시 호출 또는 std::map 사용 |
| 6.8 | thickness/opacity 글로벌 슬라이더의 *변경 시 즉시 갱신 vs 다음 프레임 갱신* 정책 | UI 응답 차이 — §1.4.1 violation | legacy 의 응답 시점을 ImGui 흐름 grep 으로 확인 (예: 슬라이더 값 변경 직후 SetGlobalThickness 호출 vs ApplyButton 클릭 후) |
| 6.9 | Phase 3.4.2 의 onAtomsChanged *두 번째 구독자* 옵션 (bondsCtrl.SubscribeAtomChanges) 도입 시 atom 변경마다 본드 재계산 — 성능 저하 | 대형 구조 (1,000+ atoms) 시 UI 정지 | 본 sub-phase 에서는 *옵션 미구현* 또는 *throttle/debounce* 적용. PR 본문에 *"옵션 미구현"* 명시 |
| 6.10 | bond_manager 라인수가 계획 대비 작아질 수 있음 — legacy 부채 (디버그 print, 미사용 메서드) 정리로 ~750 → ~600 줄 가능 | 라인수 예상 폭 | §9.1 의 라인수 예상은 *상한* 임을 명시 |

---

## 7. 롤백 절차

```bash
git restore --staged .
git restore .
git clean -fd webassembly/src/features/edit/bonds
```

원인 진단 우선순위:

1. **bond_manager 자동 검출 알고리즘 누락** — §6.1 의 시나리오 S4 검증 실패 → legacy 의 RecomputeAll 흐름 1:1 비교.
2. **element_database 두 번째 사용자 미해결** — §6.2 의 `covalentRadius` 필드 부재 시 Phase 2 보강.
3. **onBondsChanged 무한 루프** — §6.3 의 emit/subscribe 분리 검토.
4. **vtk_renderer bond 분할 누락** — §6.4 의 grep 검증 실패 → legacy 측 행 분포 재추정.
5. **§1.4.1 UI 보존 위반** — §6.7 / §6.8 의 즉시 복원 또는 사유서.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] §2 의 Phase 3.4.2 전제 충족
- [ ] §5 검증 매트릭스 14 항목 통과
- [ ] features/edit/bonds/ 안에서 legacy 호출 0 + legacy include 0 + vtk_renderer 의존 0
- [ ] **`onBondsChanged.Emit` 1+ 곳** (bond_manager) + **`onBondsChanged.Subscribe` 1+ 곳** (bond_renderer) — 단일 단계 양방 입증
- [ ] **`core::data::ElementDatabase::getInstance` 호출 1+ 곳** (bond_manager — Phase 2 두 번째 사용자)
- [ ] **vtk_renderer bond 분할 — vtk* 호출 갯수 legacy 와 비교 (±10%)** + **분할 100% 완료** (atom + bond + cell 합계가 legacy vtk_renderer 의 vtk* 호출 총량과 일치)
- [ ] **§1.4.1 UI 보존 — Side-by-side 스크린샷 첨부** (Bonds Management)
- [ ] **시나리오 S4 — Recompute 후 H-O 본드 cylinder 표시 legacy 와 동일**
- [ ] **Phase 3.4.1 시나리오 S3-cell 회귀 부재** + **Phase 3.4.2 시나리오 S1/S2/S5/S6 회귀 부재**
- [ ] **Intentional UI deviation 사유서 작성** (없으면 *"None"*)
- [ ] `legacy/` 한 글자도 변경 없음 (vtk_renderer + bond_renderer 분할은 *복사 + 새 트리 정리* 로만)
- [ ] PR 본문에 본 계획서 + Phase 3.4 인덱스 + 분할 제안서 + 선행 sub-phase 두 PR commit hash 링크
- [ ] PR 헤더에 *(3/3) Bonds — Phase 3.4 split completion* 표기

검토자 — 머지 전:

- [ ] diff 가 (a) features/edit/bonds/ 신규 8 파일, (b) features/edit/edit_menu 갱신 (Bonds 항목 + InitOnce 의 bondsCtrl 등록), (c) app/app.cpp 변동 *0* (3.4.2 가 이미 시그니처 확장 완료), (d) CMakeLists.txt source 추가 — 4 가지로만 구성?
- [ ] **첨부 스크린샷에서 legacy 와 새 트리의 Bonds Management 윈도우가 시각적으로 동일?**
- [ ] **시나리오 S4 본인 환경에서도 동일 결과 재현?** (특히 H-O 본드 갯수 + 위치)
- [ ] Phase 3.4.1/3.4.2 시나리오 회귀 부재 본인 환경 재현?
- [ ] vtk_renderer bond 분할 — vtk* 호출 갯수가 legacy 와 거의 동일?
- [ ] vtk_renderer 분할 100% 완료 — atom + bond + cell 합계가 legacy 와 일치?
- [ ] `legacy/` 0 changed lines
- [ ] CI 빌드 (debug + release) 통과

---

## 9. 부록

### 9.1 features/edit/bonds/ 8 파일 요약 + 라인수 예상

| 파일 | 역할 | 라인 (legacy) | 압축 후 (얇은 controller) | 압축 후 (풍부 controller) |
|---|---|---|---|---|
| `bonds/bond_manager.{cpp,h}` | 자동 검출 알고리즘 보존 + element_database 두 번째 사용자 | 1,065 | ~750 | ~750 |
| `bonds/bond_renderer.{cpp,h}` | vtk_renderer bond 분할 (3/3) + bond_renderer.cpp (42) 흡수 | (1,792 의 ~700 + 76) | ~500 | ~500 |
| `bonds/bonds_controller.{cpp,h}` | 통합 진입점 + onBondsChanged 구독 + (선택) onAtomsChanged 두 번째 구독 | — | ~150 | ~350 |
| `bonds/bond_ui.{cpp,h}` | Bonds Management (§1.4.1) | 261 | ~180 | ~180 |
| **합계** | | **~2,102** | **~1,580** (≈ 75%) | **~1,780** (≈ 85%) |

> Phase 3.3 평가서 §1.6.2 의 *controller 두께 변동성* 반영 — 두 시나리오 모두 명시. 본 단계는 *(선택) onAtomsChanged 두 번째 구독자* 가 controller 풍부도를 가를 결정 요인. *옵션 미구현 시 ~1,580 줄, 구현 시 ~1,780 줄*.

### 9.2 사후 점검: Phase 3.4.3 머지 직후 트리 — Phase 3.4 완료 시점

```
webassembly/src/
├─ ... (Phase 0-2 + 3.1 + 3.2 + 3.3 + 3.4.1 + 3.4.2 유지)
├─ features/
│  ├─ utilities/brillouin_zone/        (Phase 3.1)
│  ├─ data/                            (Phase 3.2)
│  ├─ build/                           (Phase 3.3)
│  └─ edit/                            ★ Phase 3.4 완료 (3.4.1 + 3.4.2 + 3.4.3)
│     ├─ edit_menu.{cpp,h}             (★ 갱신 — Cell + Atoms + Bonds 3 항목 모두 활성)
│     ├─ cell/                          (Phase 3.4.1 — 8 파일)
│     ├─ atoms/                         (Phase 3.4.2 — 10 파일)
│     └─ bonds/                         ★ 신규 Phase 3.4.3 (8 파일)
│        ├─ bond_manager.{cpp,h}         ★ element_database 두 번째 사용자
│        ├─ bond_renderer.{cpp,h}        ★ vtk_renderer 분할 결과 (3/3) + bond_renderer.cpp 흡수
│        ├─ bonds_controller.{cpp,h}     ★ onBondsChanged 첫 구독자 (단일 단계 양방)
│        └─ bond_ui.{cpp,h}              ★ §1.4.1 보존
└─ legacy/                              (동결 — vtk_renderer 도 그대로, 단 *분할 100% 완료* 로 vtk_renderer 의 *모든 책임* 이 새 트리로 이주)
```

### 9.3 Phase 3.3 평가서의 학습 반영

| Phase 3.3 평가서 권장 | 본 sub-phase 적용 위치 |
|---|---|
| §1.6.1 — `core/data` 시그니처 lowerCamelCase | §1.1 의 element_database 두 번째 사용자 + §4 Step 2 의 코드 예시 (`getInstance() / getElementInfo()`) + §6.2 의 시그니처 사전 확인 |
| §1.6.2 — controller 두께 변동성 | §9.1 의 *얇은/풍부 controller* 두 시나리오 + §6.10 + (선택) onAtomsChanged 두 번째 구독자 옵션 |
| §1.6.3 — commit 미수행 정책 | §2 전제 항목 + §8 PR 체크리스트 |
| §1.6.4 — `onStructureAdded` 미발신 보강 | 3.4.2 에서 처리됨 — 본 sub-phase 는 *완료된 상태* 전제 |
| §1.6.6 — 클래스 vs free function 패턴 | §4 Step 2~5 코드 예시는 *의도된 형태* 임 명시 |

### 9.4 Phase 3.4 분할 마무리 정리

본 sub-phase 머지 후 [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) §6 의 13 리스크 변화 결과:

| 분할 효과 (실제) |
|---|
| 검토자 부담: 단일 PR ~4,400 줄 → sub-phase 별 700/1,900/1,400 줄. **결정적 완화** ✓ |
| §1.4 UI 보존 위반 위험: 3 윈도우 동시 검증 → sub-phase 별 1 윈도우. **결정적 완화** ✓ |
| vtk_renderer 분할 누락 위험: 1,792 줄 한 PR → sub-phase 별 ~300/~700/~700 줄. **결정적 완화** ✓ |
| EventBus 5 종 검증: 동시 도입 → sub-phase 별 분산 (3.4.1 onCellChanged, 3.4.2 onAtomsChanged + onStructureAdded + mouse_interactor, 3.4.3 onBondsChanged 양방 + element_database 두 번째). **각 단계 명확** ✓ |
| Phase 3.3 의 atom 추가 시각화: PR 머지 시점 → 3.4.2 시점 (~3 일 지연). **약간 지연** △ |
| 작성자 부담: 5~7 일 → 6.5~8.5 일 (+20~30%). **약간 격상** △ |

→ 분할 채택의 결정적 완화 3 종이 약간 격상 2 종을 충분히 상쇄. 분할 정책의 효과 *최종 확인*.

---

## 10. 후속 단계 연결 — Phase 3.5 (measurement)

Phase 3.4 (3.4.1 + 3.4.2 + 3.4.3) 모두 머지 후 Phase 3.5 (`features/measurement`) 진입.

1. `features/measurement/` 신설 — 단일 sub-folder, 5 개 모드 토글 (Distance / Angle / Dihedral / Geometric Center / Center of Mass).
2. legacy `atoms_template.cpp` 의 측정 관련 메서드 (Enter/Exit/Click/Drag/Render/Store/Visible) 이식.
3. **`core/vtk::MouseInteractor` 의 *두 번째 구독자* 도착** — Phase 3.4.2 의 atoms_controller 와 *동일 publisher 의 두 구독자* 패턴 첫 검증.
4. **`core/scene/EventBus::onAtomsChanged` 의 *두 번째 구독자* 도착** — atom 삭제 시 잘못된 측정 자동 제거.
5. 메뉴: `Measurement / Distance · Angle · Dihedral · Geometric Center · Center of Mass`.

본 sub-phase (3.4.3) 머지 시점에 **Phase 3.4 의 분할 작업 종결** + **vtk_renderer 1,792 줄 분할 100% 완료** + **Phase 2 인프라 (EventBus 5 종 + mouse_interactor + element_database) 의 *결정적 시험대* 통과**. Phase 3.5 세부계획서: `phase3_5_measurement.md` 별도 작성.

---

## 11. 관련 문서

- 분할 컨텍스트: [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) §3.1 (Option A) + §5.3 (3.4.3 검증 매트릭스 7+7=14 항목)
- 모체 계획서: [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md) (전체 그림 + EventBus 5 종 + 시나리오 S1~S6)
- 선행 sub-phase: [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md), [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md)
- 선행 평가서: [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + **§6.0 공통 지침 (UI 1:1 보존)** + §11 (vtk_renderer 분할 리스크 — 본 sub-phase 머지로 해소) + §13 (UI 이식 공통 지침)
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
- UI 이식 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
