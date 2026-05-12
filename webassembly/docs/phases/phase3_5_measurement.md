# Phase 3.5 — Measurement 이식 (Fifth Feature — *동일 publisher 의 두 구독자 첫 검증*) 세부계획서

> 상위 문서:    [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.5) + §6.0 공통 지침
> 선행 문서:    [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md) (분할 진행) + [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md) + [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md) + [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md)
> 선행 평가서:  [`./phase3_4_evaluation_2026-05-11.md`](./phase3_4_evaluation_2026-05-11.md)
> 메뉴 매핑:    [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §6 Measurement
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01~UI-12)
> 작성일:      2026-05-11
> 대상 브랜치: `refactor/menu-aligned`
> 단위 PR:     1 개 (단일 sub-folder — 분할 불필요)
> 예상 소요:   4~5 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-11 | 초안 작성 — Phase 3.4 평가서 §6 권장 다음 단계 항목을 본 문서로 확장 |

---

## 0. 한 줄 요약

> Phase 3.1/3.2/3.3/3.4 가 확립한 *features/ 4 layer 패턴 + UI 보존 정책* 을 그대로 복제하여 **Measurement 메뉴의 5 항목 (Distance / Angle / Dihedral / Geometric Center / Center of Mass)** 을 단일 sub-folder `features/measurement/` 로 이식한다. Phase 3.1 (BZ) 와 같은 *단일 sub-folder + N 모드 토글* 패턴 — Phase 3.4 의 *N sub-folder 분할* 과 다른 *단일 sub-folder + 5 모드 EnterMode 디스패치* 형태. 본 단계는 또한 **`core/vtk::MouseInteractor` 의 *두 번째 구독자* + `core/scene/EventBus::onAtomsChanged` 의 *두 번째 구독자* 동시 도달** — Phase 3.4 의 atoms_controller / bonds_controller 와 *동일 publisher 의 N 구독자* 패턴의 *첫 외부 검증*. Phase 3.4 평가서 §1.5 의 *Phase 2 인프라 최종 시험대* 이후 *후속 시험대* 의 역할.
> Phase 3.1 (53%) / 3.2 (57.5%) / 3.3 (61.2%) / 3.4 (74.6%) 의 압축 패턴이 본 단계에서도 자연 발생할 것으로 예상. legacy 측정 관련 코드 약 **3,000 줄** (atoms_template.cpp 의 3755~5500 라인 분량 + 보조 메서드) → 약 **2,200~2,400 줄** 예상 (압축률 73~80%, *5 모드 + drag selection + style + 객체 리스트* 의 데이터 비중이 큼). Phase 3 평균 (~2,200 줄) 과 거의 일치.

---

## 1. 목표 / 비목표

| 구분 | 항목 |
|---|---|
| **목표** | (a) `features/measurement/` 단일 sub-folder 신설 + 약 18 파일 작성. (b) 메뉴 `Measurement / Distance · Angle · Dihedral · Geometric Center · Center of Mass` 클릭 시 *해당 모드 진입* (EnterMode 단일 디스패치). (c) **`core/vtk::MouseInteractor` 의 *두 번째 구독자* 도달** — Phase 3.4.2 의 atoms_controller 와 *동일 publisher 의 두 구독자* 패턴 *첫 외부 검증*. (d) **`core/scene/EventBus::onAtomsChanged` 의 *두 번째 구독자* 도달** — atom 삭제 시 잘못된 측정 자동 제거 (atom_renderer 가 *첫 구독자*, measurement_store 가 *두 번째*). (e) **drag selection** — *사각 영역 안의 atoms 다중 선택* 으로 측정 객체 생성. (f) **5 모드 별 응답** — Distance (2 atom pick), Angle (3 atom pick), Dihedral (4 atom pick), GeometricCenter (N atom + drag), CenterOfMass (N atom + drag). (g) **측정 객체 리스트 + 구조별 visibility** — `measurement_store` 에 저장 + UI 패널 표시 + visibility 토글. (h) **화면 오버레이 UI** — 측정 결과 (거리/각도/이면각/중심점) 를 *3D 씬 위에 텍스트 + 라인/호* 로 표시. (i) `npm run build-wasm:debug` + `:release` 통과 + 런타임에서 5 메뉴 항목 동작 + 시나리오 S1~S5 (각 모드 1 종) 통과. (j) **legacy 의 Measurement 동작이 1:1 보존됨** (§1.4 — Phase 3.4 평가서 §1.7.3 의 *Intentional deviation* 정책 적용) |
| **비목표** | 다른 메뉴 항목 부활 (Phase 3.6 이후 진행), `menu_router` 정식 도입 (Phase 4), `WindowFlags` 통합 (Phase 4), 측정 객체의 *영구 저장 / 내보내기* (CSV / JSON — 마무리 단계의 영역), 모든 18 항목 회귀 전체 통과, legacy/ 내부 코드 수정 (회색지대 §1.2~§1.3 예외), *분할 진행* — 본 단계는 *단일 sub-folder + 단일 PR* 로 충분 (legacy ~3,000 줄, 예상 ~2,200 줄 = Phase 평균 수준), **legacy UI 의 *재설계* — 오버레이 텍스트 형식 / 5 모드 진입 순서 / atom 선택 응답 패턴 변경 금지 (Phase 3.4 의 Intentional deviation 정신 적용)** |

> Phase 3.5 의 미덕: *"검토자가 git diff 를 보고 `features/measurement/` 의 신규 18 파일 + `app/app.cpp` 의 메뉴 hook 1 줄 + `CMakeLists.txt` 의 source 추가 + (선택) `core/vtk/mouse_interactor` 의 drag selection 보강 1~2 곳 외에 의심할 게 없고, 시나리오 S1~S5 에서 *Distance / Angle / Dihedral / Center 의 5 모드가 legacy 와 동일 응답* 으로 작동한다"*.

### 1.1 Phase 3.5 의 *다섯 번째 feature* 의의 — Phase 2 인프라 *동일 publisher 의 N 구독자* 첫 검증

본 단계는 [`./phase3_4_evaluation_2026-05-11.md`](./phase3_4_evaluation_2026-05-11.md) §1.5 의 *Phase 2 인프라 최종 시험대 통과* 이후 **인프라의 *재사용성* 첫 검증** 단계. Phase 3.4 가 *publisher 추가 + 첫 구독자* 였다면, Phase 3.5 는 *동일 publisher 의 두 번째 구독자* — 인프라의 *재사용 패턴* 정착.

| Phase 2 인프라 | Phase 3.4 까지 사용 (publisher / subscriber) | Phase 3.5 의 추가 |
|---|---|---|
| `core/vtk::MouseInteractor` | publisher 1 + atoms_controller (첫 구독자) | **measurement_controller (두 번째 구독자)** — 동일 publisher 의 두 구독자 패턴 첫 검증 |
| `core/scene/EventBus::onAtomsChanged` | emit 7 + atom_renderer (첫 구독자) + bonds_controller (두 번째) | **measurement_store (세 번째 구독자)** — atom 삭제 시 잘못된 측정 자동 제거 |
| `core/scene/EventBus::onStructureRemoved` | subscribe 7 (Phase 3.1~3.4 모든 controller / renderer) | **measurement_store (8 번째 구독자)** — 구조 제거 시 해당 구조의 측정 일괄 제거 |
| `core/scene/EventBus::onSelectionChanged` (이미 정의됨) | (구독자 0) | **measurement_controller (첫 구독자)** — atom 선택 시 모드별 응답 |
| `core/scene/SceneState::structureRecords` | atoms / bonds / cell *변경자* 도달 (Phase 3.4) | **measurements 필드 추가 가능** — 측정 객체를 구조별로 저장 |
| `core/data/element_database` | 16 호출 (Phase 3.3 8 + Phase 3.4 8) | bond / 원소 정보 활용 가능 (mass for CenterOfMass) |

→ Phase 3.4 (4 publisher + 첫 구독자 + Phase 2 인프라 최종 시험대) → **3.5 (동일 publisher 의 두 번째 구독자 + 인프라 재사용 패턴 첫 검증) — *재사용성 시험대***.

### 1.2 회색지대 정책 인계 (Phase 0 §1.1 + Phase 1 §1.2 + Phase 3.1 §1.3 + Phase 3.4 §1.2.1)

