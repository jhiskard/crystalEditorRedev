# Phase 3.5 시도 평가서 v2 (2026-05-11)

> 평가 대상: Phase 3.5 (Measurement - 다섯 번째 feature 이식, 단일 sub-folder + 5 모드)
> 평가일: 2026-05-11
> 평가 브랜치: `refactor/menu-aligned` (Phase 3.4 commit `8521e3b` + Phase 3.5 코드 working tree)
> 참조 계획서: [`./phase3_5_measurement.md`](./phase3_5_measurement.md)
> 선행 평가서: [`./phase3_4_evaluation_2026-05-11.md`](./phase3_4_evaluation_2026-05-11.md)
> 평가 입력: 계획서 §5 검증 매트릭스 + 현재 코드 정적 검증 + debug/release wasm 빌드 결과 + 사용자 런타임 확인 사항
> 사용자 확인 사항: 검증 #17~#19 는 런타임에서 확인 완료. 검증 #20~#25 는 Viewer 미복구로 수행 불가.
> 결과: **조건부 진행 가능 (GO with blocked visual scenarios)** - 정적 구조와 빌드는 통과했으나, Viewer 미복구로 시각/시나리오 검증 6개 항목은 보류.

---

## 0. 집중 결론

> Phase 3.5 는 계획서가 요구한 `features/measurement/` 단일 sub-folder 18파일 구조, Measurement 5모드, `app/app.cpp` 4 hook, `CMakeLists.txt` 18 source 등록, `core/scene/events.h` 신규 3 이벤트, `core/vtk/mouse_interactor` drag/pick 보강, `onAtomsChanged` 및 `onStructureRemoved` 추가 구독, Center of Mass 의 `ElementDatabase::getAtomicMass()` 사용까지 정적 검증 기준을 충족했다.
>
> 계획서 §5 검증 매트릭스 27항목 기준 결과는 **18 PASS / 1 PARTIAL / 6 BLOCKED / 2 NOT VERIFIED / 0 FAIL** 로 집계한다. PASS 는 #1~#15 및 #17~#19 이며, #16 은 overlay/list UI와 format 함수가 구현되었지만 legacy 1:1 시각 비교가 Viewer 미복구로 보류되고 distance unit 표기가 `A` 로 대체되어 **PARTIAL** 로 본다. #20~#25 는 사용자 확인대로 Viewer 미복구로 수행 불가하므로 **BLOCKED** 로 분리한다. #26 콘솔 오류 0, #27 wasm size 회귀는 명시 검증이 없어 **NOT VERIFIED** 로 둔다.
>
> 코드 규모는 신규 18파일 **2,192라인** 으로 계획서 예상 2,200~2,400라인의 하한에 거의 일치하고, legacy 예상 3,000라인 대비 약 **73.1%** 수준이다. Phase 3.4 의 74.6%보다 약간 더 압축되었으나, 이는 `measurement_overlay_ui` 가 legacy overlay의 VTK actor 생성 책임을 각 mode 파일로 분산했기 때문이다.
>
> 최종 판단은 **조건부 GO** 다. 구현의 골격과 빌드 안정성은 충분하지만, Phase 3.5 의 본질인 측정 actor 시각화, pick/drag 상호작용, legacy side-by-side 동등성, 동일 publisher N 구독자 동시 동작은 Viewer 복구 후 별도 검증이 필요하다.

---

# Part 1 - Phase 3.5 시도 개요

## 1.1 타임라인

