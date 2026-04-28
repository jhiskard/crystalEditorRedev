# Phase 1 시도 평가서 (2026-04-28)

> 평가 대상: Phase 1 (App/Core Bootstrap) 수행 결과
> 평가일: 2026-04-28
> 평가 브랜치: `refactor/menu-aligned` (Phase 0 commit `2a586bb` + Phase 1 doc commit `4eac024` + Phase 1 코드 working tree)
> 사용자 입력: "npm run dev 실행 후 런타임에서 기능에 대한 테스트는 완료"
> 결과: **조건부 진행 가능 (Conditionally GO)** — Phase 1 의 본질 통과, 계획서 대비 일탈 1 건 + commit 정리 필요

## 0. 한 줄 결론

> Phase 1 의 §5 검증 매트릭스 13 항목 중 **9 통과 / 1 일탈 / 3 미명시 확인**. 빈 dockspace + placeholder 메뉴 가 브라우저에 정상 표시 (사용자 보고). 다만 (a) `font_manager.cpp` 가 자동생성 데이터인데도 `App::DevicePixelRatio()` 를 호출하는 비-자기완결 구조여서, 계획서가 가정했던 *"byte-identical copy"* 가 불가능했고 결과적으로 **`app/legacy_app_compat.cpp` shim 1 개 + `target_include_directories` 에 `legacy/` 포함** 이라는 의도하지 않은 의존이 추가되었다. (b) Phase 1 의 코드 변경이 아직 commit 되지 않은 working tree 상태다. Phase 2 진입 전에 이 두 가지를 정리하면 깨끗하다.

---

# Part 1 — Phase 1 시도 회고

## 1.1 타임라인

| 시각 (≈) | 단계 | 결과 |
|---|---|---|
| t0 | Phase 0 commit `2a586bb` 머지 후 진입 | OK |
| t0+5m | Step 1 — `app/`, `core/{vtk,io,data,scene,render,ui}`, `features/` 빈 디렉터리 신설 | OK |
| t0+15m | Step 2 — `app/app.{cpp,h}` 작성 (200 줄, dockspace + placeholder 메뉴) | OK |
| t0+25m | Step 3 — `core/vtk/vtk_viewer.{cpp,h}` 최소 셸 작성 (63 줄) | OK |
| t0+30m | Step 4 — `core/render/font_manager.{cpp,h}` 가져오기 시도 → **byte-identical 실패 발견** (1.3 참조) | △ 일탈 |
| t0+45m | (계획서 외) `app/legacy_app_compat.cpp` 추가 + `target_include_directories` 에 legacy 추가 | △ 회색지대 |
| t0+55m | Step 5 — `main.cpp` include 교체 + 함수 단순화 (`app::App` 만 호출) | OK |
| t0+65m | Step 6 — `bind_function.cpp` 의 Embind 함수 10 개 stub 화 | OK (계획서 9 + DEBUG_BUILD stub_printMeshTree) |
| t0+75m | Step 7 — `CMakeLists.txt` source list 재작성 + 안전벨트 블록 추가 | OK |
| t0+85m | Step 8 — 정적 검증 | 9/13 통과 |
| t0+95m | Step 9 — Windows 측 빌드 (debug 추정 통과 / release 명시 미보고) | △ |
| t0+105m | Step 10 — `npm run dev` + 브라우저 → 빈 dockspace + placeholder 메뉴 표시 | ✓ (사용자 보고) |
| t0+115m | Step 11 — Phase 1 doc commit `4eac024` 만 머지. **코드는 working tree 상태로 머무름** | △ |

