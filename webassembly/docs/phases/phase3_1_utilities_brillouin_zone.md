# Phase 3.1 — Utilities/Brillouin Zone 이식 (First Feature) 세부계획서

> 상위 문서:    [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.1)
> 선행 문서:    [`./phase2_core_skeleton.md`](./phase2_core_skeleton.md)
> 선행 평가서:  [`./phase2_evaluation_2026-04-28.md`](./phase2_evaluation_2026-04-28.md)
> 메뉴 매핑:    [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §8 Utilities
> 작성일:      2026-04-28
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR:     1 개
> 예상 소요:   3~4 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-04-28 | 초안 작성 (Phase 2 통합 평가서 §6 권장 다음 단계 항목을 본 문서로 확장. 첫 feature 이식 단계) |

---

## 0. 한 줄 요약

> Phase 2 가 만들어 둔 `core/{scene, vtk, ui, ...}` 위에, **메뉴 → 폴더 = 1:1 매핑** 의 첫 시범 사례인 **Utilities / Brillouin Zone** 을 `features/utilities/brillouin_zone/` 으로 이식한다. 메뉴 클릭 → BZ Plot 윈도우 표시 → 원자 격자 입력 시 BZ 다이어그램 렌더 까지의 *수직 슬라이스* 를 처음으로 완성한다.
> Phase 0 (rename) → Phase 1 (빈 셸) → Phase 2 (인프라 골격) → **Phase 3.1 (첫 feature)** 의 흐름에서, 본 단계는 *"인프라가 실제로 동작하는지"* 를 처음으로 검증하는 시험대다.

> 의존이 가장 적은 BZ 가 첫 feature 로 선택된 이유: legacy 의 `atoms/domain/bz_plot.{cpp,h}` 가 `<vector>`, `<array>`, `<map>` 등 STL 외에는 외부 호출이 거의 없는 *상대적 자기완결* 모듈이고, `core/scene/SceneState::currentStructureId` 와 cell 정보 1 종만 외부에서 받으면 동작 가능하기 때문.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `features/utilities/brillouin_zone/` 폴더 신설 + 7 파일 이식 (special_points, bz_plot, bz_plot_layer, bz_plot_ui + 신규 bz_menu). (b) 메뉴 `Utilities / Brillouin Zone` 클릭 시 BZ Plot 윈도우 표시. (c) 윈도우 안에서 path/npoints 입력 → "Show BZ Plot" 클릭 → VTK 렌더러에 BZ 다이어그램 actor 추가. (d) 윈도우의 `[X]` 닫기로 비활성화. (e) `npm run build-wasm:debug` + `:release` 통과 + 런타임에서 BZ 메뉴 동작 |
| **비목표** | 다른 메뉴 항목(File/Edit/Build/Measurement/Data) 부활, `menu_router` 정식 도입 (Phase 4), `WindowFlags` 통합 (Phase 4), unit cell 입력 기능 부활 (Phase 3.4 의 features/edit/cell 가 들어와야 외부에서 cell 주입 가능), 18 항목 회귀 전체 통과 (Phase 3.9 까지 점진), legacy/ 내부 코드 수정 (회색지대 §1.2~§1.3 예외) |

> Phase 3.1 의 미덕: *"검토자가 git diff 를 보고 `features/utilities/brillouin_zone/` 신규 7 파일 + `app/app.cpp` 에 메뉴 hook 1 줄 + `CMakeLists.txt` 의 source 추가 외에 의심할 게 없다"*. 다른 features 폴더는 그대로 비어있다.

### 1.1 Phase 3.1 의 *첫 feature* 로서의 의의

Phase 3.1 은 단순히 BZ 한 기능을 살리는 것이 아니라, **앞으로 모든 feature 에 적용될 패턴의 시범 사례** 다. 본 단계에서 확립할 패턴:

| 패턴 | 적용 |
|---|---|
| `features/<menu>/<sub>/{<file>.cpp, <file>.h, <feature>_menu.cpp, <feature>_menu.h}` | 03_target_architecture.md §3 의 표준 디렉터리 트리 |
| 4 종 layer 분리 (domain / renderer / ui / menu) | bz_plot (도메인) / bz_plot_layer (렌더) / bz_plot_ui (UI) / bz_menu (진입점) |
| `core/scene/SceneState&` DI | 모든 feature 의 매니저 생성자 인자 |
| `core/scene/EventBus` 구독 | feature 간 broadcast 통로 |
| legacy include 0 | features/ 의 어떤 파일도 `#include "../legacy/..."` 사용 금지 |
| 임시 메뉴 hook → Phase 4 의 menu_router 로 흡수 | app/app.cpp 의 BeginMenuBar 안에 직접 features::X::DrawMenu(ctx) 호출 |

> Phase 3.2 (data/charge_density), 3.3 (build/...), 3.4 (edit/...) 등이 본 패턴을 *복사* 해서 진행한다. 따라서 Phase 3.1 의 코드 품질이 이후 8 개 sub-phase 의 품질에 직결된다.

### 1.2 회색지대 정책 인계 (Phase 0 §1.1 + Phase 1 §1.2)

본 계획서에서도 두 회색지대 정책을 그대로 적용한다.

| 회색지대 출처 | 적용 범위 (Phase 3.1) |
|---|---|
| Phase 0 §1.1 — typo build-fix | legacy/ 내부의 1~5 줄 typo. 본 단계에서 발견 시 별도 PR 또는 분리 commit |
| Phase 1 §1.2 — 빌드 호환성 shim | 새 트리 측의 얇은 redirect 파일. 본 단계에서 BZ 가 legacy 심볼을 부르는 *절대* 안 되지만, 만약 발견되면 회색지대로 처리 |

### 1.3 신규 회색지대 — Phase 4 도착 전 임시 메뉴 hook (참고)

Phase 3.1 에서 BZ 메뉴를 사용자가 클릭할 수 있게 하려면 어딘가에서 메뉴 항목을 그려야 한다. 정식 위치는 `app/menu_router.cpp` 이지만 **Phase 4 까지는 그게 없다**.

따라서 Phase 3.1 은 **임시로 `app/app.cpp::renderDockSpace()` 안의 `BeginMenuBar()` 블록에 features::utilities::bz::DrawMenu(ctx) 한 줄을 추가** 한다. 이 호출은 Phase 4 에서 `MenuRouter` 가 도착하면 그곳으로 옮겨진다. 본 PR 의 app.cpp 변경은 의도된 임시 조치이며, 회색지대 §1.4 로 추가 명문화될 수 있다.

```cpp
// app/app.cpp (Phase 3.1 임시 추가)
if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Crystal Viewer (rebuilding...)")) {
        ImGui::Text("Phase 3.1: utilities/brillouin_zone");
        ImGui::EndMenu();
    }
    features::utilities::bz::DrawMenu(ctx_);   // ← Phase 3.1 신규
    ImGui::EndMenuBar();
}
```

---

## 2. 전제 — Phase 2 완료 상태

본 계획서는 다음이 모두 충족된 상태에서 시작한다.

- [ ] `webassembly/src/{app, core/{scene, io, data, vtk, render, ui}, features (빈), legacy, main.cpp, bind_function.cpp}` 트리 확립
- [ ] `core/scene/SceneState`, `EventBus`, `StructureRegistry`, `Selection`, `Hover` 인터페이스 작성됨
- [ ] `core/vtk/{vtk_viewer, mouse_interactor, batch_update_system}` 작성됨
- [ ] `core/ui/{widgets, ui_color_utils, icons/}` 작성됨
- [ ] `npm run build-wasm:debug` + `:release` 둘 다 exit 0
- [ ] 빈 dockspace + 콘솔 에러 0 (사용자 보고)
- [ ] Phase 2 commit(s) 가 머지됨

위 통과 여부는 [`phase2_evaluation_2026-04-28.md`](./phase2_evaluation_2026-04-28.md) §4.3 의 정리 항목 (특히 4.3.D commit) 통과로 확인.

---

## 3. legacy 참조 인벤토리

본 단계에서 *읽기 전용 참조* 로만 사용하는 legacy 모듈 (총 7 파일, 약 2,378 줄).

| legacy 파일 | 라인 수 | 역할 | 새 트리 행선지 |
|---|---|---|---|
| `legacy/atoms/domain/bz_plot.h` | 216 | `BZFacet`, `BZVerticesResult`, `BZCalculator` 클래스 선언 (Voro++ 기반 BZ 계산) | `features/utilities/brillouin_zone/bz_plot.h` |
| `legacy/atoms/domain/bz_plot.cpp` | 533 | BZCalculator 구현 (역격자 → Voronoi → BZ vertices) | `features/utilities/brillouin_zone/bz_plot.cpp` |
| `legacy/atoms/domain/special_points.h` | 615 | High-symmetry k-point 데이터 테이블 (header-only) | `features/utilities/brillouin_zone/special_points.h` |
| `legacy/atoms/infrastructure/bz_plot_layer.h` | 153 | `BZPlotLayer::ActorGroup` 등 VTK 레이어 시스템 | `features/utilities/brillouin_zone/bz_plot_layer.h` |
| `legacy/atoms/infrastructure/bz_plot_layer.cpp` | 181 | 레이어 구현 | `features/utilities/brillouin_zone/bz_plot_layer.cpp` |
| `legacy/atoms/ui/bz_plot_ui.h` | 52 | `BZPlotUI` 클래스 — ImGui 윈도우 | `features/utilities/brillouin_zone/bz_plot_ui.h` |
| `legacy/atoms/ui/bz_plot_ui.cpp` | 628 | 윈도우 ImGui 렌더 + 사용자 입력 처리 | `features/utilities/brillouin_zone/bz_plot_ui.cpp` |
| **(신규)** | — | feature 진입점 (DrawMenu, HandleRequest, RenderWindows) | `features/utilities/brillouin_zone/bz_menu.{cpp,h}` |

### 3.1 외부 의존 사전 분석 (Phase 1 의 font_manager 교훈 적용)

각 legacy 파일이 어떤 외부 클래스를 호출하는지 사전에 파악.

| legacy 파일 | 외부 의존 | 새 트리 처리 |
|---|---|---|
| `bz_plot.h` | `struct CellInfo` (forward), `class AtomsTemplate` (forward) | CellInfo 는 본 feature 내부 구조체로 정의 (Phase 3.4 의 cell 이 도착하면 통합). AtomsTemplate forward 는 *제거* — 도메인은 외부 클래스를 모른다 |
| `bz_plot.cpp` | Voro++ (`voro/voro++.hh`) | 그대로 유지. CMakeLists 의 `${VOROPP_LIBRARY}` 는 이미 링크됨 |
| `special_points.h` | (없음 — 순수 데이터) | 변경 없이 이식 |
| `bz_plot_layer.h` | `<vtkSmartPointer>`, `<vtkActor>` | 변경 없이 이식 |
| `bz_plot_layer.cpp` | (구체 호출은 분석 필요 — `core::vtk::VtkViewer::Instance().GetRenderer()->AddActor(...)` 패턴으로 정리 권장) | 호출 갱신 |
| `bz_plot_ui.h` | `class AtomsTemplate` (멤버 `m_parent`) | `AtomsTemplate*` → `core::scene::SceneState&` + 본 feature 의 `BZPlotController*` 로 분리 |
| `bz_plot_ui.cpp` | `m_parent->RenderXxx()` (도메인/렌더 호출) | 본 feature 의 `BZPlotController` 로 분기 |

> **font_manager 교훈** (Phase 1): 자동생성으로 보이는 파일이 실제로는 외부 호출을 가질 수 있다. 본 단계에서는 **모든 legacy 파일을 한 번씩 grep** 하여 외부 의존을 사전에 파악하고 §4 의 각 Step 에서 처리한다.

---

## 4. 작업 절차 (단계별)

### Step 1 — `features/utilities/brillouin_zone/` 폴더 신설

```bash
cd webassembly/src
mkdir -p features/utilities/brillouin_zone
```

> 이 시점에 `features/` 안에 첫 sub-folder 가 생긴다. 다음 sub-phase (3.2~3.9) 도 동일 패턴 (`features/<menu>/<sub>/`) 으로 진행된다.

### Step 2 — `special_points.h` 이식 (header-only)

가장 단순한 단계. legacy 의 615 줄 데이터 테이블을 그대로 가져온다.

```bash
cp webassembly/src/legacy/atoms/domain/special_points.h \
   webassembly/src/features/utilities/brillouin_zone/special_points.h
```

namespace 만 수정:
- 변경 전: `namespace atoms { namespace domain { ... } }`
- 변경 후: `namespace features::utilities::bz { ... }`

검증: `wc -l` 이 615 와 거의 같은지 확인 (namespace 변경으로 ±2 줄).

### Step 3 — `bz_plot.{cpp,h}` 도메인 이식 + 외부 의존 정리

#### 3.1 `bz_plot.h` 작성 (~220 줄)

legacy 를 base 로 하되 다음을 정리:

| 항목 | 변경 |
|---|---|
| namespace | `atoms::domain` → `features::utilities::bz` |
| `class AtomsTemplate` forward | **제거** (도메인은 외부 클래스를 모름) |
| `struct CellInfo` forward | 본 feature 내부에 *경량 struct* 로 정의: `struct CellInfo { std::array<std::array<double,3>,3> matrix; };`. Phase 3.4 가 도착하면 `core/scene/cell_info.h` 로 승격 |
| `BZCalculator::compute(const CellInfo&)` 시그너처 | 그대로 — Voro++ 호출은 내부 구현 |
| Doxygen 주석 | 그대로 (한국어 주석 보존) |

```cpp
/**
 * @file features/utilities/brillouin_zone/bz_plot.h
 * @brief Brillouin Zone 계산 (Voro++ 기반).
 */
#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

namespace features::utilities::bz {

struct CellInfo {
    std::array<std::array<double, 3>, 3> matrix{};
    bool valid = false;
};

struct BZFacet {
    std::vector<std::array<double, 3>> vertices;
    std::array<double, 3> normal = {0, 0, 0};
    int neighborId = -1;
};

struct BZVerticesResult {
    std::vector<BZFacet> facets;
    bool success = false;
    std::string errorMessage;
};

class BZCalculator {
public:
    /// @brief 역격자 벡터로부터 BZ 의 면들을 계산.
    static BZVerticesResult Compute(const CellInfo& cell);
};

} // namespace features::utilities::bz
```

#### 3.2 `bz_plot.cpp` 작성 (~540 줄)

legacy 의 BZCalculator 구현을 그대로 복사 + namespace 만 수정. Voro++ 호출 (`#include <voro++/voro++.hh>`) 은 그대로.

> **검증**: 새 .cpp 안에서 `grep -nE "AtomsTemplate|atoms::domain|atoms::infrastructure"` 결과가 0 이어야 함.

### Step 4 — `bz_plot_layer.{cpp,h}` VTK 렌더링 이식

legacy 코드는 자기완결적인 VTK actor 그룹 매니저. namespace 만 수정.

| 변경 항목 | 처리 |
|---|---|
| namespace | `atoms::infrastructure` → `features::utilities::bz` |
| AddActor 호출 | `core::vtk::VtkViewer::Instance().GetRenderer()->AddActor(...)` 로 갱신 |
| `<vtkSmartPointer>`, `<vtkActor>` include | 그대로 |

```cpp
/**
 * @file features/utilities/brillouin_zone/bz_plot_layer.h
 * @brief BZ Plot 전용 VTK 액터 레이어 시스템.
 */
#pragma once

#include <vector>
#include <vtkActor.h>
#include <vtkSmartPointer.h>

namespace features::utilities::bz {

class BZPlotLayer {
public:
    struct ActorGroup {
        std::vector<vtkSmartPointer<vtkActor>> actors;
        bool visible = true;
        void SetVisibility(bool vis);
        void Clear();
        size_t Count() const { return actors.size(); }
    };

    /// @brief BZ 면 액터 그룹.
    ActorGroup facetActors;

    /// @brief 역격자 벡터 화살표 액터 그룹.
    ActorGroup reciprocalVectorActors;

    /// @brief IBZ 라인 액터 그룹.
    ActorGroup ibzLineActors;

    /// @brief 모든 그룹 청산.
    void ClearAll();

    /// @brief 모든 액터를 viewer 에서 제거.
    void RemoveFromViewer();
};

} // namespace features::utilities::bz
```

### Step 5 — `bz_plot_ui.{cpp,h}` ImGui 윈도우 이식 + DI

가장 복잡한 단계 (628 줄). legacy 의 `BZPlotUI(AtomsTemplate*)` 를 새 트리에서 *2 단계 DI* 로 분해.

#### 5.1 컨트롤러 클래스 신설 — `BZPlotController`

legacy 의 `m_parent->RenderBZPlotXxx()` 호출은 모두 `BZPlotController` 로 분기.

```cpp
/**
 * @file features/utilities/brillouin_zone/bz_plot_controller.h
 * @brief BZ feature 의 도메인/렌더 통합 진입점. UI 가 호출하는 단일 사용자.
 */
#pragma once

#include "bz_plot.h"
#include "bz_plot_layer.h"
#include "../../../core/scene/scene_state.h"

namespace features::utilities::bz {

class BZPlotController {
public:
    explicit BZPlotController(core::scene::SceneState& scene);

    /// @brief BZ 계산 + actor 추가.
    void Show(const std::string& path, int npoints, bool showVectors, bool showLabels);

    /// @brief 모든 BZ actor 제거.
    void Clear();

    /// @brief BZ 가 현재 표시 중인지.
    bool IsShowing() const { return showing_; }

private:
    core::scene::SceneState& scene_;
    BZPlotLayer              layer_;
    BZVerticesResult         lastResult_;
    bool                     showing_ = false;
};

} // namespace features::utilities::bz
```

> 본 컨트롤러의 `Show(...)` 안에서 `BZCalculator::Compute(cellInfo)` 호출 + `BZPlotLayer::facetActors` 채움 + `core::vtk::VtkViewer::Instance().GetRenderer()->AddActor(...)` 까지 한 흐름으로 처리.

#### 5.2 `bz_plot_ui.h` 갱신

```cpp
/**
 * @file features/utilities/brillouin_zone/bz_plot_ui.h
 * @brief "Brillouin Zone Plot" 윈도우 ImGui 렌더.
 */
#pragma once

#include <imgui.h>
#include <string>

namespace features::utilities::bz {

class BZPlotController;  // forward

class BZPlotUI {
public:
    explicit BZPlotUI(BZPlotController& controller);

    /// @brief 메뉴/Windows 메뉴 토글로 활성화되는 윈도우.
    /// @param[in,out] open 윈도우 열림 상태 (포인터). nullptr 가능.
    void Render(bool* open);

private:
    BZPlotController& controller_;
    char        pathInput_[128] = "All";
    int         npointsInput_   = 50;
    bool        showVectors_    = true;
    bool        showLabels_     = true;
    std::string lastErrorMessage_;
};

} // namespace features::utilities::bz
```

#### 5.3 `bz_plot_ui.cpp` 갱신

legacy 의 `BZPlotUI::render()` 의 ImGui 흐름을 그대로 복사 + `m_parent->...` 호출만 `controller_.Show(...)`, `controller_.Clear()` 등으로 치환. 흐름은 변경 없음.

### Step 6 — `bz_menu.{cpp,h}` 메뉴 + HandleRequest (신규)

기존 legacy 에 없던 새 파일 — feature 의 진입점. 03_target_architecture.md §3 의 컨벤션에 따라 5 진입점 정의.

```cpp
/**
 * @file features/utilities/brillouin_zone/bz_menu.h
 * @brief Utilities / Brillouin Zone 의 5 진입점 (DrawMenu/HandleRequest/RenderWindows/Tick/Shutdown).
 */
#pragma once

#include <imgui.h>

namespace app { struct MenuContext; }

namespace features::utilities::bz {

/// @brief Utilities 메뉴 안에 "Brillouin Zone" 항목을 그린다.
void DrawMenu(const app::MenuContext& ctx);

/// @brief Phase 4 menu_router 가 디스패치하는 Request 처리 (현재는 빈 구현).
struct Request { enum Type { Show } type = Show; };
void HandleRequest(const Request& r);

/// @brief Windows 메뉴에서 토글되는 BZ Plot 윈도우 렌더.
void RenderWindows(bool* showBZPlotWindow);

/// @brief 1 프레임마다 호출되는 업데이트 (현재 비어있음).
void Tick(float dt);

/// @brief feature 종료 시 리소스 정리 (BZ actor 제거).
void Shutdown();

/// @brief Singleton-like 컨트롤러 접근자 (Phase 3.1 한정 — Phase 4 의 SceneState DI 로 대체될 수 있음).
class BZPlotController;
BZPlotController& Controller();

} // namespace features::utilities::bz
```

```cpp
/**
 * @file features/utilities/brillouin_zone/bz_menu.cpp
 * @brief Utilities / Brillouin Zone 메뉴 진입점 구현.
 */
#include "bz_menu.h"
#include "bz_plot_controller.h"
#include "bz_plot_ui.h"
#include "../../../core/scene/scene_state.h"
#include "../../../app/app.h"   // app::MenuContext (forward) — 단, app 이 features 를 호출하므로 순환 회피 검토

namespace features::utilities::bz {

namespace {
    static bool g_showWindow = false;   // Phase 4 의 WindowFlags 가 들어오면 그곳으로 이전.
    static BZPlotController* g_controller = nullptr;
    static BZPlotUI*         g_ui = nullptr;
}

void DrawMenu(const app::MenuContext& /*ctx*/) {
    if (ImGui::BeginMenu("  Utilities")) {
        if (ImGui::MenuItem("Brillouin Zone")) {
            g_showWindow = true;
        }
        ImGui::EndMenu();
    }
}

void HandleRequest(const Request& r) {
    if (r.type == Request::Show) {
        g_showWindow = true;
    }
}

void RenderWindows(bool* /*flagsPlaceholder*/) {
    if (!g_showWindow) return;
    if (!g_controller || !g_ui) return;
    g_ui->Render(&g_showWindow);
}

void Tick(float /*dt*/) {
    // Phase 3.1 에서는 유지 작업 없음.
}

void Shutdown() {
    if (g_controller) g_controller->Clear();
}

BZPlotController& Controller() {
    return *g_controller;
}

// 외부 호출자가 SceneState 를 1 회 주입하도록 하는 lazy-init 헬퍼
void InitOnce(core::scene::SceneState& scene) {
    if (g_controller) return;
    static BZPlotController controller{scene};
    static BZPlotUI         ui{controller};
    g_controller = &controller;
    g_ui = &ui;
}

} // namespace features::utilities::bz
```

> **InitOnce(scene)** 패턴: Phase 4 의 menu_router 가 도착하면 모든 feature 의 InitOnce 가 그곳에서 한 번 호출된다. Phase 3.1 한정으로는 `app/app.cpp::Init` 끝에 `features::utilities::bz::InitOnce(SceneState 인스턴스)` 호출.

### Step 7 — `app/app.cpp` 임시 메뉴 hook + RenderWindows 호출

#### 7.1 메뉴바에 Utilities/Brillouin Zone 추가

`app/app.cpp::renderDockSpace()` 의 `BeginMenuBar` 블록에 다음 두 줄 추가:

```cpp
#include "../features/utilities/brillouin_zone/bz_menu.h"   // 새 include

void App::renderDockSpace() {
    ...
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Crystal Viewer (rebuilding...)")) {
            ImGui::Text("Phase 3.1: utilities/brillouin_zone 활성");
            ImGui::EndMenu();
        }
        features::utilities::bz::DrawMenu(/*ctx*/{});  // ← 신규 (임시 hook)
        ImGui::EndMenuBar();
    }

    // 윈도우 렌더 (Phase 4 의 WindowFlags 가 도착하면 통합됨)
    features::utilities::bz::RenderWindows(nullptr);  // ← 신규
    ...
}
```

#### 7.2 `App::Init()` 끝에서 InitOnce 호출

```cpp
int App::Init() {
    ...
    // Phase 2 의 SceneState 인스턴스 — 본 단계에서 처음으로 사용
    static core::scene::SceneState scene;

    // 첫 feature InitOnce
    features::utilities::bz::InitOnce(scene);
    ...
}
```

> **app.cpp 가 features 를 직접 import** 하는 것은 03_target_architecture.md §1 의 P5 *"메뉴 → 피처는 한 방향(Request 디스패치)"* 와 충돌해 보일 수 있다. 본 단계는 *menu_router 가 아직 없는 상태의 임시 직접 호출* 로, Phase 4 에서 `app::MenuRouter::DrawMenuBar()` 안으로 흡수된다.

### Step 8 — `CMakeLists.txt` 갱신

`SOURCES_FEATURES` 에 본 feature 의 8 파일 추가.

```cmake
set(SOURCES_FEATURES
    # Phase 3.1: Utilities / Brillouin Zone
    webassembly/src/features/utilities/brillouin_zone/bz_plot.cpp
    webassembly/src/features/utilities/brillouin_zone/bz_plot.h
    webassembly/src/features/utilities/brillouin_zone/special_points.h
    webassembly/src/features/utilities/brillouin_zone/bz_plot_layer.cpp
    webassembly/src/features/utilities/brillouin_zone/bz_plot_layer.h
    webassembly/src/features/utilities/brillouin_zone/bz_plot_controller.cpp
    webassembly/src/features/utilities/brillouin_zone/bz_plot_controller.h
    webassembly/src/features/utilities/brillouin_zone/bz_plot_ui.cpp
    webassembly/src/features/utilities/brillouin_zone/bz_plot_ui.h
    webassembly/src/features/utilities/brillouin_zone/bz_menu.cpp
    webassembly/src/features/utilities/brillouin_zone/bz_menu.h
)

add_executable(${PROJECT_NAME}
    webassembly/src/main.cpp
    webassembly/src/bind_function.cpp
    ${SOURCES_APP}
    ${SOURCES_CORE}
    ${SOURCES_FEATURES}
)
```

> Voro++ 라이브러리는 이미 `target_link_libraries` 의 `${VOROPP_LIBRARY}` 로 링크되어 있다. 별도 추가 불필요.

### Step 9 — 정적 검증

```bash
cd webassembly/src

# 본 feature 안의 legacy 호출 0 확인
grep -rnE "AtomsTemplate::|atoms::domain::|atoms::infrastructure::|atoms::ui::" \
  features/utilities/brillouin_zone/ | grep -v "//" || echo "OK"

# 본 feature 안의 #include "../legacy/" 0 확인
grep -rnE '#include\s+"(\.\./)*legacy/' features/utilities/brillouin_zone/ \
  | grep -v "//" || echo "OK"

# 본 feature 의 namespace 일관성
grep -rnE "^namespace features::utilities::bz" features/utilities/brillouin_zone/

# CMake 안전벨트 유지 확인
grep -a "Phase 1 violation" ../../CMakeLists.txt
```

### Step 10 — 빌드 + 런타임 검증 (Windows 측)

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release
```

기대:
- emcc 가 약 8 신규 .cpp 파일 컴파일 (Phase 2 의 ~24 + 본 8 = 32).
- Voro++ 링크 통과.
- `legacy/` 미컴파일 (안전벨트 유효).

```powershell
npm run dev
# 브라우저 → http://localhost:3000/workbench
```

기대:
- 빈 dockspace + placeholder 메뉴 (Phase 1/2 와 동일).
- **메뉴바에 `Utilities` 메뉴 추가됨 → `Brillouin Zone` 항목 클릭 → BZ Plot 윈도우 표시**.
- 윈도우 안에서 path/npoints 입력 가능.
- "Show BZ Plot" 클릭 시 (cell info 가 아직 외부에서 주입되지 않으므로) 에러 메시지 표시 또는 fallback. — Phase 3.4 가 도착해 cell info 를 SceneState 에 채우면 비로소 BZ 다이어그램이 그려진다.

> Phase 3.1 의 *최소 통과 기준* 은 *"메뉴 → 윈도우 표시"* 까지. BZ 다이어그램 실제 렌더는 cell 입력이 가능한 시점(Phase 3.4 후) 에 비로소 자체 검증 가능.

### Step 11 — 커밋 & PR

```powershell
git add -A
git status --short
git commit -m "Phase 3.1: features/utilities/brillouin_zone — first feature migration

- New folder: webassembly/src/features/utilities/brillouin_zone/
- 7 ported files (special_points.h, bz_plot, bz_plot_layer, bz_plot_ui)
  + 2 new files (bz_plot_controller, bz_menu).
- legacy includes 0; namespace = features::utilities::bz.
- core/scene::SceneState DI via BZPlotController(scene).
- Temporary menu hook in app/app.cpp::renderDockSpace
  (will be absorbed into Phase 4 menu_router).
- CMakeLists.txt: SOURCES_FEATURES extended.

Runtime: Utilities / Brillouin Zone menu opens BZ Plot window.
BZ diagram rendering requires cell info, which arrives in Phase 3.4
(features/edit/cell). Until then, the window opens but Show button
shows an error message.

Reference:
  - webassembly/docs/05_redevelopment_plan.md (Phase 3.1)
  - webassembly/docs/phases/phase3_1_utilities_brillouin_zone.md
"
```

PR 본문 템플릿:

```markdown
## What
Phase 3.1 — first feature migration: Utilities / Brillouin Zone.

## Why
See webassembly/docs/05_redevelopment_plan.md (Phase 3.1) and
webassembly/docs/phases/phase3_1_utilities_brillouin_zone.md.

## Scope
- New folder: features/utilities/brillouin_zone/.
- 9 files (7 ported from legacy + 2 new entry points).
- Temporary menu hook in app/app.cpp (to be migrated to Phase 4 menu_router).
- CMakeLists.txt: source list extended.

## What you will see at runtime
- Empty dockspace (Phase 1/2 unchanged).
- New "Utilities" menu in the menu bar.
- Click "Utilities → Brillouin Zone" → BZ Plot window opens.
- The window allows path/npoints input. The "Show BZ Plot" button
  shows an error message because cell info is not yet wired (Phase 3.4
  will provide it).

## Verification
- [x] npm run build-wasm:debug 통과
- [x] npm run build-wasm:release 통과
- [x] features/utilities/brillouin_zone/ 안에서 legacy 호출 0
- [x] namespace = features::utilities::bz 일관성
- [x] BZ Plot 윈도우 열기/닫기 동작
```

---

## 5. 검증 매트릭스

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `features/utilities/brillouin_zone/` 폴더 존재 | `ls webassembly/src/features/utilities/` | `brillouin_zone` | 정적 |
| 2 | 본 폴더의 파일 수 | `ls features/utilities/brillouin_zone/` | 11 파일 (.cpp 5 + .h 6) | 정적 |
| 3 | `special_points.h` 라인 수 | `wc -l features/utilities/brillouin_zone/special_points.h` | ≈ 615 | 정적 |
| 4 | namespace 일관성 | `grep -rnE "^namespace features::utilities::bz" features/utilities/brillouin_zone/ \| wc -l` | 모든 .cpp/.h 가 namespace 시작 | 정적 |
| 5 | legacy 호출 0 | `grep -rnE "AtomsTemplate::\|atoms::domain::\|atoms::infrastructure::\|atoms::ui::" features/utilities/brillouin_zone/` (주석 제외) | 0 hit | 정적 |
| 6 | `#include "../legacy/"` 0 | `grep -rnE '#include\s+"(\.\./)*legacy/' features/utilities/brillouin_zone/` | 0 hit | 정적 |
| 7 | `app/app.cpp` 변경 — features 호출 추가 | `grep -n "features::utilities::bz" app/app.cpp` | 2 hit (DrawMenu + RenderWindows) + InitOnce | 정적 |
| 8 | CMakeLists.txt 의 SOURCES_FEATURES 확장 | `grep -c "brillouin_zone" CMakeLists.txt` | 11 hit | 정적 |
| 9 | CMake 안전벨트 유지 | `grep -a "Phase 1 violation" CMakeLists.txt` | 1 hit | 정적 |
| 10 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 11 | Release 빌드 | `npm run build-wasm:release` | exit 0 | 동적 |
| 12 | 빈 dockspace + placeholder 메뉴 | `npm run dev` 후 브라우저 | Phase 2 와 시각적 동일 + Utilities 메뉴 추가 | 동적 |
| 13 | Utilities 메뉴 클릭 → BZ Plot 윈도우 | 메뉴 클릭 | 윈도우 표시 | 동적 |
| 14 | BZ Plot 윈도우의 path/npoints 입력 가능 | 윈도우 안 ImGui 입력 | 입력 가능 | 동적 |
| 15 | BZ Plot 윈도우 닫기 (`[X]`) | 윈도우 닫기 버튼 | 윈도우 사라짐, 다음 클릭 시 다시 열림 | 동적 |
| 16 | "Show BZ Plot" 클릭 → cell info 부재 에러 | 버튼 클릭 | 에러 메시지 표시 (cell info 미주입 — Phase 3.4 까지 정상 동작) | 동적 |
| 17 | 콘솔 에러 0 | DevTools 콘솔 | 0 errors | 동적 |
| 18 | wasm 사이즈 | Phase 2 ± 5 % | 8 신규 파일 + Voro++ 링크로 약간 증가 가능 (10 % 이내 권장) | 동적 |

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | `bz_plot.cpp` 가 의도하지 않은 외부 클래스 호출 (예: `AtomsTemplate::Get*`) 을 가지고 있어 빌드 실패 | link error | Step 9 의 grep sweep 으로 사전 점검. 발견 시 호출 제거 (회색지대 §1.3 우선순위 1) |
| 6.2 | Voro++ 라이브러리 링크 실패 | 빌드 실패 | `${VOROPP_LIBRARY}` 가 `target_link_libraries` 에 이미 있는지 확인. 없으면 추가 |
| 6.3 | `bz_plot_ui.cpp` 의 628 줄 이식 중 ImGui 흐름 누락 | 윈도우 동작 이상 | legacy 의 render() 흐름을 한 줄씩 비교. 변경은 `m_parent->X()` → `controller_.X()` 만 |
| 6.4 | cell info 가 SceneState 에 없어 "Show BZ Plot" 이 동작 안 함 | 사용자 혼란 가능 | Phase 3.1 의 비목표로 명시. 윈도우 안에서 *"Cell info will be wired in Phase 3.4"* 같은 안내 메시지 표시 |
| 6.5 | 임시 메뉴 hook (app.cpp 의 features 직접 호출) 이 *"app → feature → app 순환"* 으로 보일 위험 | 검토자 혼란 | PR 본문에 *"Phase 4 의 menu_router 가 흡수할 임시 조치"* 명시. Phase 4 PR 에서 정식 분리 |
| 6.6 | `bz_menu.cpp` 의 InitOnce 가 SceneState 를 1 번만 받는 패턴이 다른 feature 에 일반화될 수 있는지 | 8 sub-phase 가 각자 InitOnce 를 가지면 app.cpp 에 8 호출이 줄지어 늘어남 | Phase 4 의 menu_router 도입 시 *"모든 feature InitOnce 를 한 곳에서 호출"* 패턴으로 정리 |
| 6.7 | `core/vtk/VtkViewer::GetRenderer()` API 가 BZ 가 기대하는 형태와 다를 가능성 | actor 추가 실패 | Phase 2 의 vtk_viewer.h 에서 `GetRenderer()` signature 확인. 다르면 BZPlotLayer 에서 어댑터 추가 |
| 6.8 | namespace 변경에 따른 special_points.h 의 use site 누락 | 컴파일 에러 | `bz_plot.cpp` 와 `bz_plot_ui.cpp` 가 special_points 를 호출하는 부분의 namespace prefix 일괄 갱신 |
| 6.9 | `.git/index.lock` 환경 제약 (Phase 0 §7.9) | Linux 측 git 작업 실패 | Linux 측은 파일 작성/이동만, git 은 PowerShell 측에서 |
| 6.10 | BZ 의존이 cell info 만이 아니라 "현재 활성 구조 ID" 를 요구할 가능성 | controller 가 SceneState::currentStructureId 를 읽어야 함 | controller 생성자에서 SceneState 받음 (이미 §5.1 에 반영) |
| 6.11 | Phase 3.1 PR 의 검토자가 *"기능이 절반만 동작"* 이라며 거절 | 머지 지연 | PR 본문 §1.1 의 *"첫 feature 의 의의"* 와 §4 Step 10 의 *"BZ 다이어그램 렌더는 Phase 3.4 후 비로소 가능"* 을 명시. 본 PR 은 *수직 슬라이스 패턴 확립* 의 PR |

---

## 7. 롤백 절차

```bash
# 변경 취소 (커밋 전)
git restore --staged .
git restore .
git clean -fd webassembly/src/features/utilities

# 또는 커밋 후라면
git reset --hard HEAD~1
```

원인 진단:

1. 빌드 에러
   - `error: 'AtomsTemplate' has not been declared` → bz_plot.* 안에 legacy 호출 잔여. Step 9 sweep 강화.
   - `undefined reference to 'features::utilities::bz::...'` → CMakeLists 의 SOURCES_FEATURES 누락.
   - Voro++ 미해결 심볼 → `target_link_libraries` 의 VOROPP_LIBRARY 점검.
   - `error: app::MenuContext does not name a type` → bz_menu.h 의 forward 와 app.h 의 정의 불일치. forward 만 사용하고 .cpp 에서 include.
2. 런타임 에러
   - 메뉴는 보이지만 클릭 시 윈도우 안 뜸 → InitOnce 가 호출되지 않음. App::Init 의 호출 위치 점검.
   - 윈도우 뜨지만 "Show BZ Plot" 시 crash → CellInfo::valid 가 false 인데 BZCalculator::Compute 가 가드 누락. 가드 추가.
   - Utilities 메뉴 자체가 안 보임 → app.cpp 의 features::utilities::bz::DrawMenu 호출 누락.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] §2 의 Phase 2 전제 모두 충족 (Phase 2 commit 머지 완료)
