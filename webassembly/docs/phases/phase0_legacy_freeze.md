# Phase 0 — 현재 구조 동결 (Legacy Freeze) 세부계획서

> 상위 문서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §3 Phase 0
> 작성일: 2026-04-27
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR: 1 개
> 예상 소요: 1~2시간 (실제 코드 수정 없음, 파일 이동 + CMake 패치만)

## 0. 한 줄 요약

> `webassembly/src/legacy/` 폴더를 만들고, 현재 모든 `webassembly/src/*` 파일을 그 아래로 이동시킨 뒤, **빌드와 런타임 동작이 100 % 동일** 한 상태로 끝낸다.
> 이 단계의 핵심 가치는 *"새 코드를 어디에 쓸 빈 공간을 만든다"* 는 것이지, 코드 자체를 고치는 것이 아니다.

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `webassembly/src/legacy/` 트리 신설, (b) `webassembly/src/{main.cpp, bind_function.cpp}` 외 모든 `.cpp/.h/.md/.ini` 파일을 legacy/ 로 이동, (c) `CMakeLists.txt` 가 새 경로를 참조하도록 갱신, (d) `npm run build-wasm:debug` + `npm run build-wasm:release` 빌드 통과, (e) 런타임 동작이 변경 전과 비교해 1px 도 다르지 않음 |
| **비목표** | 코드 수정, 인터페이스 변경, 새 폴더(app/, core/, features/) 생성, 메뉴 동작 조정, Doxygen 주석 추가, `legacy/` 내부 정리 |

> **비목표가 어겨지면 PR 을 거부한다.** Phase 0 의 한 가지 미덕은 "검토자가 git diff 를 보고 'rename' 외에 의심할 게 없다" 가 되는 것.

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

## 3. 사전 준비 (체크리스트)

작업 시작 전에 다음을 확인한다.

- [ ] 현재 브랜치가 `refactor/menu-aligned` 인지 확인 — `git branch --show-current`
- [ ] working tree 의 unstaged 변경분이 본인 의도와 일치하는지 확인 (현재 `M` 마크 205개 + `??` 3개) — `git status --short | head`
- [ ] Phase 0 작업을 별도 commit 으로 기록할 수 있도록 *기존 변경분을 먼저 한 번 커밋* 또는 *stash 처리* 한다. 이동 작업이 기존 modified 파일과 섞이면 diff 가 폭발한다
- [ ] Phase 0 PR 단위가 너무 커지는 것을 막기 위해, 빌드/디스패치 검증용 emscripten 환경이 손에 잡힌 상태인지 확인 (Docker 또는 emsdk 4.0.3)

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

### Step 2 — 파일 이동 (`git mv` 사용)

> **반드시 `git mv` 를 사용한다.** 일반 `mv` 후 `git add` 를 하면 git 이 rename 을 인식하지 못해 diff 가 폭발한다.

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

| 검증 항목 | 명령 / 방법 | 기대값 |
|---|---|---|
| `webassembly/src/` 직속 파일 | `ls webassembly/src` | `bind_function.cpp  legacy  main.cpp` 3 항목만 |
| `legacy/` 안의 헤더 개수 | `find webassembly/src/legacy -name "*.h" \| wc -l` | 약 70 개 (현재 갯수와 동일) |
| `legacy/` 안의 cpp 개수 | `find webassembly/src/legacy -name "*.cpp" \| wc -l` | 약 50 개 |
| git rename 인식 | `git status --short \| grep "^R" \| wc -l` | 100+ 개 |
| Debug 빌드 | `npm run build-wasm:debug` | exit 0 |
| Release 빌드 | `npm run build-wasm:release` | exit 0 |
| 18 항목 체크리스트 | 수동 | 모두 ✓ |
| `font_manager.cpp` SHA | `sha256sum webassembly/src/legacy/font_manager.cpp` | main 브랜치의 `webassembly/src/font_manager.cpp` 와 동일 |

## 7. 리스크 / 완화책

| 리스크 | 영향 | 완화책 |
|---|---|---|
| `git mv` 중 충돌 | rename 인식 실패 → diff 폭발 | working tree 가 clean 한 상태에서 시작. 사전에 stash/commit |
| CRLF/LF 라인엔딩 차이 | rename 이 modify 로 잡힘 | `.gitattributes` 가 EOL 정책을 가지고 있는지 확인. 본 PR 에서는 `core.autocrlf` 설정을 변경하지 않는다 |
| `font_manager.cpp` 33,885 줄 자동생성 | rename 이 modify 로 오인 가능 | `git mv` 만 사용. 내용 한 글자도 수정 X. 검증 시 SHA 비교 |
| 상대 include 깨짐 | 빌드 실패 | Step 3.3 의 grep 으로 사전 점검. `#include "../"` 패턴 0 또는 모두 legacy/ 내부에서 해석 가능한 형태인지 확인 |
| CMake source list 업데이트 누락 | undefined symbol 링크 에러 | Step 4 의 sed 가 50 줄 일괄 처리. 결과 diff 를 한 번 더 검토 |
| 동시 작업자가 main 에 머지 | rebase 부담 | Phase 0 PR 은 가능한 한 빨리(1~2 시간 안에) 머지. 작업 중에는 main 의 webassembly/src 변경 freeze 요청 |
| 한국어 인코딩 깨진 주석이 git diff 노이즈를 만듦 | 검토 부담 | 본 PR 에서는 인코딩을 일절 건드리지 않는다. legacy 내 주석은 그대로 동결 |

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