| 시점 | 단계 | 결과 |
|---|---|---|
| t0 | Phase 3.5 계획서 기준 구현 범위 확인 | 완료 |
| t0+ | `core/scene/events.h` 에 `AtomPickedEvent`, `EmptyClickEvent`, `DragSelectionEvent` 및 3개 Bus 추가 | PASS |
| t0+ | `core/vtk/mouse_interactor` 에 click pick, empty click, drag selection 발행 보강 | PASS |
| t0+ | `features/measurement/` 18파일 추가 | PASS |
| t0+ | `measurement_mode` 에 5모드 enum/state helper 추가 | PASS |
| t0+ | `measurement_store` 에 measurement lifecycle, visibility, atom/structure 변경 구독 추가 | PASS |
| t0+ | `measurement_controller` 에 MouseInteractor/EventBus 구독과 pick state machine 추가 | PASS |
| t0+ | `distance`, `angle`, `dihedral`, `center` 계산 및 VTK actor 생성 분리 | PASS |
| t0+ | `measurement_overlay_ui` 에 mode overlay 및 measurement list panel 추가 | PARTIAL - 시각 동등성 검증 보류 |
| t0+ | `measurement_menu` 에 5개 메뉴 항목 및 list/exit 연결 | PASS |
| t0+ | `app/app.cpp` 와 `CMakeLists.txt` 연결 | PASS |
| t0+ | `npm run build-wasm:debug`, `npm run build-wasm:release` 확인 | PASS |
| t0+ | 런타임 #17~#19 사용자 확인 | PASS |
| t0+ | 런타임 #20~#25 Viewer 미복구로 수행 불가 | BLOCKED |

## 1.2 신규 파일 구성과 라인수

| 파일군 | 역할 | 라인수 |
|---|---|---:|
| `measurement_menu.{cpp,h}` | 5모드 메뉴 dispatch + InitOnce + RenderWindows | 92 |
| `measurement_mode.{cpp,h}` | 5모드 enum + target pick count/helper | 99 |
| `measurement_store.{cpp,h}` | measurement lifecycle + actor ownership + visibility + event subscriptions | 518 |
| `measurement_controller.{cpp,h}` | MouseInteractor/EventBus 구독 + pick state machine + drag selection | 472 |
| `measurement_overlay_ui.{cpp,h}` | mode overlay + measurement list window | 133 |
| `distance.{cpp,h}` | Distance 계산 + segmented line/text actor | 133 |
| `angle.{cpp,h}` | Angle 계산 + line/arc/text actor | 228 |
| `dihedral.{cpp,h}` | Dihedral 계산 + lines/planes/arc/text actor | 299 |
| `center.{cpp,h}` | Geometric Center + Center of Mass + marker/text actor | 218 |
| **합계** | **18파일** | **2,192** |

### 라인수 평가

| 기준 | 값 | 평가 |
|---|---:|---|
| 계획서 예상 | 2,200~2,400 | 실제 2,192로 하한과 거의 일치 |
| legacy 예상 | 약 3,000 | 실제 73.1% 수준 |
| Phase 3.4 압축률 | 74.6% | Phase 3.5 는 73.1%로 유사하거나 약간 더 압축 |
| 위험 해석 | 낮음 | 라인수 급증 없음. 단 controller/store 책임이 비교적 큼 |

---

# Part 2 - 검증 매트릭스 결과

## 2.1 정적 항목 (#1~#16)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 1 | `features/measurement/` 폴더 존재 | measurement 폴더 | 존재 | PASS |
| 2 | 파일 수 | 18 (.cpp 9 + .h 9) | 18 | PASS |
| 3 | namespace 일관성 | `features::measurement` | namespace hit 36 | PASS |
| 4 | legacy 호출 0 | 0 hit | `legacy`, `vtk_renderer`, legacy include hit 0 | PASS |
| 5 | `#include "../legacy/"` 0 | 0 hit | 0 hit | PASS |
| 6 | legacy `vtk_renderer` 의존 0 | 0 hit | 0 hit | PASS |
| 7 | `MouseInteractor` 두 번째 구독자 | measurement_controller 구독 | `MeasurementController::Subscribe(core::vtk::MouseInteractor&)` 구현 | PASS |
| 8 | `onAtomsChanged` 추가 구독자 | 1+ | measurement_controller + measurement_store 2곳 | PASS |
| 9 | `onStructureRemoved` 추가 구독자 | 1+ | measurement_controller + measurement_store 2곳 | PASS |
| 10 | Center of Mass atomic mass 사용 | `ElementDatabase` 1+ | `center.cpp` 에 `ElementDatabase::getInstance().getAtomicMass()` 1곳 | PASS |
| 11 | 5모드 enum 보존 | Distance/Angle/Dihedral/GeometricCenter/CenterOfMass | `MeasurementMode` + `MeasurementType` 분리 구현 | PASS |
| 12 | mouse_interactor drag selection 보강 | `OnLeftButtonUp`, `OnMouseMove`, drag state | 모두 존재 | PASS |
| 13 | 신규 이벤트 3종 | 3 struct + 3 Bus | `AtomPickedEvent`, `EmptyClickEvent`, `DragSelectionEvent` + Bus 3개 | PASS |
| 14 | `app/app.cpp` Measurement hook | include + InitOnce + DrawMenu + RenderWindows | 4 hook 확인 | PASS |
| 15 | CMake source 등록 | 18 entries | 18 entries | PASS |
| 16 | overlay/list UI + format 문자열 | legacy overlay/list 동등성 | overlay/list 및 format 함수 구현. 단 side-by-side 미검증, distance unit 은 `A` 표기 | PARTIAL |

