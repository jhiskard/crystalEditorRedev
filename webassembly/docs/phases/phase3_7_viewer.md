# Phase 3.7 - Viewer / Toolbar 이식 세부계획서
> 상위 문서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.7) + §6.0 공통 지침
> 선행 문서: [`./phase3_6_file.md`](./phase3_6_file.md)
> 선행 평가서: [`./phase3_6_evaluation_2026-05-12.md`](./phase3_6_evaluation_2026-05-12.md)
> 메뉴 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §9 Settings, §10 Windows, §12 Viewer 위 진입점
> 메뉴 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §3, §5
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01~UI-12)
> 작성일: 2026-05-12
> 대상 브랜치: `refactor/menu-aligned2` 또는 후속 작업 브랜치
> 단위 PR: 1개, 단일 sub-folder 중심
> 예상 소요: 2일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-12 | 초안 작성 - Phase 3.6 평가서의 조건부 GO, #28 BLOCKED, #8/#15 PARTIAL 항목을 Phase 3.7 실행 계획에 반영 |

---

## 0. 한 줄 요약

> Phase 3.7은 `features/viewer/`를 신설해 `Windows / Viewer` 창과 Viewer 상단 toolbar 7종을 복구하고, 현재 placeholder 상태인 `core/vtk::VtkViewer`를 legacy의 WebAssembly offscreen framebuffer + ImGui texture 렌더링 흐름으로 되살린다. Phase 3.6에서 남은 #28 `Measurement on imported atoms`는 본 단계의 필수 런타임 게이트로 재검증한다.

---

# Part 1 - 목표 / 비목표

## 1.1 목표

| 구분 | 항목 |
|---|---|
| Viewer 창 복구 | `Windows / Viewer` 토글로 `Viewer` ImGui 창을 열고 닫을 수 있게 한다. |
| 실제 VTK 화면 표시 | 현재 `core/vtk::VtkViewer::Render()`의 placeholder 문구를 제거하고, legacy처럼 VTK render window를 offscreen texture로 렌더한 뒤 `ImGui::Image`로 표시한다. |
| 창 resize 동기화 | Viewer content size 변경 시 VTK render window size, framebuffer texture size, picker 좌표계를 함께 갱신한다. |
| mouse interactor 연동 | Viewer viewport 안의 click, drag, wheel, hover 좌표를 `core/vtk::MouseInteractor`와 VTK interactor에 전달해 Phase 3.5 Measurement pick/drag가 실제 화면에서 동작하게 한다. |
| toolbar 7종 복구 | Viewer 창 내부에 legacy 순서의 toolbar를 복구한다. 대상은 Mesh Display, Projection, Reset View, Cell Align, Boundary Atoms, Charge Density Quick, Arrow Step이다. |
| Settings 연동 | `Settings / Viewer FPS Overlay` 토글을 복구하고 `VtkViewer::SetPerformanceOverlayEnabled`로 연결한다. |
| Phase 3.6 잔여 게이트 해소 | File import로 생성된 실제 구조에서 Measurement Distance/Angle 등 pick이 동작하는지 확인해 Phase 3.6 평가서 #28 BLOCKED를 해소한다. |
| 기존 feature actor 가시화 | Phase 3.3~3.6에서 이미 `VtkViewer::AddActor`에 추가하던 cell, atom, bond, measurement, charge density actor가 Viewer 안에 실제로 보이도록 한다. |
| legacy 격리 유지 | `webassembly/src/legacy/`는 수정하지 않고, 신규 코드에서 legacy header/source를 직접 include하지 않는다. 필요한 enum/API는 새 위치에 재정의한다. |
| 빌드 검증 | `npm run build-wasm:debug`, `npm run build-wasm:release`를 모두 통과시킨다. |

## 1.2 비목표

| 구분 | 항목 |
|---|---|
| full mesh feature | mesh actor import, mesh group, mesh detail, UNV 표시, mesh display mode 실제 적용은 Phase 3.9 `features/mesh` 범위다. Phase 3.7은 toolbar UI와 상태만 복구하고, mesh 대상이 없을 때 graceful no-op으로 둔다. |
| Model Tree | `Windows / Model Tree`, visibility tree, context menu는 Phase 3.8 범위다. |
| MenuRouter 정식 도입 | `app/menu_router`, `app/settings`, `app/layout_manager` 정식 구조는 Phase 4 범위다. Phase 3.7에서는 기존 Phase 3 패턴대로 `app/app.cpp`에 임시 hook을 둔다. |
| 전체 Settings 메뉴 | Background Color, Style, Font Size, Full Screen 등은 Phase 4 범위다. 단, `Viewer FPS Overlay`만 본 단계에서 복구한다. |
| Layout preset | Layout 1/2/3/Reset 버튼은 Phase 4 `layout_manager` 범위다. |
| File parser 수정 | Phase 3.6의 parser/progress/import 로직은 원칙적으로 변경하지 않는다. 다만 Viewer 복구 후 progress popup side-by-side 확인에서 명백한 표시 버그가 발견되면 최소 수정만 허용한다. |
| Data UI 재설계 | Charge Density Viewer 창 자체의 옵션 재설계는 금지한다. toolbar quick control에 필요한 public API만 보강한다. |
| Measurement 로직 재개발 | Phase 3.5의 측정 알고리즘은 변경하지 않는다. Viewer 좌표/picker 연결만 보강한다. |