| 회색지대 출처 | 적용 범위 (Phase 3.5) |
|---|---|
| Phase 0 §1.1 — typo build-fix | legacy/ 내부의 1~5 줄 typo |
| Phase 1 §1.2 — 빌드 호환성 shim | 새 트리 측 얇은 redirect |
| Phase 3.1 §1.3 — 임시 메뉴 hook | `app/app.cpp::renderDockSpace` 에 `features::measurement::DrawMenu()` 추가 |
| **Phase 3.4 §2.2.2 — core/* 인프라 경미한 보강** *(재적용)* | 본 단계에서도 `core/vtk/mouse_interactor` 에 **drag selection 이벤트** 추가 가능. Phase 3.4.2 가 *SetEventBus / SetRenderRequestHandler* 인터페이스를 추가한 것과 동일 패턴 — *수요에 따라 진화*. 단 Phase 3.4 평가서 §1.7.2 의 *"Phase 4 의 안정화 권장"* 도 반영 — 보강이 *최소화* 되어야 함 |

### 1.2.1 mouse_interactor 보강 정책 (Phase 3.5 신규 회색지대)

Phase 3.4.2 가 mouse_interactor 에 다음 인터페이스를 정착:

```cpp
// core/vtk/mouse_interactor.h (Phase 3.4.2 머지 후 30 줄)
void SetEventBus(core::scene::EventBus* eventBus);
void SetRenderRequestHandler(std::function<void()> handler);
void SetActiveStructureId(int32_t structureId);
void OnLeftButtonDown() override;
void OnMouseWheelForward/Backward() override;
```

본 단계의 *드래그 셀렉션 + 픽킹 좌표 포함 이벤트* 가 추가로 필요. 다음 두 옵션 중 **(A) EventBus 확장 + mouse_interactor 보강 (~50 줄)** 권장:

| 옵션 | 설명 | 채택 |
|---|---|---|
| (A) | `core/scene/events.h` 에 `AtomPickedEvent {atomId, pickPos[3]}` + `DragSelectionEvent {atomIds, additive}` 추가 + mouse_interactor 의 OnLeftButtonDown / OnMouseMove / OnLeftButtonUp 보강 — *Phase 3.4 패턴 재적용* | **권장** |
| (B) | measurement_controller 가 *직접 vtkPicker 호출* — mouse_interactor 우회 | 백업 — 단 *동일 publisher 의 N 구독자* 패턴 검증 의의 약화 |
| (C) | mouse_interactor 보강을 *Phase 4 의 안정화 PR* 로 미루기 — 본 단계는 (B) 임시 | 비권장 — Phase 3.4 평가서 §1.7.2 의 *진화* 정책과 양립 |

**(A) EventBus 확장 + mouse_interactor 보강 권장**. 회색지대 §1.2.1 의 신규 적용 사례.

### 1.3 라인수 압축 정책 (Phase 3.1~3.4 의 학습 적용)

Phase 3.4 평가서 §1.3 의 *데이터 비중이 큰 단계는 압축률 보수적* 패턴 적용. 본 단계는 *5 모드 + drag selection + style + 객체 리스트 + 오버레이 UI* 의 *알고리즘 + UI* 균형이라 ~75% 예상.

**보존 필수**:

- 5 측정 모드의 *target pick count* 정책 (Distance 2 / Angle 3 / Dihedral 4 / GeometricCenter N / CenterOfMass N)
- *각도 / 이면각 계산 알고리즘* (`rebuildAngleMeasurementGeometry` 등 — 벡터 외적·내적 기반)
- *Center of Mass* 계산 — `core::data::ElementDatabase::getInstance().getElementInfo(symbol)->atomicMass` 호출
- *드래그 셀렉션 + 추가 모드* (Shift/Ctrl 으로 additive 선택) 응답
- *측정 객체 ID 발급 + 구조별 visibility* 정책 (legacy 의 `uint32_t measurementId` 그대로 채용)
- 측정 결과 *오버레이 텍스트 형식* (legacy 의 "%.4f Å" 같은 format)
- §1.4 의 모든 UI 보존 항목

### 1.4 legacy UI 작동방식 1:1 보존 (상위 §6.0.1 + Phase 3.4 평가서 §1.7.3 적용)

상위 §6.0.1 의 *legacy UI 1:1 보존 원칙* 을 Phase 3.4 평가서 §1.7.3 의 *Intentional deviation 정신* 과 양립하여 적용. *사용자 워크플로우 재학습 불필요* 가 본질.

#### 1.4.1 Measurement 오버레이 보존 대상

`legacy/atoms/atoms_template.cpp::RenderMeasurementModeOverlay` (line 4230~) 가 정의하는 다음 UI 동작은 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| 모드 표시 | 화면 좌상단 (또는 legacy 정의 위치) 의 *"Measurement: Distance"* 등 텍스트 + 폰트 색상 |
| 진행 상태 | 픽 진행 표시 (예: *"2/2 atoms picked"* — Distance 모드의 *2 개 클릭 필요*) |
| 결과 텍스트 형식 | 거리 *%.4f Å* / 각도 *%.2f deg* / 이면각 *%.2f deg* — legacy 의 정확한 format |
| 라인/호 시각화 | Distance 의 *atom A — atom B* 선분 / Angle 의 *3 atom 으로 만들어진 호* / Dihedral 의 *두 평면* — color palette (`measurementStyleColor`) 보존 |
| Center 마커 | GeometricCenter / CenterOfMass 의 *작은 sphere + 라벨* — legacy 의 색상 + 크기 |
| atom 선택 강조 | 픽된 atoms 의 *outline / glow* 효과 — legacy 의 RGBA |
| Esc / 우클릭 | Esc 또는 우클릭으로 *현재 모드 종료* (legacy 가 가졌다면 보존) |

#### 1.4.2 Measurement 객체 리스트 UI 보존 대상

legacy 의 `GetDistanceMeasurementsForStructure` / `SetDistanceMeasurementVisible` / `RemoveDistanceMeasurement` 가 노출하는 UI 패널 동작은 **그대로 보존**.

| 카테고리 | 보존 항목 |
|---|---|
| 객체 리스트 | Distance / Angle / Dihedral / GeometricCenter / CenterOfMass 의 5 탭 또는 통합 리스트 — legacy 의 탭/그룹 구성 |
| 각 측정 행 | 측정 ID + 표시 이름 (예: *"H1-O2: 0.96 Å"*) + visibility 체크박스 + 삭제 버튼 |
| 정렬 순서 | 생성 순서 (legacy 의 std::vector 순서 그대로) |
| 색상 키 | 측정 종 별 색상 (Distance 빨강 / Angle 파랑 등 — legacy 의 `measurementStyleColor` 팔레트) |
| Clear All 버튼 | 모든 측정 삭제 — legacy 의 위치 + 확인 다이얼로그 (있다면) |

#### 1.4.3 보존 검증 절차 (상위 §6.0.2 의 본 단계 적용)

PR 작성자는 본 PR 본문에 다음 검증을 첨부:

1. **Side-by-side 스크린샷** — legacy Measurement (오버레이 + 객체 리스트) vs 새 트리.
2. **사용자 시나리오 정합성** — 다음 5 시나리오를 legacy 와 새 트리에서 차례로 실행해 동일 결과 확인:
   - **(S1)** Measurement / Distance → atom A 클릭 → atom B 클릭 → *거리 표시 (예: "H1-O2: 0.96 Å") + 선분 시각화*
   - **(S2)** Measurement / Angle → atom A → atom B → atom C → *각도 표시 (예: "104.5 deg") + 호 시각화*
   - **(S3)** Measurement / Dihedral → atom 4 개 → *이면각 표시 + 두 평면 시각화*
   - **(S4)** Measurement / Geometric Center → **드래그 셀렉션으로 atom 다수 선택** → *중심점 sphere 표시 + 라벨*
   - **(S5)** Measurement / Center of Mass → 드래그 셀렉션 → *질량 가중 중심점 표시 (Geometric Center 와 다름 입증 — `core::data::ElementDatabase` 의 atomicMass 사용)*
3. **추가 시나리오** — 다음 2 종 추가 검증:
   - **(S6)** S1 의 측정 결과 후 *atom A 삭제* → **측정 객체 자동 제거** (onAtomsChanged 두 번째 구독자 검증)
   - **(S7)** S4 의 측정 결과 후 *구조 제거* → **해당 구조의 모든 측정 자동 제거** (onStructureRemoved 8 번째 구독자 검증)
4. **Intentional UI deviation 사유서** — 의도적 변경 시 PR 본문 명기 (없으면 *"None"*). Phase 3.4 평가서 §1.7.3 의 5 종 사례 참조.

#### 1.4.4 압축과 UI 보존의 조화

| 압축 가능 ✓ | 보존 필수 ✗ |
|---|---|
| 한국어 깨진 인코딩 주석 → Doxygen | 5 모드의 *target pick count* (Distance 2 / Angle 3 / Dihedral 4 / Center N) |
| `m_parent->X()` → `controller_.X()` | 측정 결과 텍스트 format (`%.4f Å`, `%.2f deg` 등) |
| 미사용 `m_MeasurementXxx` 멤버 제거 | drag selection 의 *Shift/Ctrl 추가 모드* 동작 |
| `printf` 디버그 제거 | atom 삭제 시 *영향받는 측정 객체 자동 제거* 정책 |

---

## 2. 전제 — Phase 3.4 완료 상태

본 계획서는 다음이 충족된 상태에서 시작한다 (Phase 3.4 평가서 §4.3.A 의 commit 통과로 확인).

- [ ] **Phase 3.4 commit 머지** (3.4.1 + 3.4.2 + 3.4.3 의 28 파일 + edit_menu + core/scene/events.h + core/vtk/mouse_interactor 보강) — *Phase 0~3.4 의 7 회 연속 working tree 패턴 해소 필수*
- [ ] `webassembly/src/features/edit/` 28 파일 + 임시 메뉴 hook 정상 동작
- [ ] `core/{scene, io, data, vtk, render, ui}` 인프라 빌드 가능
- [ ] `npm run build-wasm:debug` + `:release` exit 0
- [ ] Edit 메뉴 3 항목 동작 (Phase 3.4 §5 검증 매트릭스 42 항목 PASS 34 / PARTIAL 8 / FAIL 0)
- [ ] **`core::vtk::MouseInteractor` 의 *첫 구독자 (atoms_controller)* 도달 + 인터페이스 안정화** (Phase 3.4 §1.5 + §2.2.2)
- [ ] **`core::scene::EventBus::onAtomsChanged` 의 *첫 + 두 번째 구독자 (atom_renderer + bonds_controller)* 도달** — 본 단계는 *세 번째 구독자* 가 될 예정
- [ ] **`vtk_renderer.cpp` 분할 100% 완료** (Phase 3.4 §1.2.3 V4) — 본 단계는 *legacy vtk_renderer 의존 0* 환경에서 진입

---

## 3. legacy 참조 인벤토리

| legacy 위치 | 라인 분량 | 역할 | 새 트리 행선지 |
|---|---|---|---|
| `legacy/atoms/atoms_template.cpp` line 3755~5500 (대략) | ~1,750 | EnterMeasurementMode / ExitMeasurementMode / HandleMeasurementEmptyClick / HandleMeasurementClickByPicker / RenderMeasurementModeOverlay / clampMeasurementStyles / 5 모드별 createXxxMeasurement / rebuildAngleMeasurementGeometry / applyXxxStyleToMeasurement / buildMeasurementAtomLabel | `features/measurement/` 18 파일로 분기 |
| `legacy/atoms/atoms_template.h` line 147~210 (public) + 969~1085 (private) | ~250 | MeasurementMode enum / DistanceMeasurement / AngleMeasurement / DihedralMeasurement / CenterMeasurement struct + DistanceStyle / AngleStyle / DihedralStyle / CenterStyle struct + GetDistanceMeasurementsForStructure / SetDistanceMeasurementVisible / RemoveDistanceMeasurement | `features/measurement/measurement_mode.{cpp,h}` (enum + state) + `measurement_store.{cpp,h}` (객체 리스트 + visibility) |
| `legacy/atoms/atoms_template.cpp::HandleDragSelectionInScreenRect` + `applyDragSelectionToMeasurement` (line 3825~3995) | ~170 | 드래그 셀렉션 + 측정에 적용 | `features/measurement/measurement_controller.{cpp,h}` 흡수 + `core/vtk/mouse_interactor` 보강 |
| `legacy/atoms/atoms_template.cpp::measurementOrderColor / measurementStyleColor` (line 942~957) | ~30 | 측정 종별 색상 팔레트 | `features/measurement/measurement_style.{cpp,h}` 또는 각 모드 파일 내부 |
| 측정 객체 lifecycle (vector::push_back, ID 발급, applyXxxStyleToAllMeasurements) | ~400 (분산) | 측정 객체 관리 | `measurement_store.{cpp,h}` 흡수 |
| **(신규)** | — | `measurement_controller`, `measurement_menu`, `measurement_overlay_ui`, 5 모드 (`distance`, `angle`, `dihedral`, `center`) | 신규 작성 |

**총 legacy 참조: 약 3,000 줄** (atoms_template.cpp 의 일부 + atoms_template.h 의 measurement 관련 분량). 압축 후 약 **2,200~2,400 줄** 예상 (Phase 3.1~3.4 의 압축 패턴 적용).

### 3.1 외부 의존 사전 분석

| legacy 코드 | 외부 의존 | 새 트리 처리 |
|---|---|---|
| `EnterMeasurementMode` 등 5 모드 진입 | `MeasurementMode` enum + `m_MeasurementMode` 멤버 | `measurement_mode.{cpp,h}` 의 state machine 으로 흡수 |
| `HandleMeasurementClickByPicker` | `m_MeasurementPickedAtomIds` + atom_manager 의 atoms 조회 | controller 로 분기 + `features::edit::atoms::AtomManager` (Phase 3.4.2) 의 *읽기 전용* 인터페이스 |
| `HandleDragSelectionInScreenRect` | vtkRenderer + atom 좌표 + screen rect | **`core::vtk::MouseInteractor` 의 drag selection 보강 (§1.2.1 옵션 A)** + measurement_controller 가 구독 |
| `createDistanceMeasurement` 등 | `m_DistanceMeasurements` vector + `uint32_t m_NextMeasurementId` | `measurement_store.{cpp,h}` |
| `rebuildAngleMeasurementGeometry` (벡터 외적/내적) | STL math | namespace 정리 후 그대로 이식 |
| `RenderMeasurementModeOverlay` | ImGui + vtkRenderer + atom 좌표 | `measurement_overlay_ui.{cpp,h}` 흡수 |
| `applyXxxStyleToMeasurement` | `DistanceStyle` / `AngleStyle` 등 struct | 5 모드 파일 안에 분산 또는 `measurement_style.{cpp,h}` 통합 |
| Center of Mass — atomicMass 조회 | `class ElementDatabase` (legacy) | **`core::data::ElementDatabase::getInstance().getElementInfo(symbol)->atomicMass` — Phase 3.3 / 3.4 와 동일 시그니처** |

### 3.2 메뉴 트리 매핑 (04 §6 Measurement 참조)

5 항목 모두 `EnterMode` 단일 디스패치.

| 메뉴 항목 | 새 진입점 | 윈도우 / 응답 |
|---|---|---|
| `Measurement / Distance` | `features::measurement::EnterMode(Mode::Distance)` | 오버레이 + 픽 시작 (2 atom) |
| `Measurement / Angle` | `features::measurement::EnterMode(Mode::Angle)` | 오버레이 + 픽 시작 (3 atom) |
| `Measurement / Dihedral` | `features::measurement::EnterMode(Mode::Dihedral)` | 오버레이 + 픽 시작 (4 atom) |
| `Measurement / Geometric Center` | `features::measurement::EnterMode(Mode::GeometricCenter)` | 오버레이 + 픽 시작 (N atom + drag) |
| `Measurement / Center of Mass` | `features::measurement::EnterMode(Mode::CenterOfMass)` | 오버레이 + 픽 시작 (N atom + drag) |

→ Phase 3.4 의 *3 메뉴 → 3 sub-folder* 와 달리 본 단계는 *5 메뉴 → 단일 sub-folder + 5 모드 dispatch* — Phase 3.1 (BZ) 의 *1 메뉴 → 1 sub-folder* 패턴과 동일 구조이되, *모드 토글* 차원에서 풍부.

### 3.3 mouse_interactor 두 번째 구독자 도착 (Phase 2 인프라 *재사용 패턴* 첫 검증)

Phase 3.4.2 의 atoms_controller (atom 클릭 → 테이블 행 강조) 가 첫 구독자였고, 본 단계의 measurement_controller (atom 클릭 → 모드별 응답) 가 두 번째 구독자. *동일 publisher 의 두 구독자가 충돌 없이 작동* 하는지 검증.

| 이벤트 | atoms_controller (Phase 3.4.2 첫 구독자) | measurement_controller (본 단계 두 번째 구독자) |
|---|---|---|
| `OnAtomPicked(atomId, pickPos)` | Created Atoms 테이블 행 강조 + 선택 갱신 | **모드별 응답** — Distance/Angle/Dihedral 의 pick count 진행, Center 의 atomIds 누적 |
| `OnDragSelection(atomIds, additive)` | Created Atoms 다중 선택 + 테이블 다중 강조 | **GeometricCenter/CenterOfMass 의 atomIds 입력** |
| `OnEmptyClick()` | 선택 해제 | **현재 모드의 pick 진행 상태 초기화** (legacy 의 `HandleMeasurementEmptyClick`) |

→ Phase 2 의 `core/vtk::MouseInteractor` 가 *N 구독자 모두에게 동일 이벤트 전달* 하는 *broadcaster* 인지 *first-come-first-served* 인지 본 단계가 첫 검증. 일반적으로 broadcaster 가 권장.

---

## 4. 작업 절차 (단계별)

### Step 1 — `features/measurement/` 폴더 신설

```bash
cd webassembly/src
mkdir -p features/measurement
```

### Step 2 — *(권장)* `core/vtk/mouse_interactor` + `core/scene/events.h` 보강

§1.2.1 옵션 A 채택. *최소 보강* 으로 본 단계의 needs 충족.

#### 2.1 `core/scene/events.h` 확장

```cpp
// 신규 이벤트 (Phase 3.5 신규)
struct AtomPickedEvent {
    int32_t structureId = -1;
    int32_t atomId = -1;
    double  pickPos[3] = {0.0, 0.0, 0.0};
};

struct DragSelectionEvent {
    int32_t              structureId = -1;
    std::vector<int32_t> atomIds;
    bool                 additive = false;
};

struct EmptyClickEvent {
    int32_t structureId = -1;
};

class EventBus {
public:
    // 기존 Bus...
    Bus<AtomPickedEvent>     onAtomPicked;          // ← 신규
    Bus<DragSelectionEvent>  onDragSelection;       // ← 신규
    Bus<EmptyClickEvent>     onEmptyClick;          // ← 신규
};
```

#### 2.2 `core/vtk/mouse_interactor` 보강

```cpp
// core/vtk/mouse_interactor.cpp 추가
void MouseInteractor::OnLeftButtonUp() override;     // 신규 — drag 종료
void MouseInteractor::OnMouseMove() override;        // 신규 — drag 중 사각 영역 갱신

private:
    // drag selection 상태
    bool   isDragging_ = false;
    int    dragStartX_ = 0, dragStartY_ = 0;
```

> *최소 보강* — legacy `HandleDragSelectionInScreenRect` 의 vtkPicker + screen rect 알고리즘을 mouse_interactor 안에 흡수 + DragSelectionEvent 발신.

### Step 3 — `measurement_mode.{cpp,h}` (state machine)

legacy `MeasurementMode` enum + state 변화 흐름.

```cpp
/**
 * @file features/measurement/measurement_mode.h
 * @brief Measurement 5 모드 state machine.
 */
