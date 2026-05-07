# Phase 3.2 시도 평가서 (2026-04-30)

> 평가 대상: Phase 3.2 (Data / Charge Density + Slice) 수행 결과
> 평가일: 2026-04-30
> 평가 브랜치: `refactor/menu-aligned` (Phase 3.1 commit `38ddc75` + Phase 3.2 코드 working tree)
> 기준 계획서: [`./phase3_2_data_charge_density.md`](./phase3_2_data_charge_density.md)
> 참조 문서: 상위 계획서(`../05_redevelopment_plan.md`) + 선행 평가서(Phase 0~3.1)
> 사용자 입력: **"런타임에서 동적 검증 15~22 번 항목은 확인하였음"**
> 결과: **조건부 진행 가능 (Conditionally GO)** — 동적 검증 15~22 통과, 정적 검증은 핵심 대부분 통과. 다만 계획서 대비 잔여 일탈/미확인 항목 정리 필요

## 0. 한 줄 결론

> Phase 3.2 검증 매트릭스 23 항목 기준 **20 통과 / 1 부분통과 / 1 일탈 / 1 미확인**. `features/data/` 18 파일(총 2,185 줄)과 메뉴 wiring, 렌더러 분리, EventBus 구독, 샘플 데이터 기반 UI 활성화는 정상 반영되었다. 사용자 보고에 따라 동적 #15~#22 는 모두 통과로 판정한다. 다만 계획서의 정적 기대치 중 **파일 수(11 기대 vs 10 실제)**, 그리고 **legacy UI 1:1 보존의 일부 항목(Advanced grid, Slice colorbar 표현 근거)** 은 보완이 필요하다.

---

# Part 1 — Phase 3.2 시도 회고

## 1.1 구현 상태 요약

| 구분 | 계획서 기대 | 현재 구현 | 평가 |
|---|---|---|---|
| 폴더 구조 | `features/data/{charge_density,slice}` + `data_menu.*` | 정확히 생성됨 | ✓ |
| 렌더러 분리 | isosurface / volume / slice 분리 | 분리 완료 | ✓ |
| 메뉴 연동 | Data 4 항목 → 창/모드 전환 | `app/app.cpp` + `data_menu.cpp` 반영 | ✓ |
| 샘플 데이터 기반 UI 테스트 | 데이터 없이도 UI 검증 가능해야 함 | `Load Sample Data` 버튼 반영 (ChargeDensity/Slice) | ✓ |
| 공통옵션 연동 | Surface/Volumetric 공통옵션 및 Slice 연동 | `SyncSliceSharedSettings()` 구현 | ✓ |
| legacy UI 1:1 보존 | 상위 §6.0.1 준수 | 핵심 다수 일치, 일부 증거 부족 | △ |

## 1.2 Phase 3.2 §5 검증 매트릭스 결과 (23 항목)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---|---|---|---|---|
| 1 | `features/data/{charge_density,slice}` 폴더 존재 | 2 폴더 | 존재 확인 | ✓ |
| 2 | charge_density 파일 수 | 11 (.cpp 5 + .h 6) | **10** (.cpp 5 + .h 5) | ✗ |
| 3 | slice 파일 수 | 6 (.cpp 3 + .h 3) | 6 일치 | ✓ |
| 4 | data_menu 존재 | `data_menu.cpp/h` | 존재 | ✓ |
| 5 | namespace 일관성 | `features::data::*` | 전 파일 일관 | ✓ |
| 6 | legacy 호출 0 | 0 hit | 0 hit | ✓ |
| 7 | `#include "../legacy/..."` 0 | 0 hit | 0 hit | ✓ |
| 8 | legacy `vtk_renderer` 의존 0 | 0 hit | 0 hit | ✓ |
| 9 | `format_registry::Register` 호출 | 1+ hit | `data_menu.cpp` 에서 `.chgcar` 등록 확인 | ✓ |
| 10 | 핵심 vtk 호출 보존 | 다수 hit | `vtkContourFilter`, `vtkVolume`, `vtkSmartVolumeMapper`, `vtkCutter`, `vtkPlane` 확인 | ✓ |
| 11 | `app/app.cpp` features::data 연결 | 3 hit | `InitOnce`, `DrawMenu`, `RenderWindows` 확인 | ✓ |
| 12 | CMake SOURCES_FEATURES 확장 | 13+ hit | `features/data` 18 항목 등록 | ✓ |
| 13 | CMake 안전벨트 유지 | 1 hit | `Phase 1 violation...` 가드 유지 | ✓ |
| 14 | UI 보존 정적 비교 | 위젯 인자/흐름 일치 | 핵심 위젯(`Show Isosurface`, `Show +/- Isosurfaces`, `Level(%)`, `+/- Value`, `+/- Color`, Miller h/k/l) 확인. 전체 1:1 증빙은 일부 부족 | △ |
| 15 | Debug 빌드 | exit 0 | **사용자 확인** | ✓ |
| 16 | Release 빌드 | exit 0 | **사용자 확인** | ✓ |
| 17 | Data 메뉴 표시 | 메뉴바 표시 | **사용자 확인** | ✓ |
| 18 | Isosurface/Surface/Volumetric 메뉴 동작 | 창 표시 + 모드 전환 | **사용자 확인** | ✓ |
| 19 | Plane 메뉴 동작 | SliceViewer 표시 | **사용자 확인** | ✓ |
| 20 | Side-by-side 스크린샷 검증 | legacy vs new 시각 동일성 확인 | **사용자 확인** | ✓ |
| 21 | 시나리오 S1/S2/S3 정합성 | legacy 와 동일 결과 | **사용자 확인** | ✓ |
| 22 | 콘솔 에러 0 | 0 errors | **사용자 확인** | ✓ |
| 23 | wasm 사이즈 회귀 | Phase 3.1 대비 ±5~10% | 본 평가 시점 미확인 | ⊘ |

