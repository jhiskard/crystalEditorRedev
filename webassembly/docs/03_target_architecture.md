# 03. 새 아키텍처 — 메뉴-정렬 버티컬 슬라이스

## 1. 5대 설계 원칙

### P1. 메뉴-우선 버티컬 슬라이스 (메뉴 = 폴더)

`features/` 1단계 하위 폴더 이름은 메뉴바 항목 이름과 **반드시** 1:1 일치한다.

```
File       → features/file/
Edit       → features/edit/
Build      → features/build/
Measurement→ features/measurement/
Data       → features/data/
Utilities  → features/utilities/
```

> **목표 상태**: "Edit/Bonds 의 X 를 고치고 싶다" → 개발자는 `features/edit/bonds/` 만 열면 된다.

### P2. 슬라이스 내부에는 레이어를 유지한다

각 feature 폴더 안에는 atoms 가 시도했던 3-layer 를 **축소 복제** 한다.

```
features/<feature>/<sub>/
├─ <name>_domain.cpp/h         ← 순수 데이터/로직 (VTK 의존 X)
├─ <name>_renderer.cpp/h       ← VTK 액터/렌더링 (VTK 의존 O)
├─ <name>_ui.cpp/h             ← ImGui 위젯/창
└─ <name>_menu.cpp/h           ← 메뉴 항목 그리기 + 라우터 진입
```

> **이유**: 메뉴 단위로만 쪼개면 한 폴더가 다시 god object 가 된다 (이미 한 번 본 실패).
> Layer 를 유지해야 도메인 단위 테스트와 렌더 교체가 가능하다.

### P3. 공유는 `core/`. 단, 둘 이상이 쓸 때만 승격한다.

**판정 기준**: 어떤 모듈을 `core/` 로 올리려면 **현재 시점에 2개 이상의 feature 가 그것을 import** 해야 한다.

| 분류 | core/ 로 올릴 것 | feature 안에 둘 것 |
|---|---|---|
| 자료구조 | `lcrs_tree`, `colormap` (Model Tree·Charge Density 둘 다 사용) | feature 전용 자료구조 |
| 데이터 | `element_database` (Edit·Build 둘 다 사용) | feature 전용 enum |
| VTK | `vtk_viewer`, `mouse_interactor`, `batch_update_system` | atoms 전용 렌더러 → feature 안에 |
| IO | 파일 다이얼로그 브리지, 포맷 레지스트리, 모든 파서 | (단일 사용 파서가 등장하면 feature 로 내려보냄) |

### P4. God Object 해체 → SceneState + 작은 Manager

`atoms_template` 가 가지고 있던 **공유 상태** (`currentStructureId`, `m_Structures`, `m_*Visible`, hover/selection, measurement list 등) 은 `core/scene/SceneState` 로 분리한다.

```
core/scene/scene_state.h  ───  데이터의 단일 소유
features/.../<manager>.h  ───  로직 (SceneState& 를 DI 로 받음)
features/.../<ui>.h       ───  표현 (Manager 를 통해 SceneState 를 읽고 씀)
```

> Singleton 한 점이 모든 것을 알지 않고, **데이터 ↔ 로직 ↔ 표현** 이 분리된 상태.

### P5. 메뉴 → 피처는 한 방향(Request 디스패치)

피처는 `app` 을 모르고, 다른 피처도 모른다. 통신은 다음 두 방향만 허용된다.

```
app/menu_router.cpp ─── (Request) ──▶ features::<feature>::HandleRequest(...)

features::<f1>  ─ writes ──▶ core/scene/SceneState  ◀── reads ─ features::<f2>
features::<f1>  ─ emits ───▶ core/events            ◀── subscribes ─ features::<f2>
```

| 호출 방향 | 허용? | 비고 |
|---|---|---|
| app → feature | ○ | MenuRouter 가 Request 전달 |
| feature → core | ○ | Scene/Vtk/IO/UI 자유 사용 |
| feature → core/events broadcast | ○ | 옵저버 |
| feature ↔ feature **직접** | × | 우회 경로(SceneState 또는 events) 만 사용 |
| core → feature | × | core 는 feature 를 모른다 |

## 2. 디렉터리 트리 (Top-Level)

