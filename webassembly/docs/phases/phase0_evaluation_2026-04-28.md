# Phase 0 시도 평가 및 Phase 1 진행가능 여부 통합 평가서 (2026-04-28)

> 평가 대상: Phase 0 (Legacy Freeze) 의 1차/2차 시도 회고 + Phase 1 입구 조건 충족 여부
> 평가일: 2026-04-28
> 평가 브랜치: `refactor/menu-aligned`
> 결과: **조건부 진행 가능 (Conditionally GO)** — Phase 0 의 본질 통과, 진입 전 정리 3 항목 남음
> 본 문서는 2026-04-28 작성된 두 개의 평가서(Phase 0 시도 평가 + Phase 1 readiness) 를 하나로 통합한 결과물이다.

## 0. 한 줄 결론

> 1차 Phase 0 시도는 **정적 검증 7/7 통과 후 release 빌드의 사전 존재하던 잠복 버그 1 건이 노출되어 사용자 결정으로 전체 롤백** 되었다. 그 학습으로 계획서를 보강하고(§1.1 회색지대, §2a Pre-Phase 0, §7.9 환경 제약 등) 2 차 시도를 진행한 결과 working tree 적용 + 런타임 18 항목 통과까지 도달했다. 다만 Phase 1 PR 의 base commit 을 위해 **3 가지 정리 항목** (release 빌드 명시 통과 / git stage 정리 + 단일 commit / SHA baseline 비교) 처리 후 Phase 1 진입을 권장한다.

---

# Part 1 — 1차 Phase 0 시도 회고

## 1.1 타임라인

| 시각 (≈) | 단계 | 결과 |
|---|---|---|
| t0 | 브랜치 확인: `refactor/menu-aligned` 위 + `webassembly/docs` commit `33eb219` | OK |
| t0+1m | Step 1 — `webassembly/src/legacy/` 폴더 신설 | OK |
| t0+3m | Step 2 — 54 항목을 `legacy/` 로 이동 (plain `mv` — 사유: §1.5 환경 제약) | OK |
| t0+4m | Step 3 — `main.cpp` / `bind_function.cpp` include 6 줄 패치 | OK |
| t0+5m | Step 4 — `CMakeLists.txt` path prefix 70 줄 갱신 | OK |
| t0+6m | Step 5(정적) — `ls`/`grep`/구조 검증 7 종 | 7/7 통과 |
| t0+8m | (사용자) Windows 측 `npm run build-wasm:release` 시도 | **실패** — `legacy/atoms/atoms_template.cpp:3168` `AtomType::ORIGINAL` undeclared |
| t0+10m | 1 줄 build-fix 적용: `AtomType::` → `atoms::domain::AtomType::` | OK |
| t0+11m | (사용자) 콘솔 preload 경고 보고 — Phase 0 와 **무관**한 Next.js 자동 preload 노이즈 | 별도 이슈 분리 |
| t0+12m | `app/layout.tsx` 의 Geist 폰트에 `preload: false` 옵션 추가 | OK (Phase 0 외 변경) |
| t0+15m | (사용자) **전체 롤백** 결정 | — |

## 1.2 1차 시도의 §6 검증 매트릭스 결과

| # | 검증 항목 | 기대값 | 실제값 | 결과 |
|---|---|---|---|---|
| 1 | `webassembly/src/` 직속 | `bind_function.cpp  legacy  main.cpp` | 동일 | ✓ |
| 2 | `legacy/` `.h` 갯수 | 약 70 | 72 | ✓ |
| 3 | `legacy/` `.cpp` 갯수 | 약 50 | 47 | ✓ |
| 4 | git rename 인식 100+ | — | **미검증** (git add 단계 미실행) | ⊘ |
| 5 | `npm run build-wasm:debug` exit 0 | 0 | **미수행** (사용자가 release 부터 시도) | ⊘ |
| 6 | `npm run build-wasm:release` exit 0 | 0 | **1 (실패)** — 잠복 버그 노출 | ✗ |
| 7 | 18 항목 메뉴 회귀 테스트 | 모두 ✓ | **미수행** (release 빌드 실패로 도달 못 함) | ⊘ |
| 8 | `font_manager.cpp` SHA 동일성 | main 과 일치 | **미검증** | ⊘ |

