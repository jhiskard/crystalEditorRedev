# 05. 재구성 계획 — Legacy 동결 + Greenfield 재구성

> 본 작업은 **in-place 점진 리팩터가 아니다.**
> 현 코드 전체를 `webassembly/src/legacy/` 로 옮기고, 새 트리를 빈 상태에서 처음부터 다시 짠다.
> 새 트리의 각 모듈은 `legacy/` 의 같은 책임 모듈을 참조해서 로직을 옮기되,
> **API 표면, 의존 방향, 주석 형식**은 본 문서들의 규칙에 맞춘다.

## 1. 왜 점진 리팩터가 아니라 Greenfield 인가

| 점진 리팩터 (refactory6 안) | Greenfield 재구성 (본 안) |
|---|---|
| 매 PR 빌드 가능 상태를 유지하기 쉬움 | Phase 1 동안 빌드가 임시로 깨질 수 있음 |
| 의존 그래프를 한 번에 끊기 어려움 | 의존을 처음부터 깨끗하게 설정 |
| `AtomsTemplate` 같은 god object 가 단계별로 살아 있음 | god object 가 처음부터 존재하지 않음 |
| 기존 코드와 새 코드가 한 폴더에 섞여 readability 떨어짐 | `legacy/` 는 격리됨 — 무엇이 새 코드인지 자명 |
| Doxygen 주석을 일괄 적용하기 어려움 | 모든 신규 파일이 동일 컨벤션으로 시작 |

→ 작은 프로젝트가 아니라면 점진 리팩터가 일반적으로 더 안전하지만, 본 프로젝트는
- (a) 메뉴 트리가 명확하게 정의되어 있어 새 트리의 외형을 미리 합의 가능,
- (b) `AtomsTemplate` 의 의존 면이 너무 넓어 한 번에 분리할 곳이 마땅치 않음,
- (c) 주석/네이밍 컨벤션을 일괄 정비하고 싶음

→ 본 작업에서는 **Greenfield** 를 채택한다.

## 2. 단계 개관

```
Phase 0 ─ 현재 구조 동결 (1~2시간)
Phase 1 ─ 새 셸 부트스트랩: app/ + core/ 빈 빌드 (1~2일)
Phase 2 ─ core/scene + core/io 골격 (3~5일)
Phase 3 ─ features/ 메뉴별 이식 (2~3주)
Phase 4 ─ menu_router + app.cpp 슬림화 (2~3일)
Phase 5 ─ legacy/ 의존 단절 + 빌드에서 제외 (2~3일)
Phase 6 ─ 이벤트 버스, CMake/문서/주석 마무리 (2~3일)
─────────────────────────────────────────────────
총 예상: 4~6주 (single dev + Codex/Claude 보조)
```

## 3. Phase 0 — 현재 구조 동결

**목표**: 한 PR 안에서 끝나는 단순 이동. 기능 변경 없음.

작업:
1. `webassembly/src/legacy/` 폴더 생성.
2. 다음 항목을 **있는 그대로** `legacy/` 로 이동:
   - 루트의 `.cpp/.h` 전부 (`main.cpp`, `bind_function.cpp` **제외** — 빌드 엔트리)
   - `atoms/` 폴더 전체
   - `common/`, `enum/`, `icon/`, `macro/`, `config/`
3. `webassembly/src/main.cpp`, `bind_function.cpp` 는 루트에 그대로 둔다 (단, 임시로 비활성화 코드로 만들어서 빌드만 가능하게).
4. `CMakeLists.txt` 의 source 리스트를 `legacy/` 경로로 갱신하거나, 임시로 `legacy/` 를 컴파일 대상에서 제외하고 main 만 남긴 최소 빌드를 만든다.
5. 본 문서들 (`webassembly/docs/*`) 그대로 유지.
6. 검증: `npm run build-wasm:debug` 가 빌드 통과 (런타임은 빈 화면 / Hello WebGL 정도라도 OK).

**커밋 단위**: 단 1개 PR.
**리스크**: CMake 의 file 리스트가 깨질 수 있다. 이동만으로는 동작이 바뀌지 않으므로 일단 `legacy/` 전체를 임시 빌드 제외시키고 빈 main 으로 빌드 검증을 먼저 한다.