## 1.3 Phase 3.6 평가서 반영

| Phase 3.6 평가 항목 | 상태 | Phase 3.7 반영 |
|---|---|---|
| #18~#27 File runtime | PASS | Viewer 검증용 실제 입력 경로로 사용한다. XSF/CHGCAR import 후 actor 표시를 확인한다. |
| #28 Measurement on imported atoms | BLOCKED | 본 Phase의 최우선 런타임 게이트. Viewer 복구 후 imported atoms에서 Distance/Angle pick을 수행한다. |
| #29 Data viewer on imported CHGCAR | PASS | Charge Density Quick toolbar의 조건부 노출과 연동을 확인하는 입력으로 사용한다. |
| #30 console error 0 | PASS | Phase 3.7에서도 동일 기준을 유지한다. Viewer event/texture 오류가 없어야 한다. |
| #31 wasm size delta | PASS | Phase 3.7 release build 후 새 wasm size와 delta를 평가서에 기록한다. |
| #8 FormatRegistry partial | PARTIAL | 본 Phase에서 직접 수정하지 않는다. 단, File import를 건드리지 않는다는 원칙을 검증 항목에 둔다. |
| #15 progress popup side-by-side partial | PARTIAL | Viewer 복구 후 File import 시나리오를 함께 실행하며 legacy side-by-side 표시 비교를 추가한다. |

---

# Part 2 - 현재 상태와 gap 분석

## 2.1 현재 코드 상태

| 영역 | 현재 상태 | gap |
|---|---|---|
| `core/vtk/vtk_viewer` | renderer/window/interactor 객체는 있으나 `Render()`는 placeholder ImGui 문구만 표시 | VTK texture를 화면에 그리지 않으므로 actor가 보여도 사용자가 볼 수 없음 |
| `app/app.cpp` | `core::vtk::VtkViewer::Instance().Render()`를 매 frame 직접 호출 | Viewer 창 show/hide 상태가 없고 `Windows / Viewer` 메뉴가 없음 |
| `core/vtk/mouse_interactor` | pick, drag, wheel event emit 기능은 있음 | Viewer viewport 좌표와 VTK interactor event feeding이 충분히 복구되지 않으면 실제 pick 검증이 불가 |
| edit renderers | atom/cell/bond actors를 `VtkViewer::AddActor`로 등록 | Viewer texture 미표시 때문에 시각 검증 불가 |
| measurement | picker event와 overlay actor 경로 구현 | Viewer가 살아나야 imported atoms 시나리오 검증 가능 |
| data renderers | charge density/slice actor를 `VtkViewer`에 등록 | Viewer quick toolbar에서 접근할 public API가 부족함 |
| toolbar | 신규 tree에는 없음. legacy `toolbar.cpp/h`만 참조 원본으로 존재 | `features/viewer/toolbar.*`로 이식 필요 |
| Settings/Windows menu | 신규 app에는 없음 | Phase 3.7 한정으로 Viewer/FPS 두 항목을 임시 hook해야 함 |

## 2.2 legacy 참조 범위

| legacy 파일 | 참조할 내용 | 신규 위치 |
|---|---|---|
| `legacy/vtk_viewer.{h,cpp}` | WebAssembly render window, framebuffer texture, `ImGui::Image`, event processing, camera reset/projection/FPS overlay | `core/vtk/vtk_viewer.{h,cpp}` 보강 + `features/viewer/viewer_panel.*` |
| `legacy/toolbar.{h,cpp}` | toolbar 배치, 버튼 순서, tooltip, popup, arrow step input, charge density quick controls | `features/viewer/toolbar.{h,cpp}` |
| `legacy/app.cpp` | `m_bShowVtkViewer`, `Windows / Viewer`, `Settings / Viewer FPS Overlay` | `features/viewer/viewer_menu.*` + `app/app.cpp` 임시 hook |
| `legacy/enum/viewer_enums.h` | `ProjectionMode`, `MeshDisplayMode`, `CameraDirection` 값 | 신규 `core/vtk/viewer_types.h` 또는 `features/viewer/viewer_types.h` |
| `legacy/enum/toolbar_enums.h` | toolbar anchor 값 | 신규 `features/viewer/viewer_types.h` |

## 2.3 핵심 판단

| 판단 | 내용 |
|---|---|
| Viewer는 feature이지만 VTK 저수준 렌더는 core에 둔다 | `features/viewer`는 창/toolbar/menu를 맡고, `core/vtk::VtkViewer`는 render window, framebuffer, camera, actor ownership을 맡는다. |
| Phase 4 이전 임시 메뉴 hook을 허용한다 | Phase 3.1~3.6과 같은 패턴으로 `app/app.cpp`에 `features::viewer::DrawMenus()`와 `RenderWindows()`를 호출한다. Phase 4에서 `app/settings`와 `app/layout_manager`로 이관한다. |
| Mesh Display는 UI 복구 + no-op 상태로 둔다 | Phase 3.9 전에는 mesh actor가 없으므로 실제 display mode 적용 대상이 없다. 버튼과 popup, 상태 저장, tooltip은 복구하되 적용은 deferred로 기록한다. |
| Cell Align은 lattice를 변경하면 안 된다 | 현재 `CellManager::AlignAxis`는 cell matrix를 바꾸는 placeholder 성격이므로 toolbar에서 사용하지 않는다. legacy처럼 camera를 cell axis에 맞추는 동작만 수행한다. |
| #28은 DoD의 일부다 | Viewer만 보이는 것으로는 완료가 아니다. File import 후 imported atoms에서 Measurement pick이 되어야 한다. |