#pragma once

namespace features::measurement {

enum class Mode {
    None = 0,
    Distance,
    Angle,
    Dihedral,
    GeometricCenter,
    CenterOfMass,
};

/// @brief 모드별 target pick count.
size_t TargetPickCount(Mode mode);

/// @brief 모드가 중심 측정 (Geometric/Mass) 인지.
bool IsCenterMode(Mode mode);

/// @brief 모드가 drag selection 을 활성화하는지.
bool IsDragSelectionEnabled(Mode mode);

/// @brief 모드의 사용자 표시 이름.
const char* ModeName(Mode mode);

} // namespace features::measurement
```

### Step 4 — `measurement_store.{cpp,h}` (객체 리스트 + visibility)

legacy 의 `m_DistanceMeasurements` / `m_AngleMeasurements` / `m_DihedralMeasurements` + `CenterMeasurement` vector + visibility 정책.

```cpp
/**
 * @file features/measurement/measurement_store.h
 * @brief 5 종 측정 객체 lifecycle + 구조별 visibility.
 */
#pragma once
#include "measurement_mode.h"
#include "../../core/scene/scene_state.h"
#include <vector>
#include <array>
#include <string>

namespace features::measurement {

struct DistanceMeasurement {
    uint32_t id = 0;
    int32_t  structureId = -1;
    int32_t  atomA = -1, atomB = -1;
    float    distance = 0.0f;
    bool     visible = true;
};

struct AngleMeasurement { /* ... */ };
struct DihedralMeasurement { /* ... */ };
struct CenterMeasurement {
    uint32_t id = 0;
    Mode     type = Mode::GeometricCenter;
    int32_t  structureId = -1;
    std::vector<int32_t> atomIds;
    std::array<float, 3> centroid;
    bool     visible = true;
};

class MeasurementStore {
public:
    explicit MeasurementStore(core::scene::SceneState& scene);

