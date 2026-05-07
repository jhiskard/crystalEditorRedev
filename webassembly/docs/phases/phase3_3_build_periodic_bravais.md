# Phase 3.3 — Build / Periodic Table + Bravais Lattice 이식 (Third Feature) 세부계획서

> 상위 문서:    [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.3) + §6.0 공통 지침
> 선행 문서:    [`./phase3_2_data_charge_density.md`](./phase3_2_data_charge_density.md)
> 선행 평가서:  [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md)
> 메뉴 매핑:    [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §5 Build
> 작성일:      2026-05-06
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR:     1 개 (단일) 또는 2 개 (PR1 periodic_table + PR2 bravais)
> 예상 소요:   3~4 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-06 | 초안 작성 (Phase 3.2 평가서 §6 권장 다음 단계 항목을 본 문서로 확장) |

---

## 0. 한 줄 요약

> Phase 3.1/3.2 가 확립한 *features/ 4 layer 패턴 + UI 1:1 보존 정책* 을 그대로 복제하여 **Build 메뉴의 2 항목 (Add atoms / Bravais Lattice Templates)** 을 두 sub-folder (`features/build/periodic_table/` + `features/build/bravais/`) 로 이식한다. 본 단계는 또한 **`core/data/element_database` 의 첫 외부 사용자** 로서 Phase 2 인프라의 두 번째 검증을 수행한다.
> Phase 3.2 의 57.5% 압축 패턴 + 상위 §6.0.1 *legacy UI 1:1 보존* 정책 그대로 적용. legacy 약 **3,075 줄** → 약 **1,800 줄** 예상.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `features/build/{periodic_table, bravais}/` 두 sub-folder 신설 + 약 12 파일 작성. (b) 메뉴 `Build / Add atoms` 클릭 시 Periodic Table 윈도우 표시 + 7×18 표에서 원소 선택 → atom 추가. (c) 메뉴 `Build / Bravais Lattice Templates` 클릭 시 Crystal Templates 윈도우 표시 + 14 Bravais lattice 중 선택 + 파라미터 입력 → unit cell + atoms 생성. (d) **`core/data/element_database` 의 첫 외부 사용자** — periodic_table_controller 가 ElementDatabase::Instance() 호출. (e) `npm run build-wasm:debug` + `:release` 통과 + 런타임에서 2 메뉴 항목 동작. (f) **legacy 의 Periodic Table + Crystal Templates UI 동작이 1:1 보존됨** (§1.4) |
| **비목표** | 다른 메뉴 항목 부활 (Phase 3.4 이후 진행), `menu_router` 정식 도입 (Phase 4), `WindowFlags` 통합 (Phase 4), atom 추가 후 *VTK 렌더* 의 완전 동작 (Phase 3.4 의 features/edit/atoms 의 atom_renderer 가 들어와야 — 본 단계는 *추가 기록 + 메뉴 동작* 까지), 18 항목 회귀 전체 통과, legacy/ 내부 코드 수정 (회색지대 §1.2~§1.3 예외), **legacy UI 의 *재설계* — 위젯 위치 / 라티스 항목 순서 / 파라미터 형식 등은 변경 금지** |

> Phase 3.3 의 미덕: *"검토자가 git diff 를 보고 `features/build/{periodic_table, bravais}/` 의 신규 12 파일 + `app/app.cpp` 의 메뉴 hook 1 줄 + `CMakeLists.txt` 의 source 추가 외에 의심할 게 없고, side-by-side 스크린샷에서 legacy 와 새 트리의 두 윈도우가 시각적으로 동일하다"*.

### 1.1 Phase 3.3 의 *세 번째 feature* 의의 — element_database 첫 사용자

본 단계는 **Phase 2 가 만든 `core/data/element_database` 의 첫 외부 사용자** 가 된다. Phase 3.2 가 `format_registry` 를 첫 검증했다면, 본 단계는 **두 번째 인프라 검증**.

| Phase 2 인프라 | Phase 3.3 의 첫 사용 |
|---|---|
| `core/data/element_database` | periodic_table_controller 가 `ElementDatabase::Instance().GetElementInfo(symbol)` 호출 |
| `core/data/color` | 원소 색상 (Jmol/CPK) 표시 |
| `core/scene/SceneState::structures` | Bravais lattice 적용 시 *currentStructureId 에 atom 추가* + 새 구조 Register |
| `core/scene/EventBus::onAtomsChanged` | atom 추가 시 emit (Phase 3.4 의 features/edit/atoms 가 구독) |
| `core/scene/EventBus::onStructureAdded` | Bravais lattice 의 새 구조 생성 시 emit |

→ Phase 3.2 가 *읽기 인프라* (format_registry) 를 검증했다면, **Phase 3.3 은 *데이터 인프라 + 변경 이벤트* 를 검증**한다.

### 1.2 회색지대 정책 인계 (Phase 0 §1.1 + Phase 1 §1.2 + Phase 3.1 §1.3)

본 계획서에서도 세 회색지대 정책을 그대로 적용한다.

| 회색지대 출처 | 적용 범위 (Phase 3.3) |
|---|---|
| Phase 0 §1.1 — typo build-fix | legacy/ 내부의 1~5 줄 typo |
| Phase 1 §1.2 — 빌드 호환성 shim | 새 트리 측의 얇은 redirect 파일 |
| Phase 3.1 §1.3 — 임시 메뉴 hook | `app/app.cpp::renderDockSpace` 에 `features::build::DrawMenu(ctx)` 추가 |

### 1.3 라인수 압축 정책 (Phase 3.1/3.2 의 학습 적용)

Phase 3.1 (53%) / Phase 3.2 (57.5%) 의 압축 패턴이 본 단계에서도 자연 발생할 것으로 예상. Legacy 3,075 줄 → 약 1,800 줄 예상 (압축률 58~60%).

**보존 필수 (압축 시에도 변경 금지)**:

- `crystal_structure.cpp` 의 14 Bravais lattice 격자 파라미터 → 매트릭스 변환 알고리즘 (361 줄)
- `crystal_system.cpp` 의 7 Crystal System ↔ 14 Lattice 매핑 (145 줄)
- `atoms_template_periodic_table.cpp` 의 atom 추가 흐름 (475 줄)
- `atoms_template_bravais_lattice.cpp` 의 setBravaisLattice 흐름 (475 줄)
- `core/data/element_database` 호출 (`Instance()`, `GetElementInfo`, `GetElementPosition`)
- §1.4 의 모든 UI 보존 항목

### 1.4 legacy UI 작동방식 1:1 보존 (상위 §6.0.1 의 본 단계 적용)

상위 계획서 [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6.0.1 의 **legacy UI 작동방식 1:1 보존 원칙** 이 본 단계에 그대로 적용된다.

#### 1.4.1 Periodic Table 윈도우 보존 대상

`legacy/atoms/ui/periodic_table_ui.cpp` (453 줄) 가 정의하는 다음 UI 동작은 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| 표 레이아웃 | **7 × 18** 격자 형태 (legacy 의 정확한 행/열 구조) — 원소 칸 크기 / 간격 / 정렬 |
| 원소 칸 표시 | 원자번호 + 기호 + 원자량 (legacy 의 표시 형식 / 폰트 크기 / 색상) |
| 색상 코딩 | Jmol/CPK 표준 색 — 알칼리/할로겐/희가스 등 그룹별 색 |
| 선택 / 호버 | 선택된 원소 강조 표시 / 호버 시 보조 정보 패널 (mass, electroneg., crystalradius 등) |
| 우측 정보 패널 | 선택된 원소의 *상세 정보* (legacy 의 정확한 행 순서 / 필드명) |
| Add atom 버튼 | 클릭 시 atom 추가 sequence — legacy 의 default 위치 (예: cell 중심 또는 origin) 보존 |
| 검색 / 필터 | 원소 기호로 빠른 검색 — legacy 가 가졌다면 보존 |

#### 1.4.2 Crystal Templates (Bravais Lattice) 윈도우 보존 대상

`legacy/atoms/ui/bravais_lattice_ui.cpp` (599 줄) 가 정의하는 다음 UI 동작은 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| 14 라티스 선택 | 라디오/콤보의 항목 순서 — Cubic 3 종 → Tetragonal 2 종 → Orthorhombic 4 종 → Monoclinic 2 종 → Triclinic 1 종 → Rhombohedral 1 종 → Hexagonal 1 종 (총 14) |
| 7 Crystal System 그룹 | 라티스 항목이 *Crystal System 별로 그룹핑* 된 상태 (legacy 의 그룹 헤더 / 들여쓰기) |
| 라티스 별 파라미터 입력 | 각 라티스의 *제약 조건이 반영된 입력* (Cubic 의 a 만, Tetragonal 의 a+c, Triclinic 의 a/b/c/α/β/γ) — legacy 의 grey-out 규칙 보존 |
| 미리보기 / 격자 시각화 | 윈도우 안의 격자 미리보기 패널 (legacy 가 가졌다면 보존) |
| `preserveExistingAtoms` 토글 | 체크박스 라벨 / 기본 상태 / 변경 시 응답 동작 |
| Apply 버튼 sequence | 클릭 시 *기존 액터 청산 → 새 unit cell + atoms 생성 → SceneState 갱신* sequence |

#### 1.4.3 보존 검증 절차 (상위 §6.0.2 의 본 단계 적용)

PR 작성자는 본 PR 본문에 다음 검증을 첨부:

1. **Side-by-side 스크린샷** — legacy Periodic Table + Crystal Templates vs 새 트리의 두 윈도우.
2. **사용자 시나리오 정합성** — 다음 4 시나리오를 legacy 와 새 트리에서 차례로 실행해 동일 결과 확인:
   - **(S1)** Periodic Table → "C" (Carbon) 클릭 → 우측 정보 패널의 mass/electroneg. 표시 확인
   - **(S2)** Periodic Table → "C" 클릭 → "Add atom" → SceneState 에 atom 추가 확인 (Phase 3.4 의 atom_renderer 도착 후 시각 확인 가능, 본 단계는 *기록* 까지)
   - **(S3)** Crystal Templates → "FCC (Face-Centered Cubic)" → a=3.5 → Apply → unit cell 매트릭스 계산 + atom 4 개 추가
   - **(S4)** Crystal Templates → "Triclinic" → a=4, b=5, c=6, α=80, β=90, γ=100 → Apply → 격자 파라미터 그대로 적용
3. **Intentional UI deviation 사유서** — 의도적인 UI 변경 발생 시 PR 본문 명기 (없으면 *"None"*).

#### 1.4.4 압축과 UI 보존의 조화 (예시)

| 압축 가능 ✓ | 보존 필수 ✗ (변경 금지) |
|---|---|
| 한국어 깨진 인코딩 주석 → Doxygen | 7×18 원소 표의 *행/열 좌표 매핑* (어떤 원소가 어디에 있는지) |
| `m_parent->X()` → `controller_.X()` | 14 Bravais lattice 의 *순서와 라벨* (드롭다운 항목 텍스트) |
| 미사용 `m_parent` 멤버 제거 | "Add atom" 버튼 클릭 시 *default 위치 결정 알고리즘* (origin? cell 중심?) |
| `printf` 디버그 제거 | Apply 버튼 클릭 시 *기존 atom 의 처리 정책* (preserveExistingAtoms 의 동작) |

---

## 2. 전제 — Phase 3.2 완료 상태

본 계획서는 다음이 충족된 상태에서 시작한다.

- [ ] Phase 3.2 commit 머지 (Phase 3.3 PR 의 base commit)
- [ ] `webassembly/src/features/data/{charge_density, slice}/` 17 파일 + 임시 메뉴 hook 정상 동작
- [ ] `core/{scene, io, data, vtk, render, ui}` 인프라 빌드 가능
- [ ] `npm run build-wasm:debug` + `:release` exit 0
- [ ] Data 메뉴 4 항목 동작 (Phase 3.2 §5 #15-22 통과)
- [ ] **`core/io::FormatRegistry::RegisterDefaults` 호출 확인됨** (Phase 3.2 §1.4)

위 통과 여부는 [`phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md) §4.3.A 의 commit 통과로 확인.

---

## 3. legacy 참조 인벤토리

| legacy 파일 | 라인 수 | 역할 | 새 트리 행선지 |
|---|---|---|---|
| `legacy/atoms/domain/crystal_structure.{cpp,h}` | 361 + 125 | `BravaisLatticeType` enum (14 종) + 격자 파라미터 → 매트릭스 변환 | `features/build/bravais/crystal_structure.{cpp,h}` |
| `legacy/atoms/domain/crystal_system.{cpp,h}` | 145 + 69 | `CrystalSystem` enum (7 종) + Lattice ↔ System 매핑 | `features/build/bravais/crystal_system.{cpp,h}` |
| `legacy/atoms_template_periodic_table.cpp` | 523 (확인 필요) | AtomsTemplate 의 분리 — *Add atom* 흐름 (PeriodicTable → atom 추가) | `features/build/periodic_table/periodic_table_controller.cpp` 로 흡수 |
| `legacy/atoms_template_bravais_lattice.cpp` | 475 | AtomsTemplate 의 분리 — *setBravaisLattice* 흐름 | `features/build/bravais/bravais_controller.cpp` 로 흡수 |
| `legacy/atoms/ui/periodic_table_ui.{cpp,h}` | 453 + 148 | `PeriodicTableUI` — 7×18 표 + 원소 선택 + 정보 패널 | `features/build/periodic_table/periodic_table_ui.{cpp,h}` |
| `legacy/atoms/ui/bravais_lattice_ui.{cpp,h}` | 599 + 177 | `BravaisLatticeUI` — 14 라티스 + 7 system 그룹 + 파라미터 입력 | `features/build/bravais/bravais_lattice_ui.{cpp,h}` |
| **(신규)** | — | `periodic_table_controller`, `bravais_controller`, `build_menu` | 신규 작성 |

**총 legacy 참조: 약 3,075 줄.** 압축 후 약 **1,800 줄** 예상 (Phase 3.2 의 57.5% 와 비슷).

### 3.1 외부 의존 사전 분석

| legacy 파일 | 외부 의존 | 새 트리 처리 |
|---|---|---|
| `crystal_structure.{cpp,h}` | STL 만 (`<array>`, `<vector>`, `<string>`) | namespace 만 `features::build::bravais` 로 변경, 그대로 이식 |
| `crystal_system.{cpp,h}` | `crystal_structure.h` 만 | 동상 |
| `periodic_table_ui.h` | `class AtomsTemplate` (forward), `class ElementDatabase` (forward), `struct ElementInfo` | AtomsTemplate forward → controller 로 대체. **`core::data::ElementDatabase` 첫 호출** |
| `periodic_table_ui.cpp` | `m_parent->X()`, `m_elementDB->getElementInfo(...)` 등 | controller 로 분기. ElementDatabase::Instance() → `core::data::ElementDatabase::Instance()` |
| `bravais_lattice_ui.h` | `crystal_structure.h`, `crystal_system.h`, `class AtomsTemplate` (forward) | namespace 정리 + controller 분기 |
| `bravais_lattice_ui.cpp` | `m_parent->setBravaisLattice(...)` 등 | controller 로 분기 |
| `atoms_template_periodic_table.cpp` | AtomsTemplate 의 atom 추가 로직 (createdAtoms, atomGroups 갱신) | controller 안에 흡수 — SceneState DI 사용 |
| `atoms_template_bravais_lattice.cpp` | AtomsTemplate 의 setBravaisLattice + cell 갱신 + atom 생성 | controller 안에 흡수 — SceneState + EventBus emit |

### 3.2 메뉴 트리 매핑 (04 §5 Build 참조)

| 메뉴 항목 | 새 진입점 | 윈도우 |
|---|---|---|
| `Build / Add atoms` | `features::build::periodic_table::Show()` | Periodic Table |
| `Build / Bravais Lattice Templates` | `features::build::bravais::Show()` | Crystal Templates |

→ Phase 3.2 와 같은 *2 항목 → 2 sub-folder 분기* 패턴 (단, 4 항목 → 2 sub-folder 와 다름).

---

## 4. 작업 절차 (단계별)

### Step 1 — `features/build/{periodic_table, bravais}/` 폴더 신설

```bash
cd webassembly/src
mkdir -p features/build/periodic_table
mkdir -p features/build/bravais
```

### Step 2 — `bravais/{crystal_structure, crystal_system}.{cpp,h}` 도메인 이식

legacy 의 자기완결적 도메인 코드를 namespace 정리만 하며 그대로 이식.

```cpp
/**
 * @file features/build/bravais/crystal_structure.h
 * @brief Bravais 격자 14 종 + 격자 파라미터 → 매트릭스 변환.
 */
#pragma once

#include <array>
#include <string>
#include <vector>

namespace features::build::bravais {

enum class BravaisLatticeType {
    SIMPLE_CUBIC = 0,
    BODY_CENTERED_CUBIC = 1,
    FACE_CENTERED_CUBIC = 2,
    SIMPLE_TETRAGONAL = 3,
    BODY_CENTERED_TETRAGONAL = 4,
    SIMPLE_ORTHORHOMBIC = 5,
    BODY_CENTERED_ORTHORHOMBIC = 6,
    FACE_CENTERED_ORTHORHOMBIC = 7,
    BASE_CENTERED_ORTHORHOMBIC = 8,
    SIMPLE_MONOCLINIC = 9,
    BASE_CENTERED_MONOCLINIC = 10,
    TRICLINIC = 11,
    RHOMBOHEDRAL = 12,
    HEXAGONAL = 13
};

struct BravaisParameters {
    float a = 1.0f, b = 1.0f, c = 1.0f;
    float alpha = 90.0f, beta = 90.0f, gamma = 90.0f;
};

/// @brief 격자 파라미터 → 3x3 lattice matrix 변환.
std::array<std::array<float, 3>, 3> ComputeLatticeMatrix(
    BravaisLatticeType type, const BravaisParameters& params);

/// @brief Lattice type 의 사용자 표시 이름.
std::string LatticeTypeName(BravaisLatticeType type);

/// @brief Lattice type 별 default atom 위치 (fractional 좌표).
std::vector<std::array<float, 3>> DefaultAtomPositions(BravaisLatticeType type);

} // namespace features::build::bravais
```

### Step 3 — `periodic_table/periodic_table.{cpp,h}` 도메인 이식 (선택)

PeriodicTable 자체는 *데이터* 도메인이 거의 없음 (대부분 `core::data::ElementDatabase` 에 위임). 단, 본 feature 안의 *atom 추가 정책* (default 위치, group 분류) 만 정의.

```cpp
/**
 * @file features/build/periodic_table/periodic_table.h
 * @brief PeriodicTable 의 도메인 헬퍼 — atom 추가 정책.
 */
#pragma once

#include <array>
#include <string>

namespace features::build::periodic_table {

/// @brief 새 atom 의 default 위치 결정 (legacy 의 origin 또는 cell 중심).
std::array<float, 3> ComputeDefaultPosition(const std::string& symbol);

} // namespace features::build::periodic_table
```

### Step 4 — `bravais/bravais_controller.{cpp,h}` (Bravais 흐름)

legacy 의 `atoms_template_bravais_lattice.cpp` (475 줄) 의 `setBravaisLattice` 흐름을 흡수.

```cpp
/**
 * @file features/build/bravais/bravais_controller.h
 * @brief Bravais Lattice Templates 의 통합 진입점.
 */
#pragma once

#include "crystal_structure.h"
#include "crystal_system.h"
#include "../../../core/scene/scene_state.h"

namespace features::build::bravais {

class BravaisController {
public:
    explicit BravaisController(core::scene::SceneState& scene);

    /// @brief Bravais lattice 적용 — unit cell + atoms 생성.
    /// @param[in] type Bravais lattice 타입.
    /// @param[in] params 격자 파라미터.
    /// @param[in] preserveExistingAtoms 기존 atom 좌표 보존 여부.
    void Apply(BravaisLatticeType type, const BravaisParameters& params,
               bool preserveExistingAtoms);

    /// @brief 현재 선택된 라티스 / 파라미터 / system.
    BravaisLatticeType CurrentType() const { return currentType_; }
    const BravaisParameters& CurrentParams() const { return params_; }
    CrystalSystem CurrentSystem() const;

private:
    core::scene::SceneState&    scene_;
    BravaisLatticeType          currentType_ = BravaisLatticeType::SIMPLE_CUBIC;
    BravaisParameters           params_;
    bool                        preserveExistingAtoms_ = true;
};

} // namespace features::build::bravais
```

### Step 5 — `periodic_table/periodic_table_controller.{cpp,h}` (Add atom 흐름)

legacy 의 `atoms_template_periodic_table.cpp` (≈ 475 줄) 의 *atom 추가* 흐름을 흡수.

```cpp
/**
 * @file features/build/periodic_table/periodic_table_controller.h
 * @brief Periodic Table 의 통합 진입점 — atom 추가.
 */
#pragma once

#include "../../../core/scene/scene_state.h"
#include "../../../core/data/element_database.h"

#include <string>

namespace features::build::periodic_table {

class PeriodicTableController {
public:
    explicit PeriodicTableController(core::scene::SceneState& scene);

    /// @brief 선택된 원소를 default 위치에 atom 으로 추가.
    void AddAtom(const std::string& symbol);

    /// @brief 선택된 원소를 특정 위치에 atom 으로 추가.
    void AddAtomAt(const std::string& symbol, const std::array<float, 3>& position);

    /// @brief 현재 선택된 원소 기호.
    const std::string& SelectedSymbol() const { return selectedSymbol_; }
    void SetSelectedSymbol(const std::string& symbol) { selectedSymbol_ = symbol; }

    /// @brief 현재 선택된 원소의 정보 — `core::data::ElementDatabase` 위임.
    const core::data::ElementInfo* SelectedElementInfo() const;

private:
    core::scene::SceneState&    scene_;
    std::string                 selectedSymbol_;
};

} // namespace features::build::periodic_table
```

> **§1.1 Phase 2 인프라 첫 사용**: `SelectedElementInfo()` 가 `core::data::ElementDatabase::Instance().GetElementInfo(symbol)` 를 호출하는 첫 외부 사용자. Phase 2 의 element_database.h 인터페이스가 이 시점에 처음 검증됨.

### Step 6 — UI 이식 (`periodic_table_ui` + `bravais_lattice_ui`)

#### 6.1 `periodic_table/periodic_table_ui.{cpp,h}`

legacy `periodic_table_ui.cpp` (453 줄) 의 ImGui 흐름을 *위젯 단위로 1:1 보존* 하며 압축 이식.

```cpp
/**
 * @file features/build/periodic_table/periodic_table_ui.h
 * @brief Periodic Table 윈도우 ImGui 렌더.
 */
#pragma once
#include <imgui.h>

namespace features::build::periodic_table {

class PeriodicTableController;

class PeriodicTableUI {
public:
    explicit PeriodicTableUI(PeriodicTableController& controller);
    void Render(bool* open);

private:
    PeriodicTableController&   controller_;
    char                       searchBuffer_[64] = {0};   // legacy 가 검색 기능 가졌다면
    // ★ §1.4.1 보존: 7×18 표 / 우측 정보 패널 / Add atom 버튼 등 legacy 흐름 그대로
};

} // namespace features::build::periodic_table
```

> **§1.4.1 보존 핵심**: 7×18 표의 좌표 매핑은 legacy 의 정확한 위치를 그대로 따름. 색상 (Jmol/CPK) 은 `core/data/color.h` 사용.

#### 6.2 `bravais/bravais_lattice_ui.{cpp,h}`

legacy `bravais_lattice_ui.cpp` (599 줄) 의 ImGui 흐름을 *위젯 단위로 1:1 보존* 하며 압축 이식.

```cpp
/**
 * @file features/build/bravais/bravais_lattice_ui.h
 * @brief Crystal Templates (Bravais Lattice) 윈도우 ImGui 렌더.
 */
#pragma once
#include <imgui.h>

namespace features::build::bravais {

class BravaisController;

class BravaisLatticeUI {
public:
    explicit BravaisLatticeUI(BravaisController& controller);
    void Render(bool* open);

private:
    BravaisController&     controller_;
    BravaisLatticeType     selectedType_ = BravaisLatticeType::SIMPLE_CUBIC;
    BravaisParameters      params_;
    bool                   preserveExistingAtoms_ = true;
    // ★ §1.4.2 보존: 14 라티스 / 7 system 그룹 / 파라미터 grey-out / Apply sequence 그대로
};

} // namespace features::build::bravais
```

### Step 7 — `build_menu.{cpp,h}` 작성 (2 항목 dispatch)

Phase 3.2 의 `data_menu` 와 동일 패턴.

```cpp
/**
 * @file features/build/build_menu.h
 * @brief Build 메뉴의 2 항목 (Add atoms / Bravais Lattice Templates) 진입점.
 */
#pragma once

namespace app { struct MenuContext; }
namespace core::scene { struct SceneState; }

namespace features::build {

void DrawMenu(const app::MenuContext& ctx);
void HandleRequest(int requestType);
void RenderWindows(bool* showPeriodicTable, bool* showBravais);
void Tick(float dt);
void Shutdown();
void InitOnce(core::scene::SceneState& scene);

} // namespace features::build
```

```cpp
// features/build/build_menu.cpp
#include "build_menu.h"
#include "periodic_table/periodic_table_controller.h"
#include "periodic_table/periodic_table_ui.h"
#include "bravais/bravais_controller.h"
#include "bravais/bravais_lattice_ui.h"

namespace features::build {

namespace {
    bool g_showPeriodicTable = false;
    bool g_showBravais       = false;
    periodic_table::PeriodicTableController* g_ptCtrl = nullptr;
    periodic_table::PeriodicTableUI*         g_ptUI   = nullptr;
    bravais::BravaisController*              g_brCtrl = nullptr;
    bravais::BravaisLatticeUI*               g_brUI   = nullptr;
}

void DrawMenu(const app::MenuContext& /*ctx*/) {
    if (ImGui::BeginMenu("  Build")) {
        if (ImGui::MenuItem("Add atoms")) {
            g_showPeriodicTable = true;
        }
        if (ImGui::MenuItem("Bravais Lattice Templates")) {
            g_showBravais = true;
        }
        ImGui::EndMenu();
    }
}

void RenderWindows(bool* /*pt*/, bool* /*br*/) {
    if (g_showPeriodicTable && g_ptUI)  g_ptUI->Render(&g_showPeriodicTable);
    if (g_showBravais       && g_brUI)  g_brUI->Render(&g_showBravais);
}

void InitOnce(core::scene::SceneState& scene) {
    static periodic_table::PeriodicTableController ptCtrl(scene);
    static periodic_table::PeriodicTableUI         ptUI(ptCtrl);
    static bravais::BravaisController              brCtrl(scene);
    static bravais::BravaisLatticeUI               brUI(brCtrl);
    g_ptCtrl = &ptCtrl; g_ptUI = &ptUI;
    g_brCtrl = &brCtrl; g_brUI = &brUI;
}

} // namespace features::build
```

### Step 8 — `app/app.cpp` 임시 메뉴 hook

```cpp
#include "../features/build/build_menu.h"   // 신규

void App::renderDockSpace() {
    ...
    if (ImGui::BeginMenuBar()) {
        ...
        features::utilities::bz::DrawMenu(/*ctx*/{});
        features::data::DrawMenu(/*ctx*/{});
        features::build::DrawMenu(/*ctx*/{});            // ← 신규 Phase 3.3
        ...
    }
    features::utilities::bz::RenderWindows(nullptr);
    features::data::RenderWindows(nullptr, nullptr);
    features::build::RenderWindows(nullptr, nullptr);     // ← 신규
}

int App::Init() {
    ...
    static core::scene::SceneState scene;
    features::utilities::bz::InitOnce(scene);
    features::data::InitOnce(scene);
    features::build::InitOnce(scene);                     // ← 신규
}
```

### Step 9 — `CMakeLists.txt` SOURCES_FEATURES 확장

```cmake
set(SOURCES_FEATURES
    # Phase 3.1
    webassembly/src/features/utilities/brillouin_zone/...
    # Phase 3.2
    webassembly/src/features/data/...

    # Phase 3.3 신규
    webassembly/src/features/build/build_menu.cpp
    webassembly/src/features/build/build_menu.h
    webassembly/src/features/build/periodic_table/periodic_table.cpp
    webassembly/src/features/build/periodic_table/periodic_table.h
    webassembly/src/features/build/periodic_table/periodic_table_controller.cpp
    webassembly/src/features/build/periodic_table/periodic_table_controller.h
    webassembly/src/features/build/periodic_table/periodic_table_ui.cpp
    webassembly/src/features/build/periodic_table/periodic_table_ui.h
    webassembly/src/features/build/bravais/crystal_structure.cpp
    webassembly/src/features/build/bravais/crystal_structure.h
    webassembly/src/features/build/bravais/crystal_system.cpp
    webassembly/src/features/build/bravais/crystal_system.h
    webassembly/src/features/build/bravais/bravais_controller.cpp
    webassembly/src/features/build/bravais/bravais_controller.h
    webassembly/src/features/build/bravais/bravais_lattice_ui.cpp
    webassembly/src/features/build/bravais/bravais_lattice_ui.h
)
```

총 16 신규 파일 (Phase 3.2 의 17 파일과 비슷).

### Step 10 — 정적 검증

```bash
cd webassembly/src

# legacy 호출 0
grep -rnE "AtomsTemplate::|atoms::domain::|atoms::infrastructure::|atoms::ui::" \
  features/build/ | grep -v "//" || echo "OK"

# legacy include 0
grep -rnE '#include\s+"(\.\./)*legacy/' features/build/ | grep -v "//" || echo "OK"

# **element_database 첫 호출 확인**
grep -rnE "core::data::ElementDatabase::Instance|core::data::ElementDatabase::|GetElementInfo|GetElementPosition" \
  features/build/ | head -5

# 14 Bravais lattice enum 보존
grep -nE "SIMPLE_CUBIC|BODY_CENTERED_CUBIC|FACE_CENTERED_CUBIC|HEXAGONAL|TRICLINIC" \
  features/build/bravais/crystal_structure.h | wc -l   # 5+ 기대 (14 종 모두)

# 7 Crystal System enum 보존
grep -nE "CUBIC|TETRAGONAL|ORTHORHOMBIC|MONOCLINIC|TRICLINIC|RHOMBOHEDRAL|HEXAGONAL" \
  features/build/bravais/crystal_system.h | wc -l

# namespace 일관성
grep -rnE "^namespace features::build" features/build/

# **§1.4 UI 보존 — ImGui 위젯 인자 grep diff**
grep -nE 'ImGui::(BeginTable|TableNextRow|MenuItem|Button|RadioButton|Combo|InputFloat)' \
  legacy/atoms/ui/{periodic_table_ui,bravais_lattice_ui}.cpp \
  features/build/{periodic_table/periodic_table_ui.cpp,bravais/bravais_lattice_ui.cpp} 2>/dev/null
```

### Step 11 — 빌드 + 런타임 검증 (Windows 측)

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release
npm run dev
```

**§1.4 보존 검증**:

- [ ] Side-by-side 스크린샷 (legacy Periodic Table + Crystal Templates vs 새 트리)
- [ ] §1.4.3 의 시나리오 S1/S2/S3/S4 각 legacy 와 새 트리 동일 결과 확인
- [ ] Intentional UI deviation 사유서

### Step 12 — 커밋 & PR

```powershell
git commit -m "Phase 3.3: features/build/{periodic_table, bravais} — third feature

- New folders + 16 ported files (legacy ~3,075 lines compressed to ~1,800).
- Menu bar: Build / Add atoms / Bravais Lattice Templates.
- core/data::ElementDatabase first external user — Phase 2 infra second validation.
- core/scene/EventBus::onAtomsChanged + onStructureAdded first emitters.
- legacy Periodic Table + Crystal Templates UI preserved 1:1 per §6.0.1.

Side-by-side screenshots and S1/S2/S3/S4 scenario tests attached.
Intentional UI deviation: None.

Reference:
  - webassembly/docs/05_redevelopment_plan.md (Phase 3.3 + §6.0)
  - webassembly/docs/phases/phase3_3_build_periodic_bravais.md
"
```

---

## 5. 검증 매트릭스

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `features/build/{periodic_table, bravais}/` 폴더 존재 | `ls features/build/` | 2 폴더 | 정적 |
| 2 | periodic_table 파일 수 | `ls features/build/periodic_table/` | 6 (.cpp 3 + .h 3) | 정적 |
| 3 | bravais 파일 수 | `ls features/build/bravais/` | 8 (.cpp 4 + .h 4) | 정적 |
| 4 | build_menu | `ls features/build/build_menu.*` | `build_menu.cpp build_menu.h` | 정적 |
| 5 | namespace 일관성 (`features::build::*`) | grep | 모든 .cpp/.h | 정적 |
| 6 | legacy 호출 0 | grep | 0 hit | 정적 |
| 7 | `#include "../legacy/"` 0 | grep | 0 hit | 정적 |
| 8 | **`core::data::ElementDatabase::Instance` 호출 1+ 곳** | grep | 1+ hit | 정적 — *Phase 2 인프라 두 번째 검증의 핵심* |
| 9 | 14 Bravais lattice enum 보존 | grep `crystal_structure.h` | 14 enum 항목 | 정적 |
| 10 | 7 Crystal System enum 보존 | grep `crystal_system.h` | 7 enum 항목 | 정적 |
| 11 | `app/app.cpp` 의 features::build 호출 추가 | grep | 3 hit | 정적 |
| 12 | CMakeLists.txt 의 SOURCES_FEATURES 확장 | grep | 16+ hit | 정적 |
| 13 | CMake 안전벨트 유지 | grep | 1 hit | 정적 |
| 14 | **§1.4 UI 보존 — ImGui 위젯 인자 비교** | legacy 와 새 트리 grep diff | 일치 (의도적 차이는 §1.4.3 사유서) | 정적 |
| 15 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 16 | Release 빌드 | `npm run build-wasm:release` | exit 0 | 동적 |
| 17 | 메뉴바에 Build 메뉴 추가 | `npm run dev` | Data 옆 Build 메뉴 | 동적 |
| 18 | Build → Add atoms | 메뉴 클릭 | Periodic Table 윈도우 표시 | 동적 |
| 19 | Build → Bravais Lattice Templates | 메뉴 클릭 | Crystal Templates 윈도우 | 동적 |
| 20 | **§1.4 — Side-by-side 스크린샷** (PT + CT) | legacy vs 새 트리 시각 비교 | 위젯 동일 | 동적 — 핵심 |
| 21 | **§1.4 — 시나리오 S1/S2** (Periodic Table) | C 클릭 + 정보 패널 + Add atom | legacy 와 동일 | 동적 — 핵심 |
| 22 | **§1.4 — 시나리오 S3/S4** (Bravais) | FCC + Triclinic 적용 | legacy 와 동일 | 동적 — 핵심 |
| 23 | 콘솔 에러 0 | DevTools | 0 errors | 동적 |
| 24 | wasm 사이즈 회귀 | Phase 3.2 ± 5~10 % | 정상 범위 | 동적 |

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | `core::data::ElementDatabase` 의 인터페이스 (Instance / GetElementInfo / GetElementPosition) 가 legacy 와 시그니처 불일치 | link error | Phase 2 의 element_database.h 사전 확인 |
| 6.2 | `atoms_template_periodic_table.cpp` (475 줄) 의 *AtomsTemplate 멤버 직접 변경* (createdAtoms 등) 패턴 | controller 로 흡수 시 *atom 추가 sequence* 누락 가능 | legacy 의 setBravaisLattice / addAtom 흐름을 1:1 매핑 + EventBus emit 추가 |
| 6.3 | `bravais_lattice_ui.cpp` (599 줄) 의 14 라티스 + 7 system 그룹 표시가 압축 이식 중 누락 | UI 보존 위반 — §1.4 violation | §5 #14 의 ImGui 위젯 grep diff 로 사전 검증 + side-by-side 스크린샷 |
| 6.4 | `periodic_table_ui.cpp` (453 줄) 의 7×18 표 좌표 매핑 누락 | 원소 위치 잘못됨 — §1.4 violation | legacy 의 좌표 매핑 (예: Hydrogen → row 1 col 1) 을 1:1 보존. 데이터 테이블이므로 압축 시 변경 금지 |
| 6.5 | atom 추가 후 *VTK 렌더 부재* — 사용자가 "Add atom 했는데 화면에 안 보임" 으로 오해 | 사용자 경험 혼란 | UI 안에 *"atom rendering will be available after Phase 3.4"* 안내. SceneState 에 기록 + EventBus emit 까지는 정상 |
| 6.6 | Bravais lattice 적용 시 `setBravaisLattice` 의 *기존 atom 처리* 정책이 controller 에서 미반영 | preserveExistingAtoms 토글이 동작 안 함 | controller.Apply(...) 의 preserveExistingAtoms 분기를 legacy 와 1:1 비교 |
| 6.7 | EventBus::onAtomsChanged + onStructureAdded 의 첫 emit | 향후 Phase 3.4 의 features/edit/atoms 가 구독해야 함 | controller 의 Apply() 끝에서 명시 emit + Phase 3.4 진입 시 구독 검증 |
| 6.8 | Phase 3.2 의 controller 풍부도 (CD 338%, Slice 217%) 패턴이 본 단계에도 적용될지 | bravais_controller 의 라인수가 계획 대비 클 수 있음 | 계획서 §9.1 의 라인수 예상 표에 controller 풍부도 +200% 가능성 명시 |
| 6.9 | element_database 가 *전역 singleton* 인 점이 features 간 충돌 가능 | 다른 feature 가 ElementDatabase 동시 변경 시 race | Phase 3.3 시점에는 element_database 첫 사용자라 충돌 없음. 후속 feature 가 추가될 때 검토 |
| 6.10 | PR diff 큼 (3,075 base + UI 보존 검증) | 머지 지연 | commit 분할 (periodic_table / bravais) + 스크린샷 사전 준비 |
| 6.11 | **§1.4 UI 보존 위반** — 14 라티스 항목 순서 / 7 system 그룹 / 7×18 표 좌표 변경 | 사용자 워크플로우 변경 | §5 #14 grep diff sweep + Side-by-side 스크린샷 + S1-S4 시나리오 테스트 |

---

## 7. 롤백 절차

```bash
git restore --staged .
git restore .
git clean -fd webassembly/src/features/build
```

원인 진단:

1. **§1.4 UI 보존 위반** — §6.11 의 즉시 복원 또는 사유서.
2. element_database 미해결 심볼 → Phase 2 인터페이스 점검.
3. Bravais Apply 후 atom 미생성 → controller.Apply 의 createdAtoms 갱신 sequence 점검.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] §2 의 Phase 3.2 전제 충족
- [ ] §5 검증 매트릭스 24 항목 통과
- [ ] features/build/ 안에서 legacy 호출 0 + legacy include 0
- [ ] **`core::data::ElementDatabase` 호출 1+ 곳** (Phase 2 두 번째 검증)
- [ ] **§1.4 UI 보존 — Side-by-side 스크린샷 첨부** (Periodic Table + Crystal Templates)
- [ ] **§1.4 — 시나리오 S1/S2/S3/S4 각 legacy 와 동일 결과 확인**
- [ ] **Intentional UI deviation 사유서 작성** (없으면 *"None"*)
- [ ] `legacy/` 한 글자도 변경 없음
- [ ] PR 본문에 *"atom 렌더링은 Phase 3.4 의 atom_renderer 후 보강"* 명시

검토자 — 머지 전:

- [ ] diff 가 (a) features/build/ 신규 16 파일, (b) app/app.cpp 임시 hook 3 곳, (c) CMakeLists.txt source 추가 — 3 가지로만 구성?
- [ ] **첨부 스크린샷에서 legacy 와 새 트리의 두 윈도우가 시각적으로 동일?**
- [ ] **시나리오 S1-S4 본인 환경에서도 동일 결과 재현?**
- [ ] `legacy/` 0 changed lines
- [ ] CI 빌드 (debug + release) 통과

---

## 9. 부록

### 9.1 features/build/ 16 파일 요약 + 라인수 예상

| 파일 | 역할 | 라인 (legacy) | 압축 후 예상 |
|---|---|---|---|
| `build_menu.{cpp,h}` | 2 항목 dispatch (신규) | — | ~140 |
| `periodic_table/periodic_table.{cpp,h}` | 도메인 헬퍼 (신규) | — | ~80 |
| `periodic_table/periodic_table_controller.{cpp,h}` | atom 추가 흐름 통합 | (475 의 일부) | ~280 |
| `periodic_table/periodic_table_ui.{cpp,h}` | 7×18 표 ImGui (§1.4 보존) | 601 | ~350~400 |
| `bravais/crystal_structure.{cpp,h}` | 14 Bravais + 매트릭스 변환 | 486 | ~400 (데이터/알고리즘 보존) |
| `bravais/crystal_system.{cpp,h}` | 7 system 매핑 | 214 | ~180 |
| `bravais/bravais_controller.{cpp,h}` | setBravaisLattice 흐름 | (475 의 일부) | ~280 |
| `bravais/bravais_lattice_ui.{cpp,h}` | 14 라티스 + 7 system + 파라미터 (§1.4 보존) | 776 | ~450 |
| **합계** | | **~3,075** | **~2,160** (≈ 70% — 데이터/알고리즘 비중이 커서 보수적) |

> Phase 3.2 의 57.5% 보다 보수적 — *14 Bravais 격자 매트릭스 변환 + 7 system 매핑* 같은 *알고리즘 + 데이터* 가 다수라 압축 여지 적음.

### 9.2 후속 sub-phase 자동 적용

| sub-phase | 본 패턴 적용 |
|---|---|
| 3.4 edit/{atoms, bonds, cell} | 1 메뉴 → 3 sub-folder. UI 보존 부담 가장 큼 (atom_editor_ui 896 줄) |
| 3.5 measurement | 단일 sub-folder. 5 모드 토글 §1.4 보존 |
| 3.6 file | 단일 sub-folder. format_registry *읽기* 사용자 |
| 3.7 viewer + toolbar | toolbar 위젯 보존 (Mesh Display Mode / Projection / Reset View 등) |

### 9.3 사후 점검: Phase 3.3 머지 직후 트리

```
webassembly/src/
├─ ... (Phase 0-2 + 3.1 + 3.2 유지)
├─ features/
│  ├─ utilities/brillouin_zone/        (Phase 3.1)
│  ├─ data/                            (Phase 3.2)
│  └─ build/                           ★ 신규 Phase 3.3
│     ├─ build_menu.{cpp,h}
│     ├─ periodic_table/
│     │  ├─ periodic_table.{cpp,h}
│     │  ├─ periodic_table_controller.{cpp,h}
│     │  └─ periodic_table_ui.{cpp,h}        ★ §1.4.1 보존 핵심 (7×18 표)
│     └─ bravais/
│        ├─ crystal_structure.{cpp,h}
│        ├─ crystal_system.{cpp,h}
│        ├─ bravais_controller.{cpp,h}
│        └─ bravais_lattice_ui.{cpp,h}       ★ §1.4.2 보존 핵심 (14 라티스 + 7 system)
└─ legacy/                             (동결)
```

---

## 10. 후속 단계 연결 — Phase 3.4

Phase 3.3 머지 후 Phase 3.4 (`features/edit/{atoms, bonds, cell}`) 진입 — Phase 3 중 가장 무거운 단계.

1. `features/edit/` 신설 + **3 sub-folder** (atoms, bonds, cell).
2. legacy `atoms/domain/{atom_manager, bond_manager, cell_manager, surrounding_atom_manager}` + UI 이식.
3. **`vtk_renderer.cpp` 1,792 줄을 atom/bond/cell 별로 분할** — 가장 무거운 작업.
4. Phase 3.3 의 *EventBus::onAtomsChanged + onStructureAdded* 의 *첫 구독자* 도착.
5. Phase 3.3 의 atom 추가 → 시각화 (atom_renderer) 가 본 단계에서 비로소 완성.
6. 메뉴: `Edit / Atoms / Bonds / Cell`.

Phase 3.4 세부계획서는 `phase3_4_edit_atoms_bonds_cell.md` 에 별도 작성.

---

## 11. 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.3) + **§6.0 공통 지침 (UI 1:1 보존)**
- 선행 평가서: [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md)
- 선행 계획서: [`./phase3_2_data_charge_density.md`](./phase3_2_data_charge_density.md)
- Phase 2 (인프라): [`./phase2_core_skeleton.md`](./phase2_core_skeleton.md) — `core/data/element_database`
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §5 Build
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