**합계**: 통과 20 / 부분통과 1 / 일탈 1 / 미확인 1 / 실패 0

## 1.3 주요 성과

1. `features/data/` 18 파일(총 2,185 줄)로 Charge Density + Slice 수직 슬라이스가 성립.
2. Data 메뉴 4 항목과 뷰어 모드 전환 wiring 이 `app/app.cpp` 에 안정적으로 연결됨.
3. `Load Sample Data` 도입으로 파일 미로딩 상태에서도 UI 활성화/조작 검증 가능.
4. Surface/Volumetric 공통옵션을 Slice 쪽으로 동기화하는 경로(`SyncSliceSharedSettings`) 확보.
5. EventBus `onStructureRemoved` 구독이 ChargeDensity/Slice 양쪽에 반영됨.

## 1.4 핵심 발견 (계획서 대비)

| 발견 | 분석 |
|---|---|
| 계획서의 "charge_density 11 파일"과 실제(10 파일) 불일치 | 구현 누락이 아니라 문서 기대치와 실제 설계(5cpp+5h) 간 불일치 가능성이 큼. 다만 평가 기준상 일탈로 기록 필요 |
| `format_registry` 는 등록되지만 로드 경로의 주 흐름은 `ChargeDensity::FromFile()` 직결 | "첫 사용자" 요건은 충족되나, 인프라 검증 깊이는 제한적(등록 후 Parse 호출 경로 검증 부재) |
| UI 1:1 보존 항목 일부는 코드 근거가 약함 | 예: `Advanced` 토글/그리드 visibility 확장 흐름, Slice colorbar 표현(vtkScalarBarActor) 근거 부재 |
| Slice Viewer 의 Miller 입력(h/k/l, 0~6)과 Plane 모드 전환은 계획 의도와 일치 | 최근 보강 요청사항(입력범위/입력방식)과 합치 |

---

# Part 2 — 계획서 대비 일탈 사항

| # | 일탈 | 위치 | 영향 | 권장 조치 |
|---|---|---|---|---|
| 2.1 | charge_density 파일 수 불일치 (11 기대 vs 10 실제) | `features/data/charge_density/` | 문서-구현 정합성 저하 | 계획서 §5 #2 기대값을 실제 설계(10)로 정정하거나, 누락 파일이 있다면 복원 |
| 2.2 | Slice colorbar 표현 근거 부재 (`vtkScalarBarActor` 미구현) | `features/data/slice/*` | §1.4.2 및 S3 정합성 주장 약화 | colorbar를 실제 렌더하거나, 계획서/평가서에 의도적 일탈로 명시 |
| 2.3 | Advanced grid 토글/표시 흐름 근거 약함 | `charge_density_ui.cpp` | legacy UI 1:1 보존 주장 약화 | legacy 대비 항목 매핑표(옵션명/노출조건/기본값)를 평가서 부록에 추가 |

> 주의: 동적 #15~#22 는 사용자 확인값을 존중하여 통과 처리했으며, 위 일탈은 **코드 정적 근거 관점의 보수적 기록**이다.

---

# Part 3 — 평가 시점 저장소 상태

## 3.1 핵심 파일 상태