> 통과 3 / 실패 1 / 미검증 4. 정적 검증은 100% 통과했고, 동적 검증은 release 빌드에서 좌초.

## 1.3 핵심 발견 — release-only 잠복 버그

### 위치
`webassembly/src/atoms/atoms_template.cpp:3168` (Phase 0 이전 경로 기준).

### 코드
```cpp
SPDLOG_DEBUG("Created atom {} with radius {:.3f}, ID {}, type {} (unified system only)",
            symbolStr, adjustedRadius, atomId,
            atomType == AtomType::ORIGINAL ? "ORIGINAL" : "SURROUNDING");  // ← namespace 누락
```

같은 함수 안의 다른 7 군데(line 3087, 3111, 3122, 3157, 3498, 3641, 3713 등) 는 모두 `atoms::domain::AtomType::ORIGINAL` 형태로 namespace qualifier 를 붙여 사용. 즉 line 3168 만 한 단어 누락된 typo.

### debug 빌드에서는 왜 통과했는가
- `SPDLOG_ACTIVE_LEVEL` / `SPDLOG_LEVEL_*` 매크로 정책 차이로, debug 빌드에서는 `SPDLOG_DEBUG(...)` 매크로가 인자 부분을 컴파일러에 노출하지 않거나 dead-code-eliminate 처리됨.
- release 빌드에서는 동일 매크로가 인자를 평가/컴파일하면서 미해결 식별자가 표면화.

### 책임 소재
**Phase 0 가 일으킨 회귀가 아니다.** Phase 0 는 파일 이동만 했고 내용을 1 byte 도 건드리지 않았다. 원인은 PR/commit 단계에서 release 빌드 회귀 점검이 없었던 것에 있다.

## 1.4 typo build-fix 의 분류 문제 (1차 시도가 노출시킨 회색지대)

계획서 §1 의 비목표는 **"코드 수정"**. 1차 시도에서 적용한 1 줄은 형식상 코드 수정이지만, *typo* 이며 *동작은 0 변화* 이고 *빌드를 통과시키기 위한 최소 변경* 이다.

→ 회색지대로 인정하고 보강된 계획서 §1.1 에 명문화.

## 1.5 환경 제약 — `.git/index.lock` 일관성 이슈

- 본 환경(Windows-Linux 바인드 마운트) 에서는 `.git/index.lock` 의 일관성 이슈로 Linux 측에서 `git mv` / `git add` / `git commit` 이 일시적으로 차단된다.
- 1차 시도에서 plain `mv` 로 이동 후 git rename detection 에 의존한다는 우회를 채택했다.
- 이 환경 제약은 보강된 계획서 §7.9 에 명문화.

## 1.6 1차 시도가 계획서에 남긴 빈틈 6 가지

| # | 빈틈 | 보강된 §위치 |
|---|---|---|
| 1 | 사전 release 빌드 점검 부재 → 잠복 버그가 Phase 0 단계에서 노출됨 | §2a Pre-Phase 0 PR 신설 |
| 2 | `git mv` 가 막혔을 때의 우회 절차 미명시 | §5 Step 2, §7.9 |
| 3 | 빈 `legacy/` 디렉터리 정리 절차 미명시 | §5 Step 1 롤백 노트 |
| 4 | release-only / debug-only 매크로 가드 코드의 사전 sweep 부재 | §2a 의 typo sweep 절차 |
| 5 | Phase 0 와 무관한 노이즈(예: preload 경고)의 분리 정책 부재 | §7.10 |
| 6 | 18 항목 회귀 테스트의 자동화 가능성 미검토 | §7.11 |

---

# Part 2 — 계획서 보강 (1차 → 2차 사이의 회복 단계)

평가서를 토대로 `phase0_legacy_freeze.md` 를 다음과 같이 보강.