    /// @brief **`onAtomsChanged` 두 번째 구독자** — atom 삭제 시 잘못된 측정 자동 제거.
    void Subscribe();

    uint32_t AddDistance(int32_t structureId, int32_t atomA, int32_t atomB);
    uint32_t AddAngle(int32_t structureId, int32_t atomA, int32_t atomB, int32_t atomC);
    uint32_t AddDihedral(int32_t structureId, int32_t atomA, int32_t atomB, int32_t atomC, int32_t atomD);
    uint32_t AddCenter(int32_t structureId, Mode mode, const std::vector<int32_t>& atomIds);

    void SetVisible(uint32_t measurementId, bool visible);
    void Remove(uint32_t measurementId);
    void RemoveByStructure(int32_t structureId);
    void Clear();

    // 조회 (UI 패널 표시용)
    std::vector<DistanceMeasurement>  GetDistancesForStructure(int32_t structureId) const;
    /* ... */

private:
    void OnAtomsChanged(int32_t structureId);   // 측정 객체의 atomId 가 여전히 유효한지 검증

    core::scene::SceneState&             scene_;
    uint32_t                             nextMeasurementId_ = 1;
    std::vector<DistanceMeasurement>     distances_;
    std::vector<AngleMeasurement>        angles_;
    std::vector<DihedralMeasurement>     dihedrals_;
    std::vector<CenterMeasurement>       centers_;
};

} // namespace features::measurement
```

> **§1.1 인프라 재사용 검증**: `Subscribe()` 안에서:
> ```cpp
> scene_.events.onAtomsChanged.Subscribe([this](const auto& e) { OnAtomsChanged(e.structureId); });
> scene_.events.onStructureRemoved.Subscribe([this](const auto& e) { RemoveByStructure(e.structureId); });
> ```
> *onAtomsChanged 세 번째 구독자* + *onStructureRemoved 8 번째 구독자* 동시 도달.

### Step 5 — 5 모드 파일 — `distance.{cpp,h}` / `angle.{cpp,h}` / `dihedral.{cpp,h}` / `center.{cpp,h}`

각 모드의 *클릭 응답 / 계산 / 시각화* 분리.

```cpp
/**
 * @file features/measurement/distance.h
 * @brief Distance 모드 — 2 atom pick → 거리 계산 + 선분 시각화.
 */
#pragma once
#include "measurement_store.h"

namespace features::measurement::distance {

/// @brief 픽된 2 atom 으로 distance 측정 생성.
uint32_t CreateFromPickedAtoms(MeasurementStore& store, int32_t structureId,
                                int32_t atomA, int32_t atomB);

/// @brief 측정 결과의 vtk 액터 생성 / 갱신 / 제거.
void Render(const DistanceMeasurement& m, const core::scene::SceneState& scene);

} // namespace features::measurement::distance
```

> `angle.cpp` 는 legacy 의 *각도 계산 + 호 geometry* (`rebuildAngleMeasurementGeometry`), `dihedral.cpp` 는 *이면각 계산 + 두 평면*, `center.cpp` 는 *GeometricCenter + CenterOfMass 통합* (Mode 별 분기 — CenterOfMass 가 `core::data::ElementDatabase::getInstance().getElementInfo(symbol)->atomicMass` 호출).

### Step 6 — `measurement_controller.{cpp,h}` (통합 진입점)

Phase 3.4 평가서 §1.3 의 *외부 publisher 가 많을수록 풍부 controller* 패턴 적용. 본 단계는 *mouse_interactor + onSelectionChanged + onAtomsChanged + onStructureRemoved* 4 구독으로 풍부 controller 예상.

```cpp
/**
 * @file features/measurement/measurement_controller.h
 * @brief Measurement 의 통합 진입점.
 */