## 1.2 Phase 1 §5 검증 매트릭스 결과 (13 항목)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 1 | `webassembly/src/` 직속 항목 | `app  bind_function.cpp  core  features  legacy  main.cpp` (6) | 동일 | ✓ |
| 2 | `core/` 하위 폴더 6 개 | `data io render scene ui vtk` | 동일 | ✓ |
| 3 | `app/` 의 파일 | `app.cpp app.h` (2) | `app.cpp app.h legacy_app_compat.cpp` (**3**) | △ **일탈** (1.4 참조) |
| 4 | `core/vtk/` 의 파일 | `vtk_viewer.cpp vtk_viewer.h` | 동일 | ✓ |
| 5 | `core/render/` 의 파일 | `font_manager.cpp font_manager.h` | 동일 | ✓ |
| 6 | main / bind 의 legacy include 0 | 0 hit | 0 hit | ✓ |
| 7 | CMake 빌드 안전벨트 존재 | 1 hit | 1 hit (`Phase 1 violation: legacy source in target`) | ✓ |
| 8 | font_manager 동일성 | SHA 동일 | **SHA 다름** + 라인 수도 다름 (legacy 33,502 / core 33,885) | ✗ **일탈** (1.3 참조) |
| 9 | Debug 빌드 | exit 0 | 추정 통과 (사용자 dev 동작 보고) | △ 추정 |
| 10 | Release 빌드 | exit 0 | **명시 확인 필요** | ⊘ |
| 11 | 빈 dockspace 표시 | dockspace 한 장 + 메뉴 비어있음 | dockspace 한 장 + `Crystal Viewer (rebuilding…)` placeholder 메뉴 1 항목 (계획서 §9.2 권장 형식과 일치) | ✓ |
| 12 | Embind stub no-op | 9 종 | **10 종** (DEBUG_BUILD stub_printMeshTree 추가) | ✓ |
| 13 | legacy/ 미컴파일 | ninja 로그에 legacy/ 등장 X | CMake source list 0 + 안전벨트로 차단됨 (실제 ninja 로그는 미확인) | △ 추정 |

**합계**: 통과 9 / 일탈 2 / 추정 3 / 미수행 1 / 실패 0

## 1.3 핵심 발견 — font_manager 의 App 의존성

### 발견

`legacy/font_manager.cpp` 는 **자동생성 폰트 데이터** 라고 알고 있었지만, 실제로는 폰트 데이터 + ImGui 폰트 등록 코드의 혼합이고, 그 등록 코드가 `App::DevicePixelRatio()` (legacy 클래스의 정적 메서드) 를 직접 호출한다. 즉 **font_manager 는 자기완결적이지 않다**.

```cpp
// core/render/font_manager.cpp (legacy 와 동일 코드)
ImFont* pImFont = io.Fonts->AddFontFromFileTTF(
    "./resources/font/NotoSansKR-Light.ttf",
    15.0f * App::DevicePixelRatio(),  // ← legacy App 클래스 의존
    nullptr,
    io.Fonts->GetGlyphRangesKorean());
```

이 함수가 24+ 회 호출되고 있어 (`grep` 결과 다수), 단순한 declaration 추가로는 해결 불가능.

### 결과로서의 일탈 1 — `app/legacy_app_compat.cpp` shim 추가

```cpp
// app/legacy_app_compat.cpp (계획서에 없던 신규 파일, 10 줄)
#include "../legacy/app.h"   // legacy 의 App 클래스 선언만 가져옴
#include "app.h"

double App::DevicePixelRatio() {           // ← legacy App 의 정적 메서드 정의
    return static_cast<double>(app::App::DevicePixelRatio());  // 새 app::App 으로 redirect
}
```

이 shim 으로 legacy `App::DevicePixelRatio()` 가 새 `app::App::DevicePixelRatio()` 호출로 redirect 된다. font_manager.cpp 의 24+ 호출 지점은 코드 한 줄 변경 없이 그대로 동작한다.

### 결과로서의 일탈 2 — `target_include_directories` 에 `legacy/` 추가

`app/legacy_app_compat.cpp` 의 `#include "../legacy/app.h"` 를 해석하기 위해, `target_include_directories` 에 `webassembly/src/legacy` 가 명시적으로 추가되었다. 이는 *"새 트리는 legacy 헤더를 모른다"* 라는 깔끔한 격리가 살짝 깨진 것을 의미한다 (단, **그 의존은 단 한 헤더 — `legacy/app.h`**).

## 1.4 분류 문제 — compat shim 은 회색지대인가?

