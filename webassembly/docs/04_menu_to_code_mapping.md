# 04. 메뉴 ↔ 코드 매핑 표

> 메뉴 항목 하나하나가 새 트리의 어느 파일/함수에 대응되는지의 1:1 매핑.
> 본 표는 `02_menu_tree.md` 의 18+ 항목 체크리스트와 1:1 짝을 이룬다.

## 1. 표 읽는 법

| 컬럼 | 의미 |
|---|---|
| **메뉴 경로** | 사용자에게 보이는 라벨 (메뉴 → 서브메뉴 → 항목) |
| **Request** | `app::menu_router` 가 만드는 4종 중 하나 (OpenWindow/EnterMode/InvokeAction/TogglePref) |
| **새 디렉터리** | `webassembly/src/` 기준 행선지 |
| **엔트리 함수** | 클릭 시 호출되는 한 줄 |
| **legacy 참조** | 새로 작성할 때 로직을 가져올 기존 코드 |

## 2. Crystal Viewer

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Crystal Viewer / About | InvokeAction | `app/about.*` | `app::about::Show()` | `app.cpp::renderAboutPopup()` |

## 3. File (메뉴 #2)

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| File / Open Structure File | InvokeAction | `features/file/` | `features::file::OpenStructure()` | `FileLoader::RequestOpenStructureImport()` + `FileLoader::handleXSFFile()` + `AtomsTemplate::LoadXSFParsedData()` + `AtomsTemplate::LoadChgcarFile()` |
| File / Open Recent (disabled) | — | `features/file/recent_files.*` | placeholder | — |

> 파서 자체는 **`core/io/`** 로 이동: `xsf_parser`, `chgcar_parser`, `rho_parser`. `features/file/structure_import.cpp` 은 `core/io::format_registry` 에서 매칭되는 파서를 골라 `core/scene::SceneState` 에 결과를 반영한다.

## 4. Edit (메뉴 #3)

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Edit / Atoms | OpenWindow(CreatedAtoms) + Section(Atoms) | `features/edit/atoms/` | `features::edit::atoms::Show()` | `AtomsTemplate::RenderCreatedAtomsWindow()`, `RequestEditorSection(Atoms)` |
| Edit / Bonds | OpenWindow(BondsManagement) + Section(Bonds) | `features/edit/bonds/` | `features::edit::bonds::Show()` | `AtomsTemplate::RenderBondsManagementWindow()`, `RequestEditorSection(Bonds)` |
| Edit / Cell | OpenWindow(CellInformation) + Section(Cell) | `features/edit/cell/` | `features::edit::cell::Show()` | `AtomsTemplate::RenderCellInformationWindow()`, `RequestEditorSection(Cell)` |

내부 매핑:
- `atoms/domain/atom_manager.*` → `features/edit/atoms/atom_manager.*`
- `atoms/domain/bond_manager.*` → `features/edit/bonds/bond_manager.*`
- `atoms/domain/cell_manager.*` → `features/edit/cell/cell_manager.*`
- `atoms/domain/surrounding_atom_manager.*` → `features/edit/atoms/surrounding_atom_manager.*`
- `atoms/infrastructure/vtk_renderer.cpp` 내 atom/bond/cell 관련 부분은 각각 `*/atom_renderer.*`, `bonds/bond_renderer.*`, `cell/cell_renderer.*` 로 분할.
- `atoms/infrastructure/bond_renderer.*` 는 `features/edit/bonds/bond_renderer.*` 의 일부로 흡수.
- `atoms/ui/atom_editor_ui.*`, `bond_ui.*`, `cell_info_ui.*` 는 그대로 동일 폴더로 이동.

## 5. Build (메뉴 #4)

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Build / Add atoms | OpenWindow(PeriodicTable) + Section(AddAtoms) | `features/build/periodic_table/` | `features::build::periodic_table::Show()` | `AtomsTemplate::RenderPeriodicTableWindow()`, `atoms_template_periodic_table.cpp` |
| Build / Bravais Lattice Templates | OpenWindow(CrystalTemplates) + Section(BravaisLatticeTemplates) | `features/build/bravais/` | `features::build::bravais::Show()` | `AtomsTemplate::RenderCrystalTemplatesWindow()`, `atoms_template_bravais_lattice.cpp`, `atoms/domain/crystal_system.*`, `atoms/domain/crystal_structure.*` |

