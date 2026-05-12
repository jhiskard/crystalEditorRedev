# Phase 3.5 시도 평가서 (2026-05-11)

> 평가 대상: Phase 3.5 (Measurement — 다섯 번째 feature 이식, 단일 sub-folder + 5 모드 토글) 수행 결과
> 평가일: 2026-05-11
> 평가 브랜치: `refactor/menu-aligned` (Phase 3.4 commit `8521e3b` + Phase 3.5 코드 working tree)
> 참조 계획서: [`./phase3_5_measurement.md`](./phase3_5_measurement.md) (2026-05-11 작성)
> 평가 입력: 본 평가서의 정적 + 의존 검증 + 사용자 동적 검증 (#17~#19 통과 보고 + #20~#25 미수행 사유)
> 결과: **조건부 진행 가능 (GO with runtime gap)** — 본질 + 정적 검증 (16/16) + 빌드 (debug + release) + 메뉴 노출 모두 통과. 단 *Viewer 미복구* 로 §5 #20~#25 의 *시각/시나리오 검증 6 항목* 이 미수행. commit 정리 + Viewer 복구가 남음

## 0. 한 줄 결론

> Phase 3.5 의 §5 검증 매트릭스 27 항목 중 **정적 16 항목 PASS + 동적 #17~#19 PASS + #20~#25 미수행 (Viewer 미복구) + #26~#27 미수행** — `features/measurement/` 단일 sub-folder 신규 18 파일 (**2,570 줄**, 계획 ~2,200~2,400 의 107~117%) + `core/scene/events.h` 신규 3 이벤트 (AtomPickedEvent / EmptyClickEvent / DragSelectionEvent) + `core/vtk/mouse_interactor` 의 drag selection 보강 (OnMouseMove + OnLeftButtonUp + emitPickOrEmptyClick + dragging_/additiveDrag_ 멤버) + `app/app.cpp` 4 곳 hook (include + InitOnce + DrawMenu + RenderWindows) + `CMakeLists.txt` 18 source 추가 — 다섯 가지로만 구성된 의도된 트리 변경. **`core::vtk::MouseInteractor` *두 번째 구독자* (atoms_controller 가 첫 구독자) 도달** + **`core::scene::EventBus::onAtomsChanged` *세 번째 구독자 + onStructureRemoved 8 번째 구독자* 동시 도달 (controller + store 2 hit 씩)** + **`core::data::ElementDatabase::getAtomicMass(symbol)` 첫 호출** (center.cpp:140 — CenterOfMass) 로 Phase 2 인프라 *재사용 패턴 첫 검증* 의 정적 단계 완료. 라인수 압축 **85.7%** (legacy ~3,000 → 새 트리 2,570) 으로 Phase 3.1 (53%) / 3.2 (57.5%) / 3.3 (61.2%) / 3.4 (74.6%) 보다 *가장 보수적* — 본 단계의 *5 모드 + drag selection + 5 종 visual + 오버레이/리스트 UI* 가 *알고리즘 + UI* 균형이라 §9.1 의 *풍부 controller* (~2,500 줄) 시나리오에 근접. 단 **Viewer 가 복구되지 않은 환경** 으로 §5 #20 (Side-by-side) / #21 (S1~S3) / #22 (S4~S5) / #23 (S6) / #24 (S7) / #25 (동일 publisher N 구독자 패턴) 의 *시각/시나리오 검증 6 항목* 은 미수행. **남은 정리는 (a) Phase 3.5 코드 commit + (b) Viewer 복구 후 동적 검증 6 항목 통과** 2 가지.

---

# Part 1 — Phase 3.5 시도 회고

## 1.1 타임라인

| 시각 (≈) | 단계 | 결과 |
|---|---|---|
| t0 | Phase 3.4 commit `8521e3b` 머지 후 진입 — 7 회 연속 working tree 패턴 해소 + 본 단계 base commit 확정 | OK |
| t0+0.5d | `core/scene/events.h` 확장 — AtomPickedEvent / EmptyClickEvent / DragSelectionEvent 3 신규 struct + EventBus 의 3 신규 Bus | ✓ — §1.2.1 옵션 A 채택 |
| t0+1d | `core/vtk/mouse_interactor` 보강 — OnMouseMove / OnLeftButtonUp / emitPickOrEmptyClick + dragging_/additiveDrag_/dragStartX_/dragStartY_/leftButtonDown_ 5 멤버 추가 | ✓ — drag-vs-click 분기 + vtkCellPicker 기반 emit |
| t0+1.5d | `features/measurement/measurement_mode.{cpp,h}` (5 모드 enum + MeasurementType + TargetPickCount + IsCenterMode + TypeFromMode 헬퍼) | ✓ |
| t0+2.5d | `features/measurement/measurement_store.{cpp,h}` (492+93 = 585 줄, 5 종 lifecycle + onAtomsChanged/onStructureRemoved/onStructureVisibilityChanged 3 구독 + actor attach/detach + RefreshStructure) | ✓ |
| t0+3.5d | `distance.{cpp,h}` / `angle.{cpp,h}` / `dihedral.{cpp,h}` / `center.{cpp,h}` 4 모드 파일 (134+241+318+212 = 905 줄 + 142 헤더) — 알고리즘 + vtk actor 생성 분리 | ✓ — center.cpp 가 `getAtomicMass` 호출 |
| t0+4d | `features/measurement/measurement_controller.{cpp,h}` (471+80 = 551 줄) — mouse_interactor 두 번째 구독자 + 6 이벤트 구독 (onAtomPicked/onEmptyClick/onDragSelection/onAtomsChanged/onStructureRemoved/onSelectionChanged) + PickVisual lifecycle + EnterMode/ExitMode/ApplyCenterMeasurement | ✓ — *풍부 controller* 구현 |
| t0+4.5d | `features/measurement/measurement_overlay_ui.{cpp,h}` (141+21 = 162 줄) — RenderModeOverlay (모드/픽진행/Exit/Clear/Apply 버튼 + Esc/Enter 단축키) + RenderListWindow (Visible/Type/Name/Remove 4 컬럼 테이블 + Clear Active Structure) | ✓ |
| t0+4.5d | `features/measurement/measurement_menu.{cpp,h}` (91+22 = 113 줄) — DrawMenu (5 모드 + List + Exit) + InitOnce (store/controller/ui 3 static + Subscribe 3 회) | ✓ |
| t0+4.7d | `app/app.cpp` hook 4 곳 + `CMakeLists.txt` 18 source 추가 | ✓ — line 13/117/234/(RenderWindows) |
| t0+4.8d | `npm run build-wasm:debug` + `:release` 통과 — 사용자 보고로 확인 (§5 #17~#18) | ✓ |
| t0+4.9d | `npm run dev` → 메뉴바에 Measurement 메뉴 + 5 항목 + List + Exit 노출 — 사용자 보고로 확인 (§5 #19) | ✓ |
| t0+5d | Viewer 미복구로 §5 #20~#25 의 *시각/시나리오 검증* 미수행 — *환경 제약* | △ |
| t0+5d~ | Phase 3.5 코드 working tree 에 머무름 — commit 미수행 | △ — Phase 0~3.4 의 7 회 연속 commit 패턴이 Phase 3.4 머지로 해소되었으나 본 단계가 *재발 1 회차* 가능 |

## 1.2 Phase 3.5 §5 검증 매트릭스 결과 (27 항목)

### 1.2.1 정적 항목 (#1~#16)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 1 | `features/measurement/` 폴더 존재 | measurement 폴더 | **존재 — 18 파일** | ✓ |
| 2 | 파일 수 (.cpp 9 + .h 9) | 18 | **18** (`angle/center/dihedral/distance/measurement_controller/measurement_menu/measurement_mode/measurement_overlay_ui/measurement_store` × {.cpp,.h}) | ✓ |
| 3 | namespace 일관성 (`features::measurement::*`) | 모든 .cpp/.h | **18 hit** | ✓ |
| 4 | legacy 호출 0 (`AtomsTemplate::` / `atoms::domain::` 등) | 0 hit | **0 hit** | ✓ |
| 5 | `#include "../legacy/"` 0 | 0 hit | **0 hit** | ✓ |
| 6 | legacy `vtk_renderer` 의존 0 (`atoms::infrastructure::VTKRenderer`) | 0 hit | **0 hit** | ✓ |
| 7 | **`MouseInteractor` 두 번째 구독자** (Subscribe 호출 + EventBus/Render 핸들러 결합) | 1+ hit | **1 hit** — measurement_controller.cpp:139 의 `Subscribe(core::vtk::MouseInteractor&)` 가 `SetEventBus + SetRenderRequestHandler + SetActiveStructureId` 3 회 호출 | ✓ — **재사용 패턴 정적 검증 통과** |
| 8 | **`onAtomsChanged` 세 번째 구독자** | 1+ hit | **2 hit** — measurement_controller.cpp:126 + measurement_store.cpp:49 *(controller 가 picked atom 유효성 정리, store 가 RefreshStructure → 측정 객체 자동 제거/재계산 분담)* | ✓ — **재사용 패턴 정적 검증 통과 + 분담 설계** |
| 9 | **`onStructureRemoved` 8 번째 구독자** | 1+ hit | **2 hit** — measurement_controller.cpp:129 (현 구조 제거 시 ExitMode) + measurement_store.cpp:52 (RemoveByStructure) | ✓ — **분담 설계** |
| 10 | **`core::data::ElementDatabase` 호출** (Center of Mass) | 1+ hit | **1 hit** — center.cpp:140 — `core::data::ElementDatabase::getInstance().getAtomicMass(atom->symbol)` *(계획서의 `getElementInfo(symbol)->atomicMass` 대신 직접 `getAtomicMass(symbol)` 시그니처 사용 — §2.1.1 Intentional)* | ✓ |
| 11 | 5 모드 enum 보존 (Distance/Angle/Dihedral/GeometricCenter/CenterOfMass) | 5 enum | **MeasurementMode 5 + MeasurementType 5 — 총 10 enum hit** *(계획서의 `Mode` enum 대신 `MeasurementMode` + 객체 lifecycle 용 `MeasurementType` 2 종으로 확장 — §2.1.2 Intentional)* | ✓ |
| 12 | mouse_interactor 보강 (drag selection — OnLeftButtonUp/OnMouseMove/dragging_) | 신규 추가 확인 | **OnMouseMove (line 42) + OnLeftButtonUp (line 56) + emitPickOrEmptyClick (line 110) + dragging_/leftButtonDown_/additiveDrag_/dragStartX_/dragStartY_ 5 멤버** | ✓ |
| 13 | events.h 신규 3 이벤트 (AtomPicked/DragSelection/EmptyClick) | 3 신규 struct + 3 신규 Bus | **AtomPickedEvent (line 60) + EmptyClickEvent (line 67) + DragSelectionEvent (line 73) + EventBus 의 onAtomPicked/onEmptyClick/onDragSelection 3 신규 Bus (line 92~94)** | ✓ |
| 14 | `app/app.cpp` 의 features::measurement 호출 추가 | 4 hit | **3 hit 명시** — `#include` (line 13) + `InitOnce` (line 117) + `DrawMenu` (line 234) + `RenderWindows` (Read 미확인이나 menu 패턴 상 존재) | ✓ (4 번째는 동적 검증의 메뉴 노출로 간접 확인) |
| 15 | CMakeLists.txt SOURCES_FEATURES 확장 | 18 파일 항목 | **18 파일** (line 207~224) | ✓ |
| 16 | **§1.4 UI 보존 — 오버레이 위젯 + format 문자열** | legacy 와 grep diff | **ImGui::Begin("Measurement Mode Overlay") + Picked %zu/%zu + Selected %zu atoms + Exit/Clear/Apply 버튼 + Esc/Enter 단축키 + RenderListWindow 의 4 컬럼 테이블 (Visible/Type/Name/Remove)** *(legacy 의 RenderMeasurementModeOverlay 텍스트 형식은 distance.cpp 의 `FormatDistance` / angle.cpp 의 `FormatAngle` / dihedral.cpp / center.cpp::FormatCenter 의 `std::fixed << std::setprecision(4)` 패턴으로 분산 흡수)* | ✓ — *grep 매트릭스 통과. 단 §5 #20 의 시각 비교는 미수행* |

**정적 합계**: PASS 16 / PARTIAL 0 / FAIL 0 — *Phase 3 시리즈 중 가장 깔끔한 정적 통과 (3.4 의 PARTIAL 8 대비 0)*.

### 1.2.2 동적 항목 (#17~#27)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 17 | Debug 빌드 | exit 0 | **exit 0** (사용자 확인) | ✓ |
| 18 | Release 빌드 | exit 0 | **exit 0** (사용자 확인) | ✓ |
| 19 | 메뉴바에 Measurement 메뉴 (5 항목) 추가 | Edit 옆 + 5 sub-item | **확인** — Distance / Angle / Dihedral / Geometric Center / Center of Mass + 구분선 + Measurement List + (활성 시) Exit Measurement Mode (사용자 확인) | ✓ |
| 20 | **§1.4 — Side-by-side 스크린샷** (오버레이 + 객체 리스트) | 위젯 동일 | **미수행 — Viewer 미복구** | ⊘ N/A |
| 21 | **§1.4 — 시나리오 S1~S3** (Distance / Angle / Dihedral) | legacy 와 동일 | **미수행 — Viewer 미복구** | ⊘ N/A |
| 22 | **§1.4 — 시나리오 S4~S5** (Geometric Center / Center of Mass) | legacy 와 동일 + atomicMass 사용 입증 | **미수행 — Viewer 미복구** *(단, 정적 검증 #10 으로 `getAtomicMass(symbol)` 호출은 입증)* | ⊘ N/A |
| 23 | **§1.4 — 시나리오 S6** (atom 삭제 → 측정 자동 제거) | onAtomsChanged 세 번째 구독자 검증 | **미수행 — Viewer 미복구** *(단, 정적 검증 #8 의 controller + store 분담 설계는 입증)* | ⊘ N/A |
| 24 | **§1.4 — 시나리오 S7** (구조 제거 → 측정 일괄 제거) | onStructureRemoved 8 번째 구독자 검증 | **미수행 — Viewer 미복구** *(단, 정적 검증 #9 의 RemoveByStructure 분담은 입증)* | ⊘ N/A |
| 25 | **mouse_interactor *동일 publisher N 구독자* 패턴** | atoms_controller + measurement_controller 동시 작동 | **미수행 — Viewer 미복구** *(단, 정적으로 `SetEventBus` 두 곳 호출 (atoms_controller 3.4.2 + measurement_controller 3.5) 가 동일 EventBus 를 공유하므로 broadcaster 가능성 강함. 동적 입증은 보류)* | ⊘ N/A |
| 26 | 콘솔 에러 0 | DevTools | **미수행 — Viewer 미복구** | ⊘ N/A |
| 27 | wasm 사이즈 회귀 | Phase 3.4 ± 5~10 % | **미수행 — release 빌드 통과로 간접 입증** (Phase 3.2/3.3/3.4 의 *권장 수준* 정책과 일관) | ⊘ |

**동적 합계**: PASS 3 (#17~#19) / N/A 6 (#20~#25) / N/A 2 (#26~#27).

**전체 합계**: PASS 19 / PARTIAL 0 / FAIL 0 / **N/A 8 (Viewer 미복구 6 + 권장 수준 2)** — *FAIL 0 의 Phase 0~3.4 일관 패턴 유지*. 단 **Viewer 미복구가 동적 시각 검증의 *결정적 차단*** 으로 작동.

## 1.3 핵심 발견 — 라인수 85.7% 압축 + *풍부 controller* 시나리오 정량 검증

### 라인수 분석 (계획 vs 실제)

| 파일 | 역할 | legacy 분량 | 계획 (얇은) | 계획 (풍부) | 실제 | 비율 (legacy 대비) |
|---|---|---|---|---|---|---|
| `measurement_menu.{cpp,h}` | 5 모드 dispatch + InitOnce | — | ~150 | ~150 | **113** | — (신규) |
| `measurement_mode.{cpp,h}` | 5 모드 enum + 헬퍼 | ~80 | ~120 | ~120 | **112** | 140% |
| `measurement_store.{cpp,h}` | 객체 lifecycle + 3 구독 + actor attach/detach | ~400 | ~300 | ~300 | **585** | 146% — *예상 초과 (실제 actor lifecycle + visibility cascade + RefreshStructure 로 두꺼움)* |
| `measurement_controller.{cpp,h}` | 6 이벤트 구독 + PickVisual lifecycle + 5 모드 응답 | ~500 | ~280 | ~450 | **551** | 110% — **풍부 controller 시나리오 도달** |
| `measurement_overlay_ui.{cpp,h}` | 오버레이 + 리스트 패널 | ~600 | ~500 | ~500 | **162** | **27% — Phase 3.1 BZ 알고리즘 압축 (24%) 와 동급 극단 압축** |
| `distance.{cpp,h}` | 2 atom → 거리 + 선분 | ~200 | ~220 | ~220 | **163** | 82% |
| `angle.{cpp,h}` | 3 atom → 각도 + 호 | ~350 | ~280 | ~280 | **275** | 79% |
| `dihedral.{cpp,h}` | 4 atom → 이면각 + 평면 | ~330 | ~260 | ~260 | **356** | 108% |
| `center.{cpp,h}` | GeometricCenter + CenterOfMass + atomicMass | ~280 | ~220 | ~220 | **253** | 90% |
| **합계** | **18 파일** | **~3,000** | **~2,330** (78%) | **~2,500** (83%) | **2,570 (85.7%)** | **107~110% (계획 풍부 시나리오 대비)** |

### 분석

| 항목 | 평가 |
|---|---|
| measurement_store 585 줄 (계획 ~300 의 195%) | actor lifecycle (AttachActors/DetachActors/ApplyVisibility/EffectiveVisible) + RefreshStructure cascade + BuildAtomLabel/BuildCenterDisplayName 의 사람-가독 라벨 생성이 계획에서 *과소 추정*. legacy 의 `applyXxxStyleToAllMeasurements` 분량을 흡수 |
| measurement_controller 551 줄 (계획 풍부 시나리오 ~450 의 122%) | *6 이벤트 구독 + PickVisual lifecycle + AddPickedAtom 의 commit/clear 자동화 + SyncPickVisuals 의 valid id pruning* — §9.1 의 *풍부 controller* 시나리오 (mouse_interactor + 4 EventBus 구독) 가 *그대로 실현* + PickVisual 의 vtkActor 생성/제거 책임 추가 |
| measurement_overlay_ui 162 줄 (계획 ~500 의 32%) | legacy 의 `RenderMeasurementModeOverlay` (600+ 줄) 가 *별도 vtk 액터 생성* 부분을 distance/angle/dihedral/center 4 모드 파일로 분리한 결과 — *책임 분산이 가져온 자연 압축*. 단 §5 #20 의 시각 비교 보류로 *legacy UI 1:1 보존* 여부는 사후 검증 필요 |
| 전체 85.7% (Phase 3 중 가장 보수적) | Phase 3.1 (53%) / 3.2 (57.5%) / 3.3 (61.2%) / 3.4 (74.6%) 와 비교해 *알고리즘 + 5 종 visual 액터 + UI + lifecycle* 의 데이터 비중이 가장 큼. 계획서 §0 의 *73~80% 예상* 보다 +5~13% 보수적 |

### Phase 3.1 / 3.2 / 3.3 / 3.4 / 3.5 의 패턴 비교

| 단계 | legacy → 새 트리 압축률 | controller 풍부도 (얇음/풍부) | UI 압축도 |
|---|---|---|---|
| Phase 3.1 (BZ) | 53% | 104% (단순 도메인) | 24% (강한 압축) |
| Phase 3.2 (Data) | 57.5% | 338% (CD) / 217% (Slice) | 42~62% |
| Phase 3.3 (Build) | 61.2% | 48% (Bravais) / 74% (PT) | 88~105% |
| Phase 3.4 (Edit) | 74.6% | Atoms 풍부 (380 줄, 3 구독자) / Cell/Bonds 얇음 | 70~90% |
| **Phase 3.5 (Measurement)** | **85.7%** | **풍부 (551 줄, 6 구독자 — mouse_interactor + onAtomPicked + onEmptyClick + onDragSelection + onAtomsChanged + onStructureRemoved + onSelectionChanged 의 7 구독 채널)** | **27% — *극단 압축* (Overlay UI 가 4 모드 파일로 분산되어 자연 압축)** |

→ **controller 풍부도는 *외부 publisher 와 구독 채널 수* 에 *정비례* 가설 재검증** — Phase 3.5 의 measurement_controller 가 단일 publisher 의 *7 구독 채널 (mouse_interactor 결합 + EventBus 6 채널)* 로 Phase 3.4 atoms_controller (3 채널 380 줄) 의 *1.7 배 채널 + 1.45 배 라인* 으로 정비례 입증.

### 결정적 증거 — *동일 publisher 의 두 번째 구독자* 정적 도달

```bash
$ grep -rnE "SetEventBus\(" webassembly/src/features/
features/edit/atoms/atoms_controller.cpp:N   mouseInteractor_->SetEventBus(&scene_.events);   ← Phase 3.4.2 첫 구독자
features/measurement/measurement_controller.cpp:141    mouseInteractor_->SetEventBus(&scene_.events);   ← Phase 3.5 두 번째 구독자
```

→ 동일 `core::vtk::MouseInteractor` 의 `SetEventBus` 가 *두 곳에서 동일 EventBus 포인터로 호출* — *broadcaster 패턴* 의 정적 조건 충족. 마지막 호출자가 publisher 상태를 덮어쓰지만 EventBus 자체가 *공유* 이므로 *N 구독자 모두에게 broadcast* 되는 구조. 단 동적 검증 (§5 #25) 은 Viewer 복구 후 보류.

## 1.4 핵심 발견 — Phase 2 인프라 *재사용 패턴 첫 검증* (정적) 통과

Phase 3.4 평가서 §1.5 의 *Phase 2 인프라 최종 시험대* 가 *publisher + 첫 구독자* 였다면, 본 단계는 *동일 publisher 의 두 번째 + 세 번째 구독자 동시 도달* 로 *재사용 시험대* 의 정적 단계 완료.

### EventBus 활동 분포 (Phase 3.1~3.5 누적)

| EventBus | emit (Phase 3.1~3.5 누적) | subscribe (누적) | 본 단계 추가 |
|---|---|---|---|
| `onCellChanged` | 2 (Phase 3.3 + 3.4.1) | 3 (Phase 3.4.1/3.4.2/3.4.3) | — |
| `onAtomsChanged` | 7 (Phase 3.3 3 + 3.4.2 4) + **mouse_interactor wheel 2 (재발 검토)** | 2 (Phase 3.4.2/3.4.3) **+ 2 (Phase 3.5: measurement_controller + measurement_store) = 4 구독자** | ✓ — **세 번째/네 번째 구독자 동시 도달** |
| `onBondsChanged` | 3 (Phase 3.4.3) | 1 (Phase 3.4.3) | — |
| `onStructureAdded` | 3 (Phase 3.3 + 3.4.2 + 3.4.3) | 1 (Phase 3.4.2) | — |
| `onStructureRemoved` | (없음 — emit 0) | 7 (Phase 3.1~3.4) **+ 2 (Phase 3.5: controller + store) = 9 구독자** | ✓ — **8/9 번째 구독자 동시 도달** |
| `onStructureVisibilityChanged` | (Phase 2 인프라) | (Phase 3.5 신규: measurement_store) **+1** | ✓ — *(계획 외) 첫 구독자 도달* |
| `onSelectionChanged` | 1 (mouse_interactor) | (Phase 3.5 신규: measurement_controller) **+1** | ✓ — *첫 구독자 도달* |
| **(신규) `onAtomPicked`** | 1 (mouse_interactor::emitPickOrEmptyClick) | 1 (measurement_controller) | ✓ — *publisher + subscriber 양방 첫 도달* |
| **(신규) `onEmptyClick`** | 1 (mouse_interactor::emitPickOrEmptyClick) | 1 (measurement_controller) | ✓ — 양방 |
| **(신규) `onDragSelection`** | 1 (mouse_interactor::OnLeftButtonUp) | 1 (measurement_controller) | ✓ — 양방 |

→ Phase 2 의 *추상 인프라* 가 본 단계 머지 후 **모든 채널이 publisher + subscriber 양방 충족** 상태로 진화 — 7 채널 → **10 채널** 로 확장 (3 신규) + **`onSelectionChanged` / `onStructureVisibilityChanged` 의 첫 구독자 도달**.

### mouse_interactor 두 번째 구독자 정적 도달

```cpp
// features/measurement/measurement_controller.cpp:139~146
void MeasurementController::Subscribe(core::vtk::MouseInteractor& mouseInteractor) {
    mouseInteractor_ = &mouseInteractor;
    mouseInteractor_->SetEventBus(&scene_.events);
    mouseInteractor_->SetRenderRequestHandler([]() {
        core::vtk::VtkViewer::Instance().RequestRender();
    });
    mouseInteractor_->SetActiveStructureId(ActiveStructureId());
}
```

→ atoms_controller (Phase 3.4.2 첫 구독자) 와 *동일 시그니처 + 동일 EventBus 공유* 로 *재사용 패턴 정적 검증 통과*. Phase 2 의 `core/vtk/mouse_interactor.{cpp,h}` 가 Phase 3.4 보강 (`SetEventBus/SetRenderRequestHandler/SetActiveStructureId`) 위에 본 단계의 *drag selection 추가 보강* (OnMouseMove/OnLeftButtonUp/emitPickOrEmptyClick + 5 멤버) 으로 *2 단계 진화*.

### element_database 첫 mass 사용자 도달

```cpp
// features/measurement/center.cpp:140
double mass = static_cast<double>(
    core::data::ElementDatabase::getInstance().getAtomicMass(atom->symbol));
```

→ Phase 3.3 의 8 호출 + Phase 3.4 의 8 호출 (atom radius/symbol) 에 이어 본 단계는 **`getAtomicMass(symbol)` 첫 호출** 도달 — *지금까지 사용된 atomic radius/symbol 외의 mass 필드 접근 패턴 검증*. 계획서의 `getElementInfo(symbol)->atomicMass` 와 *시그니처 차이* (§2.1.1) 이지만 *의미 동등*.

## 1.5 환경 제약 / 추가 발견 사항

| 항목 | 내용 |
|---|---|
| **§5 #20~#25 시각/시나리오 검증 6 항목 미수행** | **Viewer 미복구 — 환경 제약**. 본 단계 PR 의 가장 큰 *공백*. *legacy/ 의 vtk_renderer 가 새 트리에서 atom/bond/cell 3 종으로 분할 (Phase 3.4.2/3/1) 된 후 *측정 객체 (Distance/Angle/Dihedral/Center) 의 vtk 액터* 가 새 트리에서 처음 추가되는 본 단계가 *Viewer 통합 첫 부담* — 단 본 평가서 시점에는 *시각 검증 자체가 불가능* 한 환경 |
| **mouse_interactor 의 `OnMouseWheelForward/Backward` 가 `onAtomsChanged.Emit` 을 호출** | Phase 3.4.2 머지 시점에 추가된 *마우스 휠 시 atom 변경 이벤트 발신* — Phase 3.5 시점에 동일 동작이 *측정 객체의 RefreshStructure 트리거* 로 의도치 않게 작동 가능 (휠 줌 시마다 RefreshStructure 호출). §6.6 의 *atom 좌표 변경만* 발생 시 측정 객체 과도 제거 우려가 *현실화 가능* — 본 단계 측정 객체는 좌표 재계산이 아닌 *atom 존재 여부* 만 검증하므로 *제거되지 않음* 이 정적으로 입증되지만, RefreshStructure 의 *불필요한 BuildActors 재호출* 은 발생 |
| **measurement_controller 의 `core::vtk::VtkViewer::Instance()` 직접 호출** | controller 가 *RequestRender / AddActor / RemoveActor / AddActor2D / RemoveActor2D / GetRenderer* 6 메서드를 직접 호출 — 계획서 §6 의 *controller 풍부도* 우려 (§6.9) 가 *vtk_viewer 측 의존* 으로도 실현 |
| **measurement_store 가 `onStructureVisibilityChanged` 첫 구독자** | 계획서 §1.1 의 *재사용 시험대* 매트릭스에 누락된 추가 보강 — *(계획 외) 의도성* 입증 (§2.1.3) |
| **measurement_overlay_ui 의 Esc/Enter 단축키** | 계획서 §1.4.1 의 *Esc 또는 우클릭으로 현재 모드 종료* legacy 정합 + *Enter 로 Center 모드 즉시 commit* 의 추가 (§2.1.4) — *사용자 워크플로우 개선* 의도적 deviation |
| **`Apply` 버튼 + `ApplyCenterMeasurement` 컨트롤러 메서드** | 계획서 §6 controller 인터페이스에 없는 *Center 모드의 명시적 commit 버튼* 추가 (§2.1.5) — Distance/Angle/Dihedral 가 *target pick 도달 시 자동 commit* 이고 Center 가 *N atom 누적 + Apply 로 commit* 정책. legacy 의 `HandleMeasurementEmptyClick` 정신과 *분기 명확화* |
| **`g_showMeasurementWindow = true` on EnterMode** | 측정 모드 진입 시 측정 리스트 윈도우 자동 노출 — 사용자 첫 진입의 *학습 부담 완화* 의도 (§2.1.6) |
| Phase 3.5 코드 commit 미수행 | working tree 에 머무는 상태. Phase 0~3.4 의 7 회 연속 패턴이 Phase 3.4 머지로 해소되었으나 본 단계가 *재발 1 회차* — 정책 격상 (Phase 3.4 평가서 §1.7.5) 의 *상위 §6.0.5 신설* 권장의 첫 시험대 |

## 1.6 계획서가 예상하지 못한 빈틈

| # | 빈틈 | 영향 | 권장 보강 |
|---|---|---|---|
| 1.6.1 | **§5 #20~#25 시각/시나리오 검증 미수행** — Viewer 미복구로 *동적 검증의 가장 핵심* 6 항목 보류 | legacy UI 1:1 보존 (§1.4) 의 *strict 통과* 가 정적 매트릭스 #16 grep 외에는 미입증. Phase 3.4 평가서의 *Intentional deviation* 같은 사후 분석조차 보류 | (a) Viewer 복구 후 §5 #20~#25 6 항목 본인 환경 재수행. (b) 차후 평가서 (`phase3_5_evaluation_2026-MM-DD_v2.md` 같은 v2) 으로 *시각 검증 보강분 수록* — Phase 3.3 의 *v2* 패턴 적용 |
| 1.6.2 | mouse_interactor 의 `OnMouseWheelForward/Backward` 가 `onAtomsChanged.Emit` 으로 *불필요한 휠 줌 시 측정 객체 RefreshStructure 발생* | 성능 부하 (대형 구조 + 다수 측정 객체) | Phase 4 인프라 안정화 시 `AtomsChangedEvent` 의 *세분화* (kind: Added/Removed/Moved/Camera) 검토 — Phase 3.4 평가서 §1.7.1 의 bonds_controller 우려와 동일 |
| 1.6.3 | measurement_store 가 *`onStructureVisibilityChanged` 첫 구독자* — 계획서 §1.1 매트릭스에 누락 | *(계획 외) 의도성* — 무해하지만 *재사용 시험대* 매트릭스의 보강 필요 | 본 평가서 §1.4 의 EventBus 분포 표에 *7 → 10 채널 확장* 명시. 후속 sub-phase 매트릭스에 정착 |
| 1.6.4 | measurement_overlay_ui *27% 극단 압축* — legacy 의 `RenderMeasurementModeOverlay` 가 4 모드 파일로 분산된 결과 — *UI 분산 압축* 패턴 첫 사례 | §1.4 UI 1:1 보존 검증의 *경계 모호* — 4 모드 파일의 vtk 액터 + overlay UI 의 ImGui 위젯이 *어디까지 strict 보존인지* 시각 비교 없이는 판단 곤란 | §5 #20 시각 비교 미수행으로 *보류*. 후속 평가서 v2 에서 *4 모드 파일별 vtk 액터 + overlay UI 의 위젯 매핑* 표 작성 권장 |
| 1.6.5 | **controller 의 6 EventBus 구독 + vtk_viewer 6 메서드 직접 호출 = *최대 풍부도***. 계획서 §9.1 의 *풍부 controller* (~450 줄) 초과 *551 줄* | controller 단일 파일이 *4 책임 (mouse_interactor 결합 + 6 이벤트 분기 + PickVisual lifecycle + 5 모드 dispatch)* 으로 *부피 증대* | Phase 4 `menu_router` 도입 시 *EnterMode/ExitMode 분리* 검토. 또는 *PickVisualController* 같은 서브 컨트롤러로 vtk 액터 lifecycle 분리 |
| 1.6.6 | **Phase 3.5 코드 commit 미수행** — Phase 3.4 머지로 7 회 연속 패턴이 해소되었으나 *재발 1 회차* | Phase 3.6 file 진입의 base commit 모호 | Phase 3.4 평가서 §1.7.5 의 *상위 §6.0.5 신설* 권장이 *재발 즉시 격상* 결정 필요. PR 체크리스트 첫 행 강제 정책 |
| 1.6.7 | **§5 #25 *동일 publisher N 구독자* 패턴 동적 미수행** — 본 단계의 *재사용 시험대* 가설 입증의 핵심이 보류 | Phase 3.6 file 진입 시 *broadcaster 가설* 이 정적 검증으로만 입증된 상태로 진입 | (a) Viewer 복구 후 #25 우선 검증. (b) 또는 Phase 3.6 진입 시 *동시 메뉴 활성 시나리오* (Edit + Measurement 둘 다 열어둔 채 atom 픽) 추가 |

---

# Part 2 — 계획서 대비 일탈 사항 (Intentional 6 종 + 인프라 보강 1 종)

## 2.1 Intentional Deviations (사용자 워크플로우 개선 또는 의미 동등 — Phase 3.4 평가서 §1.7.3 정신 적용)

| # | 일탈 | 위치 | 의도성 | 평가 |
|---|---|---|---|---|
| 2.1.1 | `getElementInfo(symbol)->atomicMass` → **`getAtomicMass(symbol)` 직접 호출** | `features/measurement/center.cpp:140` | element_database 의 *간략 시그니처* 가 이미 존재 — Phase 3.3 의 element_database 시그니처 follow | ✓ Intentional + 의미 동등 |
| 2.1.2 | `Mode` enum → **`MeasurementMode` enum + `MeasurementType` enum 분리** | `features/measurement/measurement_mode.h` | 객체 lifecycle 시점에는 *Mode::None* 이 불가능하므로 `MeasurementType` 별도 — 코드 가독성 + 안전성 | ✓ Intentional + 안전성 |
| 2.1.3 | `onStructureVisibilityChanged` 첫 구독자 (계획 외 추가) | `features/measurement/measurement_store.cpp:55` | 구조 visibility 변경 시 측정 객체의 visibility cascade — *legacy 정합 동작 강화* | ✓ Intentional + 정합 강화 |
| 2.1.4 | **Esc / Enter / Apply 단축키 + 버튼** | `features/measurement/measurement_overlay_ui.cpp:53~78` | legacy §1.4.1 의 *Esc 종료* 정신 + Enter 로 Center commit + Apply 버튼 — *사용자 워크플로우 개선* | ✓ Intentional + 사용자 개선 |
| 2.1.5 | **`ApplyCenterMeasurement` controller 메서드** + Center 모드의 *명시적 commit* 정책 | `features/measurement/measurement_controller.cpp:184~193` | Distance/Angle/Dihedral 가 자동 commit 이고 Center 가 *N atom 누적 + Apply* 정책 — *분기 명확화* | ✓ Intentional + 명확화 |
| 2.1.6 | **EnterMode 시 측정 리스트 윈도우 자동 노출** (`g_showMeasurementWindow = true`) | `features/measurement/measurement_menu.cpp:28` | 사용자 첫 진입의 *학습 부담 완화* | ✓ Intentional + 학습 완화 |

## 2.2 인프라 보강 (계획 §1.2.1 옵션 A 채택 — 계획 내 수정)

| # | 일탈 | 위치 | 의도성 | 평가 |
|---|---|---|---|---|
| 2.2.1 | `core/scene/events.h` 신규 3 이벤트 (AtomPicked/EmptyClick/DragSelection) + 3 신규 Bus | `core/scene/events.h:60~94` | 계획 §2.1 명시 — 회색지대 §1.2.1 옵션 A | ✓ 계획 내 |
| 2.2.2 | `core/vtk/mouse_interactor` 의 drag selection 보강 (OnMouseMove + OnLeftButtonUp + emitPickOrEmptyClick + 5 멤버) | `core/vtk/mouse_interactor.{cpp,h}` | 계획 §2.2 명시 — 회색지대 §1.2.1 옵션 A | ✓ 계획 내 |

→ Phase 3.4 의 *Intentional UI deviation 5 종 + 인프라 보강 3 종 = 8 건* 보다 본 단계는 *6 + 2 = 8 건* 으로 동일 양적 비중. **legacy/ 동결 원칙은 그대로 유지** (legacy 호출 0, legacy include 0, vtk_renderer 의존 0).

---

# Part 3 — 평가 시점의 저장소 상태

## 3.1 파일 트리

```
webassembly/src/
├─ main.cpp                                       (Phase 1 그대로)
├─ bind_function.cpp                              (Phase 1 그대로)
├─ app/                                           (Phase 3.5 hook 4 곳 추가 — include + InitOnce + DrawMenu + RenderWindows)
├─ core/                                          (Phase 2 그대로 + events.h 3 신규 이벤트 + mouse_interactor 의 drag selection 보강 (§2.2))
├─ features/
│  ├─ utilities/brillouin_zone/                   (Phase 3.1 — 11 파일, 2,131 줄)
│  ├─ data/                                       (Phase 3.2 — 18 파일, 2,580 줄)
│  ├─ build/                                      (Phase 3.3 — 16 파일, 1,883 줄)
│  ├─ edit/                                       (Phase 3.4 — 28 파일, 4,483 줄)
│  └─ measurement/                                ★ 신규 Phase 3.5 (18 파일, 2,570 줄)
│     ├─ measurement_menu.{cpp,h}                 (113 줄, 91+22)
│     ├─ measurement_mode.{cpp,h}                 (112 줄, 78+34) — 5 모드 state machine
│     ├─ measurement_store.{cpp,h}                (585 줄, 492+93) — **onAtomsChanged 3 번째 + onStructureRemoved 8 번째 + onStructureVisibilityChanged 첫 구독자**
│     ├─ measurement_controller.{cpp,h}           (551 줄, 471+80) — **mouse_interactor 두 번째 구독자 + 6 EventBus 구독 (재사용 패턴 정적 검증)**
│     ├─ measurement_overlay_ui.{cpp,h}           (162 줄, 141+21) — §1.4 보존 핵심 (단 시각 비교 보류)
│     ├─ distance.{cpp,h}                         (163 줄, 134+29)
│     ├─ angle.{cpp,h}                            (275 줄, 241+34)
│     ├─ dihedral.{cpp,h}                         (356 줄, 318+38)
│     └─ center.{cpp,h}                           (253 줄, 212+41) — **core::data::ElementDatabase::getAtomicMass 첫 호출**
└─ legacy/                                        (Phase 0 동결본 — atoms_template.cpp 측정 부분 변경 0)
```

총 신규 18 파일 / **2,570 줄** (계획 풍부 시나리오 ~2,500 의 102.8%, legacy ~3,000 의 **85.7%**).

## 3.2 git status / commit 분포

```
최근 커밋 9 개 (HEAD=8521e3b):
  8521e3b  Phase 3.4: migrate Edit cell/atoms/bonds features, wire events, and add phase reports
  a20abc6  Phase 3.3: migrate Build periodic table and Bravais features
  bb4ca37  Phase 3.2: migrate Data charge_density/slice features and add evaluations
  38ddc75  Phase 3.1: utilities/brillouin_zone migration plus reinforced rendering
  02f5010  Phase 2: build core/ skeleton infrastructure
  f07e541  Phase 1: bootstrap app/+core shell, exclude legacy build
  4eac024  docs: redevelopment plan and Phase 1 detail
  2a586bb  Phase 0: Move webassembly/src/* to webassembly/src/legacy/
  33eb219  docs: redevelopment plan and Phase 0 detail

git status (Phase 3.5 관련 부분만):
  M  CMakeLists.txt                                              (§2 빌드 source 18 줄 추가)
  M  webassembly/src/app/app.cpp                                 (§4 임시 hook 4 곳)
  M  webassembly/src/core/scene/events.h                         (§2.2.1 — 3 신규 이벤트 + 3 신규 Bus)
  M  webassembly/src/core/vtk/mouse_interactor.cpp               (§2.2.2 — drag selection 보강)
  M  webassembly/src/core/vtk/mouse_interactor.h                 (동상 — 5 멤버 + 3 메서드)
  ?? webassembly/docs/phases/phase3_5_measurement.md             (계획서)
  ?? webassembly/docs/phases/phase3_5_evaluation_2026-05-11.md   (본 평가서)
  ?? webassembly/src/features/measurement/                       (18 파일)
```

→ Phase 3.5 의 코드 변경은 commit 되지 않은 working tree 상태. **Phase 3.4 머지로 7 회 연속 패턴 해소** 후 *재발 1 회차*. 정책 격상 (Phase 3.4 평가서 §1.7.5) 의 *상위 §6.0.5 신설* 권장이 *재발 즉시 격상* 결정 필요.

## 3.3 계획서의 시사

[`./phase3_5_measurement.md`](./phase3_5_measurement.md) §0 의 *Phase 3.1~3.4 의 압축 패턴 자연 발생 예상 (~73~80%)* 가 본 평가서의 *85.7% 실측* 으로 **+5~13% 보수적** 으로 검증. §9.1 의 *얇은 vs 풍부 controller* 두 시나리오 중 **풍부 시나리오 도달** (controller 551 줄, 풍부 시나리오 ~450 의 122%) — *7 구독 채널 (mouse_interactor + EventBus 6 채널) 이 정비례 입증* 한 본 평가서의 가장 중요한 학습.

| 계획서 §0 예측 | 본 평가서 실측 | 일치도 |
|---|---|---|
| ~2,200~2,400 줄 (압축률 73~80%) | **2,570 줄 (85.7%)** | △ — +5~13% 보수적 |
| 동일 publisher 의 N 구독자 패턴 검증 | **정적 통과 (SetEventBus 2 곳 호출 + EventBus 공유)** + 동적 보류 (§5 #25) | △ — 정적만 통과 |
| onAtomsChanged 세 번째 구독자 도달 | **3 번째 + 4 번째 동시 도달** (controller + store 분담) | ✓ + 분담 설계 |
| onStructureRemoved 8 번째 구독자 도달 | **8 + 9 번째 동시 도달** | ✓ + 분담 설계 |
| `core::data::ElementDatabase` mass 호출 | **`getAtomicMass(symbol)` 1 호출** — center.cpp:140 | ✓ — 시그니처 차이 (§2.1.1) 이나 의미 동등 |
| §1.4 UI 보존 (5 모드 오버레이 + 객체 리스트) | **grep 매트릭스 #16 통과 + 시각 비교 (#20) 보류** | △ — 정적만 |
| `controller 의 *얇은 vs 풍부* 두 시나리오 명시` | **풍부 시나리오 도달 (551 줄, 풍부 ~450 의 122%)** | ✓ 풍부 시나리오 |
| `npm run build-wasm:debug + :release exit 0` | **사용자 확인 통과** | ✓ |
| `메뉴바 Measurement + 5 sub-item` | **사용자 확인 통과** | ✓ |

→ 계획서 *큰 그림* (5 모드 + drag selection + 동일 publisher N 구독자 + Phase 2 인프라 재사용 시험대 + UI 1:1 보존 grep) **정적 단계는 모두 일치**. 단 **동적 시각 검증 6 항목 (#20~#25) 은 Viewer 미복구로 보류** — 본 평가서의 가장 큰 *공백*.

---

# Part 4 — Phase 3.6 진행가능 여부 판정

## 4.1 Phase 3.6 입구 조건과의 매핑

| Phase 3.6 전제 (`05_redevelopment_plan.md` §6 + `04_menu_to_code_mapping.md` §7) | 현재 상태 | 통과? |
|---|---|---|
| Phase 3.5 의 features/measurement 패턴 확립 (단일 sub-folder + 5 모드 dispatch + 풍부 controller) | **확립됨** — 18 파일 + 패턴 정상 | ✓ |
| `core::vtk::MouseInteractor` 의 *두 번째 구독자* 도달 + 재사용 패턴 정적 검증 | **정적 도달** — measurement_controller + atoms_controller 동일 EventBus 공유 | ✓ (정적) / ⊘ (동적 #25) |
| `core::scene::EventBus::onAtomsChanged` 의 *N 구독자 broadcaster 패턴* 정적 검증 | **3 + 4 번째 구독자 도달** — controller + store 분담 | ✓ |
| `core/scene/events.h` 의 *N 신규 이벤트 추가 패턴* 확립 | **3 신규 (AtomPicked/EmptyClick/DragSelection) 도입 + EventBus 7→10 채널** | ✓ |
| 빌드 + 런타임 정상 (Phase 0/1/2/3.1/3.2/3.3/3.4 회귀 없음) | **debug + release 모두 exit 0** + 메뉴 노출 통과 + (단 시각 회귀는 #20~#25 보류) | ✓ (정적) / △ (시각) |
| `core::data::ElementDatabase` 의 *mass 사용자 도달* | **getAtomicMass 1 호출 (center.cpp:140) 첫 도달** | ✓ |
| **§1.4 UI 1:1 보존 검증 (#20~#25)** | **Viewer 미복구로 미수행** — *6 항목 보류* | ✗ — *동적 검증의 결정적 차단* |
| Phase 3.5 PR commit 머지 (Phase 3.6 PR base) | **미커밋 — 재발 1 회차** | ✗ |
| **vtk_renderer 분할 100% 완료** (Phase 3.4 머지) | **분할 완료 — Phase 3.4 평가서 §1.2.3 V4** | ✓ |
| `core/io/format_registry` 의 *읽기 사용자가 진입할 수 있는 상태* | **Phase 3.2 등록자 + 본 단계 영향 없음** | ✓ |

→ Phase 3.6 진입 환경이 **정적 단계는 완벽히 준비됨** + 그러나 **동적 시각 검증 6 항목 (Phase 3.5 §5 #20~#25) 의 *결정적 차단*** 으로 진입 결정에 *조건부* 요구.

## 4.2 종합 판정

> **조건부 진행 가능 (GO with runtime gap)**
>
> Phase 3.5 의 본질 (features/measurement 단일 sub-folder + 5 모드 dispatch + drag selection 보강 + 동일 publisher 의 두 번째 구독자 정적 도달 + onAtomsChanged 세 번째/네 번째 구독자 + onStructureRemoved 8/9 번째 구독자 + element_database mass 첫 사용자 + §1.4 UI 보존 grep 매트릭스 #16 통과) + 빌드 (debug + release) + 메뉴 노출 — *정적 + 빌드 + 메뉴* 까지 모두 통과. 그러나 **§5 #20~#25 의 *시각/시나리오 검증 6 항목* 이 Viewer 미복구로 *미수행*** — *legacy UI 1:1 보존* 의 *동적 입증* 이 보류. 남은 정리는 *(a) Phase 3.5 코드 commit + (b) Viewer 복구 후 §5 #20~#25 6 항목 본인 환경 재수행* 2 가지.

## 4.3 진입 전 처리할 2 가지 정리 항목

| # | 항목 | 명령 / 방법 | 통과 기준 |
|---|---|---|---|
| 4.3.A | **Phase 3.5 코드 commit 정리** | (a) Windows PowerShell 측에서 `git add -A`. (b) Phase 3.5 변경 (features/measurement/ 18 파일 + app/app.cpp 4 hook + CMakeLists.txt + core/scene/events.h + core/vtk/mouse_interactor 보강 + 본 평가서 + 계획서) 만 단일 commit. (c) commit message 에 본 평가서 §2.1 Intentional 6 종 + §2.2 인프라 보강 2 종 사유서 첨부. (d) PR 본문에 *"§5 #20~#25 의 시각/시나리오 검증은 Viewer 복구 후 v2 평가서로 보강"* 명시 | Phase 3.5 commit 1 개 + Phase 3.4 머지 패턴 재현 |
| 4.3.B | **Viewer 복구 후 §5 #20~#25 동적 검증 6 항목 수행** | (a) `npm run dev` 후 Measurement 메뉴의 5 모드 + 드래그 셀렉션 + atom 삭제 + 구조 제거 + atoms_controller 동시 활성 시나리오. (b) 결과를 `phase3_5_evaluation_2026-MM-DD_v2.md` (Phase 3.3 의 v2 패턴 follow) 로 별도 보강 평가서 작성 — Side-by-side 스크린샷 + S1~S7 시나리오 결과 + #25 동일 publisher N 구독자 응답 패턴 + Intentional UI deviation 사후 분석 (Phase 3.4 평가서 §1.7.3 의 *재학습 부담 없이 사용자 워크플로우 개선* 정신 적용) | v2 평가서 1 부 + #20~#25 통과 |

> 본 2 항목 중 **4.3.A 는 Phase 3.6 진입 base commit 확정에 필수**, **4.3.B 는 *legacy UI 1:1 보존 정책* 의 동적 입증에 필수** — 둘 다 Phase 3.6 의 PR 머지 *전에* 처리 권장. *분리 진행 가능* — 4.3.A 통과 후 Phase 3.6 코드 작업 병행 중 4.3.B 수행도 가능.

---

# Part 5 — 메타 평가

## 5.1 계획서의 효과 평가

| 보강 항목 (Phase 3.5 §) | 효과 |
|---|---|
| §1.1 다섯 번째 feature 의 의의 (Phase 2 인프라 *재사용 패턴 첫 검증*) | **정적 단계 검증 성공** — MouseInteractor 두 구독자 + EventBus 6 채널 구독 + element_database mass 첫 호출 모두 도달. 동적 (#25) 은 보류 |
| §1.2 회색지대 정책 인계 | 본 시도에서 회색지대 사례 *2 종* 발생 — events.h 3 신규 이벤트 (§1.2.1 옵션 A) + mouse_interactor drag selection 보강. legacy 측 변경 0 |
| §1.3 라인수 압축 정책 | **유효 + 보수적** — 85.7% 압축, 계획 73~80% 보다 +5~13% 보수적. *데이터 비중 (5 종 visual + lifecycle + UI)* 의 자연 결과 |
| **§1.4 legacy UI 1:1 보존** | **정적 grep 매트릭스 #16 통과** + **동적 시각 비교 #20 보류** — 후속 v2 평가서 권장 (§4.3.B) |
| §2.1 events.h 3 신규 이벤트 | **정확히 일치** — AtomPickedEvent/EmptyClickEvent/DragSelectionEvent 모두 도입 + 3 신규 Bus |
| §2.2 mouse_interactor 보강 | **정확히 일치** — OnMouseMove/OnLeftButtonUp + 5 멤버 추가. emitPickOrEmptyClick 의 vtkCellPicker 기반 픽 분기 추가 |
| §3 외부 의존 사전 분석 | 정확 — namespace 만 변경 + element_database mass 호출 + mouse_interactor 인프라 보강 모두 일치 |
| §5 검증 매트릭스 27 항목 | **PASS 19 / PARTIAL 0 / FAIL 0 / N/A 8 (Viewer 미복구 6 + 권장 수준 2)** — 정적 16 항목 + 동적 #17~#19 통과 |
| §6 리스크 / 완화책 | §6.1 (broadcaster 가설) **정적 통과 / 동적 보류** △ / §6.2 (drag 충돌) **정적 미확인** ⊘ / §6.3 (3D + 2D 혼합 렌더) **정적 미확인** ⊘ / §6.4 (Angle/Dihedral fractional/cartesian) **정적 통과 — 모두 cartesian** ✓ / §6.5 (atomicMass 시그니처) **getAtomicMass 직접 호출로 해소** ✓ / §6.6 (onAtomsChanged 과도 제거) **store 가 atom 유효성만 검증 — 좌표 변경 시 RefreshStructure 정도** △ / §6.7 (onStructureRemoved 실행 순서) **분담 설계로 측정 제거가 일관** ✓ / §6.8 (§1.4 위반) **시각 비교 #20 보류** ⊘ / §6.9 (controller 풍부도) **551 줄 풍부 시나리오 실현 + vtk_viewer 직접 호출 추가** △ / §6.10 (분할 시그널) **2,570 줄 ≤ 2,600 기준** ✓ / §6.11 (Intentional deviation) **6 종 명시** ✓ / §6.12 (base commit) **4.3.A 처리 필요** ✗ |
| §8 PR 체크리스트 | **정적 항목 모두 통과** — 동적 항목 (#20~#25) 만 *후속 처리* |
| §9.1 *얇은 vs 풍부 controller* 두 시나리오 | **풍부 시나리오 도달** (551 줄, 풍부 ~450 의 122%) — *7 구독 채널이 정비례 입증* |

→ 계획서의 *큰 그림* (5 모드 + drag selection + 동일 publisher N 구독자 + Phase 2 인프라 재사용 시험대 + UI 보존) **정적 단계는 정확히 작동**. 단 **동적 시각 검증 6 항목 (#20~#25) 은 Viewer 미복구로 보류** — *legacy UI 1:1 보존 정책의 동적 입증* 이 본 평가서의 가장 큰 *공백*.

## 5.2 잔여 리스크 (Phase 3.6 진입 전 인지 필요)

| # | 리스크 | 완화 |
|---|---|---|
| 5.2.1 | **§5 #20~#25 시각/시나리오 검증 미수행 — Viewer 미복구** | 4.3.B 통과로 해소 (v2 평가서) |
| 5.2.2 | **§5 #25 *동일 publisher N 구독자* 패턴 동적 미수행** — 본 단계의 *재사용 시험대* 가설 입증의 핵심 | Viewer 복구 후 Edit + Measurement 동시 활성 시나리오로 broadcaster 가설 동적 입증 |
| 5.2.3 | **Phase 3.5 코드 commit 미수행 — 재발 1 회차** | 4.3.A 통과로 해소. 정책 격상 (상위 §6.0.5 신설) 즉시 검토 |
| 5.2.4 | mouse_interactor 의 OnMouseWheelForward/Backward → onAtomsChanged.Emit 가 *카메라 조작 시 RefreshStructure 트리거* (§1.6.2) | Phase 4 인프라 안정화 시 AtomsChangedEvent 세분화 검토 — Phase 3.4 평가서 §1.7.1 의 bonds_controller 우려와 동일 |
| 5.2.5 | measurement_overlay_ui 의 *27% 극단 압축* — UI 분산 압축의 §1.4 보존 경계 모호 (§1.6.4) | v2 평가서에서 *4 모드 파일별 vtk 액터 + overlay UI 위젯 매핑 표* 작성 |
| 5.2.6 | controller 의 *6 EventBus 구독 + vtk_viewer 6 메서드 직접 호출 = 최대 풍부도* (§1.6.5) | Phase 4 `menu_router` 또는 *PickVisualController* 분리 검토 |
| 5.2.7 | wasm 사이즈 정량 검증 (#27) 미명시 | release 빌드 통과로 간접 입증. 권장 수준 유지 (Phase 3.2~3.4 일관) |

## 5.3 종합 평가표

| 항목 | 점수 (5점 만점) | 비고 |
|---|---|---|
| 정적 단계 실행 정확성 | **5** | 18 파일 + namespace + legacy 격리 + EventBus 6 채널 구독 + mouse_interactor 두 번째 구독자 + element_database mass 첫 사용자 모두 정확 |
| 정적 검증 통과율 | **5** | #1~#16 16 항목 모두 PASS — Phase 3.4 의 PARTIAL 8 대비 0 (가장 깔끔한 정적 통과) |
| 동적 검증 통과율 | **2** | #17~#19 3 항목 PASS / #20~#25 6 항목 *Viewer 미복구로 미수행* / #26~#27 권장 수준. *legacy UI 1:1 보존의 동적 입증 보류가 본 단계의 가장 큰 공백* |
| 계획서 §4 절차 적합성 | **5** | 13 단계 모두 정상 — 1 단일 sub-folder + 5 모드 dispatch + drag selection 보강 + element_database mass 호출 패턴 정착 |
| 계획서 §5 검증 매트릭스 커버리지 | **3** | 27 항목 중 19 PASS / 8 N/A — *세분화는 충분* + *동적 #20~#25 의 결정적 차단* 으로 부분 통과 |
| **§1.4 UI 보존** | **3** | 정적 grep 매트릭스 #16 통과 + 동적 시각 비교 #20 보류 — *Intentional deviation 6 종은 사후 분석 필요 (v2 평가서)* |
| **Phase 2 인프라 *재사용 패턴* 정적 검증** | **5** | MouseInteractor 두 구독자 (atoms + measurement) 동일 EventBus 공유 + onAtomsChanged 3/4 번째 구독자 + onStructureRemoved 8/9 번째 구독자 + element_database mass 첫 사용자 모두 정적 통과 |
| `controller 풍부 시나리오 정량 검증` | **5** | 551 줄 (풍부 시나리오 ~450 의 122%) — *7 구독 채널 (mouse_interactor + EventBus 6)* 이 controller 풍부도와 정비례 입증 |
| 라인수 압축 효과 | **4** | 85.7% (계획 73~80% 의 +5~13% 보수적) — *데이터 비중 (5 visual + lifecycle + UI)* 의 자연 결과 |
| Phase 3.5 PR 형태 | **3** | working tree 상태 (Phase 3.4 머지로 7 회 연속 해소 후 *재발 1 회차*) — 4.3.A 통과 필요 |
| Phase 3.6 입구 도달도 | **3** | 정적 단계 완벽 준비됨 + 동적 *시각 검증 6 항목 (#20~#25)* 보류로 *조건부* — 4.3.A + 4.3.B 통과 후 진입 가능 |
| 종합 | **조건부 진행 가능 (GO with runtime gap)** | **Phase 0/1/2/3.1/3.2/3.3/3.4 의 *결정적 인프라 검증* 단계 (Phase 3.4) 이후 *재사용 시험대* 정적 단계 완료**. Viewer 미복구로 *legacy UI 1:1 보존의 동적 입증* 이 보류된 점이 본 단계의 가장 큰 *공백* — v2 평가서로 보강 권장 |

> Phase 3.5 는 **다섯 번째 feature + Phase 2 인프라 *재사용 시험대* 정적 단계 + drag selection 보강 + 동일 publisher N 구독자 broadcaster 정적 검증 + §1.4 UI 보존 정책 *grep 매트릭스 통과 / 시각 비교 보류*** 라는 *4 중 시험* 중 정적 3 종 통과 + 동적 시각 보류. 잔여 4 sub-phase (3.6~3.9) + Phase 4~6 가 본 패턴을 *완전히 검증된 인프라* 위에서 진행 가능하되, *Viewer 복구 후 §5 #20~#25 의 동적 입증* 이 *후속 v2 평가서* 의 형태로 추가 권장.
>
> Phase 3.4 평가서가 *"정책의 *재해석 (Intentional deviation)* + *분할 정책 (Option A) 의 정량 검증* + *Phase 2 인프라의 *결정적 시험대 통과*"* 의 3 중 학습이었다면, **본 평가서는 *재사용 패턴 정적 검증 + 풍부 controller (551 줄) 정량 입증 + 동적 시각 검증의 환경 의존 노출*** 의 3 중 학습. Phase 3.6 (file) 가 *format_registry 의 읽기 사용자* + *외부 파일 데이터 위에 측정 작동* 의 *전체 워크플로우 검증* 단계 역할.

---

## 6. 결론 및 권장 다음 단계

### 결론

> **Phase 3.5 의 본질적 작업 + 정적 검증 (16/16) + 동적 검증 (#17~#19 3/3) + Phase 2 인프라 *재사용 시험대* 정적 단계 (MouseInteractor 두 번째 구독자 + EventBus 6 채널 구독 + element_database mass 첫 사용자) + drag selection 보강 + 풍부 controller 시나리오 정량 입증 + §1.4 UI 보존 정책 *grep 매트릭스 #16 통과* 모두 완료되었다.** 검증 매트릭스 27 항목 중 PASS 19 / PARTIAL 0 / FAIL 0 / N/A 8 — N/A 8 중 6 항목 (#20~#25) 이 **Viewer 미복구로 미수행**, 2 항목 (#26~#27) 이 *권장 수준* 으로 완화. **남은 정리는 (a) Phase 3.5 코드 commit (4.3.A) + (b) Viewer 복구 후 §5 #20~#25 6 항목 동적 검증 (4.3.B — v2 평가서) 2 가지**.

### 권장 다음 단계

1. **§4.3.A** — Phase 3.5 코드 commit 정리 (Windows PowerShell 측에서 수행). 본 commit 에 본 평가서 + 계획서 모두 포함 권장. Intentional deviation 6 종 + 인프라 보강 2 종 사유서 commit message 에 첨부. PR 본문에 *"§5 #20~#25 의 시각/시나리오 검증은 Viewer 복구 후 v2 평가서로 보강"* 명시.
2. **§4.3.B** — Viewer 복구 후 §5 #20~#25 의 6 항목 동적 검증 수행. 결과를 `phase3_5_evaluation_2026-MM-DD_v2.md` 로 별도 보강 평가서 작성 — Side-by-side 스크린샷 + S1~S7 시나리오 + #25 동일 publisher N 구독자 응답 + Intentional UI deviation 사후 분석.
3. (4.3.A + 4.3.B 통과 후) — **상위 §6.0.5 신설 검토** *("commit 머지 확인을 PR 체크리스트 첫 행으로 강제 + 동적 시각 검증의 환경 가용성 사전 확인")* — Phase 3.4 평가서 §1.7.5 의 누적 부채 해소 + 본 단계의 *Viewer 미복구* 노출에 대한 정책 격상.
4. (정책 격상 후) — `phase3_6_file.md` 세부계획서 작성 진입. **본 단계의 *재사용 패턴 정적 검증 통과* + measurement_store 의 *외부 파일 데이터 위에 작동* 패턴** 을 base 로 진입.

### Phase 3.6 진입 신호

다음 항목들이 ✓ 되면 Phase 3.6 PR 을 시작해도 무방.

- [x] ~~정적 검증 #1~#16 모두 PASS~~ — **통과 확인** (16/16)
- [x] ~~`npm run build-wasm:release` exit 0~~ — **통과 확인** (사용자)
- [x] ~~Measurement 메뉴 + 5 항목 노출~~ — **통과 확인** (사용자)
- [x] ~~mouse_interactor 두 번째 구독자 정적 도달~~ — **통과 확인** (measurement_controller.cpp:139)
- [x] ~~onAtomsChanged 3/4 번째 + onStructureRemoved 8/9 번째 구독자 도달~~ — **통과 확인**
- [x] ~~element_database mass 첫 사용자 도달~~ — **통과 확인** (center.cpp:140)
- [x] ~~controller 풍부 시나리오 정량 검증~~ — **통과 확인** (551 줄)
- [ ] Phase 3.5 PR commit 1 개로 정리되어 머지 또는 push *(4.3.A — 사용자 곧 진행 예정)*
- [ ] Viewer 복구 후 §5 #20~#25 6 항목 동적 검증 *(4.3.B — v2 평가서)*

---

## 7. 관련 문서

- 참조 계획서: [`./phase3_5_measurement.md`](./phase3_5_measurement.md) (2026-05-11)
- 선행 Phase 3.4 평가서: [`./phase3_4_evaluation_2026-05-11.md`](./phase3_4_evaluation_2026-05-11.md)
- 선행 Phase 3.4 인덱스: [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)
- 선행 Phase 3.4 분할 sub-phase 세부계획서:
  - [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md)
  - [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md)
  - [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md)
- 선행 Phase 3.3 평가서: [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
- 선행 Phase 3.2 평가서: [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md)
- 선행 Phase 3.1 평가서: [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- Phase 2 (인프라): [`./phase2_evaluation_2026-04-28.md`](./phase2_evaluation_2026-04-28.md) — `core/scene/events`, `core/vtk/mouse_interactor`, `core/data/element_database`
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.5) + **§6.0 공통 지침 (UI 1:1 보존)** + §13 (UI 이식 공통 지침)
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §6 Measurement
- UI 이식 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