```
webassembly/src/
│
├─ main.cpp                 ─── Emscripten main (얇음)
├─ bind_function.cpp        ─── Embind 바인딩 (얇음)
│
├─ app/                     ─── 애플리케이션 셸
│   ├─ app.cpp / app.h
│   ├─ menu_router.cpp / menu_router.h
│   ├─ menu_request.h            (4종 Request 타입: OpenWindow / EnterMode / InvokeAction / TogglePref)
│   ├─ window_flags.h            (m_bShow* 전체를 한 구조체로)
│   ├─ about.cpp / about.h
│   ├─ settings.cpp / settings.h
│   └─ layout_manager.cpp / layout_manager.h
│
├─ core/                    ─── 둘 이상이 쓰는 기반
│   ├─ vtk/
│   │   ├─ vtk_viewer.cpp/h
│   │   ├─ mouse_interactor.cpp/h
│   │   ├─ batch_update_system.cpp/h
│   │   └─ vtk_types.h           (smart-ptr alias 등)
│   ├─ io/
│   │   ├─ file_dialog.cpp/h     (Emscripten 파일 다이얼로그 + 청크 전송)
│   │   ├─ format_registry.cpp/h (확장자 → Parser 전략)
│   │   ├─ xsf_parser.cpp/h
│   │   ├─ chgcar_parser.cpp/h
│   │   ├─ rho_parser.cpp/h
│   │   └─ unv_reader.cpp/h
│   ├─ data/
│   │   ├─ element_database.cpp/h
│   │   ├─ color.h
│   │   ├─ colormap.cpp/h
│   │   ├─ lcrs_tree.cpp/h
│   │   └─ string_utils.cpp/h
│   ├─ scene/                ★ 신규
│   │   ├─ scene_state.cpp/h
│   │   ├─ structure_registry.cpp/h
│   │   ├─ selection.cpp/h
│   │   ├─ hover.cpp/h
│   │   └─ events.h          (이벤트 버스 — 옵저버)
│   ├─ render/
│   │   ├─ image.cpp/h
│   │   ├─ texture.cpp/h
│   │   └─ font_manager.cpp/h
│   └─ ui/
│       ├─ widgets.cpp/h     (custom_ui 전임자 — IconButton, AddTooltip 등)
│       ├─ ui_color_utils.h
│       └─ icons/            (icon/* 전부)
│
├─ features/                ─── 메뉴 = 폴더
│   │
│   ├─ file/                  (메뉴 #2)
│   │   ├─ file_menu.cpp/h
│   │   ├─ structure_import.cpp/h
│   │   └─ recent_files.cpp/h
│   │
│   ├─ edit/                  (메뉴 #3)
│   │   ├─ edit_menu.cpp/h
│   │   ├─ atoms/
│   │   │   ├─ atom_manager.cpp/h
│   │   │   ├─ atom_renderer.cpp/h
│   │   │   ├─ atom_editor_ui.cpp/h
│   │   │   └─ surrounding_atom_manager.cpp/h
│   │   ├─ bonds/
│   │   │   ├─ bond_manager.cpp/h
│   │   │   ├─ bond_renderer.cpp/h
│   │   │   └─ bond_ui.cpp/h
│   │   └─ cell/
│   │       ├─ cell_manager.cpp/h
│   │       ├─ cell_renderer.cpp/h
│   │       └─ cell_info_ui.cpp/h
│   │
│   ├─ build/                 (메뉴 #4)
│   │   ├─ build_menu.cpp/h
│   │   ├─ periodic_table/
│   │   │   ├─ periodic_table.cpp/h
│   │   │   └─ periodic_table_ui.cpp/h
│   │   └─ bravais/
│   │       ├─ bravais_lattice.cpp/h     (crystal_system + crystal_structure 통합)
│   │       └─ bravais_lattice_ui.cpp/h
│   │
│   ├─ measurement/           (메뉴 #5)
│   │   ├─ measurement_menu.cpp/h
│   │   ├─ measurement_mode.cpp/h        (모드 상태머신)
│   │   ├─ distance.cpp/h
│   │   ├─ angle.cpp/h
│   │   ├─ dihedral.cpp/h
│   │   ├─ center.cpp/h                  (Geometric Center + Center of Mass)
│   │   ├─ measurement_store.cpp/h       (구조별 측정 리스트 + visibility)
│   │   └─ measurement_overlay_ui.cpp/h  (화면 오버레이 + 리스트 UI)
│   │
│   ├─ data/                  (메뉴 #6)
│   │   ├─ data_menu.cpp/h
│   │   ├─ charge_density/
│   │   │   ├─ charge_density.cpp/h          (도메인 — 볼륨 그리드)
│   │   │   ├─ isosurface_renderer.cpp/h     (Isosurface / Surface)
│   │   │   ├─ volume_renderer.cpp/h         (Volumetric)
│   │   │   └─ charge_density_ui.cpp/h
│   │   └─ slice/
│   │       ├─ slice_renderer.cpp/h          (Plane)
│   │       └─ slice_ui.cpp/h
│   │
│   ├─ utilities/             (메뉴 #7)
│   │   └─ brillouin_zone/
│   │       ├─ bz_plot.cpp/h
│   │       ├─ bz_plot_layer.cpp/h
│   │       ├─ special_points.h
│   │       └─ bz_plot_ui.cpp/h
│   │
│   ├─ viewer/                (메뉴 #9 Viewer + 툴바)
│   │   ├─ viewer_panel.cpp/h            (VTK 캔버스 ImGui window)
│   │   └─ toolbar.cpp/h                 (Mesh Display / Projection / Reset / Cell Align / …)
│   │
│   ├─ model_tree/            (메뉴 #9 Model Tree)
│   │   ├─ model_tree.cpp/h
│   │   └─ model_tree_actions.cpp/h
│   │
│   └─ mesh/                  (mesh 파이프라인)
│       ├─ mesh.cpp/h
│       ├─ mesh_manager.cpp/h
│       ├─ mesh_group.cpp/h
│       ├─ mesh_group_detail.cpp/h
│       ├─ mesh_detail.cpp/h
│       └─ mesh_ui.cpp/h
│
├─ legacy/                  ─── 현재 코드 동결본 (빌드에서 제외)
│   └─ ... (현 webassembly/src 전체가 그대로 들어옴)
│
└─ dev/                     ─── 디버그 전용 (선택, test_window 등)
```

