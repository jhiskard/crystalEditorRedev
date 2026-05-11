# Phase 3.4 분할 방안 제안서 (2026-05-07)

> 상위 문서:  [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md) §4 Step 14 의 *3 PR 분할 권장* 항목 확장
> 선행 평가서:  [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
> 작성일:      2026-05-07
> 목적:        Phase 3.4 (legacy ~6,010 줄, 새 트리 ~4,000~5,000 줄, 22 파일) 를 *Phase 3.1/3.2/3.3 와 비슷한 단위* (16 파일 ± 2, 1,800~2,600 줄) 로 분할하는 권장안 제시

## 0. 한 줄 결론

> Phase 3.4 의 22 파일 / ~4,400 줄을 단일 sub-phase 로 진행하면 *Phase 3 통계 평균 (16 파일 / 2,200 줄) 의 2 배* 이고, vtk_renderer 1,792 줄 분할이 동시 진입하면 검토 부담이 ~3 배로 늘어난다. 본 문서는 4 가지 분할 대안 (A/B/C/D) 을 비교하여 **권장: Option A — 메뉴 항목 단위 3 sub-phase 분할 + 의존성 역순서 (Cell → Atoms → Bonds)** 를 제시한다. 각 sub-phase 가 단독으로 *런타임 검증 가능* + *Phase 3.1/3.2/3.3 와 비슷한 규모* 라는 두 조건을 모두 만족하는 유일한 안.

---

## 1. 단위 기준 — Phase 3.1/3.2/3.3 통계

본 분할의 기준이 되는 *"비슷한 단위"* 를 정량화.

| Phase | 파일 수 | 라인 수 (실제) | legacy → 실제 압축률 | 소요 (계획) | sub-folder 수 |
|---|---|---|---|---|---|
| 3.1 (BZ) | 11 | **2,131** | (legacy ~2,685, 79%) *— 최종 코드는 1차 평가 후 보강됨* | 3~4 일 | 1 |
| 3.2 (Data) | 18 | **2,580** | 57.5% (legacy 4,485) | 4~5 일 | 2 (charge_density + slice) |
| 3.3 (Build) | 16 | **1,883** | 61.2% (legacy 3,075) | 3~4 일 | 2 (periodic_table + bravais) |
| **평균** | **15** | **~2,200** | **~60%** | **3.5 일** | **1.7** |

### Phase 3.4 (단일 진행 시 예상)

| 항목 | 값 | 평균 대비 |
|---|---|---|
| 파일 수 | 22 | 1.5× |
| 라인 수 | ~4,400 | 2.0× |
| sub-folder 수 | 3 | 1.8× |
| 추가 부담 | vtk_renderer 1,792 줄 분할 + EventBus 5 종 검증 + mouse_interactor 첫 구독자 + UI 3 윈도우 보존 | — |

→ **검토자 부담** 을 *Phase 평균* 으로 정렬하려면 **2~3 분할이 필요**.

---

## 2. 분할 대안 비교

### 2.1 Option A — 메뉴 항목 단위 3 sub-phase (권장)

| sub-phase | 범위 | 새 sub-folder | 파일 | legacy | 실제 예상 | 평균 대비 |
|---|---|---|---|---|---|---|
| **3.4a** | Cell | `features/edit/cell/` + `features/edit/edit_menu.{cpp,h}` (skeleton) | 10 (cell 8 + edit_menu 2) | 626 | **~700** | 0.32× |
| **3.4b** | Atoms | `features/edit/atoms/` + edit_menu 갱신 | 10 | 2,755 | **~1,900** | 0.86× |
| **3.4c** | Bonds | `features/edit/bonds/` + edit_menu 갱신 | 8 | 2,102 | **~1,400** | 0.64× |
| **합계** | | 3 sub-folder + edit_menu | **28** *(edit_menu 3 회 touch)* | **6,010** *(83% 의 ~5,000 줄로 압축)* | **~4,400** | 단일 진행 시 2.0× |

| 장점 | 단점 |
|---|---|
| 메뉴 항목과 1:1 — 검토자가 *"이 sub-phase 는 어디?"* 망설임 0 | 3.4a (Cell) 의 라인수가 작음 (~700) — Phase 평균의 32% |
| 각 sub-phase 가 *단독 런타임 검증 가능* — Cell 메뉴만, Atoms 메뉴만, Bonds 메뉴만 따로 시연 가능 | edit_menu 가 3 회 touch — diff 가 *progressive* (큰 영향 아님) |
| vtk_renderer 분할 부담 분산 (cell ~300 / atom ~700 / bond ~700) | 3.4b (Atoms) 가 Phase 3.3 (1,883) 보다 약간 무거움 — 단 평균 (2,200) 보다는 가벼움 |
| Phase 3.3 의 *atom 추가 시각화 검증* 이 3.4b 첫 시점에 가능 | 3.4c (Bonds) 가 마지막에 와서 사용자 가치 관점에서 *대기 시간* 발생 |

### 2.2 Option B — 도메인/렌더 분리 2 sub-phase

| sub-phase | 범위 | 파일 | legacy | 실제 예상 |
|---|---|---|---|---|
| **3.4a** | 모든 *_manager (atoms / bonds / cell / surrounding) — *데이터 변경자만* | 8 | 2,360 | ~1,650 |
| **3.4b** | 모든 *_renderer + 3 controller + 3 UI + edit_menu | 16 | 3,650 | ~2,750 |

| 장점 | 단점 |
|---|---|
| *_manager 가 한 sub-phase 에 모이면 EventBus emit 인프라 일관 검토 가능 | **3.4a 가 단독 런타임 검증 불가** — UI 도 renderer 도 없어 사용자가 "메뉴에서 보이는 차이가 0" 으로 *Phase 0/1/2 식 빈 dockspace* 회귀처럼 보임 |
| 3.4b 의 라인수가 평균 (2,200) 에 가까움 (2,750 의 약 25% 초과) | 3.4b 가 여전히 평균보다 무거움 + UI 3 윈도우 + vtk_renderer 분할 동시 진입 — *모든 기술 리스크 누적* |
| | Phase 0~3.3 의 *"각 PR 은 메뉴 단위로 검증"* 정책과 충돌 — 검증 정책 격하 |

→ **거부**. 단독 런타임 검증 불가가 결정적 결함.

### 2.3 Option C — vtk_renderer 분할 선행 sub-phase

| sub-phase | 범위 | 파일 | legacy | 실제 예상 |
|---|---|---|---|---|
| **3.4a** | vtk_renderer 분할 (atom_renderer / bond_renderer / cell_renderer) + 빈 stub controller / UI | 6 | 1,792 (vtk_renderer 분할만) | ~1,250 |
| **3.4b** | Atoms (atom_manager + surrounding_atom_manager + atoms_controller 풍부화 + atom_editor_ui + edit_menu Atoms 항목) | 7 | 2,055 | ~1,400 |
| **3.4c** | Bonds (bond_manager + bonds_controller 풍부화 + bond_ui + edit_menu Bonds 추가) | 6 | 1,326 | ~900 |
| **3.4d** | Cell (cell_manager + cell_controller 풍부화 + cell_info_ui + edit_menu Cell 추가) | 6 | 326 | ~350 |

| 장점 | 단점 |
|---|---|
| *Phase 3 의 가장 큰 기술 리스크 (vtk_renderer 1,792 줄 분할)* 가 격리되어 작은 PR 로 검증 가능 | **4 sub-phase 로 격증** — 통합 시각 추적 부담 ↑ |
| 각 도메인 sub-phase 가 작아짐 (Atoms 1,400, Bonds 900, Cell 350) | 3.4a 가 단독 런타임 검증 불가 — *액터 캐싱 코드만 있고 호출자 없음* (재현하면 *legacy 시점에서 갈라진 작은 dead code*) |
| | 3.4d (Cell, ~350 줄) 가 Phase 평균 (2,200) 의 16% — *너무 가벼움*. Phase 3.7 viewer (예상 ~600) 보다도 작음 |
| | edit_menu 4 회 touch — *progressive diff* 누적 |

→ **부분 거부**. 4 sub-phase 격증과 3.4a 단독 검증 불가가 결정적 결함. 단 *vtk_renderer 분할의 기술 리스크 격리* 라는 발상은 본 안의 §3.3 반영.

### 2.4 Option D — Option A + 우선순위 역전 (Atoms-first)

Option A 의 sub-phase 정의는 동일하되 진행 순서를 *사용자 가치 관점* 에서 **Atoms → Bonds → Cell** 로 변경.

| 장점 | 단점 |
|---|---|
| **Phase 3.3 의 Add atom 이 *첫 sub-phase 에서* 시각적으로 보임** — 사용자 가치 즉시 회수 | 3.4a (Atoms) 가 Phase 평균 (2,200) 의 86% — 첫 sub-phase 부터 무거움 |
| 가장 무거운 작업 (atoms_controller + mouse_interactor + element_database 호출 + atom_editor_ui) 가 첫 PR 로 와서 *후속 sub-phase 의 패턴* 이 즉시 정립 | atom_renderer 분할이 Cell/Bonds 의 vtk_renderer 분할 패턴 *templating* 역할이지만 *최초 진입* 이라 패턴 부재 — 검토 부담 ↑ |
| | atom_manager + bond_manager 의 의존 관계 (bond 는 atom 에 의존) 가 *Bonds 단계에서 만족* 되어 자연스럽지만, atoms 가 cell 에 *암묵적 의존* (image atom 생성 시 cell matrix 사용) 이 있어 cell 부재 상태에서 *fallback* 처리 필요 |

---

## 3. 권장안 — Option A + 의존성 순서 (Cell → Atoms → Bonds)

### 3.1 sub-phase 정의

| sub-phase | 메뉴 항목 | 새 sub-folder | 의존 (선행 sub-phase) | 신규 파일 | legacy | 새 트리 (예상) |
|---|---|---|---|---|---|---|
| **3.4a Cell** | `Edit / Cell` | `features/edit/cell/` + `edit_menu.{cpp,h}` (skeleton, Cell 항목만 wiring) | Phase 3.3 commit | **10** (cell 8 + edit_menu 2) | 626 (cell_manager 182 + cell_renderer ~300 + cell_info_ui 144) | **~700** |
| **3.4b Atoms** | `Edit / Atoms` (+ Cell 유지) | `features/edit/atoms/` + edit_menu 갱신 (Atoms 항목 추가) | 3.4a + Phase 3.3 onStructureAdded 보강 | **10** (atoms 10 + edit_menu touch) | 2,755 (atom_manager 704 + surrounding 409 + atom_renderer ~700 + atom_editor_ui 942) | **~1,900** |
| **3.4c Bonds** | `Edit / Bonds` (+ Cell, Atoms 유지) | `features/edit/bonds/` + edit_menu 갱신 (Bonds 항목 추가) | 3.4b | **8** (bonds 8 + edit_menu touch) | 2,102 (bond_manager 1,065 + bond_renderer ~700 + bond_ui 261 + bond_renderer.cpp 76) | **~1,400** |
| **합계** | 3 항목 | 3 sub-folder | | **28** *(edit_menu 3 회)* | **6,010** | **~4,400** (73%) |

### 3.2 권장 근거

1. **의존성 자연 순서**:
   - Cell 은 *atom 미의존* — cell_manager 가 lattice matrix 만 다룸. SceneState::structureRecords::cell 만 변경.
   - Atoms 는 *cell 가능 의존* — surrounding_atom_manager 의 image atom 생성이 cell matrix 필요. cell 부재 시 *fallback* (단위 행렬) 처리 가능하지만 cell 선행이 자연스러움.
   - Bonds 는 *atom 직접 의존* — bond_manager 의 자동 검출 알고리즘이 atom 좌표 + 원소 종 필요. atoms 선행 필수.
2. **단독 검증 가능성**:
   - 3.4a 만으로 *Edit / Cell 메뉴 + Cell Information 윈도우 + Bravais Apply 후 cell matrix 갱신* 검증 가능 — Phase 3.3 의 emit 자가 이미 작동 중.
   - 3.4b 만으로 *Edit / Atoms 메뉴 + Created Atoms 테이블 + 시나리오 S1/S2/S5/S6* 검증 가능.
   - 3.4c 만으로 *Edit / Bonds 메뉴 + 시나리오 S4* 검증 가능.
3. **vtk_renderer 분할 부담 분산** — 3.4a 에서 cell ~300 줄, 3.4b 에서 atom ~700 줄, 3.4c 에서 bond ~700 줄 분할. 한 PR 에 1,792 줄 통째 들어가는 것보다 검토 용이.
4. **각 sub-phase 의 라인수가 Phase 평균 (~2,200) 이내** — 3.4a 700 (작음), 3.4b 1,900 (Phase 3.3 와 비슷), 3.4c 1,400 (Phase 3.1 과 비슷).
5. **EventBus 5 종 검증의 자연 분산**:
   - 3.4a: `onCellChanged` 첫 구독자 + Phase 3.3 의 emit 자와 결합 + (선택) `onStructureAdded` Phase 3.3 보강과 *구독자 미도착 상태로* 통과
   - 3.4b: `onAtomsChanged` 첫 구독자 + `onStructureAdded` 첫 구독자 + `mouse_interactor` 첫 구독자 + Phase 3.3 의 emit 자와 결합
   - 3.4c: `onBondsChanged` 첫 emit + 첫 구독자 (단일 sub-phase 안 양방) + `element_database` 두 번째 사용자
6. **§1.4 UI 보존 검증의 자연 분산**:
   - 3.4a: Cell Information 윈도우 1 종 + 시나리오 S3 (Bravais Apply → cell + atoms — *atom 측은 미시각화* 라 *cell + lattice matrix 갱신* 까지만 검증)
   - 3.4b: Created Atoms 윈도우 + 시나리오 S1/S2/S3-atoms/S5/S6 (atom 측 시각화 도착 — *Phase 3.3 의 모든 atom 추가가 비로소 화면에 보이는 시점*)
   - 3.4c: Bonds Management 윈도우 + 시나리오 S4

### 3.3 Option C 의 *vtk_renderer 분할 격리* 발상 반영

Option C 의 *vtk_renderer 분할 PR 격리* 가 매력적이었으나 단독 검증 불가가 결함이었다. 본 권장안은 다음 절충:

- 3.4a Cell — vtk_renderer 의 cell 부분만 (~300 줄) 분할 + cell_renderer 가 *즉시 Bravais 의 cell matrix 적용* 으로 검증 가능
- 3.4b Atoms — vtk_renderer 의 atom 부분 (~700 줄) 분할 + atom_renderer 가 *Periodic Table 의 Add atom* + *Bravais 의 atom 4 개 추가* 즉시 검증
- 3.4c Bonds — vtk_renderer 의 bond 부분 (~700 줄) 분할 + bond_renderer 가 *Recompute* 즉시 검증

→ 각 sub-phase 가 분할한 분량을 *그 안에서 즉시 사용자 검증* 한다. Option C 의 *분할 격리* 의도를 *각 sub-phase 안에서 격리* 형태로 유지.

### 3.4 일정 예상

| sub-phase | 일정 | 작업량 (Phase 3 평균 비율) |
|---|---|---|
| 3.4a Cell | 1.5~2 일 | 0.4× |
| 3.4b Atoms | 3~4 일 | 1.0× (Phase 평균 수준) |
| 3.4c Bonds | 2~2.5 일 | 0.7× |
| **합계** | **6.5~8.5 일** | 본 단계 단일 5~7 일 예상보다 *1~1.5 일 길어짐* (분할 오버헤드 = +20~30%) |

→ 검토자 부담은 ~3× 감소, 작성자 부담은 ~1.2~1.3× 증가. **순 효과: 검토 통과 시간 단축 + 회귀 추적 용이**.

---

## 4. 분할 시 PR 의존성 그래프

```
Phase 3.3 commit
  └→ (선택) Phase 3.3 의 onStructureAdded emit 보강 — 별도 hot-fix 또는 3.4b 와 동시
       │
       ├→ PR 3.4a Cell  (최소 의존, 가장 가벼움)
       │     │
       │     └→ PR 3.4b Atoms  (cell 의존, 가장 무거움)
       │            │
       │            └→ PR 3.4c Bonds  (atom 의존, 중간 무게)
       │
       └→ Phase 3.5 measurement (3.4 머지 후 진입)
```

**병렬화 가능 여부**: Cell 과 Atoms/Bonds 는 *독립적 작성 가능* (서로 다른 sub-folder). 단 머지 순서는 위 그래프대로 권장 — 3.4a 부터 base 잡기.

---

## 5. 각 sub-phase 의 검증 매트릭스 항목 분배

원래 Phase 3.4 §5 의 32 항목을 다음과 같이 분배.

### 5.1 Phase 3.4a (Cell) — 14 항목

| 영역 | 항목 |
|---|---|
| 정적 (5) | features/edit/cell 폴더, cell 파일 수, edit_menu skeleton, namespace 일관성, legacy 호출 0 |
| 인프라 (3) | onCellChanged 구독 1+, vtk_renderer cell 분할 vtk* 호출 보존, cell_manager 의 lattice matrix 변환 보존 |
| 빌드 (2) | debug + release |
| UI 보존 (3) | §1.4.3 Cell Information UI, ImGui 위젯 grep diff (cell_info_ui), Side-by-side 1 종 |
| 시나리오 (1) | S3 (Bravais Apply → cell matrix + lattice 표시 갱신) — *atom 측 시각화는 3.4b 후에야 가능, 본 단계는 cell + matrix 까지만 검증* |

### 5.2 Phase 3.4b (Atoms) — 14 항목

| 영역 | 항목 |
|---|---|
| 정적 (4) | features/edit/atoms 폴더, atoms 파일 수, edit_menu Atoms 항목 추가, legacy 호출 0 |
| 인프라 (5) | onAtomsChanged 구독 1+, onStructureAdded 구독 1+, mouse_interactor 첫 구독자, vtk_renderer atom 분할 vtk* 호출 보존, surrounding_atom_manager PBC 알고리즘 보존 |
| 빌드 (1) | release (debug 는 3.4a 에서 통과) |
| UI 보존 (2) | §1.4.1 Created Atoms UI, Side-by-side 1 종 |
| 시나리오 (2) | **S1 (Add atom 시각화 — 본 단계 핵심 보상)**, S2 (셀 편집), S5/S6 (mouse_interactor) |

### 5.3 Phase 3.4c (Bonds) — 7 항목

| 영역 | 항목 |
|---|---|
| 정적 (3) | features/edit/bonds 폴더, bonds 파일 수, edit_menu Bonds 항목 추가 |
| 인프라 (3) | onBondsChanged emit + 구독 양방, element_database 두 번째 사용자 (covalentRadius), vtk_renderer bond 분할 vtk* 호출 보존 |
| UI 보존 (1) | §1.4.2 Bonds Management UI + 시나리오 S4 |

→ 32 → 14 + 14 + 7 = **35 항목** (3.4a 의 빌드 2 종을 3.4b 가 release 만 추가 검증하므로 합계는 +3). *추가된 3 항목* 은 *각 sub-phase 의 자기완결 검증* 을 위한 보강.

---

## 6. 분할 정책에 따른 리스크 변화

| Phase 3.4 §6 의 리스크 | 단일 진행 | 3 분할 | 변화 |
|---|---|---|---|
| 6.1 vtk_renderer 1,792 줄 분할 누락 | 한 PR 에 통째 — 검토 부담 ~3× | 3 분할 (cell ~300 / atom ~700 / bond ~700) | **↓ 결정적 완화** |
| 6.2 bond_manager 900 줄 알고리즘 압축 누락 | 단독 검토 부담 | 3.4c 에 격리 — 그 단계만 집중 | ↓ 완화 |
| 6.3 atom_editor_ui 896 줄 압축 중 §1.4.1 위반 | 단독 검토 부담 | 3.4b 에 격리 — 그 단계만 집중 | ↓ 완화 |
| 6.4 mouse_interactor 시그니처 미스매치 | 한 PR 에 진입 | 3.4b 에서만 검증 | ↓ 완화 |
| 6.5 EventBus 무한 루프 (controller emit + 자기 구독) | 5 종 동시 도입 | 3 분할 — 5 종이 sub-phase 별 분산 | ↓ 완화 |
| 6.6 onStructureAdded 보강 시 BravaisController 회귀 | 보강 시점 명확 | 3.4b 에서 보강 — 구독자 도착 시점과 일치 | ↑ 약간 (별도 commit 추적) |
| 6.7 Bravais Apply 시 atom 4 개 → 4 reflow | 단독 분석 부담 | 3.4b 에서만 검증 | ↓ 완화 |
| 6.8 element_database 두 번째 사용자 covalentRadius 부재 | 한 PR 에서 발견 시 회귀 부담 큼 | 3.4c 에서만 검증 — Phase 2 보강 PR 따로 | ↓ 완화 |
| 6.9 단일 PR 거대 diff 머지 지연 | 4,400 줄 단일 | 700 + 1,900 + 1,400 분산 | **↓ 결정적 완화** |
| 6.10 controller 두께 변동성 | 3 controller 동시 | sub-phase 별 controller 1 개씩 | ↓ 완화 |
| 6.11 §1.4 UI 보존 위반 (3 윈도우) | 동시 검증 부담 ~3× | sub-phase 별 1 윈도우 | **↓ 결정적 완화** |
| 6.12 클래스 vs free function 패턴 차이 | 동시 — 첫 패턴이 정착되지 않음 | 3.4a 에서 cell_manager 패턴 정착 → 3.4b/3.4c 가 *템플릿화 적용* | ↓ 완화 |
| 6.13 core/data 시그니처 lowerCamelCase | 한 PR 에서 발견 | 3.4c 에서 검증 (bond_manager) | ↓ 완화 |

→ 13 리스크 중 **결정적 완화 3 종 + 일반 완화 9 종 + 약간 격상 1 종**. 순 효과: **분할이 위험 감소 측면에서 강하게 정당화됨**.

---

## 7. 분할 시 추가 작업

각 sub-phase 의 PR 헤더에 *분할 컨텍스트* 를 명시.

```
Phase 3.4a Cell (1/3)  —  features/edit/cell migration

This PR is the first of three in Phase 3.4. It introduces the cell sub-folder
and the edit_menu skeleton. Atoms (3.4b) and Bonds (3.4c) follow.

- New folder: features/edit/cell/ (8 files) + edit_menu.{cpp,h} skeleton (2 files).
- Menu bar: Edit (skeleton) / Cell.
- core/scene::EventBus::onCellChanged first subscriber — pairs with Phase 3.3 emit.
- vtk_renderer.cpp 1,792-line split (1/3) — cell portion ported to cell_renderer.
- legacy Cell Information UI preserved 1:1 per §6.0.1.

Side-by-side screenshot (Cell Information) and S3 (cell+matrix only) test attached.
Intentional UI deviation: None.

Reference:
  - phase3_4_split_proposal.md (this proposal)
  - phase3_4_edit_atoms_bonds_cell.md (parent plan)
```

3.4b / 3.4c 의 PR 헤더는 위와 같은 형식으로 *(2/3) Atoms*, *(3/3) Bonds* 표기 + 의존 PR 머지 commit hash 명시.

---

## 8. 권장 채택 결정

> **권장: Option A 채택 + Cell → Atoms → Bonds 의 의존성 순서**.
>
> 부수 권장:
> 1. 각 sub-phase 의 세부계획서를 별도 작성하지 *않고*, 본 분할 제안서 + 기존 [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md) 를 *참조 문서* 로 사용. PR 본문에 두 문서 링크 + sub-phase 번호만 명시.
> 2. Cell 의 단일 sub-phase 라인수가 작은 (~700) 점이 *과잉 분할* 우려가 있다면, **3.4a 와 3.4b 를 단일 PR 로 합치는 옵션** (Cell + Atoms = ~2,600, Phase 3.2 와 비슷) 도 고려 가능. 단 분할의 §6 리스크 완화 효과는 절반으로 감소.
> 3. 사용자 가치 우선 (Phase 3.3 의 atom 추가 시각화 즉시 보상) 이 의존성 순서보다 중요하다면 **Option D (Atoms → Bonds → Cell)** 채택 — 본 분할안의 sub-phase 정의는 동일하되 진행 순서만 역전.

### 사용자 결정 요청 항목

다음 3 가지 중 1 개 선택 필요.

| 선택 | 의미 |
|---|---|
| (A) **3 분할 + Cell→Atoms→Bonds** *(권장)* | 의존성 자연 순서. 첫 PR 가 가벼워서 분할 패턴 정착 |
| (B) 3 분할 + Atoms→Bonds→Cell *(가치 우선)* | 사용자 가치 (atom 시각화) 즉시 보상. 첫 PR 부터 무거움 |
| (C) 2 분할 — *Cell+Atoms 합본* (~2,600) → Bonds (~1,400) | 분할 오버헤드 최소화. 단 첫 PR 가 Phase 3.2 와 비슷한 수준 |

---

## 9. 관련 문서

- 본 분할 제안의 모체: [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)
- 선행 평가서: [`./phase3_3_evaluation_2026-05-07_v2.md`](./phase3_3_evaluation_2026-05-07_v2.md)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6.0 (UI 보존) + §11 (vtk_renderer 분할 리스크)
- 이전 sub-phase 평가서: [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md), [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- UI 이식 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