#pragma once
#include "measurement_mode.h"
#include "measurement_store.h"
#include "../../core/scene/scene_state.h"
#include "../../core/vtk/mouse_interactor.h"

namespace features::measurement {

class MeasurementController {
public:
    explicit MeasurementController(core::scene::SceneState& scene);

    /// @brief **mouse_interactor 두 번째 구독자** (atoms_controller 가 첫 구독자).
    void Subscribe(core::vtk::MouseInteractor& mouseInteractor);

    /// @brief 모드 진입 / 종료.
    void EnterMode(Mode mode);
    void ExitMode();
    Mode CurrentMode() const { return mode_; }

    /// @brief 현재 픽 진행 상태 (UI 오버레이 표시용).
    size_t PickedCount() const { return pickedAtomIds_.size(); }

    MeasurementStore& Store() { return store_; }

private:
    /// @brief AtomPickedEvent 응답 (Phase 3.5 신규 이벤트 — §2.1).
    void OnAtomPicked(const core::scene::AtomPickedEvent& event);

    /// @brief DragSelectionEvent 응답 (Center 모드).
    void OnDragSelection(const core::scene::DragSelectionEvent& event);

    /// @brief 빈 영역 클릭 — pick 진행 초기화.
    void OnEmptyClick();

    /// @brief 픽 진행 완료 시 측정 객체 생성.
    void TryCommitMeasurement();

    core::scene::SceneState&  scene_;
    MeasurementStore          store_;
    Mode                      mode_ = Mode::None;
    std::vector<int32_t>      pickedAtomIds_;
};

} // namespace features::measurement
```

### Step 7 — `measurement_overlay_ui.{cpp,h}` (오버레이 + 객체 리스트 UI)

§1.4.1 + §1.4.2 보존.

```cpp
/**
 * @file features/measurement/measurement_overlay_ui.h
 * @brief Measurement 오버레이 + 객체 리스트 — §1.4 보존.
 */
#pragma once
#include <imgui.h>

namespace features::measurement {

class MeasurementController;

class MeasurementOverlayUI {
public:
    explicit MeasurementOverlayUI(MeasurementController& controller);

    /// @brief 매 프레임 호출 — 오버레이 (모드 표시 + 픽 진행 + 결과) 그리기.
    void RenderOverlay();

    /// @brief 객체 리스트 패널 — Measurement 윈도우 안.
    void RenderListPanel(bool* open);

private:
    MeasurementController& controller_;
    // ★ §1.4 보존: 5 종 탭 + 색상 키 + visibility 토글 + 삭제 버튼 + Clear All 그대로
};

} // namespace features::measurement
```

### Step 8 — `measurement_menu.{cpp,h}` (5 항목 dispatch)

Phase 3.1 의 `bz_menu` + Phase 3.4 의 `edit_menu` 패턴 결합.

```cpp
/**
 * @file features/measurement/measurement_menu.h
 * @brief Measurement 메뉴 — 5 모드 EnterMode 단일 디스패치.
 */
#pragma once
namespace core::scene { struct SceneState; }
namespace core::vtk { class MouseInteractor; }

namespace features::measurement {

void DrawMenu();
void RenderWindows();
void Tick(float dt);
void Shutdown();
void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mouseInteractor);

}
```

```cpp
// features/measurement/measurement_menu.cpp
namespace features::measurement {

namespace {
    bool g_showList = false;
    MeasurementController* g_ctrl    = nullptr;
    MeasurementOverlayUI*  g_overlay = nullptr;
}

void DrawMenu() {
    if (ImGui::BeginMenu("Measurement")) {
        if (ImGui::MenuItem("Distance"))          g_ctrl->EnterMode(Mode::Distance);
        if (ImGui::MenuItem("Angle"))             g_ctrl->EnterMode(Mode::Angle);
        if (ImGui::MenuItem("Dihedral"))          g_ctrl->EnterMode(Mode::Dihedral);
        if (ImGui::MenuItem("Geometric Center"))  g_ctrl->EnterMode(Mode::GeometricCenter);
        if (ImGui::MenuItem("Center of Mass"))    g_ctrl->EnterMode(Mode::CenterOfMass);
        ImGui::Separator();
        if (ImGui::MenuItem("List", nullptr, g_showList)) g_showList = !g_showList;
        ImGui::EndMenu();
    }
}

void RenderWindows() {
    if (g_overlay) {
        g_overlay->RenderOverlay();
        if (g_showList) g_overlay->RenderListPanel(&g_showList);
    }
}

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mi) {
    static MeasurementController ctrl(scene);
    static MeasurementOverlayUI  overlay(ctrl);
    ctrl.Store().Subscribe();       // ★ onAtomsChanged 3 번째 + onStructureRemoved 8 번째 구독자
    ctrl.Subscribe(mi);              // ★ mouse_interactor 두 번째 구독자
    g_ctrl    = &ctrl;
    g_overlay = &overlay;
}

}
```

### Step 9 — `app/app.cpp` 임시 메뉴 hook

```cpp
#include "../features/measurement/measurement_menu.h"   // 신규

void App::renderDockSpace() {
    if (ImGui::BeginMenuBar()) {
        ...
        features::utilities::bz::DrawMenu();
        features::data::DrawMenu();
        features::build::DrawMenu();
        features::edit::DrawMenu();
        features::measurement::DrawMenu();             // ← 신규 Phase 3.5
        ImGui::EndMenuBar();
    }
    features::utilities::bz::RenderWindows(nullptr);
    features::data::RenderWindows();
    features::build::RenderWindows();
    features::edit::RenderWindows();
    features::measurement::RenderWindows();             // ← 신규
}

int App::Init() {
    static core::scene::SceneState scene;
    static core::vtk::MouseInteractor mi(scene);
    features::utilities::bz::InitOnce(scene);
    features::data::InitOnce(scene);
    features::build::InitOnce(scene);
    features::edit::InitOnce(scene, mi);
    features::measurement::InitOnce(scene, mi);          // ← 신규 (mi 재사용 — 두 번째 구독자)
}
```

### Step 10 — `CMakeLists.txt` SOURCES_FEATURES 확장

```cmake
set(SOURCES_FEATURES
    # Phase 3.1~3.4
    ...
    # Phase 3.5 신규 (~18 파일)
    webassembly/src/features/measurement/measurement_menu.cpp
    webassembly/src/features/measurement/measurement_menu.h
    webassembly/src/features/measurement/measurement_mode.cpp
    webassembly/src/features/measurement/measurement_mode.h
    webassembly/src/features/measurement/measurement_store.cpp
    webassembly/src/features/measurement/measurement_store.h
    webassembly/src/features/measurement/measurement_controller.cpp
    webassembly/src/features/measurement/measurement_controller.h
    webassembly/src/features/measurement/measurement_overlay_ui.cpp
    webassembly/src/features/measurement/measurement_overlay_ui.h
    webassembly/src/features/measurement/distance.cpp
    webassembly/src/features/measurement/distance.h
    webassembly/src/features/measurement/angle.cpp
    webassembly/src/features/measurement/angle.h
    webassembly/src/features/measurement/dihedral.cpp
    webassembly/src/features/measurement/dihedral.h
    webassembly/src/features/measurement/center.cpp
    webassembly/src/features/measurement/center.h
)
```

총 18 신규 파일.

### Step 11 — 정적 검증

```bash
cd webassembly/src

# legacy 호출 0
grep -rnE "AtomsTemplate::|atoms::domain::|atoms::infrastructure::|atoms::ui::" \
  features/measurement/ | grep -v "//" || echo "OK"

# legacy include 0
grep -rnE '#include\s+"(\.\./)*legacy/' features/measurement/ | grep -v "//" || echo "OK"

# legacy vtk_renderer 의존 0
grep -rnE "atoms::infrastructure::VTKRenderer" features/measurement/ | grep -v "//" || echo "OK"

# **mouse_interactor 두 번째 구독자 확인**
grep -rnE "MouseInteractor|onAtomPicked\.Subscribe|onDragSelection\.Subscribe" \
  features/measurement/ | head -10

# **onAtomsChanged 세 번째 구독자 확인**
grep -rnE "onAtomsChanged\.Subscribe" features/measurement/ | head -5