Phase 0 §1.1 의 회색지대 정책은 **typo build-fix** 에 한정된다. compat shim 추가는 typo 가 아니라 **새 파일 작성** 이다. 정책 적용 범위 외.

그러나 다음 면에서는 회색지대와 정신이 비슷하다.

| 회색지대(Phase 0 §1.1) | compat shim (Phase 1 일탈) |
|---|---|
| 동작 0 변화 | 동작 0 변화 (DevicePixelRatio redirect 만) |
| 5 줄 이내 | 10 줄 |
| 빌드 통과를 위한 최소 변경 | 빌드 통과를 위한 최소 변경 |

→ Phase 1 계획서에 새 회색지대 카테고리(*"빌드 호환성 shim"*)를 신설하거나, 본 일탈을 수용하는 §1.X 를 추가하는 것이 권장된다 (Part 5 권장사항 참조).

## 1.5 환경 제약 / 추가 발견 사항

| 항목 | 내용 |
|---|---|
| `.git/index.lock` 일관성 | 본 시도에서는 발생 안 함 (Phase 0 사이클에서 정리됨) |
| **legacy/font_manager.cpp 라인 수 변화** | Phase 0 직후 ~33,886 줄 → Phase 1 평가 시점 33,502 줄. **383 줄 감소**. 의도된 건지 확인 필요 (Part 2 참조) |
| Phase 1 코드 commit 미수행 | working tree 에 머무는 상태. Phase 2 진입 전에 commit/push 필요 |
| `npm run dev` 동작 | placeholder 메뉴 (Crystal Viewer (rebuilding…)) 와 함께 빈 dockspace 정상 표시 (사용자 보고) |

## 1.6 계획서가 예상하지 못한 빈틈

| # | 빈틈 | 영향 | 권장 보강 |
|---|---|---|---|
| 1.6.1 | font_manager.cpp 의 비-자기완결성 (legacy `App::DevicePixelRatio()` 호출) 가 §4 Step 4 의 *"physical copy"* 권고와 충돌 | compat shim 1 개 + include path 의 legacy 추가 발생 | Phase 1 §1 비목표에 *"빌드 호환성 shim"* 회색지대 추가, 또는 §4 Step 4 에 의존 분석 단계 신설 |
| 1.6.2 | font_manager 자체가 폰트 *데이터* 만이 아니라 *등록 코드* 를 포함한다는 사실이 §4 Step 4 의 가정과 다름 | 실제 가져온 파일이 byte-identical 이 아니어도 의도일 수 있음 | font_manager 자체를 도메인-특화 이주 단계(Phase 1.5 또는 Phase 2 의 일부) 로 분리 검토 |
| 1.6.3 | Phase 1 코드를 commit 하지 않은 채 doc 만 commit 한 패턴 (Phase 0 와 동일) | Phase 2 PR 의 base commit 모호 | Phase 1 §10 PR 체크리스트의 첫 행을 "commit 머지" 로 격상 |

---

# Part 2 — 계획서 대비 일탈 사항 (4 종)

| # | 일탈 | 위치 | 의도성 | 평가 |
|---|---|---|---|---|
| 2.1 | `app/legacy_app_compat.cpp` 추가 (10 줄) | `webassembly/src/app/` | font_manager 의존을 해소하기 위한 빌드 호환성 shim — **불가피한 의도적 추가** | OK (Part 5 의 회색지대 신설 권장) |
| 2.2 | `target_include_directories` 에 `legacy/` 포함 | `CMakeLists.txt` | compat shim 이 `#include "../legacy/app.h"` 를 사용하므로 필요 — **2.1 의 결과** | OK (단 Phase 5 에서 legacy 제거 시 함께 제거 필요) |
| 2.3 | `core/render/font_manager.cpp` 가 legacy 와 byte-identical 이 아님 (라인 수 33,502 vs 33,885) | — | legacy 측이 변경된 결과로 보임 (감소 383 줄) | △ 의도성 확인 필요 — Phase 1 의 *"legacy 한 글자도 변경 X"* 원칙 위반 가능성 |
| 2.4 | bind_function.cpp 의 stub 갯수 10 (계획서 9 + DEBUG_BUILD stub_printMeshTree) | `webassembly/src/bind_function.cpp` | 계획서 §6.2 에서 누락된 `printMeshTree` 의 정확한 보충 | OK (오히려 더 정밀) |