## 3. 피처 컨벤션 (강제 베이스 클래스 X, 컨벤션 O)

각 feature 는 같은 5 개 진입점을 노출한다 (없으면 비워둠).

```cpp
// features/<feature>/<feature>_menu.h
namespace features::<feature> {

/**
 * @brief 메뉴바에 이 피처의 항목을 그린다.
 *        클릭 시 HandleRequest 로 연결된다.
 */
void DrawMenu(const app::MenuContext& ctx);

/**
 * @brief 라우터로부터 받은 요청을 처리한다.
 */
void HandleRequest(const Request& r);

/**
 * @brief Windows 메뉴에서 토글하는 독립 창을 그린다.
 */
void RenderWindows(app::WindowFlags& flags);

/**
 * @brief 1 프레임마다 한 번 호출되는 업데이트 훅 (선택).
 */
void Tick(float dt);

/**
 * @brief 피처가 보유한 리소스를 정리한다 (선택).
 */
void Shutdown();

}
```

## 4. 핵심 신규 컴포넌트

### 4.1 `core/scene/SceneState`

```cpp
/**
 * @file core/scene/scene_state.h
 * @brief 모든 피처가 공유하는 장면 상태. AtomsTemplate 의 god 부분을 추출.
 */
struct SceneState {
    int32_t                       currentStructureId = -1;
    StructureRegistry             structures;     // id → name/visible/hasCell
    SelectionSet                  selection;
    HoverInfo                     hover;
    VisibilityMaskCollection      visibility;     // atomId/bondId/labelId/cellId

    // 옵저버 — feature 간 broadcast 통로
    events::Bus<StructureAddedEvent>   onStructureAdded;
    events::Bus<StructureRemovedEvent> onStructureRemoved;
    events::Bus<AtomsChangedEvent>     onAtomsChanged;
    events::Bus<BondsChangedEvent>     onBondsChanged;
    events::Bus<CellChangedEvent>      onCellChanged;
    events::Bus<SelectionChangedEvent> onSelectionChanged;
};
```

각 피처 매니저는 `MeasurementStore(SceneState&)`, `AtomManager(SceneState&)` 처럼 DI 로 받는다.

### 4.2 `app/menu_router`

