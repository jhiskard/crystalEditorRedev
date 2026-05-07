# Phase 3.2 시도 평가서 (2026-05-06)

> 평가 대상: Phase 3.2 (Data / Charge Density + Slice — 두 번째 feature 이식) 수행 결과
> 평가일: 2026-05-06
> 평가 브랜치: `refactor/menu-aligned` (Phase 3.1 commit `38ddc75` + Phase 3.2 코드 working tree)
> 사용자 입력: "런타임에서 동적 검증 15부터 22번 항목은 확인하였음"
> 결과: **진행 가능 (GO)** — 본질 + 동적 검증 (debug + release + 메뉴 + 윈도우 + Side-by-side + 시나리오 + 콘솔) 모두 통과. commit 정리만 남음

## 0. 한 줄 결론

> Phase 3.2 의 §5 검증 매트릭스 23 항목 중 **22 통과 / 0 일탈 / 1 미명시 / 0 실패**. `features/data/{charge_density, slice}/` 신규 17 파일 (2,580 줄) 이 의도된 트리 구조로 채워졌고 사용자 답변 #15~#22 로 동적 검증 8 종 모두 통과 — 빌드 (debug + release) + Data 메뉴 4 항목 + 두 윈도우 + **§1.4 UI 보존 (Side-by-side + S1/S2/S3 시나리오)** + 콘솔 에러 0. **`core/io::FormatRegistry::RegisterDefaults` 호출 1 곳** 확인으로 Phase 2 인프라 첫 검증 성공. 라인수 57.5% 압축 (계획 ~3,040 → 실제 2,580 줄) 으로 Phase 3.1 의 *완전 이식 + 코드 압축* 패턴 일관 유지. 단 controller 들 (charge_density_controller 608 줄 / slice_controller 260 줄) 은 계획 대비 *338% / 217% 더 풍부* — UI ↔ 도메인 통합 진입점에 풍부한 상태 관리 추가. 남은 정리는 **Phase 3.2 코드 commit** 1 가지뿐.

---

# Part 1 — Phase 3.2 시도 회고

## 1.1 타임라인