---

# Part 3 - 대상 산출물

## 3.1 신규 파일 계획

| 파일 | 역할 |
|---|---|
| `webassembly/src/features/viewer/viewer_menu.h` | Phase 3.7 public entrypoint. `InitOnce`, `DrawMenus`, `RenderWindows`, `Shutdown` 선언 |
| `webassembly/src/features/viewer/viewer_menu.cpp` | `Windows / Viewer`, `Settings / Viewer FPS Overlay` 메뉴 hook과 Viewer show/hide 상태 보관 |
| `webassembly/src/features/viewer/viewer_panel.h` | Viewer window/panel 렌더러 선언 |
| `webassembly/src/features/viewer/viewer_panel.cpp` | `ImGui::Begin("Viewer")`, viewport rect 계산, `core::vtk::VtkViewer` texture draw 호출, toolbar overlay draw |
| `webassembly/src/features/viewer/toolbar.h` | toolbar state/API 선언 |
| `webassembly/src/features/viewer/toolbar.cpp` | toolbar 7종 UI 이식, feature public API 호출 |
| `webassembly/src/features/viewer/viewer_types.h` | toolbar anchor, mesh display mode 등 viewer feature 전용 enum |

## 3.2 보강 대상 파일

| 파일 | 변경 내용 |
|---|---|
| `webassembly/src/core/vtk/vtk_viewer.h` | WebAssembly texture 렌더, projection, reset, cell-axis camera align, FPS overlay, arrow step public API 추가 |
| `webassembly/src/core/vtk/vtk_viewer.cpp` | legacy `VtkViewer`의 framebuffer, resize, event processing, camera operation, performance overlay를 신규 core 스타일로 이식 |
| `webassembly/src/core/vtk/viewer_types.h` | `ProjectionMode`, `CameraDirection` 등 core camera enum 추가. legacy enum 직접 include 금지 |
| `webassembly/src/core/vtk/mouse_interactor.*` | 필요한 경우 Viewer viewport-relative 좌표와 render request 연동 보강 |
| `webassembly/src/core/ui/widgets.*` | tooltip helper 또는 transparent icon button helper 보강. 임의 UI 재설계 금지 |
| `webassembly/src/features/edit/edit_menu.*` | toolbar용 boundary atoms public API와 cell-axis camera action wrapper 노출 |
| `webassembly/src/features/data/data_menu.*` | toolbar용 charge density quick public API 노출 |
| `webassembly/src/features/data/charge_density/*` | quick play/pause/level API가 부족할 경우 최소 public method 추가 |
| `webassembly/src/app/app.cpp` | `features/viewer` include, `InitOnce`, `DrawMenus`, `RenderWindows` hook 추가. 기존 direct `VtkViewer::Render()` 제거 |
| `CMakeLists.txt` | 신규 `features/viewer` 파일과 `core/vtk/viewer_types.h` 등록 |

## 3.3 예상 규모

| 구분 | 예상 |
|---|---:|
| 신규 `features/viewer` 파일 | 7개 |
| core/app/data/edit 보강 파일 | 8~12개 |
| 신규/수정 라인 수 | 약 900~1,400 라인 |
| legacy 참조 수정 | 0 라인 |
| build target legacy source 추가 | 금지 |

---

# Part 4 - 아키텍처 세부 계획

## 4.1 호출 흐름

```text
app::App::Init()
  -> core::vtk::VtkViewer::Instance().Init()
  -> features::viewer::InitOnce(g_sceneState, *g_mouseInteractor)
  -> existing features InitOnce...

app::App::renderDockSpace()
  -> features::viewer::DrawMenus()
       - Settings / Viewer FPS Overlay
       - Windows / Viewer
  -> features::viewer::RenderWindows()
       - ViewerPanel::Render(&showViewer)
       - core::vtk::VtkViewer::DrawRenderTexture(...)
       - Toolbar::Render(...)
```

## 4.2 책임 분리

| 계층 | 책임 | 금지 |
|---|---|---|
| `features/viewer/viewer_menu` | top menu hook, show/hide flag, FPS toggle dispatch | render window 직접 조작 금지 |
| `features/viewer/viewer_panel` | `Viewer` ImGui window, viewport size/position 계산, toolbar overlay 배치 | actor 생성/삭제 금지 |
| `features/viewer/toolbar` | toolbar UI와 public feature API 호출 | `legacy/` include 금지, 다른 feature 내부 객체 직접 접근 금지 |
| `core/vtk/vtk_viewer` | VTK renderer/window/interactor, framebuffer texture, camera, event feeding, FPS overlay | top menu state 보관 금지 |
| `features/edit` | boundary atoms, cell matrix/camera align action public wrapper | toolbar UI 렌더 금지 |
| `features/data` | charge density quick control public wrapper | toolbar UI 렌더 금지 |

