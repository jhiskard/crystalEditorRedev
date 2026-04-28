# Phase 1 — app/ + core/ 빈 셸 부트스트랩 (App/Core Bootstrap) 세부계획서

> 상위 문서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §4 Phase 1
> 선행 문서: [`./phase0_legacy_freeze.md`](./phase0_legacy_freeze.md) (rev. 2026-04-28)
> 평가서:  [`./phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md)
> 작성일: 2026-04-28
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR: 1 개
> 예상 소요: 1~2 일 (Linux 측 코드 작성 + Windows 측 빌드 검증)

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-04-28 | 초안 작성 (Phase 0 통합 평가서의 §4.4 진입 후 첫 작업 항목을 본 문서로 확장) |

---

## 0. 한 줄 요약

> Phase 0 가 만들어 둔 `webassembly/src/legacy/` 동결본 위에, **`app/` + `core/` 의 최소 셸** 을 새로 작성해 *빈 dockspace 한 장만 떠 있는 상태* 로 빌드/실행을 끌어올린다.
> 본 단계의 핵심 가치는 *"새 코드를 둘 자리"* 가 실제로 빌드되어 돌아간다는 보증이지, 기능을 다시 살리는 것이 아니다.

> Phase 0 는 *"rename 만, 동작 동일"* 이었고, Phase 1 은 그 정반대다 — *"코드는 새로 짜고, 동작은 의도적으로 거의 0"*. 이 비대칭이 Phase 1 의 본질이다.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `webassembly/src/{app, core/{vtk,io,data,scene,render,ui}, features}` 빈 디렉터리 트리 신설. (b) `app/app.{cpp,h}` 가 ImGui dockspace 한 장을 띄우는 최소 셸을 제공. (c) `core/vtk/vtk_viewer.{cpp,h}` 가 빈 VTK 렌더 윈도우를 셋업. (d) `main.cpp` 와 `bind_function.cpp` 가 `legacy/` include 0 개 + `app::App` 만 참조하도록 점진 교체. (e) `CMakeLists.txt` 가 **`legacy/` 를 빌드에서 제외** 하고 새 트리만 빌드. (f) `npm run build-wasm:debug` 통과 + 브라우저에서 빈 dockspace 가 보임 |
| **비목표** | 메뉴 부활, VTK actor/렌더링 부활, 파일 로딩 부활, 18 항목 회귀 테스트 통과 (이는 Phase 3 진행 중 점진 회복), Embind 모든 export 부활, `features/` 안에 어떤 코드든 작성 (Phase 2~) |

> Phase 1 의 미덕: *"검토자가 git diff 를 보고 '새 폴더 + 새 파일 + CMake 변경 + main/bind 의 include 교체' 외에 의심할 게 없다"*. legacy/ 는 한 글자도 변경되지 않는다 (Phase 0 의 동결 원칙이 본 PR 에서도 유지됨).

### 1.1 비목표가 뜻하는 것 — 의도적인 기능 손실

Phase 1 직후의 사용자 경험:

| 기능 | Phase 0 후 | Phase 1 후 | 회복 단계 |
|---|---|---|---|
| 빈 dockspace 표시 | ✓ | ✓ | — |
| 메뉴바 | ✓ | (비어있음 또는 placeholder 1~2 항목) | Phase 4 (`menu_router`) |
| Crystal Viewer About 모달 | ✓ | ✗ | Phase 4 |
| File / Edit / Build / Measurement / Data / Utilities 메뉴 | ✓ | ✗ | Phase 3 |
| Settings / Windows / Layout 메뉴 | ✓ | ✗ | Phase 4 |
| VTK 렌더 (atoms/mesh/charge density) | ✓ | ✗ | Phase 3 |
| Toolbar | ✓ | ✗ | Phase 3.7 |
| Model Tree | ✓ | ✗ | Phase 3.8 |
| 파일 로딩 (XSF / CHGCAR / UNV) | ✓ | ✗ | Phase 3.6 |

→ Phase 1 PR 의 검토자에게 **이 기능 손실은 의도된 것** 임을 PR 본문에서 명확히 알려야 한다.

---

## 2. 전제 — Phase 0 완료 상태

본 계획서는 다음이 머지된 상태에서 시작한다.

- [ ] `webassembly/src/{bind_function.cpp, main.cpp, legacy/}` 3 항목으로 정리된 트리
- [ ] `legacy/atoms/atoms_template.cpp:3168` build-fix 적용 (Phase 0 의 회색지대 build-fix)
- [ ] `npm run build-wasm:debug` + `:release` 둘 다 exit 0
- [ ] 18 항목 메뉴 회귀 테스트 통과 (Phase 0 baseline)
- [ ] Phase 0 commit(s) 가 머지됨

위 통과 여부는 [`phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md) §4.3 의 3 가지 정리 항목으로 확인.