- [ ] §5 검증 매트릭스 18 항목 모두 통과
- [ ] features/utilities/brillouin_zone/ 안에서 legacy 호출 0 (Step 9 sweep 결과 첨부)
- [ ] `git diff --stat -- webassembly/src/legacy` 의 모든 항목 0 changed lines
- [ ] PR 본문에 *"BZ 다이어그램은 Phase 3.4 까지 미동작"* 명시
- [ ] PR 본문에 본 문서와 상위 계획서 링크 포함

검토자 — 머지 전:

- [ ] diff 가 (a) features/utilities/brillouin_zone/ 신규 11 파일, (b) app/app.cpp 의 임시 hook 3 곳, (c) CMakeLists.txt 의 source 추가 — 3 가지로만 구성되어 있는가?
- [ ] `legacy/` 파일은 한 글자도 변경되지 않았는가?
- [ ] CI 빌드 (debug + release) 통과
- [ ] 본인 환경에서 Utilities / Brillouin Zone 메뉴 동작
- [ ] 본인 환경에서 BZ Plot 윈도우 열고 닫음 정상

---

## 9. 부록

### 9.1 features/utilities/brillouin_zone/ 의 11 파일 요약

| 파일 | 역할 | 라인 수 (예상) |
|---|---|---|
| `bz_plot.h` | BZFacet, BZVerticesResult, BZCalculator + CellInfo 경량 struct | ~210 |
| `bz_plot.cpp` | BZCalculator 구현 (Voro++) | ~540 |
| `special_points.h` | High-symmetry k-point 데이터 (그대로) | ~615 |
| `bz_plot_layer.h` | BZPlotLayer + ActorGroup | ~140 |
| `bz_plot_layer.cpp` | 레이어 구현 + VtkViewer AddActor | ~190 |
| `bz_plot_controller.h` | BZPlotController (도메인+렌더 통합 진입점) | ~50 |
| `bz_plot_controller.cpp` | controller 구현 | ~120 |
| `bz_plot_ui.h` | BZPlotUI ImGui 윈도우 | ~50 |
| `bz_plot_ui.cpp` | 윈도우 ImGui 흐름 | ~640 |
| `bz_menu.h` | DrawMenu/HandleRequest/RenderWindows/Tick/Shutdown/InitOnce | ~50 |
| `bz_menu.cpp` | 진입점 구현 | ~80 |