| 보강 | 내용 |
|---|---|
| §1.1 회색지대 — typo build-fix | 동작 0변화 + 5줄 이내 + 빌드 통과 필수 — 셋 다 만족 시 비목표 위반으로 보지 않음. 단 §0a 사전 처리 우선 |
| §2a Pre-Phase 0 PR (신규) | Phase 0 진입 전 release 빌드 baseline 확보 + typo sweep |
| §3 사전 준비 체크리스트 | "§2a 머지 baseline" + "`git mv` 막힘 가능성 인지" 추가 |
| §5 Step 1 — 빈 디렉터리 롤백 노트 | mkdir 빈 폴더는 git 추적 X, 수동 정리 필요 |
| §5 Step 2 — plain `mv` 우회 절차 | git rename detection 의존 명시 |
| §6 검증 매트릭스 | #0 baseline 행 추가, #4 git rename 인식 행 격상, 검증 순서 명시 |
| §7 리스크 11 행 (4 행 신규 추가) | 7.8 release-only 잠복 / 7.9 `.git/index.lock` / 7.10 무관 노이즈 / 7.11 회귀 사람-손 비용 |
| §11 관련 문서 (신규) | 평가서 / 매핑 / 메뉴 트리 양방향 링크 |
| 변경 이력 표 (헤더) | 2026-04-28 보강 내역 명시 |

---

# Part 3 — 2차 Phase 0 시도 평가 (사용자 재실행)

## 3.1 평가 시점의 저장소 상태

### 파일 트리

| 항목 | 상태 | 계획서 기대치 |
|---|---|---|
| `webassembly/src/` 직속 | 3 (`bind_function.cpp`, `legacy`, `main.cpp`) | 3 ✓ |
| `legacy/` `.h` 파일 수 | 72 | 약 70 ✓ |
| `legacy/` `.cpp` 파일 수 | 47 | 약 50 ✓ |
| `legacy/` `.md` 파일 수 | 8 | 8 ✓ (refactory*.md, batchOnly.md) |
| `legacy/` `.ini` 파일 수 | 1 | 1 ✓ (imgui.ini) |

### 핵심 파일 변경 상태

| 파일 | 적용 여부 |
|---|---|
| `main.cpp` 의 `#include "legacy/X.h"` 6 줄 | ✓ |
| `bind_function.cpp` 의 `#include "legacy/X.h"` 6 줄 | ✓ |
| `CMakeLists.txt` 의 `webassembly/src/legacy/` prefix 70 줄 | ✓ |
| `legacy/atoms/atoms_template.cpp:3168` build-fix (`atoms::domain::AtomType::ORIGINAL`) | ✓ |

### git status 분포

| 분류 | 개수 | 의미 |
|---|---|---|
| `RM` rename-modified | 122 | Phase 0 의 rename 들이 git 의 content-similarity 로 자동 인식됨 ✓ |
| `AM` added-modified | 2 | `legacy/atoms/atoms_template.cpp` 등 — build-fix 로 내용이 달라져 rename 미인식 (정상) |
| `MM` modify-modified | 4 | `CMakeLists.txt`, `bind_function.cpp` 등 patch 들 |
| `M` (가운데 공백) | 77 | filemode 노이즈 (Phase 0 와 무관) |
| `D` deleted | 4 | (아래 정합성 이슈 참조) |
| `??` untracked | 16 | (아래 정합성 이슈 참조) |

### 정합성 이슈

```text
D  webassembly/src/main.cpp    ← tracked 상태에서 삭제로 잡힘
?? webassembly/src/main.cpp    ← 동시에 untracked 로도 잡힘
?? webassembly/src/legacy/{toolbar,unv_reader,vtk_viewer}.{cpp,h}
```

분석:
- `main.cpp` 가 `D` + `??` 양쪽에 동시 등장: 1차/2차 사이클에서 staging 어긋남의 흔적. **기능 영향 없음**. `git add -A` 한 번으로 정리됨.
- 일부 legacy 파일이 `??` 인 것: 같은 사이클에서 일부가 중간 상태에 멈춤. `git add -A` 로 단번에 정리됨.

→ 파일 시스템 레벨에서는 Phase 0 가 깔끔하게 적용되어 있다. 다만 git index 가 부분적으로 불일치.

## 3.2 2차 시도의 §6 검증 매트릭스 충족도

