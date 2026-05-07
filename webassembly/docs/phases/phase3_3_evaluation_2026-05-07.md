# Phase 3.3 세부계획서 평가서 (2026-05-07)

> 평가 대상: [`./phase3_3_build_periodic_bravais.md`](./phase3_3_build_periodic_bravais.md)  
> 평가 일시: 2026-05-07  
> 평가 기준 브랜치: `refactor/menu-aligned`  
> 기준 커밋: `bb4ca37` (Phase 3.2 완료 커밋) + 현재 working tree  
> 사용자 입력: "런타임에서 동적 검증 15부터 23번 항목은 확인하였음."  
> 결과: **조건부 진행 가능 (GO-Conditional)** — 핵심 구현/동적 항목(#15~#24) 통과, UI 편차 사유화/커밋 정리만 잔여

## 0. 한 줄 결론

> Phase 3.3 계획서의 핵심 목표(신규 `features/build/` 16 파일, `app/app.cpp` Build 메뉴 hook, `CMakeLists.txt` 소스 확장, `ElementDatabase` 외부 사용 시작, 런타임 동작)는 현재 코드 기준으로 충족되었고, 사용자 확인으로 동적 검증 #15~#23도 통과했다. 추가로 #24(와즘 사이즈 회귀)까지 수치 검증 결과 **Phase 3.2 baseline 대비 wasm +0.373%**로 정상 범위(±5~10%)를 만족한다.

---

# Part 1 — 구현 결과 요약

## 1.1 계획서 Step 대비 구현 상태

| Step | 계획서 요구 | 현재 상태 | 판정 |
|---|---|---|---|
| 1 | `features/build/{periodic_table, bravais}/` 신설 | 생성 완료 | ✓ |
| 2 | `crystal_structure`, `crystal_system` 이식 | 4 파일 생성 완료 | ✓ |
| 3 | `periodic_table.{cpp,h}` (선택) | 생성 완료 | ✓ |
| 4 | `bravais_controller.{cpp,h}` | 생성 완료 | ✓ |
| 5 | `periodic_table_controller.{cpp,h}` | 생성 완료 | ✓ |
| 6 | `periodic_table_ui`, `bravais_lattice_ui` | 4 파일 생성 완료 | ✓ |
| 7 | `build_menu.{cpp,h}` | 생성 완료 | ✓ |
| 8 | `app/app.cpp` 임시 hook 3곳 | `InitOnce/DrawMenu/RenderWindows` 연결 완료 | ✓ |
| 9 | `CMakeLists.txt` SOURCES_FEATURES 확장 | `features/build` 16 항목 등록 완료 | ✓ |
| 10 | 정적 검증 통과 | 핵심 항목 확인 완료 | ✓ |
| 11 | 동적 검증 | #15~#23 사용자 확인 완료 | ✓ (사용자 입증) |
| 12 | 커밋/PR 정리 | 미수행 (working tree 상태) | △ |

## 1.2 파일/라인 압축 결과

- 신규 파일: `features/build/` 총 16개 (`build_menu` 2 + `periodic_table` 6 + `bravais` 8)
- 신규 코드 라인: **1,549 줄**
- legacy 참조 라인(계획서 인벤토리 기준 대상 10파일): **2,920 줄**
- 압축률: **약 53.0%** (`1549 / 2920`)

→ 계획서의 "legacy 압축 이식" 방향성과 일치한다.

---

# Part 2 — §5 검증 매트릭스 평가 (24 항목)

| # | 검증 항목 | 결과 | 근거 |
|---|---|---|---|
| 1 | `features/build/{periodic_table, bravais}/` 폴더 존재 | ✓ | 디렉터리 2개 확인 |
| 2 | periodic_table 파일 수 6 | ✓ | `.cpp 3 + .h 3` 확인 |
| 3 | bravais 파일 수 8 | ✓ | `.cpp 4 + .h 4` 확인 |
| 4 | `build_menu.cpp/.h` 존재 | ✓ | 2파일 확인 |
| 5 | namespace 일관성 (`features::build::*`) | ✓ | build 하위 파일 전반 namespace 선언 확인 |
| 6 | legacy 호출 0 | ✓ | `AtomsTemplate/atoms::...` 검색 0 hit |
| 7 | `#include ../legacy` 0 | ✓ | legacy include 0 hit |
| 8 | `ElementDatabase` 호출 1+ | ✓ | controller/UI/Bravais seed 등 다수 호출 확인 |
| 9 | 14 Bravais enum 보존 | ✓ | `BravaisLatticeType` 14개 확인 |
| 10 | 7 CrystalSystem enum 보존 | ✓ | `CrystalSystem` 7개 확인 |
| 11 | `app/app.cpp`의 features::build 호출 3곳 | ✓ | `InitOnce/DrawMenu/RenderWindows` 3곳 확인 |
| 12 | CMake SOURCES_FEATURES 확장 | ✓ | `features/build` 16개 항목 등록 확인 |
| 13 | CMake 안전벨트 유지 | ✓ | `Phase 1 violation` FATAL guard 1곳 유지 |
| 14 | §1.4 UI 보존 정적 비교 | △ | 위젯 구조는 유지되나 일부 라벨/흐름 차이 존재 (Part 3 참조) |
| 15 | Debug 빌드 exit 0 | ✓ | 사용자 확인(#15~#23 범위) |
| 16 | Release 빌드 exit 0 | ✓ | 사용자 확인(#15~#23 범위) |
| 17 | 메뉴바 Build 메뉴 표시 | ✓ | 사용자 확인(#15~#23 범위) |
| 18 | Add atoms 윈도우 표시 | ✓ | 사용자 확인(#15~#23 범위) |
| 19 | Bravais Templates 윈도우 표시 | ✓ | 사용자 확인(#15~#23 범위) |
| 20 | Side-by-side 스크린샷 비교 | ✓ | 사용자 확인(#15~#23 범위) |
| 21 | 시나리오 S1/S2 (Periodic Table) | ✓ | 사용자 확인(#15~#23 범위) |
| 22 | 시나리오 S3/S4 (Bravais) | ✓ | 사용자 확인(#15~#23 범위) |
| 23 | 콘솔 에러 0 | ✓ | 사용자 확인(#15~#23 범위) |
| 24 | wasm 사이즈 회귀(Phase3.2 ±5~10%) | ✓ | baseline 14,940,226B → current 14,996,023B (**+0.373%**) |

**집계:** 통과 23 / 조건부 1 / 미확인 0 / 실패 0

---

# Part 3 — 계획 대비 차이(편차) 및 리스크

## 3.1 기능 편차(문서화 필요)

| # | 편차 | 영향 | 판정 |
|---|---|---|---|
| 3.1.1 | 계획서 예시는 `ElementDatabase::Instance().GetElementInfo` 표기이나, 실제 구현은 `getInstance().getElementInfo` 사용 | 인터페이스 스타일 차이(기능 동등) | 허용 |
| 3.1.2 | Periodic Table 액션 버튼 라벨이 legacy `"Add to Structure"` 대비 신규 `"Add atom"` | §1.4 "1:1 보존" 관점에서 표현 차이 | 조건부(사유서 권장) |
| 3.1.3 | legacy Bravais UI의 overlap 경고 팝업 흐름(기존 atom 처리 분기) 대비 신규는 `Preserve Existing Atoms` 토글+Apply 중심 | 특정 edge-case UX 차이 가능성 | 조건부(추가 검증 권장) |
| 3.1.4 | `.gitignore`에 `features/build` unignore 규칙 2줄 추가 | `build/` 전역 ignore 충돌 해소 목적, 필수 기반 작업 | 허용 |

## 3.2 비기능/프로세스 상태

- `legacy/` 변경: **없음** (동결 원칙 준수)
- Phase 3.3 코드 상태: 아직 commit 전 (working tree + untracked 포함)
- Phase 3.2 기준 커밋(`bb4ca37`) 존재로 Phase 3.3 시작 전제는 충족

---

# Part 4 — 저장소 상태 및 최종 판정

## 4.1 현재 변경 상태 요약

- 수정 파일: `.gitignore`, `CMakeLists.txt`, `webassembly/src/app/app.cpp`, `webassembly/src/core/scene/scene_state.h` (외 docs 1건)
- 신규: `webassembly/src/features/build/**` 16파일, `phase3_3_build_periodic_bravais.md`(계획서)

## 4.2 판정

> **조건부 진행 가능 (GO-Conditional)**
>
> 구현 본체와 동적 핵심(#15~#23)은 충족되었으며 실패 항목은 없다.  
> 계획서 매트릭스의 동적 항목(#15~#24)은 모두 충족되었고 실패 항목은 없다.

## 4.3 머지 전 필수 잔여 항목

1. UI 1:1 보존 편차(버튼 라벨/Bravais edge-flow) 사유서 명시 여부 결정
2. Phase 3.3 변경분 커밋 정리 및 PR 본문에 동적 검증 근거(#15~#24) 첨부

---

## 5. 관련 문서

- Phase 3.3 계획서: [`./phase3_3_build_periodic_bravais.md`](./phase3_3_build_periodic_bravais.md)
- 선행 평가서: [`./phase3_2_evaluation_2026-05-06.md`](./phase3_2_evaluation_2026-05-06.md)
- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md)
- 메뉴 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md)