총 약 2,690 줄 (legacy 의 2,378 + 신규 controller/menu 약 300).

### 9.2 후속 sub-phase 가 본 패턴을 그대로 적용할 수 있는 모듈 매핑

| sub-phase | 의존 정도 | 첫 시도 권장도 |
|---|---|---|
| 3.1 utilities/brillouin_zone | 가장 적음 (cell info 만) | ★★★ (본 단계) |
| 3.2 data/{charge_density, slice} | 보통 (chgcar 파서 필요) | ★★ |
| 3.3 build/{periodic_table, bravais} | 보통 (element_db + crystal_structure) | ★★ |
| 3.4 edit/{atoms, bonds, cell} | 가장 무거움 (vtk_renderer 분할) | ★ — 마지막 권장 |
| 3.5 measurement | 큼 (mouse_interactor 구독) | ★ |
| 3.6 file | 큼 (format_registry 첫 사용자) | ★★ |
| 3.7 viewer + toolbar | 보통 | ★★ |
| 3.8 model_tree | 큼 (structure_registry 의 첫 외부 사용자) | ★ |
| 3.9 mesh | 보통 | ★★ |

본 패턴 (4 layer + bz_menu) 은 위 8 sub-phase 모두에 그대로 적용된다. 단 3.4 (edit/atoms-bonds-cell) 는 *3 sub-folder* 로 더 분할되며 vtk_renderer 의 1,800 줄을 atom/bond/cell 별 renderer 로 *분할* 하는 추가 작업이 있다.