(보강된 계획서 §6 기준)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 0 | §2a Pre-Phase 0 baseline (release 빌드 통과) | 0 | 별도 PR 없이 회색지대 build-fix 1 줄 적용 후 통과한 것으로 추정 | △ 부분 |
| 1 | `webassembly/src/` 직속 = 3 | 3 | 3 | ✓ |
| 2 | `legacy/` `.h` 갯수 | ≈ 70 | 72 | ✓ |
| 3 | `legacy/` `.cpp` 갯수 | ≈ 50 | 47 | ✓ |
| 4 | git rename 인식 100+ | 100+ | 122 (RM) | ✓ |
| 5 | `npm run build-wasm:debug` exit 0 | 0 | 추정 통과 (사용자 보고: dev 동작 중) | △ 추정 |
| 6 | `npm run build-wasm:release` exit 0 | 0 | **명시 확인 필요** | ⊘ |
| 7 | 18 항목 메뉴 회귀 | 모두 ✓ | **통과 (사용자 명시 보고)** | ✓ |
| 8 | `font_manager.cpp` SHA baseline 동일 | 동일 | **완료** — baseline blob SHA256 = legacy blob SHA256 (`6247D6F3DF70908D14187D414061F00BFA43BB43DB22C24712FBCA2FCAC7C613`) | ✓ |

**합계**: 통과 6 / 부분 2 / 미수행 1 / 실패 0

## 3.3 사용자 런타임 테스트의 의미

사용자가 보고한 *"npm run dev 실행 후 런타임에서 기능에 대한 테스트는 완료"* 는 §6 의 #5 (debug 빌드) + #7 (18 항목 회귀) 에 대한 강한 증거다.

`npm run dev` 가 트리거하는 것:
1. Next.js 개발 서버 부팅 → React/TS/CSS 컴파일.
2. 브라우저에서 `/workbench` 접근 시 `/wasm/VTK-Workbench.{js,wasm,data}` 로딩.
3. 사용자가 메뉴/툴바/렌더링을 직접 조작 → 18 항목 체크리스트 검증 가능.

따라서 **현재 working tree 가 실제로 동작하는 빌드 산출물을 만들어내고 그 산출물이 메뉴 18 항목을 정상 처리** 한다는 것이 입증되었다. 이는 Phase 0 의 가장 중요한 미덕(*"빌드와 런타임 동작이 100 % 동일"*) 의 직접 증거다.

다만 dev 만으로는 입증되지 않는 것:
- (a) `npm run build-wasm:release` 의 emcc release 최적화 빌드가 통과하는가? — release-only 잠복 버그가 또 있을 가능성 0 은 아님.
- (b) `font_manager.cpp` 의 33,885 줄 자동생성 데이터가 byte-identical 로 옮겨졌는가?

---

# Part 4 — Phase 1 진행가능 여부 판정

## 4.1 Phase 1 입구 조건과의 매핑

| Phase 1 전제 (`05_redevelopment_plan.md` §4) | 현재 상태 | 통과? |
|---|---|---|
| Phase 0 의 디렉터리 트리(`webassembly/src/{legacy, main.cpp, bind_function.cpp}`) 가 확립되어 있다 | working tree 에 확립됨 | ✓ |
| 빌드 통과 (`npm run build-wasm:debug`) | 통과 추정 | △ |
| 런타임 동작 확인됨 | 사용자 보고 통과 | ✓ |
| Phase 0 PR 머지 (Phase 1 PR 의 base commit) | **미커밋** (working tree 상태) | ✗ |
| `app/`, `core/`, `features/` 빈 폴더를 신설할 수 있는 여유 공간 | 가능 | ✓ |

→ Phase 1 의 작업 자체는 시작할 수 있는 환경이지만, **Phase 0 PR 이 commit 되어야 Phase 1 PR 의 base 가 정의된다.** 두 PR 이 working tree 에 섞이면 검토자에게 diff 가 폭발한다.

## 4.2 종합 판정

> **조건부 진행 가능 (Conditionally GO)**
>
> 정적 변경 + 런타임 동작은 모두 입증되었다. Phase 1 의 작업 자체를 시작하기에 부족함이 없다.
> 단 **Phase 0 PR 을 먼저 commit/머지** 해야 Phase 1 PR 의 base commit 이 명확해지고 검토 비용이 폭발하지 않는다.

## 4.3 진입 전 처리할 3 가지 정리 항목