## 4. Phase 1 — 새 셸 부트스트랩

**목표**: `webassembly/src/app/`, `webassembly/src/core/` 가 **빈 빌드 가능한 Hello dockspace** 를 띄우는 상태.

작업:
1. 신규 폴더 생성: `app/`, `core/{vtk,io,data,scene,render,ui}/`, `features/` (빈 폴더).
2. `app/app.cpp`, `app/app.h` 작성:
   - `legacy/app.cpp` 의 `App::Init / Render / RenderWindows` 골격을 ImGui dockspace 만 표시하는 최소 코드로 옮김.
   - 메뉴바는 일단 placeholder (`File / Help` 만 비어있는 채로).
3. `core/vtk/vtk_viewer.cpp/h` 의 최소 셸 작성: VTK 윈도우 1개 + 빈 렌더러.
4. `core/render/font_manager.cpp` 는 `legacy/font_manager.cpp` 그대로 import (자동생성된 폰트 데이터는 변경 없이 유지).
5. `main.cpp`, `bind_function.cpp` 가 `app::App` 만 인스턴스화하도록 수정.
6. `CMakeLists.txt` 를 새 트리만 빌드하도록 갱신:
   ```
   set(SOURCES_APP    app/app.cpp app/menu_router.cpp ...)
   set(SOURCES_CORE   core/...)
   set(SOURCES_FEAT   ${FEATURES_FILE_SOURCES} ${FEATURES_EDIT_SOURCES} ...)
   add_executable(... ${SOURCES_APP} ${SOURCES_CORE} ${SOURCES_FEAT} main.cpp bind_function.cpp)
   ```
   `legacy/` 는 그대로 빌드 제외.

**검증**: 브라우저에서 빈 dockspace 가 떠야 한다. 메뉴는 비어 있어도 OK.

## 5. Phase 2 — core/scene + core/io 골격

**목표**: 모든 feature 가 의존할 공유 컴포넌트의 인터페이스 확정.

작업:
1. `core/scene/scene_state.h` 작성 (구조체 + DI 슬롯).
2. `core/scene/structure_registry.*`, `selection.*`, `hover.*`, `events.h` 작성.
3. `core/io/file_dialog.*` — `legacy/file_loader.cpp` 의 Emscripten 다이얼로그/청크 전송 로직을 그대로 가져와 thin wrapper 로 정리.
4. `core/io/format_registry.*` — 확장자 → 파서 함수 포인터 매핑.
5. `core/io/xsf_parser.*` (legacy `atoms/infrastructure/file_io_manager.cpp` 의 XSF 파트만 분리).
6. `core/io/chgcar_parser.*` (legacy `atoms/infrastructure/chgcar_parser.*` 그대로 이동).
7. `core/io/rho_parser.*`, `core/io/unv_reader.*` (legacy 의 같은 파일 그대로 이동).
8. `core/data/element_database.*`, `colormap.*`, `lcrs_tree.*`, `string_utils.*`, `color.h` 이동.
9. `core/render/{image,texture,font_manager}.*` 이동 (font_manager 는 자동생성 데이터라 변경 X).
10. `core/ui/widgets.*` (`legacy/custom_ui.*` 가 비어 있어 신규 작성), `ui_color_utils.h`, `icons/` 이동.
11. `core/vtk/mouse_interactor.*` — legacy `mouse_interactor_style.*` 에서 ImGui 입력 → 이벤트 emit 형태로 전환. 이때는 dispatch 대상이 없어도 emit 만 되는 빈 옵저버 상태로 둔다.

**검증**: 빌드 통과 + 기존 빈 화면 유지.

## 6. Phase 3 — features/ 메뉴별 이식 (한 PR = 한 메뉴)

이 페이즈가 가장 길다. 각 메뉴를 **의존도 낮은 쪽부터** 한 번에 한 개씩 이식한다.

### 6.0 공통 지침 — 모든 sub-phase 에 적용 (Phase 3.1~3.9 공통)