---

## 3. 디렉터리 트리 — Phase 1 종료 시

```
webassembly/src/
│
├─ main.cpp                ─── app::App 만 호출하는 최소 엔트리
├─ bind_function.cpp       ─── app::App 의 정적 메서드만 노출 (Embind 최소화)
│
├─ app/                    ─── 신규: 애플리케이션 셸
│   ├─ app.cpp             [GLFW + ImGui dockspace 부트스트랩 + 메인 루프]
│   ├─ app.h               [class App 선언]
│   └─ (placeholder 파일 없음 — menu_router/about/settings 는 Phase 4)
│
├─ core/                   ─── 신규: 둘 이상이 쓰는 기반 (Phase 1 에서는 vtk + render 만)
│   ├─ vtk/
│   │   ├─ vtk_viewer.cpp  [빈 VTK 윈도우 + 빈 vtkRenderer]
│   │   └─ vtk_viewer.h
│   ├─ render/
│   │   └─ font_manager.cpp/h    [legacy/font_manager.* 동일 데이터 — 자동생성 폰트, 본 단계에서는 그대로 가져옴]
│   ├─ io/                 [빈 폴더 — Phase 2 에서 채움]
│   ├─ data/               [빈 폴더 — Phase 2 에서 채움]
│   ├─ scene/              [빈 폴더 — Phase 2 에서 채움]
│   └─ ui/                 [빈 폴더 — Phase 2 에서 채움]
│
├─ features/               ─── 신규: 빈 폴더 (Phase 3 에서 채움)
│
└─ legacy/                 ─── Phase 0 에서 만든 동결본 — 본 PR 에서는 한 글자도 변경하지 않음
   └─ ... (그대로)
```

### 3.1 핵심 변화 한 줄 요약

| 구분 | Phase 0 후 | Phase 1 후 |
|---|---|---|
| `webassembly/src/` 직속 | `bind_function.cpp  legacy  main.cpp` (3) | `app  bind_function.cpp  core  features  legacy  main.cpp` (6) |
| 빌드 대상 source | `legacy/**` 전체 + main + bind | `app/** + core/**(최소) + main + bind` (legacy 제외, font_manager 만 예외) |
| 런타임 결과 | 18 항목 메뉴 모두 동작 | 빈 dockspace 한 장만 표시 |

---

## 4. 작업 절차 (단계별)

### Step 1 — 빈 디렉터리 트리 신설

```bash
cd webassembly/src
mkdir -p app
mkdir -p core/{vtk,io,data,scene,render,ui}
mkdir -p features
```

> `core/{io,data,scene,ui}` 와 `features/` 는 본 단계에서 빈 채로 둔다 (Phase 2~3 에서 채움). git 은 빈 폴더를 추적하지 않으므로, 본 단계에서 placeholder 파일 (`.gitkeep`) 을 둘지 여부는 §9.1 참조.

### Step 2 — `app/app.h`, `app/app.cpp` 작성

#### 2.1 `app/app.h`

```cpp
/**
 * @file app/app.h
 * @brief 애플리케이션 셸 — GLFW 윈도우 + ImGui dockspace 부트스트랩.
 *
 * @details
 *  Phase 1 에서는 빈 dockspace 한 장만 띄운다. 메뉴/툴바/렌더 컨텐츠는
 *  Phase 3~4 에서 점진 추가된다.
 */
#pragma once

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <string>

namespace app {

class App {
public:
    /// @brief 싱글턴 접근자.
    static App& Instance();

    /// @brief 애플리케이션 부트스트랩 (GLFW + GL + ImGui + dockspace).
    /// @return 0 이면 성공, 그 외는 실패 코드.
    int Init();

    /// @brief Emscripten main loop 콜백 한 프레임.
    void RenderFrame();

    /// @brief Embind 에서 노출되는 IDBFS 초기화 진입점.
    static void InitIdbfs();

    /// @brief Embind 에서 노출되는 ImGui ini 저장 진입점.
    static void SaveImGuiIniFile();

    /// @brief Embind 에서 노출되는 ImGui ini 로드 진입점.
    static void LoadImGuiIniFile();

    /// @brief HiDPI 보정용 디바이스 픽셀 비율.
    static float DevicePixelRatio();

private:
    App() = default;
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void renderDockSpace();   // Phase 1: 빈 dockspace 만 그림.
    bool m_Initialized = false;
};

} // namespace app
```

#### 2.2 `app/app.cpp` 골격 (~150~250 줄 예상)

`legacy/app.cpp` 의 `App::Init / Render / renderDockSpaceAndMenu` 에서 다음만 추출.