| # | 항목 | 명령 / 방법 | 통과 기준 |
|---|---|---|---|
| A | **release 빌드 명시 확인** | `npm run build-wasm:release` | exit 0 |
| B | **git stage 정리 + Phase 0 commit** | (a) `git add -A` 한 번으로 D+?? 충돌 정리. (b) Phase 0 의 변경(122 rename + 4 patch) 만 별도 commit. (c) `legacy/atoms/atoms_template.cpp` 의 build-fix 1 줄은 회색지대 정책에 따라 rename commit 과 분리해 별도 commit. (d) filemode 노이즈(`M` 77 개) 는 본 PR 에 포함하지 않도록 분리 | (1) Phase 0 commit 1~2 개 (rename + build-fix). (2) PR 본문에 본 평가서와 보강된 계획서 링크 |
| C | **`font_manager.cpp` SHA 동일성 검증** | PowerShell: baseline/legacy **blob** 을 각각 추출해 `Get-FileHash -Algorithm SHA256` 비교 (실행 완료) | 두 SHA 동일 (확인됨) |

> A 가 실패하면 §2a Pre-Phase 0 의 추가 typo sweep 이 필요. 같은 패턴(namespace 누락) 의 잠복 버그가 또 있다는 의미.

## 4.4 진입 *후* 의 첫 작업 (Phase 1 §1)

`phase1_app_core_bootstrap.md` 가 작성되기 전, 상위 계획서 `05_redevelopment_plan.md` §4 에 따라:

1. `webassembly/src/{app, core/{vtk, io, data, scene, render, ui}, features}` 빈 폴더 신설.
2. `app/app.cpp` + `app/app.h` 작성 — `legacy/app.cpp` 의 dockspace 코드 골격을 최소화해 가져옴.
3. `core/vtk/vtk_viewer.cpp/h` 작성 — `legacy/vtk_viewer.*` 에서 최소 셸 추출.
4. `main.cpp` 가 새 `app::App` 만 참조하도록 갱신 (legacy/ 의존을 점진 제거).
5. `bind_function.cpp` 도 동일하게 점진 교체.
6. `CMakeLists.txt` 에 새 source 들 추가.
7. 검증: `npm run build-wasm:debug` 통과 + 빈 dockspace 가 브라우저에 뜸.

> Phase 1 은 Phase 0 와 달리 **빈 화면이 정상 결과** 다. 18 항목 회귀는 Phase 1 의 종료 기준이 아니라 Phase 3 (피처 이식) 진행 중에 점진적으로 회복된다.

---

# Part 5 — 메타 평가

## 5.1 보강된 계획서의 효과 평가

| 보강 항목 | 효과 |
|---|---|
| §1.1 회색지대 typo build-fix 정책 | atoms_template.cpp:3168 build-fix 가 합법적으로 처리됨 ✓ |
| §2a Pre-Phase 0 PR 신설 | (별도 PR 로 분리하지 않았지만) build-fix 의 분리 commit 정책으로 흡수 가능 △ |
| §3 사전 준비 — release 빌드 baseline 인지 | 명시적 통과 보고 누락 — **다음 시도에서도 강조 필요** ⚠ |
| §5 Step 1 빈 디렉터리 롤백 노트 | 본 시도에서는 영향 없음 |
| §5 Step 2 plain `mv` 우회 절차 | git rename detection 이 122 항목 인식으로 작동 ✓ |
| §6 #4 git rename 인식 격상 | 122 RM 으로 명확히 가시화됨 ✓ |
| §7.9 `.git/index.lock` 환경 제약 | 이번 시도에서 lock 이슈 발생 안 함 (이전 사이클에서 정리됨) |

→ 보강 효과는 대체로 긍정적. 다만 *"release 빌드 명시 통과 확인"* 이 다시 비명시 상태라, **계획서 §3 사전 준비 체크리스트의 첫 항목** 으로 더 강조할 가치가 있다.

## 5.2 잔여 리스크 (Phase 1 진입 전 인지 필요)