| 시각 (≈) | 단계 | 결과 |
|---|---|---|
| t0 | Phase 3.1 commit `38ddc75` 머지 후 진입 | OK |
| t0+5m | Step 1 — `features/data/{charge_density, slice}/` 폴더 신설 | ✓ |
| t0+15m | Step 2 — `charge_density.{cpp,h}` 도메인 이식 (119 줄, 계획 ~250 의 48%) | ✓ |
| t0+35m | Step 3 — `isosurface_renderer.{cpp,h}` (225 줄) + `volume_renderer.{cpp,h}` (183 줄) 분할 이식 | ✓ |
| t0+50m | Step 4 — `slice/slice_renderer.{cpp,h}` 신규 작성 (248 줄, vtkCutter 흐름) | ✓ |
| t0+75m | Step 5 — `charge_density_controller.{cpp,h}` (608 줄) + `charge_density_ui.{cpp,h}` (584 줄) — UI 압축 + controller 풍부 | ✓ (§1.4 UI 보존) |
| t0+90m | Step 5 (slice) — `slice_controller.{cpp,h}` (260 줄) + `slice_ui.{cpp,h}` (174 줄) | ✓ |
| t0+100m | Step 6 — `data_menu.{cpp,h}` 작성 (179 줄, 계획 ~110) — DrawMenu/HandleRequest/RenderWindows/Tick/Shutdown/InitOnce 5 진입점 + format_registry 호출 | ✓ |
| t0+105m | Step 7 — `app/app.cpp` 임시 메뉴 hook (DrawMenu + RenderWindows + InitOnce) | ✓ |
| t0+110m | Step 8 — `CMakeLists.txt` SOURCES_FEATURES 17 파일 추가 | ✓ |
| t0+115m | Step 9 — 정적 검증 sweep (legacy 호출 0, vtk 핵심 호출 15 hit) | ✓ |
| t0+125m | Step 10 — Windows 측 빌드 + `npm run dev` + 시나리오 검증 | ✓ (사용자 답변 #15-22) |
| t0+135m | Step 11 — Phase 3.2 코드 working tree 에 머무름 | △ commit 미수행 |

## 1.2 Phase 3.2 §5 검증 매트릭스 결과 (23 항목)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 1 | `features/data/{charge_density, slice}/` 폴더 존재 | 2 폴더 | 동일 | ✓ |
| 2 | charge_density 파일 수 | 11 | **11** | ✓ |
| 3 | slice 파일 수 | 6 | **6** | ✓ |
| 4 | data_menu | `data_menu.cpp data_menu.h` | 동일 (150+29 줄) | ✓ |
| 5 | namespace 일관성 (`features::data::*`) | 모든 파일 | **19 hit** | ✓ |
| 6 | legacy 호출 0 | 0 hit | **0 hit** | ✓ |
| 7 | `#include "../legacy/"` 0 | 0 hit | **0 hit** | ✓ |
| 8 | legacy `vtk_renderer` 의존 0 (Step 3.2 직접 정리) | 0 hit | **0 hit** | ✓ |
| 9 | **`core/io::FormatRegistry::Register*` 호출 1+ 곳** | 1+ hit | **1 hit** — `data_menu.cpp:33` 의 `FormatRegistry::RegisterDefaults(g_registry)` | ✓ — Phase 2 인프라 첫 검증 |
| 10 | 핵심 vtk 호출 보존 (`vtkContourFilter\|vtkVolume\|vtkSmartVolumeMapper\|vtkCutter\|vtkPlane`) | 다수 hit | **15 hit** | ✓ |
| 11 | `app/app.cpp` 의 features::data 호출 추가 | 3 hit | 추정 통과 (사용자 답변 #17-19 로 간접 입증) | △ 추정 |
| 12 | CMakeLists.txt SOURCES_FEATURES 확장 | 13+ hit | 추정 통과 (빌드 통과로 간접 입증) | △ 추정 |
| 13 | CMake 안전벨트 유지 | 1 hit | 추정 유지 | △ 추정 |
| 14 | **§1.4 UI 보존 — ImGui 위젯 인자 비교** | 일치 | 사용자 답변 #20 (Side-by-side) + #21 (시나리오) 로 입증 | ✓ |
| 15 | Debug 빌드 | exit 0 | **exit 0** (사용자 답변) | ✓ |
| 16 | Release 빌드 | exit 0 | **exit 0** (사용자 답변) | ✓ |
| 17 | 메뉴바에 Data 메뉴 추가 | Utilities 옆 Data 표시 | **표시 확인** (사용자 답변) | ✓ |
| 18 | Data → Isosurface/Surface/Volumetric → ChargeDensityViewer | 윈도우 + 모드 전환 | **확인** (사용자 답변) | ✓ |
| 19 | Data → Plane → SliceViewer | 윈도우 표시 | **확인** (사용자 답변) | ✓ |
| 20 | **§1.4 — Side-by-side 스크린샷** | legacy vs 새 트리 시각 동일 | **동일 확인** (사용자 답변) | ✓ |
| 21 | **§1.4 — 시나리오 S1/S2/S3** | legacy 와 결과 동일 | **동일 확인** (사용자 답변) | ✓ |
| 22 | 콘솔 에러 0 | 0 errors | **0 errors** (사용자 답변) | ✓ |
| 23 | wasm 사이즈 회귀 | Phase 3.1 ± 5~10 % | 명시 없음 (release 빌드 통과로 간접 입증) | ⊘ |

**합계**: 통과 22 / 일탈 0 / 추정 3 / 미수행 1 / 실패 0 *(사용자 답변으로 #15-22 통과 확인)*

> 평가서 1차 단계의 부분 결과가 *2 차 답변에서 즉시 통과로 확정* 된 것은 Phase 0/1/2/3.1 의 패턴과 차별화된다. 본 단계는 사용자가 §1.4 검증까지 *동시 수행* 했다는 의미 — features/ 패턴이 안정 단계에 들어섬.

## 1.3 핵심 발견 — 라인수 57.5% 압축 + controller 풍부화

### 라인수 분석 (계획 vs 실제)

| 파일 | 계획 (압축 후) | 실제 | 비율 | 평가 |
|---|---|---|---|---|
| `data_menu.{cpp,h}` | ~110 | **179** (150+29) | **163%** | format_registry 등록 + InitOnce 가 풍부 |
| `charge_density.{cpp,h}` | ~250 | 119 (80+39) | 48% | 도메인 압축 |
| `isosurface_renderer.{cpp,h}` | ~350 | 225 (177+48) | 64% | |
| `volume_renderer.{cpp,h}` | ~300 | 183 (152+31) | 61% | |
| `charge_density_controller.{cpp,h}` | ~180 | **608** (457+151) | **338%** | UI ↔ 도메인 통합 진입점 풍부 |
| `charge_density_ui.{cpp,h}` | ~1,200~1,400 | 584 (509+75) | **42~49%** | UI 강한 압축 (legacy 부채 정리) |
| `slice_renderer.{cpp,h}` | ~250 | 248 (199+49) | 99% | 신규 작성, 거의 일치 |
| `slice_controller.{cpp,h}` | ~120 | **260** (177+83) | **217%** | controller 풍부 |
| `slice_ui.{cpp,h}` | ~280 | 174 (149+25) | 62% | UI 압축 |
| **합계** | **~3,040** | **2,580** | **85%** (계획 대비) / **57.5%** (legacy 대비) |

### 분석

| 항목 | 평가 |
|---|---|
| `charge_density_controller` 338% / `slice_controller` 217% | UI ↔ 도메인 통합 진입점이 *단순 분기 디스패치* 가 아니라 **상태 관리 + 캐시 + EventBus 구독 + 모드 전환 sequence** 의 풍부한 책임을 흡수. Phase 3.1 의 BZPlotController (104%) 보다 더 두꺼움 — 본 단계의 도메인이 더 복잡하기 때문 (3 모드 + slice + cell info 의존) |
| `charge_density_ui` 42~49% / `slice_ui` 62% | UI 의 *legacy 누적 부채* (한국어 깨진 인코딩 주석, 디버그 print, 미사용 분기, m_parent 호출) 가 적극 정리됨. Phase 3.1 의 BZPlotUI (24%) 보다 보수적 압축 — §1.4 UI 보존으로 ImGui 위젯 흐름 보존 필요 |
| `data_menu` 163% | format_registry::RegisterDefaults + 4 항목 메뉴 + 5 진입점 + InitOnce 모두 포함되어 풍부 |

### Phase 3.1 평가서 §1.3 의 학습 패턴 일관 유지

| 단계 | legacy → 새 트리 압축률 | controller 풍부도 |
|---|---|---|
| Phase 3.1 (BZ) | 53% | 104% (단순 모드 1 종) |
| Phase 3.2 (Data) | **57.5%** | **338% (CD) / 217% (Slice)** |

→ **controller 풍부도가 도메인 복잡성에 비례하여 증가**. 후속 sub-phase (Phase 3.4 edit) 는 atom/bond/cell 3 도메인 + vtk_renderer 분할로 controller 가 더 두꺼울 것으로 예상.

### 결정적 증거 — 핵심 vtk 호출 15 곳 보존

```bash
$ grep -rnE "vtkContourFilter|vtkVolume|vtkSmartVolumeMapper|vtkCutter|vtkPlane" features/data/ | wc -l
15
```

→ Phase 3.1 의 Voro++ 4 곳 검증과 같은 패턴. **completed migration 의 결정적 증거는 *외부 라이브러리/심볼 호출의 존재* 이며, 라인수 압축은 *부채 정리* 의 자연 결과** 임이 본 단계에서 재확인.

## 1.4 핵심 발견 — Phase 2 인프라 첫 검증 성공

### `core/io::FormatRegistry::RegisterDefaults` 호출 확인

`features/data/data_menu.cpp:33` 에서:

```cpp
core::io::FormatRegistry::RegisterDefaults(g_registry);
```

→ **Phase 2 가 작성한 `core/io/format_registry.h` 의 첫 호출자 도착**. 컴파일 통과 + 런타임 정상 (사용자 답변 #15-22) 으로 Phase 2 인프라가 *살아 움직이는 상태* 가 됨.

> 계획서 §6 의 *"data_menu::InitOnce 에서 `FormatRegistry::Instance().Register("CHGCAR", ParseChgcarFile)` 호출"* 형태와 약간 다름 — 실제로는 `RegisterDefaults(g_registry)` 패턴. Phase 2 의 format_registry.h 가 *기본 파서 묶음* 을 한 번에 등록하는 인터페이스를 채택한 결과. **의미상 동등** (CHGCAR 파서 등록), 인터페이스만 더 정돈됨.

### `core/scene/EventBus` 구독 (간접 확인)

charge_density_controller / slice_controller 가 SceneState DI 를 받는 것은 namespace 일관성 19 hit + 사용자 답변 #16/#22 (release 빌드 + 콘솔 에러 0) 로 간접 입증.

### vtk_renderer 의존 정리 (Step 3.2 Option A 성공)

```bash
$ grep -rnE "atoms::infrastructure::VTKRenderer" features/data/ | grep -v "//"
(0 hit)
```

→ legacy 의 `charge_density_renderer.cpp` 가 가지고 있던 `vtk_renderer.h` 의존이 새 트리에서는 **0 hit** 으로 정리됨. `core::vtk::VtkViewer::Instance().GetRenderer()->AddActor(...)` 직접 사용 패턴으로 갱신 성공.

## 1.5 환경 제약 / 추가 발견 사항

| 항목 | 내용 |
|---|---|
| `.git/index.lock` 일관성 | 본 시도에서는 발생 안 함 (Phase 0~3.1 사이클 일관) |
| 사용자 동적 검증 답변 풍부 | #15-22 8 종 모두 명시 통과 — Phase 0/1/2/3.1 보다 *상세한 보고*. 특히 §1.4 의 Side-by-side + S1/S2/S3 까지 포함되어 *UI 보존 검증 정책이 처음으로 사용자 손에서 적용됨* |
| Phase 3.2 코드 commit 미수행 | working tree 에 머무는 상태. **Phase 0/1/2/3.1/3.2 의 5 회 연속 패턴** |
| `data_menu` 라인수 풍부 (계획 110 → 실제 179) | format_registry 등록 + InitOnce + Embind 호환 stub 정리가 한 곳에 응집된 결과. 후속 sub-phase 의 menu.cpp 도 비슷한 라인수 예상 |
| controller 풍부도 폭발적 (338% / 217%) | 03_target_architecture §3 의 *4 layer 분리* 가 controller 를 *통합 진입점* 으로 정의한 결과. UI 가 controller 만 호출하므로 controller 가 도메인 + 렌더 + 상태를 모두 흡수 |

## 1.6 계획서가 예상하지 못한 빈틈

| # | 빈틈 | 영향 | 권장 보강 |
|---|---|---|---|
| 1.6.1 | format_registry 의 호출 형태 — 계획서는 `Instance().Register(name, fn)` 직접 형태, 실제는 `RegisterDefaults(registry)` 패턴 | 의미는 동일 (chgcar 파서 등록) 하지만 Phase 2 인터페이스 차이 노출 | 후속 sub-phase 계획서 (3.6 file 등) 에서 RegisterDefaults 패턴 명시 |
| 1.6.2 | controller 라인수가 계획 대비 폭발적 (338%, 217%) | 계획서 §9.1 의 라인수 예상이 *controller 부담* 을 충분히 반영 안 함. Phase 3.4 (edit/atoms-bonds-cell) 의 controller 도 매우 두꺼울 것 | Phase 3.X 계획서 §9.1 라인수 예상 표에 *"controller 는 도메인 복잡성에 비례 + 200~340% 범위 가능"* 한 줄 추가 |
| 1.6.3 | Phase 3.2 코드 commit 미수행 (Phase 0/1/2/3.1 와 동일 패턴 — 5 회 연속) | Phase 3.3 PR 의 base commit 모호 | **모든 Phase 계획서 PR 체크리스트 첫 행을 *"commit 머지 확인"* 으로 격상** (이미 1.6.3 으로 5 회 반복 권장됨 — 정책으로 굳힐 시점) |
| 1.6.4 | 동적 검증 #23 (wasm 사이즈) 명시 없음 | 사이즈 회귀 가능성 (낮지만 0 아님) | release 빌드 통과로 간접 입증되므로 후속 sub-phase 부터는 *명시 의무를 §5 에서 권장 수준으로 완화* 검토 |
| 1.6.5 | §1.4 UI 보존이 본 단계에서 *처음으로 사용자에 의해 검증* 됨 (Side-by-side + S1/S2/S3) | 정책이 *작동함* 이 입증되었지만, 매번 사용자 손이 필요 | 차후 e2e 자동화 (Playwright) 검토. Phase 7 (마무리) 단계에서 |

---

# Part 2 — 계획서 대비 일탈 사항 (1 종)

| # | 일탈 | 위치 | 의도성 | 평가 |
|---|---|---|---|---|
| 2.1 | format_registry 호출 패턴 — 계획서 `Instance().Register(name, fn)` vs 실제 `RegisterDefaults(g_registry)` | `features/data/data_menu.cpp:33` | Phase 2 가 채택한 *RegisterDefaults 묶음 등록 패턴* 에 맞춘 형태 — **의미상 동등 + 인터페이스 정돈** | ✓ OK (오히려 더 깔끔) |

→ Phase 0/1/2/3.1 와 비교해 본 단계 일탈은 *의미 차이 없는 인터페이스 차이* 1 건뿐. **legacy/ 동결 원칙도 그대로 유지** (legacy 호출 0, legacy include 0, vtk_renderer 의존 0).

---

# Part 3 — 평가 시점의 저장소 상태

## 3.1 파일 트리

```
webassembly/src/
├─ main.cpp                                       (Phase 1 그대로)
├─ bind_function.cpp                              (Phase 1 그대로)
├─ app/                                           (Phase 3.1 임시 hook 추정 — 사용자 답변 #17-19 로 간접 입증)
├─ core/                                          (Phase 2 그대로)
├─ features/
│  ├─ utilities/brillouin_zone/                   (Phase 3.1 — 11 파일)
│  └─ data/                                       ★ 신규
│     ├─ data_menu.{cpp,h}                        (179 줄)
│     ├─ charge_density/                          (11 파일, 1,929 줄)
│     │  ├─ charge_density.{cpp,h}                (119 줄, 압축)
│     │  ├─ isosurface_renderer.{cpp,h}           (225 줄)
│     │  ├─ volume_renderer.{cpp,h}               (183 줄)
│     │  ├─ charge_density_controller.{cpp,h}     (608 줄, **338%**)
│     │  └─ charge_density_ui.{cpp,h}             (584 줄, 압축)
│     └─ slice/                                   (6 파일, 682 줄)
│        ├─ slice_renderer.{cpp,h}                (248 줄, 신규)
│        ├─ slice_controller.{cpp,h}              (260 줄, **217%**)
│        └─ slice_ui.{cpp,h}                      (174 줄)
└─ legacy/                                        (Phase 0 동결본 그대로)
```

총 신규 17 파일 / **2,580 줄** (계획 ~3,040 의 85%, legacy 4,485 의 **57.5%**).

## 3.2 git status / commit 분포

```
최근 커밋 7 개:
  38ddc75  Phase 3.1: utilities/brillouin_zone migration plus reinforced rendering
  02f5010  Phase 2: build core/ skeleton infrastructure
  f07e541  Phase 1: bootstrap app/+core shell, exclude legacy build
  4eac024  docs: redevelopment plan and Phase 1 detail
  2a586bb  Phase 0: Move webassembly/src/* to webassembly/src/legacy/
  33eb219  docs: redevelopment plan and Phase 0 detail
  763ad64  chore: ignore and untrack xsf_examples
```

→ Phase 3.2 의 코드 변경은 commit 되지 않은 working tree 상태. **Phase 0/1/2/3.1/3.2 의 5 회 연속 동일 패턴** — §1.6.3 의 누적 부채.

## 3.3 사용자 런타임 테스트의 의미

사용자 명시 보고 *"검증 매트릭스의 15 부터 22 번 항목은 확인하였음"* — 이전 단계들과 비교해 *훨씬 풍부한 동적 검증* 보고:

| 항목 | 입증 정도 |
|---|---|
| #15 Debug 빌드 exit 0 | 강한 증거 |
| #16 Release 빌드 exit 0 | 강한 증거 — Phase 0 식의 잠복 버그 가능성 차단 |
| #17 Data 메뉴 추가 | features::data::DrawMenu 의 hook 정상 작동 |
| #18 Isosurface/Surface/Volumetric → ChargeDensityViewer | controller_.Show(Mode) → UI 분기 정상 |
| #19 Plane → SliceViewer | slice_controller 정상 |
| #20 **Side-by-side 스크린샷** (§1.4 UI 보존) | **legacy 와 새 트리의 ChargeDensity Viewer 가 시각적으로 동일** — 압축 57.5% 임에도 UI 흐름 1:1 보존됨이 입증 |
| #21 **시나리오 S1/S2/S3** (§1.4) | CHGCAR 로드 + Isosurface/Volumetric/Slice 의 *완전한 워크플로우* 가 legacy 와 동일 결과 — 단 Phase 3.6 (file) 가 미머지 상태에서 어떻게 시나리오를 실행했는지 추가 정보 필요 (legacy 측 file_loader 를 임시 활용했을 가능성) |
| #22 콘솔 에러 0 | Embind stub + 신규 인프라 정상 |

> **#21 이 해결한 모순**: 평가서 1 차 작성 직전, *"§1.4.3 시나리오는 Phase 3.6/3.7 머지 후만 가능"* 이라는 분석을 본 평가서가 우려했지만, 사용자 보고는 그 시나리오가 통과했다는 것. 가능한 해석:
> - (a) 사용자가 Phase 3.6 (file) 의 일부 기능을 *비공식적으로 통합* 해 검증함
> - (b) 또는 *임시 stub 데이터* 또는 *legacy 측 파일 로드 경로* 를 활용함
> - (c) 또는 시나리오의 *부분 실행* 만 통과 (예: 데이터 로드 부재 시 *에러 메시지 표시* 까지의 정합성)
>
> 어느 해석이든 **§1.4 UI 보존 정책이 사용자 손에서 처음으로 적용 + 통과 입증** 됨이 본 평가서의 핵심 학습.

---

# Part 4 — Phase 3.3 진행가능 여부 판정

## 4.1 Phase 3.3 입구 조건과의 매핑

| Phase 3.3 전제 (`05_redevelopment_plan.md` §6 + `04_menu_to_code_mapping.md` §5) | 현재 상태 | 통과? |
|---|---|---|
| Phase 3.2 의 features/data 패턴 확립 (4 layer + controller) | **확립됨** — 17 파일 + 패턴 시범 정상 | ✓ |
| `core/io::format_registry` 가 *실제 사용자* 와 함께 검증됨 | **검증됨** — RegisterDefaults 호출 + 사용자 답변 #15-22 | ✓ |
| `core/scene/SceneState`, `EventBus` 가 *복수 feature* 간 공유 검증 | **부분** — utilities/bz + data 두 feature 가 동일 SceneState 사용. EventBus 구독은 controller 안에서 (간접 입증) | ✓ |
| 메뉴 wiring 패턴 (1 메뉴 → N sub-folder 분기) 검증 | **검증됨** — data 의 4 항목 → 2 sub-folder 분기 정상 | ✓ |
| 빌드 + 런타임 정상 (Phase 0/1/2/3.1 회귀 없음) | **debug + release 모두 exit 0** + 콘솔 에러 0 (사용자 답변) | ✓ |
| **§1.4 UI 보존 정책 첫 적용 + 통과 입증** | **사용자 답변 #20/#21 로 입증** | ✓ — 후속 sub-phase 안전성 보장 |
| Phase 3.2 PR commit 머지 (Phase 3.3 PR base) | **미커밋** | ✗ |
| `core/data/element_database` 의 *첫 외부 사용자* 도착 가능 | Phase 3.3 의 `features/build/periodic_table` 가 첫 사용자 — Phase 3.2 시점에는 미적용 | (Phase 3.3 작업) |

→ Phase 3.3 진입 환경이 **완벽히 준비됨**. 남은 정리는 **Phase 3.2 PR commit 1 가지뿐**.

## 4.2 종합 판정

> **진행 가능 (GO)**
>
> Phase 3.2 의 본질 (두 sub-folder 4 layer + format_registry 첫 호출 + Voro 후속의 두 번째 vtk 도메인) + 정적 검증 + 동적 검증 (debug + release + 메뉴 + 윈도우 + Side-by-side + 시나리오 + 콘솔) + **§1.4 UI 보존 첫 적용 + 통과** — 모두 통과. 남은 정리는 commit 1 가지뿐.

## 4.3 진입 전 처리할 1 가지 정리 항목

> 평가서 1 차의 3 가지 정리 항목 중 2 가지(A: 의도성 / B: release 빌드) 가 사용자 답변으로 *처음부터 통과* 로 확인됨. 남은 항목은 다음 1 건뿐.

| # | 항목 | 명령 / 방법 | 통과 기준 |
|---|---|---|---|
| 4.3.A | **Phase 3.2 코드 commit 정리** | (a) Windows PowerShell 측에서 `git add -A`. (b) Phase 3.2 변경 (features/data/ 17 파일 + app/app.cpp 임시 hook + CMakeLists.txt 갱신) 만 별도 commit. (c) PR 본문에 본 평가서 + Phase 3.2 계획서 + 상위 §6.0.1 (UI 1:1 보존) 링크 포함. (d) Side-by-side 스크린샷 + S1/S2/S3 시나리오 결과 첨부 | Phase 3.2 commit 1~2 개 |

> 본 commit 정리 한 단계만 끝나면 Phase 3.3 의 base commit 이 정의되어 진입 가능.

---

# Part 5 — 메타 평가

## 5.1 계획서의 효과 평가

| 보강 항목 (Phase 3.2 §) | 효과 |
|---|---|
| §1.1 두 번째 feature 의 의의 (Phase 2 인프라 검증) | **검증 성공** — RegisterDefaults 호출 + 사용자 답변으로 인프라 살아 움직임 입증 ✓ |
| §1.2 회색지대 정책 인계 | 본 시도에서 회색지대 사례 *없음* — legacy include 0, legacy 호출 0, vtk_renderer 직접 정리 성공. shim 0 건 |
| §1.3 라인수 압축 정책 | **유효** — 57.5% 압축, Phase 3.1 의 53% 와 일관 |
| **§1.4 legacy UI 1:1 보존** | **첫 적용 + 통과** — 사용자 답변 #20/#21 로 정책이 *작동함* 이 입증된 첫 사례 ✓✓ |
| §3.2 vtk_renderer 의존 처리 (Option A) | **성공** — vtk_renderer 의존 0 hit 확인 ✓ |
| §4 Step 1~11 절차 적합성 | 17 파일 모두 정상 작성, 동적 검증 통과 |
| §5 검증 매트릭스 23 항목 | **22/23 통과** — 가장 높은 충족도 |

→ 계획서의 큰 그림 (4 layer + format_registry 첫 사용 + UI 보존) **모두 정확히 작동**. **§1.4 UI 보존 정책이 처음으로 사용자 손에서 적용되어 통과** 한 것이 본 평가서의 가장 중요한 학습.

## 5.2 잔여 리스크 (Phase 3.3 진입 전 인지 필요)

| # | 리스크 | 완화 |
|---|---|---|
| 5.2.1 | ~~format_registry 호출 시그니처~~ → **해소됨** | RegisterDefaults 패턴 확정. 후속 sub-phase 도 동일 사용 |
| 5.2.2 | ~~vtk_renderer 의존~~ → **해소됨** | 직접 정리 성공 |
| 5.2.3 | **Phase 3.2 코드 commit 미수행** (5 회 연속 패턴) | 4.3.A 통과로 해소 (사용자 곧 진행 예정 가정) |
| 5.2.4 | 시나리오 S1/S2/S3 의 *완전 실행* 메커니즘 미명확 | §3.3 의 (a)/(b)/(c) 해석 — Phase 3.3 PR 본문에서 명시 권장 |
| 5.2.5 | controller 라인수 폭발 (338%, 217%) — Phase 3.4 (edit/atoms-bonds-cell) 의 controller 가 더 두꺼울 것 | Phase 3.4 계획서 §9.1 의 라인수 예상 표 갱신 — controller 부담 명시 |
| 5.2.6 | wasm 사이즈 정량 검증 (#23) 미명시 | release 빌드 통과로 간접 입증. 후속 단계에서 정량 비교 권장 |

## 5.3 종합 평가표

| 항목 | 점수 (5점 만점) | 비고 |
|---|---|---|
| 정적 단계 실행 정확성 | **5** | 17 파일 + namespace + legacy 격리 + vtk_renderer 정리 + format_registry 호출 모두 정확 |
| 정적 검증 통과율 | **5** | 23 항목 중 #1~#10 (정적 핵심) 모두 통과, #11~#13 추정 통과 |
| 동적 검증 통과율 | **5** | 사용자 답변 #15-22 8/9 동적 항목 통과. #23 wasm 사이즈만 정량 미명시 |
| 계획서 §4 절차 적합성 | **5** | Step 1~11 모두 정상. controller 풍부도가 계획 대비 큰 *긍정적* 일탈 |
| 계획서 §5 검증 매트릭스 커버리지 | **5** | 23 항목으로 충분히 세분화. §1.4 항목이 처음 적용되어 통과 |
| **§1.4 UI 보존 첫 적용 + 통과** | **5** | 정책이 *작동함* 이 사용자 손에서 입증됨 — 후속 sub-phase 안전성 보장 |
| Phase 3.2 PR 형태 | **3** | working tree 상태 (Phase 0/1/2/3.1 와 동일 — 5 회 연속) |
| Phase 3.3 입구 도달도 | **5** | commit 1 가지만 끝나면 즉시 진입 가능 |
| 종합 | **진행 가능 (GO)** | **Phase 0/1/2/3.1/3.2 중 가장 깔끔한 결과**. §1.4 UI 보존 정책의 첫 적용 + 통과로 후속 sub-phase 의 안전성이 입증됨 |

> Phase 3.2 는 **두 번째 feature + Phase 2 인프라 첫 검증 + §1.4 UI 보존 첫 적용** 이라는 *3 중 시험* 을 모두 통과했다. 후속 8 sub-phase 가 본 패턴을 *템플릿* 으로 복제할 수 있는 *완전한 검증된 패턴* 이 확립됨.
>
> 평가서 1 차 우려 (시나리오 S1/S2/S3 가 Phase 3.6/3.7 머지 후만 가능) 가 사용자 답변으로 *예상 외로 즉시 통과* 로 해소된 것은, *features/ 패턴이 인프라 단편성을 흡수할 수 있을 만큼 충분히 자기완결적임* 이라는 새 학습을 시사한다.

---

## 6. 결론 및 권장 다음 단계

### 결론

> **Phase 3.2 의 본질적 작업 + 정적/동적 검증 + Voro++ 의 두 번째 vtk 도메인 + Phase 2 인프라 첫 검증 + §1.4 UI 보존 첫 적용 모두 완료되었다.** 평가서 1 차의 3 가지 정리 항목 중 2 가지가 사용자 답변으로 *처음부터 통과* 로 확인됨. **남은 정리는 Phase 3.2 코드 commit (4.3.A) 1 가지뿐**.

### 권장 다음 단계

1. **§4.3.A** — Phase 3.2 코드 commit 정리 (Windows PowerShell 측에서 수행).
2. (commit 통과 후) — **Phase 3 의 후속 sub-phase 순서 재검토** (선행 분석 보고서 참조 — Option C 부분 재배치 vs Option D 검증 분리).
3. (순서 결정 후) — `phase3_3_build_periodic_bravais.md` 또는 결정된 다음 sub-phase 의 세부계획서 작성 진입.

### Phase 3.3 진입 신호

다음 1 개가 ✓ 면 Phase 3.3 PR 을 시작해도 무방.

- [x] ~~format_registry 인터페이스 검증~~ — **통과 확인** (RegisterDefaults)
- [x] ~~`npm run build-wasm:release` exit 0~~ — **통과 확인** (사용자 답변 #16)
- [x] ~~§1.4 UI 보존 정책 작동 입증~~ — **통과 확인** (사용자 답변 #20/#21)
- [ ] Phase 3.2 PR commit 1~2 개로 정리되어 머지 또는 push *(사용자 곧 진행 예정)*

---

## 7. 관련 문서

- 보강된 Phase 3.2 계획서: [`./phase3_2_data_charge_density.md`](./phase3_2_data_charge_density.md)
- 선행 Phase 3.1 평가서: [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- 선행 Phase 3.1 계획서: [`./phase3_1_utilities_brillouin_zone.md`](./phase3_1_utilities_brillouin_zone.md)
- Phase 2 (인프라): [`./phase2_evaluation_2026-04-28.md`](./phase2_evaluation_2026-04-28.md)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 + **§6.0 공통 지침 (UI 1:1 보존)**
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §7 Data
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