## 6. Measurement (메뉴 #5)

5 개 항목 모두 `EnterMode` 단일 디스패치.

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Measurement / Distance | EnterMode(Distance) | `features/measurement/` | `features::measurement::EnterMode(Distance)` | `AtomsTemplate::EnterMeasurementMode(Distance)` |
| Measurement / Angle | EnterMode(Angle) | `features/measurement/` | 동일 | 동일 (Angle) |
| Measurement / Dihedral | EnterMode(Dihedral) | `features/measurement/` | 동일 | 동일 (Dihedral) |
| Measurement / Geometric Center | EnterMode(GeometricCenter) | `features/measurement/` | 동일 | 동일 (GeometricCenter) |
| Measurement / Center of Mass | EnterMode(CenterOfMass) | `features/measurement/` | 동일 | 동일 (CenterOfMass) |

내부 매핑:
- 모드 상태머신 → `measurement_mode.*`
- 모드별 클릭/렌더 핸들러 → `distance.*`, `angle.*`, `dihedral.*`, `center.*`
- 측정 객체 리스트 + 구조별 visibility → `measurement_store.*`
- 화면 오버레이/UI 패널 → `measurement_overlay_ui.*`
- 드래그 셀렉션 (`AtomsTemplate::HandleDragSelectionInScreenRect`) 은 `core/vtk/mouse_interactor` 에서 이벤트 발생, `features/measurement` 가 구독.

## 7. Data (메뉴 #6)

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Data / Isosurface | DataMenu(Isosurface) → OpenWindow(ChargeDensityViewer) | `features/data/charge_density/` | `features::data::charge_density::Show(Isosurface)` | `AtomsTemplate::RequestDataMenu(Isosurface)`, `atoms/infrastructure/charge_density_renderer.cpp`, `atoms/ui/charge_density_ui.cpp` |
| Data / Surface | DataMenu(Surface) → OpenWindow(ChargeDensityViewer) | 동일 | `Show(Surface)` | 동일 (Surface) |
| Data / Volumetric | DataMenu(Volumetric) → OpenWindow(ChargeDensityViewer) | 동일 | `Show(Volumetric)` | 동일 (Volumetric) |
| Data / Plane | DataMenu(Plane) → OpenWindow(SliceViewer) | `features/data/slice/` | `features::data::slice::Show()` | `AtomsTemplate::RenderSliceViewerWindow()`, `atoms/infrastructure/slice_renderer.h` |

## 8. Utilities (메뉴 #7)

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Utilities / Brillouin Zone | OpenWindow(BrillouinZonePlot) + Section(BrillouinZone) | `features/utilities/brillouin_zone/` | `features::utilities::bz::Show()` | `AtomsTemplate::RenderBrillouinZonePlotWindow()`, `atoms/domain/bz_plot.*`, `atoms/infrastructure/bz_plot_layer.*`, `atoms/ui/bz_plot_ui.*`, `atoms/domain/special_points.h` |

## 9. Settings (메뉴 #8)

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Settings / Node Tooltip | TogglePref(NodeTooltip) | `app/settings.*` | `app::settings::SetNodeTooltip(bool)` | `AtomsTemplate::SetNodeInfoEnabled` |
| Settings / Viewer FPS Overlay | TogglePref(FpsOverlay) | `app/settings.*` | `app::settings::SetFpsOverlay(bool)` | `VtkViewer::SetPerformanceOverlayEnabled` |
| Settings / Style ▸ Dark/Light/Classic | TogglePref(Style) | `app/settings.*` | `app::settings::SetColorStyle(Style)` | `App::SetColorStyle` |
| Settings / Background Color | InvokeAction(OpenBgColorPopup) | `app/settings.*` | `app::settings::OpenBackgroundPopup()` | `m_bShowBgColorPopup` |
| Settings / Font Size ▸ small/medium/large | TogglePref(FontSize) | `app/settings.*` | `app::settings::SetFontSize(Preset)` | `App::SetFontSizePreset` |
| Settings / Full Screen | InvokeAction(FullScreen) | `app/settings.*` | `app::settings::EnterFullScreen()` | `EM_ASM(canvas.requestFullscreen())` |

## 10. Windows (메뉴 #9)