- GLFW 윈도우 생성 (`glfwCreateWindow`).
- OpenGL ES3 / WebGL2 컨텍스트 생성.
- ImGui + ImGuiBackend (`imgui_impl_glfw_init_for_opengl` + `imgui_impl_opengl3_init`).
- DockSpace builder (Layout 1 의 기본 분할만 — Layout 2/3/Reset 은 Phase 4).
- `emscripten_set_main_loop` 진입.
- 메뉴바: `if (ImGui::BeginMenuBar()) { ImGui::EndMenuBar(); }` 로 빈 채로 둠 (또는 placeholder `Crystal Viewer (rebuilding…)` 1 항목만).

> *주의*: legacy 의 GLFW/ImGui 초기화 코드를 *복사* 해서 가져오는 것은 의도다. 본 PR 에서는 reference 가 legacy 가 아니라 새 트리이지만, 코드 패턴은 검증된 legacy 의 것을 그대로 쓰는 것이 안전하다.

### Step 3 — `core/vtk/vtk_viewer.cpp/h` 최소 셸 작성

#### 3.1 `core/vtk/vtk_viewer.h`

```cpp
/**
 * @file core/vtk/vtk_viewer.h
 * @brief VTK 윈도우 + 렌더러 셀턴. Phase 1 에서는 빈 scene 만 가짐.
 */
#pragma once

#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

namespace core::vtk {

class VtkViewer {
public:
    static VtkViewer& Instance();

    /// @brief VTK 렌더 윈도우/렌더러/인터랙터를 생성하고 ImGui 윈도우에 장착한다.
    void Init();

    /// @brief 한 프레임 렌더 (ImGui::Begin/End 안에서 호출).
    void Render();

    /// @brief 윈도우 크기 변경 시 호출 (Emscripten resize 콜백 전달).
    void Resize(int w, int h);

    vtkRenderer*               GetRenderer() const;
    vtkRenderWindow*           GetRenderWindow() const;
    vtkRenderWindowInteractor* GetInteractor() const;

private:
    VtkViewer() = default;
    ~VtkViewer() = default;

    vtkSmartPointer<vtkRenderWindow>            m_renderWindow;
    vtkSmartPointer<vtkRenderer>                m_renderer;
    vtkSmartPointer<vtkRenderWindowInteractor>  m_interactor;
    bool m_Initialized = false;
};

} // namespace core::vtk
```

#### 3.2 `core/vtk/vtk_viewer.cpp` 골격

legacy/vtk_viewer.cpp 에서 다음만 추출.

- `vtkRenderWindow::New`, `vtkRenderer::New`, `vtkRenderWindowInteractor::New`.
- `m_renderer->SetBackground(...)` 으로 단일 배경색.
- `m_renderWindow->AddRenderer(m_renderer)`.
- ImGui::Begin("Viewer") + canvas 렌더 + ImGui::End.

> 메쉬/atom/charge density 관련 코드는 일절 가져오지 않는다. 빈 scene 으로 둔다.

### Step 4 — `core/render/font_manager.cpp/h` 가져오기

`legacy/font_manager.{cpp,h}` 는 자동생성된 폰트 데이터(33,885 줄)다. 본 단계에서는 두 가지 옵션 중 (a) 권장.

| 옵션 | 방법 | 장단 |
|---|---|---|
| (a) **physical copy (권장)** | `cp legacy/font_manager.{cpp,h} core/render/` | core/ 가 자체 완결됨. legacy/ 가 완전히 빌드에서 빠져도 폰트가 동작. 단점: 동일 데이터 2 부 보관 — repo 약 1.6 MB 증가 |
| (b) CMake target 으로 legacy/font_manager.cpp 만 예외 컴파일 | CMakeLists 에서 legacy/ 전체를 제외하되 `font_manager.cpp` 한 줄만 명시 추가 | repo 사이즈 동일. 단점: legacy/ 가 *부분적으로* 살아있어 "legacy 빌드 제외" 라는 원칙이 깨짐 |

본 계획서는 **(a) physical copy** 를 채택한다. Phase 5 에서 legacy/ 전체 삭제 시 자연스럽게 `core/render/font_manager.cpp` 한 부만 남는다.

```bash
cp webassembly/src/legacy/font_manager.cpp webassembly/src/core/render/
cp webassembly/src/legacy/font_manager.h   webassembly/src/core/render/
```