## 4.3 `core/vtk::VtkViewer` 보강 항목

| API | 목적 |
|---|---|
| `void RenderToImGuiImage(const ImVec2& viewportSize, const ImVec2& viewportPos)` 또는 동등 API | VTK render result를 ImGui image로 표시 |
| `void Resize(int w, int h)` | render window와 framebuffer texture 크기 동기화 |
| `void RequestRender()` | dirty flag 설정 후 필요 시 VTK render 수행 |
| `void ResetView()` | legacy reset view 버튼 동작 복구 |
| `void FitViewToVisibleProps()` | 현재 actor bounds에 맞게 camera reset |
| `void SetProjectionMode(ProjectionMode mode)` | perspective/parallel 전환 |
| `ProjectionMode GetProjectionMode() const` | toolbar 현재 상태 표시 |
| `void SetPerformanceOverlayEnabled(bool enabled)` | Settings / Viewer FPS Overlay 토글 |
| `bool IsPerformanceOverlayEnabled() const` | menu check 상태 |
| `void AlignCameraToCellAxis(const std::array<std::array<float, 3>, 3>& cell, int axis)` | cell matrix를 바꾸지 않고 camera 방향만 변경 |
| `void SetArrowRotateStepDeg(float stepDeg)` | Arrow Step input 반영 |
| `float GetArrowRotateStepDeg() const` | Arrow Step input 표시 |
| `void ProcessViewerEvents(...)` | viewport-relative mouse/keyboard event를 VTK interactor에 전달 |

## 4.4 WebAssembly 렌더 복구 원칙

| 항목 | 원칙 |
|---|---|
| render window type | legacy처럼 WebAssembly용 render window를 사용한다. 현재 generic `vtkRenderWindow`로 안 보이면 `vtkWebAssemblyOpenGLRenderWindow`로 교체한다. |
| offscreen rendering | Viewer 창 내부에 texture로 표시해야 하므로 offscreen framebuffer 흐름을 복구한다. |
| texture 좌표 | OpenGL/ImGui y-axis 차이를 legacy와 동일하게 처리한다. |
| resize | content size가 1px 이하인 경우 render를 skip하고, 정상 크기에서만 framebuffer를 재생성한다. |
| dirty render | 매 frame full render가 아니라 actor 변경, resize, interaction, forced render 시점을 우선한다. 단 초기 복구 단계에서는 안정성을 우선해 legacy와 같은 렌더 주기를 허용한다. |
| overlay renderer | measurement 2D/3D overlay가 depth에 묻히지 않도록 legacy overlay renderer 구조를 검토한다. 현재 actor 경로와 충돌하면 최소 보강만 한다. |
| camera widget | legacy camera orientation widget은 가능하면 복구한다. WebAssembly/ImGui texture와 충돌하면 본 Phase 평가서에 deviation으로 기록하고 후속 보강으로 분리한다. |

## 4.5 event/picker 좌표 원칙

| 항목 | 원칙 |
|---|---|
| 좌표 기준 | ImGui screen coordinate를 Viewer viewport local coordinate로 변환한다. |
| y-axis | VTK picker가 기대하는 bottom-left origin과 ImGui top-left origin 차이를 legacy와 동일하게 보정한다. |
| hover gating | Viewer image 영역이 hovered/focused일 때만 VTK interactor에 mouse event를 보낸다. 메뉴/toolbar 클릭은 VTK camera drag로 오인하지 않게 한다. |
| drag selection | Phase 3.5 `onDragSelection`이 받을 viewport height를 실제 Viewer texture height로 전달한다. |
| empty click | actor pick 실패 시 기존 `onEmptyClick` 경로가 유지되어 Measurement mode exit/clear 동작이 깨지지 않아야 한다. |
| wheel | wheel camera zoom 후 `RequestRender()`와 필요한 LOD/event fanout이 유지되어야 한다. |
| keyboard arrow | arrow key camera rotation은 `Arrow Step` 값으로 회전량이 바뀌어야 한다. |

---

# Part 5 - Viewer UI 계약

## 5.1 Viewer 창 계약

| 항목 | legacy 기준 | Phase 3.7 요구 |
|---|---|---|
| 창 title | `Viewer` | 동일 |
| 열림 기본값 | app 시작 시 표시 | 동일. 단 `Windows / Viewer`로 끌 수 있어야 함 |
| 메뉴 위치 | `Windows / Viewer` | 동일 label과 checkbox 동작 |
| 창 close 버튼 | close 시 Windows 메뉴 check 해제 | 동일 |
| content | VTK scene texture | placeholder 문구 금지 |
| toolbar 위치 | Viewer 창 내부 overlay | 동일. toolbar를 별도 top-level window로 빼지 않음 |
| resize | 창 크기에 따라 scene texture resize | 동일 |
| focus/hover | Viewer 영역에서만 camera interaction | 동일 |
| FPS overlay | Settings toggle이 켜진 경우 Viewer 안에 표시 | 동일 |