### #16 PARTIAL 사유

| 항목 | 평가 |
|---|---|
| mode overlay | `Measurement: <mode>`, picked/selected count, Exit/Clear/Apply 구현 |
| list UI | Visible/Type/Name/Remove table 및 Clear Active Structure 구현 |
| distance format | `%.4f A` 형태. 계획서의 legacy Angstrom 표기와 1:1은 아님 |
| angle/dihedral format | `%.2f deg` 형태 |
| center format | `Center(geom.)`, `Center(mass)` + 좌표 4자리 |
| 시각 동등성 | Viewer 미복구로 side-by-side 검증 불가 |

## 2.2 동적 항목 (#17~#27)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 17 | Debug 빌드 | exit 0 | `npm run build-wasm:debug` 통과 | PASS |
| 18 | Release 빌드 | exit 0 | `npm run build-wasm:release` 통과 | PASS |
| 19 | Measurement 메뉴 5항목 | 메뉴 표시 및 5 sub-item | 사용자 런타임 확인 완료 | PASS |
| 20 | side-by-side screenshot | legacy vs 신규 overlay/list 시각 비교 | Viewer 미복구로 수행 불가 | BLOCKED |
| 21 | S1~S3 Distance/Angle/Dihedral | 각 모드 1 measurement 생성 | Viewer 미복구로 수행 불가 | BLOCKED |
| 22 | S4~S5 Geometric Center/Center of Mass | drag selection + center marker | Viewer 미복구로 수행 불가 | BLOCKED |
| 23 | S6 atom 삭제 후 measurement 자동 제거 | `onAtomsChanged` 동적 검증 | Viewer 미복구로 수행 불가 | BLOCKED |
| 24 | S7 structure 삭제 후 measurement 일괄 제거 | `onStructureRemoved` 동적 검증 | Viewer 미복구로 수행 불가 | BLOCKED |
| 25 | 동일 publisher N 구독자 동시 동작 | atoms_controller + measurement_controller 동시 응답 | Viewer 미복구로 수행 불가 | BLOCKED |
| 26 | 콘솔 오류 0 | DevTools 0 errors | 명시 확인 없음 | NOT VERIFIED |
| 27 | wasm size 회귀 | Phase 3.4 대비 +5~10% 범위 | 명시 측정 없음. release 빌드 통과만 확인 | NOT VERIFIED |

## 2.3 종합 집계

| 구분 | 항목 | 결과 |
|---|---:|---|
| PASS | 18 | #1~#15, #17~#19 |
| PARTIAL | 1 | #16 |
| BLOCKED | 6 | #20~#25 |
| NOT VERIFIED | 2 | #26~#27 |
| FAIL | 0 | 없음 |

> 실패 항목은 없다. 다만 BLOCKED 6개가 Phase 3.5 의 실제 사용자 가치와 가장 가까운 검증 항목이므로, Viewer 복구 후 반드시 별도 확인해야 한다.

---

# Part 3 - Phase 2 인프라 재사용성 평가