> 이 시점에 `core/render/font_manager.cpp` 와 `legacy/font_manager.cpp` 는 byte-identical 이다. SHA256 으로 확인 (Phase 0 §6 #8 의 baseline 과 동일).

### Step 5 — `main.cpp` 갱신

`webassembly/src/main.cpp` 의 include + 함수 호출을 다음으로 교체.

#### 5.1 변경 전 (Phase 0 후)

```cpp
#include "legacy/app.h"
#include "legacy/font_manager.h"
#include "legacy/mesh_manager.h"
// ... (GLFW / Emscripten)
int main(int argc, char* argv[]) {
    // legacy::App 의 init 흐름
    ...
}
```

#### 5.2 변경 후 (Phase 1)

```cpp
/**
 * @file main.cpp
 * @brief Emscripten WASM 엔트리. app::App 인스턴스를 부트스트랩하고 메인 루프를 시작한다.
 */
#include "app/app.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

int main(int argc, char* argv[]) {
    auto& a = app::App::Instance();
    if (a.Init() != 0) {
        return 1;
    }
    emscripten_set_main_loop([]() {
        app::App::Instance().RenderFrame();
    }, 0, true);
    return 0;
}
```

### Step 6 — `bind_function.cpp` 갱신

#### 6.1 변경 전 (Phase 0 후)

```cpp
#include "legacy/app.h"
#include "legacy/file_loader.h"
#include "legacy/mesh_manager.h"

EMSCRIPTEN_BINDINGS(Constant) {
    emscripten::function("initIdbfs",         &App::InitIdbfs);
    emscripten::function("saveImGuiIniFile",  &App::SaveImGuiIniFile);
    emscripten::function("loadImGuiIniFile",  &App::LoadImGuiIniFile);
    emscripten::function("loadArrayBuffer",   &FileLoader::LoadArrayBuffer);
    emscripten::function("loadChgcarFile",    &FileLoader::LoadChgcarFile);
    emscripten::function("handleXSFGridFile", &FileLoader::HandleXSFGridFile);
    emscripten::function("handleStructureFile", &FileLoader::HandleStructureFile);
    emscripten::function("writeChunk",        &FileLoader::WriteChunk);
    emscripten::function("closeFile",         &FileLoader::CloseFile);
    emscripten::function("processFileInBackground", &FileLoader::ProcessFileInBackground);
    emscripten::function("showProgressPopup",     &App::ShowProgressPopup);
    emscripten::function("setProgressPopupText",  &App::SetProgressPopupText);
#ifdef DEBUG_BUILD
    emscripten::function("printMeshTree",     &MeshManager::PrintMeshTree);
#endif
}
```

#### 6.2 변경 후 (Phase 1) — 최소 + JS 호환 stub

> **중요**: Next.js 페이지(`app/workbench/page.tsx`) 가 `loadArrayBuffer`, `handleStructureFile` 등을 호출할 수 있으므로 *이름이 사라지면 런타임 에러* 가 난다. Phase 1 에서는 (a) 호출 자체가 일어나지 않도록 JS 측에서 가드를 두거나, (b) 이름을 유지하되 no-op stub 으로 만든다.

본 계획서는 **(b) no-op stub** 을 채택한다 — Next.js 측을 건드리지 않고 wasm 측에서만 정리하는 편이 안전.

```cpp
/**
 * @file bind_function.cpp
 * @brief Embind 바인딩. Phase 1 에서는 IDBFS/ini 외의 모든 export 가 no-op stub.
 */
#include "app/app.h"
#include <emscripten/bind.h>

namespace {

// ---- Phase 1 stubs --------------------------------------------------------
// Next.js 측에서 이름을 호출할 수 있으므로 export 만 유지.
// Phase 3.6 (`features/file/`) 에서 진짜 구현으로 교체된다.
void stub_loadArrayBuffer(const std::string& /*name*/, uintptr_t /*ptr*/, size_t /*len*/) {}
void stub_loadChgcarFile(const std::string& /*path*/) {}
void stub_handleXSFGridFile(const std::string& /*name*/) {}
void stub_handleStructureFile(const std::string& /*name*/) {}
void stub_writeChunk(uintptr_t /*ptr*/, size_t /*len*/) {}
void stub_closeFile() {}
void stub_processFileInBackground(const std::string& /*name*/) {}
void stub_showProgressPopup(bool /*show*/) {}
void stub_setProgressPopupText(const std::string& /*text*/) {}

} // namespace

EMSCRIPTEN_BINDINGS(Constant) {
    emscripten::function("initIdbfs",                &app::App::InitIdbfs);
    emscripten::function("saveImGuiIniFile",         &app::App::SaveImGuiIniFile);
    emscripten::function("loadImGuiIniFile",         &app::App::LoadImGuiIniFile);

    // Phase 1 stubs — Phase 3.6 에서 실제 구현으로 교체
    emscripten::function("loadArrayBuffer",          &stub_loadArrayBuffer);
    emscripten::function("loadChgcarFile",           &stub_loadChgcarFile);
    emscripten::function("handleXSFGridFile",        &stub_handleXSFGridFile);
    emscripten::function("handleStructureFile",      &stub_handleStructureFile);
    emscripten::function("writeChunk",               &stub_writeChunk);
    emscripten::function("closeFile",                &stub_closeFile);
    emscripten::function("processFileInBackground",  &stub_processFileInBackground);
    emscripten::function("showProgressPopup",        &stub_showProgressPopup);
    emscripten::function("setProgressPopupText",     &stub_setProgressPopupText);
}
```

> Embind 함수 시그니처는 Next.js 의 호출부와 일치해야 한다. 정확한 시그니처는 `app/workbench/page.tsx` 의 호출 패턴을 grep 해서 확인한 뒤 stub 을 그에 맞게 조정.

### Step 7 — `CMakeLists.txt` 갱신

`add_executable(...)` 의 source list 를 새 트리만 가리키도록 교체.

```cmake
# ============================================================================
# Source list — Phase 1: legacy/ 전체 빌드 제외, app/ + core/ 최소 셸만
# ============================================================================
set(SOURCES_APP
    webassembly/src/app/app.cpp
    webassembly/src/app/app.h
)

set(SOURCES_CORE
    webassembly/src/core/vtk/vtk_viewer.cpp
    webassembly/src/core/vtk/vtk_viewer.h
    webassembly/src/core/render/font_manager.cpp
    webassembly/src/core/render/font_manager.h
)

# Phase 2~3 에서 늘어남
set(SOURCES_FEAT)

add_executable(${PROJECT_NAME}
    webassembly/src/main.cpp
    webassembly/src/bind_function.cpp
    ${SOURCES_APP}
    ${SOURCES_CORE}
    ${SOURCES_FEAT}
)

# legacy/ 디렉터리는 빌드 대상에서 명시적으로 제외 (안전벨트)
# 참고: 위 source list 에 legacy/* 가 한 줄도 없으므로 자동 제외되지만,
# CI 등에서 누군가 file(GLOB) 을 실수로 추가하더라도 본 줄이 차단한다.
get_target_property(_TARGET_SRCS ${PROJECT_NAME} SOURCES)
foreach(_src IN LISTS _TARGET_SRCS)
    if(_src MATCHES "webassembly/src/legacy/")
        message(FATAL_ERROR "Phase 1 violation: legacy/ source in target — ${_src}")
    endif()
endforeach()
```

> 위 `foreach` 안전벨트 블록은 Phase 5 에서 legacy/ 가 완전히 사라질 때 함께 제거한다.

`target_include_directories` 의 `webassembly/src` 항목은 그대로 둔다 (`#include "legacy/X.h"` 가 main/bind 에서 제거되었으므로 영향 없음). 단 새 코드의 `#include "app/app.h"` 형태가 해석되도록 `webassembly/src` 가 include path 에 있어야 한다 — 이미 있으므로 추가 변경 불요.

### Step 8 — 정적 검증

```bash
cd /path/to/vtk-workbench

# 디렉터리 트리 점검
ls webassembly/src/         # 기대: app  bind_function.cpp  core  features  legacy  main.cpp
ls webassembly/src/app/     # 기대: app.cpp  app.h
ls webassembly/src/core/    # 기대: data  io  render  scene  ui  vtk
ls webassembly/src/features # 기대: (비어있거나 .gitkeep)

# legacy 의존 0 확인 (main.cpp / bind_function.cpp)
grep -nE '#include "legacy/' webassembly/src/main.cpp webassembly/src/bind_function.cpp || echo "OK — legacy include 0"

# CMake 빌드 안전벨트 확인
grep -A3 "Phase 1 violation" CMakeLists.txt
```

기대 결과: legacy include 0, 안전벨트 블록 존재.

### Step 9 — 빌드 검증 (Windows 측)

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run rm-wasm
npm run build-wasm:debug
```

기대:
- emcc 가 `app/`, `core/vtk/`, `core/render/` 의 4 개 .cpp 를 컴파일.
- 링크 시 `vtkRenderingOpenGL2`, `vtkRenderingVolumeOpenGL2` 등 VTK 라이브러리 의존은 그대로 통과.
- `legacy/` 안의 파일은 한 개도 컴파일되지 않음 (verbose ninja 로그에서 "legacy/" 경로가 안 보여야 정상).

> Phase 1 PR 에서는 release 빌드도 통과해야 한다 (`npm run build-wasm:release`). 하지만 본 단계의 코드량이 작아 release-only 잠복 버그가 새로 추가될 가능성은 매우 낮다.

### Step 10 — 런타임 검증 (Windows 측)

```powershell
npm run dev
# 브라우저 → http://localhost:3000/workbench
```

기대:
- 빈 dockspace 한 장이 표시됨.
- 메뉴바는 비어있거나 placeholder (`Crystal Viewer (rebuilding…)` 정도) 1 항목만.
- 콘솔에 wasm 로딩 성공 로그가 보임.
- File 다이얼로그 / About 모달 / Edit 윈도우 등은 일절 안 보임 (의도된 결과).
- 콘솔에서 Embind stub 함수가 호출되어도 에러 없음 (no-op).

### Step 11 — 커밋 & PR

```powershell
git add -A
git status --short
git commit -m "Phase 1: bootstrap empty app/+core/ shell, exclude legacy/ from build

- Add webassembly/src/{app, core/{vtk,io,data,scene,render,ui}, features}.
- app/app.{cpp,h}: minimal GLFW+ImGui dockspace (no menus, no widgets).
- core/vtk/vtk_viewer.{cpp,h}: empty VTK render window.
- core/render/font_manager.{cpp,h}: copied from legacy/font_manager.* (byte-identical).
- main.cpp: include only app/app.h, call app::App.
- bind_function.cpp: keep export names but stub all non-IDBFS bindings as no-op.
- CMakeLists.txt: rebuild source list to compile only the new tree;
  add a safety belt that fails the build if any legacy/* source slips in.

Runtime: empty dockspace renders. Menus/features intentionally absent.
They will be progressively restored from legacy/ in Phases 3~4.

Reference:
  - webassembly/docs/05_redevelopment_plan.md (Phase 1)
  - webassembly/docs/phases/phase1_app_core_bootstrap.md
"
```

PR 본문 템플릿:

```markdown
## What
Phase 1 — bootstrap minimal app/+core/ shell. Build now compiles the new tree
only (legacy/ excluded). Runtime shows an empty ImGui dockspace.

## Why
See webassembly/docs/05_redevelopment_plan.md and
webassembly/docs/phases/phase1_app_core_bootstrap.md.

## Scope
- New folders + initial files in `app/` and `core/{vtk,render}/`.
- `core/render/font_manager.{cpp,h}` is a byte-identical copy of
  `legacy/font_manager.{cpp,h}` (auto-generated font data).
- `main.cpp` and `bind_function.cpp` no longer include `legacy/*`.
- `CMakeLists.txt` rewritten to compile only the new tree.

## What you will see at runtime
An empty ImGui dockspace. **No menus, no file loading, no rendering.**
This is intentional. Features will be progressively restored from `legacy/`
in Phases 3 and 4.

## Verification
- [x] npm run build-wasm:debug 통과
- [x] npm run build-wasm:release 통과
- [x] 빈 dockspace 가 브라우저에 표시됨
- [x] grep "legacy/" 결과 0 (main.cpp / bind_function.cpp)

## Risks / Reviewer Hints
- legacy/ 내 파일은 한 글자도 변경되지 않아야 한다.
  `git diff --stat -- webassembly/src/legacy` 의 모든 항목이 0 changed lines.
- core/render/font_manager.{cpp,h} 의 SHA256 이 legacy 의 그것과 동일한지
  확인하면 자동생성 데이터가 깨끗하게 옮겨졌는지 보증된다.
```

---

## 5. 검증 매트릭스

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `webassembly/src/` 직속 항목 | `ls webassembly/src` | `app  bind_function.cpp  core  features  legacy  main.cpp` (6) | 정적 |
| 2 | `core/` 하위 폴더 6 개 | `ls webassembly/src/core` | `data io render scene ui vtk` | 정적 |
| 3 | `app/` 의 파일 | `ls webassembly/src/app` | `app.cpp app.h` | 정적 |
| 4 | `core/vtk/` 의 파일 | `ls webassembly/src/core/vtk` | `vtk_viewer.cpp vtk_viewer.h` | 정적 |
| 5 | `core/render/` 의 파일 | `ls webassembly/src/core/render` | `font_manager.cpp font_manager.h` | 정적 |
| 6 | main / bind 의 legacy include 0 | `grep -nE '#include "legacy/' webassembly/src/main.cpp webassembly/src/bind_function.cpp` | 0 hit | 정적 |
| 7 | CMake 빌드 안전벨트 존재 | `grep "Phase 1 violation" CMakeLists.txt` | 1 hit | 정적 |
| 8 | font_manager 동일성 | `sha256sum webassembly/src/legacy/font_manager.cpp webassembly/src/core/render/font_manager.cpp` | 두 SHA 동일 | 정적 |
| 9 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 10 | Release 빌드 | `npm run build-wasm:release` | exit 0 | 동적 |
| 11 | 빈 dockspace 표시 | `npm run dev` 후 브라우저 | dockspace 한 장 표시, 콘솔 에러 0 | 동적 |
| 12 | Embind stub no-op | 브라우저 콘솔에서 `Module.handleStructureFile("test")` 호출 | 에러 없이 즉시 반환 | 동적 |
| 13 | legacy/ 미컴파일 | ninja 빌드 로그 검색 | "webassembly/src/legacy/" 가 한 번도 안 보여야 정상 | 동적 |

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | GLFW/ImGui 부트스트랩 코드의 사소한 차이가 dockspace 가 안 뜨는 회귀를 일으킴 | 빈 화면조차 안 나옴 | legacy/app.cpp 의 init 흐름을 *복사* 해서 사용. 한 줄씩 비교하며 옮긴다 |
| 6.2 | `core/render/font_manager.cpp` 가 legacy 와 byte-identical 이 아님 | 폰트 깨짐 또는 컴파일 실패 | Step 4 에서 `cp` 사용. SHA256 비교를 §5 #8 에서 검증 |
| 6.3 | Embind stub 의 시그니처가 Next.js 호출부와 어긋남 | 런타임 TypeError | `app/workbench/page.tsx` 의 호출 패턴을 grep 으로 파악한 뒤 stub 시그니처를 일치시킴 |
| 6.4 | Next.js 가 처음 로드 시 자동으로 wasm 함수 호출 → stub 이 의도하지 않게 트리거 | 콘솔에 stub 호출 로그가 보임 | stub 안에서 `SPDLOG_DEBUG("phase1 stub: ...")` 한 줄로 가시화. 디버깅 부담 X |
| 6.5 | ImGui dockspace 구성이 layout-builder 호출 순서에 민감 | 분할이 깨지거나 윈도우가 사라짐 | Phase 1 에서는 builder 호출을 단순화 (Layout 1 의 기본 분할 1 회만) |
| 6.6 | `.git/index.lock` 환경 제약 (Phase 0 §7.9 참조) | Linux 측에서 git mv/add/commit 실패 | Linux 측은 파일 작성/이동만, git 작업은 PowerShell 측에서 |
| 6.7 | font_manager 자동생성 데이터가 너무 커서 git diff 폭발 | 검토 부담 | PR 본문에 *"font_manager 는 legacy 와 byte-identical copy — diff 는 새 경로 1 개뿐"* 명시. SHA 첨부 |
| 6.8 | VTK 라이브러리 링크 실패 (legacy 가 끌어오던 vtk module 이 사라져서) | 링크 에러 | CMakeLists 의 `find_package(VTK COMPONENTS ...)` 와 `target_link_libraries` 는 그대로 둠. Phase 1 의 core/vtk/vtk_viewer 가 이들을 동일하게 사용 |
| 6.9 | 빈 dockspace 만 떠 있는 PR 을 본 검토자가 *"기능이 다 사라졌다"* 며 거절 | 머지 지연 | PR 본문 §1.1 의 *"의도적 기능 손실 표"* 를 그대로 인용. 메뉴/툴바/렌더 회복 단계(Phase 3~4) 를 명시 |

---

## 7. 롤백 절차

```bash
# 변경 취소 (커밋 전)
git restore --staged .
git restore .
git clean -fd webassembly/src/{app,core,features}

# 또는 커밋 후라면
git reset --hard HEAD~1
```

> 롤백 시 `webassembly/src/{app,core,features}` 가 빈 디렉터리로 남을 수 있다. `rmdir` 로 수동 정리 권장.

원인 진단:

1. 빌드 에러
   - `error: 'app' was not declared` → main.cpp 가 `#include "app/app.h"` 를 가지고 있는지 확인.
   - `undefined reference to 'app::App::Init'` → CMakeLists 의 SOURCES_APP 에 `app.cpp` 가 빠졌는지 확인.
   - `legacy/...` 가 빌드 로그에 보임 → 안전벨트가 걸렸을 것. CMakeLists 의 source list 재점검.
   - VTK 미해결 심볼 → `target_link_libraries` 의 VTK_LIBRARIES 매크로가 살아있는지 확인.
2. 런타임 에러
   - 빈 화면도 안 보임 (브라우저 흰 화면) → `app::App::Init` 의 GLFW/GL 초기화 분기 점검.
   - 콘솔 TypeError (`Module.X is not a function`) → bind_function.cpp 의 stub 누락. 누락된 이름을 추가.
   - Phase 0 와 동일한 18 항목이 동작 → CMakeLists 갱신 누락 (legacy 가 여전히 빌드되고 있음). source list 재점검.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] §2 의 Phase 0 전제 모두 충족 (Phase 0 PR 머지 완료)
- [ ] §5 검증 매트릭스 13 항목 모두 통과
- [ ] `git diff --stat -- webassembly/src/legacy` 의 모든 항목이 0 changed lines
- [ ] PR 본문에 §1.1 *"의도적 기능 손실 표"* 인용
- [ ] PR 본문에 본 문서와 상위 계획 링크 포함

검토자 — 머지 전:

- [ ] diff 가 (a) `app/`, `core/{vtk,render}/` 신규 파일, (b) `main.cpp` / `bind_function.cpp` 의 include 교체, (c) `CMakeLists.txt` 의 source list 재작성, (d) `core/render/font_manager.{cpp,h}` 복사 4 가지로만 구성되어 있는가?
- [ ] `legacy/` 파일은 한 글자도 변경되지 않았는가?
- [ ] CI 빌드 (debug + release) 통과
- [ ] 빈 dockspace 가 본인 환경에서도 표시
- [ ] `core/render/font_manager.cpp` 의 SHA 가 `legacy/font_manager.cpp` 와 동일

---

## 9. 부록

### 9.1 빈 폴더 처리 (`.gitkeep`)

`core/{io,data,scene,ui}` 와 `features/` 는 본 단계에서 빈 채로 둔다. git 은 빈 디렉터리를 추적하지 않으므로, 다음 두 옵션 중 선택.

| 옵션 | 방법 | 비고 |
|---|---|---|
| (a) 그대로 빈 폴더 (권장) | 별도 처리 없음 | Phase 2 에서 첫 파일 추가 시 자연 추적 시작. `.gitkeep` 의존하지 않음 |
| (b) `.gitkeep` placeholder | 각 폴더에 `touch .gitkeep` | 빈 폴더 자체가 git history 에 명시. Phase 2 에서 `.gitkeep` 제거 필요 |

본 계획서는 (a) 채택. Phase 1 PR 의 diff 에는 `core/{io,data,scene,ui}` 와 `features/` 가 등장하지 않는다. 그게 자연스럽다.

### 9.2 placeholder 메뉴 항목 (선택)

빈 메뉴바가 사용자에게 *"개발 중"* 임을 알리도록 다음 1 항목을 두는 것을 권장.

```cpp
if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("  Crystal Viewer (rebuilding…)")) {
        ImGui::Text("Phase 1: app/+core/ bootstrap");
        ImGui::Text("See webassembly/docs/05_redevelopment_plan.md");
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}
```

### 9.3 사후 점검: Phase 1 머지 직후 트리

```
webassembly/src/
├─ main.cpp                    ← #include "app/app.h"
├─ bind_function.cpp           ← #include "app/app.h" + stub 9 종
├─ app/
│  ├─ app.cpp / app.h          ← 빈 dockspace 셸
├─ core/
│  ├─ vtk/
│  │  └─ vtk_viewer.cpp / vtk_viewer.h  ← 빈 VTK scene
│  ├─ render/
│  │  └─ font_manager.cpp / font_manager.h  ← legacy 와 byte-identical
│  ├─ io/   (빈 폴더 — Phase 2 에서 채움)
│  ├─ data/ (빈 폴더 — Phase 2)
│  ├─ scene/(빈 폴더 — Phase 2)
│  └─ ui/   (빈 폴더 — Phase 2)
├─ features/                   ← 빈 폴더 — Phase 3 에서 채움
└─ legacy/                     ← Phase 0 동결본 (한 글자도 변경 X)
```

---

## 10. 후속 단계 연결 — Phase 2

Phase 1 가 머지되면 Phase 2 (`core/scene/SceneState` + `core/io/format_registry` 골격) 의 입구가 열린다. Phase 2 의 첫 작업은:

1. `core/scene/scene_state.{cpp,h}` 작성 — `legacy/atoms/atoms_template.h` 의 god 부분(currentStructureId, structures, selection, hover)을 추출.
2. `core/scene/{structure_registry, selection, hover, events}.{cpp,h}` 작성.
3. `core/io/file_dialog.{cpp,h}` — `legacy/file_loader.cpp` 의 Emscripten 다이얼로그/청크 전송 wrapper.
4. `core/io/format_registry.{cpp,h}` — 확장자 → 파서 함수 포인터 매핑.
5. `core/io/{xsf,chgcar,rho}_parser.{cpp,h}` — legacy 의 같은 코드 그대로 이식.
6. `core/data/{element_database, colormap, lcrs_tree, string_utils, color}` — legacy 의 같은 파일 그대로 이식 (단, namespace 정리).
7. `core/ui/widgets.{cpp,h}` 등 공유 UI helper.
8. `core/vtk/{mouse_interactor, batch_update_system}` — legacy 에서 이식.

Phase 2 검증: 빌드는 통과하지만 런타임 동작은 Phase 1 과 동일 (빈 dockspace 유지). Phase 2 는 *기능 회복이 아니라 인프라 골격 완성* 단계.

Phase 2 세부계획서는 `phase2_core_skeleton.md` 에 별도로 작성한다.

---

## 11. 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §4 Phase 1 / §5 Phase 2
- 선행 계획: [`./phase0_legacy_freeze.md`](./phase0_legacy_freeze.md) (rev. 2026-04-28)
- Phase 0 평가서: [`./phase0_evaluation_2026-04-28.md`](./phase0_evaluation_2026-04-28.md)
- 새 아키텍처 (`app/`, `core/`, `features/` 의 의미): [`../03_target_architecture.md`](../03_target_architecture.md) §2~§3
- Doxygen 주석 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