## 5.2 toolbar 7종 계약

| 순서 | toolbar 항목 | legacy 동작 | Phase 3.7 요구 |
|---:|---|---|---|
| 1 | Boundary Atoms Quick | boundary atoms ON/OFF 토글 | `features::edit` public API로 토글. 상태 색/tooltip 유지 |
| 2 | Mesh Display Mode | Wireframe/Shaded/WireShaded popup | popup과 state 복구. 실제 mesh 적용은 Phase 3.9 전까지 no-op/deferred |
| 3 | Projection | Perspective/Parallel popup | `VtkViewer::SetProjectionMode`로 camera projection 전환 |
| 4 | Reset View | 현재 scene에 맞게 camera reset | `VtkViewer::ResetView` 또는 `FitViewToVisibleProps` 호출 |
| 5 | Cell Align | a1/a2/a3 또는 BZ mode의 b1/b2/b3 camera align | cell matrix를 변경하지 않고 camera만 정렬 |
| 6 | Charge Density Quick | play/pause/restart/level slider 조건부 표시 | data feature에 loaded density가 있을 때만 표시. quick API로 level/render 갱신 |
| 7 | Arrow Step | 1~180 degree input | `VtkViewer::SetArrowRotateStepDeg`, default 45 유지 |

## 5.3 toolbar 표시 조건

| 항목 | 표시 조건 |
|---|---|
| Boundary Atoms | active structure가 있고 atoms feature가 초기화되어 있으면 표시. 구조가 없어도 버튼 disabled 또는 no-op으로 안전해야 함 |
| Mesh Display | 항상 표시하되 mesh feature 미구현 상태에서는 tooltip에 Phase 3.9 deferred 명시 |
| Projection | 항상 표시 |
| Reset View | 항상 표시 |
| Cell Align | active structure에 unit cell이 있으면 표시 |
| Charge Density Quick | charge density data가 있고 surface/volumetric/simple view에 대응되는 렌더 상태가 있을 때 표시 |
| Arrow Step | 항상 표시 |

## 5.4 Intentional UI deviation 사전 정의

| 항목 | legacy 동작 | Phase 3.7 계획 | 사유 | 후속 |
|---|---|---|---|---|
| Mesh Display 실제 적용 | mesh actors 전체 display mode 변경 | UI state와 popup은 복구하되 실제 mesh actor 적용은 deferred | Phase 3.9 `features/mesh`가 아직 없음 | Phase 3.9에서 `features::mesh::SetAllDisplayMode` 연결 |
| Settings 메뉴 전체 | legacy Settings 전체 제공 | `Viewer FPS Overlay`만 제공 | Phase 4 범위와 충돌 방지 | Phase 4에서 `app/settings`로 이관 |
| Windows 메뉴 전체 | legacy Windows 전체 제공 | `Viewer`만 제공 | Model Tree/Layout 등은 후속 Phase 범위 | Phase 3.8/4에서 확장 |

---

# Part 6 - 구현 순서

## 6.1 Step 0 - 정적 조사와 기준 캡처

| 작업 | 산출물 |
|---|---|
| legacy `vtk_viewer.cpp`의 `Render`, framebuffer, event, FPS overlay, camera 함수 위치 정리 | 구현 메모 또는 PR 본문 |
| legacy `toolbar.cpp`의 버튼 순서, label, tooltip, 표시 조건 표 추출 | 본 계획서 Part 5와 대조 |
| 현재 `core/vtk::VtkViewer` public API와 actor 사용처 정리 | API 보강 diff 최소화 |
| Phase 3.6 평가서 #28/#15 입력 파일 시나리오 정리 | 런타임 검증 시나리오 |

## 6.2 Step 1 - `features/viewer` skeleton 추가

| 작업 | 산출물 |
|---|---|
| `features/viewer/viewer_menu.{h,cpp}` 추가 | `InitOnce`, `DrawMenus`, `RenderWindows`, `Shutdown` |
| `features/viewer/viewer_panel.{h,cpp}` 추가 | `ViewerPanel::Render(bool* open)` |
| `features/viewer/toolbar.{h,cpp}` 추가 | toolbar state와 render 함수 |
| `features/viewer/viewer_types.h` 추가 | toolbar enum |
| `CMakeLists.txt` 등록 | build source list 갱신 |
| `app/app.cpp` hook | direct `VtkViewer::Render()`를 feature window render로 교체 |

## 6.3 Step 2 - core viewer texture 렌더 복구

| 작업 | 세부 내용 |
|---|---|
| WebAssembly render window 전환 | 필요 시 `vtkWebAssemblyOpenGLRenderWindow`, `vtkWebAssemblyRenderWindowInteractor` 사용 |
| framebuffer lifecycle 구현 | init, resize, clear, delete를 RAII에 맞게 보강 |
| `ImGui::Image` 표시 | `ViewerPanel`이 계산한 viewport size로 color texture 표시 |
| dirty render 처리 | `RequestRender`, resize, interaction 시 render 수행 |
| actor 경로 유지 | 기존 `AddActor`, `RemoveActor`, `AddActor2D`, `RemoveActor2D` 호출부가 깨지지 않아야 함 |
| camera reset | `ResetView`, `FitViewToVisibleProps` 동작 복구 |