## 3.1 MouseInteractor: 동일 publisher N 구독자 패턴

| 항목 | Phase 3.4 | Phase 3.5 | 평가 |
|---|---|---|---|
| 구독자 | `atoms_controller` | `measurement_controller` 추가 | 정적 구조상 두 번째 구독자 도달 |
| EventBus 연결 | `SetEventBus(&scene_.events)` | 동일 | 동일 bus 공유 |
| Render handler | `SetRenderRequestHandler(...)` | 동일 | 같은 render 요청 경로 사용 |
| active structure | `SetActiveStructureId(...)` | 동일 | 동일 구조 context 사용 |
| 동적 검증 | Phase 3.4 에서 일부 확인 | #25 Viewer 미복구로 보류 | BLOCKED |

### 발견

`MouseInteractor` 는 이제 click과 drag를 모두 EventBus로 publish한다. 이 구조는 계획서의 "동일 publisher - N 구독자" 재사용성 시험에 맞는다. 다만 #25 동적 검증은 아직 보류이므로 "정적 통과, 런타임 미확정" 으로 평가한다.

## 3.2 EventBus 추가 구독 분포

| EventBus | Phase 3.5 추가 구독 | 역할 | 평가 |
|---|---|---|---|
| `onAtomPicked` | measurement_controller | atom pick 처리 | PASS |
| `onEmptyClick` | measurement_controller | 현재 pick clear | PASS |
| `onDragSelection` | measurement_controller | center mode drag selection | PASS |
| `onAtomsChanged` | measurement_controller + measurement_store | picked id pruning + measurement refresh/removal | PASS |
| `onStructureRemoved` | measurement_controller + measurement_store | mode exit + measurement 일괄 제거 | PASS |
| `onSelectionChanged` | measurement_controller | pick visual sync | PASS |
| `onStructureVisibilityChanged` | measurement_store | measurement visibility cascade | PASS - 계획 외 보강 |

### 평가

Phase 3.5 는 Phase 3.4 의 "첫 구독자" 검증에서 한 단계 더 나아가, 동일 이벤트에 두 개 이상의 measurement 내부 구독자가 붙는 구조를 구현했다. 특히 `measurement_store` 와 `measurement_controller` 의 역할이 분리되어 있어, atom 변경 시 UI state pruning 과 measurement object lifecycle 이 분리된다.

## 3.3 ElementDatabase mass 사용

| 항목 | 결과 |
|---|---|
| 호출 위치 | `features/measurement/center.cpp` |
| 호출 API | `core::data::ElementDatabase::getInstance().getAtomicMass(atom->symbol)` |
| 계획서와 차이 | 계획서는 `getElementInfo(symbol)->atomicMass` 를 예시로 들었으나, 실제 core API 의 `getAtomicMass` 를 사용 |
| 평가 | 의미상 동등. Center of Mass 의 mass-weighted 계산 경로 존재 |

---

# Part 4 - 구현 품질 및 리스크

## 4.1 잘된 점

| 항목 | 평가 |
|---|---|
| 폴더/파일 정책 | 계획서의 단일 sub-folder + 18파일 구조를 정확히 따름 |
| legacy 격리 | `features/measurement` 내부 legacy/vtk_renderer 참조 0 |
| 빌드 안정성 | debug/release wasm 빌드 모두 통과 |
| EventBus 재사용 | 신규 3 이벤트 + 기존 이벤트 추가 구독이 명확히 분리됨 |
| measurement lifecycle | visibility, atom/structure removal, actor attach/detach 경로 존재 |
| Center of Mass | atomic mass API 사용으로 계획서 핵심 요구 충족 |

## 4.2 주요 리스크

