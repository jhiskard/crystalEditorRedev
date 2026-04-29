# Phase 2 — core/ 인프라 골격 (Core Skeleton) 세부계획서

> 상위 문서:    [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §5 Phase 2
> 선행 문서:    [`./phase1_app_core_bootstrap.md`](./phase1_app_core_bootstrap.md)
> 선행 평가서:  [`./phase1_evaluation_2026-04-28.md`](./phase1_evaluation_2026-04-28.md)
> 작성일:      2026-04-28
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR:     1~2 개 (`core/scene` + `core/io+data+vtk+render+ui`)
> 예상 소요:   3~5 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-04-28 | 초안 작성 (Phase 1 통합 평가서 §4.4 진입 후 첫 작업 항목을 본 문서로 확장) |

---

## 0. 한 줄 요약

> Phase 1 이 만들어 둔 빈 `core/{vtk, render}/` 위에 **`core/{scene, io, data, ui, vtk, render}/` 의 인프라 골격을 채운다**. 각 모듈은 legacy 의 같은 책임 코드를 *참조* 해 옮기되, 외부 의존(`AtomsTemplate`, `App`, `VtkViewer` 등) 을 미리 분석해 *DI / 이벤트 / 등가 심볼 redirect* 로 끊는다.
> Phase 0 가 *"rename 만"*, Phase 1 이 *"빈 셸"* 이었다면, Phase 2 는 *"피처들이 의지할 인프라 — 단, 아직 누구도 호출하지 않는 코드"* 다. **런타임은 여전히 빈 dockspace** 다.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `core/scene/{scene_state, structure_registry, selection, hover, events}` 작성 — `AtomsTemplate` 의 god 부분 추출. (b) `core/io/{file_dialog, format_registry, xsf_parser, chgcar_parser, rho_parser, unv_reader}` 작성 — 파일 다이얼로그·포맷 매핑·파서 4 종. (c) `core/data/{element_database, colormap, lcrs_tree, string_utils, color}` 이식. (d) `core/vtk/{mouse_interactor, batch_update_system}` 작성 — 인터랙터를 이벤트 emit 으로 추상화 + 배치 업데이트 시스템. (e) `core/render/{image, texture}` 이식 + Phase 1 의 `font_manager` 의 `App::DevicePixelRatio` 의존 정리. (f) `core/ui/{widgets, ui_color_utils, icons/}` 이식. (g) `npm run build-wasm:debug` 통과 + 빈 dockspace 가 그대로 유지됨 |
| **비목표** | features/ 안에 어떤 코드도 작성, 메뉴/툴바/렌더 컨텐츠 부활, 18 항목 회귀 테스트 통과 (Phase 3), Phase 1 의 `legacy_app_compat.cpp` 와 `target_include_directories` 의 `legacy/` 를 제거 (Phase 5 정리 항목으로 남김 — 단 `font_manager` 의 의존만 정리 가능하면 정리), legacy 내부 코드 수정 (회색지대 정책 §1.2 ~ §1.3 예외 참조) |

> Phase 2 의 미덕: *"검토자가 git diff 를 보고 '새 헤더/소스 다수 + CMake source list 확장 + (필요 시) shim N 개' 외에 의심할 게 없다"*. 새 코드가 실제로 *호출되는지* 는 Phase 3 가 도착해야 비로소 검증된다 — 따라서 Phase 2 의 검증은 *컴파일 통과 + 인터페이스 자기 일관성* 에 집중한다.

### 1.1 비목표가 뜻하는 것 — 인프라만, 기능은 그대로 0

| 기능 | Phase 1 후 | Phase 2 후 | 회복 단계 |
|---|---|---|---|
| 빈 dockspace | ✓ | ✓ | — |
| placeholder 메뉴 1~2 항목 | ✓ | ✓ | — |
| 메뉴/툴바/렌더 컨텐츠 | ✗ | ✗ (그대로) | Phase 3, 4 |
| `core/scene/SceneState` 인스턴스 | (없음) | **존재하지만 아직 비어있음** (DI 슬롯만) | Phase 3 부터 채워짐 |
| `core/io/format_registry` 등록 | (없음) | **빈 레지스트리** | Phase 3 의 features/file 진입 시 채움 |
| `core/vtk/mouse_interactor` 의 emit | (없음) | **이벤트 emit 가능하지만 구독자 0** | Phase 3 의 features/measurement 등이 구독 시작 |

> Phase 2 PR 의 검토자에게 *"새 코드가 어디서도 호출되지 않는다"* 는 점이 의심스러워 보일 수 있음을 PR 본문에서 미리 알린다. 본 단계의 산출물은 *"Phase 3 가 가져다 쓸 도구 상자"* 다.

### 1.2 회색지대 정책 인계 (Phase 0 §1.1 + Phase 1 §1.2)

본 계획서에서도 두 회색지대를 그대로 적용한다.

| 회색지대 출처 | 적용 범위 |
|---|---|
| Phase 0 §1.1 — typo build-fix | legacy/ 내부의 1~5 줄 typo 수정 (release/debug 매크로 가드 안에서 노출되는 미해결 식별자 등). Phase 2 에서도 발견 시 별도 PR 또는 분리 commit 으로 처리 |
| Phase 1 §1.2 — 빌드 호환성 shim | 새 트리 측의 얇은 redirect 파일. Phase 2 에서는 **font_manager 의 `App::DevicePixelRatio` 의존을 정식으로 정리** 함으로써 기존 `legacy_app_compat.cpp` shim 1 건이 제거될 가능성이 있다 (§4 Step 1 참조) |

### 1.3 신규 회색지대 — DI 가 불가능한 legacy 호출의 *어댑터 패턴* (참고)

Phase 2 에서 인프라를 작성하다 보면 *"legacy 모듈이 외부 클래스 X 를 호출하지만 새 트리에서는 X 가 아직 없다"* 는 상황이 발생할 수 있다 (Phase 1 의 font_manager 와 동일한 패턴이지만 더 큰 규모로 발생 가능).

이 경우 다음 우선순위로 처리한다.

1. **legacy 호출 자체를 제거** — 새 코드에서 legacy 심볼을 부르지 않도록 다시 짠다 (가장 깔끔, 권장).
2. **DI 로 끊기** — 외부 클래스를 인자/멤버로 받아오게 한다 (예: `MouseInteractor(EventBus& bus)`).
3. **얇은 어댑터** — Phase 1 §1.2 의 회색지대 정책에 부합한다면 그 안에서 처리.

본 계획서는 가능한 한 (1) 을 추구하고, 불가피한 경우만 (2)/(3) 으로 떨어진다 (구체적 사례는 §4 의 각 Step 참조).

---

## 2. 전제 — Phase 1 완료 상태

본 계획서는 다음이 머지된 상태에서 시작한다 (Phase 1 평가서 §4.3 의 4 가지 정리 항목이 모두 통과).

- [ ] Phase 1 §1.X 회색지대 (빌드 호환성 shim) 보강 commit 머지
- [ ] `npm run build-wasm:debug` + `:release` 둘 다 exit 0
- [ ] Phase 1 코드 commit (셸 + shim) 머지
- [ ] `legacy/font_manager.cpp` 변경의 의도성 확인 완료 (또는 원복)
- [ ] `webassembly/src/{app, core/{vtk, render}, features, legacy, main.cpp, bind_function.cpp}` 의 6 항목 트리 확립

위 통과 여부는 [`phase1_evaluation_2026-04-28.md`](./phase1_evaluation_2026-04-28.md) §4.3 의 4 가지 정리 항목으로 확인.

---

## 3. 디렉터리 트리 — Phase 2 종료 시

```
webassembly/src/
│
├─ main.cpp                            ─── (Phase 1 그대로)
├─ bind_function.cpp                   ─── (Phase 1 그대로 — Phase 3.6 에서 stub → 실 구현)
│
├─ app/                                ─── (Phase 1 그대로)
│   ├─ app.cpp / app.h
│   └─ legacy_app_compat.cpp           ★ §4 Step 1 에서 제거 가능성 검토
│
├─ core/
│   ├─ vtk/
│   │   ├─ vtk_viewer.cpp / vtk_viewer.h        (Phase 1)
│   │   ├─ mouse_interactor.cpp / mouse_interactor.h  ★ 신규 — emit 모델
│   │   └─ batch_update_system.cpp / batch_update_system.h  ★ 신규
│   ├─ io/                              ★ 신규 폴더 채움
│   │   ├─ file_dialog.cpp / file_dialog.h
│   │   ├─ format_registry.cpp / format_registry.h
│   │   ├─ xsf_parser.cpp / xsf_parser.h
│   │   ├─ chgcar_parser.cpp / chgcar_parser.h
│   │   ├─ rho_parser.cpp / rho_parser.h
│   │   └─ unv_reader.cpp / unv_reader.h
│   ├─ data/                            ★ 신규 폴더 채움
│   │   ├─ element_database.cpp / element_database.h
│   │   ├─ colormap.cpp / colormap.h
│   │   ├─ lcrs_tree.cpp / lcrs_tree.h
│   │   ├─ string_utils.cpp / string_utils.h
│   │   └─ color.h
│   ├─ scene/                           ★ 신규 폴더 채움 — Phase 2 의 핵심
│   │   ├─ scene_state.cpp / scene_state.h
│   │   ├─ structure_registry.cpp / structure_registry.h
│   │   ├─ selection.cpp / selection.h
│   │   ├─ hover.cpp / hover.h
│   │   └─ events.h
│   ├─ render/
│   │   ├─ font_manager.cpp / font_manager.h     (Phase 1)
│   │   ├─ image.cpp / image.h          ★ 신규
│   │   └─ texture.cpp / texture.h      ★ 신규
│   └─ ui/                              ★ 신규 폴더 채움
│       ├─ widgets.cpp / widgets.h      (legacy/custom_ui.* 가 거의 비어있어 신규 작성)
│       ├─ ui_color_utils.h
│       └─ icons/                       (legacy/icon/ 의 13 종 그대로)
│
├─ features/                            (그대로 비어있음 — Phase 3)
│
└─ legacy/                              (Phase 0 동결본 그대로)
```

### 3.1 핵심 변화 한 줄 요약

| 구분 | Phase 1 후 | Phase 2 후 |
|---|---|---|
| `core/` 의 .cpp 갯수 | 약 3 (vtk_viewer, font_manager, app/legacy_app_compat) | 약 24 |
| `core/` 의 .h 갯수 | 약 3 (vtk_viewer, font_manager, app/app.h) | 약 25 |
| 빌드 시간 | 빠름 (작은 코드) | 중간 (`element_database.cpp` 32KB, `file_io_manager` 의 XSF 파트 등) |
| 런타임 결과 | 빈 dockspace | 빈 dockspace (변화 없음) |

---

## 4. 작업 절차 (단계별)

### Step 1 — Phase 1 의 `legacy_app_compat.cpp` 정리 가능 여부 검토 (선행 정리)

#### 분석

Phase 1 §1.2 의 compat shim 은 `legacy/font_manager.cpp` 의 `App::DevicePixelRatio()` 호출을 redirect 하기 위한 것이다. Phase 2 에서 `core/render/font_manager.cpp` 는 그대로 가져오면서, **호출 코드를 `app::App::DevicePixelRatio()` 로 직접 갱신** 할 수 있다 (회색지대 §1.3 의 우선순위 1 — legacy 호출 자체 제거).

#### 작업

`webassembly/src/core/render/font_manager.cpp` 의 24+ 회 호출을 sed/Python 으로 일괄 치환:

```bash
sed -i 's/\bApp::DevicePixelRatio()/app::App::DevicePixelRatio()/g' \
  webassembly/src/core/render/font_manager.cpp
```

치환 후 `app/legacy_app_compat.cpp` 는 더 이상 필요 없다. **단 `target_include_directories` 의 `legacy/` 항목은 Phase 5 까지 유지** (`bind_function.cpp` 의 stub 들이 향후 Phase 3.6 에서 실 구현으로 교체될 때 legacy 헤더 참조가 일시적으로 필요할 수 있음).

| 액션 | 파일 |
|---|---|
| 수정 | `core/render/font_manager.cpp` (호출 24+ 곳) |
| 삭제 | `app/legacy_app_compat.cpp` |
| `CMakeLists.txt` 갱신 | `SOURCES_APP` 에서 `legacy_app_compat.cpp` 제거 |

> 이 정리가 끝나면 Phase 1 평가서 §5.2.1 의 잔여 리스크 1 건이 해소된다.

### Step 2 — `core/scene/` — `SceneState` 추출

Phase 2 의 핵심. `legacy/atoms/atoms_template.h` 의 god 부분을 추출해 새 컨테이너로 만든다. **legacy 코드는 한 글자도 변경하지 않는다** — 그저 새 데이터 구조를 *별도로* 만든다. legacy 와 새 SceneState 는 Phase 3 가 도착하기 전까지 평행 존재한다.

#### 2.1 `core/scene/scene_state.h` (골격)

```cpp
/**
 * @file core/scene/scene_state.h
 * @brief 모든 피처가 공유하는 장면 상태. AtomsTemplate 의 god 부분 추출.
 *
 * @details
 *  Phase 2 에서는 인터페이스만 정의하고, 실제 사용은 Phase 3 의 features 가
 *  도착해서야 시작된다. 본 헤더의 모든 멤버는 *값 의미* 를 가지며, 외부에서
 *  포인터/싱글턴으로 접근하지 않는다 (DI 로 주입).
 */
#pragma once

#include "structure_registry.h"
#include "selection.h"
#include "hover.h"
#include "events.h"

#include <cstdint>

namespace core::scene {

/**
 * @struct SceneState
 * @brief 장면 전체의 공유 상태 컨테이너.
 *
 * @par 책임
 *  - currentStructureId: 사용자가 현재 편집/관찰 중인 구조의 ID.
 *  - structures: 등록된 모든 구조의 메타데이터 (StructureRegistry 위임).
 *  - selection: 현재 선택된 atom/bond 집합.
 *  - hover: 마우스 위에 있는 atom 정보.
 *  - events: cross-feature broadcast 용 옵저버 버스 (events.h 참조).
 *
 * @par 비책임
 *  - VTK actor 의 보유/렌더링 (각 feature 의 renderer 가 책임).
 *  - 실제 atom 좌표/원소 정보 (각 feature 의 manager 가 책임).
 */
struct SceneState {
    int32_t              currentStructureId = -1;  ///< 현재 편집 중 구조 ID. -1 = 미선택.
    StructureRegistry    structures;                ///< 모든 등록된 구조의 메타.
    SelectionSet         selection;                 ///< 현재 선택 집합.
    HoverInfo            hover;                     ///< 호버 정보.
    EventBus             events;                    ///< cross-feature broadcast.
};

} // namespace core::scene
```

#### 2.2 `core/scene/structure_registry.{cpp,h}`

`legacy/atoms/atoms_template.h` 의 다음 멤버/메서드를 흡수.

| legacy | core/scene/structure_registry |
|---|---|
| `m_Structures` (`std::unordered_map<int32_t, StructureEntry>`) | 멤버 `entries_` |
| `RegisterStructure(int32_t, std::string)` | `Register(int32_t, std::string)` |
| `RemoveStructure(int32_t)` | `Remove(int32_t)` |
| `IsStructureVisible(int32_t)` | `IsVisible(int32_t)` |
| `SetStructureVisible(int32_t, bool)` | `SetVisible(int32_t, bool)` |
| `GetStructures()` | `List()` |
| `HasStructures()` | `Empty()` (반대 의미로) |

`SetVisible` 시 `events.onStructureVisibilityChanged.emit(...)` 을 발행하여 옵저버에게 알림.

#### 2.3 `core/scene/selection.{cpp,h}`

```cpp
/**
 * @class SelectionSet
 * @brief 선택된 atom/bond ID 의 집합.
 *
 * @details legacy 의 `m_SelectedAtom` / `selectedBondIds` 를 통합. 변경 시 events 발행.
 */
class SelectionSet {
public:
    void AddAtom(uint32_t atomId);
    void RemoveAtom(uint32_t atomId);
    void Clear();
    bool ContainsAtom(uint32_t atomId) const;
    const std::unordered_set<uint32_t>& Atoms() const;

    void AddBond(uint32_t bondId);
    void RemoveBond(uint32_t bondId);
    bool ContainsBond(uint32_t bondId) const;
    const std::unordered_set<uint32_t>& Bonds() const;

private:
    std::unordered_set<uint32_t> atoms_;
    std::unordered_set<uint32_t> bonds_;
};
```

#### 2.4 `core/scene/hover.{cpp,h}`

```cpp
/**
 * @struct HoverInfo
 * @brief 마우스 호버 상태. legacy 의 m_HoveredAtom 흡수.
 */
struct HoverInfo {
    uint32_t  atomId      = UINT32_MAX;   ///< 호버 atom ID. UINT32_MAX = 호버 없음.
    int32_t   structureId = -1;           ///< 호버 atom 의 구조 ID.
    double    pickPos[3]  = {0, 0, 0};    ///< 픽킹 좌표.
    bool      hasHover    = false;        ///< 호버 활성 여부.
};
```

#### 2.5 `core/scene/events.h`

```cpp
/**
 * @file core/scene/events.h
 * @brief feature 간 broadcast 를 위한 단일-스레드 이벤트 버스.
 *
 * @details Phase 2 에서는 emit 인터페이스만 정의. 구독은 Phase 3 가 시작하면서 채워진다.
 *          단순 std::function 콜백 리스트로 구현한다 (외부 의존 없음).
 */
#pragma once

#include <functional>
#include <vector>
#include <cstdint>

namespace core::scene {

template <typename Event>
class Bus {
public:
    using Handler = std::function<void(const Event&)>;
    void Subscribe(Handler h)         { subs_.push_back(std::move(h)); }
    void Emit(const Event& e) const   { for (auto& h : subs_) h(e); }
private:
    std::vector<Handler> subs_;
};

// ---- 이벤트 정의들 -----------------------------------------------------------

struct StructureAddedEvent          { int32_t  structureId; };
struct StructureRemovedEvent        { int32_t  structureId; };
struct StructureVisibilityChanged   { int32_t  structureId; bool visible; };
struct AtomsChangedEvent            { int32_t  structureId; };
struct BondsChangedEvent            { int32_t  structureId; };
struct CellChangedEvent             { int32_t  structureId; };
struct SelectionChangedEvent        { /* atomIds 등 */ };

class EventBus {
public:
    Bus<StructureAddedEvent>          onStructureAdded;
    Bus<StructureRemovedEvent>        onStructureRemoved;
    Bus<StructureVisibilityChanged>   onStructureVisibilityChanged;
    Bus<AtomsChangedEvent>            onAtomsChanged;
    Bus<BondsChangedEvent>            onBondsChanged;
    Bus<CellChangedEvent>             onCellChanged;
    Bus<SelectionChangedEvent>        onSelectionChanged;
};

} // namespace core::scene
```

> Phase 2 의 SceneState 는 *비어있는 컨테이너* 다. 등록된 구조 0, 선택 0, 호버 비활성. Phase 3 의 features/file 이 import 한 구조를 `SceneState::structures.Register(...)` 로 등록하기 시작하면서 비로소 살아난다.

### Step 3 — `core/io/` — file_dialog + format_registry + parsers

가장 무거운 단계. `legacy/file_loader.cpp` 가 52 KB 의 단일 파일이고 *Emscripten 다이얼로그 + 청크 전송 + 확장자 분기 + Embind 콜백* 을 한 군데에 묶어두었다. 본 단계에서 4 개로 쪼갠다.

#### 3.1 `core/io/file_dialog.{cpp,h}` (Emscripten 다이얼로그 + 청크 전송)

`legacy/file_loader.cpp` 에서 다음만 추출.

- `RequestOpenStructureImport()` (다이얼로그 호출)
- `WriteChunk(...)` (청크 전송)
- `CloseFile(...)` (다이얼로그 종료)
- `LoadArrayBuffer(...)` (메모리 → 파일시스템 전송)

> 이때 `AtomsTemplate::LoadXSF…` / `…ChgcarParsedData` 같은 직접 호출은 **모두 제거** 한다. 본 모듈은 *순수 IO* 만 담당. 파싱 결과를 어떻게 처리할지는 호출자가 결정한다 (Phase 3.6 의 `features/file`).

#### 3.2 `core/io/format_registry.{cpp,h}`

확장자 → 파서 함수 포인터 매핑. **전략 패턴**.

```cpp
/**
 * @class FormatRegistry
 * @brief 파일 확장자별 파서를 등록·조회하는 정적 레지스트리.
 *
 * @details
 *  Phase 2 에서는 등록 메커니즘만 작성. 실제 등록은 Phase 3.6 의 features/file 이
 *  진입점에서 한 번 호출 ( `RegisterDefaults()` ) 하여 채운다.
 */
class FormatRegistry {
public:
    using ParseFn = std::function<bool(const std::string& path, ParseResult& out)>;

    /// @brief 확장자(소문자, 점 포함 — 예: ".xsf") 와 파서 함수를 등록.
    void Register(std::string ext, ParseFn fn);

    /// @brief 확장자에 해당하는 파서를 호출. 등록이 없으면 false.
    bool Parse(const std::string& path, ParseResult& out) const;

    /// @brief 본 트리의 모든 파서를 등록 (Phase 3.6 에서 호출).
    static void RegisterDefaults(FormatRegistry& reg);

private:
    std::unordered_map<std::string, ParseFn> entries_;
};
```

`ParseResult` 는 *XSF / CHGCAR / RHO / UNV 모든 파서가 공통으로 채울 수 있는 union-like 구조* 가 좋다. 또는 `std::variant<...>`. 본 계획서는 `std::variant` 권장.

#### 3.3 `core/io/{xsf, chgcar, rho}_parser.{cpp,h}`

| legacy 위치 | core/io 의 새 파일 | 변경 사항 |
|---|---|---|
| `legacy/atoms/infrastructure/file_io_manager.cpp` 의 XSF 파트 (약 10~12 KB) | `core/io/xsf_parser.{cpp,h}` | 분리 + namespace `core::io` 로 이동. `AtomsTemplate` 호출 제거 |
| `legacy/atoms/infrastructure/chgcar_parser.{cpp,h}` (15 KB) | `core/io/chgcar_parser.{cpp,h}` | 거의 그대로 + namespace 이동 |
| `legacy/atoms/infrastructure/rho_file_parser.h` (559 B) | `core/io/rho_parser.{cpp,h}` | header-only 였다면 cpp 로 분리 검토 |

#### 3.4 `core/io/unv_reader.{cpp,h}`

`legacy/unv_reader.{cpp,h}` 의 거의 그대로 이식. namespace `core::io` 로.

> *체크포인트*: 각 파서가 외부 클래스(`AtomsTemplate`, `MeshManager` 등) 를 호출하지 않는지 확인. 호출이 있다면 **호출을 제거** 하고 *결과 데이터를 ParseResult 에 채워서 반환만* 하도록 정리. 호출자(Phase 3.6) 가 결과를 받아 SceneState 에 반영.

### Step 4 — `core/data/` — 데이터/유틸 이식

| legacy | core/data | 비고 |
|---|---|---|
| `atoms/domain/element_database.{cpp,h}` (32 KB + 8 KB) | `element_database.{cpp,h}` | namespace `atoms::domain` → `core::data` |
| `common/colormap.{cpp,h}` | `colormap.{cpp,h}` | namespace 통일 |
| `lcrs_tree.{cpp,h}` | `lcrs_tree.{cpp,h}` | 그대로 |
| `common/string_utils.{cpp,h}` | `string_utils.{cpp,h}` | 그대로 |
| `atoms/domain/color.h` | `color.h` | 그대로 |

> 이식 시 namespace 정리만 하고 알고리즘은 손대지 않는다. legacy 코드의 알려진 결함이 있더라도 **Phase 2 에서 고치지 않는다** (회색지대 정책 §1.2 ~ §1.3 의 *DI 가 불가능한 외부 호출* 정도만 정리).

### Step 5 — `core/vtk/` — mouse_interactor + batch_update_system

#### 5.1 `core/vtk/mouse_interactor.{cpp,h}` — emit 모델로 추상화

legacy `mouse_interactor_style.cpp` (4 KB) 는 ImGui 입력에 따라 `AtomsTemplate::SelectAtomByPicker` / `HandleMeasurementClickByPicker` / `HandleMeasurementEmptyClick` / `HandleDragSelectionInScreenRect` 를 직접 호출한다. Phase 2 에서는 이 직접 호출을 **이벤트 emit** 으로 교체.

```cpp
/**
 * @class MouseInteractor
 * @brief VTK 인터랙터 스타일을 ImGui 입력으로 받아 이벤트를 emit 한다.
 *
 * @par 발행 이벤트
 *  - LeftClickEvent { actor, pickPos[3] }
 *  - DragSelectionEvent { x0, y0, x1, y1, additive }
 *  - HoverChangedEvent { actor, pickPos[3] }
 *
 * @par 비책임
 *  - feature 별 분기 (atoms 픽킹 / measurement 클릭 등) 는 features 가 구독해서 처리.
 */
class MouseInteractor {
public:
    explicit MouseInteractor(core::scene::EventBus& bus);
    void Update(float dt);  // ImGui 입력 → 이벤트 emit
    ...
private:
    core::scene::EventBus& bus_;
};
```

> 이벤트 enum/struct 는 `core/scene/events.h` 에 추가하거나 별도 `core/vtk/vtk_events.h` 로 분리.

#### 5.2 `core/vtk/batch_update_system.{cpp,h}`

legacy `atoms/infrastructure/batch_update_system.{cpp,h}` 의 거의 그대로. namespace 만 `atoms::infrastructure` → `core::vtk` 로.

> RAII `BatchGuard` 패턴이 그대로 유지되어야 한다 (legacy 의 다수 호출자가 의존). Phase 3 의 features 가 import 시 같은 패턴으로 사용한다.

### Step 6 — `core/ui/` — widgets, ui_color_utils, icons

#### 6.1 `core/ui/widgets.{cpp,h}` — 신규 작성

`legacy/custom_ui.{cpp,h}` 는 거의 비어있는 파일이다 (.cpp 549 B / .h 247 B — placeholder 만). `app.cpp` 안에 `AddTooltip`, `IconButton` 같은 헬퍼가 흩어져 있는 것을 본 단계에서 추출하여 `core/ui/widgets` 로 정리.

```cpp
namespace core::ui {

/// @brief 마우스 호버 시 풍선말 툴팁을 표시.
void AddTooltip(const char* itemLabel, const char* tooltipText);

/// @brief 아이콘 + 라벨이 있는 버튼.
bool IconButton(const char* icon, const char* label, const ImVec2& size = {0, 0});

}
```

#### 6.2 `core/ui/ui_color_utils.h`

`legacy/atoms/ui/ui_color_utils.h` (1 KB) 그대로 이식. namespace 정리.

#### 6.3 `core/ui/icons/`

`legacy/icon/` 의 13 종 헤더 (`FontAwesome6.h`, `MaterialDesign.h` 등) 를 그대로 복사.

> 이는 ImGui 의 폰트 아이콘 매크로 정의 헤더로, 자기완결적이고 외부 의존이 없으므로 byte-identical copy 가 안전.

### Step 7 — `core/render/` — image, texture (font_manager 정리)

#### 7.1 `core/render/image.{cpp,h}`, `core/render/texture.{cpp,h}` 이식

`legacy/image.*` (1.5 KB), `legacy/texture.*` (3.2 KB) — 작고 자기완결적이므로 그대로 이식 + namespace `core::render` 로 정리.

#### 7.2 `font_manager` 호출부 갱신 (Step 1 의 후속)

Step 1 에서 sed 로 갱신한 결과를 검토. `core/render/font_manager.cpp` 에 잔여 `App::DevicePixelRatio()` 가 있는지 grep:

```bash
grep -nE "\bApp::DevicePixelRatio" webassembly/src/core/render/font_manager.cpp || echo "OK — 0 hit"
```

### Step 8 — CMakeLists.txt 갱신

`SOURCES_CORE` 변수를 모든 신규 모듈을 포함하도록 확장.

```cmake
set(SOURCES_CORE
    # vtk
    webassembly/src/core/vtk/vtk_viewer.cpp
    webassembly/src/core/vtk/vtk_viewer.h
    webassembly/src/core/vtk/mouse_interactor.cpp
    webassembly/src/core/vtk/mouse_interactor.h
    webassembly/src/core/vtk/batch_update_system.cpp
    webassembly/src/core/vtk/batch_update_system.h
    # io
    webassembly/src/core/io/file_dialog.cpp
    webassembly/src/core/io/file_dialog.h
    webassembly/src/core/io/format_registry.cpp
    webassembly/src/core/io/format_registry.h
    webassembly/src/core/io/xsf_parser.cpp
    webassembly/src/core/io/xsf_parser.h
    webassembly/src/core/io/chgcar_parser.cpp
    webassembly/src/core/io/chgcar_parser.h
    webassembly/src/core/io/rho_parser.cpp
    webassembly/src/core/io/rho_parser.h
    webassembly/src/core/io/unv_reader.cpp
    webassembly/src/core/io/unv_reader.h
    # data
    webassembly/src/core/data/element_database.cpp
    webassembly/src/core/data/element_database.h
    webassembly/src/core/data/colormap.cpp
    webassembly/src/core/data/colormap.h
    webassembly/src/core/data/lcrs_tree.cpp
    webassembly/src/core/data/lcrs_tree.h
    webassembly/src/core/data/string_utils.cpp
    webassembly/src/core/data/string_utils.h
    webassembly/src/core/data/color.h
    # scene
    webassembly/src/core/scene/scene_state.cpp
    webassembly/src/core/scene/scene_state.h
    webassembly/src/core/scene/structure_registry.cpp
    webassembly/src/core/scene/structure_registry.h
    webassembly/src/core/scene/selection.cpp
    webassembly/src/core/scene/selection.h
    webassembly/src/core/scene/hover.cpp
    webassembly/src/core/scene/hover.h
    webassembly/src/core/scene/events.h
    # render
    webassembly/src/core/render/font_manager.cpp
    webassembly/src/core/render/font_manager.h
    webassembly/src/core/render/image.cpp
    webassembly/src/core/render/image.h
    webassembly/src/core/render/texture.cpp
    webassembly/src/core/render/texture.h
    # ui
    webassembly/src/core/ui/widgets.cpp
    webassembly/src/core/ui/widgets.h
    webassembly/src/core/ui/ui_color_utils.h
)
```

> Step 1 의 `legacy_app_compat.cpp` 제거가 성공했다면 `SOURCES_APP` 에서 그 줄도 함께 제거.

### Step 9 — legacy/ 와의 의존 사전 분석 (자동생성/외부 호출 함정 sweep)

Phase 1 의 가장 큰 교훈: **font_manager 가 자동생성으로 알려졌지만 실제로는 `App::DevicePixelRatio()` 를 24+ 회 호출하는 비자기완결 파일** 이었다는 점. Phase 2 에서 같은 함정이 또 있는지 사전에 sweep.

#### 9.1 외부 클래스 호출 sweep

각 이식 대상 파일에서 다음 grep:

```bash
for f in webassembly/src/legacy/atoms/domain/element_database.cpp \
         webassembly/src/legacy/common/colormap.cpp \
         webassembly/src/legacy/lcrs_tree.cpp \
         webassembly/src/legacy/atoms/infrastructure/chgcar_parser.cpp \
         webassembly/src/legacy/atoms/infrastructure/file_io_manager.cpp \
         webassembly/src/legacy/atoms/infrastructure/batch_update_system.cpp \
         webassembly/src/legacy/mouse_interactor_style.cpp \
         webassembly/src/legacy/file_loader.cpp \
         webassembly/src/legacy/unv_reader.cpp \
         webassembly/src/legacy/image.cpp \
         webassembly/src/legacy/texture.cpp; do
    echo "=== $f ===" 
    grep -nE "AtomsTemplate::|VtkViewer::|App::|MeshManager::|MeshDetail::|ModelTree::" "$f" | head -10
done
```

발견된 호출은 다음 두 가지 정책으로 처리:

| 발견 | 처리 |
|---|---|
| feature-도메인 호출 (`AtomsTemplate::Get*`, `MeshDetail::Render*`) | 그 자리에서 *제거* 하고 결과 데이터만 반환하도록 정리 (호출자 Phase 3 가 SceneState 에 반영) |
| 인프라 호출 (`App::DevicePixelRatio`, `VtkViewer::GetRenderer`) | DI 로 끊거나 (생성자 인자로 받음) 등가 신트리 심볼로 직접 redirect |

#### 9.2 자동생성 데이터 후보 sweep

자동생성으로 의심되는 파일을 식별 (라인 수 / 반복 패턴):

```bash
for f in webassembly/src/legacy/font_manager.cpp \
         webassembly/src/legacy/icon/*.h; do
    echo "$(wc -l < "$f") $f"
done
```

본 sweep 의 결과를 §9 부록의 *"의존 분석 표"* 로 정리하여 PR 본문에 첨부.

### Step 10 — 정적 검증

```bash
cd webassembly/src

# core/ 트리 점검
ls core/        # 기대: data io render scene ui vtk
for d in core/{data,io,render,scene,ui,vtk}; do
  echo "=== $d ===" ; ls $d
done

# legacy 호출 잔여 0 확인 (core/* 안에서)
grep -rnE "AtomsTemplate::|MeshManager::|MeshDetail::|ModelTree::" core/ \
  | grep -v "//" || echo "OK — feature-domain 호출 0"

# core/render/font_manager 의 App:: 잔여 0 확인 (Step 1 후속)
grep -nE "\bApp::DevicePixelRatio" core/render/font_manager.cpp \
  | grep -v "//" || echo "OK — App:: 0 hit"

# CMake 안전벨트 유지 확인
grep -a "Phase 1 violation" ../../CMakeLists.txt
```

### Step 11 — 빌드 + 런타임 검증

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release
```

기대:
- emcc 가 약 24 개 .cpp 파일 컴파일.
- 링크 시 모든 새 심볼이 main / bind / app 에서 호출되지 않으므로 dead-code-elim 으로 사라질 가능성 높음 — 그 경우 wasm 사이즈가 Phase 1 과 거의 동일.
- legacy/ 는 한 줄도 컴파일되지 않음 (안전벨트 유효).

```powershell
npm run dev
# 브라우저 → http://localhost:3000/workbench
```

기대:
- Phase 1 과 동일한 빈 dockspace + placeholder 메뉴.
- 콘솔 에러 0.
- Embind stub 호출 시 no-op (Phase 1 과 동일).

> Phase 2 의 검증은 *기능이 회복되었는지* 가 아니라 *기능 회복 도구가 컴파일 가능하고 인터페이스가 자기일관적인지* 다.

### Step 12 — 커밋 & PR

본 계획서는 **PR 분할** 을 권장한다.

#### 12.A PR 1 — `core/{scene, data}` (가벼움)

- `core/scene/` 5 개 파일 (인터페이스만, 알고리즘 없음)
- `core/data/` 9 개 파일 (legacy 의 그대로 이식 + namespace)
- `CMakeLists.txt` 부분 갱신
- 검증: 빌드 통과 + 빈 dockspace 동일

#### 12.B PR 2 — `core/{io, vtk, render, ui}` + Step 1 정리 (무거움)

- `core/io/` 6 개 파일 + `format_registry`
- `core/vtk/` 4 개 파일 (mouse_interactor + batch_update_system)
- `core/render/` 4 개 파일 (image, texture, font_manager 호출부 갱신)
- `core/ui/` 2 개 파일 + icons/ 13 개
- Step 1: `app/legacy_app_compat.cpp` 제거 + `SOURCES_APP` 갱신
- `CMakeLists.txt` 의 `SOURCES_CORE` 완전 갱신
- 검증: 빌드 통과 + 빈 dockspace 동일

#### 커밋 메시지 템플릿

```
Phase 2 [PR1/PR2]: build core/<modules> infrastructure

- Add core/<modules>/* with the same logic as legacy reference,
  but with feature-domain calls (AtomsTemplate, MeshDetail, …) removed.
- All new code is dead-code-eliminated at link time because no caller
  exists yet — features arrive in Phase 3.
- Build passes; runtime is the same empty dockspace as Phase 1.

Reference:
  - webassembly/docs/05_redevelopment_plan.md (Phase 2)
  - webassembly/docs/phases/phase2_core_skeleton.md
```

---

## 5. 검증 매트릭스

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `core/` 의 6 sub-folder 모두 비어있지 않음 | `for d in core/{data,io,render,scene,ui,vtk}; do ls $d; done` | 모두 파일 1+ 존재 | 정적 |
| 2 | `core/scene/` 의 SceneState + 4 sub-module | `ls core/scene` | `events.h hover.{cpp,h} scene_state.{cpp,h} selection.{cpp,h} structure_registry.{cpp,h}` | 정적 |
| 3 | `core/io/` 의 6 모듈 (file_dialog + format_registry + 4 parsers) | `ls core/io` | 12 파일 (.cpp + .h × 6) | 정적 |
| 4 | `core/data/` 의 5 모듈 | `ls core/data` | 9 파일 (4×.cpp + 4×.h + color.h) | 정적 |
| 5 | `core/vtk/` 의 추가 2 모듈 | `ls core/vtk` | `vtk_viewer.* mouse_interactor.* batch_update_system.*` (6 파일) | 정적 |
| 6 | `core/render/` 의 추가 2 모듈 + font_manager | `ls core/render` | `font_manager.* image.* texture.*` (6 파일) | 정적 |
| 7 | `core/ui/` 의 widgets + ui_color_utils + icons/ | `ls core/ui` | 3 항목 (`widgets.* ui_color_utils.h icons/`) | 정적 |
| 8 | core/ 안의 feature-domain 호출 0 | `grep -rnE "AtomsTemplate::\|MeshManager::\|MeshDetail::\|ModelTree::" core/` (주석 제외) | 0 hit | 정적 |
| 9 | core/render/font_manager 의 App:: 잔여 0 | `grep -nE "\bApp::DevicePixelRatio" core/render/font_manager.cpp` (주석 제외) | 0 hit | 정적 |
| 10 | `app/legacy_app_compat.cpp` 제거 | `ls app/` | `app.cpp app.h` (2) — Step 1 의 정리 결과 | 정적 |
| 11 | CMake 안전벨트 유지 | `grep -a "Phase 1 violation" CMakeLists.txt` | 1 hit | 정적 |
| 12 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 13 | Release 빌드 | `npm run build-wasm:release` | exit 0 | 동적 |
| 14 | wasm 사이즈 회귀 | Phase 1 의 wasm 크기 ± 5 % 이내 | 동일 수준 | 동적 |
| 15 | 빈 dockspace 표시 | `npm run dev` 후 브라우저 | Phase 1 과 동일 | 동적 |
| 16 | 콘솔 에러 0 | DevTools 콘솔 | 0 errors | 동적 |
| 17 | legacy/ 미컴파일 | ninja 로그 검색 | "webassembly/src/legacy/" 가 한 번도 안 보여야 정상 | 동적 |
| 18 | (선택) 자가검증 — `core::scene::EventBus` 의 emit/subscribe 단위 테스트 | 임시 main 안에서 emit 후 subscribe 가 호출되는지 1 회 확인 | 호출됨 | 동적 |

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | legacy 의 외부 클래스 호출이 grep sweep 에서 발견 안 됨 | Phase 2 PR 빌드 통과 후 Phase 3 가 첫 호출 시 link error | Step 9 의 sweep 을 grep + ctags 결합으로 강화. PR 본문에 의존 분석 표 첨부 |
| 6.2 | `file_io_manager.cpp` 의 XSF 파트와 atoms 도메인 파트 경계가 모호 | 분리 시 누락/중복 | XSF 파서 함수 이름(`parseXSFFile`, `parseXSFGrid` 등) 만 추출하고 도메인 호출은 모두 끊는다. 결과는 `ParseResult` 로만 반환 |
| 6.3 | `element_database.cpp` (32 KB) 의 namespace 변경 시 컴파일 에러 다수 | 빌드 실패 | namespace 변경은 sed 일괄 + 컴파일 한번씩 점검. legacy 측은 손대지 않음 |
| 6.4 | `mouse_interactor` 의 emit 모델이 legacy 의 직접 호출과 의미가 미묘하게 다름 (예: 동기/비동기 차이) | Phase 3 의 features/measurement 가 동작 안 함 | Phase 2 에서는 emit 인터페이스만 정의. 동기 호출 보장 (Bus::Emit 이 즉시 모든 구독자 호출). Phase 3.5 에서 features/measurement 가 들어올 때 검증 |
| 6.5 | `core/io/format_registry` 의 `ParseResult` variant 가 모든 파서를 커버하지 못함 | XSF/CHGCAR/RHO/UNV 의 결과 형태가 너무 달라 통합 어려움 | 변형 (variant) 보다는 *각 파서가 자체 결과 struct 를 채우고 caller 가 그 struct 의 데이터를 SceneState 로 옮긴다* 는 패턴 채택. format_registry 는 *어떤 파서를 호출할지* 만 결정. ParseResult 는 추후 정의 |
| 6.6 | font_manager 의 Step 1 sed 가 다른 코드(주석 등) 의 `App::` 도 변경 | 의도하지 않은 변경 | sed 전후로 `git diff` 검토. 본 작업은 font_manager.cpp 단일 파일에만 적용 |
| 6.7 | core/data/element_database 가 legacy 의 `atoms::domain` namespace 와 충돌 | 컴파일 에러 또는 ODR 위반 | core/data 는 `core::data` namespace 만 사용. legacy 는 컴파일 대상 외이므로 ODR 충돌 없음 |
| 6.8 | Phase 2 PR 의 diff 가 너무 커서 검토 부담 | 머지 지연 | §4 Step 12 의 PR 분할 (PR1: scene+data / PR2: io+vtk+render+ui) 강력 권장 |
| 6.9 | `.git/index.lock` 일관성 이슈 (Phase 0 §7.9) | Linux 측 git 작업 실패 | Linux 측은 파일 작성/이동만, git 은 PowerShell 측에서 |
| 6.10 | 자동생성 데이터의 외부 의존 함정 (Phase 1 의 font_manager 와 동일 패턴) | 빌드 실패 또는 link error | Step 9 의 sweep 으로 사전 점검. 발견 시 Phase 1 §1.2 회색지대 정책으로 처리 |
| 6.11 | 18 항목 회귀 미통과를 본 검토자가 *"기능 또 손상"* 으로 거절 | 머지 지연 | PR 본문에 *"본 PR 은 인프라만 추가, 기능 회복은 Phase 3"* 명시. Phase 1 PR 의 *"의도적 기능 손실 표"* 인용 |

---

## 7. 롤백 절차

```bash
# 변경 취소 (커밋 전)
git restore --staged .
git restore .
git clean -fd webassembly/src/core/{scene,io,data,ui}
# 단, 이미 Phase 1 에서 만든 core/{vtk,render} 의 일부는 살려야 하므로 신중

# 또는 커밋 후라면
git reset --hard HEAD~1   # PR 별로 역순
```

원인 진단:

1. 빌드 에러
   - `error: 'AtomsTemplate' has not been declared` → core 안에서 legacy 호출 잔여. Step 9 sweep 강화.
   - `multiple definition of 'X'` → namespace 충돌. legacy 와 core 가 같은 심볼 노출. core/ 의 namespace 를 `core::*` 로 통일.
   - `undefined reference to 'core::scene::SceneState::...'` → 새 .cpp 파일이 CMakeLists 에 누락. `SOURCES_CORE` 점검.
2. 런타임 에러
   - 빈 dockspace 가 다른 모양으로 변형 → app/app.cpp 가 새 core 의 무엇인가를 호출하기 시작했다는 뜻 (Phase 2 의 비목표 위반). 호출 제거.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전 (PR1, PR2 각각):

- [ ] §2 의 Phase 1 전제 모두 충족
- [ ] §5 검증 매트릭스의 본 PR 범위 항목 통과
- [ ] core 안에서 feature-domain 호출 0 (Step 9 sweep 결과 첨부)
- [ ] `git diff --stat -- webassembly/src/legacy` 의 모든 항목 0 changed lines
- [ ] PR 본문에 *"인프라만 추가, 기능 회복은 Phase 3"* 명시
- [ ] PR 본문에 본 문서와 Phase 1 평가서 링크 포함

검토자 — 머지 전:

- [ ] diff 가 (a) `core/<sub-modules>/` 신규 파일, (b) `CMakeLists.txt` 의 `SOURCES_CORE` 확장, (c) (PR2 의 경우) `app/legacy_app_compat.cpp` 제거 + `core/render/font_manager.cpp` 의 `App::` 갱신, 4 가지로만 구성되어 있는가?
- [ ] `legacy/` 파일은 한 글자도 변경되지 않았는가?
- [ ] CI 빌드 (debug + release) 통과
- [ ] 빈 dockspace 가 본인 환경에서도 그대로 표시 (Phase 1 과 시각적으로 동일)
- [ ] core/ 안의 legacy 호출 0 확인 (`grep -rnE "AtomsTemplate::|MeshManager::|MeshDetail::|ModelTree::" core/`)

---

## 9. 부록

### 9.1 SceneState ↔ AtomsTemplate 매핑 표

Phase 2 에서 legacy `AtomsTemplate` 의 god 부분을 `core/scene/SceneState` 로 추출하면서, 어떤 멤버/메서드가 어디로 가는지를 한 페이지로 정리. Phase 3.4 (`features/edit/atoms`) 가 첫 사용자가 된다.

| legacy `AtomsTemplate` | core/scene 위치 | 비고 |
|---|---|---|
| `int32_t m_CurrentStructureId` | `SceneState::currentStructureId` | 직접 멤버 |
| `std::unordered_map<int32_t, StructureEntry> m_Structures` | `StructureRegistry::entries_` | private + Register/Remove/List API |
| `RegisterStructure(int32_t, const std::string&)` | `StructureRegistry::Register(...)` | events 발행 추가 |
| `RemoveStructure(int32_t)` | `StructureRegistry::Remove(int32_t)` | 동일 |
| `IsStructureVisible(int32_t)` / `SetStructureVisible(int32_t, bool)` | `StructureRegistry::IsVisible/SetVisible` | events 발행 |
| `GetStructures()` | `StructureRegistry::List()` | 스냅샷 반환 |
| `m_HoveredAtom` 관련 (atomId, structureId, pickPos) | `HoverInfo` | struct 통합 |
| `m_SelectedAtom` (단일) + `selectedBondIds` | `SelectionSet` | 다중 선택으로 일반화 |
| `EnterMeasurementMode/...` | (Phase 3.5 의 features/measurement) | SceneState 에 두지 않음 |
| `Hover/Selection 변화 broadcast` | `EventBus::onSelectionChanged` 등 | 신규 |

### 9.2 `core/io/format_registry` 확장자 매핑 표

Phase 3.6 의 `features/file::structure_import::RegisterDefaults(...)` 가 채울 매핑.

| 확장자 | 파서 함수 | 출처 |
|---|---|---|
| `.xsf` | `core::io::ParseXSFFile` | `legacy/atoms/infrastructure/file_io_manager.cpp` 의 XSF 파트 |
| `.xsf` (grid mode) | `core::io::ParseXSFGridFile` | 동상 |
| (없음 / 헤더) | `core::io::ParseChgcarFile` | `legacy/atoms/infrastructure/chgcar_parser.*` |
| `.rho` | `core::io::ParseRhoFile` | `legacy/atoms/infrastructure/rho_file_parser.h` |
| `.unv` | `core::io::ParseUnvFile` | `legacy/unv_reader.*` |

> CHGCAR 는 표준 확장자가 없고 파일명 자체가 `CHGCAR` 인 것이 관례. format_registry 의 매칭 규칙은 *확장자 + (옵션) 파일명 패턴* 두 키를 받도록 설계.

### 9.3 core/data 의 namespace 정리 가이드

| legacy namespace | core/data 의 새 namespace | 적용 파일 |
|---|---|---|
| (전역) | `core::data` | `colormap`, `string_utils`, `lcrs_tree`, `color` |
| `atoms::domain` | `core::data` | `element_database` (사용처는 Phase 3 의 features/edit/atoms, features/build/periodic_table 등) |

> namespace 변경은 sed 일괄 가능. legacy 는 손대지 않으므로 새 트리에서만 작업.

### 9.4 사후 점검: Phase 2 머지 직후 트리

```
webassembly/src/
├─ main.cpp                                (그대로)
├─ bind_function.cpp                       (그대로 — Phase 3.6 에서 변경)
├─ app/
│  ├─ app.cpp / app.h                      (그대로)
│  └─ (legacy_app_compat.cpp 제거됨)
├─ core/
│  ├─ vtk/        ← 6 파일
│  ├─ io/         ← 12 파일
│  ├─ data/       ← 9 파일
│  ├─ scene/      ← 9 파일
│  ├─ render/     ← 6 파일
│  └─ ui/         ← 3 항목 (widgets.{cpp,h} + ui_color_utils.h + icons/)
├─ features/                               (그대로 빈 폴더)
└─ legacy/                                 (Phase 0 동결본 그대로)
```

총 신규 파일: 약 48 개 (.cpp + .h 합산).

---

## 10. 후속 단계 연결 — Phase 3

Phase 2 가 머지되면 Phase 3 (`features/` 의 메뉴별 이식) 의 입구가 열린다. Phase 3 의 진행 순서 (`05_redevelopment_plan.md` §6 권장 순서) 는:

1. **Phase 3.1** — `features/utilities/brillouin_zone` (의존 가장 적음)
2. **Phase 3.2** — `features/data/{charge_density, slice}`
3. **Phase 3.3** — `features/build/{periodic_table, bravais}`
4. **Phase 3.4** — `features/edit/{atoms, bonds, cell}` (가장 무거움 — vtk_renderer 분할)
5. **Phase 3.5** — `features/measurement`
6. **Phase 3.6** — `features/file` (format_registry 의 첫 사용자!)
7. **Phase 3.7** — `features/viewer` + 툴바
8. **Phase 3.8** — `features/model_tree`
9. **Phase 3.9** — `features/mesh`

각 Phase 3.X 는 작은 PR 1~2 개로 끝나도록 분할. 첫 진입(3.1) 에서 Phase 2 의 인프라(SceneState, EventBus 등) 가 실제로 어떻게 사용되는지 검증된다.

Phase 3 세부계획서는 `phase3_<sub>.md` 형태로 각 sub-phase 별로 작성하거나, `phase3_features_migration.md` 단일 문서로 통합 작성 (선택). 본 계획서는 통합 작성을 권장.

---

## 11. 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §5 Phase 2 / §6 Phase 3
- 선행 계획: [`./phase1_app_core_bootstrap.md`](./phase1_app_core_bootstrap.md)
- Phase 1 평가서: [`./phase1_evaluation_2026-04-28.md`](./phase1_evaluation_2026-04-28.md)
- Phase 0 통합 평가서: [`./phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md)
- 새 아키텍처 (`app/`, `core/`, `features/` 의 의미): [`../03_target_architecture.md`](../03_target_architecture.md)
- 메뉴 ↔ 코드 매핑 표 (Phase 3 의 진입점 참조): [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