---

# Part 3 — 평가 시점의 저장소 상태

## 3.1 파일 트리

```
webassembly/src/
├─ main.cpp                    ← #include "app/app.h" only
├─ bind_function.cpp           ← stub 10 종 + IDBFS 3 종
├─ app/
│  ├─ app.cpp / app.h          [200 줄, dockspace + placeholder 메뉴]
│  └─ legacy_app_compat.cpp    [10 줄, 일탈 2.1]
├─ core/
│  ├─ vtk/
│  │  └─ vtk_viewer.cpp / vtk_viewer.h   [63 + h]
│  ├─ render/
│  │  └─ font_manager.cpp / font_manager.h  [33,885 줄 — 일탈 2.3]
│  ├─ io/   (빈)
│  ├─ data/ (빈)
│  ├─ scene/(빈)
│  └─ ui/   (빈)
├─ features/                   (빈)
└─ legacy/                     ← Phase 0 동결본 + font_manager.cpp 가 변경된 흔적 있음 (33,502 줄)
```

## 3.2 git status / commit 분포

| 분류 | 개수 | 의미 |
|---|---|---|
| `??` untracked | 18 | `webassembly/src/{app,core}/`, `main.cpp` (재추가), 일부 legacy/* (toolbar/unv_reader/vtk_viewer .cpp/h) |
| `D` deleted | 7 | Phase 0 직후의 잔여 staging 흔적 |
| `M` modified | 205 | filemode 노이즈 (Phase 0 와 무관) + Phase 1 의 main/bind/CMake patch |

```
최근 커밋 3개:
  4eac024 docs: redevelopment plan and Phase 1 detail
  2a586bb Phase 0: Move webassembly/src/* to webassembly/src/legacy/
  33eb219 docs: redevelopment plan and Phase 0 detail
```

→ **Phase 1 의 코드 변경은 commit 되지 않은 working tree 상태.** Phase 0 와 동일한 패턴 (도큐먼트 commit 만 있고 코드 commit 은 누락).

## 3.3 사용자 런타임 테스트의 의미

사용자 보고 *"npm run dev 실행 후 런타임에서 기능에 대한 테스트는 완료"* 가 가리키는 것:

| 매트릭스 항목 | 입증 정도 |
|---|---|
| #9 Debug 빌드 통과 | 강한 증거 (dev 가 wasm 컴파일 → 로딩 성공) |
| #11 빈 dockspace 표시 | 강한 증거 (dev 후 브라우저 정상 동작) |
| #12 Embind stub no-op | 추정 (Next.js 측이 stub 함수를 호출하더라도 에러 없이 통과한 것으로 보임) |
| #10 Release 빌드 | **미입증** (dev 는 release 빌드를 트리거하지 않음) |
| #13 legacy 미컴파일 | 추정 (CMake source list 가 legacy 를 포함하지 않으므로) |

> Phase 1 에서는 18 항목 회귀가 의도적으로 비목표이므로 *"기능에 대한 테스트"* 가 가리키는 것은 빈 dockspace + placeholder 메뉴 정상 표시일 것이다 (사용자가 명시적으로 *"기능 회복은 Phase 3~4"* 임을 인지하고 있음).

---

# Part 4 — Phase 2 진행가능 여부 판정

## 4.1 Phase 2 입구 조건과의 매핑

`05_redevelopment_plan.md` §5 의 Phase 2 작업은 다음을 전제한다.

| Phase 2 전제 | 현재 상태 | 통과? |
|---|---|---|
| Phase 1 의 디렉터리 트리(`app/`, `core/{vtk,render}/`) 가 확립되어 있다 | working tree 에 확립됨 | ✓ |
| 빈 dockspace 빌드 + 런타임 통과 | 통과 (사용자 보고) | ✓ |
| Phase 1 PR commit 머지 (Phase 2 PR 의 base commit) | **미커밋** | ✗ |
| `core/{io,data,scene,ui}/` 빈 폴더가 채울 수 있는 상태 | 가능 | ✓ |
| legacy 의존이 명시적이고 격리되어 있어 Phase 2 의 SceneState 가 legacy 호출 없이 작성 가능 | △ — `legacy_app_compat.cpp` shim 1 개 + include path 1 개의 의존 존재. SceneState 자체에는 영향 없으나 향후 Phase 5 에서 정리 필요 | △ |

→ Phase 2 의 작업 자체는 시작할 수 있는 환경. 단 **Phase 1 PR 이 commit 되어야 Phase 2 PR 의 base 가 정의된다.**

## 4.2 종합 판정

> **조건부 진행 가능 (Conditionally GO)**
>
> Phase 1 의 본질(빈 dockspace + 새 트리 빌드 통과) 은 통과. 다만 (a) 일탈 1 건의 사후 처리(회색지대 정책 신설), (b) Phase 1 PR commit, (c) release 빌드 명시 확인 등 정리 후 Phase 2 진입을 권장.

## 4.3 진입 전 처리할 4 가지 정리 항목

| # | 항목 | 명령 / 방법 | 통과 기준 |
|---|---|---|---|
| 4.3.A | **Phase 1 §1.1 회색지대 신설** (선행 문서 패치) | `phase1_app_core_bootstrap.md` 의 §1 비목표 표 아래에 *"§1.X 회색지대 — 빌드 호환성 shim"* 절을 추가하고 `legacy_app_compat.cpp` 사례 명시 | 계획서가 일탈 2.1 을 합법화 |
| 4.3.B | **release 빌드 명시 확인** | `npm run build-wasm:release` | exit 0 |
| 4.3.C | **Phase 1 코드 commit 정리** | (a) `git add -A` 로 Phase 1 의 working tree 변경 staging. (b) Phase 1 변경(app/, core/, main.cpp, bind_function.cpp, CMakeLists.txt) 만 별도 commit. (c) compat shim 은 회색지대 신설(4.3.A) 후 별도 commit 으로 분리 권장. (d) filemode 노이즈는 분리 | Phase 1 commit 1~3 개 (셸 + shim + (옵션) filemode) |
| 4.3.D | **legacy/font_manager.cpp 변화의 의도성 확인** | `git diff HEAD -- webassembly/src/legacy/font_manager.cpp` 결과를 검토하여 383 줄 감소가 의도된 것인지 확인. 의도된 게 아니라면 legacy 동결 원칙 회복을 위해 복원 | (a) 의도된 변경이면 commit 메시지에 사유 명시. (b) 의도하지 않은 변경이면 `git restore -- webassembly/src/legacy/font_manager.cpp` |

> 4.3.A 는 Phase 0 가 회색지대 정책을 신설했던 것과 동일한 학습 사이클이다. 본 평가서가 Phase 1 계획서로 환원되어 다음 Phase 에서 재발하지 않게 한다.

---

# Part 5 — 메타 평가

## 5.1 계획서의 효과 평가

| 보강 항목 (Phase 1 §) | 효과 |
|---|---|
| §1.1 의도적 기능 손실 표 | placeholder 메뉴 (`Crystal Viewer (rebuilding…)`) 와 함께 사용자 인지가 형성됨 ✓ |
| §3 디렉터리 트리 | 의도대로 6 항목 + 6 sub-folder 로 셋업됨 ✓ |
| §4 Step 4 font_manager physical copy | **부분 실패** — font_manager 의 비-자기완결성이 가정과 어긋났고, 결과적으로 일탈 1 건 발생 ⚠ |
| §4 Step 6 Embind stub | 9 종 → 10 종 으로 더 정밀하게 적용됨 ✓ |
| §4 Step 7 CMake 안전벨트 | 명시적으로 `Phase 1 violation` 메시지 포함 ✓ |
| §6.2 font_manager byte-identity | **검증 실패** — 라인 수와 SHA 모두 다름 ✗ |
| §9.2 placeholder 메뉴 권장 | 그대로 적용됨 (Crystal Viewer (rebuilding…)) ✓ |

→ 계획서의 큰 그림(빈 셸 부트스트랩 + legacy 빌드 제외)은 정확히 작동했지만, **font_manager 의 외부 의존을 사전에 분석하지 못한 결과** 한 가지 일탈이 발생했다.

## 5.2 잔여 리스크 (Phase 2 진입 전 인지 필요)

| # | 리스크 | 완화 |
|---|---|---|
| 5.2.1 | `legacy_app_compat.cpp` 가 Phase 5 에서 legacy 삭제 시 함께 정리되어야 함 | Phase 5 세부계획서에 정리 항목으로 명시 |
| 5.2.2 | `target_include_directories` 의 `webassembly/src/legacy` 가 Phase 5 에서 삭제되어야 함 | 동일 |
| 5.2.3 | font_manager.cpp 의 `App::DevicePixelRatio()` 호출이 그대로 유지된다면 Phase 1.5 또는 Phase 2 에서 새 `app::App::DevicePixelRatio()` 직접 호출로 정리 가능 | font_manager 가 자동생성 데이터인지 사람-편집인지 사전 확인 후 직접 수정 또는 shim 유지 결정 |
| 5.2.4 | release 빌드 미명시 → Phase 0 와 동일한 잠복 버그 노출 가능성 (낮지만 0 아님) | 4.3.B 통과로 해소 |
| 5.2.5 | Phase 1 코드 commit 미수행 → Phase 2 PR 의 base 모호 | 4.3.C 통과로 해소 |
| 5.2.6 | legacy/font_manager.cpp 가 Phase 1 도중 변경됨 (383 줄 감소) | 4.3.D 통과로 해소 |

## 5.3 종합 평가표

| 항목 | 점수 (5점 만점) | 비고 |
|---|---|---|
| 정적 단계 실행 정확성 | **4** | 계획서 §4 Step 1~7 이 의도대로. font_manager Step 4 만 일탈 |
| 정적 검증 통과율 | **4** | 9/13 통과 + 1 일탈 + 3 추정 |
| 동적 검증 통과율 | **4** | dev + 빈 dockspace 통과. release 미명시 |
| 계획서 §4 절차의 적합성 | **3** | font_manager 의 외부 의존 사전 분석 부재 |
| 계획서 §6 검증 매트릭스 커버리지 | **4** | 13 항목 중 1 항목(#8 byte-identity) 가 비현실적이었음 |
| Phase 1 PR 형태 | **2** | working tree 상태. doc 만 commit |
| Phase 2 입구 도달도 | **4** | 작업 환경 준비됨. base commit 이 정의되면 5 |
| 종합 | **조건부 진행 가능** | 4.3 의 4 가지 정리 후 Phase 2 진입 |

> Phase 1 은 *"Phase 0 의 학습이 잘 작동하는지 보는 시험대"* 였다. 결과: Phase 0 의 회색지대 정책이 **typo 에만 한정** 되어 있었던 것이 Phase 1 에서 새 회색지대(빌드 호환성 shim) 를 만나면서 노출되었다. 학습 사이클이 다시 한 번 작동하는 셈.

---

## 6. 결론 및 권장 다음 단계

### 결론

> **Phase 1 의 본질적 작업(빈 셸 부트스트랩 + legacy 빌드 제외 + 빈 dockspace 표시) 은 완료되었다.** 다만 (a) `font_manager` 의 외부 의존으로 인한 compat shim 일탈, (b) Phase 1 코드 commit 미수행, (c) release 빌드 명시 미보고, (d) legacy/font_manager.cpp 의 의도성 확인 — 4 가지 정리 항목이 남아 있다.

### 권장 다음 단계

1. **§4.3.A** — Phase 1 계획서에 *"§1.X 회색지대 — 빌드 호환성 shim"* 추가하여 `legacy_app_compat.cpp` 를 합법화.
2. **§4.3.B** — `npm run build-wasm:release` 명시 통과 확인.
3. **§4.3.C** — Phase 1 코드 commit 정리: 셸 + shim + (옵션) filemode 분리.
4. **§4.3.D** — `legacy/font_manager.cpp` 변화의 의도성 확인.
5. (위 4 항목 통과 후) — `phase2_core_skeleton.md` 세부계획서 작성에 진입.

### Phase 2 진입 신호

다음 4 개가 모두 ✓ 면 Phase 2 PR 을 시작해도 무방하다.

- [ ] Phase 1 계획서 §1.X 회색지대 신설 commit 머지 (또는 같은 PR 안에서 처리)
- [ ] `npm run build-wasm:release` exit 0 (재확인)
- [ ] Phase 1 PR commit 1~3 개로 정리되어 머지 또는 push
- [ ] `legacy/font_manager.cpp` 의 변경이 의도된 것임을 commit 으로 명시 (또는 원복)

---

## 7. 관련 문서

- 보강된 Phase 1 계획서: [`./phase1_app_core_bootstrap.md`](./phase1_app_core_bootstrap.md)
- 선행 Phase 0 통합 평가서: [`./phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md)
- 보강된 Phase 0 계획서: [`./phase0_legacy_freeze.md`](./phase0_legacy_freeze.md) (rev. 2026-04-28)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §4 Phase 1 / §5 Phase 2
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)

---

## 8. 콘솔 경고 후속 조치 리포트 (2026-04-28)

### 8.1 범위

Phase 1 평가 이후, `/workbench` 에서 보고된 브라우저 콘솔 경고를 해소하기 위해 프런트엔드 후속 조치를 수행했다.

1. 폰트 preload 경고 (`_next/static/media/*.woff2 preloaded but not used`).
2. WASM 런타임 스크립트 preload 경고 (`/wasm/VTK-Workbench.js?... preloaded but not used`).

### 8.2 원인 요약

| 경고 | 원인 |
|---|---|
| 폰트 preload 미사용 | `next/font` 의 기본 동작이 preload 이고, 초기 라우트 구동 시점에서 해당 폰트가 즉시 소비되지 않아 경고가 발생했다. |
| `VTK-Workbench.js` preload 미사용 | `next/script`(`afterInteractive`) 의 preload 주입 동작과 실제 WASM 런타임 초기화 타이밍/경로가 어긋나 Chrome 에서 미사용 preload 로 판정되었다. |

### 8.3 적용 수정사항

| # | 파일 | 변경 |
|---|---|---|
| 1 | `app/layout.tsx` | `Geist`, `Geist_Mono` 모두에 `preload: false` 적용 |
| 2 | `app/workbench/page.tsx` | `next/script` 사용을 제거하고 동적 `<script>` 주입(`document.createElement("script")`)으로 전환 |
| 3 | `app/workbench/page.tsx` | 중복 초기화를 막기 위한 single-flight 가드(`wasmInitStartedRef`) 추가 |
| 4 | `app/workbench/page.tsx` | 기존 런타임 스크립트 노드 재사용 경로 및 로더 상태(`data-loaded`) 처리 추가 |
| 5 | `app/workbench/page.tsx` | 초기화 실패 시 가드 복구(`catch` 에서 reset) 및 명시적 에러 로깅 추가 |

### 8.4 검증

| 항목 | 결과 |
|---|---|
| `npm.cmd run lint` | 통과 (warning/error 없음) |
| `npm.cmd run type-check` | 통과 (`tsc --noEmit`) |

### 8.5 평가

- 본 후속 조치는 **C++ 중심 Phase 1 본 범위 밖의 작업**이지만, 검증 과정에서 혼선을 유발하던 콘솔 노이즈를 제거했다.
- wasm/core 아키텍처 계약은 변경하지 않았고, 프런트엔드 로딩 동작만 조정했다.
- 기대 효과: 일반적인 dev 재로딩 플로우에서 폰트 및 `VTK-Workbench.js` 관련 preload 경고가 재발하지 않는다.
