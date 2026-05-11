# Phase 3.4 결과보고서 (2026-05-11)

> 평가 대상:  
> - [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)  
> - [`./phase3_4_1_edit_cell.md`](./phase3_4_1_edit_cell.md)  
> - [`./phase3_4_2_edit_atoms.md`](./phase3_4_2_edit_atoms.md)  
> - [`./phase3_4_3_edit_bonds.md`](./phase3_4_3_edit_bonds.md)
>
> 평가일: 2026-05-11  
> 평가 브랜치: `refactor/menu-aligned` (`HEAD=a20abc6`, Phase 3.4는 working tree 상태)

## 0. 한 줄 요약

Phase 3.4의 구조 이식(`features/edit/{cell,atoms,bonds}` + `edit_menu`)과 core event 결선은 완료되었고 debug/release 빌드가 모두 통과했다.  
다만, 계획서의 "legacy UI 1:1 보존" 항목 일부는 **후속 사용자 요구사항 반영 과정에서 의도적으로 변경**되어, 결과는 **조건부 완료 (GO with intentional deviations)** 로 평가한다.

---

# Part 1 — 범위 및 기준

## 1.1 평가 기준

1. Phase 3.4 통합 계획서의 분할 구조/의존성
2. Phase 3.4.1/3.4.2/3.4.3의 목표/비목표
3. 각 문서의 §5 검증 매트릭스(각 14항목)
4. 현재 코드, 빌드 로그, 사용자 런타임 피드백 기반 추가 보정 이력

## 1.2 평가 결과 등급

| 구분 | 평가 |
|---|---|
| 구조 이식 완료도 | **완료** |
| 이벤트 버스 결선 | **완료** |
| 빌드 안정성(debug/release) | **완료** |
| 계획서 strict UI 1:1 보존 | **부분** (의도적 변경 포함) |
| 종합 | **조건부 완료 (GO)** |

---

# Part 2 — 구현 산출물 점검

## 2.1 파일/라인 산출물

| 영역 | 계획 파일 수 | 실제 파일 수 | 실제 라인 수(현 시점) | 상태 |
|---|---:|---:|---:|---|
| `features/edit/cell/` | 8 | 8 | 768 | 완료 |
| `features/edit/atoms/` | 10 | 10 | 1,713 | 완료 |
| `features/edit/bonds/` | 8 | 8 | 1,122 | 완료 |
| `features/edit/edit_menu.{cpp,h}` | 2 | 2 | 120 | 완료 |
| 합계 | 28 | 28 | 3,723 | 완료 |

## 2.2 빌드/연동 증빙

| 항목 | 결과 |
|---|---|
| `app/app.cpp`에서 `features::edit::InitOnce/DrawMenu/RenderWindows` 연결 | 확인 |
| `CMakeLists.txt`의 `SOURCES_FEATURES`에 `features/edit/**` 등록 | 확인 |
| `npm run build-wasm:release` | 통과 (2026-05-11) |
| `npm run build-wasm:debug` | 통과 (2026-05-11) |

---

# Part 3 — Sub-phase별 평가

## 3.1 Phase 3.4.1 (Edit/Cell)

| 계획 핵심 | 평가 | 근거 |
|---|---|---|
| `features/edit/cell/` 8파일 이식 | PASS | 폴더/파일 구성 일치 |
| `edit_menu` Cell 항목 wiring | PASS | Edit 메뉴 항목 동작 코드 존재 |
| `onCellChanged` 첫 구독자 결선 | PASS | `cell_renderer.Subscribe()`에서 구독 |
| `vtk_renderer` cell 분할 이식 | PASS | `cell_renderer` 독립 모듈 존재/렌더 갱신 동작 |
| debug/release 빌드 통과 | PASS | 2026-05-11 빌드 로그 통과 |
| Cell UI strict 1:1 보존 | PARTIAL | matrix 중심 편집은 동작하나 계획서의 파라미터/부가표시 항목 일부 축소 |

평가: 구조/이벤트/빌드 측면은 완료. UI strict 보존 관점은 부분 달성.

## 3.2 Phase 3.4.2 (Edit/Atoms)

| 계획 핵심 | 평가 | 근거 |
|---|---|---|
| `features/edit/atoms/` 10파일 이식 | PASS | 폴더/파일 구성 일치 |
| `edit_menu` Atoms 항목 + `InitOnce(scene, MouseInteractor&)` | PASS | 시그니처/호출 연결 확인 |
| `onAtomsChanged`, `onStructureAdded` 첫 구독자 | PASS | `atoms_controller` 구독 코드 존재 |
| `MouseInteractor` 첫 구독자 | PASS | `atoms_controller.Subscribe(mouseInteractor)` |
| Phase 3.3 `onStructureAdded.Emit` 보강 | PASS | `bravais_controller.Apply`에 emit 존재 |
| debug/release 빌드 통과 | PASS | 2026-05-11 빌드 통과 |
| Created Atoms strict 1:1 보존 | PARTIAL | 이후 사용자 요구로 컬럼/컨트롤 구조 조정(의도적 스펙 변경) |