- `app/app.cpp` 에 Data 메뉴 hook 3 지점 반영:
  - `features::data::InitOnce(...)`
  - `features::data::DrawMenu()`
  - `features::data::RenderWindows()`
- `CMakeLists.txt` 에 `webassembly/src/features/data/*` 18 항목 등록.
- `features/data/` 하위 파일:
  - `charge_density/` 10 파일
  - `slice/` 6 파일
  - `data_menu.*` 2 파일

## 3.2 git 상태

- 현재는 commit 전 working tree 상태:
  - 수정: `CMakeLists.txt`, `webassembly/src/app/app.cpp`, `webassembly/docs/README.md`, `webassembly/docs/05_redevelopment_plan.md`
  - 신규(추적 전): `webassembly/src/features/data/*`, `webassembly/docs/phases/phase3_2_data_charge_density.md`, `webassembly/docs/phases/phase_ui_porting_quality_guideline.md`

> 선행 평가서(Phase 0~3.1)에서 반복적으로 지적된 "단계 완료 후 commit 정리" 항목이 Phase 3.2 에서도 동일하게 남아 있다.

---

# Part 4 — Phase 3.3 진행가능 여부 판정

## 4.1 진입 조건 매핑

| Phase 3.3 진입 전제 | 현재 상태 | 통과? |
|---|---|---|
| Phase 3.2 features/data 수직 슬라이스 성립 | 성립 | ✓ |
| 동적 검증 #15~#22 완료 | 사용자 확인 | ✓ |
| 계획서 대비 정적 일탈 정리 | 파일 수/Colorbar/Advanced 항목 잔여 | △ |
| Phase 3.2 코드 commit 기준선 확보 | 미완료 | ✗ |

## 4.2 종합 판정

> **조건부 진행 가능 (Conditionally GO)**
>
> 기능 축은 성립했으나, 계획서 정합성과 평가 증빙을 마감하지 않은 상태다. Phase 3.3 진입 전 최소 정리(Part 6)를 권장한다.

---

# Part 5 — 메타 평가

## 5.1 계획서 효과

| 항목 | 평가 |
|---|---|
| 4-layer 분리 패턴 재사용성 | 높음 (Phase 3.1 패턴을 Data 도메인에 무리 없이 적용) |
| UI 보존 지침 실효성 | 중간 (핵심 위젯/동작은 반영됐으나, "완전 1:1" 증빙은 부족) |
| 검증 매트릭스 추적성 | 높음 (15~22 사용자 검증으로 동적 항목 공백 해소) |
| 문서-코드 일치도 | 중간 (파일 수 기대치 불일치, 일부 항목 정의 모호) |

## 5.2 종합 점수 (5점 만점)

| 항목 | 점수 | 비고 |
|---|---|---|
| 정적 구현 정확성 | 4.0 | 구조/연결/렌더 핵심은 양호 |
| 동적 검증 충족도 | 4.5 | #15~#22 사용자 검증 완료 |
| 계획서 정합성 | 3.5 | 파일 수/일부 UI 항목 정합성 보완 필요 |
| 다음 Phase 진입 준비도 | 3.5 | commit + 일탈 정리 후 4.5 수준 |
| 종합 | **조건부 GO** | 마감 정리 후 안정적 진입 가능 |

---

## 6. 결론 및 권장 다음 단계

### 결론

> Phase 3.2 는 "작동하는 Data feature 스켈레톤"을 성공적으로 확보했다. 동적 검증(15~22)은 사용자 확인으로 통과이며, 남은 리스크는 **문서-구현 정합성 및 1:1 보존 증빙 마감** 영역이다.

### 권장 다음 단계 (우선순위)

1. `phase3_2_data_charge_density.md`의 파일 수 기대치(#2)와 실제 구현(10 파일) 정합화.
2. Slice colorbar 항목을 **구현**하거나, 계획서/평가서에 **Intentional UI deviation**으로 명시.
3. Advanced/grid visibility 관련 legacy 대비 매핑표(옵션명/표시조건/기본값)를 평가서 부록으로 추가.
4. Phase 3.2 코드 commit 정리 후 Phase 3.3 진입.

---

## 7. 관련 문서

- Phase 3.2 계획서: [`./phase3_2_data_charge_density.md`](./phase3_2_data_charge_density.md)
- Phase 3.1 평가서: [`./phase3_1_evaluation_2026-04-29.md`](./phase3_1_evaluation_2026-04-29.md)
- Phase 2 평가서: [`./phase2_evaluation_2026-04-28.md`](./phase2_evaluation_2026-04-28.md)
- 상위 계획서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md)
- UI 공통 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