본 §은 Phase 3 의 모든 sub-phase 가 *반드시* 따라야 하는 전역 지침이다. 각 sub-phase 의 세부계획서는 본 §을 *상속* 한다.

#### 6.0.1 **legacy UI 작동방식 1:1 보존 원칙** (필수)

각 메뉴 항목을 features/ 로 이식할 때, **legacy 의 UI 동작은 1:1 그대로 보존** 한다. 사용자가 *"버튼을 눌렀을 때 어떤 일이 일어나는가"* 의 관점에서 본 동작이 변경되어서는 안 된다.

**보존 대상 (변경 금지)**:

| 항목 | 의미 |
|---|---|
| **ImGui 위젯 배치 순서** | 윈도우 안의 콤보박스/슬라이더/체크박스/버튼의 *상대적 순서와 위치* |
| **콤보 항목 / 토글 항목 목록** | 드롭다운의 항목 텍스트, 라디오 버튼의 라벨, 체크박스의 의미 |
| **기본값 / 초기 상태** | 슬라이더의 시작 값, 콤보의 기본 선택, 토글의 default on/off |
| **값의 응답 패턴** | "isovalue 슬라이더 변경 → 즉시 렌더 갱신" / "Apply 버튼 클릭 후에만 적용" 같은 *동기/비동기 응답 정책* |
| **상태 전이 규칙** | 모드 토글 시 다른 옵션의 활성/비활성 변화, 의존 위젯의 *grey-out* 규칙 |
| **단축키 / 우클릭 메뉴** | legacy 가 가졌던 모든 키보드 단축키와 컨텍스트 메뉴 |
| **에러/안내 메시지의 텍스트와 시점** | 사용자가 잘못된 입력을 했을 때 표시되는 메시지의 *내용과 등장 시점* |
| **애니메이션 / 동적 업데이트 빈도** | 매 프레임 갱신 / 사용자 입력 변화 시점 갱신 / Apply 시점 갱신 정책 |
| **윈도우 크기·dock 정책 / 탭 구성** | legacy 가 default 로 설정한 윈도우 크기, dockspace 안의 위치, 탭 / 패널 분할 |

**변경 허용 (오히려 권장)**:

- 한국어 깨진 인코딩 주석 → Doxygen 영문 주석 또는 정상 한국어 주석
- 디버그 print/log 문 → spdlog 로 통일 또는 제거
- 미사용 메서드 / 죽은 코드 / 중복 분기 → 정리
- legacy 의 `m_parent->X()` 호출 → 새 트리의 `controller_.X()` / `SceneState&` DI 분기
- namespace `atoms::ui` → `features::<menu>::<sub>`

#### 6.0.2 보존 검증 절차 (각 sub-phase PR 의 필수 단계)

PR 작성자는 각 sub-phase 의 PR 본문에 다음을 명시한다.

1. **Side-by-side 스크린샷** — legacy (Phase 0 baseline) vs 새 트리의 같은 윈도우. 위젯 배치, 텍스트, 색상, 레이아웃이 시각적으로 동일한지 검증.
2. **사용자 시나리오 정합성 테스트** — 본 메뉴의 *대표 사용자 시나리오 1~3 개* 를 legacy 와 새 트리에서 차례로 재현해 동일 결과가 나오는지 확인 (예: BZ 의 경우 *"path=All, npoints=50, Show BZ Plot 클릭"* 의 결과 비교).
3. **legacy UI 흐름의 변경이 발생한 부분이 있다면 *명시적 사유 기록*** — PR 본문에 `Intentional UI deviation` 섹션 신설, 변경 사유와 사용자 영향 분석.

#### 6.0.3 본 지침의 정신

새 아키텍처 전환의 가치는 *"코드 트리의 이해 용이성"* 이지 *"UI 의 재설계"* 가 아니다. 사용자가 Phase 0 → Phase 3 → Phase 4 의 머지 흐름을 따라가면서 **자기 워크플로우를 다시 학습할 필요가 없어야** 한다. 메뉴 이름·위젯 위치·키보드 단축키 등이 그대로 유지되어야 사용자가 *"리팩터링이 끝났다"* 를 *"새 버전이 출시되었다"* 로 오해하지 않는다.

