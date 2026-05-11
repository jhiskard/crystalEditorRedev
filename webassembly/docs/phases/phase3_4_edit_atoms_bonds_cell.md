# Phase 3.4 — Edit / Atoms + Bonds + Cell 이식 (분할 진행) 통합 인덱스

> 상위 문서:    [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + §6.0 공통 지침
> 분할 근거:    [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) — Option A 채택
> 선행 평가서:  [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
> 메뉴 매핑:    [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
> 작성일:      2026-05-07
> 대상 브랜치: `refactor/menu-aligned`
> 진행 단위:   **3 sub-phase** (3.4.1 Cell → 3.4.2 Atoms → 3.4.3 Bonds)
> 예상 총 소요: 6.5~8.5 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-07 | 초안 작성 — Phase 3.3 평가서 §6 권장 다음 단계 항목을 본 문서로 확장 (단일 통합 계획서) |
| 2026-05-07 (분할) | [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) 의 Option A 채택. 본 문서를 *통합 인덱스* 로 재구성. 세부 계획은 3 sub-phase 문서로 이관 |

---

## 0. 한 줄 요약

> Phase 3.4 는 *Phase 3 중 가장 무거운 단계* (legacy ~6,010 줄, 새 트리 ~4,400 줄, 22 파일, vtk_renderer.cpp 1,792 줄 분할 + EventBus 5 종 검증 + mouse_interactor 첫 구독자 + UI 3 윈도우 보존) 로, 단일 진행 시 Phase 3 평균 (16 파일 / 2,200 줄) 의 2 배. 분할 제안서 [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) 의 Option A 를 채택하여 **메뉴 항목 단위 3 sub-phase + 의존성 자연 순서 (Cell → Atoms → Bonds)** 로 진행한다. 각 sub-phase 가 단독 런타임 검증 가능 + Phase 3.1~3.3 와 비슷한 규모.

---

## 1. 분할 진행 sub-phase 인덱스

| sub-phase | 메뉴 항목 | 세부계획서 | 신규 파일 | legacy | 새 트리 (예상) | 일정 |
|---|---|---|---|---|---|---|
| **3.4.1** | `Edit / Cell` | [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md) | 10 (cell 8 + edit_menu skeleton 2) | 626 | ~700 | 1.5~2 일 |
| **3.4.2** | `Edit / Atoms` | [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md) | 10 (atoms 10 + edit_menu touch) | 2,755 | ~1,900 | 3~4 일 |
| **3.4.3** | `Edit / Bonds` | [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md) | 8 (bonds 8 + edit_menu touch) | 2,102 | ~1,400 | 2~2.5 일 |
| **합계** | 3 항목 | — | **28** *(edit_menu 3 회 touch)* | **6,010** | **~4,400** *(73% 압축)* | **6.5~8.5 일** |

### 1.1 진행 순서 (의존성 그래프)

```
Phase 3.3 commit (bb4ca37 등)
  │
  └→ Phase 3.4.1 (Cell)
        │  · features/edit/cell/ 신설
        │  · edit_menu skeleton (Cell 항목만 wiring)
        │  · vtk_renderer cell 부분 (~300 줄) 분할
        │  · onCellChanged 첫 구독자 (Phase 3.3 emit 자와 결합)
        │
        └→ Phase 3.4.2 (Atoms)
              │  · features/edit/atoms/ 신설
              │  · edit_menu 에 Atoms 항목 추가
              │  · vtk_renderer atom 부분 (~700 줄) 분할
              │  · onAtomsChanged + onStructureAdded 첫 구독자
              │  · mouse_interactor 첫 구독자
              │  · (선택) Phase 3.3 의 onStructureAdded emit 보강
              │
              └→ Phase 3.4.3 (Bonds)
                    · features/edit/bonds/ 신설
                    · edit_menu 에 Bonds 항목 추가
                    · vtk_renderer bond 부분 (~700 줄) 분할
                    · onBondsChanged emit + 구독 양방
                    · element_database 두 번째 사용자
```

각 sub-phase 는 *직전 sub-phase commit 머지* 를 base 로 진행. PR3 (3.4.3) 머지 후 Phase 3.5 (`features/measurement`) 진입.

---

## 2. 분할 채택의 효과 요약

[`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) §6 의 리스크 변화 분석 결과:

| 영역 | 단일 진행 | 3 분할 | 변화 |
|---|---|---|---|
| 검토자 부담 | 단일 PR diff ~4,400 줄 + 3 윈도우 + 5 EventBus + mouse_interactor 동시 진입 | sub-phase 별 1 윈도우 + 1~3 EventBus + 1 컴포넌트 | **결정적 완화** |
| 작성자 부담 | 5~7 일 | 6.5~8.5 일 (+20~30% 분할 오버헤드) | 약간 격상 |
| 회귀 추적 | 한 PR 안 32 항목 동시 검증 | sub-phase 별 14+14+7 = 35 항목 분산 | 완화 |
| Phase 3.3 atom 추가 시각화 | PR 머지 시점에 한 번에 보임 | 3.4.2 시점에 보임 (3.4.1 후 cell 만 보이고 atom 은 미시각화) | 약간 지연 (~3 일) |

→ 순 효과: **검토 통과 시간 단축 + 회귀 추적 용이**. 작성자 부담 +20~30% 는 분할 오버헤드의 자연 결과.

---

## 3. 공통 지침 (3 sub-phase 모두 적용)

각 sub-phase 의 세부계획서가 *상속* 하는 공통 지침은 본 §과 상위 §6.0 이다.

### 3.1 회색지대 정책 (Phase 0 §1.1 + Phase 1 §1.2 + Phase 3.1 §1.3)

| 회색지대 출처 | 적용 |
|---|---|
| Phase 0 §1.1 — typo build-fix | legacy/ 내부 1~5 줄 typo |
| Phase 1 §1.2 — 빌드 호환성 shim | 새 트리 측 얇은 redirect |
| Phase 3.1 §1.3 — 임시 메뉴 hook | `app/app.cpp::renderDockSpace` 의 `features::edit::DrawMenu()` |
| **Phase 3.4 §3.1.1 — vtk_renderer 분할 회색지대** *(신규)* | legacy `atoms/infrastructure/vtk_renderer.cpp` 1,792 줄을 *3 sub-phase 로 분산 분할*. legacy 측 코드 자체는 변경하지 않음 (동결본 유지). 새 트리 측 `*_renderer.{cpp,h}` 가 *해당 부분 코드 복사 + namespace 정리* |

### 3.2 라인수 압축 정책

Phase 3.1 (53%) / 3.2 (57.5%) / 3.3 (61.2%) 의 압축 패턴 적용. 본 단계는 *vtk_renderer 분할 + UI 보존 + 데이터 비중* 로 보수적 (~73% 예상).

**공통 보존 필수 (3 sub-phase 모두)**:

- vtk_renderer 의 *그룹별 액터 캐싱 정책* (`AtomGroupVTKData` / `BondGroupVTKData` / cell 캐시 구조)
- 각 *_renderer 의 vtk* 호출 sequence (legacy 와 1:1)
- §3.3 UI 보존 항목 (각 sub-phase 의 §1.4 참조)

### 3.3 legacy UI 1:1 보존 정책 (상위 §6.0.1)

각 sub-phase 가 *자신의 윈도우 1 종* 만 보존:

| sub-phase | 보존 대상 |
|---|---|
| 3.4.1 | Cell Information 윈도우 — `legacy/atoms/ui/cell_info_ui.cpp` (101 줄) |
| 3.4.2 | Created Atoms 윈도우 — `legacy/atoms/ui/atom_editor_ui.cpp` (896 줄) |
| 3.4.3 | Bonds Management 윈도우 — `legacy/atoms/ui/bond_ui.cpp` (220 줄) |

각 sub-phase 의 §1.4 에 *그 윈도우의 보존 항목* 표로 정리. 시나리오는 다음과 같이 분배.

| 시나리오 | 위치 | sub-phase |
|---|---|---|
| (S1) Add atom 시각화 | 3.4.2 — Phase 3.3 의 atom 추가가 *비로소 화면에 보이는 시점* | 3.4.2 |
| (S2) 좌표 편집 | 3.4.2 | 3.4.2 |
| (S3-cell) Bravais Apply → cell + matrix 갱신 | 3.4.1 (cell 측만), 3.4.2 (atoms 까지 완전 검증) | 3.4.1 + 3.4.2 |
| (S4) Bond Recompute → cylinder 표시 | 3.4.3 | 3.4.3 |
| (S5) atom 클릭 → 테이블 행 강조 | 3.4.2 — mouse_interactor | 3.4.2 |
| (S6) atom 드래그 셀렉션 | 3.4.2 | 3.4.2 |

### 3.4 EventBus 5 종 검증 분배

| EventBus | emit 자 (Phase 3.3) | 첫 구독자 sub-phase |
|---|---|---|
| `onAtomsChanged` | Phase 3.3 (BravaisController + PeriodicTableController 3 곳) | **3.4.2** atoms_controller / atom_renderer |
| `onCellChanged` | Phase 3.3 (BravaisController.Apply 1 곳) | **3.4.1** cell_controller / cell_renderer |
| `onBondsChanged` | (없음 — 3.4.3 이 첫 emit) | **3.4.3** (단일 sub-phase 안 양방) |
| `onStructureAdded` | (Phase 3.3 미발신 — 3.4.2 에서 emit 보강 + 구독 양방) | **3.4.2** (보강 + 구독 양방) |
| `onStructureRemoved` | Phase 3.3 (BravaisController 가 *구독* 만, emit 자 없음) | (Phase 3.4 에서 신규 도입 안 함 — 후속 단계 검토) |

### 3.5 PR 체크리스트 공통 항목 (3 sub-phase 모두)

각 sub-phase 의 §8 에서 다음 항목을 *상속*:

- [ ] 직전 sub-phase commit 머지 (3.4.1 의 경우 Phase 3.3 commit, 3.4.2 의 경우 3.4.1 commit, …)
- [ ] features/edit/<sub>/ 안에서 legacy 호출 0 + legacy include 0 + vtk_renderer 의존 0
- [ ] vtk_renderer 분할 — 해당 sub-phase 분의 vtk* 호출 갯수 legacy 와 비교 (±10% 범위)
- [ ] **§1.4 UI 보존 — Side-by-side 스크린샷 1 종 첨부** (해당 sub-phase 의 윈도우)
- [ ] **§1.4 — 시나리오 (sub-phase 별 정의) 각 legacy 와 동일 결과 확인**
- [ ] **Intentional UI deviation 사유서 작성** (없으면 *"None"*)
- [ ] `legacy/` 한 글자도 변경 없음 (vtk_renderer 분할은 *복사 + 새 트리 정리* 로만 진행)
- [ ] PR 본문에 본 인덱스 + 분할 제안서 + 해당 sub-phase 세부계획서 링크
- [ ] PR 헤더에 *(N/3) <name>* 표기 (3.4.1 = 1/3 Cell, 3.4.2 = 2/3 Atoms, 3.4.3 = 3/3 Bonds)

---

## 4. 사후 점검 — 3 sub-phase 머지 직후 트리

```
webassembly/src/
├─ ... (Phase 0-2 + 3.1 + 3.2 + 3.3 유지)
├─ features/
│  ├─ utilities/brillouin_zone/        (Phase 3.1)
│  ├─ data/                            (Phase 3.2)
│  ├─ build/                           (Phase 3.3)
│  └─ edit/                            ★ Phase 3.4 (3.4.1 + 3.4.2 + 3.4.3)
│     ├─ edit_menu.{cpp,h}             (3.4.1 신설 + 3.4.2/3.4.3 갱신)
│     ├─ cell/                         (3.4.1 — 8 파일)
│     │  ├─ cell_manager.{cpp,h}
│     │  ├─ cell_renderer.{cpp,h}        ★ vtk_renderer 분할 결과 (cell)
│     │  ├─ cell_controller.{cpp,h}      ★ onCellChanged 첫 구독자
│     │  └─ cell_info_ui.{cpp,h}         ★ §1.4 보존
│     ├─ atoms/                        (3.4.2 — 10 파일)
│     │  ├─ atom_manager.{cpp,h}
│     │  ├─ surrounding_atom_manager.{cpp,h}
│     │  ├─ atom_renderer.{cpp,h}        ★ vtk_renderer 분할 결과 (atom)
│     │  ├─ atoms_controller.{cpp,h}     ★ mouse_interactor + onAtomsChanged + onStructureAdded 첫 구독자
│     │  └─ atom_editor_ui.{cpp,h}       ★ §1.4 보존 핵심 (896 줄)
│     └─ bonds/                        (3.4.3 — 8 파일)
│        ├─ bond_manager.{cpp,h}         ★ element_database 두 번째 사용자
│        ├─ bond_renderer.{cpp,h}        ★ vtk_renderer 분할 결과 (bond)
│        ├─ bonds_controller.{cpp,h}
│        └─ bond_ui.{cpp,h}              ★ §1.4 보존
└─ legacy/                             (동결 — vtk_renderer 도 그대로)
```

---

## 5. 후속 단계 연결 — Phase 3.5

3 sub-phase 모두 머지 후 Phase 3.5 (`features/measurement`) 진입.

1. `features/measurement/` 신설 — 단일 sub-folder, 5 개 모드 토글.
2. legacy `atoms_template.cpp` 의 측정 관련 메서드 (Enter/Exit/Click/Drag/Render/Store/Visible) 이식.
3. **`core/vtk::MouseInteractor` 의 *두 번째 구독자* 도착** — Phase 3.4.2 의 atoms_controller 와 *동일 publisher 의 두 구독자* 패턴 첫 검증.
4. **`core/scene/EventBus::onAtomsChanged` 의 *두 번째 구독자* 도착** — atom 삭제 시 잘못된 측정 자동 제거.
5. 메뉴: `Measurement / Distance · Angle · Dihedral · Geometric Center · Center of Mass`.

Phase 3.5 세부계획서는 `phase3_5_measurement.md` 에 별도 작성.

---

## 6. 관련 문서

- 분할 근거: [`./phase3_4_split_proposal.md`](./phase3_4_split_proposal.md) — Option A 채택
- 분할 sub-phase 세부계획서:
  - [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md) — Phase 3.4.1 Cell
  - [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md) — Phase 3.4.2 Atoms
  - [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md) — Phase 3.4.3 Bonds
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.4) + §6.0 공통 지침 + §11 (vtk_renderer 분할 리스크) + §13 (UI 이식 공통 지침)
- 선행 평가서: [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
- 선행 계획서: [`./phase3_3_build_periodic_bravais.md`](./phase3_3_build_periodic_bravais.md)
- 추가 선행 평가서: [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md), [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- Phase 2 (인프라): [`./phase2_core_skeleton.md`](./phase2_core_skeleton.md) — `core/scene/events`, `core/vtk/mouse_interactor`, `core/data/element_database`
- 새 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3
- 메뉴 ↔ 코드 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §4 Edit
- UI 이식 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
- Doxygen 컨벤션: [`../06_doxygen_style_guide.md`](../06_doxygen_style_guide.md)
