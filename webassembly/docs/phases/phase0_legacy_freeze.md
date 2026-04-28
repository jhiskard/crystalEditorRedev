# Phase 0 — 현재 구조 동결 (Legacy Freeze) 세부계획서

> 상위 문서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §3 Phase 0
> 평가서:  [`./phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md)
> 작성일: 2026-04-27
> 최근 보강: 2026-04-28 (첫 번째 시도 평가서 §8 권장사항 반영)
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR: **2 개** (Pre-Phase 0 build-fix PR + Phase 0 freeze PR)
> 예상 소요: 1~2 시간 (Pre-Phase 0) + 1~2 시간 (Phase 0)

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-04-27 | 초안 작성 (전략 A — Path-Shift Freeze) |
| 2026-04-28 | 첫 번째 시도 평가서 §8 권장사항 반영: §0a Pre-Phase 0 PR 신설, §1 비목표의 typo build-fix 회색지대 정책, §3 사전 점검에 release 빌드/typo sweep 추가, §5 Step 1 빈 디렉터리 처리 노트, §6 검증 매트릭스에 git rename 인식 행 격상, §7 리스크에 `.git/index.lock` 환경 제약 추가 |

## 0. 한 줄 요약

> `webassembly/src/legacy/` 폴더를 만들고, 현재 모든 `webassembly/src/*` 파일을 그 아래로 이동시킨 뒤, **빌드와 런타임 동작이 100 % 동일** 한 상태로 끝낸다.
> 이 단계의 핵심 가치는 *"새 코드를 어디에 쓸 빈 공간을 만든다"* 는 것이지, 코드 자체를 고치는 것이 아니다.

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `webassembly/src/legacy/` 트리 신설, (b) `webassembly/src/{main.cpp, bind_function.cpp}` 외 모든 `.cpp/.h/.md/.ini` 파일을 legacy/ 로 이동, (c) `CMakeLists.txt` 가 새 경로를 참조하도록 갱신, (d) `npm run build-wasm:debug` + `npm run build-wasm:release` 빌드 통과, (e) 런타임 동작이 변경 전과 비교해 1px 도 다르지 않음 |
| **비목표** | 코드 수정(아래 회색지대 예외 참조), 인터페이스 변경, 새 폴더(app/, core/, features/) 생성, 메뉴 동작 조정, Doxygen 주석 추가, `legacy/` 내부 정리 |

> **비목표가 어겨지면 PR 을 거부한다.** Phase 0 의 한 가지 미덕은 "검토자가 git diff 를 보고 'rename' 외에 의심할 게 없다" 가 되는 것.

### 1.1 회색지대 — typo build-fix

다음 조건을 **전부** 만족하는 변경은 비목표 위반으로 보지 **않는다** (단, **§0a Pre-Phase 0 PR 에서 미리 처리하는 것을 우선** 으로 한다).

1. 동작 0 변화 (런타임 의미가 동일한 typo 수준 — 예: namespace qualifier 누락, 미사용 변수 제거).
2. 변경 라인 수가 함수 1 개 또는 5 줄 이내.
3. 빌드를 통과시키지 않으면 §6 의 동적 검증 4/4 를 만족할 수 없는 경우.

회색지대에 해당되어 Phase 0 PR 안에 끼워 넣는 경우, **rename commit 과 build-fix commit 을 분리** 한다 (검토자가 build-fix 쪽만 따로 정성 검토할 수 있게).

> 첫 번째 시도(2026-04-28) 에서 발견된 `legacy/atoms/atoms_template.cpp:3168` 의 `AtomType::ORIGINAL` → `atoms::domain::AtomType::ORIGINAL` 가 정확히 이 회색지대에 해당된다. 본 계획서의 §0a Pre-Phase 0 PR 단계에서 사전 처리하면 Phase 0 PR 의 코드 수정은 0 줄이 된다.

## 2. 두 가지 전략 비교

| | 전략 A — Path-Shift Freeze (권장) | 전략 B — Stub Replacement |
|---|---|---|
| 정의 | legacy/ 로 옮기고 CMake/include 경로만 갱신. **런타임 동작 동일** | legacy/ 로 옮기고 main.cpp 를 빈 셸로 대체. **빌드만 통과, 기능 0** |
| 위험도 | 낮음 (rename PR) | 매우 높음 (한 번에 모든 기능 정지) |
| diff 크기 | rename 다수 + CMakeLists.txt 1줄/파일 + include 1~2 곳 | rename 다수 + main.cpp 전면 재작성 + CMakeLists.txt 거의 전체 |
| 검증 난이도 | 메뉴 ✓ 18 항목 그대로 통과 | 빈 화면만 보임 |
| 후속 Phase 1 | `app/` 도입 시 점진 교체 가능 | `app/` 가 들어와야 비로소 화면이 다시 생김 |

→ **본 계획서는 전략 A 만 다룬다.** 전략 B 는 Phase 1 에서 실시한다.

> 상위 `05_redevelopment_plan.md` §3 의 4번 항목 *"임시로 `legacy/` 를 컴파일 대상에서 제외하고 main 만 남긴 최소 빌드"* 는 전략 B 와 일치하지만, 본 세부계획서에서는 **검토 안전성** 을 우선해 전략 A 를 채택한다. Phase 1 에서 자연스럽게 전략 B 의 "빈 셸" 상태로 전환된다.

## 2a. Pre-Phase 0 PR — 사전 build-fix (신규)

> **반드시 Phase 0 진입 *전* 한 번의 별도 PR 로 처리한다.**

### 배경

첫 번째 시도(평가서 §3) 에서 release 빌드 잠복 버그 1 건이 Phase 0 단계에 노출되어, "코드 수정 0 줄" 이라는 Phase 0 의 미덕이 손상되었다. 그 잠복 버그는 Phase 0 가 만든 회귀가 아니라 main 에 이미 있던 typo 였다. 따라서 본 계획서는 Phase 0 에 들어가기 전에 **release 빌드를 한 번 깨끗이 통과시키는 것** 을 명시적 단계로 둔다.

### 작업

1. `main` 또는 `refactor/menu-aligned` 의 **현재 경로 그대로** 다음 두 빌드를 시도:
   ```powershell
   npm run rm-build
   npm run build-wasm:debug
   npm run build-wasm:release
   ```
2. release 빌드가 실패하면 발생한 typo 들을 §1.1 회색지대 정책에 따라 정리. 알려진 1 건:
   ```cpp
   // webassembly/src/atoms/atoms_template.cpp:3168
   //   atomType == AtomType::ORIGINAL ? "ORIGINAL" : "SURROUNDING");
   //  →
   //   atomType == atoms::domain::AtomType::ORIGINAL ? "ORIGINAL" : "SURROUNDING");
   ```
3. 같은 패턴의 잠복 버그 sweep (다른 enum 도 동일한 누락이 있는지 한 번 훑는다):
   ```bash
   grep -rn "AtomType::"     webassembly/src/ | grep -v "atoms::domain::AtomType::" | grep -v "enum class" | grep -v "//" | grep -v ".md:"
   grep -rn "BondType::"     webassembly/src/ | grep -v "atoms::domain::BondType::"  | grep -v "enum class" | grep -v "//" | grep -v ".md:"
   grep -rn "MeasurementType::" webassembly/src/ | grep -v "//" | grep -v ".md:"
   ```
4. 각 fix 는 한 commit 으로 작게 쪼개고, PR 본문에 *"release-only 잠복 버그 정리 — Phase 0 의 사전 점검"* 임을 명시.
5. release + debug 둘 다 0 error 통과 확인 후 머지.

### 통과 기준

- [ ] `npm run build-wasm:debug` exit 0
- [ ] `npm run build-wasm:release` exit 0
- [ ] 위 grep sweep 결과가 0 항목 (또는 fix 후 0 항목)
- [ ] 18 항목 메뉴 회귀 테스트 통과 (Phase 0 직전 baseline)

### 산출물

- 별도 PR 1 개 (typo fix 들).
- 본 PR 의 머지 커밋 SHA 를 §6 검증 매트릭스의 행 #5/#6 비교 baseline 으로 사용.

> Pre-Phase 0 PR 이 끝나야 비로소 Phase 0 PR 에 진입한다.

## 3. 사전 준비 (체크리스트)

작업 시작 전에 다음을 확인한다.

- [ ] **§2a Pre-Phase 0 PR 이 머지되어 있는지 확인.** `npm run build-wasm:debug` + `npm run build-wasm:release` 가 둘 다 0 error 로 통과하는 baseline 이어야 한다 — 이 baseline 이 Phase 0 의 §6 검증 매트릭스의 비교 기준이 된다
- [ ] 현재 브랜치가 `refactor/menu-aligned` 인지 확인 — `git branch --show-current`
- [ ] working tree 의 unstaged 변경분이 본인 의도와 일치하는지 확인 (현재 `M` 마크 205개 + `??` 3개) — `git status --short | head`
- [ ] Phase 0 작업을 별도 commit 으로 기록할 수 있도록 *기존 변경분을 먼저 한 번 커밋* 또는 *stash 처리* 한다. 이동 작업이 기존 modified 파일과 섞이면 diff 가 폭발한다
- [ ] Phase 0 PR 단위가 너무 커지는 것을 막기 위해, 빌드/디스패치 검증용 emscripten 환경이 손에 잡힌 상태인지 확인 (Docker 또는 emsdk 4.0.3)
- [ ] 본 환경에서 `git mv` 가 막힐 가능성을 인지 (자세한 사항은 §7.9 — `.git/index.lock` 일관성). 막힐 경우 우회 절차도 §7.9 에 명시

권장 첫 커밋:
```bash
git add -A
git commit -m "WIP: pre-phase0 working tree snapshot"
# 또는
git stash push -u -m "pre-phase0"
```

## 4. 파일 이동 매트릭스

### 4.1 legacy/ 로 이동 (A → B)

| 항목 | A: 이동 전 | B: 이동 후 |
|---|---|---|
| 루트 `.cpp/.h` (main.cpp, bind_function.cpp **제외**) | `webassembly/src/<name>.{cpp,h}` | `webassembly/src/legacy/<name>.{cpp,h}` |
| `atoms/` 폴더 전체 | `webassembly/src/atoms/...` | `webassembly/src/legacy/atoms/...` |
| `common/` | `webassembly/src/common/...` | `webassembly/src/legacy/common/...` |
| `enum/` | `webassembly/src/enum/...` | `webassembly/src/legacy/enum/...` |
| `icon/` | `webassembly/src/icon/...` | `webassembly/src/legacy/icon/...` |
| `macro/` | `webassembly/src/macro/...` | `webassembly/src/legacy/macro/...` |
| `config/` | `webassembly/src/config/...` | `webassembly/src/legacy/config/...` |
| `imgui.ini` | `webassembly/src/imgui.ini` | `webassembly/src/legacy/imgui.ini` |
| 기존 리팩터링 메모 (`refactory*.md`, `batchOnly.md`) | `webassembly/src/*.md` | `webassembly/src/legacy/*.md` |
| 분리된 atoms_template 조각 | `webassembly/src/atoms_template_bravais_lattice.cpp`, `atoms_template_periodic_table.cpp` | `webassembly/src/legacy/atoms_template_bravais_lattice.cpp`, `... periodic_table.cpp` |

### 4.2 루트에 남기는 파일 (이동 X)

| 파일 | 이유 |
|---|---|
| `webassembly/src/main.cpp` | Emscripten/CMake 빌드 엔트리. 본 PR 에서 include 경로만 패치 |
| `webassembly/src/bind_function.cpp` | Embind 바인딩 엔트리. 본 PR 에서 include 경로만 패치 |

### 4.3 옮기지 않는 영역

| 경로 | 이유 |
|---|---|
| `webassembly/dependencies/` | 외부 의존성. 본 작업 범위 밖 |
| `webassembly/build/` | 빌드 산출물 |
| `webassembly/resources/` | 폰트/아이콘 리소스. 코드가 아님 |
| `webassembly/docs/` | 본 설계 문서 |

## 5. 작업 절차 (단계별)

### Step 1 — legacy/ 폴더 생성

```bash
cd webassembly/src
mkdir -p legacy
```

> **롤백 노트**: 작업 중 `git restore` 등으로 롤백할 경우, `mkdir` 로 생성된 빈 디렉터리는 git 이 추적하지 않아 그대로 남는다. 신경 쓰이면 Windows 파일 탐색기 또는 `rmdir webassembly/src/legacy` 로 수동 제거. 차후 재시도 시에는 그대로 재사용 가능하므로 보통 그냥 두어도 무방.

### Step 2 — 파일 이동 (`git mv` 우선, 환경 제약 시 plain `mv` 우회)

> **원칙**: `git mv` 사용. 그래야 git status 에서 명시적으로 `R` (rename) 으로 표기되어 검토자에게 친절하다.
>
> **예외**: 본 환경(Windows-Linux 바인드 마운트)에서 `.git/index.lock` 일관성 이슈로 `git mv` 가 실패할 수 있다 (자세한 사항은 §7.9). 이 경우 plain `mv` 로 이동한 뒤, 차후 `git add -A` 단계에서 git 의 content-similarity 기반 rename detection 에 의존한다 (내용이 100 % 동일하면 자동 인식). 이때 §6 검증 매트릭스의 #4 (rename 인식 100+) 검사를 더 엄격하게 수행한다.

```bash
cd webassembly/src

# 2-1. 루트 .cpp/.h (main.cpp, bind_function.cpp 제외)
git mv app.cpp app.h \
       custom_ui.cpp custom_ui.h \
       file_loader.cpp file_loader.h \
       font_manager.cpp font_manager.h \
       image.cpp image.h \
       lcrs_tree.cpp lcrs_tree.h \
       mesh.cpp mesh.h \
       mesh_detail.cpp mesh_detail.h \
       mesh_group.cpp mesh_group.h \
       mesh_group_detail.cpp mesh_group_detail.h \
       mesh_manager.cpp mesh_manager.h \
       model_tree.cpp model_tree.h \
       mouse_interactor_style.cpp mouse_interactor_style.h \
       test_window.cpp test_window.h \
       texture.cpp texture.h \
       toolbar.cpp toolbar.h \
       unv_reader.cpp unv_reader.h \
       vtk_viewer.cpp vtk_viewer.h \
       atoms_template_bravais_lattice.cpp \
       atoms_template_periodic_table.cpp \
       legacy/

# 2-2. 폴더 단위
git mv atoms common enum icon macro config legacy/

# 2-3. 루트 markdown / ini
git mv batchOnly.md refactory.md refactory3.md refactory4.md refactory5.md \
       refactory6_menu_aligned.md refactory_atom_manager.md \
       refactory_details.md refactory_details2.md \
       imgui.ini \
       legacy/
```

> **체크포인트**: `ls webassembly/src/` 결과가 `bind_function.cpp  legacy  main.cpp` 3 항목만 남아야 한다.

### Step 3 — main.cpp / bind_function.cpp include 경로 패치

main.cpp / bind_function.cpp 는 `#include "app.h"` 같은 인접 헤더에 의존한다. 헤더는 이제 `legacy/` 안에 있으므로 경로를 갱신한다.

#### 3.1 `webassembly/src/main.cpp`

| 변경 전 | 변경 후 |
|---|---|
| `#include "app.h"` | `#include "legacy/app.h"` |
| `#include "font_manager.h"` | `#include "legacy/font_manager.h"` |
| `#include "mesh_manager.h"` | `#include "legacy/mesh_manager.h"` |

> 그 외 GLFW / ImGui / Emscripten 헤더는 외부 include path 라 수정 불필요.

#### 3.2 `webassembly/src/bind_function.cpp`

| 변경 전 | 변경 후 |
|---|---|
| `#include "app.h"` | `#include "legacy/app.h"` |
| `#include "file_loader.h"` | `#include "legacy/file_loader.h"` |
| `#include "mesh_manager.h"` | `#include "legacy/mesh_manager.h"` |

#### 3.3 `legacy/` 내부 파일들의 include 는 손대지 않는다

이유: legacy 내부 끼리의 상대 경로는 모두 같은 `legacy/` 안에 있어서 `#include "app.h"` 가 그대로 해석된다. 이때 컴파일러는 같은 디렉터리(`legacy/`) 를 우선 검색하므로 OK.

> **단**, legacy 내 어떤 파일이 `#include "../app.h"` 같은 상대 경로를 쓰고 있었다면 깨질 수 있다. 다음 grep 으로 확인:
> ```bash
> grep -rn '#include "\.\.' webassembly/src/legacy/ | head -50
> ```
> 이미 검토 결과 `atoms/atoms_template.h` 의 `#include "../macro/singleton_macro.h"` 같은 패턴이 존재한다. 이 형태는 `legacy/` 내부에서 `legacy/macro/singleton_macro.h` 로 자동 해석되므로 **수정 불필요**.

### Step 4 — CMakeLists.txt 패치

루트 `CMakeLists.txt` 의 `add_executable(${PROJECT_NAME} ...)` 블록에서 `webassembly/src/` 다음에 곧장 오는 모든 경로를 `webassembly/src/legacy/` 로 prefix 변경. 단, **`webassembly/src/main.cpp` 와 `webassembly/src/bind_function.cpp` 두 줄은 그대로 둔다**.

#### 4.1 변경 방법 — sed 일괄 (권장)

```bash
# 백업
cp CMakeLists.txt CMakeLists.txt.bak

# 일괄 치환: webassembly/src/<X> → webassembly/src/legacy/<X>
# 단, main.cpp / bind_function.cpp 는 보존
python3 - <<'PY'
import re, pathlib
p = pathlib.Path("CMakeLists.txt")
text = p.read_text()
keep = {"webassembly/src/main.cpp", "webassembly/src/bind_function.cpp"}
def repl(m):
    path = m.group(0)
    return path if path in keep else path.replace("webassembly/src/", "webassembly/src/legacy/", 1)
out = re.sub(r"webassembly/src/[A-Za-z0-9_./-]+", repl, text)
p.write_text(out)
PY

# diff 확인
diff CMakeLists.txt.bak CMakeLists.txt | head -80
```

#### 4.2 수동 확인 항목

치환 후 다음을 점검:
- [ ] `add_executable` 안의 모든 `webassembly/src/<X>` 가 `legacy/` prefix 를 가지고 있다 (main, bind 제외).
- [ ] `target_include_directories` / `target_link_directories` 에 `webassembly/src` 가 있다면 그대로 둔다 (헤더 검색 경로 — `#include "legacy/app.h"` 도 여기서 해석됨).
- [ ] `Dependency.cmake`, `webassembly/dependencies/...` 경로는 변경 없음.

### Step 5 — 빌드 검증

```bash
# 클린 빌드 (안전)
npm run rm-build
npm run rm-wasm

# Debug 빌드
npm run build-wasm:debug

# Release 빌드
npm run build-wasm:release
```

성공 기준:
- [ ] 두 빌드 모두 0 error 0 warning (기존 warning 수준 유지).
- [ ] `public/wasm/VTK-Workbench.{js,wasm,data}` 가 갱신됨.
- [ ] 빌드 결과 wasm 크기가 직전 main 브랜치 빌드와 ±5 % 이내.

### Step 6 — 런타임 회귀 테스트

`docs/02_menu_tree.md` §5 의 18 항목 체크리스트를 수동 클릭으로 한 번 훑는다.

```bash
npm run dev
# 브라우저에서 http://localhost:3000/workbench 접속
```

다음 항목이 *Phase 0 직전 main 브랜치와 동일* 하게 동작해야 한다:
- [ ] About 모달
- [ ] File / Open Structure File (XSF / CHGCAR / UNV 파일 1개씩)
- [ ] Edit / Atoms · Bonds · Cell 윈도우
- [ ] Build / Add atoms (Periodic Table) · Bravais Lattice Templates
- [ ] Measurement / Distance · Angle · Dihedral · Geometric Center · Center of Mass
- [ ] Data / Isosurface · Surface · Volumetric · Plane
- [ ] Utilities / Brillouin Zone
- [ ] Settings / 모든 토글
- [ ] Windows / 모든 토글
- [ ] Layout 1/2/3/Reset
- [ ] 툴바 7 종 (Mesh Display, Projection, Reset, Cell Align, Boundary Atoms, Charge Density Quick, Arrow Step)

회귀가 발견되면 Step 7 (롤백) 로 직행.

### Step 7 — 커밋 & PR

```bash
# 단일 커밋 권장 (검토자가 rename 만 보면 됨)
git add -A
git status --short  # rename: A -> B 형태가 다수 보여야 정상
git commit -m "Phase 0: Move all webassembly/src/* to webassembly/src/legacy/

- Create webassembly/src/legacy/ as a frozen snapshot of the current code.
- Move all .cpp/.h/.md/.ini except main.cpp and bind_function.cpp.
- Update main.cpp and bind_function.cpp to include legacy/ headers.
- Update CMakeLists.txt to reference legacy/ paths.
- Build and runtime behavior unchanged.

Reference: webassembly/docs/05_redevelopment_plan.md §3 (Phase 0)
Detailed plan: webassembly/docs/phases/phase0_legacy_freeze.md
"
```

PR 본문 템플릿:
```markdown
## What
Phase 0 — Legacy freeze. Move existing webassembly/src code under legacy/.

## Why
See webassembly/docs/05_redevelopment_plan.md and phases/phase0_legacy_freeze.md.

## Scope
- File rename only (no logic changes)
- main.cpp/bind_function.cpp include path patch
- CMakeLists.txt path prefix update

## Verification
- [x] npm run build-wasm:debug 통과
- [x] npm run build-wasm:release 통과
- [x] webassembly/docs/02_menu_tree.md §5 체크리스트 18 항목 수동 통과

## Risks / Reviewer Hints
- git diff 의 99 % 가 'rename' 이어야 한다.
- main.cpp / bind_function.cpp / CMakeLists.txt 의 실제 textual change 만 정성 검토 대상.
```

## 6. 검증 매트릭스

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 0 | **§2a Pre-Phase 0 baseline** | Pre-Phase 0 PR 머지 후 main 에서 `npm run build-wasm:debug` + `:release` | 둘 다 exit 0 | Phase 0 진입 전 |
| 1 | `webassembly/src/` 직속 파일 | `ls webassembly/src` | `bind_function.cpp  legacy  main.cpp` 3 항목만 | 정적 |
| 2 | `legacy/` 안의 헤더 개수 | `find webassembly/src/legacy -name "*.h" \| wc -l` | 약 70 개 (현재 갯수와 동일) | 정적 |
| 3 | `legacy/` 안의 cpp 개수 | `find webassembly/src/legacy -name "*.cpp" \| wc -l` | 약 50 개 | 정적 |
| 4 | **git rename 인식** (격상) | `git add -A && git status --short \| grep "^R" \| wc -l` 또는 `git diff --cached --stat -M \| grep " => " \| wc -l` | **100 +** 개 — 첫 번째 시도에서 미검증된 항목. plain `mv` 우회를 사용했을 때 특히 중요 | 정적 (git add 직후) |
| 5 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 6 | Release 빌드 | `npm run build-wasm:release` | exit 0 (Pre-Phase 0 통과 했으면 자동 통과해야 함) | 동적 |
| 7 | 18 항목 체크리스트 | 수동 | 모두 ✓ | 동적 |
| 8 | `font_manager.cpp` SHA | `sha256sum webassembly/src/legacy/font_manager.cpp` 와 baseline 비교 | 동일 | 정적 |

> **검증 순서**: 0 → 1~3 → 4 → 5 → 6 → 7 → 8. 각 단계가 통과해야만 다음 단계로 넘어간다. 빌드 실패 시 §8 롤백 절차로 직행.

## 7. 리스크 / 완화책

| # | 리스크 | 영향 | 완화책 |
|---|---|---|---|
| 7.1 | `git mv` 중 충돌 | rename 인식 실패 → diff 폭발 | working tree 가 clean 한 상태에서 시작. 사전에 stash/commit |
| 7.2 | CRLF/LF 라인엔딩 차이 | rename 이 modify 로 잡힘 | `.gitattributes` 가 EOL 정책을 가지고 있는지 확인. 본 PR 에서는 `core.autocrlf` 설정을 변경하지 않는다 |
| 7.3 | `font_manager.cpp` 33,885 줄 자동생성 | rename 이 modify 로 오인 가능 | `git mv` 만 사용. 내용 한 글자도 수정 X. 검증 시 SHA 비교 |
| 7.4 | 상대 include 깨짐 | 빌드 실패 | Step 3.3 의 grep 으로 사전 점검. `#include "../"` 패턴 0 또는 모두 legacy/ 내부에서 해석 가능한 형태인지 확인 |
| 7.5 | CMake source list 업데이트 누락 | undefined symbol 링크 에러 | Step 4 의 sed/Python 일괄 처리. 결과 diff 를 한 번 더 검토 |
| 7.6 | 동시 작업자가 main 에 머지 | rebase 부담 | Phase 0 PR 은 가능한 한 빨리(1~2 시간 안에) 머지. 작업 중에는 main 의 webassembly/src 변경 freeze 요청 |
| 7.7 | 한국어 인코딩 깨진 주석이 git diff 노이즈를 만듦 | 검토 부담 | 본 PR 에서는 인코딩을 일절 건드리지 않는다. legacy 내 주석은 그대로 동결 |
| 7.8 | **release-only 잠복 버그** (예: SPDLOG 매크로 가드 안의 typo) | Phase 0 PR 에서 빌드 실패로 노출 → "코드 수정 0 줄" 미덕 손상 | **§2a Pre-Phase 0 PR 에서 사전 처리.** 첫 번째 시도(2026-04-28)에서 실제로 발생한 리스크 |
| 7.9 | **`.git/index.lock` 일관성 이슈** (Windows-Linux 바인드 마운트) | Linux 측에서 `git mv`/`git add`/`git commit` 실패. ls 는 lock 파일 부재라고 보고하지만 git 의 `O_CREAT\|O_EXCL` 은 즉시 "File exists" 로 실패 | (a) 파일 이동·include 패치·CMake 패치는 Linux 측에서 일괄 수행. (b) git add 이후 단계(rename 인식 확인, commit, push) 는 PowerShell 또는 Windows Git Bash 측에서 진행. (c) GUI git 도구가 lock 을 만들고 있는지 의심되면 잠시 종료. (d) lock 이 남으면 Windows 파일 탐색기에서 `.git\index.lock` 직접 삭제 |
| 7.10 | **Phase 0 와 무관한 노이즈** (예: 브라우저 콘솔 preload 경고) | 검증 도중 인지하면 작업 흐름 흐트러짐 | 발견 시 즉시 별도 issue/PR 로 분리. **Phase 0 PR 에 묶지 않는다.** Phase 0 의 미덕은 "diff 의 99 % 가 rename" — 노이즈가 끼면 그 미덕이 깨진다 |
| 7.11 | 18 항목 회귀 테스트의 사람-손 비용 | 수동 클릭 부담 → 머뭇거림 | 차후 단계로 e2e 테스트 자동화 검토 (Playwright dependencies 가 이미 있음). Phase 0 자체에서는 수동 18 항목으로 충분 |

## 8. 롤백 절차

빌드 또는 회귀 테스트가 실패하면 즉시 롤백한다.

```bash
# 변경 취소 (커밋 전)
git restore --staged .
git restore .
git clean -fd webassembly/src/legacy

# 또는 커밋 후라면
git reset --hard HEAD~1
```

> **롤백 후 빈 `legacy/` 디렉터리**: §5 Step 1 의 노트 참조. `git restore` 는 `mkdir` 로 만든 빈 폴더를 제거하지 않는다. 신경 쓰이면 Windows 파일 탐색기 또는 `rmdir` 로 수동 제거.

원인 진단:

1. 빌드 에러가 어떤 파일에서 발생했는가?
   - `error: 'X' was not declared` → include 경로 누락. main.cpp / bind_function.cpp 또는 legacy 내부 `#include "../"` 형태 점검.
   - `undefined reference to ...` → CMakeLists.txt 의 source list 누락. 누락된 .cpp 를 path-shift 했는지 확인.
   - `error: use of undeclared identifier 'AtomType'` 같은 release-only 노출 → §2a Pre-Phase 0 단계에서 미리 처리되었어야 함. 빠졌다면 Phase 0 를 중단하고 §2a 부터 다시.
2. 런타임에서만 실패한다면 — Phase 0 은 동작이 같아야 하므로 이론상 발생하지 않는다. 발생 시 `font_manager.cpp` 또는 리소스 경로(`resources/icon_image/...`) 가 변경되었는지 점검.

## 9. PR 체크리스트 (검토자 / 작성자 공용)

작성자 — PR 올리기 전:

- [ ] **§2a Pre-Phase 0 PR 이 먼저 머지되어 있는가** (debug + release 빌드 baseline 통과)
- [ ] working tree clean (이전 변경분 별도 처리)
- [ ] `ls webassembly/src/` = `bind_function.cpp  legacy  main.cpp`
- [ ] `git status --short` 의 R(rename) 항목이 100+ 개 (또는 plain `mv` 우회 시 git add 후 `git diff --cached --stat -M` 으로 rename 항목 100+ 확인)
- [ ] Debug + Release 빌드 통과
- [ ] 18 항목 회귀 체크리스트 통과
- [ ] 커밋 메시지에 본 문서와 평가서 링크 포함

검토자 — 머지 전:

- [ ] diff 의 비-rename 변경이 (a) `main.cpp` include 3 줄, (b) `bind_function.cpp` include 3 줄, (c) `CMakeLists.txt` 의 path prefix 한 종류로만 구성되어 있는가? (회색지대 build-fix 가 Phase 0 PR 안에 끼어 있다면 별도 commit 으로 분리되어 있는가?)
- [ ] `legacy/` 내부 파일은 한 글자도 변경되지 않았는가? (`git diff --stat -- webassembly/src/legacy` 의 모든 항목이 0 changed lines)
- [ ] CI 빌드 통과 + (수동) wasm 사이즈 회귀 없음
- [ ] 본인 환경에서도 18 항목 통과

## 10. 후속 단계 연결

Phase 0 가 머지되면 Phase 1 (`app/` + `core/` 빈 셸 부트스트랩) 의 입구가 열린다. Phase 1 의 첫 작업은 다음과 같다.

1. `webassembly/src/{app,core/{vtk,io,data,scene,render,ui},features}` 빈 폴더 신설.
2. 새 `app/app.cpp` 가 dockspace 만 띄우는 형태로 main.cpp 와 연결됨.
3. `bind_function.cpp` 가 새 `app::App` 의 정적 메서드만 노출하도록 점진 교체.

Phase 1 세부계획서는 `phases/phase1_app_core_bootstrap.md` 에 별도로 작성한다.

## 11. 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §3 Phase 0
- 첫 번째 시도 평가서 (본 보강의 근거): [`./phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md)
- 메뉴 트리 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md)
- 회귀 테스트 18 항목 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §5

---

## 부록 A — 한 줄 요약 명령 모음 (참고용, 그대로 실행 금지)

```bash
# 0. Pre-Phase 0 baseline 확보 (§2a)
cd /path/to/vtk-workbench
git checkout refactor/menu-aligned    # 또는 main
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release            # 실패 시 typo fix 후 별도 PR 머지

# 1. Phase 0 시작
git status   # working tree clean 확인 (또는 stash/commit 처리)

# 2. legacy/ 디렉터리 + 파일 이동 (git mv 우선, 환경 제약 시 plain mv)
cd webassembly/src
mkdir -p legacy
git mv app.cpp app.h custom_ui.cpp custom_ui.h file_loader.cpp file_loader.h \
       font_manager.cpp font_manager.h image.cpp image.h lcrs_tree.cpp lcrs_tree.h \
       mesh.cpp mesh.h mesh_detail.cpp mesh_detail.h \
       mesh_group.cpp mesh_group.h mesh_group_detail.cpp mesh_group_detail.h \
       mesh_manager.cpp mesh_manager.h model_tree.cpp model_tree.h \
       mouse_interactor_style.cpp mouse_interactor_style.h \
       test_window.cpp test_window.h texture.cpp texture.h \
       toolbar.cpp toolbar.h unv_reader.cpp unv_reader.h \
       vtk_viewer.cpp vtk_viewer.h \
       atoms_template_bravais_lattice.cpp atoms_template_periodic_table.cpp \
       legacy/
git mv atoms common enum icon macro config legacy/
git mv batchOnly.md refactory.md refactory3.md refactory4.md refactory5.md \
       refactory6_menu_aligned.md refactory_atom_manager.md \
       refactory_details.md refactory_details2.md imgui.ini \
       legacy/

# 3. main.cpp / bind_function.cpp 의 include 패치 (수동 또는 sed)
cd ../..
sed -i 's|#include "app\.h"|#include "legacy/app.h"|'                webassembly/src/main.cpp
sed -i 's|#include "font_manager\.h"|#include "legacy/font_manager.h"|' webassembly/src/main.cpp
sed -i 's|#include "mesh_manager\.h"|#include "legacy/mesh_manager.h"|' webassembly/src/main.cpp
sed -i 's|#include "app\.h"|#include "legacy/app.h"|'                webassembly/src/bind_function.cpp
sed -i 's|#include "file_loader\.h"|#include "legacy/file_loader.h"|' webassembly/src/bind_function.cpp
sed -i 's|#include "mesh_manager\.h"|#include "legacy/mesh_manager.h"|' webassembly/src/bind_function.cpp

# 4. CMakeLists.txt path prefix 갱신
python3 - <<'PY'
import re, pathlib
p = pathlib.Path("CMakeLists.txt")
keep = {"webassembly/src/main.cpp", "webassembly/src/bind_function.cpp"}
def repl(m):
    path = m.group(0)
    return path if path in keep else path.replace("webassembly/src/", "webassembly/src/legacy/", 1)
p.write_text(re.sub(r"webassembly/src/[A-Za-z0-9_./-]+", repl, p.read_text()))
PY

# 5. 빌드 확인
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release

# 6. 런타임 확인 후 커밋
git add -A
git status --short -uall   # rename 항목 100+ 인식 확인
git commit -m "Phase 0: Move webassembly/src/* to webassembly/src/legacy/"
```

## 부록 B — 사후 점검: 본 PR 머지 직후 main 트리 상태

```
webassembly/src/
├─ bind_function.cpp           ← include 만 legacy/ 로 변경됨
├─ main.cpp                    ← include 만 legacy/ 로 변경됨
└─ legacy/                     ← 현 코드 전체 동결본
   ├─ app.cpp / app.h
   ├─ vtk_viewer.cpp / vtk_viewer.h
   ├─ ... (이하 기존 모든 파일)
   ├─ atoms/
   ├─ common/
   ├─ enum/
   ├─ icon/
   ├─ macro/
   ├─ config/
   ├─ refactory*.md
   └─ imgui.ini
```

이 상태에서 Phase 1 이 시작될 수 있다.
