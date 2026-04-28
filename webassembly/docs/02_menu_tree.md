# 02. 메인 메뉴 트리 (자동 추출)

> 출처: `webassembly/src/app.cpp` `App::renderDockSpaceAndMenu()` (대략 라인 604–833)
> 추출 시점: 2026-04-27
> 추출 방식: `BeginMenu` / `MenuItem` / `BeginMainMenuBar` 패턴 grep 후 직접 정독

이 트리가 새 디렉터리 구조의 **단일 진실(single source of truth)** 이 된다.

## 1. 메뉴바 — 좌→우 순서

| # | Top-level | 토글/팝업 | 트리거 코드 (현재) | 비고 |
|---|---|---|---|---|
| 1 | `Crystal Viewer` | `About` | `m_bShowAboutPopup = true` | 단순 모달 |
| 2 | `File` | `Open Structure File` | `FileLoader::Instance().RequestOpenStructureImport()` | XSF / XSF(Grid) / vasp CHGCAR 통합 다이얼로그 |
|   |        | `Open Recent` | (disabled) | 미구현 |
| 3 | `Edit` | `Atoms` | `m_bShowCreatedAtomsWindow = true` + `RequestEditorSection(Atoms)` | Created Atoms 윈도우 포커스 |
|   |        | `Bonds` | `m_bShowBondsManagementWindow = true` + `RequestEditorSection(Bonds)` | Bonds Management 윈도우 |
|   |        | `Cell`  | `m_bShowCellInformationWindow = true` + `RequestEditorSection(Cell)` | Cell Information 윈도우 |
| 4 | `Build` | `Add atoms` | `m_bShowPeriodicTableWindow = true` + `RequestBuilderSection(AddAtoms)` | 주기율표 |
|   |         | `Bravais Lattice Templates` | `m_bShowCrystalTemplatesWindow = true` + `RequestBuilderSection(BravaisLatticeTemplates)` | 14 Bravais 템플릿 |
| 5 | `Measurement` | `Distance` | `EnterMeasurementMode(Distance)` | 토글 |
|   |               | `Angle` | `EnterMeasurementMode(Angle)` | 토글 |
|   |               | `Dihedral` | `EnterMeasurementMode(Dihedral)` | 토글 |
|   |               | `Geometric Center` | `EnterMeasurementMode(GeometricCenter)` | 토글 |
|   |               | `Center of Mass` | `EnterMeasurementMode(CenterOfMass)` | 토글 |
| 6 | `Data` | `Isosurface` | `m_bShowChargeDensityViewerWindow = true` + `RequestDataMenu(Isosurface)` | + Model Tree 표시 |
|   |        | `Surface` | `RequestDataMenu(Surface)` | + Model Tree |
|   |        | `Volumetric` | `RequestDataMenu(Volumetric)` | + Model Tree |
|   |        | `Plane` | `m_bShowSliceViewerWindow = true` + `RequestDataMenu(Plane)` | 2D Slice 윈도우 + Model Tree |
| 7 | `Utilities` | `Brillouin Zone` | `m_bShowBrillouinZonePlotWindow = true` + `RequestBuilderSection(BrillouinZone)` | BZ Plot 윈도우 |
| 8 | `Settings` (기어) | `Node Tooltip` (toggle) | `AtomsTemplate::SetNodeInfoEnabled` |  |
|   |                  | `Viewer FPS Overlay` (toggle) | `VtkViewer::SetPerformanceOverlayEnabled` |  |
|   |                  | `Style ▸ Dark / Light / Classic` | `SetColorStyle(...)` | 라디오 |
|   |                  | `Background Color` | `m_bShowBgColorPopup = true` | 팝업 |
|   |                  | `Font Size ▸ small / medium / large` | `SetFontSizePreset(...)` | 라디오 |
|   |                  | `Full Screen` | `EM_ASM(... canvas.requestFullscreen())` | 브라우저 fullscreen |
| 9 | `Windows` (창) | `Viewer` (toggle) | `m_bShowVtkViewer` |  |
|   |               | `Model Tree` (toggle) | `m_bShowModelTree` |  |
|   |               | `Created Atoms` (toggle) | `m_bShowCreatedAtomsWindow` | Edit/Atoms 와 동일 창 |
|   |               | `Bonds Management` (toggle) | `m_bShowBondsManagementWindow` |  |
|   |               | `Cell Information` (toggle) | `m_bShowCellInformationWindow` |  |
|   |               | `Periodic Table` (toggle) | `m_bShowPeriodicTableWindow` |  |
|   |               | `Crystal Templates` (toggle) | `m_bShowCrystalTemplatesWindow` |  |
|   |               | `Brillouin Zone Plot` (toggle) | `m_bShowBrillouinZonePlotWindow` |  |
|   |               | `Charge Density Viewer` (toggle) | `m_bShowChargeDensityViewerWindow` |  |
|   |               | `2D Slice Viewer` (toggle) | `m_bShowSliceViewerWindow` |  |
|   |               | `Full Dockspace` (DEBUG) | `m_bFullDockSpace` | DEBUG_BUILD only |
|   |               | `Show Font Icons` (cond) | `m_bShowFontIcons` | SHOW_FONT_ICONS only |
|10 | (메뉴바 우측 인라인 버튼) | `Layout 1` / `Layout 2` / `Layout 3` / `Reset` | `m_PendingLayoutPreset = ...` | 도크 프리셋 |

## 2. 트리 다이어그램