> **위반 시**: 위 *변경 금지* 항목 중 하나라도 의도적으로 변경된 PR 은 **검토자가 reject** 하거나, *Intentional UI deviation* 사유서가 첨부되어 있어야 머지된다.

#### 6.0.4 라인수 압축 정책과의 조화 (Phase 3.1 §1.3 의 학습)

Phase 3.1 평가서에서 입증된 *legacy 의 누적 부채 정리 + 코드 압축* 패턴 (라인수 53%) 은 본 지침과 충돌하지 않는다.

| 압축 가능 (예) | 보존 필수 (예) |
|---|---|
| ImGui 위젯 *주변* 의 한국어 깨진 주석 정리 | ImGui 위젯 자체 (`ImGui::SliderFloat("isovalue", &v, 0, 1)`) 의 인자 (라벨, 범위, 기본값) |
| 디버그 print 제거 | `if (ImGui::Button("Apply")) { renderer.Update(); }` 의 *흐름과 응답 시점* |
| 외부 클래스 forward + 호출 단순화 | 사용자가 보는 *결과* (Apply 버튼이 즉시 vs 다음 프레임에 갱신하는지) |

→ **압축 ≠ UI 변경**. UI 동작은 보존하되, 그 주위의 *코드 정돈* 만 자유롭게 한다.

---

권장 순서 (의존이 적은 → 많은):

### 3.1 `features/utilities/brillouin_zone` (3~4일)
- legacy 참조: `atoms/domain/{bz_plot, special_points}.*` + `atoms/infrastructure/bz_plot_layer.*` + `atoms/ui/bz_plot_ui.*`.
- SceneState 에서 currentStructureId 만 읽음. 다른 의존 없음.
- 메뉴: `Utilities / Brillouin Zone`.

### 3.2 `features/data/charge_density` + `features/data/slice` (4~5일)
- legacy 참조: `atoms/domain/charge_density.*`, `atoms/infrastructure/{charge_density_renderer, slice_renderer}.*`, `atoms/ui/charge_density_ui.*`.
- 파서는 `core/io/chgcar_parser` 사용.
- 메뉴: `Data / Isosurface · Surface · Volumetric · Plane`.

### 3.3 `features/build/{periodic_table, bravais}` (3~4일)
- legacy 참조: `atoms_template_periodic_table.cpp`, `atoms_template_bravais_lattice.cpp`, `atoms/domain/{crystal_structure, crystal_system}.*`, `atoms/ui/{periodic_table_ui, bravais_lattice_ui}.*`.
- 메뉴: `Build / Add atoms`, `Build / Bravais Lattice Templates`.

### 3.4 `features/edit/{atoms, bonds, cell}` (5~7일)
- legacy 참조: `atoms/domain/{atom_manager, bond_manager, cell_manager, surrounding_atom_manager}.*`, `atoms/ui/{atom_editor_ui, bond_ui, cell_info_ui}.*`, `atoms/infrastructure/{vtk_renderer, bond_renderer}.*`.
- **여기서 `vtk_renderer.cpp` 1,792 줄을 atom/bond/cell 별 렌더러로 분할**한다 — 가장 무거운 작업.
- 메뉴: `Edit / Atoms · Bonds · Cell`.

### 3.5 `features/measurement` (4~5일)
- legacy 참조: `atoms_template.cpp` 의 측정 관련 메서드 전부 (Enter/Exit/Click/Drag/Render/Store/Visible).
- `core/vtk::mouse_interactor` 의 픽킹 이벤트를 구독.
- 메뉴: `Measurement / Distance · Angle · Dihedral · Geometric Center · Center of Mass`.

### 3.6 `features/file` (2~3일)
- legacy 참조: `file_loader.cpp` 의 메뉴 핸들러 부분 + `app.cpp` 의 File 메뉴 분기.
- `core/io/format_registry` 사용해서 파서 자동 선택.
- 메뉴: `File / Open Structure File`.