| 메뉴 경로 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Windows / Viewer | TogglePref(WinViewer) | `app/layout_manager.*` (Windows 메뉴 그리기) + `features/viewer/viewer_panel.*` (실제 창) | `app::layout::ToggleWindow(Viewer)` | `m_bShowVtkViewer` |
| Windows / Model Tree | TogglePref(WinModelTree) | `features/model_tree/` | 동일 | `m_bShowModelTree` |
| Windows / Created Atoms | TogglePref(WinCreatedAtoms) | `features/edit/atoms/atom_editor_ui.*` | 동일 | `m_bShowCreatedAtomsWindow` |
| Windows / Bonds Management | TogglePref(WinBondsManagement) | `features/edit/bonds/bond_ui.*` | 동일 | `m_bShowBondsManagementWindow` |
| Windows / Cell Information | TogglePref(WinCellInformation) | `features/edit/cell/cell_info_ui.*` | 동일 | `m_bShowCellInformationWindow` |
| Windows / Periodic Table | TogglePref(WinPeriodicTable) | `features/build/periodic_table/periodic_table_ui.*` | 동일 | `m_bShowPeriodicTableWindow` |
| Windows / Crystal Templates | TogglePref(WinCrystalTemplates) | `features/build/bravais/bravais_lattice_ui.*` | 동일 | `m_bShowCrystalTemplatesWindow` |
| Windows / Brillouin Zone Plot | TogglePref(WinBzPlot) | `features/utilities/brillouin_zone/bz_plot_ui.*` | 동일 | `m_bShowBrillouinZonePlotWindow` |
| Windows / Charge Density Viewer | TogglePref(WinChargeDensity) | `features/data/charge_density/charge_density_ui.*` | 동일 | `m_bShowChargeDensityViewerWindow` |
| Windows / 2D Slice Viewer | TogglePref(WinSlice) | `features/data/slice/slice_ui.*` | 동일 | `m_bShowSliceViewerWindow` |
| Windows / Full Dockspace (DEBUG) | TogglePref(FullDockspace) | `app/layout_manager.*` | 동일 | `m_bFullDockSpace` |
| Windows / Show Font Icons | TogglePref(FontIcons) | `app/layout_manager.*` (디버그 패널) | 동일 | `m_bShowFontIcons` |

## 11. Layout 인라인 버튼

| 라벨 | Request | 새 디렉터리 | 엔트리 함수 | legacy 참조 |
|---|---|---|---|---|
| Layout 1 | InvokeAction(Layout1) | `app/layout_manager.*` | `app::layout::ApplyPreset(DefaultFloating)` | `LayoutPreset::DefaultFloating` |
| Layout 2 | InvokeAction(Layout2) | 동일 | `ApplyPreset(DockRight)` | `LayoutPreset::DockRight` |
| Layout 3 | InvokeAction(Layout3) | 동일 | `ApplyPreset(DockBottom)` | `LayoutPreset::DockBottom` |
| Reset | InvokeAction(ResetLayout) | 동일 | `ApplyPreset(ResetDocking)` | `LayoutPreset::ResetDocking` |

## 12. 메뉴 외 진입점

| 진입점 | 새 위치 | legacy 참조 |
|---|---|---|
| Viewer 위 툴바 | `features/viewer/toolbar.*` | `toolbar.cpp/h` |
|   - Mesh Display Mode | `features/mesh/mesh_ui.*` 가 노출하는 setter 호출 | `toolbar::renderMeshDisplayModeButtons` |
|   - Projection (Parallel/Perspective) | `core/vtk::vtk_viewer` setter | `toolbar::renderProjectionModeButtons` |
|   - Reset View | 동일 | `toolbar::renderResetViewButton` |
|   - Cell Align (X/Y/Z) | `features/edit/cell/cell_manager` 콜백 | `toolbar::renderCellAlignButtons` |
|   - Boundary Atoms Quick | `features/edit/atoms/surrounding_atom_manager` setter | `toolbar::renderBoundaryAtomsQuickButton` |
|   - Charge Density Quick | `features/data/charge_density` 가 노출하는 quick controls | `toolbar::renderChargeDensityControls` |
|   - Arrow Rotation Step | `features/viewer/viewer_panel` 의 step 위젯 | `toolbar::renderArrowRotationStepInput` |
| 마우스 인터랙터 (픽킹/드래그) | `core/vtk/mouse_interactor.*` (이벤트 emit) → 각 feature 구독 | `mouse_interactor_style.*`, `AtomsTemplate::SelectAtomByPicker`, `HandleMeasurementClickByPicker`, `HandleDragSelectionInScreenRect` |
| 드래그-앤-드롭 / Embind | `bind_function.cpp` 는 그대로, `features/file/structure_import` 이 진입점 | `bind_function.cpp`, `file_loader.cpp` |
| Model Tree 우클릭 액션 | `features/model_tree/model_tree_actions.*` | `model_tree.cpp` 의 컨텍스트 메뉴 부분 |