## 6.4 Step 3 - event/picker 복구

| 작업 | 세부 내용 |
|---|---|
| mouse position 변환 | ImGui mouse pos에서 Viewer viewport local pos 계산 |
| VTK interactor event feeding | left down/up/move/wheel/key event를 VTK interactor에 전달 |
| toolbar click 차단 | toolbar 위 클릭은 camera drag/pick으로 들어가지 않게 guard |
| pick 검증 | `MouseInteractor::emitPickOrEmptyClick`가 imported atom actor를 pick하는지 확인 |
| drag 검증 | drag selection rectangle과 viewport height 전달 확인 |
| render request | 모든 interaction 후 `RequestRender()` 호출 확인 |

## 6.5 Step 4 - toolbar 7종 구현

| 항목 | 구현 API |
|---|---|
| Boundary Atoms | `features::edit::BoundaryAtomsEnabled`, `features::edit::SetBoundaryAtomsEnabled` 추가 |
| Mesh Display | `features/viewer` 내부 state. Phase 3.9 전까지 `features::mesh` 호출 없음 |
| Projection | `core::vtk::VtkViewer::SetProjectionMode`, `GetProjectionMode` |
| Reset View | `core::vtk::VtkViewer::ResetView` |
| Cell Align | `features::edit::AlignCameraToCurrentCellAxis(axis)` 또는 cell matrix getter + `VtkViewer` camera align |
| Charge Density Quick | `features::data::HasChargeDensity`, `SetChargeDensityLevelPercent`, `Start/Stop/RestartQuickAnimation` 등 최소 API |
| Arrow Step | `core::vtk::VtkViewer::GetArrowRotateStepDeg`, `SetArrowRotateStepDeg` |

## 6.6 Step 5 - Settings / Windows menu hook

| 메뉴 | 구현 |
|---|---|
| `Settings / Viewer FPS Overlay` | `features::viewer::DrawMenus()`에서 `Settings` 메뉴를 열고 checkbox를 `VtkViewer`에 반영 |
| `Windows / Viewer` | 같은 함수에서 `Windows` 메뉴를 열고 `g_showViewerWindow`를 토글 |
| 메뉴 순서 | 기존 메뉴 뒤에 `Settings`, `Windows` 순서로 추가. Phase 4 전 임시 hook임을 코드 주석에 남김 |

## 6.7 Step 6 - Phase 3.6 잔여 검증

| 검증 | 목적 |
|---|---|
| imported XSF visual | File import 결과가 Viewer actor로 표시되는지 확인 |
| imported CHGCAR visual | Data renderer와 Viewer 표시 연동 확인 |
| Measurement on imported atoms | Phase 3.6 #28 BLOCKED 해소 |
| progress popup visual | Phase 3.6 #15 PARTIAL 보강. Viewer 복구 후 legacy side-by-side 비교 |

---

# Part 7 - 검증 매트릭스

## 7.1 정적 / 구조 검증

| # | 검증 항목 | 기대값 | 방법 |
|---:|---|---|---|
| 1 | 신규 viewer 파일 | `features/viewer/` 7개 내외 | `rg --files webassembly/src/features/viewer` |
| 2 | legacy 수정 없음 | `webassembly/src/legacy` 변경 0 | `git diff -- webassembly/src/legacy` |
| 3 | legacy include 없음 | 신규 viewer/core 보강 코드에서 `legacy/` include 0 | `rg -n "legacy|src/legacy|#include.*legacy" ...` |
| 4 | app hook | `InitOnce`, `DrawMenus`, `RenderWindows` 호출 존재 | `rg -n "features::viewer" webassembly/src/app/app.cpp` |
| 5 | direct placeholder 제거 | placeholder text 제거 | `rg -n "Phase 1 bootstrap|Rendering features will be restored" webassembly/src/core/vtk` |
| 6 | CMake 등록 | 신규 viewer files 등록 | `rg -n "features/viewer" CMakeLists.txt` |
| 7 | core viewer API | projection/reset/FPS/arrow step API 존재 | `rg -n "SetProjectionMode|ResetView|SetPerformanceOverlayEnabled|SetArrowRotateStepDeg" webassembly/src/core/vtk` |
| 8 | edit quick API | boundary/cell align wrapper 존재 | `rg -n "BoundaryAtoms|AlignCamera" webassembly/src/features/edit` |
| 9 | data quick API | charge density quick wrapper 존재 | `rg -n "ChargeDensity.*Quick|LevelPercent|HasChargeDensity" webassembly/src/features/data` |
| 10 | File import code 비침범 | Phase 3.6 file hot path 변경 없음 또는 의도 기록 | `git diff -- webassembly/src/features/file webassembly/src/core/io` |

## 7.2 빌드 검증

| # | 검증 항목 | 기대값 | 명령 |
|---:|---|---|---|
| 11 | debug build | exit 0 | `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"` |
| 12 | release build | exit 0 | `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"` |
| 13 | wasm size 기록 | 이전 명시 baseline 대비 정상 범위 | `Get-Item public\wasm\VTK-Workbench.wasm` |

