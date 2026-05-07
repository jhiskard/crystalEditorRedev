# Phase 3.2 — Data/Charge Density + Slice 이식 (Second Feature) 세부계획서

> 상위 문서:    [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.2)
> 선행 문서:    [`./phase3_1_utilities_brillouin_zone.md`](./phase3_1_utilities_brillouin_zone.md)
> 선행 평가서:  [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
> 메뉴 매핑:    [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §7 Data
> 작성일:      2026-04-29
> 최근 보강:   2026-04-29 (상위 §6.0 의 *legacy UI 작동방식 1:1 보존* 지침 신설에 따라 §1.4 추가)
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR:     1 개 (단일) 또는 2 개 (PR1 charge_density + PR2 slice)
> 예상 소요:   4~5 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-04-29 | 초안 작성 (Phase 3.1 평가서 §6 권장 다음 단계 항목을 본 문서로 확장) |
| 2026-04-29 (보강) | 상위 §6.0.1 *legacy UI 1:1 보존 원칙* 신설을 반영하여 §1.4 추가, §5 검증 매트릭스에 UI 보존 항목 격상, §8 PR 체크리스트에 스크린샷+사용자 시나리오 검증 추가, §6 리스크에 UI 보존 위반 가능성 추가 |

---

## 0. 한 줄 요약

> Phase 3.1 가 확립한 *features/ 4 layer 패턴* 을 그대로 복제하여 **Data 메뉴의 4 항목 (Isosurface / Surface / Volumetric / Plane)** 을 두 sub-folder (`features/data/charge_density/` + `features/data/slice/`) 로 이식한다. 본 단계는 또한 **`core/io/format_registry::Register(...)` 의 첫 사용자** 가 되어 Phase 2 인프라를 처음으로 검증한다.
> Phase 3.1 의 Voro++ 완전 이식 + 코드 압축 (라인수 53%) 패턴을 그대로 적용. legacy 4,485 줄 → 약 2,500~3,000 줄 예상.
> **단** 상위 §6.0.1 의 *legacy UI 작동방식 1:1 보존 원칙* 에 따라 ChargeDensity Viewer 윈도우의 위젯 배치/모드 토글/슬라이더 응답 패턴은 그대로 유지한다 (§1.4 참조).

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `features/data/{charge_density, slice}/` 두 sub-folder 신설 + 약 13 파일 작성. (b) 메뉴 `Data / Isosurface / Surface / Volumetric` 클릭 시 ChargeDensityViewer 윈도우 표시 + 모드(Isosurface/Surface/Volumetric) 전환. (c) 메뉴 `Data / Plane` 클릭 시 2D Slice Viewer 윈도우 표시. (d) `core/io/format_registry::Register(...)` 의 첫 호출자 — chgcar 파서 등록. (e) `npm run build-wasm:debug` + `:release` 통과 + 런타임에서 4 메뉴 항목 동작. (f) **legacy 의 ChargeDensity Viewer UI 동작이 1:1 보존됨** (§1.4) |
| **비목표** | 다른 메뉴 항목 부활 (Phase 3.3 이후 진행), `menu_router` 정식 도입 (Phase 4), `WindowFlags` 통합 (Phase 4), CHGCAR 파일 실제 로딩 (`features/file` 가 들어와야 — Phase 3.6), `vtk_renderer.cpp` 1,792 줄의 atom/bond/cell 분할 (Phase 3.4 의 영역), Volumetric 의 vtk_renderer 통합 (Phase 3.4 후 보완), 18 항목 회귀 전체 통과, legacy/ 내부 코드 수정 (회색지대 §1.2~§1.3 예외), **legacy UI 의 *재설계* — 위젯 위치 변경 / 새 옵션 추가 / 응답 패턴 수정 등은 모두 비목표** |

> Phase 3.2 의 미덕: *"검토자가 git diff 를 보고 `features/data/{charge_density, slice}/` 의 신규 13 파일 + `app/app.cpp` 의 메뉴 hook 1 줄 + `CMakeLists.txt` 의 source 추가 외에 의심할 게 없고, side-by-side 스크린샷에서 legacy 와 새 트리의 ChargeDensity Viewer 가 시각적으로 동일하다"*.

### 1.1 Phase 3.2 의 *두 번째 feature* 의의 — Phase 2 인프라 검증

본 단계는 **Phase 2 가 만든 `core/io/format_registry`, `core/data/colormap`, `core/scene/SceneState` 의 첫 실사용자**가 된다. Phase 3.1 (BZ) 은 SceneState 만 사용했지만, Phase 3.2 는:

| Phase 2 인프라 | Phase 3.2 의 첫 사용 |
|---|---|
| `core/io/format_registry::Register` | charge_density 가 ".chgcar" / 파일명 패턴 등록 |
| `core/io/chgcar_parser` | charge_density 의 도메인이 직접 사용 (Phase 3.6 의 features/file 도착 전까지는 *간접 검증*) |
| `core/data/colormap` | charge_density_renderer + slice_renderer 가 사용 (rainbow/viridis 등) |
| `core/scene/SceneState::structures` | controller 가 currentStructureId 와 charge density 연결 |
| `core/scene/EventBus::onStructureRemoved` | controller 가 구독 → charge density 자동 청산 |

### 1.2 회색지대 정책 인계 (Phase 0 §1.1 + Phase 1 §1.2 + Phase 3.1 §1.3)

| 회색지대 출처 | 적용 범위 (Phase 3.2) |
|---|---|
| Phase 0 §1.1 — typo build-fix | legacy/ 내부의 1~5 줄 typo |
| Phase 1 §1.2 — 빌드 호환성 shim | 새 트리 측의 얇은 redirect 파일 |
| Phase 3.1 §1.3 — 임시 메뉴 hook | `app/app.cpp::renderDockSpace` 에 `features::data::DrawMenu(ctx)` 추가 |

### 1.3 라인수 압축 정책 (Phase 3.1 §1.3 의 학습 적용)

Phase 3.1 평가서에서 입증된 패턴: *legacy 의 한국어 깨진 인코딩 주석 + 디버그 print + 미사용 메서드 + 외부 클래스 forward + 호출 단순화* 가 자연스럽게 라인수 53% 압축으로 이어진다.

**보존 필수 (압축 시에도 변경 금지)**:

- charge_density.cpp 의 pyrho 포팅 핵심 알고리즘 (`fromFile` 등)
- charge_density_renderer.cpp 의 `vtkContourFilter`, `vtkVolume`, `vtkColorTransferFunction` 호출
- slice_renderer.cpp 의 `vtkCutter`, `vtkPlane` 호출
- chgcar_parser 호출 (도메인이 `core::io::ParseChgcarFile` 사용)
- **§1.4 의 모든 UI 보존 항목** (위젯 인자, 콤보 항목 순서, 응답 패턴 등)

### 1.4 legacy UI 작동방식 1:1 보존 (상위 §6.0.1 의 본 단계 적용) **[신설]**

상위 계획서 [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6.0.1 의 **legacy UI 작동방식 1:1 보존 원칙** 이 본 단계에 그대로 적용된다.

#### 1.4.1 본 단계의 보존 대상 — Charge Density Viewer 윈도우

`legacy/atoms/ui/charge_density_ui.cpp` (2,519 줄) 가 정의하는 다음 UI 동작은 새 트리의 `features/data/charge_density/charge_density_ui.cpp` 에서 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| 모드 토글 | 3 개 모드 라디오/콤보 (Isosurface / Surface / Volumetric) 의 라벨 / 순서 / 기본 선택 |
| Isosurface 옵션 | isovalue 슬라이더 범위 (`-max(\|cd\|) ~ +max(\|cd\|)`) / 기본값 / 슬라이더 format / wireframe 토글 / 양/음 모드 별도 표시 정책 |
| Volumetric 옵션 | colormap 드롭다운 (Rainbow/Viridis/Plasma 등) 의 항목 순서 / opacity transfer function 의 기본 키포인트 |
| Advanced grid | "Advanced" 토글 → mesh 별 visibility 컬럼이 펼쳐지는 동작 |
| Animation | "Animate" 체크박스 → isovalue 가 시간에 따라 oscillate 하는 동작 + 속도 슬라이더 |
| 정보 패널 | grid dimensions / lattice / value range 의 표시 형식 (소수점 자릿수 포함) |
| 에러 메시지 | "No data loaded" / "Volume rendering not supported" 등의 텍스트 |
| 윈도우 크기·dock 정책 | legacy 가 default 로 설정한 윈도우 크기 + dockspace 위치 |

#### 1.4.2 본 단계의 보존 대상 — 2D Slice Viewer 윈도우

legacy 의 `slice_renderer.h` 22 줄 stub 만 존재하므로 *완전한 legacy 기준* 은 없다. 단 stub 의 인터페이스 (SlicePlane enum 의 XY/XZ/YZ/Custom, ColorMapType, value range) 가 제시한 *의도된 UI 형태* 를 본 단계에서 신규 구현. 향후 사용자 워크플로우와 충돌 없도록 다음 기본을 채택.

| 카테고리 | 채택 |
|---|---|
| 평면 선택 | 라디오 버튼 4 개: XY / XZ / YZ / Custom |
| Position 슬라이더 | XY/XZ/YZ 모드에서만 활성, Custom 모드에서는 비활성 (grey-out) |
| Custom plane | normal 3 컴포넌트 + origin 3 컴포넌트 입력 |
| Colormap | charge_density viewer 와 *동일한 드롭다운* 사용 |
| Value range | min/max 슬라이더 — *charge density 의 실제 데이터 범위* 를 default 로 |
| Color bar | 윈도우 우측에 ScalarBar 표시 |

#### 1.4.3 보존 검증 절차 (상위 §6.0.2 의 본 단계 적용)

PR 작성자는 본 PR 본문에 다음 3 종 검증을 첨부:

1. **Side-by-side 스크린샷** — legacy ChargeDensity Viewer (Phase 0 baseline) vs 새 트리의 같은 윈도우. 위젯 배치/텍스트/색상 동일성 비교.
2. **사용자 시나리오 정합성 테스트** — 다음 3 시나리오를 legacy 와 새 트리에서 차례로 실행해 동일 결과 확인:
   - (S1) CHGCAR 파일 로드 후 Isosurface 모드 → isovalue=0.05 → "Apply" 클릭 → 등치면 액터 생성 확인
   - (S2) Volumetric 모드 → colormap=Rainbow → opacity transfer function 기본값 확인
   - (S3) Slice → XY 평면, position=0.5 → colorbar 표시 확인
3. **Intentional UI deviation 사유서** — 의도적인 UI 변경이 발생했다면 PR 본문 `Intentional UI deviation` 섹션에 사유 명시. 변경이 없으면 *"None"* 명기.

#### 1.4.4 압축과 UI 보존의 조화 (예시)

| 압축 가능 ✓ | 보존 필수 ✗ (변경 금지) |
|---|---|
| `// 이 함수는 isovalue 를 계산해서...` 주석 → Doxygen 으로 정리 | `ImGui::SliderFloat("isovalue", &iso, -1, 1, "%.4f", ImGuiSliderFlags_Logarithmic)` 의 *모든 인자* (라벨, 범위, format, flags) |
| `m_parent->RenderChargeDensityXxx()` → `controller_.RenderXxx()` | `ImGui::Combo("Mode", &modeIdx, "Isosurface\0Surface\0Volumetric\0")` 의 항목 순서와 텍스트 |
| `printf("DEBUG: ...")` 디버그 출력 제거 | "Animate" 토글 시 매 프레임 isovalue 가 *sin(t)* 로 변하는 응답 패턴 |
| `if (foo) { ... } else if (foo) { ... }` 같은 미사용 분기 | 모드 전환 시 *기존 액터 자동 청산 → 새 액터 추가* 의 sequence |

→ **압축 ≠ UI 변경**. UI 동작은 보존하되, 그 주위의 *코드 정돈* 만 자유롭게 한다.

---

## 2. 전제 — Phase 3.1 완료 상태

본 계획서는 다음이 충족된 상태에서 시작한다.

- [ ] Phase 3.1 commit 머지 (Phase 3.2 PR 의 base commit)
- [ ] `webassembly/src/features/utilities/brillouin_zone/` 11 파일 + 임시 메뉴 hook 정상 동작
- [ ] `core/{scene, io, data, vtk, render, ui}` 인프라가 빌드 가능 상태
- [ ] `npm run build-wasm:debug` + `:release` 둘 다 exit 0
- [ ] Utilities / Brillouin Zone 메뉴 → 윈도우 표시 정상 (Phase 3.1 의 §5 #12-#17)

---

## 3. legacy 참조 인벤토리

| legacy 파일 | 라인 수 | 역할 | 새 트리 행선지 |
|---|---|---|---|
| `legacy/atoms/domain/charge_density.{cpp,h}` | 242 + 159 | `ChargeDensity` 클래스 (pyrho 포팅) | `features/data/charge_density/charge_density.{cpp,h}` |
| `legacy/atoms/infrastructure/charge_density_renderer.{cpp,h}` | 1,092 + 133 | `ChargeDensityRenderer` — Isosurface(vtkContourFilter), Volume(vtkVolume), 각 모드 액터 관리 | `features/data/charge_density/{isosurface_renderer, volume_renderer}.{cpp,h}` (분할 권장) |
| `legacy/atoms/infrastructure/slice_renderer.h` | 22 (stub 만) | `SliceRenderer` 인터페이스 선언 — *implementation 없음* | `features/data/slice/slice_renderer.{cpp,h}` (.cpp 신규 작성) |
| `legacy/atoms/ui/charge_density_ui.{cpp,h}` | 2,519 + 318 | `ChargeDensityUI` — 모드 토글 + colormap + value range + animation | `features/data/charge_density/charge_density_ui.{cpp,h}` (압축 가능, **UI 동작 보존 §1.4**) |
| **(신규)** | — | `charge_density_controller`, `slice_controller`, `data_menu` | 신규 작성 |

**총 legacy 참조: 4,485 줄.** 압축 후 약 **2,400~2,800 줄** 의 새 코드 예상 (UI 동작 보존하되 주위 정돈).

### 3.1 외부 의존 사전 분석 (Phase 1 의 font_manager 교훈 적용)

| legacy 파일 | 외부 의존 | 새 트리 처리 |
|---|---|---|
| `charge_density.h` | STL 만 | 그대로 + namespace |
| `charge_density.cpp` | `chgcar_parser.h`, STL | `core/io/chgcar_parser.h` |
| `charge_density_renderer.h` | `chgcar_parser.h`, `../domain/charge_density.h`, **`vtk_renderer.h` (legacy)**, `colormap.h`, vtk* | `core/io::chgcar_parser`, `core/data::colormap`, vtk 직접. **`vtk_renderer.h` 의존은 §3.2 처리** |
| `charge_density_renderer.cpp` | 위 + 다수 vtk 호출 | 동일 |
| `slice_renderer.h` | `vtkCutter`, `vtkPlane`, `vtkScalarBarActor`, `ColorMapType` | 신규 .cpp 작성 + `core/data::colormap::ColorMapType` |
| `charge_density_ui.h` | `chgcar_parser.h`, `colormap.h`, `class AtomsTemplate` (forward) | AtomsTemplate forward → controller 로 대체 |
| `charge_density_ui.cpp` | `m_parent->Render*ChargeDensity*()` 다수 호출 | controller 로 분기. **ImGui 위젯 흐름 1:1 보존 §1.4** |

### 3.2 `vtk_renderer.h` 의존 처리

`charge_density_renderer.cpp` 가 legacy `atoms/infrastructure/vtk_renderer.h` 를 사용한다. 이 거대한 파일 (1,792 줄) 은 *Phase 3.4* 가 분할하기로 되어 있다.

| 옵션 | 설명 | 채택 |
|---|---|---|
| (A) | `core::vtk::VtkViewer::Instance().GetRenderer()->AddActor(...)` 직접 사용 | **권장** |
| (B) | 임시 shim — `app/legacy_compat_vtk_renderer.cpp` (회색지대 §1.2) | 백업 |
| (C) | charge_density_renderer 의 vtk_renderer 의존 부분을 *Phase 3.4 후 follow-up* | Volumetric 만 follow-up |

**(A) 직접 정리 권장**.

### 3.3 메뉴 트리 매핑 (04 §7 Data 참조)

| 메뉴 항목 | 새 진입점 | 윈도우 |
|---|---|---|
| `Data / Isosurface` | `features::data::charge_density::Show(Mode::Isosurface)` | ChargeDensityViewer |
| `Data / Surface` | `features::data::charge_density::Show(Mode::Surface)` | ChargeDensityViewer (모드만 전환) |
| `Data / Volumetric` | `features::data::charge_density::Show(Mode::Volumetric)` | ChargeDensityViewer (모드만 전환) |
| `Data / Plane` | `features::data::slice::Show()` | SliceViewer |

→ ChargeDensity 의 3 모드는 *같은 윈도우 안의 토글*. legacy 의 모드 토글 동작 (§1.4.1) 이 본 단계에서도 그대로 보존되어야 한다.

---

## 4. 작업 절차 (단계별)

### Step 1 — `features/data/{charge_density, slice}/` 폴더 신설

```bash
cd webassembly/src
mkdir -p features/data/charge_density
mkdir -p features/data/slice
```

### Step 2 — `charge_density.{cpp,h}` 도메인 이식

legacy 의 401 줄을 namespace 정리하며 이식. 라인수 압축 자연 발생.

```cpp
/**
 * @file features/data/charge_density/charge_density.h
 * @brief 3D 전하 밀도 데이터 (pyrho ChargeDensity 포팅).
 */
#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace features::data::charge_density {

class ChargeDensity {
public:
    ChargeDensity() = default;
    static std::unique_ptr<ChargeDensity> FromFile(const std::string& filePath);
    const std::array<int, 3>& GridShape() const { return gridShape_; }
    const std::array<std::array<float, 3>, 3>& Lattice() const { return lattice_; }
    const std::vector<float>& Data() const { return data_; }
    std::pair<float, float> ValueRange() const;

private:
    std::vector<float>                  data_;
    std::array<int, 3>                  gridShape_{0, 0, 0};
    std::array<std::array<float, 3>, 3> lattice_{};
};

} // namespace features::data::charge_density
```

### Step 3 — `isosurface_renderer.{cpp,h}` + `volume_renderer.{cpp,h}` 분할

legacy `charge_density_renderer.cpp` (1,092 줄) 를 두 모듈로 분할.

| 새 모듈 | 책임 | vtk 클래스 |
|---|---|---|
| `isosurface_renderer.{cpp,h}` | Isosurface + Surface (등치면) | `vtkContourFilter`, `vtkPolyDataMapper`, `vtkActor` |
| `volume_renderer.{cpp,h}` | Volumetric (Volume Rendering) | `vtkVolume`, `vtkColorTransferFunction`, `vtkPiecewiseFunction`, `vtkSmartVolumeMapper` |

> **§1.4 보존**: 모드 전환 시 *기존 액터 자동 청산 → 새 액터 추가* 의 sequence 가 legacy 와 동일해야 함.

### Step 4 — `slice/slice_renderer.{cpp,h}` 신규 작성

legacy 의 22 줄 stub 위에 .cpp 신규 작성. `vtkCutter`, `vtkPlane`, `vtkScalarBarActor` 사용.

### Step 5 — UI + controller (Phase 3.1 패턴)

#### 5.1 `charge_density_controller.{cpp,h}`

```cpp
namespace features::data::charge_density {

enum class Mode { None, Isosurface, Surface, Volumetric };

class ChargeDensityController {
public:
    explicit ChargeDensityController(core::scene::SceneState& scene);
    void Show(Mode mode);
    void Clear();
    Mode CurrentMode() const { return mode_; }
    bool HasData() const     { return data_ != nullptr; }
    void SetData(std::unique_ptr<ChargeDensity> data);

private:
    core::scene::SceneState&             scene_;
    std::unique_ptr<ChargeDensity>       data_;
    IsosurfaceRenderer                   isosurfaceRenderer_;
    VolumeRenderer                       volumeRenderer_;
    Mode                                 mode_ = Mode::None;
};

}
```

#### 5.2 `charge_density_ui.{cpp,h}` — **§1.4 보존 핵심**

legacy `charge_density_ui.cpp` (2,519 줄) 의 ImGui 흐름을 *위젯 단위* 로 그대로 보존하며 이식.

```cpp
void ChargeDensityUI::Render(bool* open) {
    if (!ImGui::Begin("Charge Density Viewer", open)) { ImGui::End(); return; }

    // ★ §1.4 보존: 모드 콤보의 라벨 / 항목 순서 / 기본 선택 그대로
    const char* kModes = "Isosurface\0Surface\0Volumetric\0";
    int modeIdx = static_cast<int>(controller_.CurrentMode()) - 1;
    if (modeIdx < 0) modeIdx = 0;
    if (ImGui::Combo("Mode", &modeIdx, kModes)) {
        controller_.Show(static_cast<Mode>(modeIdx + 1));
    }

    // ★ §1.4 보존: isovalue 슬라이더의 범위/format/flags 그대로
    auto [vmin, vmax] = controller_.HasData()
        ? controller_.GetData().ValueRange()
        : std::pair{-1.0f, 1.0f};
    ImGui::SliderFloat("isovalue", &isoValue_, vmin, vmax, "%.4f");

    // (이하 wireframe 토글, colormap 드롭다운, animation 체크박스, advanced grid 등 —
    //  legacy 의 흐름과 위젯 인자 1:1 보존)
    // ...

    ImGui::End();
}
```

> 위젯 인자 (라벨, 범위, format) 를 legacy 와 grep diff 으로 비교 검증하는 절차를 §5 에 포함.

#### 5.3 `slice/slice_controller.{cpp,h}` + `slice/slice_ui.{cpp,h}`

§1.4.2 의 채택 사항 (XY/XZ/YZ/Custom 라디오, position 슬라이더 grey-out 규칙 등) 적용.

### Step 6 — `data_menu.{cpp,h}` 신규 (4 항목 dispatch)

```cpp
void DrawMenu(const app::MenuContext& /*ctx*/) {
    if (ImGui::BeginMenu("  Data")) {
        if (ImGui::MenuItem("Isosurface"))  { g_cdController->Show(Mode::Isosurface);  g_showCDWindow = true; }
        if (ImGui::MenuItem("Surface"))     { g_cdController->Show(Mode::Surface);     g_showCDWindow = true; }
        if (ImGui::MenuItem("Volumetric"))  { g_cdController->Show(Mode::Volumetric);  g_showCDWindow = true; }
        if (ImGui::MenuItem("Plane"))       { g_slController->Show();                  g_showSliceWindow = true; }
        ImGui::EndMenu();
    }
}

void InitOnce(core::scene::SceneState& scene) {
    static charge_density::ChargeDensityController cdController(scene);
    static charge_density::ChargeDensityUI         cdUI(cdController);
    static slice::SliceController                  slController(scene);
    static slice::SliceUI                          slUI(slController);
    g_cdController = &cdController; g_cdUI = &cdUI;
    g_slController = &slController; g_slUI = &slUI;

    // **format_registry 의 첫 사용** — chgcar 파서 등록
    using namespace core::io;
    FormatRegistry::Instance().Register("CHGCAR", [](auto path, auto& out) {
        return ParseChgcarFile(path, out);
    });
}
```

### Step 7 — `app/app.cpp` 임시 메뉴 hook

```cpp
#include "../features/data/data_menu.h"

void App::renderDockSpace() {
    ...
    if (ImGui::BeginMenuBar()) {
        ...
        features::utilities::bz::DrawMenu(/*ctx*/{});
        features::data::DrawMenu(/*ctx*/{});             // ← Phase 3.2 신규
        ...
    }
    features::utilities::bz::RenderWindows(nullptr);
    features::data::RenderWindows(nullptr, nullptr);     // ← 신규
}

int App::Init() {
    ...
    static core::scene::SceneState scene;
    features::utilities::bz::InitOnce(scene);
    features::data::InitOnce(scene);                     // ← 신규
    ...
}
```

### Step 8 — `CMakeLists.txt` SOURCES_FEATURES 확장

```cmake
set(SOURCES_FEATURES
    # Phase 3.1
    webassembly/src/features/utilities/brillouin_zone/...

    # Phase 3.2 신규
    webassembly/src/features/data/data_menu.cpp
    webassembly/src/features/data/data_menu.h
    webassembly/src/features/data/charge_density/charge_density.cpp
    webassembly/src/features/data/charge_density/charge_density.h
    webassembly/src/features/data/charge_density/isosurface_renderer.cpp
    webassembly/src/features/data/charge_density/isosurface_renderer.h
    webassembly/src/features/data/charge_density/volume_renderer.cpp
    webassembly/src/features/data/charge_density/volume_renderer.h
    webassembly/src/features/data/charge_density/charge_density_controller.cpp
    webassembly/src/features/data/charge_density/charge_density_controller.h
    webassembly/src/features/data/charge_density/charge_density_ui.cpp
    webassembly/src/features/data/charge_density/charge_density_ui.h
    webassembly/src/features/data/slice/slice_renderer.cpp
    webassembly/src/features/data/slice/slice_renderer.h
    webassembly/src/features/data/slice/slice_controller.cpp
    webassembly/src/features/data/slice/slice_controller.h
    webassembly/src/features/data/slice/slice_ui.cpp
    webassembly/src/features/data/slice/slice_ui.h
)
```

### Step 9 — 정적 검증

```bash
cd webassembly/src

# legacy 호출 0
grep -rnE "AtomsTemplate::|atoms::domain::|atoms::infrastructure::|atoms::ui::" \
  features/data/ | grep -v "//" || echo "OK"

# legacy include 0
grep -rnE '#include\s+"(\.\./)*legacy/' features/data/ | grep -v "//" || echo "OK"

# vtk_renderer 의존 0
grep -rnE "atoms::infrastructure::VTKRenderer" features/data/ | grep -v "//" || echo "OK"

# format_registry::Register 첫 호출
grep -nE "FormatRegistry::Instance\(\)\.Register" features/data/data_menu.cpp || echo "WARNING"

# 핵심 vtk 호출 보존
grep -nE "vtkContourFilter|vtkVolume|vtkSmartVolumeMapper|vtkCutter|vtkPlane" features/data/ | wc -l

# **§1.4 UI 보존 검증** — ImGui 위젯 인자 비교 (legacy 와 새 트리)
grep -nE 'ImGui::(Combo|SliderFloat|Checkbox|Button|RadioButton)' \
  legacy/atoms/ui/charge_density_ui.cpp \
  features/data/charge_density/charge_density_ui.cpp 2>/dev/null
```

### Step 10 — 빌드 + 런타임 검증 (Windows 측)

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release
npm run dev
```

**§1.4 보존 검증** (런타임):

- [ ] Side-by-side 스크린샷 (legacy Phase 0 baseline vs 새 트리) 캡처
- [ ] §1.4.3 의 시나리오 S1/S2/S3 각각 legacy 와 새 트리에서 차례로 실행해 동일 결과 확인
- [ ] Intentional UI deviation 이 있다면 PR 본문 사유서에 명기

### Step 11 — 커밋 & PR

```powershell
git commit -m "Phase 3.2: features/data/{charge_density, slice} — second feature

- New folders + 13 ported files + first format_registry::Register call.
- Menu bar: Data / Isosurface / Surface / Volumetric / Plane.
- core/io::format_registry first user — Phase 2 infra validated.
- legacy ChargeDensity Viewer UI preserved 1:1 per §6.0.1 of the upper plan.

Side-by-side screenshots and S1/S2/S3 scenario tests attached.
Intentional UI deviation: None.

Reference:
  - webassembly/docs/05_redevelopment_plan.md (Phase 3.2 + §6.0)
  - webassembly/docs/phases/phase3_2_data_charge_density.md
"
```

---

## 5. 검증 매트릭스

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `features/data/{charge_density, slice}/` 폴더 존재 | `ls features/data/` | 2 폴더 | 정적 |
| 2 | charge_density 파일 수 | `ls features/data/charge_density/` | 11 (.cpp 5 + .h 6) | 정적 |
| 3 | slice 파일 수 | `ls features/data/slice/` | 6 (.cpp 3 + .h 3) | 정적 |
| 4 | data_menu | `ls features/data/data_menu.*` | `data_menu.cpp data_menu.h` | 정적 |
| 5 | namespace 일관성 | grep | 모든 .cpp/.h | 정적 |
| 6 | legacy 호출 0 | grep | 0 hit | 정적 |
| 7 | `#include "../legacy/"` 0 | grep | 0 hit | 정적 |
| 8 | legacy `vtk_renderer` 의존 0 | grep | 0 hit | 정적 |
| 9 | **format_registry::Register 호출 1+** | `grep -nE "FormatRegistry.*Register" features/data/` | 1+ hit | 정적 — 핵심 |
| 10 | 핵심 vtk 호출 보존 | grep `vtkContourFilter|vtkVolume|vtkCutter|vtkPlane` | 다수 hit | 정적 |
| 11 | `app/app.cpp` 의 features::data 호출 추가 | grep | 3 hit | 정적 |
| 12 | CMakeLists.txt 의 SOURCES_FEATURES 확장 | grep | 13+ hit | 정적 |
| 13 | CMake 안전벨트 유지 | grep | 1 hit | 정적 |
| 14 | **§1.4 UI 보존 — ImGui 위젯 인자 비교** | legacy 와 새 트리의 `ImGui::(Combo\|SliderFloat\|Checkbox\|Button\|RadioButton)` grep diff | 라벨/범위/format 일치 (의도적 차이는 §1.4.3 사유서로 정당화) | 정적 |
| 15 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 16 | Release 빌드 | `npm run build-wasm:release` | exit 0 | 동적 |
| 17 | 메뉴바에 Data 메뉴 추가 | `npm run dev` | Utilities 옆 Data 메뉴 | 동적 |
| 18 | Data → Isosurface/Surface/Volumetric | 메뉴 클릭 | ChargeDensityViewer 윈도우 + 모드 전환 | 동적 |
| 19 | Data → Plane | 메뉴 클릭 | SliceViewer 윈도우 | 동적 |
| 20 | **§1.4 UI 보존 — Side-by-side 스크린샷** | legacy vs 새 트리 시각 비교 | 위젯 배치/텍스트/색상 동일 | 동적 — 핵심 |
| 21 | **§1.4 보존 — 시나리오 S1/S2/S3** | 시나리오 3 종 차례로 실행 | legacy 와 결과 동일 | 동적 — 핵심 |
| 22 | 콘솔 에러 0 | DevTools | 0 errors | 동적 |
| 23 | wasm 사이즈 회귀 | Phase 3.1 ± 5~10 % | 정상 범위 | 동적 |

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | charge_density_renderer 의 `vtk_renderer.h` (legacy) 의존 미해결 | link error | Step 3.2 Option (A) 직접 정리 |
| 6.2 | charge_density_ui.cpp 의 2,519 줄 압축 이식 중 ImGui 흐름 누락 | 윈도우 동작 이상 + **§1.4 UI 보존 위반** | legacy 의 render() 흐름 1:1 비교 + ImGui 위젯 인자 grep diff (§5 #14) |
| 6.3 | `core/io/format_registry::Register` 시그니처 미스매치 | link error | Phase 2 의 format_registry.h 와 함수 시그니처 일치 사전 확인 |
| 6.4 | Volumetric 의 `vtkSmartVolumeMapper` emcc 미지원 | 런타임 crash | VTK_MODULE_INIT 점검. 미지원 시 Volumetric stub 처리 (§1.4 사유서 명시) |
| 6.5 | slice_renderer .cpp 신규 작성 미숙 | Plane 렌더 안 됨 | VTK 공식 vtkCutter 예제 참고 |
| 6.6 | Phase 3.6 도착 전까지 charge density 데이터 미로드 | 사용자 *"기능 안 됨"* 오해 | UI 안에 *"Load CHGCAR file via File menu (available in Phase 3.6)"* 안내 |
| 6.7 | data_menu 의 4 항목 → 2 sub-folder 분기 패턴이 본 단계 첫 사례 | 후속 sub-phase 가 본 패턴 따라가야 함 | 04 §7 Data 매핑 표 그대로 따름 |
| 6.8 | `core/data/colormap` 의 ColorMapType enum 이 legacy 와 형식 다를 가능성 | 컴파일 에러 + **§1.4 colormap 드롭다운 항목 변경** | Phase 2 colormap.h 와 legacy 호환성 사전 확인. 항목 변경 시 §1.4.3 사유서 |
| 6.9 | EventBus `onStructureRemoved` 구독 첫 사례 | charge density 자동 청산 안 됨 | controller InitOnce 에서 subscribe + 단위 테스트 |
| 6.10 | PR diff 큼 (4,485 줄 base + UI 보존 검증 부담) | 머지 지연 | commit 분할 (charge_density / slice) + 스크린샷 사전 준비 |
| 6.11 | **§1.4 UI 보존 위반** — 압축 중 ImGui 위젯 인자 변경 | 사용자 워크플로우 변경 | (a) §5 #14 의 grep diff sweep 으로 사전 발견. (b) 발견 시 즉시 복원 또는 §1.4.3 사유서 명기. (c) 검토자가 reject |
| 6.12 | charge_density_ui 에 *Animation* 같은 시간 의존 동작이 있어 *side-by-side 스크린샷* 으로는 충분히 비교 불가 | UI 보존 검증 부족 | §1.4.3 의 시나리오 S1/S2/S3 + 동영상 캡처 권장 |

---

## 7. 롤백 절차

```bash
git restore --staged .
git restore .
git clean -fd webassembly/src/features/data
```

원인 진단:

1. **§1.4 UI 보존 위반** 발견 → §6.11 의 (b) 복원 또는 (c) 사유서.
2. format_registry 미해결 심볼 → Phase 2 인터페이스 점검.
3. 그 외는 Phase 3.1 평가서 §7 의 진단 패턴과 동일.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] §2 의 Phase 3.1 전제 충족
- [ ] §5 검증 매트릭스 23 항목 통과
- [ ] features/data/ 안에서 legacy 호출 0 + legacy include 0
- [ ] **format_registry::Register 호출 1+ 곳** (Phase 2 첫 검증)
- [ ] **§1.4 UI 보존 — Side-by-side 스크린샷 첨부** (ChargeDensity Viewer)
- [ ] **§1.4 UI 보존 — 시나리오 S1/S2/S3 각 legacy 와 동일 결과 확인**
- [ ] **Intentional UI deviation 사유서 작성** (없으면 *"None"*)
- [ ] `legacy/` 한 글자도 변경 없음
- [ ] PR 본문에 *"Volumetric vtk_renderer 의존 정리는 Phase 3.4 후 follow-up"* 명시
- [ ] PR 본문에 본 문서와 상위 §6.0.1 링크 포함

검토자 — 머지 전:

- [ ] diff 가 (a) features/data/ 신규 13 파일, (b) app/app.cpp 임시 hook 3 곳, (c) CMakeLists.txt source 추가 — 3 가지로만 구성?
- [ ] **첨부 스크린샷에서 legacy 와 새 트리의 ChargeDensity Viewer 가 시각적으로 동일?**
- [ ] **시나리오 S1/S2/S3 본인 환경에서도 동일 결과 재현?**
- [ ] Intentional UI deviation 사유서가 있다면 그 사유가 합리적인가?
- [ ] `legacy/` 0 changed lines
- [ ] CI 빌드 (debug + release) 통과

---

## 9. 부록

### 9.1 features/data/ 13 파일 요약 + 라인수 예상

| 파일 | 역할 | 라인 (legacy) | 압축 후 예상 |
|---|---|---|---|
| `data_menu.{cpp,h}` | 4 항목 dispatch (신규) | — | ~110 |
| `charge_density/charge_density.{cpp,h}` | 도메인 | 401 | ~250 |
| `charge_density/isosurface_renderer.{cpp,h}` | Isosurface + Surface | (1,225 일부) | ~350 |
| `charge_density/volume_renderer.{cpp,h}` | Volumetric | (1,225 일부) | ~300 |
| `charge_density/charge_density_controller.{cpp,h}` | 통합 진입점 | — | ~180 |
| `charge_density/charge_density_ui.{cpp,h}` | ImGui 윈도우 (**§1.4 보존**) | 2,837 | ~1,200~1,400 |
| `slice/slice_renderer.{cpp,h}` | Plane 렌더 (신규) | 22 (stub) | ~250 |
| `slice/slice_controller.{cpp,h}` | (신규) | — | ~120 |
| `slice/slice_ui.{cpp,h}` | (신규, §1.4.2) | — | ~280 |
| **합계** | | **~4,485** | **~3,040** (≈ 68% — UI 보존으로 압축 비율 보수적) |

> Phase 3.1 의 53% 보다 약간 보수적 — *§1.4 UI 보존* 으로 ImGui 위젯 흐름 압축이 제한됨.

### 9.2 후속 sub-phase 가 본 패턴을 그대로 적용

| sub-phase | 본 패턴 적용도 |
|---|---|
| 3.3 build/{periodic_table, bravais} | data_menu 처럼 *1 메뉴 → 2 sub-folder* 분기 + §1.4 UI 보존 |
| 3.4 edit/{atoms, bonds, cell} | *1 메뉴 → 3 sub-folder* — §1.4 UI 보존 부담 가장 큼 (`atom_editor_ui` 896 줄) |
| 3.5 measurement | 단일 sub-folder, 5 항목 모드 — §1.4 의 모드 토글 보존 |
| 3.6 file | 단일 sub-folder. format_registry *읽기* 사용자 |
| 3.7 viewer + toolbar | toolbar 위젯 인자 보존 — Mesh Display Mode / Projection / Reset View 등 |

### 9.3 사후 점검: Phase 3.2 머지 직후 트리

```
webassembly/src/
├─ ... (Phase 0-2 유지)
├─ features/
│  ├─ utilities/brillouin_zone/                  (Phase 3.1)
│  └─ data/                                       ★ 신규
│     ├─ data_menu.{cpp,h}
│     ├─ charge_density/
│     │  ├─ charge_density.{cpp,h}
│     │  ├─ isosurface_renderer.{cpp,h}
│     │  ├─ volume_renderer.{cpp,h}
│     │  ├─ charge_density_controller.{cpp,h}
│     │  └─ charge_density_ui.{cpp,h}             ★ §1.4 UI 보존 핵심
│     └─ slice/
│        ├─ slice_renderer.{cpp,h}
│        ├─ slice_controller.{cpp,h}
│        └─ slice_ui.{cpp,h}                      ★ §1.4.2 채택
└─ legacy/                                        (동결)
```

---

## 10. 후속 단계 연결 — Phase 3.3

Phase 3.2 머지 후 Phase 3.3 (`features/build/{periodic_table, bravais}`) 진입.

1. `features/build/` 신설 + 두 sub-folder.
2. legacy `atoms/ui/{periodic_table_ui, bravais_lattice_ui}` + `atoms_template_{periodic_table, bravais_lattice}.cpp` 이식.
3. Add atoms / Bravais Lattice Templates 메뉴 wiring.
4. Phase 3.2 의 *2 sub-folder 분기 패턴* + **상위 §6.0.1 의 UI 1:1 보존 원칙** 그대로 적용.

Phase 3.3 세부계획서는 `phase3_3_build_periodic_bravais.md` 에 별도 작성.

---

## 11. 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.2) + **§6.0 공통 지침 (UI 1:1 보존)**
- 선행 평가서: [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- 선행 계획서: [`./phase3_1_utilities_brillouin_zone.md`](./phase3_1_utilities_brillouin_zone.md)
- Phase 2 (인프라): [`./phase2_core_skeleton.md`](./phase2_core_skeleton.md) — `core/io/format_registry`
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md)
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §7 Data
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