## 13. AtomsTemplate 공개 API 의 행선지

`atoms/atoms_template.h` public API 를 카테고리별로 분배한 결과.

| 영역 | 행선지 |
|---|---|
| Hover/Selection/Tooltip | `core/scene/{selection, hover}.*` + `features/edit/atoms/atom_renderer` 의 표시 로직 |
| Measurement (Enter/Exit/Click/Drag/Render/Store/Visible) | 전부 `features/measurement/` |
| Charge Density (Load/Visible/Mode/Sync) | 전부 `features/data/charge_density/` + 파서는 `core/io/chgcar_parser.*` |
| Structure registry (Register/Remove/Get/IsVisible/SetVisible) | `core/scene/structure_registry.*` |
| Atom group / bond / label visibility | `core/scene/visibility.*` (새 구조), 호출자는 각 feature manager |
| Builder section / Editor section / Data menu Request | 메뉴 라우터 → 각 feature::HandleRequest |
| `RenderXxxWindow(*open)` | 각 feature 의 `RenderWindows(WindowFlags&)` |
| Forced layout 요청 | `app/layout_manager.*` |
| `LoadXSFFile`, `LoadChgcarFile` | `features/file/structure_import.cpp` (파서 호출 → SceneState 반영) |
| Bravais lattice (`setBravaisLattice` 등) | `features/build/bravais/bravais_lattice.*` |
| BZ plot toggle/ render | `features/utilities/brillouin_zone/` |

## 14. AtomsTemplate 직접 호출의 재라우팅 계획 (legacy 의존 단절)

> 본 표는 **재개발 전(현 시점)** 외부 모듈이 `AtomsTemplate` 싱글턴을 직접 호출하던 패턴을, **재개발 후** 새 트리에서 어떤 통로로 평탄화할지 1:1 로 짝지은 것이다.
>
> 표의 좌측("legacy 위치")은 *재개발 전 호출이 일어나던 파일* 을 가리키며, 우측("새 호출 경로")은 *재개발 후 동일 행위를 처리하는 새 트리의 위치* 다. 즉 표의 좌측 항목은 모두 **재개발 후에는 사라지거나 새 트리의 같은 책임 모듈로 대체** 된다 — 재개발이 끝난 시점에 새 코드가 legacy 에 의존하는 부분은 없다.
>
> 검증 기준은 `05_redevelopment_plan.md` Phase 5 에 명시되어 있다: `grep -r "legacy/" webassembly/src --include="*.cpp" --include="*.h"` 결과가 0 이어야 한다.

| legacy 위치 (재개발 전) | 호출하던 AtomsTemplate API | 새 호출 경로 (재개발 후) |
|---|---|---|
| `model_tree.cpp` | `GetStructures`, `IsStructureVisible`, `SetStructureVisible`, `IsUnitCellVisible`, … | `core/scene::structure_registry` 의 read API |
| `file_loader.cpp` | `LoadXSFParsedData`, `LoadChgcarParsedData` | `features/file::structure_import::OnParseResult(...)` |
| `toolbar.cpp` | `HasChargeDensity`, `IsChargeDensitySimpleViewActive`, `chargeDensityUI()`, `EnterMeasurementMode` | 각 feature 의 public quick API |
| `mouse_interactor_style.cpp` | `SelectAtomByPicker`, `HandleMeasurementClickByPicker`, `HandleMeasurementEmptyClick`, `HandleDragSelectionInScreenRect` | `core/vtk::mouse_interactor` 가 이벤트만 발생 → `features/edit/atoms`, `features/measurement` 가 구독 |

→ **모든 직접 호출이 `core/scene` 또는 `core/events` 한 점을 통과**하도록 만든다는 것이 본 매핑의 골자. legacy 트리는 동결본으로 보관되지만 새 코드의 빌드 그래프 안에는 들어오지 않는다.