## 7.3 런타임 / 시각 검증

| # | 검증 항목 | 기대값 | 판정 기준 |
|---:|---|---|---|
| 14 | Viewer 기본 표시 | 앱 시작 후 `Viewer` 창 표시 | placeholder가 아닌 실제 VTK background/actors 표시 |
| 15 | Windows / Viewer | 체크 해제 시 Viewer 숨김, 체크 시 재표시 | menu checkbox와 window close 상태 동기화 |
| 16 | Settings / Viewer FPS Overlay | 체크 시 FPS overlay 표시, 해제 시 숨김 | console error 없음 |
| 17 | resize | Viewer 창 resize 후 texture stretch/black screen 없음 | actor와 background가 새 크기에 맞게 표시 |
| 18 | Projection toolbar | Perspective/Parallel 전환 | camera parallel projection 여부가 시각적으로 변함 |
| 19 | Reset View toolbar | actor가 화면 중앙/적절한 bounds로 복귀 | XSF/FCC/CHGCAR actor 기준 확인 |
| 20 | Cell Align toolbar | a1/a2/a3 클릭 시 camera 방향 변경 | cell matrix는 변경되지 않음 |
| 21 | Boundary Atoms toolbar | ON/OFF 토글 시 boundary atoms 표시 변화 | Edit / Atoms checkbox와 상태 일치 |
| 22 | Charge Density Quick | data loaded 상태에서 quick control 표시/동작 | level 변경 시 surface/volume/slice 중 해당 렌더 갱신 |
| 23 | Arrow Step toolbar | 1~180 입력 clamp, arrow rotation step 반영 | keyboard arrow 회전량 변경 |
| 24 | Mesh Display toolbar | popup과 state 변경, no-op 안내 | Phase 3.9 deferred tooltip 표시, error 없음 |
| 25 | XSF imported actor 표시 | Phase 3.6 import 구조가 Viewer에 표시 | atom/cell actor visible |
| 26 | CHGCAR imported actor/data 표시 | imported atoms와 density render 연동 | Data viewer와 toolbar quick control 정상 |
| 27 | Measurement on imported atoms | imported atoms에서 Distance 또는 Angle 생성 | Phase 3.6 #28 BLOCKED 해소 |
| 28 | drag selection | Viewer drag로 selection event 발생 | Measurement center modes 또는 selection UI와 연동 |
| 29 | progress popup side-by-side | legacy와 show/hide/percentage 표시 흐름 일치 | Phase 3.6 #15 PARTIAL 보강 |
| 30 | console error 0 | runtime JS/C++ console error 없음 | DevTools 기준 |
| 31 | repeated open/close | Viewer close/open 반복 후 texture/event 정상 | context/framebuffer leak 증상 없음 |
| 32 | menu coexistence | File/Edit/Build/Measurement/Data/Utilities 메뉴 영향 없음 | 기존 Phase 3.6 PASS 시나리오 재확인 |

## 7.4 UI 지침 매트릭스

| UI ID | 검증 항목 | Phase 3.7 적용 |
|---|---|---|
| UI-01 | 옵션명/표시순서 일치 | toolbar 7종 순서와 menu label 확인 |
| UI-02 | 기본값/범위 일치 | Projection default, Arrow Step 45, clamp 1~180 |
| UI-03 | 입력방법 일치 | popup, button, checkbox, input int 유지 |
| UI-04 | 조건부 표시 규칙 일치 | Cell Align, Charge Density Quick 조건부 표시 |
| UI-05 | 활성/비활성 규칙 일치 | Mesh 미구현 no-op/deferred, data 없음 quick hidden |
| UI-06 | Apply/Direct Apply 시점 일치 | click/slider/input 즉시 반영 |
| UI-07 | 공통옵션 공유 연동 일치 | Data UI level/state와 quick toolbar state 동기화 |
| UI-08 | 색상 선택도구 위치/동작 일치 | 본 Phase 신규 색상 도구 없음 |
| UI-09 | 다중 데이터 선택 UI 일치 | Phase 3.6 XSF Grid selector 유지, toolbar가 active data만 참조 |
| UI-10 | ImGui ID 충돌 없음 | toolbar popup/button/input ID suffix 확인 |
| UI-11 | 실제 파일 로드 렌더 가능 | XSF/CHGCAR import 후 Viewer 표시 |
| UI-12 | 빈 데이터 테스트 훅 동작 | data 없음 상태에서 toolbar quick controls가 안전하게 hidden/disabled |

---

# Part 8 - 완료 기준

