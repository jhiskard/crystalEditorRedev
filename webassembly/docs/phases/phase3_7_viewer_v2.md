# Phase 3.7 - Viewer / Toolbar 이식 세부계획서 (v2)

> 상위 문서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.7) + §6.0 공통 지침
> 대상 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §2 features/viewer + §3 5-진입점 컨벤션
> 메뉴 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §9 Settings, §10 Windows, §12 메뉴 외 진입점 (Viewer 위 툴바)
> 메뉴 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §3, §5 (Viewer / Settings / Windows / 툴바 7종)
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01~UI-12)
> 선행 계획서: [`./phase3_6_file.md`](./phase3_6_file.md)
> 선행 평가서 (v1): [`./phase3_6_evaluation_2026-05-12.md`](./phase3_6_evaluation_2026-05-12.md)
> **선행 평가서 (entry gate baseline)**: [`./phase3_6_evaluation_2026-05-13.md`](./phase3_6_evaluation_2026-05-13.md)
> v1 계획서: [`./phase3_7_viewer.md`](./phase3_7_viewer.md)
> 작성일: 2026-05-13
> 대상 브랜치: `refactor/menu-aligned2` 후속 작업 브랜치 (예: `refactor/viewer-restore`)
> 단위 PR: 1 개, 단일 sub-folder 중심 (`features/viewer/`) + 명시 필요한 core/edit/data 보강
> 예상 소요: 2 ~ 3 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-12 | v1 초안 작성 — Phase 3.6 v1 평가서의 #28 BLOCKED, #8/#15 PARTIAL 을 Phase 3.7 실행 계획에 반영 |
| 2026-05-13 | **v2 작성** — Phase 3.6 의 2026-05-13 동결 평가서를 entry gate baseline 으로 인계. v1 의 *목표/범위/검증 매트릭스* 를 그대로 유지하되, (a) entry gate 3종 (#28/#15/#8) 의 인계 의무를 1차 강제 검증 항목으로 격상, (b) File hot path 비침범 원칙을 정적 검증으로 명시, (c) Viewer texture/Mouse interactor/Cell Align 의 *책임 분리* 를 강화, (d) wasm size baseline 을 15,245,923B (2026-05-13) 로 갱신, (e) PR 본문 체크리스트와 평가서 작성 지침 보강 |
| 2026-05-13 | **v2.1 (권장안 B 반영)** — 사용자 검토로 식별된 *legacy 입력 계약* (키 + 마우스 카메라 조합 / 측정 모드별 픽·드래그·시각 표시 / 비-측정 모드 Ctrl+클릭·Ctrl+드래그·Ctrl+더블클릭 same-element 선택 / 드래그 사각 오버레이) 누락을 본 계획서에 *직접* 보강. 변경: (a) Part 3.2 의 `mouse_interactor.{h,cpp}` 행을 *필수 재작성* 으로 격상, (b) Part 4.5 에 `SetEventInformationFlipY` / `BeginInteractionLod` / `EndInteractionLod` / 4px 임계 / modifier-gating 정책 명시, (c) Part 5 에 § 5.5 "마우스 / 키보드 입력 계약" 3 개 표 신설, (d) Part 7.3 에 매트릭스 행 #33 ~ #45 추가, (e) Part 8 DoD 보강, (f) Part 12 PR 체크리스트 보강 |
| 2026-05-13 | **v2.1.1 (권장안 D 반영, 본 문서)** — 사용자 추가 검토로 식별된 *legacy 렌더 스타일 보존* (원자 sphere base radius / scale 산식, shading 파라미터, 결합 cylinder proportional 2-color split, shading, opacity translucent path, 선택 시각 표시) 누락을 명시. **선결 조건**: Phase 3.7 진입 *이전* 에 별도 회귀 PR ([`./phase3_4_renderer_regression.md`](./phase3_4_renderer_regression.md)) 로 Phase 3.4 산출물의 수치 오류를 수정한다. 변경: (a) Part 1.5 신설 - Phase 3.4 회귀 PR 선결 의무, (b) Part 5 에 § 5.6 "렌더 스타일 회귀 보존" 3 개 표 신설, (c) Part 7.3 에 매트릭스 행 #46 ~ #53 추가 (DEVIATION 허용), (d) Part 8 DoD 보강, (e) Part 12 PR 체크리스트 회귀 점검 섹션 추가, (f) Part 14 v2.1 → v2.1.1 매핑 추가 |

---

## 0. 한 줄 요약

> Phase 3.7 v2 는 `features/viewer/` 를 신설해 `Windows / Viewer` 창과 Viewer 상단 toolbar 7 종 (Boundary Atoms, Mesh Display, Projection, Reset View, Cell Align, Charge Density Quick, Arrow Step) 을 복구하고, 현재 placeholder 인 `core/vtk::VtkViewer::Render()` 를 legacy 의 WebAssembly offscreen framebuffer + ImGui `Image` 렌더 흐름으로 되살린다.
>
> 본 v2 는 Phase 3.6 의 2026-05-13 동결 평가서가 명시한 **entry gate 3종** 을 Phase 3.7 의 1차 통과 기준으로 격상한다: (#28) Viewer 복구 후 imported atoms 위에서 Measurement Distance/Angle 픽킹이 PASS, (#15) progress popup 의 legacy side-by-side 시각 캡처를 평가서에 첨부, (#8) Phase 3.6 File hot path 의 비악화를 정적 검증.
>
> 본 phase 의 비목표는 v1 과 동일하다: full mesh feature 적용, Model Tree, MenuRouter 정식 도입, 전체 Settings 메뉴, Layout preset, File parser 수정, Data UI 재설계, Measurement 로직 재개발은 모두 후속 phase 범위로 분리한다.

---

# Part 1 - 목표 / 비목표 / 평가서 인계

## 1.1 목표 (계획 일치)

| # | 구분 | 항목 |
|---:|---|---|
| G1 | Viewer 창 복구 | `Windows / Viewer` 토글로 `Viewer` ImGui 창 표시/숨김 |
| G2 | 실제 VTK 화면 표시 | `core/vtk::VtkViewer::Render()` 의 placeholder 문구 제거. WebAssembly offscreen framebuffer 텍스처를 `ImGui::Image` 로 표시 |
| G3 | 창 resize 동기화 | Viewer content 크기 변경 시 render window + framebuffer texture + picker 좌표계 갱신 |
| G4 | mouse interactor 연동 | Viewer viewport 안의 click/drag/wheel/hover 좌표를 `core/vtk::MouseInteractor` 및 VTK interactor 로 전달. Phase 3.5 Measurement pick/drag 가 실제 화면 위에서 작동 |
| G5 | toolbar 7종 복구 | legacy 순서: Boundary Atoms, Mesh Display, Projection, Reset View, Cell Align, Charge Density Quick, Arrow Step |
| G6 | Settings 부분 복구 | `Settings / Viewer FPS Overlay` 만 본 phase 범위. `VtkViewer::SetPerformanceOverlayEnabled` 연결 |
| G7 | Windows 부분 복구 | `Windows / Viewer` 만 본 phase 범위. 다른 항목은 후속 phase |
| G8 | Phase 3.6 entry gate 해소 | (#28) PASS, (#15) side-by-side 캡처 첨부, (#8) 비악화 검증 |
| G9 | 기존 feature actor 가시화 | Phase 3.3 ~ 3.6 의 cell/atom/bond/measurement/charge density actor 가 Viewer 안에 보인다 |
| G10 | legacy 격리 유지 | `webassembly/src/legacy/` 미수정, 신규 파일에서 `legacy/` include 0 |
| G11 | 빌드 검증 | `npm run build-wasm:debug`, `npm run build-wasm:release` PASS |
| G12 | wasm size 추적 | 직전 baseline 15,245,923B (2026-05-13) 대비 delta 기록 |

## 1.2 비목표 (계획 일치)

| 항목 | 사유 / 후속 phase |
|---|---|
| Full mesh feature | Mesh actor 적용, Mesh Group/Detail UI, UNV full mesh import 는 Phase 3.9 |
| Model Tree | Phase 3.8 |
| MenuRouter 정식 | Phase 4 (`app/menu_router`, `app/settings`, `app/layout_manager`) |
| 전체 Settings | Background Color, Style, Font Size, Full Screen, Node Tooltip 등은 Phase 4 |
| Layout preset | Layout 1/2/3/Reset 인라인 버튼은 Phase 4 `layout_manager` |
| File parser 수정 | Phase 3.6 의 File hot path 는 본 phase 에서 변경 금지. 명백한 표시 버그 발견 시 최소 수정만 허용하고 deviation 으로 기록 |
| Data UI 재설계 | Charge Density Viewer / 2D Slice Viewer 창 자체 옵션 재설계 금지. toolbar quick control 용 최소 public API 만 추가 |
| Measurement 로직 재개발 | Phase 3.5 알고리즘 미수정. Viewer 좌표/picker 연결만 보강 |
| Cell matrix 수정 | Cell Align toolbar 는 *camera* 만 정렬. `CellManager::AlignAxis` 등 lattice 변경 호출 금지 |

## 1.3 Phase 3.6 (2026-05-13) entry gate 인계

본 phase 는 Phase 3.6 의 2026-05-13 동결 평가서가 인계한 entry gate 3종을 본 phase 의 *1차 통과 의무* 로 격상한다.

| Entry gate | Phase 3.6 상태 | Phase 3.7 v2 의무 | 검증 위치 |
|---|---|---|---|
| #28 Measurement on imported atoms | BLOCKED | imported XSF 와 CHGCAR 위에서 Distance / Angle 픽킹 PASS. drag selection 좌표계 확인 | 본 계획서 Part 7.3 #27, #28 |
| #15 progress popup side-by-side | PARTIAL | Viewer 복구 후 imported XSF/CHGCAR 시나리오와 함께 legacy 와 새 트리의 progress popup show/hide/percentage 표시를 동일 캡처로 비교. 평가서 부록에 첨부 | 본 계획서 Part 7.3 #29 |
| #8 FormatRegistry 우회 hot path | PARTIAL (구조적) | Phase 3.7 는 *수정 금지*. 본 phase PR 의 정적 검증에서 `features/file/` 및 `core/io/` 의 비변경 확인 | 본 계획서 Part 7.1 #10 |

본 entry gate 의 해소 여부는 Phase 3.7 평가서에 *전용 섹션* 으로 기재한다.

## 1.5 선결 조건: Phase 3.4 렌더 스타일 회귀 PR

Phase 3.7 v2.1.1 은 **Phase 3.4 회귀 PR ([`./phase3_4_renderer_regression.md`](./phase3_4_renderer_regression.md)) 이 머지된 후에만 진입 가능** 하다는 선결 조건을 둔다.

| 사유 | 내용 |
|---|---|
| 식별된 문제 | Phase 3.4 산출물인 `features/edit/atoms/atom_renderer.cpp` 와 `features/edit/bonds/bond_renderer.cpp` 가 legacy 대비 (a) 원자 sphere 의 유효 반경 2× 차이, (b) shading 파라미터 누락 (ambient 0.3 / diffuse 0.7 / specular 0.1 / specularPower 10), (c) 결합 cylinder 의 proportional split (`radius1/(radius1+radius2)`) 미적용, (d) opacity translucent path 미구현, (e) 선택 시각 표시가 *그룹 색 변경* 으로 잘못 구현 (legacy 는 *개별 노란 wireframe shell*) |
| 가시화 시점 | Viewer texture 가 살아나는 *Phase 3.7 시점* 에 사용자-visible 차이로 드러남 |
| 분리 사유 | Phase 3.7 의 *Viewer / toolbar 복구* 핵심 목표가 *Phase 3.4 회귀 수정* 과 섞이면 PR 규모가 폭증하고 디버깅 책임 경계가 모호해짐. legacy 1:1 보존 위반의 *origin* 이 Phase 3.4 이므로 회귀 PR 의 책임 phase 도 3.4 가 정답 |
| 본 phase 의 의무 | (a) 진입 전 Phase 3.4 회귀 PR 의 머지 확인. (b) 진입 후 Viewer 가시화 시점에 § 5.6 의 회귀 점검 매트릭스 (#46 ~ #53) 를 PASS 로 갱신. 잔여 항목은 DEVIATION 으로 명시 |

> 만약 일정상 Phase 3.4 회귀 PR 을 분리할 수 없으면, 본 v2.1.1 의 *§ 5.6 + #46 ~ #53* 항목은 Phase 3.7 평가서에서 일괄 DEVIATION 으로 기록한 뒤 후속 phase 에 인계한다. 그러나 권장 경로는 **별도 PR** 이다.

## 1.4 v1 대비 v2 변경점 요약

| 영역 | v1 | v2 (본 문서) |
|---|---|---|
| Phase 3.6 평가서 baseline | 2026-05-12 평가서 | 2026-05-13 평가서 (동결 점검 결과 인계) |
| entry gate 3종 처리 | 평가 항목 일부 | *1차 통과 의무* 로 격상 |
| File hot path 비침범 | 비목표에 언급 | 정적 검증 항목 (#10) 으로 명시 |
| `core/vtk::VtkViewer` 책임 | API 보강 목록 제공 | API 보강 + *책임 경계* (top-menu state 보관 금지) 명시 |
| Cell Align 정책 | camera-only 언급 | `CellManager::AlignAxis` 사용 금지 + `features::edit::AlignCameraToCurrentCellAxis(axis)` 신규 wrapper 권장 |
| Mesh Display | Phase 3.9 deferred | 동일. 단 tooltip 문구를 PR 본문에 고정 |
| wasm size baseline | 미명시 | 15,245,923B (2026-05-13) 명시 |
| 평가서 작성 지침 | 일반 양식 | entry gate 전용 섹션 포함 |
| PR 체크리스트 | 일반 항목 | 정적 / 빌드 / 런타임 / entry gate 분리 |

---

# Part 2 - 현재 상태와 gap 분석 (재확인)

## 2.1 현재 코드 상태 (2026-05-13 동결 시점)

| 영역 | 현재 상태 | gap |
|---|---|---|
| `core/vtk/vtk_viewer` (`vtk_viewer.h/cpp`) | renderer/window/interactor 객체는 있으나 `Render()` 가 `ImGui::Begin("Viewer")` + 텍스트 두 줄("Phase 1 bootstrap: empty VTK scene", "Rendering features will be restored in later phases.") 만 표시 | actor 가 등록되더라도 사용자가 볼 수 없음 |
| `app/app.cpp` | `features::file::InitOnce`, `DrawMenu`, `RenderWindows` 만 hook 됨. `Windows / Viewer` 메뉴 없음. `VtkViewer::Instance().Render()` 가 매 frame 직접 호출 | viewer show/hide flag 부재, Windows/Settings menu 자체 없음 |
| `core/vtk/mouse_interactor` | pick/drag/wheel emit 인터페이스 존재 | Viewer viewport 좌표 매핑/feeding 미복구 |
| edit/data/measurement actor 경로 | 등록되어 있음 | Viewer texture 미표시로 시각 검증 불가 |
| `features/viewer/` | 폴더 자체 없음 | 신설 필요 |
| `features/file/` | 11파일 동결 (Phase 3.6 완료) | Phase 3.7 에서 *건드리지 않음* |

## 2.2 legacy 참조 범위 (v1 동일)

| legacy 파일 | 참조할 내용 | 신규 위치 |
|---|---|---|
| `legacy/vtk_viewer.{h,cpp}` | WebAssembly render window, framebuffer texture, `ImGui::Image`, event processing, camera reset/projection/FPS overlay | `core/vtk/vtk_viewer.{h,cpp}` 보강 + `features/viewer/viewer_panel.*` |
| `legacy/toolbar.{h,cpp}` | toolbar 배치, 버튼 순서, tooltip, popup, arrow step input, charge density quick controls | `features/viewer/toolbar.{h,cpp}` |
| `legacy/app.cpp` | `m_bShowVtkViewer`, `Windows / Viewer`, `Settings / Viewer FPS Overlay` | `features/viewer/viewer_menu.*` + `app/app.cpp` 임시 hook |
| `legacy/enum/viewer_enums.h` | `ProjectionMode`, `MeshDisplayMode`, `CameraDirection` | 신규 `core/vtk/viewer_types.h` (camera/projection enum) + `features/viewer/viewer_types.h` (toolbar anchor, mesh display mode UI state) |
| `legacy/enum/toolbar_enums.h` | toolbar anchor 값 | 신규 `features/viewer/viewer_types.h` |

직접 include 는 금지. enum 은 새 헤더에 *재정의* 한다.

## 2.3 핵심 설계 판단 (v1 + v2 보강)

| 판단 | 내용 |
|---|---|
| Viewer = feature, 저수준 VTK = core | `features/viewer` 가 창/toolbar/menu 책임, `core/vtk::VtkViewer` 가 render window / framebuffer / camera / actor ownership 책임 |
| Phase 4 이전 임시 메뉴 hook 허용 | Phase 3.1~3.6 패턴 유지. `app/app.cpp` 에 `features::viewer::DrawMenus()` / `RenderWindows()` 호출. Phase 4 에서 `app/settings` + `app/layout_manager` 로 이관 |
| Mesh Display 는 UI + no-op 만 | Phase 3.9 전에는 적용 대상 mesh actor 없음. 버튼/popup/state/tooltip 만 복구. tooltip 에 `Mesh display mode applies to mesh actors imported via Phase 3.9.` 명시 |
| Cell Align 은 camera-only | `CellManager::AlignAxis` 등 lattice 변경 API 사용 금지. `features::edit::AlignCameraToCurrentCellAxis(axis)` wrapper 신규 또는 `core::vtk::VtkViewer::AlignCameraToCellAxis(cell, axis)` 호출 |
| #28 은 DoD 의 일부 | Viewer 가 보이는 것만으로는 완료 아님. File import 후 imported atoms 위 Measurement Distance/Angle pick PASS 가 필수 |
| File hot path 미침범 | `features/file/` 및 `core/io/` 의 코드 변경 0 이 정적 검증 항목 |

---

# Part 3 - 대상 산출물

## 3.1 신규 파일 계획

| 파일 | 역할 | 예상 라인 |
|---|---|---:|
| `webassembly/src/features/viewer/viewer_menu.h` | Phase 3.7 public entrypoint 선언 (`InitOnce`, `DrawMenus`, `RenderWindows`, `Shutdown`, `HandleRequest`) | 30 ~ 50 |
| `webassembly/src/features/viewer/viewer_menu.cpp` | `Windows / Viewer`, `Settings / Viewer FPS Overlay` 메뉴 hook + viewer show/hide flag 보관 | 120 ~ 170 |
| `webassembly/src/features/viewer/viewer_panel.h` | Viewer ImGui window/panel 렌더러 선언 | 30 ~ 50 |
| `webassembly/src/features/viewer/viewer_panel.cpp` | `ImGui::Begin("Viewer")`, viewport rect 계산, `core::vtk::VtkViewer` texture draw 호출, toolbar overlay 배치 | 180 ~ 240 |
| `webassembly/src/features/viewer/toolbar.h` | toolbar state/API 선언 | 40 ~ 70 |
| `webassembly/src/features/viewer/toolbar.cpp` | toolbar 7종 UI + feature public API 호출 | 250 ~ 360 |
| `webassembly/src/features/viewer/viewer_types.h` | toolbar anchor, mesh display mode UI state enum 등 viewer feature 전용 enum | 40 ~ 80 |
| **합계 (신규)** | **7 파일** | **약 690 ~ 1,020** |

## 3.2 보강 대상 파일

| 파일 | 변경 내용 |
|---|---|
| `webassembly/src/core/vtk/vtk_viewer.h` | WebAssembly texture 렌더 API, projection, reset, cell-axis camera align, FPS overlay, arrow step public API 추가 |
| `webassembly/src/core/vtk/vtk_viewer.cpp` | legacy `VtkViewer` 의 framebuffer, resize, event processing, camera operation, performance overlay 를 신규 core 스타일로 이식. 현 placeholder `Render()` 제거 |
| `webassembly/src/core/vtk/viewer_types.h` (신규) | `ProjectionMode`, `CameraDirection` 등 core camera enum. legacy enum 직접 include 금지 |
| `webassembly/src/core/vtk/mouse_interactor.{h,cpp}` | **필수 재작성**: legacy `vtk_viewer.cpp::processEvents()` (라인 1178~1504) 의 state machine 을 신규 트리로 이식. 포함 항목: (a) `SetEventInformationFlipY(x, y, ctrl, shift, dclick)` 호출로 modifier 전달, (b) 4px click vs drag 임계, (c) `selectionModifierDown = KeyCtrl || KeySuper` 일 때 trackball forward 차단 후 drag tracking 진입, (d) measurement mode active + `IsMeasurementDragSelectionEnabled` 분기, (e) drag rectangle overlay 그리기 (`AddRectFilled` + `AddRect`), (f) `BeginInteractionLod` / `EndInteractionLod` wrapping + wheel hold timer, (g) 비-측정 모드 빈 클릭 시 `ClearCreatedAtomSelection` 호출, (h) Ctrl + 더블클릭 = same-element select. § 5.5 의 표를 단위 검증으로 사용 |
| `webassembly/src/core/ui/widgets.{h,cpp}` | tooltip helper, transparent icon button helper 보강 (legacy 와 동일 외형). 임의 UI 재설계 금지 |
| `webassembly/src/features/edit/edit_menu.{h,cpp}` | toolbar quick API: `IsBoundaryAtomsEnabled()`, `SetBoundaryAtomsEnabled(bool)`, `AlignCameraToCurrentCellAxis(int axis)` 노출 |
| `webassembly/src/features/data/data_menu.{h,cpp}` | toolbar quick API: `HasChargeDensity()`, `GetActiveChargeDensityName()`, `SetChargeDensityLevelPercent(float)`, `IsQuickAnimationActive()`, `StartQuickAnimation()`, `StopQuickAnimation()`, `RestartQuickAnimation()` 노출 |
| `webassembly/src/features/data/charge_density/charge_density*.{cpp,h}` | 위 quick API 가 부족할 경우 *최소* public method 추가. UI 본체 변경 금지 |
| `webassembly/src/app/app.cpp` | `features/viewer` include, `InitOnce`, `DrawMenus`, `RenderWindows` hook 추가. 기존 direct `VtkViewer::Render()` 호출 제거 |
| `CMakeLists.txt` | 신규 `features/viewer` 파일과 `core/vtk/viewer_types.h` 등록 |

## 3.3 예상 규모

| 구분 | 예상 |
|---|---:|
| 신규 `features/viewer` 파일 | 7 |
| core/app/data/edit 보강 파일 | 8 ~ 12 |
| 신규/수정 라인 수 | 900 ~ 1,400 |
| legacy 직접 참조 / include | 0 |
| build target 의 legacy source 추가 | 금지 |
| `features/file/` 및 `core/io/` 의 변경 | 0 (정적 검증) |

---

# Part 4 - 아키텍처 세부 계획

## 4.1 호출 흐름

```text
app::App::Init()
  -> core::vtk::VtkViewer::Instance().Init()
  -> features::viewer::InitOnce(g_sceneState, *g_mouseInteractor)
  -> features::file::InitOnce(g_sceneState)
  -> ... 기존 features InitOnce ...

app::App::renderDockSpaceAndMenu()
  -> 메뉴바 순서
       Crystal Viewer | File | Edit | Build | Measurement | Data | Utilities | Settings | Windows | [Layout 인라인]
  -> features::viewer::DrawMenus()    // Settings / Viewer FPS Overlay + Windows / Viewer 만
  -> 기존 메뉴들 (file/edit/build/measurement/data/utilities) 그대로 호출

app::App::renderFrame()
  -> features::viewer::RenderWindows()
       - ViewerPanel::Render(&g_showViewerWindow)
       - core::vtk::VtkViewer::DrawRenderTexture(viewportSize, viewportPos)
       - Toolbar::Render(toolbarContext)
  -> 기존 feature RenderWindows()
```

## 4.2 책임 분리

| 계층 | 책임 | 금지 |
|---|---|---|
| `features/viewer/viewer_menu` | top menu hook, viewer show/hide flag, FPS toggle dispatch | render window 직접 조작 금지, 다른 feature 의 내부 객체 직접 접근 금지 |
| `features/viewer/viewer_panel` | `Viewer` ImGui window, viewport size/position 계산, toolbar overlay 배치 호출 | actor 생성/삭제 금지, top-menu state 보관 금지 |
| `features/viewer/toolbar` | toolbar 7종 UI, feature public API 호출 | `legacy/` include 금지, 다른 feature 내부 객체 직접 접근 금지 |
| `core/vtk/vtk_viewer` | VTK renderer/window/interactor, framebuffer texture, camera, event feeding, FPS overlay | top menu state 보관 금지, feature 직접 호출 금지 |
| `features/edit` | boundary atoms / cell camera align quick API | toolbar UI 직접 렌더 금지 |
| `features/data` | charge density quick API | toolbar UI 직접 렌더 금지 |
| `features/file` (Phase 3.6) | **Phase 3.7 에서 수정 금지** | 본 phase 의 정적 검증 대상 |

## 4.3 `core/vtk::VtkViewer` 보강 API (v1 + v2 확장)

| API | 목적 |
|---|---|
| `void DrawRenderTexture(const ImVec2& viewportSize, const ImVec2& viewportPos)` | VTK render result 를 ImGui `Image` 로 표시. 내부에서 dirty render 결정 |
| `void Resize(int w, int h)` | render window + framebuffer texture 크기 동기화 |
| `void RequestRender()` | dirty flag 설정 |
| `void ResetView()` | legacy reset view 버튼 동작 |
| `void FitViewToVisibleProps()` | 현재 actor bounds 에 맞게 camera reset |
| `void SetProjectionMode(ProjectionMode mode)` | perspective/parallel 전환 |
| `ProjectionMode GetProjectionMode() const` | toolbar 상태 표시 |
| `void SetPerformanceOverlayEnabled(bool enabled)` | Settings / Viewer FPS Overlay |
| `bool IsPerformanceOverlayEnabled() const` | menu check 상태 |
| `void AlignCameraToCellAxis(const std::array<std::array<float, 3>, 3>& cell, int axis)` | cell matrix 변경 금지, camera 방향만 변경 |
| `void SetArrowRotateStepDeg(float stepDeg)` | Arrow Step input 반영. clamp [1, 180] |
| `float GetArrowRotateStepDeg() const` | Arrow Step input 표시. default 45 |
| `void ProcessViewerEvents(const ViewerInputState& input)` | viewport-relative mouse/keyboard event 를 VTK interactor 에 전달 |

## 4.4 WebAssembly 렌더 복구 원칙

| 항목 | 원칙 |
|---|---|
| render window 타입 | legacy 와 동일하게 `vtkWebAssemblyOpenGLRenderWindow` (또는 동등 WebAssembly용) 사용. 현 generic `vtkRenderWindow` 로 actor 가 안 보이면 교체 |
| offscreen rendering | Viewer 창 내부에 texture 로 표시하기 위해 offscreen framebuffer 흐름 복구 |
| 텍스처 y-axis | OpenGL bottom-left vs ImGui top-left 차이를 legacy 와 동일하게 UV 보정 |
| resize | content size ≤ 1px 이면 render skip. 정상 크기에서만 framebuffer 재생성 |
| dirty render | actor 변경, resize, interaction, forced render 시점 우선. 초기 복구 단계는 안정성 우선해서 legacy 의 렌더 주기를 그대로 허용 |
| overlay renderer | measurement 2D/3D overlay 의 depth 묻힘 방지를 위해 legacy overlay renderer 구조 검토. 현 actor 경로와 충돌 시 최소 보강 |
| camera widget | legacy camera orientation widget 은 가능하면 복구. WebAssembly/ImGui texture 와 충돌 시 평가서 deviation 으로 기록하고 후속 보강 분리 |

## 4.5 event / picker 좌표 원칙

| 항목 | 원칙 |
|---|---|
| 좌표 변환 | ImGui screen coordinate → Viewer viewport local coordinate |
| y-axis 보정 | VTK picker(bottom-left) vs ImGui(top-left) 차이를 legacy 와 동일하게 보정 |
| hover gating | Viewer image 영역이 `IsItemHovered`/`IsItemFocused` 일 때만 VTK interactor 로 event 전달. menu/toolbar 클릭은 camera drag 로 들어가지 않게 차단 |
| `WantCaptureMouse` 차단 | `io.WantCaptureMouse && ImGui::IsAnyItemHovered()` 면 scene 입력 무시 (다른 ImGui 위젯 우선) |
| 타이틀바 드래그 | Viewer 창의 타이틀바 영역 좌클릭 → 창 이동으로만 처리, scene 입력 차단. `io.ConfigWindowsMoveFromTitleBarOnly = true` 정책 유지 |
| **modifier 전달** | 매 frame `SetEventInformationFlipY(xPos, yPos, static_cast<int>(io.KeyCtrl), static_cast<int>(io.KeyShift), dclick)` 호출로 ctrl / shift / 더블클릭 상태를 VTK interactor 에 전달. **이 호출이 없으면 Pan(Shift+Left), Spin(Ctrl+Left) 이 동작하지 않음** |
| **click vs drag 임계** | 좌버튼 release 시 `|Δx| + |Δy| <= 4` 면 click, 초과면 drag. legacy 와 동일하게 4 px 임계 사용 |
| **modifier 기반 분기** | `selectionModifierDown = io.KeyCtrl \|\| io.KeySuper` 일 때 — (a) 측정 모드가 아니거나 (b) 측정 모드이고 `IsMeasurementDragSelectionEnabled()` 가 true 면 drag tracking 진입, 같은 시점 trackball `LeftButtonPressEvent` forward 는 *건너뜀*. modifier 없으면 legacy 와 동일하게 trackball 로 forward (카메라 회전 경로) |
| drag selection | Phase 3.5 `onDragSelection` 의 viewport height 인자에 실제 Viewer texture height 전달. additive=true 로 발행 |
| drag rectangle 시각 | `active=true` (4px 초과 이동) 인 동안 Viewer overlay 로 반투명 파란 사각형 그리기 (`AddRectFilled` IM_COL32(90,170,255,45) + `AddRect` IM_COL32(90,170,255,220) width 1.8) |
| empty click | (a) 측정 모드: `HandleMeasurementEmptyClick` 으로 누적 픽 클리어, (b) 비-측정 모드: `features::edit::ClearCreatedAtomSelection()` 호출. **이 분기 보존 필수** |
| same-element select | `io.MouseDoubleClicked[Left]` 시점에 selectionModifier 가 눌려 있으면 `sameElementSelectAtStart` flag 셋 → release 시 단일 픽 대신 `SelectSameElementAtomsByPicker` 분기 |
| wheel | camera zoom 후 `RequestRender()` + `BeginInteractionLod` / wheel-hold timer / `EndInteractionLod` wrapping. wheel hold 동안 LOD 유지. event fanout 보존 |
| **Interaction LOD** | 모든 mouse-down / wheel 시작에 `BeginInteractionLod()`, 모든 release 후 `EndInteractionLod()`. wheel hold 가 살아 있으면 release 무시 |
| keyboard arrow | arrow key camera rotation 시 `Arrow Step` 값으로 회전량 결정. `io.WantTextInput \|\| ImGui::IsAnyItemActive()` (텍스트 입력 중) 인 경우 미작동 |
| camera direction drift | 카메라 회전이 발생하면 `setCameraDirection(NOT_ALIGNED)` 호출로 Cell Align 정렬 상태 해제 |

---

# Part 5 - Viewer UI 계약

## 5.1 Viewer 창 계약

| 항목 | legacy 기준 | Phase 3.7 v2 요구 |
|---|---|---|
| 창 title | `Viewer` | 동일 |
| 열림 기본값 | app 시작 시 표시 | 동일. `Windows / Viewer` 로 끌 수 있어야 함 |
| 메뉴 위치 | `Windows / Viewer` | 동일 라벨 + checkbox 동작 |
| 창 close 버튼 | close 시 Windows 메뉴 check 해제 | 동일 (`g_showViewerWindow` 양방향 동기화) |
| content | VTK scene texture | placeholder 문구 금지 |
| toolbar 위치 | Viewer 창 내부 overlay | 동일. 별도 top-level window 로 분리 금지 |
| resize | 창 크기에 따라 scene texture resize | 동일 |
| focus/hover | Viewer 영역에서만 camera interaction | 동일 |
| FPS overlay | Settings toggle 켜진 경우 Viewer 안에 표시 | 동일 |

## 5.2 toolbar 7종 계약

| 순서 | toolbar 항목 | legacy 동작 | Phase 3.7 v2 요구 |
|---:|---|---|---|
| 1 | Boundary Atoms Quick | boundary atoms ON/OFF 토글 | `features::edit::SetBoundaryAtomsEnabled(bool)` 호출. 상태 색/tooltip 유지 |
| 2 | Mesh Display Mode | Wireframe / Shaded / WireShaded popup | popup + state 복구. 실제 mesh 적용은 Phase 3.9 까지 no-op. tooltip: `Mesh display mode applies to mesh actors imported via Phase 3.9.` |
| 3 | Projection | Perspective / Parallel popup | `VtkViewer::SetProjectionMode` 호출 |
| 4 | Reset View | 현재 scene 에 맞게 camera reset | `VtkViewer::ResetView` 또는 `FitViewToVisibleProps` |
| 5 | Cell Align | a1/a2/a3 (또는 BZ mode 의 b1/b2/b3) camera align | cell matrix 변경 금지. `features::edit::AlignCameraToCurrentCellAxis(axis)` 또는 cell matrix getter + `VtkViewer::AlignCameraToCellAxis` |
| 6 | Charge Density Quick | play/pause/restart/level slider 조건부 표시 | `features::data::HasChargeDensity()` 가 true 일 때만 표시. quick API 로 level/animation 갱신 |
| 7 | Arrow Step | 1~180 degree input | `VtkViewer::SetArrowRotateStepDeg`, default 45, clamp [1, 180] |

## 5.3 toolbar 표시 조건

| 항목 | 표시 조건 |
|---|---|
| Boundary Atoms | active structure 존재 시 활성. 구조 없으면 disabled |
| Mesh Display | 항상 표시. mesh feature 미구현 상태에서는 tooltip 으로 안내 |
| Projection | 항상 표시 |
| Reset View | 항상 표시 |
| Cell Align | active structure 의 cell 이 존재할 때만 표시 |
| Charge Density Quick | charge density data 가 있고 surface/volumetric/simple view 에 대응되는 렌더 상태가 있을 때만 |
| Arrow Step | 항상 표시 |

## 5.4 Intentional UI deviation 사전 등록

| 항목 | legacy 동작 | Phase 3.7 v2 계획 | 사유 | 후속 |
|---|---|---|---|---|
| Mesh Display 실제 적용 | mesh actors 전체 display mode 변경 | UI state 와 popup 만 복구. mesh actor 적용은 deferred | Phase 3.9 `features/mesh` 미도착 | Phase 3.9 에서 `features::mesh::SetAllDisplayMode` 연결 |
| Settings 메뉴 전체 | legacy Settings 전체 제공 | `Viewer FPS Overlay` 만 제공 | Phase 4 범위 충돌 방지 | Phase 4 에서 `app/settings` 로 이관 |
| Windows 메뉴 전체 | legacy Windows 전체 제공 | `Viewer` 만 제공 | Model Tree/Layout 등은 후속 phase | Phase 3.8 / 4 에서 확장 |
| 메뉴 순서 | Crystal Viewer | File | Edit | Build | Measurement | Data | Utilities | Settings | Windows | legacy 와 동일 | — | Phase 4 에서 Router 가 보장 |

## 5.5 마우스 / 키보드 입력 계약 (legacy 1:1 보존 필수)

본 § 은 `05_redevelopment_plan.md §6.0.1` 의 "단축키 / 우클릭 메뉴" 보존 의무와 `phase_ui_porting_quality_guideline.md §2.1` 의 "입력 방식 그대로 유지" 의무를 *Viewer 레벨* 에서 *명시적 계약* 으로 고정한다. legacy 출처: `legacy/vtk_viewer.cpp::processEvents()` 라인 1178~1504, `legacy/atoms/atoms_template.cpp` 의 measurement / selection 경로 (3627~4228).

### 5.5.1 카메라 입력 조합 (모든 모드 공통)

| 입력 | 동작 | 구현 의무 |
|---|---|---|
| 좌클릭 + 드래그 (modifier 없음) | 카메라 회전 (Trackball Rotate) | `SetEventInformationFlipY(x, y, ctrl=0, shift=0, dclick)` 후 `InvokeEvent(LeftButtonPressEvent)` |
| **Shift + 좌클릭 드래그** | **카메라 평행이동 (Pan)** | shift=1 을 interactor 에 전달. `vtkInteractorStyleTrackballCamera` 가 자동 분기 |
| 휠클릭 (middle button) 드래그 | 카메라 평행이동 (Pan) | `InvokeEvent(MiddleButtonPressEvent)` |
| 우클릭 드래그 | 줌 (Dolly) | `InvokeEvent(RightButtonPressEvent)`. release 시 `RightButtonReleaseEvent` |
| 휠 스크롤 | 줌 (Dolly). wheel-hold 시간 (legacy 와 동일 timeout) 동안 `InteractionLod` 유지 | `InvokeEvent(MouseWheelForward/BackwardEvent)` |
| **Ctrl + 좌클릭** 또는 **Super + 좌클릭** (비-측정 모드 또는 center 측정 모드) | **선택 경로 (camera 차단)** — 자세한 동작은 5.5.3 참조 | `selectionModifierDown` 분기에서 `vtkInteractorStyleTrackballCamera::OnLeftButtonDown` *호출 금지* |
| 방향키 ↑ / ↓ / ← / → | 카메라 Azimuth/Elevation 만큼 회전. step = `m_ArrowRotateStepDeg` (default 45°, clamp [1, 180]) | `RotateCameraByKeyboard(azimuth, elevation)`. `io.WantTextInput \|\| IsAnyItemActive()` 면 미작동 |
| 마우스 다운 / 휠 | `BeginInteractionLod()` 진입 | wheel hold timer 갱신 |
| 마우스 release + wheel hold 만료 | `EndInteractionLod()` | dirty render 후 정상 LOD 복귀 |
| 카메라 회전 발생 시 | `setCameraDirection(NOT_ALIGNED)` 호출 | Cell Align 토글 상태 해제 |
| Viewer 영역 밖 / `!IsWindowHovered` / 타이틀바 드래그 / `WantCaptureMouse && IsAnyItemHovered` | 모든 scene 입력 차단 | gate 검사 |

### 5.5.2 측정 모드별 픽 / 드래그 / 시각 표시

| 모드 | target pick count | Ctrl-드래그 사각 선택 | 시각 표시 (selected 원자) | 측정 commit 시점 |
|---|---:|---|---|---|
| Distance | **2** | **비활성** | wireframe shell (모드 색상, ambient 1.0, line width 2.0) + 텍스트 actor `#1`, `#2` (font 32, frame, world coord) | 2번째 픽 시 즉시 |
| Angle | **3** | **비활성** | shell + `#1` ~ `#3` | 3번째 픽 시 즉시 |
| Dihedral | **4** | **비활성** | shell + `#1` ~ `#4` | 4번째 픽 시 즉시 |
| GeometricCenter | **무제한** (≥2 commit 가능) | **활성** (`IsMeasurementDragSelectionEnabled()` true) | shell 만 (순번 텍스트 *없음*) | Apply 버튼 또는 Enter / KeypadEnter 키 |
| CenterOfMass | **무제한** (≥2 commit 가능) | **활성** | shell 만 | Apply 또는 Enter |
| 공통 — 빈 영역 클릭 | `HandleMeasurementEmptyClick()` → 누적 픽 클리어 | — | shell / 텍스트 모두 제거 | — |
| 공통 — 이미 선택된 원자 재클릭 | 변동 없음 | — | 동일 | — |
| 공통 — 다른 structure 원자 클릭 | 누적 픽 리셋 후 새 원자만 유지 | — | shell / 텍스트 재구성 | — |
| 공통 — 측정 모드 진입 / 종료 | `clearMeasurementPickVisuals()` + `syncCreatedAtomSelectionVisuals()` + `ClearSelection()` | — | 노란 selection shell ↔ 모드 색상 shell 교체 | — |

> 책임 분리: target pick count, commit, shell/텍스트 actor 생성 자체는 `features/measurement` 책임이다. Phase 3.7 의 책임은 (a) Viewer pick 이벤트가 measurement 로 라우팅되게 하기, (b) 측정 모드 상태에 따라 drag tracking 진입 여부 결정, (c) shell / 텍스트 actor 가 Viewer texture 에 그려지게 하기, (d) drag rectangle 오버레이가 center 모드에서만 표시되게 하기.

### 5.5.3 비-측정 모드 선택 정책 (Ctrl + 클릭 / Ctrl + 드래그)

`selectionModifierDown = io.KeyCtrl || io.KeySuper` 가 **눌려 있을 때에만** 원자 선택이 가능하다. plain 좌클릭은 *카메라 전용* 이라는 강한 계약이다.

| 입력 | 동작 |
|---|---|
| 좌클릭 (modifier 없음, `|Δx|+|Δy| ≤ 4`) | 카메라 forward → release. **빈 영역 hit** 시 `features::edit::ClearCreatedAtomSelection()` 호출. 원자 hit 시 무동작 |
| 좌클릭 드래그 (modifier 없음, `> 4`) | 카메라 회전 (Trackball). release 시 `setCameraDirection(NOT_ALIGNED)` |
| **Ctrl(또는 Super) + 좌클릭** (`≤ 4`) | drag tracking 진입 → release 시 단일 픽 분기: 원자 hit 면 `features::edit::SelectAtomByPicker(actor, pickPos)` (단일 원자 *토글* 선택). 원자 없으면 `features::edit::ClearCreatedAtomSelection()` |
| **Ctrl(또는 Super) + 좌클릭 드래그** (`> 4`) | drag tracking active=true → release 시 `features::edit::HandleDragSelectionInScreenRect(x0, y0, x1, y1, renderer, viewportHeight, additive=true)`. additive 정책: 사각 안 원자 모두 *추가* 선택 |
| **Ctrl(또는 Super) + 더블클릭** | `m_DragSelection.sameElementSelectAtStart = true` 플래그 셋 → release 시 *같은 원소* 모두 선택 (`features::edit::SelectSameElementAtomsByPicker(actor, pickPos)`) |
| drag tracking active 동안 | Viewer 위에 **반투명 파란 사각 오버레이** 그리기. 채움 `IM_COL32(90, 170, 255, 45)`, 외곽선 `IM_COL32(90, 170, 255, 220)`, line width 1.8. `ImDrawList::AddRectFilled` + `AddRect` |
| 선택된 원자 시각 표시 | **노란색 wireframe shell** (`createCreatedAtomSelectionShellActor`): RGB (1.0, 1.0, 0.0), ambient 1.0, diffuse 0.0, specular 0.0, line width 2.0, theta/phi resolution 24, `SetPickable(false)`. shell 반경은 `renderedSelectionSphereRadius(atom)` 결과를 그대로 사용 |
| click vs drag 임계 | `|Δx| + |Δy| > 4` 이상이어야 drag (active=true), 미만이면 click |

### 5.5.4 modifier 와 측정 모드의 상호작용

| 측정 모드 | Ctrl(or Super) + 클릭 | Ctrl + 드래그 |
|---|---|---|
| None | 단일 원자 토글 (5.5.3) | 사각 선택 (5.5.3, additive=true) |
| Distance / Angle / Dihedral | 무시 (drag tracking 미진입, plain click 분기로 fall-through. 결국 측정 pick 으로 처리됨) | **비활성**. plain click 의 측정 pick 분기로 처리 |
| GeometricCenter / CenterOfMass | drag tracking 진입. release 시 *측정* pick 으로 라우팅 (`HandleMeasurementClickByPicker`) | drag rectangle → `HandleDragSelectionInScreenRect` (measurement 분기, additive) |

> 핵심: `selectionModifierDown` 가 true 라도 측정 모드가 `IsMeasurementDragSelectionEnabled()=false` 면 drag tracking 을 진입하지 *않고* 측정 pick 경로로 직행한다. 이 분기 보존이 필수다.

## 5.6 렌더 스타일 회귀 보존 (Phase 3.4 산출물 점검 의무)

> 본 § 의 항목은 *Phase 3.7 의 수정 대상이 아니다*. Phase 3.4 산출물 (`features/edit/atoms/atom_renderer.cpp`, `features/edit/bonds/bond_renderer.cpp`) 에서 legacy 와 다르게 구현된 부분은 **Phase 3.7 진입 *이전* 에 별도 회귀 PR ([`./phase3_4_renderer_regression.md`](./phase3_4_renderer_regression.md)) 로 수정** 되어야 한다. Phase 3.7 의 책임은 (a) 회귀 PR 머지 확인, (b) Viewer 가 살아나는 시점에 본 § 의 매트릭스를 PASS 로 갱신, (c) 잔여는 DEVIATION 으로 기록 후 후속 PR 발의다. 출처: `legacy/atoms/infrastructure/vtk_renderer.cpp`, `legacy/atoms/domain/{atom_manager,bond_manager,element_database}.cpp`, `legacy/atoms/ui/bond_ui.cpp`.

### 5.6.1 원자 sphere 스타일 계약

| 항목 | Legacy 기준 | Phase 3.7 점검 의무 |
|---|---|---|
| Base geometry | `vtkSphereSource` | 코드 확인 |
| **Base radius** | **0.5** (`vtk_renderer.cpp:102` `createSphereGeometry(0.5f)`) | 코드 확인. 회귀 후 `atom_renderer.cpp` 의 `SetRadius(0.5)` 또는 등가 산식 |
| **Per-atom 스케일 산식** | Transform.Scale(s, s, s), `s = (atom.radius * 0.5) / 0.5 = atom.radius` → **유효 반경 `atom.radius * 0.5`** (`atom_manager.cpp:113~119`) | imported XSF 의 sphere 크기를 legacy 스크린샷과 시각 비교 |
| Theta / Phi resolution | **20 / 20** (`vtk_renderer.cpp:61` `m_sphereResolution=20`) | 코드 확인 |
| Per-element 기본 반경 | `ElementDatabase::getDefaultRadius(symbol)` → `info->covalentRadius` 또는 1.0 폴백 (`element_database.cpp:71~73`) | 코드 확인 |
| Min radius clamp | `kMinAtomRadius = 0.001f` (`atom_manager.cpp:60`) | 코드 확인 |
| Per-element 기본 색상 | `info->defaultColor` 또는 (0.7, 0.7, 0.7, 1.0) 회색 폴백 | 코드 확인 |
| Pickable | **true** (`vtk_renderer.cpp:889`) | 코드 확인 |
| Edge visibility | **false** (`vtk_renderer.cpp:894`) | 코드 확인 |
| **Ambient** | **0.3** (`vtk_renderer.cpp:895`) | 코드 확인 + side-by-side highlight 비교 |
| **Diffuse** | **0.7** (`vtk_renderer.cpp:896`) | 동일 |
| **Specular** | **0.1** (`vtk_renderer.cpp:897`) | 동일 |
| **SpecularPower** | **10** (`vtk_renderer.cpp:898`) | 동일 |
| Opacity | 1.0 (sphere actor 기본) | 코드 확인 |
| 그룹핑 방식 | 같은 symbol 단일 actor + per-cell color array. (legacy 는 `vtkAppendPolyData`, 신규 트리는 `vtkGlyph3D` 가능. 결과 동등이면 OK) | 시각 비교 |
| **선택 시각** | **개별 노란 wireframe shell actor** (`(1.0, 1.0, 0.0)`, line width 2.0, ambient 1.0, `Pickable=false`). 본체 sphere actor 의 *색상은 변경하지 않음* | imported atom 의 선택 시 같은 원소의 *다른* 원자가 색을 *유지* 함을 확인 |

### 5.6.2 결합 cylinder 스타일 계약 (2-color)

| 항목 | Legacy 기준 | Phase 3.7 점검 의무 |
|---|---|---|
| Base geometry | `vtkCylinderSource`, height **1.0**, Y축 정렬 (`vtk_renderer.cpp:872~881`) | 코드 확인 |
| Resolution | **20** (`m_sphereResolution` 공유, `vtk_renderer.cpp:876`) | 코드 확인 |
| Base radius | `radius * m_bondThickness`. `m_bondThickness = 1.0` (default) (`vtk_renderer.cpp:62, 320~321`) | 코드 확인 |
| Per-bond-type 기본 radius | `atom.bondRadius = max(getDefaultRadius(symbol), 0.001)` (`atom_manager.cpp:106, 427~443`) | 코드 확인 |
| **2-color split** | 두 별도 actor (`actor1`, `actor2`) — atom1 측 half / atom2 측 half (`vtk_renderer.cpp:306~358`) | 코드 확인 + 시각 비교 |
| **Split 위치 (핵심)** | **`ratio = radius1 / (radius1 + radius2)` proportional split**. `splitCenter = position1 + ratio * (position2 - position1)` (`bond_manager.cpp:194~208`). **정확히 1/2 가 아님** | Si–O 등 비대칭 결합에서 색 경계가 작은 원자(O) 측에 더 가까이 표시됨을 확인 |
| 각 half center | `centerAC = (position1 + splitCenter) / 2`, `centerCB = (splitCenter + position2) / 2` (`bond_manager.cpp:202~208`) | 시각 비교 |
| 각 half height | `halfHeight = bondLength / 2`, `Transform.Scale(1, halfHeight, 1)` (`bond_manager.cpp:200, 264, 286`) | 시각 비교 |
| 방향 frame | Orthonormal: v3 = bond direction, v2 = normalize(v3 × v1), v1 = normalize(v2 × v3). matrix col0=v2, col1=v3, col2=v1, col3=center (`bond_manager.cpp:210~286`). 수학적 등가 구현 (예: `RotateWXYZ` + `Scale`) 도 PASS | 시각 비교 |
| Pickable | **false** (의도. `vtk_renderer.cpp:333, 346`) | 코드 확인 |
| Edge visibility | **false** | 코드 확인 |
| Ambient / Diffuse / Specular / SpecularPower | **0.3 / 0.7 / 0.1 / 10** (atom 과 동일) | 코드 확인 + highlight 비교 |
| **Default opacity** | **1.0**, `actor->ForceOpaqueOn()` (`vtk_renderer.cpp:903~907`) | 코드 확인 |
| **Opacity < 1.0 분기** | `property->SetRenderLinesAsTubes(true)` + `actor->ForceTranslucentOn()` (`vtk_renderer.cpp:902~906`) | UI 슬라이더 0.5 시 반투명 + tube 렌더 확인 |
| `bondThickness` 슬라이더 | 0.1 ~ 3.0, default 1.0, "Reset" 버튼 우측 (`bond_ui.cpp:78~92`) | UI 라벨 / 범위 / 동작 / 즉시 반영 확인 |
| `bondOpacity` 슬라이더 | 0.1 ~ 1.0, default 1.0, "Reset" 버튼 우측 (`bond_ui.cpp:104~118`) | 동일 |
| `Distance factor` 슬라이더 | 0.1 ~ 2.0 (`bond_ui.cpp:132~`) | 동일 |
| Color1 / Color2 | atom1 / atom2 의 `elementDB.getDefaultColor` 또는 사용자 지정 | 코드 확인 |

### 5.6.3 측정 픽 시각 (Phase 3.5 산출물) — § 5.5.2 와 중복 검증 가능

| 항목 | Legacy 기준 | Phase 3.7 점검 의무 |
|---|---|---|
| Shell actor | wireframe sphere, 모드 색상, ambient 1.0, diffuse 0.0, specular 0.0, line width 2.0, theta/phi **24** (`atoms_template.cpp:1024~1053`), `SetPickable(false)` | imported atom 측정 시 shell 표시 |
| `#N` 텍스트 actor | `vtkTextActor`, font 32, frame box, world coord, **Distance / Angle / Dihedral 에서만 표시**. GeometricCenter / CenterOfMass 미표시 (`atoms_template.cpp:4078~4108`) | 측정 모드별 텍스트 표시 여부 확인 |

> § 5.6 의 *모든* 항목은 Phase 3.4 회귀 PR 의 책임이다. Phase 3.7 은 *검증 의무* 만 진다. PASS 가 아니면 DEVIATION 으로 평가서에 기록하고 추가 회귀 PR 발의 권장.

---

# Part 6 - 구현 순서

## 6.1 Step 0 - 정적 조사 / 기준 캡처

| 작업 | 산출물 |
|---|---|
| legacy `vtk_viewer.cpp` 의 `Render`, framebuffer, event, FPS overlay, camera 함수 위치 정리 | 구현 메모 또는 PR 본문 |
| legacy `toolbar.cpp` 의 버튼 순서/label/tooltip/표시조건 표 추출 | 본 계획서 Part 5 와 대조 |
| 현 `core/vtk::VtkViewer` 의 public API 와 actor 사용처 정리 | API 보강 diff 최소화 |
| Phase 3.6 (#28) 입력 파일 시나리오 정리 (XSF / CHGCAR / XSF Grid 각 1~2개) | 런타임 검증 입력 |
| 직전 baseline wasm size 측정 | `Get-Item public\wasm\VTK-Workbench.wasm` → 15,245,923B 인계 |

## 6.2 Step 1 - `features/viewer` skeleton 추가

| 작업 | 산출물 |
|---|---|
| `features/viewer/viewer_menu.{h,cpp}` 신설 | `InitOnce`, `DrawMenus`, `RenderWindows`, `Shutdown`, `HandleRequest` (필요 시) |
| `features/viewer/viewer_panel.{h,cpp}` 신설 | `ViewerPanel::Render(bool* open)` |
| `features/viewer/toolbar.{h,cpp}` 신설 | toolbar state + render 함수 |
| `features/viewer/viewer_types.h` 신설 | toolbar anchor, mesh display mode UI enum |
| `CMakeLists.txt` 등록 | build source list 갱신 |
| `app/app.cpp` hook 추가 | 기존 direct `VtkViewer::Render()` 제거 + feature window render 로 교체 |

## 6.3 Step 2 - core viewer texture 렌더 복구

| 작업 | 세부 |
|---|---|
| WebAssembly render window 전환 | 필요 시 `vtkWebAssemblyOpenGLRenderWindow` 사용 |
| framebuffer lifecycle | init / resize / clear / delete 를 RAII 로 보강 |
| `ImGui::Image` 표시 | `ViewerPanel` 이 계산한 viewport size 로 color texture 표시 |
| dirty render | `RequestRender`, resize, interaction 시 render |
| actor 경로 보존 | 기존 `AddActor`, `RemoveActor`, `AddActor2D`, `RemoveActor2D` 호출부 미변경 |
| camera reset | `ResetView`, `FitViewToVisibleProps` 동작 |

## 6.4 Step 3 - event / picker 복구

| 작업 | 세부 |
|---|---|
| mouse position 변환 | ImGui mouse pos → Viewer viewport local pos |
| VTK interactor feeding | left down/up/move/wheel/key event 전달 |
| toolbar click 차단 | toolbar hovered 영역의 camera drag/pick 차단 |
| pick 검증 | `MouseInteractor::emitPickOrEmptyClick` 가 imported atom actor 를 pick |
| drag 검증 | drag selection rectangle 과 viewport height 전달 |
| render request | 모든 interaction 후 `RequestRender()` |

## 6.5 Step 4 - toolbar 7종 구현

| 항목 | 구현 |
|---|---|
| Boundary Atoms | `features::edit::IsBoundaryAtomsEnabled() / SetBoundaryAtomsEnabled(bool)` |
| Mesh Display | `features/viewer` 내부 state. Phase 3.9 까지 `features::mesh::*` 호출 0 |
| Projection | `core::vtk::VtkViewer::SetProjectionMode / GetProjectionMode` |
| Reset View | `core::vtk::VtkViewer::ResetView` 또는 `FitViewToVisibleProps` |
| Cell Align | `features::edit::AlignCameraToCurrentCellAxis(axis)` 또는 cell matrix getter + `VtkViewer::AlignCameraToCellAxis(cell, axis)` |
| Charge Density Quick | `features::data::HasChargeDensity / SetChargeDensityLevelPercent / IsQuickAnimationActive / StartQuickAnimation / StopQuickAnimation / RestartQuickAnimation` |
| Arrow Step | `core::vtk::VtkViewer::GetArrowRotateStepDeg / SetArrowRotateStepDeg` |

## 6.6 Step 5 - Settings / Windows menu hook

| 메뉴 | 구현 |
|---|---|
| `Settings / Viewer FPS Overlay` | `features::viewer::DrawMenus()` 에서 `Settings` 메뉴 진입 후 `MenuItem` 체크박스. `VtkViewer::SetPerformanceOverlayEnabled` 호출 |
| `Windows / Viewer` | 같은 함수에서 `Windows` 메뉴 진입 후 `g_showViewerWindow` 토글. `Viewer` 창 close 버튼과 양방향 동기화 |
| 메뉴 순서 | 기존 메뉴 뒤에 `Settings`, `Windows` 순서 추가. Phase 4 전 임시 hook 임을 코드 주석에 명시 |

## 6.7 Step 6 - Phase 3.6 entry gate 해소

| Entry gate | 작업 |
|---|---|
| #28 Measurement on imported atoms | XSF 1개 + CHGCAR 1개 import 후, Distance / Angle 2종 픽킹 시나리오 통과. drag selection 좌표계 확인 |
| #15 progress popup side-by-side | Viewer 복구 후 imported XSF/CHGCAR import 진행 중 progress popup 의 show / hide / percentage 표시를 legacy 와 새 트리에서 동시에 캡처. 평가서 부록에 첨부 |
| #8 FormatRegistry 비악화 | `git diff -- webassembly/src/features/file webassembly/src/core/io` 의 diff 가 없음을 정적 검증 |

## 6.8 Step 7 - 빌드 / size / 평가서 작성

| 작업 | 세부 |
|---|---|
| `npm run build-wasm:debug` | exit 0 |
| `npm run build-wasm:release` | exit 0 |
| wasm size 측정 | baseline 15,245,923B (2026-05-13) 대비 delta 기록 |
| 평가서 작성 | `phase3_7_evaluation_YYYY-MM-DD.md`. entry gate 전용 섹션 포함 |

---

# Part 7 - 검증 매트릭스

## 7.1 정적 / 구조 검증 (#1 ~ #10)

| # | 검증 항목 | 기대값 | 방법 | 판정 기준 |
|---:|---|---|---|---|
| 1 | 신규 viewer 파일 | `features/viewer/` 7개 내외 | `rg --files webassembly/src/features/viewer` | PASS / FAIL |
| 2 | legacy 수정 0 | `webassembly/src/legacy/` 변경 0 (line-ending 정규화 외) | `git diff -- webassembly/src/legacy` | PASS / FAIL |
| 3 | legacy include 0 | 신규/보강 코드에서 `legacy/` include 0 | `rg -n "legacy|src/legacy|#include.*legacy" webassembly/src/features/viewer webassembly/src/core/vtk` | PASS / FAIL |
| 4 | app hook | `InitOnce`, `DrawMenus`, `RenderWindows` 호출 존재 | `rg -n "features::viewer" webassembly/src/app/app.cpp` | PASS / FAIL |
| 5 | direct placeholder 제거 | "Phase 1 bootstrap" 텍스트 제거 | `rg -n "Phase 1 bootstrap\|Rendering features will be restored" webassembly/src/core/vtk` (hit 0) | PASS / FAIL |
| 6 | CMake 등록 | 신규 viewer files + `core/vtk/viewer_types.h` 등록 | `rg -n "features/viewer\|viewer_types" CMakeLists.txt` | PASS / FAIL |
| 7 | core viewer API | projection/reset/FPS/arrow step API 존재 | `rg -n "SetProjectionMode\|ResetView\|SetPerformanceOverlayEnabled\|SetArrowRotateStepDeg\|AlignCameraToCellAxis\|DrawRenderTexture" webassembly/src/core/vtk` | PASS / FAIL |
| 8 | edit quick API | boundary atoms + cell align wrapper 존재 | `rg -n "BoundaryAtomsEnabled\|AlignCameraToCurrentCellAxis" webassembly/src/features/edit` | PASS / FAIL |
| 9 | data quick API | charge density quick wrapper 존재 | `rg -n "HasChargeDensity\|SetChargeDensityLevelPercent\|QuickAnimation" webassembly/src/features/data` | PASS / FAIL |
| 10 | File hot path 미침범 | Phase 3.6 의 `features/file/` 및 `core/io/` 코드 변경 0 | `git diff --stat -- webassembly/src/features/file webassembly/src/core/io` | PASS (변경 0 또는 deviation 기록) / FAIL |

## 7.2 빌드 / 정량 검증 (#11 ~ #13)

| # | 검증 항목 | 기대값 | 명령 | 판정 |
|---:|---|---|---|---|
| 11 | debug build | exit 0 | `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"` | PASS / FAIL |
| 12 | release build | exit 0 | `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"` | PASS / FAIL |
| 13 | wasm size 기록 | baseline 15,245,923B (2026-05-13) 대비 delta | `Get-Item public\wasm\VTK-Workbench.wasm` | PASS / WATCH (>+3% 시 분석 요) |

## 7.3 런타임 / 시각 검증 (#14 ~ #32)

| # | 검증 항목 | 기대값 | 판정 기준 |
|---:|---|---|---|
| 14 | Viewer 기본 표시 | 앱 시작 후 `Viewer` 창 표시 | placeholder 문구 0, VTK background/actors 표시 |
| 15 | Windows / Viewer | 체크 해제 시 Viewer 숨김, 체크 시 재표시 | menu checkbox ↔ window close 양방향 동기화 |
| 16 | Settings / Viewer FPS Overlay | 체크 시 FPS overlay 표시, 해제 시 숨김 | console error 0 |
| 17 | resize | Viewer 창 resize 후 texture stretch/black screen 없음 | actor + background 정상 표시 |
| 18 | Projection toolbar | Perspective ↔ Parallel 전환 | camera projection 변화 시각 확인 |
| 19 | Reset View toolbar | actor 가 화면 중앙 / 적절한 bounds 로 복귀 | XSF / FCC / CHGCAR actor 기준 |
| 20 | Cell Align toolbar | a1 / a2 / a3 클릭 시 camera 방향 변경 | cell matrix 미변경 (검증: 이전후 cell vector 동일) |
| 21 | Boundary Atoms toolbar | ON / OFF 토글 시 boundary atoms 표시 변화 | Edit / Atoms checkbox 상태와 일치 |
| 22 | Charge Density Quick | data loaded 상태에서 quick control 표시 / 동작 | level 변경 시 surface/volume/slice 렌더 갱신 |
| 23 | Arrow Step toolbar | 1 ~ 180 clamp, arrow rotation step 반영 | keyboard arrow 회전량 변경 |
| 24 | Mesh Display toolbar | popup / state 변경 / no-op 안내 tooltip | Phase 3.9 deferred 안내 표시, error 0 |
| 25 | XSF imported actor 표시 | Phase 3.6 import 구조가 Viewer 에 표시 | atom / cell actor visible |
| 26 | CHGCAR imported actor / data 표시 | imported atoms + density render 연동 | Data viewer + toolbar quick control 정상 |
| 27 | **#28 entry gate** Measurement on imported atoms | imported atoms 위에서 Distance / Angle 생성 | Phase 3.6 #28 해소. PASS / FAIL 명시 |
| 28 | drag selection on imported atoms | Viewer drag 로 selection event 발생 | Measurement center modes 또는 selection UI 와 연동 |
| 29 | **#15 entry gate** progress popup side-by-side | legacy 와 새 트리의 show / hide / percentage 일치 | 평가서 부록에 캡처 첨부 |
| 30 | console error 0 | runtime JS / C++ console error 0 | DevTools 기준 |
| 31 | repeated open/close | Viewer close/open 반복 후 texture/event 정상 | context/framebuffer leak 증상 0 |
| 32 | menu coexistence | File / Edit / Build / Measurement / Data / Utilities 미영향 | Phase 3.6 PASS 시나리오 재확인 |
| 33 | **5.5.1** 좌클릭 드래그 = 카메라 회전 | legacy 와 시각적 동일 trackball rotation | PASS / FAIL |
| 34 | **5.5.1** Shift + 좌클릭 드래그 = Pan | legacy 와 동일 평행이동. `SetEventInformationFlipY` 의 shift=1 전달 확인 | PASS / FAIL |
| 35 | **5.5.1** 우클릭 드래그 / 휠 = Dolly + LOD | wheel hold timer 동안 `InteractionLod` 유지, release 후 정상 LOD 복귀 | PASS / FAIL |
| 36 | **5.5.3** Ctrl + 좌클릭 → 단일 원자 토글 | imported atom 위에서 노란 wireframe shell 표시 / 재클릭 시 해제. 카메라 회전이 *동시 발생하지 않음* | PASS / FAIL |
| 37 | **5.5.3** Ctrl + 좌클릭 드래그 → 사각 선택 (additive) | 파란 반투명 사각 오버레이 표시 (`IM_COL32(90,170,255,...)`) + release 시 사각 내 원자 모두 노란 shell. 기존 선택 *유지* (additive) | PASS / FAIL |
| 38 | **5.5.3** Ctrl + 더블클릭 → 같은 원소 전부 선택 | 동일 원소 atom 모두 노란 shell. 다른 원소 미선택 | PASS / FAIL |
| 39 | **5.5.3** 비-측정 모드 빈 영역 클릭 → `ClearCreatedAtomSelection` | 모든 shell 제거. modifier 유무와 무관 | PASS / FAIL |
| 40 | **5.5.2** Distance 모드 2픽 즉시 측정 생성 + `#1`/`#2` 텍스트 | shell + 텍스트 동시 표시. 2번째 픽 시 측정 추가 + 카운터 리셋 | PASS / FAIL |
| 41 | **5.5.2** Angle 3픽 / Dihedral 4픽 | 동일 패턴. target count 도달 시 즉시 측정 생성 | PASS / FAIL |
| 42 | **5.5.2** GeometricCenter / CenterOfMass = 무제한 픽 + Apply / Enter commit + Ctrl-drag 활성 | 사각 오버레이 표시 → 사각 내 원자가 픽 누적에 추가. Apply 또는 Enter 로 commit | PASS / FAIL |
| 43 | **5.5.4** Distance / Angle / Dihedral 에서 Ctrl-drag = 비활성 | 사각 오버레이 미표시. 단일 픽 누적 경로 유지 | PASS / FAIL |
| 44 | **5.5.2** 측정 모드 빈 클릭 → `HandleMeasurementEmptyClick` | shell / 텍스트 모두 제거. 누적 픽 클리어 | PASS / FAIL |
| 45 | **5.5.1** Arrow Step 변경 후 방향키 회전량 변동 | step=1°, 45°, 180° 각각 비교. text input 중 미작동 확인 | PASS / FAIL |
| 46 | **§ 5.6.1** atom sphere 유효 반경 = `atom.radius * 0.5` | imported XSF 의 Si–Si 결합 길이 대비 sphere 크기가 legacy 스크린샷과 일치 | PASS / DEVIATION |
| 47 | **§ 5.6.1** atom shading (ambient 0.3 / diffuse 0.7 / specular 0.1 / specularPower 10) | legacy 와 동일한 highlight / 반사감 | PASS / DEVIATION |
| 48 | **§ 5.6.1** atom 선택 시각 = 개별 노란 wireframe shell | Ctrl + 클릭 후 같은 원소의 *다른* 원자는 색이 *바뀌지 않음* | PASS / DEVIATION |
| 49 | **§ 5.6.2** bond 2-color split 위치 = `radius1/(radius1+radius2)` proportional | Si–O 등 비대칭 결합에서 색 경계가 작은 원자(O) 측에 더 가까움 | PASS / DEVIATION |
| 50 | **§ 5.6.2** bond shading (ambient 0.3 / diffuse 0.7 / specular 0.1 / specularPower 10) | legacy 와 동일 | PASS / DEVIATION |
| 51 | **§ 5.6.2** bond opacity slider (0.1 ~ 1.0) 작동 + < 1.0 시 `RenderLinesAsTubes` + `ForceTranslucentOn` | Edit / Bonds 의 Opacity 슬라이더로 0.5 설정 시 즉시 반투명 + tube 렌더 | PASS / DEVIATION |
| 52 | **§ 5.6.2** `bondThickness` 슬라이더 (0.1 ~ 3.0, default 1.0, Reset 우측) | UI 위치 / 라벨 / 범위 / 즉시 반영 legacy 와 동일 | PASS / DEVIATION |
| 53 | **§ 5.6.2** `Distance factor` 슬라이더 (0.1 ~ 2.0) | legacy 동일 | PASS / DEVIATION |

> #46 ~ #53 의 판정 정책: Phase 3.4 회귀 PR ([`./phase3_4_renderer_regression.md`](./phase3_4_renderer_regression.md)) 이 머지된 후 Phase 3.7 에 진입했다면 **PASS** 가 기대값. 회귀 PR 이 *부분 머지* 또는 *미머지* 상태로 진입했다면 잔여 항목은 **DEVIATION** 으로 평가서에 기록한다. **FAIL** 표기는 본 phase 에서 사용하지 않는다 (Phase 3.7 은 renderer 의 수정 책임이 없으므로).

## 7.4 UI 지침 매트릭스 (UI-01 ~ UI-12)

| UI ID | 검증 항목 | Phase 3.7 적용 |
|---|---|---|
| UI-01 | 옵션명 / 표시순서 일치 | toolbar 7종 순서 + Settings / Windows 메뉴 label |
| UI-02 | 기본값 / 범위 일치 | Projection default, Arrow Step 45, clamp [1, 180] |
| UI-03 | 입력방법 일치 | popup / button / checkbox / input int 유지 |
| UI-04 | 조건부 표시 규칙 일치 | Cell Align, Charge Density Quick 조건부 표시 |
| UI-05 | 활성 / 비활성 규칙 일치 | Mesh 미구현 no-op/deferred, data 없음 quick hidden |
| UI-06 | Apply / Direct Apply 시점 일치 | click / slider / input 즉시 반영 |
| UI-07 | 공통옵션 공유 연동 일치 | Data UI level/state ↔ quick toolbar state 동기화 |
| UI-08 | 색상 선택도구 위치 / 동작 일치 | 본 phase 신규 색상 도구 없음 |
| UI-09 | 다중 데이터 선택 UI 일치 | Phase 3.6 XSF Grid selector 유지, toolbar 는 active data 만 참조 |
| UI-10 | ImGui ID 충돌 0 | toolbar popup/button/input ID suffix 확인 |
| UI-11 | 실제 파일 로드 렌더 가능 | XSF / CHGCAR import 후 Viewer 표시 |
| UI-12 | 빈 데이터 테스트 훅 동작 | data 없음 상태에서 toolbar quick controls 안전 hidden/disabled |

---

# Part 8 - 완료 기준 (Definition of Done)

| DoD 항목 | 기준 |
|---|---|
| Viewer 창 | `Windows / Viewer` 토글과 close 버튼이 양방향 동기화, 실제 VTK scene texture 표시 |
| toolbar | 7종이 legacy 순서로 표시. Phase 3.9 deferred 인 Mesh Display 외 6종이 실제 public API 와 연결 |
| FPS overlay | `Settings / Viewer FPS Overlay` 로 표시 / 숨김 가능 |
| actor 표시 | Phase 3.3 ~ 3.6 feature actors 가 Viewer 안에서 보임 |
| pick / drag | Viewer 좌표 기반 click / drag 가 `MouseInteractor` 를 통해 feature event 로 전달 |
| **카메라 입력 계약 (§ 5.5.1)** | Shift+드래그 Pan, 우클릭/휠 Dolly, Ctrl+좌클릭 = 카메라 차단 후 선택 경로, 방향키 회전 모두 legacy 와 동일 동작 |
| **측정 입력 계약 (§ 5.5.2 / § 5.5.4)** | 모드별 target pick count 2/3/4/무제한 보존, GeometricCenter/CenterOfMass 만 drag 활성, shell + `#N` 텍스트 actor 시각 표시 보존 |
| **비-측정 선택 계약 (§ 5.5.3)** | Ctrl+클릭 토글, Ctrl+드래그 사각 선택 (additive), Ctrl+더블클릭 same-element, 4px 임계, 파란 반투명 drag rectangle 오버레이, 노란 wireframe shell 시각 표시 |
| **렌더 스타일 회귀 점검 (§ 5.6)** | Phase 3.4 회귀 PR ([`./phase3_4_renderer_regression.md`](./phase3_4_renderer_regression.md)) 머지 확인 + #46 ~ #53 PASS 또는 DEVIATION 기록 |
| **#28 해소** | imported atoms 위 Measurement Distance / Angle PASS. FAIL 시 명확한 신규 blocker 기록 후 phase exit 불가 |
| **#15 보강** | progress popup side-by-side 시각 검증 / 캡처 첨부 또는 잔여 리스크 명시 |
| **#8 비악화** | `features/file/` 및 `core/io/` 변경 0 (또는 명시적 deviation 기록) |
| legacy 격리 | legacy source / header 직접 의존 0 |
| build | debug / release wasm build PASS |
| runtime | console error 0 |
| 평가서 | Phase 3.7 평가서에 PASS/PARTIAL/BLOCKED/FAIL 집계 + entry gate 전용 섹션 + wasm size delta 기록 |

---

# Part 9 - 리스크와 대응

| # | 리스크 | 영향 | 대응 |
|---:|---|---|---|
| R1 | WebAssembly render window 교체로 build 오류 | Viewer 복구 지연 | legacy `vtk_viewer.cpp` 의 include + CMake link 상태를 기준으로 최소 이식 |
| R2 | ImGui texture vs VTK framebuffer y-axis 불일치 | 화면 상하 반전 또는 picker 좌표 오류 | legacy `ImGui::Image` UV, picker y 보정 로직을 그대로 대조 |
| R3 | toolbar 클릭이 VTK pick 으로 전달 | 버튼 클릭 시 measurement 오작동 | toolbar hovered 영역에서 event feeding 차단 |
| R4 | Cell Align 이 cell matrix 변경 | 데이터 손상 | `CellManager::AlignAxis` 사용 금지, camera-only action 신규 wrapper |
| R5 | charge density quick API 부족 | toolbar 6번 구현 지연 | Data feature 에 최소 wrapper 추가. UI 본체 변경 0 |
| R6 | Mesh Display 실제 적용 불가 | toolbar 7종 중 1종 PARTIAL 가능 | Phase 3.9 deferred 를 계획서/평가서에 명시. 안정적 no-op UI 제공 |
| R7 | Viewer render 매 frame 과도 비용 | FPS 저하 | dirty flag + FPS overlay 로 상태 확인. 필요 시 interaction 중 LOD 만 최소 이식 |
| R8 | Phase 3.6 File import 회귀 | 이미 PASS 한 file runtime 손상 | `features/file/` + `core/io/` 변경 0 원칙 (정적 검증 #10). 발생 시 별도 deviation |
| R9 | wasm size 증가 | release artifact 비대화 | Phase 3.7 release build 후 size delta 기록. baseline 15,245,923B (2026-05-13). 급증 시 framebuffer/texture 중복 보관 점검 |
| R10 | entry gate #28 미해소 | DoD 미달 | Phase 3.7 exit 불가. 미해소 사유 및 후속 phase blocker 평가서에 기록 |
| R11 | progress popup side-by-side 캡처 환경 부족 | #15 보강 지연 | 캡처 도구 / 디스플레이 조합 미리 준비. 캡처 실패 시 잔여 리스크 명시 |

---

# Part 10 - 평가서 작성 지침

Phase 3.7 완료 후 평가서는 `webassembly/docs/phases/phase3_7_evaluation_YYYY-MM-DD.md` 로 작성한다. 본 v2 의 평가 양식 요구는 v1 보다 강화된다.

| 평가 섹션 | 포함 내용 |
|---|---|
| 0. 집중 결론 | Viewer/toolbar 복구 여부, #28 / #15 / #8 entry gate 결과, GO / PARTIAL / BLOCKED 판단 |
| 1. 구현 개요 | 신규 파일 / 수정 파일 / 라인수 (`wc -l` 기준) / legacy 격리 상태 |
| 2. 검증 매트릭스 | 본 계획서 Part 7 의 #1 ~ #32 결과 |
| 3. UI 지침 결과 | UI-01 ~ UI-12 결과 + Intentional UI deviation 기록 |
| 4. **Phase 3.6 entry gate 전용 섹션** | #28 (PASS/FAIL), #15 (캡처 첨부 / 미수행), #8 (변경 0 / deviation) |
| 5. 리스크 | 미해결 항목 + 다음 phase 연결 |
| 6. 명령 요약 | build / test / static check / size 명령과 결과 |
| 7. wasm size | baseline 15,245,923B (2026-05-13) 대비 delta. 후속 baseline 갱신 |

---

# Part 11 - 사용 명령 요약

```powershell
git status --short --branch
rg --files webassembly\src\features\viewer
rg -n "legacy|src/legacy|#include.*legacy" webassembly\src\features\viewer webassembly\src\core\vtk
rg -n "features::viewer|viewer_menu" webassembly\src\app\app.cpp CMakeLists.txt
rg -n "SetProjectionMode|ResetView|SetPerformanceOverlayEnabled|SetArrowRotateStepDeg|AlignCameraToCellAxis|DrawRenderTexture" webassembly\src\core\vtk
rg -n "BoundaryAtomsEnabled|AlignCameraToCurrentCellAxis" webassembly\src\features\edit
rg -n "HasChargeDensity|SetChargeDensityLevelPercent|QuickAnimation" webassembly\src\features\data
rg -n "Phase 1 bootstrap|Rendering features will be restored" webassembly\src\core\vtk
git diff --stat -- webassembly/src/features/file webassembly/src/core/io
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"
Get-Item public\wasm\VTK-Workbench.wasm
# baseline (2026-05-13): 15,245,923 B
```

---

# Part 12 - PR 체크리스트

## 작성자

### 정적 / 구조
- [ ] `features/viewer/` 7파일 추가 (`viewer_menu.{h,cpp}`, `viewer_panel.{h,cpp}`, `toolbar.{h,cpp}`, `viewer_types.h`)
- [ ] `core/vtk/vtk_viewer.{h,cpp}` 의 placeholder 텍스트 제거 + WebAssembly 텍스처 렌더 API 추가
- [ ] `core/vtk/viewer_types.h` 신규 (projection / camera direction enum)
- [ ] `core/vtk/mouse_interactor.*` 보강 (필요 시)
- [ ] `features/edit/edit_menu.*` 에 boundary atoms + cell-axis camera align wrapper 추가
- [ ] `features/data/data_menu.*` 에 charge density quick wrapper 추가
- [ ] `app/app.cpp` 에 `features/viewer` include / `InitOnce` / `DrawMenus` / `RenderWindows` hook 추가 + 기존 direct `VtkViewer::Render()` 호출 제거
- [ ] `CMakeLists.txt` 에 신규 viewer source / viewer_types.h 등록
- [ ] `webassembly/src/legacy/` 변경 0 (line-ending 외)
- [ ] `features/viewer` / `core/vtk` 보강 코드에 `legacy/` include 0
- [ ] `features/file/` + `core/io/` 변경 0 (Phase 3.6 비침범)

### 빌드 / 정량
- [ ] `npm run build-wasm:debug` PASS
- [ ] `npm run build-wasm:release` PASS
- [ ] release `VTK-Workbench.wasm` size 기록 (baseline 15,245,923B 대비 delta)

### 런타임
- [ ] Viewer 창 표시 / Windows-Viewer 토글 동기화
- [ ] Settings / Viewer FPS Overlay 토글 동작
- [ ] toolbar 7종 배치 / 동작 (Mesh Display 는 deferred no-op)
- [ ] XSF / CHGCAR import 후 actor Viewer 표시
- [ ] console error 0
- [ ] repeated open/close 안정성

### 입력 계약 (§ 5.5)
- [ ] `core/vtk::MouseInteractor` 가 legacy `processEvents()` state machine 을 이식 (4px 임계, modifier gating, drag tracking, measurement-aware routing)
- [ ] `SetEventInformationFlipY(x, y, ctrl, shift, dclick)` 호출로 modifier 전달 (Shift Pan, Wheel Dolly 확인)
- [ ] Ctrl(or Super) + 좌클릭 = 단일 토글 (`SelectAtomByPicker`)
- [ ] Ctrl(or Super) + 좌클릭 드래그 = `HandleDragSelectionInScreenRect(..., additive=true)` + 파란 반투명 사각 오버레이
- [ ] Ctrl(or Super) + 더블클릭 = `SelectSameElementAtomsByPicker`
- [ ] 측정 모드 Distance / Angle / Dihedral 에서 Ctrl-drag = 비활성 확인
- [ ] 측정 모드 GeometricCenter / CenterOfMass 에서 Ctrl-drag = 활성 + Apply / Enter commit 확인
- [ ] 비-측정 빈 클릭 → `ClearCreatedAtomSelection`, 측정 빈 클릭 → `HandleMeasurementEmptyClick` 분기 보존
- [ ] 선택된 원자 시각 표시 (노란 wireframe shell) 와 측정 픽 시각 표시 (모드 색상 shell + `#N` 텍스트) 가 Viewer texture 에 표시
- [ ] `BeginInteractionLod` / `EndInteractionLod` + wheel hold timer wrapping 동작
- [ ] 카메라 회전 발생 시 `setCameraDirection(NOT_ALIGNED)` 호출로 Cell Align 정렬 상태 해제

### 렌더 스타일 회귀 점검 (§ 5.6) — Phase 3.4 회귀 PR 머지 *후* 점검
- [ ] **선결 조건**: [`./phase3_4_renderer_regression.md`](./phase3_4_renderer_regression.md) 의 회귀 PR 이 main 또는 Phase 3.7 작업 브랜치의 base 에 머지된 상태인가?
- [ ] **§ 5.6.1** atom sphere 유효 반경 = `atom.radius * 0.5` 검증 (#46)
- [ ] **§ 5.6.1** atom shading (ambient 0.3 / diffuse 0.7 / specular 0.1 / specularPower 10) 검증 (#47)
- [ ] **§ 5.6.1** atom 선택 시각 = 개별 노란 wireframe shell, 그룹 색 변경 *없음* (#48)
- [ ] **§ 5.6.2** bond proportional 2-color split (`radius1/(radius1+radius2)`) 검증 (#49)
- [ ] **§ 5.6.2** bond shading 검증 (#50)
- [ ] **§ 5.6.2** bond opacity slider + < 1.0 시 `RenderLinesAsTubes` + `ForceTranslucentOn` 검증 (#51)
- [ ] **§ 5.6.2** `bondThickness` (0.1 ~ 3.0, default 1.0) + `Distance factor` (0.1 ~ 2.0) 슬라이더 검증 (#52, #53)
- [ ] 잔여 항목은 DEVIATION 으로 평가서에 기록 (FAIL 표기 금지)

### Phase 3.6 entry gate
- [ ] **#28** imported atoms 위 Measurement Distance / Angle PASS
- [ ] **#15** progress popup side-by-side 캡처 또는 잔여 리스크 명시
- [ ] **#8** `features/file/` + `core/io/` 변경 0 (정적 검증)

### 기록
- [ ] PR 본문에 Phase 3.6 (2026-05-13) entry gate 인계 명시
- [ ] Intentional UI deviation 섹션 작성 (없으면 `None`)
- [ ] `phase3_7_evaluation_YYYY-MM-DD.md` 작성

## 검토자

- [ ] diff 가 `features/viewer/` + 필요한 core/vtk/app/data/edit 보강으로 제한되어 있는가?
- [ ] `features/viewer` 가 legacy 를 include 하지 않는가?
- [ ] `features/file/` + `core/io/` 변경 0 인가?
- [ ] toolbar 7종 순서 / label / tooltip 이 legacy 와 같은가?
- [ ] Cell Align 이 cell matrix 를 변경하지 않는가?
- [ ] Mesh Display 의 deferred tooltip 이 노출되는가?
- [ ] imported atoms 위 Measurement pick 이 동작하는가?
- [ ] progress popup side-by-side 캡처가 첨부되었는가?
- [ ] wasm size delta 가 정상 범위인가?
- [ ] entry gate 전용 평가 섹션이 작성되었는가?
- [ ] **§ 5.5 입력 계약** — 5.5.1 카메라 조합 / 5.5.2 측정 / 5.5.3 비-측정 / 5.5.4 상호작용 표가 legacy 와 1:1 매핑되는가?
- [ ] 매트릭스 #33 ~ #45 결과가 모두 PASS 또는 deviation 기록되었는가?
- [ ] **§ 5.6 렌더 스타일** — Phase 3.4 회귀 PR 머지 확인 후 #46 ~ #53 결과가 PASS 또는 DEVIATION 으로 기록되었는가?

---

# Part 13 - 후속 Phase 연결

| 후속 phase | Phase 3.7 v2 가 제공할 기반 | 후속 연결점 |
|---|---|---|
| Phase 3.8 Model Tree | 실제 Viewer actor + active structure 가 보이는 상태 | tree selection / visibility 변화가 Viewer 에서 즉시 검증 가능 |
| Phase 3.9 Mesh | Mesh Display toolbar UI / state 선복구 | `features::mesh::SetAllDisplayMode` 만 연결하면 실제 적용 가능 |
| Phase 4 MenuRouter | `features/viewer` public entrypoint 정리 | `Settings / Viewer FPS Overlay`, `Windows / Viewer` 를 `app/settings`, `app/layout_manager` 로 이관 |
| Phase 5 legacy 격리 | viewer/toolbar legacy 참조 제거 | `legacy/` build 제외 전 최종 큰 의존성 제거 |
| Phase 6 마무리 | Viewer runtime 기반 통합 검증 가능 | EventBus fanout 검증, CMake 정리, docs 갱신 |

---

# Part 14 - 부록: v1 → v2 매핑 표

| v1 위치 | v2 위치 | 변경 |
|---|---|---|
| Part 1.3 Phase 3.6 평가서 반영 | Part 1.3 entry gate 인계 | entry gate 3종을 1차 통과 의무로 격상 |
| Part 1.2 비목표 (File parser 수정) | Part 1.2 + Part 7.1 #10 | 정적 검증 항목 추가 |
| Part 4.3 core/vtk::VtkViewer 보강 API | Part 4.3 | API 명세 보존 + `DrawRenderTexture` 명시 |
| Part 5.2 toolbar (Mesh Display tooltip) | Part 5.2 + Part 5.4 | tooltip 문구 고정 |
| Part 5.2 toolbar (Cell Align) | Part 5.2 + Part 4.3 + Part 2.3 + R4 | camera-only 정책 강화 + cell matrix 검증 의무 |
| Part 6.4 Step 3 event/picker | Part 6.4 | 동일 |
| Part 6.7 Step 6 잔여 검증 | Part 6.7 entry gate 해소 | entry gate 3종으로 명시 |
| Part 7 검증 매트릭스 | Part 7 + #10 추가 | File hot path 미침범 정적 항목 신설 |
| Part 8 완료 기준 | Part 8 + entry gate | 3종 entry gate 항목 추가 |
| Part 9 리스크 | Part 9 + R10/R11 | entry gate 미해소 / 캡처 부족 추가 |
| Part 10 평가서 양식 | Part 10 + entry gate 전용 섹션 | section 4 신설 |
| Part 12 PR 체크리스트 (v1 미존재) | Part 12 | 신설 |

## 14.1 v2 → v2.1 변경 (권장안 B)

| v2 위치 | v2.1 변경 | 사유 |
|---|---|---|
| 변경 이력 | v2.1 행 추가 | 변경 추적성 |
| Part 3.2 `mouse_interactor.{h,cpp}` | "보강 (필요 시)" → **필수 재작성** 으로 격상 + 8 개 sub-task (a) ~ (h) 명시 | 사용자 검토에서 modifier/측정-mode-aware routing 누락 식별 |
| Part 4.5 event / picker 좌표 원칙 | 7 개 항목 → 13 개 항목 (SetEventInformationFlipY, 4px 임계, modifier gating, drag rectangle 시각, same-element select, Interaction LOD, camera direction drift 등 추가) | 동일 |
| Part 5 | § 5.5 신설: 5.5.1 카메라 / 5.5.2 측정 / 5.5.3 비-측정 / 5.5.4 상호작용 4 개 표 | legacy 입력 계약을 명시적 표로 고정 |
| Part 7.3 검증 매트릭스 | #32 까지 → #45 까지 (13 행 추가) | 5.5 의 단위 검증 |
| Part 8 DoD | 3 개 입력 계약 항목 추가 | DoD 강화 |
| Part 12 PR 체크리스트 | "입력 계약 (§ 5.5)" 섹션 신설 (작성자) + 검토자 2 행 추가 | PR 단위 검증 강제 |

## 14.2 v2.1 → v2.1.1 변경 (권장안 D)

| v2.1 위치 | v2.1.1 변경 | 사유 |
|---|---|---|
| 변경 이력 | v2.1.1 행 추가 | 변경 추적성 |
| Part 1 | § 1.5 신설: Phase 3.4 회귀 PR 선결 조건 | 책임 phase 명확화 |
| Part 5 | § 5.6 신설: 5.6.1 atom sphere / 5.6.2 bond cylinder / 5.6.3 측정 픽 시각 3 개 표 | legacy 렌더 스타일 계약 명문화 |
| Part 7.3 매트릭스 | #45 까지 → #53 까지 (8 행 추가). 판정 정책 = PASS / DEVIATION (FAIL 금지) | 회귀 점검 강제 |
| Part 8 DoD | "렌더 스타일 회귀 점검 (§ 5.6)" 행 추가 | DoD 강화 |
| Part 12 PR 체크리스트 | "렌더 스타일 회귀 점검 (§ 5.6)" 섹션 신설 (작성자) + 검토자 1 행 추가 | PR 단위 검증 |
| 신규 외부 문서 | `phase3_4_renderer_regression.md` 신설 (별도 phase 회귀 PR) | 회귀 PR 의 실행 계획 |