# **onStructureRemoved 8 번째 구독자 확인**
grep -rnE "onStructureRemoved\.Subscribe" features/measurement/ | head -5

# **element_database 호출 — CenterOfMass 의 atomicMass**
grep -rnE "core::data::ElementDatabase::getInstance|atomicMass" \
  features/measurement/ | head -5

# 5 모드 enum 보존
grep -nE "Distance|Angle|Dihedral|GeometricCenter|CenterOfMass" \
  features/measurement/measurement_mode.h

# namespace 일관성
grep -rnE "^namespace features::measurement" features/measurement/

# **§1.4 UI 보존 — 오버레이 위젯 + format 문자열**
grep -nE 'ImGui::|"%.4f|"%.2f' features/measurement/measurement_overlay_ui.cpp
grep -nE 'ImGui::|"%.4f|"%.2f' legacy/atoms/atoms_template.cpp | head -10
```

### Step 12 — 빌드 + 런타임 검증 (Windows 측)

```powershell
cd C:\Users\user\Downloads\vtk-workbench_jclee_orig
npm run rm-build
npm run build-wasm:debug
npm run build-wasm:release
npm run dev
```

**§1.4 보존 검증**:

- [ ] Side-by-side 스크린샷 (legacy Measurement 오버레이 + 객체 리스트 vs 새 트리)
- [ ] §1.4.3 의 시나리오 S1~S7 (5 모드 + atom 삭제 + 구조 제거) 각 legacy 와 동일 결과 확인
- [ ] ***동일 publisher 의 N 구독자 패턴* 검증** — atoms_controller (Phase 3.4.2 첫 구독자) + measurement_controller (본 단계 두 번째) 가 *충돌 없이 동시 작동* 입증
- [ ] Intentional UI deviation 사유서

### Step 13 — 커밋 & PR

```powershell
git commit -m "Phase 3.5: features/measurement migration

- New folder: features/measurement/ (18 files).
- Menu bar: Measurement / Distance · Angle · Dihedral · Geometric Center · Center of Mass.
- core/vtk::MouseInteractor second subscriber — pair with atoms_controller (Phase 3.4.2 first).
- core/scene::EventBus::onAtomsChanged third subscriber — auto-remove invalid measurements on atom deletion.
- core/scene::EventBus::onStructureRemoved eighth subscriber.
- (minimal infra reinforcement) core/scene/events.h: AtomPickedEvent/DragSelectionEvent/EmptyClickEvent
- (minimal infra reinforcement) core/vtk/mouse_interactor: OnLeftButtonUp/OnMouseMove for drag selection.
- core/data::ElementDatabase 17th call (Phase 3.3 8 + Phase 3.4 8 + 3.5 1) — atomicMass for CenterOfMass.
- legacy Measurement overlay + list UI preserved 1:1 per §6.0.1 + Phase 3.4 evaluation §1.7.3.

Side-by-side screenshots and S1~S7 scenario tests attached.
Intentional UI deviation: None / (or per scenario list).

Reference:
  - webassembly/docs/05_redevelopment_plan.md (Phase 3.5 + §6.0)
  - webassembly/docs/phases/phase3_5_measurement.md
"
```

---

## 5. 검증 매트릭스

| # | 검증 항목 | 명령 / 방법 | 기대값 | 시점 |
|---|---|---|---|---|
| 1 | `features/measurement/` 폴더 존재 | `ls features/` | measurement 폴더 | 정적 |
| 2 | 파일 수 | `ls features/measurement/` | 18 (.cpp 9 + .h 9) | 정적 |
| 3 | namespace 일관성 (`features::measurement::*`) | grep | 모든 .cpp/.h | 정적 |
| 4 | legacy 호출 0 | grep | 0 hit | 정적 |
| 5 | `#include "../legacy/"` 0 | grep | 0 hit | 정적 |
| 6 | legacy `vtk_renderer` 의존 0 | grep | 0 hit | 정적 |
| 7 | **`MouseInteractor` 두 번째 구독자** (Subscribe 호출) | grep | 1+ hit (measurement_controller) | 정적 — 핵심 |
| 8 | **`onAtomsChanged` 세 번째 구독자** | grep `onAtomsChanged.Subscribe` | 1+ hit (measurement_store) | 정적 — 핵심 |
| 9 | **`onStructureRemoved` 8 번째 구독자** | grep `onStructureRemoved.Subscribe` | 1+ hit | 정적 |
| 10 | **`core::data::ElementDatabase::getInstance` 호출** (Center of Mass) | grep | 1+ hit (center.cpp 의 atomicMass) | 정적 — 검증 |
| 11 | 5 모드 enum 보존 | grep `Distance.*Angle.*Dihedral.*GeometricCenter.*CenterOfMass` | 5 enum | 정적 |
| 12 | mouse_interactor 보강 (drag selection) | grep `OnLeftButtonUp\|OnMouseMove\|isDragging_` | 신규 추가 확인 | 정적 |
| 13 | events.h 의 신규 이벤트 (AtomPicked/DragSelection/EmptyClick) | grep | 3 신규 struct + 3 신규 Bus | 정적 |
| 14 | `app/app.cpp` 의 features::measurement 호출 추가 | grep | 4 hit (include + InitOnce + DrawMenu + RenderWindows) | 정적 |
| 15 | CMakeLists.txt SOURCES_FEATURES 확장 | grep `features/measurement/` | 18 파일 항목 | 정적 |
| 16 | **§1.4 UI 보존 — 오버레이 ImGui 위젯 + format 문자열** | legacy 와 새 트리 grep diff | 일치 (의도적 차이는 §1.4.4 사유서) | 정적 |
| 17 | Debug 빌드 | `npm run build-wasm:debug` | exit 0 | 동적 |
| 18 | Release 빌드 | `npm run build-wasm:release` | exit 0 | 동적 |
| 19 | 메뉴바에 Measurement 메뉴 추가 (5 항목) | `npm run dev` | Edit 옆 Measurement 메뉴 + 5 sub-item | 동적 |
| 20 | **§1.4 — Side-by-side 스크린샷** (오버레이 + 객체 리스트) | legacy vs 새 트리 시각 비교 | 위젯 동일 | 동적 — 핵심 |
| 21 | **§1.4 — 시나리오 S1~S3** (Distance / Angle / Dihedral) | 각 모드 1 측정 생성 | legacy 와 동일 결과 (값 + 시각화) | 동적 — 핵심 |
| 22 | **§1.4 — 시나리오 S4~S5** (Geometric Center / Center of Mass) | 드래그 셀렉션 → 중심 생성 | legacy 와 동일 + Center of Mass 가 atomicMass 사용 입증 | 동적 — 핵심 |
| 23 | **§1.4 — 시나리오 S6** (atom 삭제 → 측정 자동 제거) | 측정 후 atom 삭제 | 측정 객체 자동 사라짐 — onAtomsChanged 세 번째 구독자 검증 | 동적 — 핵심 |
| 24 | **§1.4 — 시나리오 S7** (구조 제거 → 측정 일괄 제거) | 측정 후 구조 제거 | 해당 구조의 측정 일괄 사라짐 — onStructureRemoved 8 번째 구독자 검증 | 동적 |
| 25 | **mouse_interactor *동일 publisher N 구독자* 패턴** | atoms_controller (Phase 3.4.2) + measurement_controller (본 단계) 동시 작동 | 두 구독자 모두 응답 (예: Edit / Atoms 윈도우 열어둔 채 Measurement 모드 진입 시 양쪽 상태 갱신) | 동적 — *재사용 시험대* |
| 26 | 콘솔 에러 0 | DevTools | 0 errors | 동적 |
| 27 | wasm 사이즈 회귀 | Phase 3.4 ± 5~10 % | 정상 범위 | 동적 — *권장* (Phase 3.2~3.4 §1.6.6 의 완화 정책 적용) |

---

## 6. 리스크 / 완화책