| DoD 항목 | 기준 |
|---|---|
| Viewer 창 | `Windows / Viewer` 토글과 close button이 동기화되고 실제 VTK scene texture가 표시됨 |
| toolbar | 7종이 legacy 순서로 표시되고, Phase 3.9 deferred인 Mesh Display 외 6종이 실제 public API와 연결됨 |
| FPS overlay | `Settings / Viewer FPS Overlay`로 표시/숨김 가능 |
| actor 표시 | Phase 3.3~3.6 feature actors가 Viewer 안에서 보임 |
| pick/drag | Viewer 좌표 기반 click/drag가 `MouseInteractor`를 통해 feature event로 전달됨 |
| Phase 3.6 #28 | imported atoms 위 Measurement 시나리오 PASS 또는 명확한 신규 blocker 기록 |
| Phase 3.6 #15 | progress popup side-by-side 시각 검증 보강 또는 명확한 잔여 리스크 기록 |
| legacy 격리 | legacy source/header 직접 의존 없음 |
| build | debug/release wasm build PASS |
| runtime | console error 0 |
| 평가서 | Phase 3.7 평가서에 PASS/PARTIAL/BLOCKED/FAIL 집계와 wasm size 기록 |

---

# Part 9 - 리스크와 대응

| 리스크 | 영향 | 대응 |
|---|---|---|
| WebAssembly render window 교체로 build 오류 | Viewer 복구 지연 | legacy `vtk_viewer.cpp`의 include와 CMake link 상태를 기준으로 최소 이식 |
| ImGui texture와 VTK framebuffer y-axis 불일치 | 화면 상하 반전 또는 picker 좌표 오류 | legacy `ImGui::Image` UV, picker y 보정 로직을 그대로 대조 |
| toolbar 클릭이 VTK pick으로 전달 | 버튼 클릭 시 measurement 오작동 | toolbar hovered 영역에서는 event feeding 차단 |
| cell align이 cell matrix를 바꿈 | 데이터 손상 | `CellManager::AlignAxis` 사용 금지, camera-only action 추가 |
| charge density quick API 부족 | toolbar 6번 구현 지연 | Data feature에 최소 wrapper를 추가하고 UI 창 본체는 변경하지 않음 |
| Mesh Display 실제 적용 불가 | toolbar 7종 중 1종 PARTIAL 가능성 | Phase 3.9 deferred를 계획서/평가서에 명시하고 no-op UI를 안정적으로 제공 |
| Viewer render가 매 frame 과도하게 비용 발생 | FPS 저하 | dirty flag와 FPS overlay로 상태 확인. 필요 시 interaction 중 LOD만 최소 이식 |
| Phase 3.6 File import 회귀 | 이미 PASS한 file runtime 손상 | File/core/io 변경 금지 원칙, 변경 발생 시 별도 deviation 기록 |
| wasm size 증가 | release artifact 비대화 | Phase 3.7 release build 후 size delta 기록. 급격한 증가 시 framebuffer/texture 중복 보관 점검 |

---

# Part 10 - 평가서 작성 지침

Phase 3.7 완료 후 평가는 이전 phase 양식과 동일하게 `webassembly/docs/phases/phase3_7_evaluation_YYYY-MM-DD.md`로 작성한다.

| 평가 섹션 | 포함 내용 |
|---|---|
| 집중 결론 | Viewer/toolbar 복구 여부, #28 해소 여부, GO/PARTIAL/BLOCKED 판단 |
| 구현 개요 | 신규 파일, 수정 파일, 라인 수, legacy 격리 상태 |
| 검증 매트릭스 | 본 계획서 Part 7의 #1~#32 결과 |
| UI 지침 결과 | UI-01~UI-12 결과와 deviation |
| Phase 3.6 carry-over | #28, #15, #8 상태 갱신 |
| 리스크 | 미해결 항목과 다음 Phase 연결 |
| 명령 요약 | build/test/static check 명령과 결과 |

---

# Part 11 - 사용 명령 요약

```powershell
git status --short --branch
rg --files webassembly\src\features\viewer
rg -n "legacy|src/legacy|#include.*legacy" webassembly\src\features\viewer webassembly\src\core\vtk
rg -n "features::viewer|viewer_menu" webassembly\src\app\app.cpp CMakeLists.txt
rg -n "SetProjectionMode|ResetView|SetPerformanceOverlayEnabled|SetArrowRotateStepDeg" webassembly\src\core\vtk
rg -n "BoundaryAtoms|AlignCamera" webassembly\src\features\edit
rg -n "ChargeDensity.*Quick|LevelPercent|HasChargeDensity" webassembly\src\features\data
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"
Get-Item public\wasm\VTK-Workbench.wasm
```

---

# Part 12 - 후속 Phase 연결

| 후속 phase | Phase 3.7이 제공할 기반 | 후속 연결 |
|---|---|---|
| Phase 3.8 Model Tree | 실제 Viewer actor와 active structure가 보이는 상태 | tree selection/visibility 변화가 Viewer에서 즉시 검증 가능 |
| Phase 3.9 Mesh | Mesh Display toolbar UI/state 선복구 | `features::mesh::SetAllDisplayMode`만 연결하면 실제 적용 가능 |
| Phase 4 MenuRouter | `features/viewer` public entrypoint 정리 | `Settings / Viewer FPS Overlay`, `Windows / Viewer`를 `app/settings`, `app/layout_manager`로 이관 |
| Phase 5 legacy 격리 | viewer/toolbar legacy 참조 제거 | `legacy/` build 제외 전 최종 큰 의존성 제거 |
| Phase 6 마무리 | Viewer runtime 기반 통합 검증 가능 | EventBus fanout, CMake 정리, docs 갱신 |