```
MainMenuBar
├─ Crystal Viewer
│  └─ About                          → app/about
│
├─ File
│  ├─ Open Structure File            → features/file/structure_import (XSF / CHGCAR / UNV)
│  └─ Open Recent (disabled)
│
├─ Edit
│  ├─ Atoms                          → features/edit/atoms
│  ├─ Bonds                          → features/edit/bonds
│  └─ Cell                           → features/edit/cell
│
├─ Build
│  ├─ Add atoms                      → features/build/periodic_table
│  └─ Bravais Lattice Templates      → features/build/bravais
│
├─ Measurement
│  ├─ Distance                       ┐
│  ├─ Angle                          │
│  ├─ Dihedral                       ├─→ features/measurement
│  ├─ Geometric Center               │
│  └─ Center of Mass                 ┘
│
├─ Data
│  ├─ Isosurface                     ┐
│  ├─ Surface                        ├─→ features/data/charge_density
│  ├─ Volumetric                     ┘
│  └─ Plane                          ──→ features/data/slice
│
├─ Utilities
│  └─ Brillouin Zone                 → features/utilities/brillouin_zone
│
├─ Settings (gear)
│  ├─ Node Tooltip (toggle)
│  ├─ Viewer FPS Overlay (toggle)
│  ├─ Style ▸ Dark/Light/Classic
│  ├─ Background Color
│  ├─ Font Size ▸ small/medium/large
│  └─ Full Screen                    → app/settings
│
├─ Windows (window)
│  ├─ Viewer
│  ├─ Model Tree
│  ├─ Created Atoms / Bonds / Cell           (Edit 윈도우와 동일)
│  ├─ Periodic Table / Crystal Templates / BZ Plot   (Build 윈도우)
│  ├─ Charge Density Viewer / 2D Slice Viewer        (Data 윈도우)
│  └─ DEBUG: Full Dockspace, Show Font Icons → app/layout_manager
│
└─ Layout 1 / 2 / 3 / Reset (인라인 SmallButton)      → app/layout_manager
```

## 3. 메뉴 외 진입점 (참고)

메뉴바 외에도 사용자 입력이 들어오는 경로가 있다. 새 아키텍처에서도 동일 피처 폴더 안에서 처리한다.

| 진입점 | 위치 | 호출하는 기능 | 새 매핑 |
|---|---|---|---|
| Viewer 위 툴바 (Mesh Display / Projection / Reset View / Cell Align / Boundary Atoms / Charge Density Quick / Arrow Step) | `toolbar.cpp` | Mesh + atoms + charge density 혼합 | `features/viewer/toolbar` (필요한 부분은 각 feature 의 public API 호출) |
| 마우스 인터랙터 (좌클릭 픽킹, 드래그 선택, 측정 클릭) | `mouse_interactor_style.cpp` | atoms 픽킹/측정 | `core/vtk/mouse_interactor` + 측정은 `features/measurement` 콜백 |
| 드래그-앤-드롭 / Embind exposed | `bind_function.cpp`, `file_loader.cpp` | 파일 자동 로딩 | `features/file` 동일 진입점 |
| 컨텍스트 메뉴 (Model Tree 우클릭) | `model_tree.cpp` | visibility 토글, 삭제 | `features/model_tree` |

## 4. 메뉴 동작 의미 분류

메뉴 항목은 동작 성격에 따라 4가지로 나눌 수 있다. 새 라우터(`app/menu_router`) 에서도 이 4가지만 다룬다.

1. **OpenWindow** — 토글 가능한 ImGui 창을 보여줌. 예: Windows/* 전부, Edit/Atoms.
2. **EnterMode** — 상태머신 모드 진입. 예: Measurement/* 전부.
3. **InvokeAction** — 즉시 실행. 예: File/Open Structure, Settings/Full Screen, Layout 1~3.
4. **TogglePref** — 환경설정 토글/라디오. 예: Settings/* 의 Style, Font Size, Tooltip, FPS.

→ `MenuRouter` 는 이 4 종류의 디스패처만 가지면 충분하다.

## 5. 검증 — 메뉴 항목 누락 점검 체크리스트

`05_redevelopment_plan.md` 의 마이그레이션 단계가 끝날 때마다 다음 18+ 항목이 모두 새 트리에서 동일하게 동작해야 한다.

- [ ] About 모달
- [ ] File / Open Structure File (XSF, XSF Grid, CHGCAR 자동 분기)
- [ ] Edit / Atoms (Created Atoms 창)
- [ ] Edit / Bonds (Bonds Management 창)
- [ ] Edit / Cell (Cell Information 창)
- [ ] Build / Add atoms (Periodic Table)
- [ ] Build / Bravais Lattice Templates
- [ ] Measurement / Distance / Angle / Dihedral / Geometric Center / Center of Mass (5종, 토글)
- [ ] Data / Isosurface / Surface / Volumetric (Charge Density Viewer)
- [ ] Data / Plane (2D Slice Viewer)
- [ ] Utilities / Brillouin Zone
- [ ] Settings / Node Tooltip (toggle)
- [ ] Settings / Viewer FPS Overlay (toggle)
- [ ] Settings / Style (Dark/Light/Classic)
- [ ] Settings / Background Color
- [ ] Settings / Font Size (small/medium/large)
- [ ] Settings / Full Screen
- [ ] Windows / Viewer · Model Tree · 6 Atoms 창 · 2 Data 창 토글
- [ ] Layout 1 / Layout 2 / Layout 3 / Reset
- [ ] (DEBUG) Full Dockspace, Show Font Icons
- [ ] 툴바 7 종 (Mesh Display, Projection, Reset View, Cell Align, Boundary Atoms, Charge Density Quick, Arrow Step)