### 3.7 `features/viewer` (2일)
- legacy 참조: `toolbar.cpp/h` + `app.cpp` 의 Viewer 창 부분.
- 메뉴: `Windows / Viewer`. 툴바는 viewer_panel 안에서 그림.

### 3.8 `features/model_tree` (2~3일)
- legacy 참조: `model_tree.cpp` 전부.
- 호출 패턴 정비: `AtomsTemplate::Get*` → `core/scene::structure_registry::*` 로 대체.
- 메뉴: `Windows / Model Tree`.

### 3.9 `features/mesh` (2일)
- legacy 참조: `mesh.*`, `mesh_manager.*`, `mesh_group*.*`, `mesh_detail.*`, `unv_reader.*` (이미 core/io 로 이동했으면 그쪽에서 import).
- mesh 는 별도 메뉴 항목이 없고 Model Tree / Toolbar 에서만 노출된다는 점 유의.

각 PR 의 끝에서:
- `02_menu_tree.md` 의 체크리스트 중 해당 항목을 ✓ 표시.
- 빌드 통과 + 해당 메뉴 항목 수동 확인.

## 7. Phase 4 — `app/menu_router` + `app.cpp` 슬림화

**목표**: `app.cpp::renderDockSpaceAndMenu` 를 `MenuRouter::DrawMenuBar()` 한 줄로 줄인다.

작업:
1. `app/menu_router.cpp` 작성 (위 03 문서의 13줄짜리 디스패치 코드).
2. `app/menu_request.h` 의 4종 Request 정의:
   - `OpenWindow{ WindowId }`
   - `EnterMode{ ModeId }`
   - `InvokeAction{ ActionId, params... }`
   - `TogglePref{ PrefKey, value }`
3. `app/window_flags.h` 작성. `m_bShow*` 18+ 개를 한 구조체에.
4. `app/about.*`, `app/settings.*`, `app/layout_manager.*` 작성. `legacy/app.cpp` 의 해당 코드 옮김.
5. `app/app.cpp` 는 다음만 책임:
   - 초기화 / 종료
   - dockspace 한 번 그리기
   - `MenuRouter::DrawMenuBar()`
   - 각 feature 의 `RenderWindows(window_flags_)` 순서 호출
   - About 모달 / Background Color 팝업 같은 셸 전용 모달

**검증**: 모든 메뉴 항목이 동일하게 동작.

## 8. Phase 5 — legacy/ 의존 단절 + 빌드 제외

**목표**: 신규 트리가 `legacy/` 의 어떤 헤더도 include 하지 않는 상태.

작업:
1. `grep -r "legacy/" webassembly/src --include="*.cpp" --include="*.h"` 결과가 0이 되도록 정리.
2. `CMakeLists.txt` 에서 `legacy/` 디렉터리를 `add_subdirectory` 또는 source list 에서 완전히 제거. (디렉터리 자체는 보관 — 참조용)
3. `font_manager.cpp` 처럼 자동생성 파일은 `core/render/font_manager.cpp` 가 동일 데이터를 들고 있어야 함 (재사용 OK, legacy import X).
4. 옵션: `legacy/` 폴더를 별도 git submodule 또는 `archive/legacy/` 로 옮겨 메인 빌드 트리 외부로 격리. (선택)

**검증**: 빌드는 그대로 통과 + `legacy/` 를 한 번에 통째로 비활성화해도 빌드와 런타임 동작이 같음.

## 9. Phase 6 — 마무리

작업:
1. `core/scene/events.h` 의 옵저버 훅을 실제로 사용하도록 정비. 예:
   - `features/measurement::measurement_store` 가 `onAtomsChanged` 를 구독해서 잘못된 측정을 자동 제거.
   - `features/data/charge_density` 가 `onStructureRemoved` 구독.
2. `CMakeLists.txt` 를 feature 별 source 변수로 정돈 (`FEATURES_FILE_SOURCES`, `FEATURES_EDIT_SOURCES`, …).
3. 모든 신규 파일이 `06_doxygen_style_guide.md` 의 컨벤션을 따르는지 점검 (clang-tidy 또는 수동 review).
4. `webassembly/docs/` 갱신: 본 단계 완료 후 발생한 결정/변경 반영.
5. `02_menu_tree.md` 의 체크리스트 100% 완료 확인.