```cpp
/**
 * @file app/menu_router.h
 * @brief 메뉴바 그리기 + 클릭 → 피처 디스패치.
 *        4 종 Request 만 다룬다 (OpenWindow / EnterMode / InvokeAction / TogglePref).
 */
class MenuRouter {
public:
    explicit MenuRouter(app::AppContext& ctx);
    void DrawMenuBar();
private:
    app::AppContext& ctx_;
};
```

`DrawMenuBar` 는 다음과 같이 단순해진다.

```cpp
void MenuRouter::DrawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        app::about::DrawMenu(ctx_);
        features::file::DrawMenu(ctx_);
        features::edit::DrawMenu(ctx_);
        features::build::DrawMenu(ctx_);
        features::measurement::DrawMenu(ctx_);
        features::data::DrawMenu(ctx_);
        features::utilities::DrawMenu(ctx_);
        app::settings::DrawMenu(ctx_);
        app::layout_manager::DrawWindowsMenu(ctx_);
        app::layout_manager::DrawLayoutButtons(ctx_);
        ImGui::EndMainMenuBar();
    }
}
```

> 결과: 현 `app.cpp::renderDockSpaceAndMenu` 의 ~230 줄짜리 메뉴 블록이 위 13 줄로 줄어든다.

### 4.3 `app::WindowFlags`

`m_bShow*` 류 18+ 개 boolean 을 한 구조체로 모은다.

```cpp
/**
 * @file app/window_flags.h
 * @brief Windows 메뉴 + 메뉴 클릭이 토글하는 모든 윈도우 가시성 플래그.
 */
struct WindowFlags {
    bool viewer                = true;
    bool modelTree             = true;
    bool createdAtoms          = false;
    bool bondsManagement       = false;
    bool cellInformation       = false;
    bool periodicTable         = false;
    bool crystalTemplates      = false;
    bool brillouinZonePlot     = false;
    bool chargeDensityViewer   = false;
    bool sliceViewer           = false;
    bool fullDockspace         = false;   // DEBUG
    bool fontIcons             = false;   // SHOW_FONT_ICONS
};
```

피처는 자신이 소유한 윈도우의 flag 만 읽고 쓴다.

## 5. 트레이드오프

| 대안 | 채택하지 않은 이유 |
|---|---|
| Layer-only(현 atoms 와 동일) 유지 | 메뉴와의 정합이 안 만들어짐. 이미 한 번 시도해 본 결론 |
| Menu-only(레이어 버림) | 한 feature 가 다시 god object 화. atoms 의 비대화를 그대로 반복 |
| Command/Undo-Redo 패턴 도입 | 범위가 너무 크고 현재 UX 요구가 없음. 향후 별도 단계에서 검토 |
| 파서 plugin 동적 로딩(.so) | WASM 환경에서 얻는 이득 없음. 정적 `format_registry` 로 충분 |
| atoms 와 mesh 를 단일 "Structure" 추상으로 통합 | 데이터 모델이 너무 다름 (격자 vs 그리드셀). 두 피처 병치가 현실적 |

## 6. "메뉴 트리를 그대로 따라간다" 의 의미 (예시)

**예시 1 — Data > Volumetric 의 isosurface 알고리즘 변경**
- 지금: `atoms/infrastructure/charge_density_renderer.cpp` + `atoms/ui/charge_density_ui.cpp` + `atoms/domain/charge_density.cpp` + `atoms_template.cpp` 4 곳.
- 새 구조: `features/data/charge_density/volume_renderer.cpp` 한 파일. 끝.

**예시 2 — Measurement > Dihedral 에 새 표시 옵션 추가**
- 지금: `atoms_template.cpp` 측정 관련 수백 줄 + `atoms/ui/atoms_template_main_window_ui.cpp` + `atoms/infrastructure/vtk_renderer.cpp` 산개.
- 새 구조: `features/measurement/dihedral.cpp` + `features/measurement/measurement_overlay_ui.cpp`.

**예시 3 — 새 파일 포맷(CIF) 지원 추가**
- 지금: `file_loader.cpp` + `atoms/infrastructure/file_io_manager.cpp` + `atoms_template.cpp` 의 Load 래퍼.
- 새 구조: `core/io/cif_parser.*` 한 파일 + `core/io/format_registry` 에 한 줄 등록. `features/file/` 은 변경 불필요.