| # | 리스크 | 영향 | 권장 보강 |
|---|---|---|---|
| R1 | Viewer 미복구로 #20~#25 미검증 | 측정 기능의 사용자 가치 검증이 아직 불완전 | Viewer 복구 후 S1~S7 재검증 및 v3 또는 보강 평가서 작성 |
| R2 | `AtomPickedEvent` 가 `atomId` 를 직접 포함하지 않고 pick position 기반 nearest atom 을 재해석 | actor pick과 atom id 매핑이 복잡한 scene 에서 오검출 가능 | Viewer 복구 후 pick 정확도 확인. 필요 시 actor-to-atom map 또는 picker metadata 추가 |
| R3 | Drag selection 좌표계는 런타임 미검증 | center mode 다중 선택 실패 가능 | #22/#25에서 viewport y-axis 및 additive 동작 확인 |
| R4 | `onAtomsChanged` 때 measurement actors 를 rebuild | 대형 구조 + measurement 다수에서 비용 증가 가능 | Phase 4 이후 event kind 또는 incremental refresh 검토 |
| R5 | Distance unit 이 `A` 로 표기 | legacy `Angstrom` 기호와 1:1 시각 동등성 이슈 | UI 정책 결정 필요. ASCII 유지 또는 명시적 Angstrom 표기 허용 중 선택 |
| R6 | `measurement_controller` 가 pick state, event subscription, VTK pick visuals 를 함께 가짐 | controller 비대화 가능 | Phase 4 안정화 시 PickVisual helper 분리 검토 |
| R7 | 콘솔 오류 및 wasm size 미측정 | 런타임 품질/용량 회귀 판단 불완전 | DevTools 확인 및 wasm artifact size 비교 추가 |

## 4.3 Intentional / 의미상 동등 deviation

| # | 항목 | 위치 | 평가 |
|---|---|---|---|
| D1 | `getElementInfo()->atomicMass` 대신 `getAtomicMass()` 사용 | `center.cpp` | core API 활용. 의미상 동등 |
| D2 | `MeasurementMode` 와 `MeasurementType` 분리 | `measurement_mode.h` | mode state 와 persisted object type 분리로 안전성 증가 |
| D3 | `onStructureVisibilityChanged` 구독 추가 | `measurement_store.cpp` | 계획서보다 보강된 visibility cascade |
| D4 | Center mode 명시 Apply 버튼 및 Enter shortcut | `measurement_overlay_ui.cpp` | N atom selection workflow 명확화 |
| D5 | Measurement mode 진입 시 list window 표시 | `measurement_menu.cpp` | 사용자가 생성 결과를 즉시 확인하기 쉬움 |
| D6 | Distance unit `A` 표기 | `distance.cpp` | ASCII 정책에는 부합하나 legacy 시각 동등성은 PARTIAL |

---

# Part 5 - 저장소 상태

## 5.1 현재 변경 범위

| 경로 | 상태 | 내용 |
|---|---|---|
| `CMakeLists.txt` | M | measurement 18 source 등록 |
| `webassembly/src/app/app.cpp` | M | measurement include/init/menu/render hook |
| `webassembly/src/core/scene/events.h` | M | 신규 이벤트 3종 + Bus 3개 |
| `webassembly/src/core/vtk/mouse_interactor.{cpp,h}` | M | click/drag/pick event 발행 보강 |
| `webassembly/src/features/measurement/` | ?? | 신규 18파일 |
| `webassembly/docs/phases/phase3_5_measurement.md` | ?? | 계획서 untracked 상태 |
| `webassembly/docs/phases/phase3_5_evaluation_2026-05-11.md` | ?? | 기존 평가서 untracked 상태 |
| `webassembly/docs/phases/phase3_5_evaluation_2026-05-11_v2.md` | ?? | 본 평가서 |

## 5.2 빌드 검증

| 명령 | 결과 |
|---|---|
| `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"` | PASS |
| `cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"` | PASS |

### 참고

PowerShell 의 `npm.ps1` 실행 정책 때문에 `npm run ...` 직접 실행은 차단되었다. 이후 `npm.cmd` 와 `emsdk_env.bat` 를 사용해 debug/release 빌드를 완료했다.

---

# Part 6 - 종합 평가표