### 9.3 사후 점검: Phase 3.1 머지 직후 트리

```
webassembly/src/
├─ main.cpp                                       (Phase 1 그대로)
├─ bind_function.cpp                              (Phase 1 그대로)
├─ app/
│  └─ app.cpp / app.h                             (Phase 3.1 임시 hook 3 곳 추가)
├─ core/                                          (Phase 2 그대로)
├─ features/
│  └─ utilities/
│     └─ brillouin_zone/                          ★ 신규 11 파일
│        ├─ bz_plot.{cpp,h}
│        ├─ special_points.h
│        ├─ bz_plot_layer.{cpp,h}
│        ├─ bz_plot_controller.{cpp,h}
│        ├─ bz_plot_ui.{cpp,h}
│        └─ bz_menu.{cpp,h}
└─ legacy/                                        (Phase 0 동결본 그대로)
```

---

## 10. 후속 단계 연결 — Phase 3.2

Phase 3.1 가 머지되면 Phase 3.2 (`features/data/{charge_density, slice}`) 의 입구가 열린다. Phase 3.2 의 첫 작업은 본 Phase 3.1 의 패턴을 *그대로 복제* 하여 다음 모듈을 이식.

1. `features/data/charge_density/` 폴더 신설.
2. `legacy/atoms/{domain,infrastructure,ui}/charge_density*.* + chgcar_parser.*` → 4 layer 분리.
3. **`core/io/format_registry::Register(".chgcar", ...)` 의 첫 호출자가 됨** — Phase 2 의 인프라 검증.
4. `features/data/slice/` 도 동일.
5. 메뉴: `Data / Isosurface / Surface / Volumetric / Plane`.

Phase 3.2 세부계획서는 `phase3_2_data_charge_density.md` 에 별도로 작성한다.

---

## 11. 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.1)
- 선행 평가서: [`./phase2_evaluation_2026-04-28.md`](./phase2_evaluation_2026-04-28.md)
- 선행 계획서: [`./phase2_core_skeleton.md`](./phase2_core_skeleton.md)
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3 (features/ 컨벤션)
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §8 Utilities
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