## 10. 단계별 산출물 / 검증 기준 요약

| Phase | 산출물 | 빌드 | 런타임 검증 |
|---|---|---|---|
| 0 | `legacy/` 폴더 + 임시 빌드 제외 | ○ | 빈 캔버스 OK |
| 1 | `app/` + `core/` 빈 셸 | ○ | 빈 dockspace |
| 2 | `core/scene/` + `core/io/` 골격 | ○ | 빈 dockspace |
| 3.1 | `features/utilities/brillouin_zone` | ○ | Utilities/BZ 동작 |
| 3.2 | `features/data/{charge_density, slice}` | ○ | Data/* 4 항목 동작 |
| 3.3 | `features/build/*` | ○ | Build/* 2 항목 동작 |
| 3.4 | `features/edit/*` | ○ | Edit/* 3 항목 동작 |
| 3.5 | `features/measurement` | ○ | Measurement/* 5 항목 동작 |
| 3.6 | `features/file` | ○ | File/Open 동작 (XSF/CHGCAR/UNV) |
| 3.7 | `features/viewer` + 툴바 | ○ | 툴바 7 종 동작 |
| 3.8 | `features/model_tree` | ○ | Model Tree 우클릭/visibility 동작 |
| 3.9 | `features/mesh` | ○ | mesh 로딩/표시 동작 |
| 4 | `menu_router`, `about/settings/layout_manager` | ○ | 모든 메뉴/Settings/Windows/Layout 동일 동작 |
| 5 | legacy 의존 0 | ○ | legacy 빌드 제외해도 동일 동작 |
| 6 | 이벤트 버스, CMake 정돈, 주석 점검 | ○ | 회귀 테스트 통과 |

## 11. 리스크와 완화책

1. **font_manager 재배치 실패**
   - 자동생성 파일이라 디프가 크게 보일 수 있음. → bit-identical 복사로 옮긴 뒤 git mv 또는 SHA 해시 검증.
2. **VTK Renderer 분할의 회귀**
   - `atoms/infrastructure/vtk_renderer.cpp` 1,792 줄을 atom/bond/cell 로 쪼개는 것이 가장 큰 리스크. → Phase 3.4 안에서 작은 PR 로 추가 분할.
3. **Mouse Interactor 이벤트 모델 변환**
   - 현재는 직접 `AtomsTemplate::*` 호출. 이벤트 emit 으로 바꾸면 픽킹 시점/순서 차이 발생 가능. → 같은 행위에 대해 legacy 동작과 새 동작을 토글하는 임시 디버그 옵션 두기.
4. **메뉴 항목 누락**
   - DEBUG 빌드 전용 항목(`Full Dockspace`, `Show Font Icons`)이 release 빌드에서 빠져 있음. → 매크로 가드를 그대로 옮겨 해당 항목을 누락하지 않을 것.
5. **CHGCAR/XSF/UNV 자동 분기**
   - `file_loader.cpp` 의 확장자 분기 로직을 `core/io/format_registry` 로 옮길 때 매핑 누락 위험. → Phase 2 에서 unit test 형태의 매핑 표 작성.

## 12. 한 줄 결론

> **"메뉴 = 폴더" 라는 한 가지 약속을 코드 트리 전체에 강제**하면, 이후 신규 기능은 어디에 넣을지 망설임 없이 정해진다.
> Greenfield 재구성은 그 약속을 어기는 코드가 만들어질 기회 자체를 차단한다.

## 13. UI 이식 공통 지침 (추가)

향후 Phase에서 UI 이식이 포함될 경우, 다음 문서를 필수 준수 기준으로 사용한다.

- [phases/phase_ui_porting_quality_guideline.md](./phases/phase_ui_porting_quality_guideline.md)

적용 규칙:
1. 계획서에 지침 문서 참조를 명시한다.
2. 검증 매트릭스에 지침의 UI-01~UI-12를 최소 포함한다.
3. 레거시 대비 일탈은 `Intentional UI deviation`으로 기록한다.