원인 진단:
1. 빌드 에러가 어떤 파일에서 발생했는가?
   - `error: 'X' was not declared` → include 경로 누락. main.cpp / bind_function.cpp 또는 legacy 내부 `#include "../"` 형태 점검.
   - `undefined reference to ...` → CMakeLists.txt 의 source list 누락. 누락된 .cpp 를 path-shift 했는지 확인.
2. 런타임에서만 실패한다면 — Phase 0 은 동작이 같아야 하므로 이론상 발생하지 않는다. 발생 시 `font_manager.cpp` 또는 리소스 경로(`resources/icon_image/...`) 가 변경되었는지 점검.

## 9. PR 체크리스트 (검토자 / 작성자 공용)

작성자 — PR 올리기 전:
- [ ] working tree clean (이전 변경분 별도 처리)
- [ ] `ls webassembly/src/` = `bind_function.cpp  legacy  main.cpp`
- [ ] `git status --short` 의 R(rename) 항목이 100+ 개
- [ ] Debug + Release 빌드 통과
- [ ] 18 항목 회귀 체크리스트 통과
- [ ] 커밋 메시지에 본 문서 링크 포함

검토자 — 머지 전:
- [ ] diff 의 비-rename 변경이 (a) `main.cpp` include 3 줄, (b) `bind_function.cpp` include 3 줄, (c) `CMakeLists.txt` 의 path prefix 한 종류로만 구성되어 있는가?
- [ ] `legacy/` 내부 파일은 한 글자도 변경되지 않았는가? (`git diff --stat -- webassembly/src/legacy` 의 모든 항목이 `0 changed lines`)
- [ ] CI 빌드 통과 + (수동) wasm 사이즈 회귀 없음
- [ ] 본인 환경에서도 18 항목 통과

## 10. 후속 단계 연결

Phase 0 가 머지되면 Phase 1 (`app/` + `core/` 빈 셸 부트스트랩) 의 입구가 열린다. Phase 1 의 첫 작업은 다음과 같다.

1. `webassembly/src/{app,core/{vtk,io,data,scene,render,ui},features}` 빈 폴더 신설.
2. 새 `app/app.cpp` 가 dockspace 만 띄우는 형태로 main.cpp 와 연결됨.
3. `bind_function.cpp` 가 새 `app::App` 의 정적 메서드만 노출하도록 점진 교체.

Phase 1 세부계획서는 `phases/phase1_app_core_bootstrap.md` 에 별도로 작성한다.

---

## 부록 A — 한 줄 요약 명령 모음 (참고용, 그대로 실행 금지)

```bash
# 0. 사전 정리
cd /path/to/vtk-workbench
git checkout refactor/menu-aligned
git status   # working tree clean 확인 (또는 stash/commit 처리)

# 1. legacy/ 디렉터리 + 파일 이동
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

# 2. main.cpp / bind_function.cpp 의 include 패치 (수동 또는 sed)
cd ../..
sed -i 's|#include "app.h"|#include "legacy/app.h"|'                 webassembly/src/main.cpp
sed -i 's|#include "font_manager.h"|#include "legacy/font_manager.h"|' webassembly/src/main.cpp
sed -i 's|#include "mesh_manager.h"|#include "legacy/mesh_manager.h"|' webassembly/src/main.cpp
sed -i 's|#include "app.h"|#include "legacy/app.h"|'                 webassembly/src/bind_function.cpp
sed -i 's|#include "file_loader.h"|#include "legacy/file_loader.h"|'  webassembly/src/bind_function.cpp
sed -i 's|#include "mesh_manager.h"|#include "legacy/mesh_manager.h"|' webassembly/src/bind_function.cpp

# 3. CMakeLists.txt path prefix 갱신
python3 - <<'PY'
import re, pathlib
p = pathlib.Path("CMakeLists.txt")
keep = {"webassembly/src/main.cpp", "webassembly/src/bind_function.cpp"}
def repl(m):
    path = m.group(0)
    return path if path in keep else path.replace("webassembly/src/", "webassembly/src/legacy/", 1)
p.write_text(re.sub(r"webassembly/src/[A-Za-z0-9_./-]+", repl, p.read_text()))
PY

# 4. 빌드 확인
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release

# 5. 런타임 확인 후 커밋
git add -A
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