| 평가 축 | 점수 | 근거 |
|---|---:|---|
| 계획서 구조 적합성 | 5/5 | 18파일, app 4 hook, CMake 18 entries 충족 |
| legacy 격리 | 5/5 | legacy/vtk_renderer 참조 0 |
| EventBus/MouseInteractor 재사용성 | 4/5 | 정적 구조 통과. #25 동적 검증 보류 |
| Measurement 계산 구현 | 4/5 | Distance/Angle/Dihedral/Center/COM 구현. 수치 동등성 런타임 검증 보류 |
| UI 보존 | 3/5 | overlay/list 구현. side-by-side 미검증 및 distance unit deviation |
| 빌드 안정성 | 5/5 | debug/release 통과 |
| 동적 검증 커버리지 | 2/5 | #17~#19 만 확인, #20~#25 Viewer 차단 |
| 리스크 관리 | 4/5 | BLOCKED 항목을 FAIL 로 혼동하지 않고 명확히 분리 |
| 종합 | 조건부 GO | Viewer 복구 후 Phase 3.5 visual scenarios 재검증 필요 |

---

# Part 7 - 결론 및 권장 다음 단계

## 7.1 결론

> Phase 3.5 는 코드 구조, 이벤트 인프라, 메뉴 wiring, 빌드 검증 측면에서 계획서의 핵심 요구를 충족했다. `features/measurement/` 18파일은 legacy 의존 없이 독립적으로 구성되었고, MouseInteractor 와 EventBus 는 계획서가 의도한 재사용성 검증 단계까지 정적으로 도달했다.
>
> 그러나 Viewer 미복구로 인해 Phase 3.5 의 가장 중요한 사용자 시나리오 검증인 #20~#25 는 수행되지 못했다. 따라서 본 단계는 **구현 완료 + 정적/빌드 통과 + 런타임 메뉴 확인 완료 + visual scenario 검증 보류** 상태로 평가한다.

## 7.2 권장 다음 단계

1. **Viewer 복구 후 #20~#25 재검증**
   Side-by-side screenshot, S1~S7, 동일 publisher N 구독자 동시 동작을 최우선으로 확인한다.

2. **#16 UI 보존 보강**
   distance unit 표기(`A` vs Angstrom 기호), overlay 위치, label 스타일, center marker 시각 동등성을 legacy 와 비교한다.

3. **pick/drag 정확도 검증**
   `AtomPickedEvent` 의 pick position 기반 nearest atom 해석과 drag selection 좌표계를 실제 Viewer에서 확인한다.

4. **콘솔/wasm size 보강**
   #26 DevTools console 0, #27 wasm artifact size 비교를 별도 기록한다.

5. **Phase 3.5 commit 정리**
   구현 파일, core 보강, app/CMake wiring, 계획서, 본 평가서를 하나의 Phase 3.5 단위로 정리한다.

---

## 부록 A - 검증 명령 요약

```powershell
rg --files webassembly\src\features\measurement
rg -n "legacy|vtk_renderer|#include.*legacy" webassembly\src\features\measurement
rg -n "features::measurement|measurement_menu" webassembly\src\app\app.cpp
rg -n "features/measurement/" CMakeLists.txt
rg -n "AtomPickedEvent|DragSelectionEvent|EmptyClickEvent" webassembly\src\core\scene\events.h
rg -n "OnLeftButtonUp|OnMouseMove|dragging_" webassembly\src\core\vtk\mouse_interactor.cpp webassembly\src\core\vtk\mouse_interactor.h
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"
```

## 부록 B - 후속 평가서 작성 조건

Viewer 복구 후 #20~#25 가 수행되면 다음 항목을 추가한 보강 평가서를 작성한다.

| 항목 | 추가 기록 |
|---|---|
| #20 | legacy/new side-by-side screenshot 결과 |
| #21 | Distance/Angle/Dihedral 수치 및 actor 표시 결과 |
| #22 | Geometric Center/Center of Mass drag selection 및 mass-weighted 차이 확인 |
| #23 | atom 삭제 후 measurement 자동 제거 |
| #24 | structure 삭제 후 measurement 일괄 제거 |
| #25 | atoms_controller + measurement_controller 동시 동작 |
| #26 | DevTools console errors |
| #27 | wasm size delta |
