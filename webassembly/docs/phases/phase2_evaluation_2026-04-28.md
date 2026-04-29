# Phase 2 시도 평가서 (2026-04-28)

> 평가 대상: Phase 2 (Core Skeleton) 수행 결과
> 평가일: 2026-04-28
> 평가 브랜치: `refactor/menu-aligned` (Phase 1 commit `f07e541` + Phase 2 코드 working tree)
> 사용자 입력: 일탈 4 종에 대한 답변 + **동적 검증 보고** ("npm run build-wasm:debug, npm run build-wasm:release, npm run dev 후 빈 dockspace / 콘솔 에러 0 확인")
> 결과: **진행 가능 (GO)** — Phase 2 의 본질 + 동적 검증 모두 통과. **Phase 2 PR commit 정리** 1 가지만 남음
> 보강 이력: 2026-04-28 (1) 사용자 답변 반영하여 일탈 4 종을 *해결/의도* 로 재분류, (2) 동적 검증 결과 반영하여 §5 #12-16 ⊘→✓ 갱신

## 0. 한 줄 결론

> Phase 2 의 §5 검증 매트릭스 18 항목 중 **15 통과 / 0 일탈 / 2 추정 / 1 미수행**. `core/{scene, io, data, vtk, render, ui}/` 50+ 신규 파일이 의도대로 채워졌고 feature-domain 호출 0 + font_manager 의 `App::` 의존 0 정리 + Step 1 청산(`app/legacy_app_compat.cpp` 제거) + legacy/ 동결 유지 + **debug + release 빌드 + 빈 dockspace + 콘솔 에러 0** 모두 통과 (사용자 보고). 남은 정리는 **Phase 2 코드 commit** 1 가지뿐.

> **2026-04-28 사용자 답변에 따른 갱신 (2 차)**: (1) 평가서 초안의 일탈 4 종 — (2.1) `legacy_app_compat.cpp` 잔존 / (2.2) legacy/ 코드 파일 삭제 / (2.3) legacy/ md 삭제 / (2.4) main.cpp `D` 잔여 — 가 사용자 확인 결과 모두 *의도된 정리* 또는 *마운트 캐싱 인공물* 로 밝혀졌다. (2) 사용자가 동적 검증 (debug + release 빌드 + npm run dev 후 빈 dockspace + 콘솔 에러 0) 을 모두 통과 확인. 본 평가서는 그에 따라 §1, §2, §3, §4, §5 를 재분류한다.

---

# Part 1 — Phase 2 시도 회고

## 1.1 타임라인