| # | 리스크 | 영향 | 완화 |
|---|---|---|---|
| 6.1 | **mouse_interactor *동일 publisher N 구독자* 패턴 실패** — atoms_controller 가 *atom 선택 흡수* 후 measurement_controller 까지 이벤트 전달 안 됨 | 측정 모드에서 atom 픽 불가 | (a) `core/vtk/mouse_interactor` 가 *broadcaster* (subscribers 의 *모두에게* 전달) 인지 확인. (b) atoms_controller 가 *event consume* 안 하도록 검증. (c) §5 #25 시나리오 검증 |
| 6.2 | drag selection 보강 시 *Phase 3.4.2 의 atoms_controller* 와 응답 충돌 — 두 구독자가 *서로 다른 사각 영역* 응답 | atoms 다중 선택과 measurement Center atomIds 가 *동일 사용자 액션* 으로 둘 다 발생 | (a) 모드 컨텍스트 (Edit/Measurement 어느 메뉴 활성 상태인지) 로 분기 — `MeasurementController::CurrentMode() != None` 이면 *atoms_controller 가 drag selection 무시*. (b) §5 #25 시나리오 |
| 6.3 | `RenderMeasurementModeOverlay` 의 *3D + 2D 혼합 렌더* — vtkRenderer 의 actors + ImGui 오버레이 동시 진행 시 layering 깨짐 | 오버레이가 atom sphere 뒤에 그려짐 | legacy 의 RenderMeasurementModeOverlay 흐름을 그대로 보존 (zOrder + ImGui::GetForegroundDrawList 사용) |
| 6.4 | Angle / Dihedral 계산 시 *atom 좌표가 fractional 인지 cartesian 인지* 혼동 | 각도 / 이면각 값 차이 | atom_manager 의 cartesian 좌표 직접 사용 — legacy 의 `m_DistanceMeasurements[i].distance` 가 cartesian 거리이므로 일관 |
| 6.5 | CenterOfMass 의 `atomicMass` 필드 부재 또는 시그니처 차이 | link error 또는 silent 0 가중치 | Phase 2 의 `core::data::ElementInfo` 에 `atomicMass` 필드 사전 확인. 부재 시 *Phase 2 보강* (회색지대 §1.2 shim 또는 element_database 확장 PR) |
| 6.6 | onAtomsChanged 세 번째 구독자 도입 시 *원자 좌표 변경만* (삭제 아님) 발생 시 측정 객체 *과도 제거* | 측정 표시 깜빡임 | AtomsChangedEvent 의 *세분화* — `kind: Added/Removed/Moved` 필드 추가 검토 (Phase 4 인프라 안정화 시) 또는 본 단계에서는 *측정 객체의 atomId 유효성만* 검증 (좌표 변경 시 재계산만, 제거 안 함) |
| 6.7 | onStructureRemoved 8 번째 구독자 — 기존 7 구독자 (Phase 3.1~3.4) 와 *실행 순서 보장 안 됨* | 다른 구독자가 *측정 객체 의존* 가능 (현 시점에는 없음) | 본 단계에서는 측정 객체가 다른 도메인에 의존하므로 *측정 제거가 가장 먼저* — Phase 4 인프라 안정화 시 priority 기능 검토 |
| 6.8 | **§1.4 UI 보존 위반** — 5 모드 진입 순서 / 오버레이 텍스트 format / Center 마커 색상 변경 가능 | 사용자 워크플로우 변경 | §5 #16 grep diff sweep + Side-by-side 스크린샷 + S1~S5 시나리오 |
| 6.9 | controller 풍부도 — mouse_interactor + onAtomPicked + onDragSelection + onEmptyClick + onSelectionChanged + onAtomsChanged + onStructureRemoved 의 *최대 7 구독* | controller 라인수 폭증 가능 | §9.1 의 라인수 예상에 *얇은 vs 풍부* 두 시나리오 명시 (Phase 3.4 평가서 §1.7.4 의 controller 두께 변동성) |
| 6.10 | 단일 PR 의 라인수가 *Phase 평균 (~2,200)* 이내라 분할 불필요 — 단 *core/* 보강 + 18 features 파일 + 시나리오 7 종 검증 부담 | 검토 부담 ~1× | 본 단계는 *단일 PR* 진행 — 단 §1.7.5 의 *분할 시그널* 모니터링 (실제 라인수가 ~2,600 초과 시 분할 재검토) |
| 6.11 | **Phase 3.4 평가서 §1.7.3 의 *Intentional deviation* 정책 적용** — *재학습 부담 없이 사용자 워크플로우 개선* 으로 일부 변경 가능 | 정책 해석 차이 | PR 본문에 *deviation 사유서* 명시 (Phase 3.4 의 5 사례 형식 참조) |
| 6.12 | **Phase 3.4 평가서 §1.7.5 의 commit 정책 격상** — 본 단계 PR 의 *base commit 미정* 가능성 | Phase 3.6 진입 base 모호 | §2 전제 항목 첫 행에 *"Phase 3.4 commit 머지 — *7 회 연속 working tree 패턴 해소 필수*"* 명시 |

---

## 7. 롤백 절차

```bash
git restore --staged .
git restore .
git clean -fd webassembly/src/features/measurement
git checkout -- webassembly/src/core/scene/events.h webassembly/src/core/vtk/mouse_interactor.{cpp,h}
```

원인 진단 우선순위:

1. **mouse_interactor 두 구독자 응답 충돌** — §6.1/§6.2 의 broadcaster 정책 점검.
2. **§1.4 UI 보존 위반** — §6.8 의 즉시 복원 또는 사유서.
3. **CenterOfMass atomicMass 미해결** — §6.5 의 Phase 2 보강.
4. **mouse_interactor / events.h 보강 회귀** — Phase 3.4 의 atoms_controller 동작 회귀 시 보강 코드 검토.

---

## 8. PR 체크리스트

작성자 — PR 올리기 전:

- [ ] **§2 의 Phase 3.4 전제 충족 — *7 회 연속 working tree 패턴 해소 (commit 머지)***  *(Phase 3.4 평가서 §1.7.5 정책 격상 사항)*
- [ ] §5 검증 매트릭스 27 항목 통과
- [ ] features/measurement/ 안에서 legacy 호출 0 + legacy include 0 + vtk_renderer 의존 0
- [ ] **`MouseInteractor.Subscribe` 호출 1+ 곳** (두 번째 구독자 — atoms_controller 와 동일 publisher)
- [ ] **`onAtomsChanged.Subscribe` 1+ 곳** (세 번째 구독자) + **`onStructureRemoved.Subscribe` 1+ 곳** (8 번째 구독자)
- [ ] **`core::data::ElementDatabase::getInstance` 호출 1+ 곳** (Center of Mass — Phase 3.3/3.4 와 동일 시그니처)
- [ ] mouse_interactor 보강 — drag selection 의 OnLeftButtonUp / OnMouseMove 추가 + DragSelectionEvent 발신
- [ ] events.h 신규 3 이벤트 (AtomPickedEvent / DragSelectionEvent / EmptyClickEvent)
- [ ] **§1.4 UI 보존 — Side-by-side 스크린샷 첨부** (오버레이 + 객체 리스트)
- [ ] **§1.4 — 시나리오 S1~S7 각 legacy 와 동일 결과 확인**
- [ ] **§5 #25 — *동일 publisher N 구독자* 패턴 검증** (atoms_controller + measurement_controller 동시 작동)
- [ ] **Intentional UI deviation 사유서 작성** (없으면 *"None"*) — Phase 3.4 평가서 §1.7.3 의 5 사례 형식 참조
- [ ] `legacy/` 한 글자도 변경 없음 (atoms_template.cpp 의 측정 부분 분할은 *복사 + 새 트리 정리* 로만)
- [ ] PR 본문에 *"Phase 3.6 file 진입 — format_registry 의 *읽기 사용자* 가 됨"* 명시

검토자 — 머지 전:

- [ ] diff 가 (a) features/measurement/ 신규 18 파일, (b) app/app.cpp 임시 hook 4 곳, (c) CMakeLists.txt source 추가, (d) core/scene/events.h 신규 3 이벤트, (e) core/vtk/mouse_interactor 의 drag selection 보강 — 5 가지로만 구성?
- [ ] **첨부 스크린샷에서 legacy 와 새 트리의 Measurement 오버레이 + 객체 리스트가 시각적으로 동일?**
- [ ] **시나리오 S1~S7 본인 환경에서도 동일 결과 재현?** (특히 S6/S7 의 자동 제거)
- [ ] **§5 #25 — atoms_controller + measurement_controller *동시 작동* 본인 환경 재현?**
- [ ] mouse_interactor 보강이 Phase 3.4.2 의 atoms_controller 동작에 *회귀 없음* 본인 환경 재현?
- [ ] `legacy/` 0 changed lines
- [ ] CI 빌드 (debug + release) 통과

---

## 9. 부록

### 9.1 features/measurement/ 18 파일 요약 + 라인수 예상

| 파일 | 역할 | 라인 (legacy) | 압축 후 (얇은 controller) | 압축 후 (풍부 controller) |
|---|---|---|---|---|
| `measurement_menu.{cpp,h}` | 5 모드 dispatch + InitOnce (mouse_interactor 두 번째 구독자 등록) | — | ~150 | ~150 |
| `measurement_mode.{cpp,h}` | 5 모드 state machine + TargetPickCount/IsCenterMode 헬퍼 | (atoms_template.h 의 enum + cpp 분량 ~80) | ~120 | ~120 |
| `measurement_store.{cpp,h}` | 객체 lifecycle + visibility + onAtomsChanged/onStructureRemoved 구독 | (atoms_template.cpp 의 vector + ID + Remove ~400) | ~300 | ~300 |
| `measurement_controller.{cpp,h}` | 통합 진입점 + mouse_interactor 구독 + onSelectionChanged 첫 구독자 + 모드별 응답 | (atoms_template.cpp 의 Handle*Click + applyDragSelectionToMeasurement ~500) | ~280 | ~450 |
| `measurement_overlay_ui.{cpp,h}` | 오버레이 (모드/픽진행/결과) + 객체 리스트 (§1.4 보존) | (RenderMeasurementModeOverlay + renderMeasurementStyleOptionsOverlay ~600) | ~500 | ~500 |
| `distance.{cpp,h}` | Distance 모드 (2 atom pick → 거리 + 선분) | (createDistanceMeasurement + applyDistanceStyle ~200) | ~220 | ~220 |
| `angle.{cpp,h}` | Angle 모드 (3 atom → 각도 + 호) | (createAngleMeasurement + rebuildAngleMeasurementGeometry + applyAngleStyle ~350) | ~280 | ~280 |
| `dihedral.{cpp,h}` | Dihedral 모드 (4 atom → 이면각 + 평면) | (createDihedralMeasurement + applyDihedralStyle ~330) | ~260 | ~260 |
| `center.{cpp,h}` | GeometricCenter + CenterOfMass 통합 (Mode 분기) — atomicMass 호출 | (createCenterMeasurement + applyCenterStyle ~280) | ~220 | ~220 |
| **합계** | **18 파일** | **~3,000** | **~2,330** (≈ 78%) | **~2,500** (≈ 83%) |

> Phase 3.4 평가서 §1.7.4 의 *controller 두께 변동성* 반영 — 두 시나리오 모두 명시. 본 단계는 *mouse_interactor + 4 EventBus 구독 + 모드 상태머신* 으로 *풍부 controller* 가능성 큼.

### 9.2 후속 sub-phase 자동 적용

| sub-phase | 본 패턴 적용 |
|---|---|
| 3.6 file | format_registry *읽기 사용자* — Phase 3.2 의 등록자 + 본 단계의 measurement_store *atomicMass 호출 패턴* 그대로 적용 |
| 3.7 viewer + toolbar | toolbar 의 inline 버튼 → 본 단계의 measurement_controller 의 *EnterMode/ExitMode* 호출 (Cell Align 처럼 *공개 콜백*) |
| 3.8 model_tree | measurement 객체의 *나무 표시* — 본 단계의 measurement_store *읽기 사용자* |

### 9.3 사후 점검: Phase 3.5 머지 직후 트리

```
webassembly/src/
├─ ... (Phase 0-2 + 3.1 + 3.2 + 3.3 + 3.4 유지)
├─ features/
│  ├─ utilities/brillouin_zone/        (Phase 3.1)
│  ├─ data/                            (Phase 3.2)
│  ├─ build/                           (Phase 3.3)
│  ├─ edit/                            (Phase 3.4)
│  └─ measurement/                     ★ 신규 Phase 3.5
│     ├─ measurement_menu.{cpp,h}
│     ├─ measurement_mode.{cpp,h}         ★ 5 모드 state machine
│     ├─ measurement_store.{cpp,h}        ★ onAtomsChanged 3 번째 + onStructureRemoved 8 번째 구독자
│     ├─ measurement_controller.{cpp,h}   ★ mouse_interactor 두 번째 구독자 (재사용 패턴 첫 검증)
│     ├─ measurement_overlay_ui.{cpp,h}   ★ §1.4 보존 핵심
│     ├─ distance.{cpp,h}
│     ├─ angle.{cpp,h}
│     ├─ dihedral.{cpp,h}
│     └─ center.{cpp,h}                   ★ core::data::ElementDatabase atomicMass 호출
└─ legacy/                              (동결)
```

### 9.4 Phase 3.4 평가서의 학습 반영 정리

본 계획서가 [`./phase3_4_evaluation_2026-05-11.md`](./phase3_4_evaluation_2026-05-11.md) 에서 흡수한 권장 항목.

| Phase 3.4 평가서 권장 | 본 계획서 적용 위치 |
|---|---|
| §1.5 Phase 2 인프라 *최종 시험대* → 본 단계 *재사용 시험대* | §0 + §1.1 + §5 #25 시나리오 |
| §1.7.1 bonds_controller 의 onAtomsChanged 두 번째 구독자 성능 부하 | §6.6 의 AtomsChangedEvent *세분화 검토* — Phase 4 인프라 안정화 시 |
| §1.7.2 core/* 인프라 진화 — 안정화 권장 | §1.2.1 의 *최소 보강* 옵션 A 채택 — events.h 3 신규 이벤트 + mouse_interactor 2 메서드 |
| §1.7.3 §1.4 UI 보존 *재해석* — Intentional deviation | §1.4 의 *재학습 부담 없이 사용자 워크플로우 개선* 정신 양립 + §6.11 |
| §1.7.4 controller 두께 변동성 — 외부 publisher 와 구독 채널 수 정비례 | §9.1 의 *얇은 vs 풍부 controller* 두 시나리오 |
| §1.7.5 commit 미수행 정책 격상 (7 회 연속) | **§2 전제 첫 행 + §8 PR 체크리스트 첫 행 강제** — *Phase 3.4 commit 머지* 명시 |
| §1.7.6 wasm 사이즈 정량 검증 완화 | §5 #27 을 *권장 수준* 으로 유지 |

### 9.5 동일 publisher N 구독자 패턴의 향후 적용 (Phase 3.6 이후)

본 단계의 mouse_interactor *두 번째 구독자 검증* 이 통과하면, 후속 sub-phase 가 동일 패턴을 *세 번째* 이상으로 확장 가능:

| 후속 단계 | mouse_interactor 추가 구독자 |
|---|---|
| 3.7 viewer + toolbar | *카메라 조작 응답* — 마우스 휠 / 우클릭 드래그 응답이 *세 번째 구독자* 가능 |
| 3.8 model_tree | *atom 강조 동기화* — model_tree 의 노드 선택이 mouse_interactor 의 *역방향* 이벤트로 발생 |
| 3.9 mesh | *mesh actor 픽킹* — 본 단계의 atom 픽킹 패턴 그대로 적용 |

---

## 10. 후속 단계 연결 — Phase 3.6

Phase 3.5 머지 후 Phase 3.6 (`features/file`) 진입.

1. `features/file/` 신설 — 단일 sub-folder, File / Open Structure File 메뉴.
2. legacy `file_loader.cpp` 의 메뉴 핸들러 부분 + `app.cpp` 의 File 메뉴 분기 이식.
3. **`core/io/format_registry` 의 *읽기 사용자* 도달** — Phase 3.2 의 *등록자* + 본 단계의 *호출자* 결합으로 *완전한 인프라 검증*.
4. *Phase 3.3 의 Add atom / Phase 3.4 의 Bravais Apply / Phase 3.5 의 Measurement* 가 *외부 파일 데이터 위에 작동* 하는 *전체 워크플로우* 가 본 단계 머지로 가능.
5. 메뉴: `File / Open Structure File`.

Phase 3.6 세부계획서는 `phase3_6_file.md` 에 별도 작성.

---

## 11. 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.5) + **§6.0 공통 지침 (UI 1:1 보존)** + §13 (UI 이식 공통 지침)
- 선행 평가서: [`./phase3_4_evaluation_2026-05-11.md`](./phase3_4_evaluation_2026-05-11.md)
- 선행 sub-phase: [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md) (인덱스) + [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md) + [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md) + [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md)
- 추가 선행 평가서: [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md), [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md), [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- Phase 2 (인프라): [`./phase2_core_skeleton.md`](./phase2_core_skeleton.md) — `core/scene/events`, `core/vtk/mouse_interactor`, `core/data/element_database`
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §6 Measurement
- UI 이식 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