추가 반영(사용자 요청 기반):
- Boundary atoms 생성 조건 legacy 정합 패치
- 편집 좌표/반지름 실시간 갱신 반영 패치

평가: 기술 이식과 이벤트 결선은 완료. UI는 최신 사용자 승인 스펙 기준으로 안정화됨.

## 3.3 Phase 3.4.3 (Edit/Bonds)

| 계획 핵심 | 평가 | 근거 |
|---|---|---|
| `features/edit/bonds/` 8파일 이식 | PASS | 폴더/파일 구성 일치 |
| `edit_menu` Bonds 항목 추가 | PASS | Edit 메뉴 3항목(Cell/Atoms/Bonds) 구성 확인 |
| `onBondsChanged` emit+subscriber 양방 결선 | PASS | `bond_manager` emit + `bond_renderer` subscribe |
| `core::data::ElementDatabase` 사용 | PASS | `bond_manager`에서 covalent radius 기반 threshold 계산 |
| debug/release 빌드 통과 | PASS | 2026-05-11 빌드 통과 |
| Bonds Management strict 1:1 보존 | PARTIAL | 사용자 요청으로 글로벌 factor/%범위/링크동작/타입표시 규칙 확장 |

추가 반영(사용자 요청 기반):
- slider reset 복구
- global/local factor 링크/해제 동작 추가
- `Bond Types` 표시 규칙: "가능한 원자쌍 표시 + threshold로 결합 0이어도 타입 유지"

평가: 구조/이벤트/재계산 루프는 완료. UI/정책은 사용자 승인 스펙으로 확장됨.

---

# Part 4 — 검증 매트릭스 집계(현재 시점)

각 sub-phase 계획서는 14항목(총 42항목) 매트릭스를 정의한다.  
현재 증빙 기준 집계:

| 구분 | 항목 수 |
|---|---:|
| PASS | 34 |
| PARTIAL | 8 |
| FAIL | 0 |

PARTIAL로 남긴 항목은 대부분 "legacy strict 1:1" 항목이며, 이유는 구현 누락이 아니라 **후속 사용자 요구사항에 따른 의도적 변경**이다.

---

# Part 5 — 계획 대비 일탈/변경 기록

## 5.1 Intentional Deviations (사용자 승인 변경)

| 영역 | 계획서 기본 방향 | 현재 반영 상태 |
|---|---|---|
| Created Atoms UI | 계획서 컬럼/입력 규약 | 사용자 요구에 맞춰 컬럼/컨트롤 재배치 및 일부 컨트롤 제거 |
| Bond distance 파라미터 | legacy 중심 보존 | `%` 단위, `-50%~50%`, 글로벌-타입 링크/해제 동작 반영 |
| Bond Types 노출 규칙 | 자동 검출 결합종 중심 | 가능한 원자쌍 기준 + 결합 0이어도 타입 유지 |
| Boundary atoms 조건 | 초기 이식 로직 | legacy 조건으로 재정렬(`hasCell && visible`, fractional 재계산) |

## 5.2 비의도 일탈

현재 기준 치명적 비의도 일탈(FAIL)은 확인되지 않았다.

---

# Part 6 — 저장소 상태 및 릴리즈 리스크

## 6.1 저장소 상태

- Phase 3.4 관련 코드/문서는 현재 **working tree 중심** 상태이며, 커밋 정리는 별도 필요
- `git status` 상 `features/edit/**` 및 Phase 3.4 문서가 신규/수정 상태로 존재

## 6.2 잔여 리스크

1. 최신 사용자 스펙 반영 후, 계획서 원문의 strict UI 1:1 항목과 문서 정합성 차이 존재
2. 시나리오 S1~S6/S4의 side-by-side 스크린샷 증빙 문서는 별도 첨부 필요
3. UI 회귀를 자동으로 잡는 테스트(Playwright/스크립트)가 아직 없음

---

# Part 7 — 결론

Phase 3.4.1/3.4.2/3.4.3의 코드 재개발은 **구조 이식·이벤트 결선·빌드 안정성 기준으로 완료**되었으며, 현재 코드는 Phase 3.5로 진행 가능한 상태다.  
다만, 계획서의 "legacy strict 1:1" 일부는 사용자 승인 변경으로 대체되었으므로, 본 보고서는 **조건부 완료(GO with intentional deviations)** 로 확정한다.

---

## 부록 A — 이번 보고서 작성 시 수행한 핵심 검증

1. `npm run build-wasm:release` 통과
2. `npm run build-wasm:debug` 통과
3. `features/edit/**` 파일/라인 집계
4. EventBus emit/subscribe 정적 grep
5. app/CMake wiring 정적 grep