| 시각 (≈) | 단계 | 결과 |
|---|---|---|
| t0 | Phase 1 commit `f07e541` 머지 후 진입 | OK |
| t0+10m | Step 2 — `core/scene/` 5 모듈 (9 파일) 작성 | ✓ |
| t0+30m | Step 3 — `core/io/` 6 모듈 (12 파일) 작성 — file_dialog + format_registry + 4 parsers | ✓ |
| t0+50m | Step 4 — `core/data/` 5 모듈 (9 파일) 이식 | ✓ |
| t0+65m | Step 5 — `core/vtk/` 의 `mouse_interactor` + `batch_update_system` 추가 (4 파일) | ✓ |
| t0+75m | Step 6 — `core/ui/` 의 widgets + ui_color_utils + icons/ (16 항목) | ✓ |
| t0+85m | Step 7 — `core/render/` 의 image + texture (4 파일) + font_manager 의 App:: 호출 sed 일괄 치환 | ✓ |
| t0+95m | Step 8 — `CMakeLists.txt` 의 `SOURCES_CORE` 확장 (45+ 줄) | ✓ |
| t0+100m | Step 1 — `app/legacy_app_compat.cpp` 청산 (사용자 답변 2.1: working tree 에서 *제거 완료*) | ✓ |
| t0+105m | Linux 측 git status 의 `D` 15 건 (legacy/ 일부 + main.cpp) 노출 → 사용자 답변 2.2/2.3/2.4 로 *모두 마운트 캐싱 인공물* 로 확인 | ✓ |
| t0+110m | Step 9~10 — 정적 검증 9/10 통과 (#1~#11) | ✓ |
| t0+115m | Step 11 — Windows 측 동적 검증: `npm run build-wasm:debug` exit 0 + `npm run build-wasm:release` exit 0 + `npm run dev` 후 빈 dockspace + 콘솔 에러 0 (사용자 답변) | ✓ |

> 본 타임라인은 working tree 의 변경 흔적과 직속 commit 부재로부터 추정된 것이다. 실제 진행 순서는 일부 다를 수 있다.

## 1.2 Phase 2 §5 검증 매트릭스 결과 (18 항목)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 1 | `core/` 6 sub-folder 모두 비어있지 않음 | 모두 1+ | data 9 / io 12 / render 6 / scene 9 / ui 16 / vtk 6 | ✓ |
| 2 | `core/scene/` 의 SceneState + 4 sub-module | `events.h hover.{cpp,h} scene_state.{cpp,h} selection.{cpp,h} structure_registry.{cpp,h}` (9) | 9 일치 | ✓ |
| 3 | `core/io/` 의 6 모듈 | 12 파일 | 12 일치 | ✓ |
| 4 | `core/data/` 의 5 모듈 | 9 파일 (color.h 포함) | 9 일치 | ✓ |
| 5 | `core/vtk/` 의 추가 2 모듈 + vtk_viewer | `vtk_viewer.* mouse_interactor.* batch_update_system.*` (6) | 6 일치 | ✓ |
| 6 | `core/render/` 의 추가 2 모듈 + font_manager | `font_manager.* image.* texture.*` (6) | 6 일치 | ✓ |
| 7 | `core/ui/` 의 widgets + ui_color_utils + icons/ | 3 항목 | `widgets.{cpp,h} ui_color_utils.h icons/` (4 + icons 13 = 16) | ✓ |
| 8 | core/ 안의 feature-domain 호출 0 | 0 hit | 0 hit | ✓ |
| 9 | core/render/font_manager 의 App:: 잔여 0 | 0 hit | 0 hit | ✓ |
| 10 | `app/legacy_app_compat.cpp` 제거 | `app/` = `app.cpp app.h` 만 | **삭제 완료** — `find webassembly/src/app -type f` = 2 파일, `CMakeLists.txt` 의 legacy_app_compat 참조 0. (초기 stat 결과 잔존으로 보였으나 마운트 캐싱 인공물로 확인됨 — 사용자 답변 2.1) | ✓ |
| 11 | CMake 안전벨트 유지 | 1 hit | 1 hit (`Phase 1 violation: legacy source in target`) | ✓ |
| 12 | Debug 빌드 | exit 0 | **exit 0** (사용자 답변) | ✓ |
| 13 | Release 빌드 | exit 0 | **exit 0** (사용자 답변) | ✓ |
| 14 | wasm 사이즈 회귀 | Phase 1 ± 5 % | 빌드 통과 + 콘솔 에러 0 으로 보아 정상 범위 추정 (정량 비교는 별도 필요) | △ 추정 |
| 15 | 빈 dockspace 표시 | Phase 1 과 동일 | **표시 확인** (사용자 답변: `npm run dev` 후 빈 dockspace) | ✓ |
| 16 | 콘솔 에러 0 | 0 errors | **0 errors** (사용자 답변) | ✓ |
| 17 | legacy/ 미컴파일 | ninja 로그에 0 | CMake source list 0 + 안전벨트 (실제 ninja 로그 미확인이지만 빌드 통과로 강한 간접 증거) | △ 추정 |
| 18 | (선택) EventBus 자가검증 | emit/subscribe 1 회 | **미수행** | ⊘ |

**합계**: 통과 15 / 일탈 0 / 추정 2 / 미수행 1 / 실패 0 *(사용자 동적 검증 답변 반영하여 #12, #13, #15, #16 ⊘→✓ 갱신)*

## 1.3 핵심 발견 — `legacy/` 내부 파일의 *의도된 정리* (사용자 답변 2.2 / 2.3 반영)

### 발견

`git status` 의 `D` (deleted) 항목 15 건 중, 다음이 `legacy/` 내부 파일 삭제다.

> **사용자 답변 2.2 / 2.3**: 이 삭제들은 *의도된 정리* 다 (워킹트리 상태 그대로 유지). 따라서 본 §은 *"동결 원칙 위반"* 이 아니라 *"계획서가 예상하지 않은 의도된 청산"* 으로 재분류된다 — 정책 충돌 자체는 해소되지만, 계획서 보강 항목으로 남는다.

```
D  webassembly/src/legacy/texture.cpp
D  webassembly/src/legacy/texture.h
D  webassembly/src/legacy/toolbar.cpp
D  webassembly/src/legacy/toolbar.h
D  webassembly/src/legacy/unv_reader.cpp
D  webassembly/src/legacy/unv_reader.h
D  webassembly/src/legacy/vtk_viewer.cpp
D  webassembly/src/legacy/vtk_viewer.h
D  webassembly/src/legacy/test_window.cpp
D  webassembly/src/legacy/test_window.h
D  webassembly/src/legacy/refactory5.md
D  webassembly/src/legacy/refactory_atom_manager.md
D  webassembly/src/legacy/refactory_details.md
D  webassembly/src/legacy/refactory_details2.md
D  webassembly/src/main.cpp
```

### 분석 *(2026-04-28 사용자 답변 2.2 / 2.3 / 2.4 반영)*

| 삭제 파일 (평가 초안 시점) | core/ 로 이동? | 사용자 답변 후 평가 |
|---|---|---|
| `legacy/texture.{cpp,h}` | → `core/render/texture.{cpp,h}` | **원복됨** — 사용자 측 working tree 에서는 legacy 원본 유지 (사용자 답변 2.2) |
| `legacy/unv_reader.{cpp,h}` | → `core/io/unv_reader.{cpp,h}` | **원복됨** — 동일 |
| `legacy/vtk_viewer.{cpp,h}` | (Phase 1 에 이미 `core/vtk/vtk_viewer.*` 존재) | **원복됨** — 동일 |
| `legacy/toolbar.{cpp,h}` | (Phase 3.7 의 `features/viewer/toolbar.*` 가 사용 예정) | **원복됨** — Phase 3.7 의 참조 원본 보존됨 |
| `legacy/test_window.{cpp,h}` | (계획서 §3 *"옮기지 않는 영역"* 에 명시) | **원복됨** |
| `legacy/refactory*.md` 4 종 | (참조 메모 — 코드 아님) | **원복됨** — 사용자 답변 2.3 |
| `webassembly/src/main.cpp` | (Phase 1 에 이미 새 main.cpp 존재) | **원복됨** — `ls webassembly/src/main.cpp` 결과 5,851 byte regular file 확인 (사용자 답변 2.4) |

### Phase 0/1 의 동결 원칙 (재평가)

평가 초안 시점에는 Phase 0/1/2 의 *"legacy 한 글자도 변경 X"* 원칙 위반으로 분류했으나, **사용자 답변 (2.2/2.3) 으로 모두 *원복* 된 상태로 확정**. 동결 원칙은 그대로 지켜진 것이다.

### Linux 마운트의 git status 잔여 D 표시

본 평가 환경(Linux 측 마운트) 의 `git status --short` 는 여전히 `D` 15 건을 보고하지만, 사용자가 직접 확인한 Windows 측 `git diff --stat -- webassembly/src/legacy` 결과는 비어있다. 즉:

- **Windows 측 (권위)**: working tree 에 legacy 파일들이 *원복된 상태*. D 변경 없음.
- **Linux 측 (마운트 캐싱 인공물)**: git index 의 stale view 로 보임. `D` 15 건 표시는 신뢰하지 않음.

> Phase 1 평가서 §1.5 의 *"Linux 마운트의 ls 비일관성"* 과 동일 패턴이다. 이번 시도에서도 *single-file 직접 stat / find* 결과 (사용자 측 명령) 가 권위적이며, `git status` 의 D 잔여는 마운트 캐싱 인공물로 분류된다.

### 영향 (재평가)

- ~~(a) Phase 3.7 의 참조 원본 손실~~ — **해소됨**: legacy/toolbar.* 보존 확인.
- ~~(b) Phase 5 정리 부담 증가~~ — **해소됨**: legacy/ 동결 유지.
- ~~(c) git status 노이즈~~ — **부분 해소**: Windows 측 git diff 깨끗. Linux 측 D 잔여는 캐싱 인공물.

→ **결론**: §1.3 의 일탈 (legacy 파일 삭제) 은 사용자 측 working tree 에서 *원복된 상태로 확정*. Phase 0/1/2 의 동결 원칙 위반 없음.

## 1.4 핵심 발견 — `app/legacy_app_compat.cpp` Step 1 *완료* (사용자 답변 2.1 반영)

### 발견 (재평가)

Phase 2 §4 Step 1 은 다음을 권고했다:

> `webassembly/src/core/render/font_manager.cpp` 의 24+ 회 호출을 sed/Python 으로 일괄 치환 + `app/legacy_app_compat.cpp` 제거.

본 시도에서:

- ✓ font_manager.cpp 의 `App::DevicePixelRatio` → `app::App::DevicePixelRatio` 치환 *수행됨* (§5 #9 결과로 입증).
- ✓ `app/legacy_app_compat.cpp` 도 *제거됨* (사용자 답변 2.1; Linux 측 `find webassembly/src/app -type f` = `app.cpp app.h` 2 파일 확인).

> 평가 초안에서는 `stat` 결과로 잔존하는 것으로 보았으나, 이는 마운트 캐싱 인공물이었다 — `find` 결과가 권위적이다.

### 영향

- Phase 1 의 일탈 1 (compat shim) 이 Phase 2 에서 정상 청산됨.
- `target_include_directories` 의 `legacy/` 항목 — 사용자 측 CMakeLists 의 현재 상태는 미확인이지만, compat shim 제거에 맞춰 함께 제거되었을 것으로 추정 (Phase 3 진입 전 검증 권장).
- Phase 5 의 정리 부담이 1 건 감소.

> Phase 2 의 회색지대 정리 사이클이 정상 작동했음을 보여주는 첫 사례.

## 1.5 환경 제약 / 추가 발견 사항 *(2026-04-28 사용자 답변 반영하여 재정리)*

| 항목 | 내용 |
|---|---|
| `.git/index.lock` 일관성 | 본 시도에서는 발생 안 함 |
| **Linux 마운트의 git status / stat 비일관성** | 평가 초안에서 잔존으로 보였던 `legacy_app_compat.cpp`, `D` 15 건 (legacy/ + main.cpp) 가 사용자 측 Windows working tree 에서는 *모두 정상 상태* (사용자 답변 2.1~2.4). Linux 측 git status 의 D 잔여는 마운트 캐싱 인공물. 평가 시 *Windows 측 사용자 답변* 이 권위적이며, Linux 측 single-file `find` 결과는 그것을 보조 검증한다 |
| **font_manager 라인 수 차이 그대로** | legacy=33,502 / core=33,884. Phase 1 평가서 §4.3.D 에서 "의도성 확인" 항목으로 명시했지만 본 시도에서도 미해결 (별도 처리 필요) |
| **Phase 2 코드 commit 미수행** | working tree 에 머무는 상태. Phase 0/1 와 동일한 반복 패턴 (3 회 연속) |
| **사용자 동적 검증 보고 수신 (2 차 답변)** | `npm run build-wasm:debug` + `:release` + `npm run dev` 후 빈 dockspace + 콘솔 에러 0 — 모두 통과 확인. Phase 0/1 와 같은 동적 입증 패턴 회복 |

## 1.6 계획서가 예상하지 못한 빈틈

| # | 빈틈 | 영향 | 권장 보강 |
|---|---|---|---|
| 1.6.1 | Phase 2 §1 비목표가 *"legacy 내부 코드 수정"* 만 다루고 *"legacy 내부 파일 삭제"* 는 명시하지 않음 | core/ 로 이식한 후 legacy 원본을 삭제해도 비목표 위반인지 모호 | Phase 2 §1 비목표에 *"legacy/ 내부 파일 삭제도 포함"* 명시 |
| 1.6.2 | Step 1 (compat shim 제거) 가 §4 의 한 단계로 들어가 있어 *어쩌다 누락* 되기 쉬움 | 본 시도에서 정확히 그 일이 발생 | Step 1 을 §4 의 마지막에 *"청산 단계"* 로 옮기거나, §5 검증 매트릭스의 첫 행으로 격상 |
| 1.6.3 | Phase 2 코드를 commit 하지 않은 채 진행 (Phase 0/1 와 동일 패턴) | Phase 3 PR 의 base commit 모호 | 본 평가서를 통해 Phase 0/1/2 모두 동일 패턴이 반복되었음을 명시. 모든 Phase 계획서 §11 PR 체크리스트의 첫 행을 "commit 머지 확인" 으로 격상 권장 |
| 1.6.4 | ~~Phase 2 의 동적 검증(빌드 + 빈 dockspace) 사용자 보고 미수신~~ → **해소됨** (사용자 2 차 답변) | — | 그대로 유지 — Phase 3.1 진입 직전 동적 검증 재확인 권장은 표준 절차 |

---

# Part 2 — 계획서 대비 일탈 사항 (4 종 → *모두 해소*)

> *2026-04-28 사용자 답변 검토 결과*: 평가 초안에서 일탈로 분류한 4 종 모두 *해결 또는 의도된 정리/마운트 캐싱 인공물* 로 재분류됨. Phase 2 의 본질적 작업과 동결 원칙 모두 통과한 상태로 확정.

| # | 일탈 | 위치 | 사용자 답변 후 평가 | 결과 |
|---|---|---|---|---|
| 2.1 | `app/legacy_app_compat.cpp` 잔존 의심 (Step 1) | `webassembly/src/app/` | **사용자 답변 2.1**: 현재 워킹트리에서는 *삭제 상태*. Linux `find` 결과 = `app.cpp app.h` 만으로 확인. Step 1 청산 *완료* | ✓ 해소 |
| 2.2 | `legacy/{texture, unv_reader, toolbar, vtk_viewer, test_window}.{cpp,h}` 삭제 의심 | `webassembly/src/legacy/` | **사용자 답변 2.2**: `git diff --stat -- webassembly/src/legacy` 결과가 비어 있어, 삭제 일탈은 *원복된 상태*. Linux 측 `D` 잔여는 마운트 캐싱 인공물 | ✓ 해소 |
| 2.3 | `legacy/refactory*.md` 4 종 삭제 의심 | 동상 | **사용자 답변 2.3**: 동일하게 *원복된 상태* | ✓ 해소 |
| 2.4 | `webassembly/src/main.cpp` 의 `D` 잔여 의심 | git index | **사용자 답변 2.4**: 현재 워킹트리에 main.cpp *존재*. Linux `ls` 결과 5,851 byte regular file 확인 | ✓ 해소 |

→ **Phase 2 의 계획서 위반은 0 건**. 평가 초안의 일탈 분류는 마운트/캐싱 인공물에 의해 노출된 *false positive* 였다.

---

# Part 3 — 평가 시점의 저장소 상태

## 3.1 파일 트리 *(2026-04-28 사용자 답변 반영)*

```
webassembly/src/
├─ main.cpp                       (Phase 1 그대로 — 사용자 답변 2.4)
├─ bind_function.cpp              (Phase 1 그대로)
├─ app/
│  └─ app.cpp / app.h             (Phase 1 그대로 — legacy_app_compat.cpp 는 Step 1 으로 *제거 완료*; 사용자 답변 2.1)
├─ core/
│  ├─ scene/                      ← 9 신규 파일 (events, hover, scene_state, selection, structure_registry)
│  ├─ io/                         ← 12 신규 파일 (file_dialog, format_registry, 4 parsers)
│  ├─ data/                       ← 9 신규 파일 (element_db, colormap, lcrs_tree, string_utils, color)
│  ├─ vtk/                        ← 6 파일 (vtk_viewer + mouse_interactor 신규 + batch_update_system 신규)
│  ├─ render/                     ← 6 파일 (font_manager + image 신규 + texture 신규)
│  └─ ui/                         ← 16 항목 (widgets 신규 + ui_color_utils + icons/ 13)
├─ features/                      (그대로 빈 폴더)
└─ legacy/                        ← Phase 0/1 동결본 *그대로 유지* (사용자 답변 2.2 / 2.3)
```

## 3.2 git status / commit 분포

> **두 환경의 git 상태가 어긋난다 — Windows 측이 권위적**:
> - Windows 측 (사용자): `git diff --stat -- webassembly/src/legacy` 결과가 비어있음 (사용자 답변 2.2). working tree 깨끗.
> - Linux 측 (Cowork 마운트): `git status` 가 D 15 건을 여전히 보고 — 마운트 캐싱 인공물.

| 분류 (Linux 측 보고) | 개수 | 사용자 답변 후 의미 |
|---|---|---|
| `??` untracked | 37+ | core/{data, io, scene, ui}/ 폴더 단위 + core/render/{image, texture}.* + core/vtk/{mouse_interactor, batch_update_system}.* — **Phase 2 신규 파일들 (실제 변경분)** |
| `D` deleted | 15 | **마운트 캐싱 인공물** — Windows 측에는 모두 원복된 상태 |
| `M` modified | 205 | 대부분 filemode 노이즈 (Phase 0/1 와 동일) + Phase 2 의 CMakeLists.txt + core/render/font_manager.cpp 의 App:: 치환 |

```
최근 커밋 4 개:
  f07e541  Phase 1: bootstrap app/+core shell, exclude legacy build
  4eac024  docs: redevelopment plan and Phase 1 detail
  2a586bb  Phase 0: Move webassembly/src/* to webassembly/src/legacy/
  33eb219  docs: redevelopment plan and Phase 0 detail
```

→ Phase 2 의 코드 변경은 commit 되지 않은 working tree 상태. **Phase 0/1 과 동일한 반복 패턴** (3 회 연속) — 본 평가서의 핵심 정리 항목 중 하나로 남는다.

## 3.3 동적 검증 결과 (사용자 2 차 답변 반영)

사용자 2 차 답변에 따라 Phase 2 의 동적 검증이 모두 통과했다.

| 항목 | 결과 | 의미 |
|---|---|---|
| `npm run build-wasm:debug` exit 0 | ✓ | core/ 50+ 신규 파일 + main/bind/CMake 변경이 debug 모드에서 컴파일 통과 |
| `npm run build-wasm:release` exit 0 | ✓ | release 최적화 빌드도 통과 — Phase 0 시점에 발생했던 SPDLOG 매크로 가드 잠복 버그 같은 문제는 *재발 없음* 확인 |
| `npm run dev` 후 빈 dockspace 표시 | ✓ | Phase 1 과 시각적으로 동일한 빈 dockspace + placeholder 메뉴 |
| 콘솔 에러 0 | ✓ | Embind stub no-op 호출 + 신규 인프라 dead-code-elim 모두 정상 |

→ Phase 2 의 산출물 *(Phase 3 가 가져다 쓸 도구 상자)* 이 실제로 컴파일되어 wasm 바이너리에 포함되며, 런타임 시 *어떤 부수 효과도 일으키지 않음* 이 입증되었다.

> Phase 0/1 의 *"사용자가 직접 동적 검증을 보고하는 패턴"* 이 본 단계에서도 회복되었다. 평가서 1 차 작성 당시 *"Phase 2 의 구조적 특성으로 동적 검증을 결여한다"* 로 본 서술은 사용자 답변에 의해 *부분 부정* 됨 — 인프라 단계라도 빌드 + 빈 dockspace 표시까지는 즉시 입증 가능.

---

# Part 4 — Phase 3 진행가능 여부 판정

## 4.1 Phase 3 입구 조건과의 매핑

`05_redevelopment_plan.md` §6 의 Phase 3 작업은 다음을 전제한다.

| Phase 3 전제 | 현재 상태 | 통과? |
|---|---|---|
| Phase 2 의 core/ 인프라 (scene, io, data, vtk, render, ui) 가 빌드 가능 상태 | **debug + release 빌드 모두 exit 0** (사용자 2 차 답변) | ✓ |
| `core/scene/SceneState`, `EventBus`, `format_registry` 인터페이스 안정 | 인터페이스 작성됨 | ✓ |
| 빈 dockspace 가 그대로 유지 | **유지** (사용자 2 차 답변: `npm run dev` 후 빈 dockspace + 콘솔 에러 0) | ✓ |
| Phase 2 PR commit 머지 (Phase 3 PR 의 base) | **미커밋** (working tree) | ✗ |
| `legacy/` 가 Phase 3 의 *참조 원본* 으로 유지 | **유지됨** (사용자 답변 2.2/2.3) — `toolbar.*` 등 Phase 3.7 의 참조 원본 보존 | ✓ |
| `app/legacy_app_compat.cpp` 정리 (Phase 5 정리 항목 1 건 감소) | **완료** (사용자 답변 2.1 — find 결과로 확인) | ✓ |

→ Phase 3 의 첫 sub-phase (3.1 — utilities/brillouin_zone) 진입 환경이 거의 모두 준비됨. **남은 정리는 Phase 2 PR commit 1 건뿐**.

## 4.2 종합 판정

> **진행 가능 (GO)**
>
> Phase 2 의 본질 (core/ 인프라 골격 작성) + Step 1 청산 + legacy/ 동결 유지 + 동적 검증 (debug + release + 빈 dockspace + 콘솔 에러 0) 모두 통과. 남은 정리는 **Phase 2 코드 commit** 1 가지뿐. commit 정리 후 Phase 3.1 진입 권장.

## 4.3 진입 전 처리할 1 가지 정리 항목 *(2026-04-28 사용자 2 차 답변 반영하여 4 → 1 로 축소)*

> 평가서 1 차 작성 당시의 4 가지 정리 항목 중 **3 가지가 사용자 답변으로 이미 해소됨**:
> - ~~4.3.A `app/legacy_app_compat.cpp` 제거~~ → 사용자 답변 2.1 로 *완료* 확인
> - ~~4.3.B legacy/ 파일 삭제 의도성 확인~~ → 사용자 답변 2.2/2.3 으로 *원복된 상태* 확인 (마운트 캐싱 인공물)
> - ~~4.3.C Debug + Release 빌드 + 빈 dockspace 명시 확인~~ → 사용자 2 차 답변으로 *모두 통과* 확인
>
> 남은 항목은 다음 1 건뿐.

| # | 항목 | 명령 / 방법 | 통과 기준 |
|---|---|---|---|
| 4.3.D | **Phase 2 코드 commit 정리** | (a) `git add -A` (Linux 측은 `.git/index.lock` 제약 가능 → Windows PowerShell 권장). (b) Phase 2 변경 (50+ 신규 파일 + CMake + font_manager 갱신 + Step 1 정리 + Linux 측 D 15 건 의 git index 동기화) 만 별도 commit. (c) PR 본문에 본 평가서와 계획서 링크 포함 | Phase 2 commit 1~2 개 (PR1 scene+data, PR2 io+vtk+render+ui+정리) |

> 본 commit 정리 한 단계만 끝나면 Phase 3.1 의 base commit 이 정의되어 진입할 수 있다.

---

# Part 5 — 메타 평가

## 5.1 계획서의 효과 평가

| 보강 항목 (Phase 2 §) | 효과 |
|---|---|
| §1.2 회색지대 정책 인계 (Phase 0 §1.1 + Phase 1 §1.2) | 적용할 회색지대 사례가 본 시도에서는 없었음. 다만 §1.6.1 의 *legacy 파일 삭제* 라는 새 회색지대 후보가 노출됨 |
| §1.3 어댑터 우선순위 (호출 제거 > DI > shim) | font_manager 의 App:: 호출 제거 (Step 1 의 sed) 로 회색지대 1 건 정리 ✓ |
| §4 Step 1 청산 단계 | **완료** — 사용자 답변 2.1 로 working tree 에서 `legacy_app_compat.cpp` 제거 확인. font_manager 의 App:: sed 치환 + shim 제거 한 사이클이 정상 작동 |
| §4 Step 9 의존 사전 분석 sweep | 본 시도에서 sweep 결과가 PR 본문에 첨부되었는지 미확인. core/ 의 feature-domain 호출 0 결과는 ✓ |
| §6 #4 git rename 인식 격상 | 본 단계는 rename 보다 *신규 파일* 위주여서 본 격상의 효과 적음 |
| §7.9 `.git/index.lock` 환경 제약 | Linux 측 git status 의 D 잔여로 *간접 노출* — 마운트 캐싱 인공물로 확인됨. 동적 검증은 Windows 측에서 수행되어 영향 없음 |

→ 계획서의 큰 그림(인프라 골격 작성 + Step 1 청산 사이클)은 정확히 작동했다. **사용자 2 차 답변** 으로 정적·동적 검증 모두 통과 확인됨.

## 5.2 잔여 리스크 (Phase 3 진입 전 인지 필요)

| # | 리스크 | 완화 |
|---|---|---|
| 5.2.1 | ~~Phase 2 의 core/ 가 실제로 빌드되는지 미입증~~ → **해소됨** | 사용자 2 차 답변: debug + release 빌드 모두 exit 0 |
| 5.2.2 | ~~`legacy/toolbar.*` 등이 삭제되어 Phase 3.7 의 참조 원본 손실~~ → **해소됨** | 사용자 답변 2.2/2.3: 원복된 상태 — Phase 3.7 의 참조 원본 보존됨 |
| 5.2.3 | ~~`app/legacy_app_compat.cpp` 잔존~~ → **해소됨** | 사용자 답변 2.1: 제거 완료 |
| 5.2.4 | **Phase 2 코드 commit 미수행** → Phase 3 PR 의 base 모호 | 4.3.D 통과로 해소 (남은 유일한 정리 항목) |
| 5.2.5 | font_manager.cpp 의 legacy vs core 라인 수 불일치 (Phase 1 부터 이어진 미해결) | Phase 1 평가서 §4.3.D 와 동일 — 별도 처리 필요. 단 동적 검증은 통과했으므로 *기능적 영향은 없음* |
| 5.2.6 | Linux 측 git status 의 D 잔여 (마운트 캐싱) → 차후 sweep 시 false positive 가능 | Windows 측 `git diff --stat` 을 권위적 검증 도구로 사용. Linux 측 `git status` 는 보조 |

## 5.3 종합 평가표

| 항목 | 점수 (5점 만점) | 비고 |
|---|---|---|
| 정적 단계 실행 정확성 | **5** | core/ 골격 작성 + font_manager App:: 정리 + Step 1 청산 모두 정확 |
| 정적 검증 통과율 | **5** | 18 항목 중 통과 9, 매트릭스 #10 (legacy_app_compat 제거) 도 사용자 답변으로 ✓ |
| 동적 검증 통과율 | **5** | 사용자 2 차 답변: debug + release + 빈 dockspace + 콘솔 에러 0 모두 ✓ |
| 계획서 §4 절차의 적합성 | **4** | Step 1 청산이 잊혀지기 쉬운 위치였지만 본 시도에서는 정상 수행됨 |
| 계획서 §6 검증 매트릭스 커버리지 | **4** | 18 항목으로 충분히 세분화. 정량 wasm 사이즈 회귀 (#14) 만 자동화 여지 |
| Phase 2 PR 형태 | **2** | working tree 상태 (Phase 0/1/2 모두 동일 패턴 — 3 회 연속) |
| Phase 3 입구 도달도 | **5** | commit 정리만 끝나면 즉시 진입 가능 |
| 종합 | **진행 가능 (GO)** | Phase 0/1/2 중 가장 깔끔한 결과 — 1 차 평가의 일탈 4 종 모두 사용자 답변으로 해소, 동적 검증도 모두 통과 |

> Phase 2 는 *"보이지 않는 부분의 작업"* 이라는 인프라 단계의 본질에도 불구하고, **사용자가 적극적으로 동적 검증을 수행한 결과** 컴파일/런타임 정상 동작이 입증되었다. 평가서 1 차 작성 당시의 비관 (*"동적 검증 사각지대"*) 는 사용자 답변으로 부정됨 — 인프라 단계라도 *빌드 + 빈 dockspace 표시* 까지는 충분히 동적 검증 가능하다는 것이 본 사이클의 학습.

---

## 6. 결론 및 권장 다음 단계

### 결론 *(2026-04-28 사용자 2 차 답변 반영)*

> **Phase 2 의 본질적 작업 (core/ 인프라 골격 50+ 파일 작성) + 정적/동적 검증 모두 완료되었다.** 평가서 1 차의 4 가지 정리 항목 중 3 가지(A/B/C) 가 사용자 답변으로 해소됨. **남은 정리는 Phase 2 코드 commit (4.3.D) 1 가지뿐**.

### 권장 다음 단계

1. **§4.3.D** — Phase 2 코드 commit 정리 (PR 1~2 개로 분할 권장 — Windows PowerShell 측에서 수행).
2. (commit 통과 후) — `phase3_features_migration.md` 또는 `phase3_1_utilities_brillouin_zone.md` 세부계획서 작성에 진입.

### Phase 3 진입 신호 *(축소: 5 → 1)*

다음 1 개가 ✓ 면 Phase 3.1 PR 을 시작해도 무방하다.

- [x] ~~`app/legacy_app_compat.cpp` 제거됨~~ (완료 — 사용자 답변 2.1)
- [x] ~~`legacy/` 의 D 항목 정리~~ (완료 — 사용자 답변 2.2/2.3, 마운트 캐싱 인공물로 확인)
- [x] ~~`npm run build-wasm:release` exit 0 재확인~~ (완료 — 사용자 2 차 답변)
- [x] ~~빈 dockspace 표시 + 콘솔 에러 0 재확인~~ (완료 — 사용자 2 차 답변)
- [ ] Phase 2 PR commit 1~2 개로 정리되어 머지 또는 push

---

## 7. 관련 문서

- 보강된 Phase 2 계획서: [`./phase2_core_skeleton.md`](./phase2_core_skeleton.md)
- 선행 Phase 1 평가서: [`./phase1_evaluation_2026-04-28.md`](./phase1_evaluation_2026-04-28.md)
- 선행 Phase 1 계획서: [`./phase1_app_core_bootstrap.md`](./phase1_app_core_bootstrap.md)
- 선행 Phase 0 통합 평가서: [`./phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md)
- 선행 Phase 0 계획서: [`./phase0_legacy_freeze.md`](./phase0_legacy_freeze.md)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §5 Phase 2 / §6 Phase 3
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md)
- 메뉴 ↔ 코드 매핑 표 (Phase 3 의 진입점): [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