| # | 리스크 | 완화 |
|---|---|---|
| 5.2.1 | release 빌드의 또 다른 잠복 버그 가능성 | 4.3.A 통과로 검증 |
| 5.2.2 | filemode 노이즈 77 개가 향후 PR 에 잡음으로 누적 | `git config core.filemode false` (글로벌 또는 repo-local) 로 향후 사이클 차단 검토 |
| 5.2.3 | Phase 0 PR 의 commit 단위가 너무 커서 검토자 부담 | rename commit + build-fix commit + (옵션) filemode-cleanup commit 으로 3 분할 |
| 5.2.4 | Phase 1 의 새 폴더가 main.cpp / bind_function.cpp 와 어떻게 점진 교체되는지 미정 | Phase 1 세부계획서(`phase1_app_core_bootstrap.md`) 작성 시 명확화 |

## 5.3 1차 시도 종합 평가표

| 항목 | 점수 (5점 만점) | 비고 |
|---|---|---|
| 정적 단계 실행 정확성 | **5** | 파일 이동·include·CMake 모두 의도대로 수행 |
| 정적 검증 통과율 | **5** | 7/7 |
| 동적 검증 통과율 | **0** | release 빌드 실패 + 그 이후 미수행 |
| 계획서 §5 절차의 적합성 | **4** | 한 가지 사전점검 단계만 보강하면 5점 |
| 계획서 §7 리스크 커버리지 | **3** | 환경 제약(`index.lock`) 누락 |
| 비목표 정의의 명확성 (§1) | **3** | typo build-fix 회색지대 |
| 종합 | **재시도 가치 있음** | 보강 항목이 명확히 좁혀져 있어 다음 시도는 깨끗하게 끝낼 수 있음 |

> 1차 시도는 **실패가 아니라 정찰** 이었다. 정찰의 산출물(이 평가서) 을 반영해 계획서를 한 차례 보강한 후 2차 시도에 들어간 것이 깨끗한 결과를 만들어냈다.

## 5.4 2차 시도 종합 평가표

| 항목 | 점수 (5점 만점) | 비고 |
|---|---|---|
| 정적 단계 실행 정확성 | **5** | 1차의 결과물 그대로 적용 |
| 정적 검증 통과율 | **5** | 1~3, 4 모두 통과 |
| 동적 검증 통과율 | **4.0** | 18 항목 회귀 ✓ + debug 추정 통과, release 명시 미보고, SHA 검증 완료 |
| Phase 0 PR 형태 | **2** | working tree 에 머무는 상태. commit/PR 분할 필요 |
| Phase 1 입구 도달도 | **4** | 작업 환경은 준비됨. base commit 이 정의되면 5 |
| 종합 | **조건부 진행 가능** | 4.3 의 3 가지 정리 후 Phase 1 PR 시작 |

---

## 6. 결론 및 권장 다음 단계

### 결론

> **Phase 0 의 본질적 작업(rename + include/CMake 패치 + build-fix) 은 완료되었다. Phase 1 진입을 위해 처리할 3 가지 정리 항목(§4.3) 이 남아 있다.**

### 권장 다음 단계

1. **§4.3.A** — `npm run build-wasm:release` 명시 통과 확인 (Windows PowerShell).
2. **§4.3.B** — Phase 0 commit 정리: rename + build-fix 2 개 commit 으로 분할. filemode 노이즈는 분리.
3. **§4.3.C** — `font_manager.cpp` SHA 동일성 검증 **완료**.
4. (위 3 항목 통과 후) — `phase1_app_core_bootstrap.md` 세부계획서 작성에 진입.

### Phase 1 진입 신호

다음 4 개가 모두 ✓ 면 Phase 1 PR 을 시작해도 무방하다.

- [ ] `npm run build-wasm:release` exit 0 (재확인)
- [ ] Phase 0 PR commit 1~3 개로 정리되어 머지 또는 push
- [ ] `git status --short -uall` 출력에 Phase 1 작업과 무관한 잔여 stage 없음
- [x] `font_manager.cpp` SHA 동일 (선택)

---

## 7. 관련 문서

- 보강된 계획서: [`./phase0_legacy_freeze.md`](./phase0_legacy_freeze.md) (rev. 2026-04-28)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §3 Phase 0 / §4 Phase 1
- 메뉴 트리 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md)
- 회귀 테스트 18 항목 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §5
