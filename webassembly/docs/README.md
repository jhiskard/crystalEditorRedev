# VTK Workbench WebAssembly — Architecture Redevelopment Plan

> 작성일: 2026-04-27
> 대상: `webassembly/src` (Crystal Viewer / Atoms 기능 포함)
> 작업 방침: 기존 코드 전체를 `webassembly/src/legacy/` 로 이동시키고, `webassembly/src/`
> 아래에 **메인 메뉴(File / Edit / Build / Measurement / Data / Utilities / Settings / Windows)**
> 트리와 1:1 로 대응되는 새 아키텍처를 처음부터 재구성한다. 새 코드는
> `legacy/` 의 로직을 참조해 옮기되, 모든 모듈/클래스/함수에 Doxygen 스타일 주석을 단다.

---

## 0. 문서 구성

| 문서 | 내용 |
|---|---|
| [01_current_structure.md](./01_current_structure.md) | 현재 `webassembly/src` 트리·라인수·계층 구조 분석과 한계 진단 |
| [02_menu_tree.md](./02_menu_tree.md) | `app.cpp::renderDockSpaceAndMenu` 에서 자동 추출한 메인 메뉴 트리와 항목별 동작 |
| [03_target_architecture.md](./03_target_architecture.md) | 새 아키텍처의 5대 원칙, 3-layer 셸/코어/피처 구조, 디렉터리 트리 설계 |
| [04_menu_to_code_mapping.md](./04_menu_to_code_mapping.md) | 메뉴 항목 ↔ 새 디렉터리/파일/엔트리 1:1 매핑 표 |
| [05_redevelopment_plan.md](./05_redevelopment_plan.md) | "legacy 동결 → 그린필드 재구성" 6단계 마이그레이션 절차 |
| [06_doxygen_style_guide.md](./06_doxygen_style_guide.md) | 모듈/클래스/함수 단위 Doxygen 주석 컨벤션과 예시 |

### Phase 별 세부계획서 (`phases/`)

| 문서 | 내용 |
|---|---|
| [phases/phase0_legacy_freeze.md](./phases/phase0_legacy_freeze.md) | Phase 0 — 현 코드 전체를 `webassembly/src/legacy/` 로 동결하는 단일 PR 의 파일 이동 매트릭스, CMake 패치, 검증/롤백 절차 |
| [phases/phase0_evaluation_2026-04-28.md](./phases/phase0_evaluation_2026-04-28.md) | Phase 0 시도 평가 + Phase 1 진행가능 여부 통합 평가서 (2026-04-28) — 1차/2차 시도 회고 + 계획서 보강 효과 + Phase 1 진입 전 정리 3 항목. *(2026-04-28 작성된 두 개의 평가서를 단일 문서로 통합한 결과물)* |
| [phases/phase1_app_core_bootstrap.md](./phases/phase1_app_core_bootstrap.md) | Phase 1 — `app/` + `core/` 빈 셸 부트스트랩 세부계획서. 빈 dockspace 한 장만 띄우는 최소 빌드, legacy/ 빌드 제외, main/bind 의 stub 화, CMake 안전벨트, 검증 매트릭스 13 항목 |
| [phases/phase1_evaluation_2026-04-28.md](./phases/phase1_evaluation_2026-04-28.md) | Phase 1 수행 결과 평가서 (2026-04-28) — 13 항목 §5 검증 매트릭스 충족도 (9 통과 / 1 일탈 / 3 추정) + font_manager App 의존성 발견 + compat shim 회색지대 분류 + Phase 2 진입 전 정리 4 항목 |
| [phases/phase2_core_skeleton.md](./phases/phase2_core_skeleton.md) | Phase 2 — `core/` 인프라 골격 세부계획서. SceneState 추출 / file_dialog + format_registry / 6 sub-folder 약 48 신규 파일 / Phase 1 의 compat shim 정리 / PR 분할 (scene+data, io+vtk+render+ui) / 검증 매트릭스 18 항목 |
| [phases/phase2_evaluation_2026-04-28.md](./phases/phase2_evaluation_2026-04-28.md) | Phase 2 수행 결과 평가서 (2026-04-28) — 사용자 2 차 답변 반영 후 **GO**: §5 검증 매트릭스 18 항목 (15 통과 / 0 일탈 / 2 추정 / 1 미수행), Step 1 청산 완료, legacy/ 동결 유지, debug+release 빌드 + 빈 dockspace 모두 통과. **Phase 3 진입 전 commit 정리 1 항목만** 남음 |
| _phase3 ~ phase6_ | _작성 예정 — Phase 별 세부계획서를 같은 폴더에 누적_ |

## 1. 한 줄 요약

> **메뉴 이름으로 폴더를 찾을 수 있다 = 신규 개발자 온보딩 비용 절감**

`webassembly/src/`
- `app/` — 도크스페이스, 메뉴 라우터, 셸(About/Settings/Layout)
- `core/` — 둘 이상의 피처가 공유하는 기반(VTK, IO, Scene 상태, 렌더 리소스, 공용 UI)
- `features/` — **메뉴 = 폴더**. 한 메뉴 항목을 수정할 때 폴더 하나만 열면 끝
- `legacy/` — 현 코드 전체. 빌드 대상에서 제외. 로직 참조용 동결본

## 2. 핵심 결정

| 항목 | 결정 |
|---|---|
| **재구성 방식** | "in-place 점진 리팩터" 가 아니라 **legacy 동결 + 그린필드 재구성** |
| **디렉터리 축** | 1축은 메뉴(features/), 2축은 레이어(domain/render/ui), 3축은 공유성(core 승격 기준) |
| **God Object 처리** | `AtomsTemplate` 에 누적된 공유 상태는 `core/scene/SceneState` 로 분리 |
| **메뉴 → 피처 호출** | `app/menu_router.*` 가 클릭을 `features::<feature>::HandleRequest(...)` 로 디스패치. 피처 → 피처 직접 호출 금지 |
| **주석** | 모든 모듈/클래스/함수에 Doxygen (`@brief`, `@param`, `@return`, `@note`) 주석 의무화. 빈 주석/의미 없는 주석 금지 |
| **빌드** | `CMakeLists.txt` 는 `legacy/` 를 컴파일 대상에서 제외하고, 새 트리만 빌드 |

## 3. 작업 흐름 한눈에

```
[Phase 0] 현재 구조 동결 (legacy/ 로 일괄 이동, 빌드 임시 OFF)
    ↓
[Phase 1] core/ + app/ 셸을 빈 빌드 가능 상태로 부트스트랩 (Hello dockspace)
    ↓
[Phase 2] core/scene/SceneState + core/io/format_registry 골격 채움
    ↓
[Phase 3] features/ 를 메뉴 단위로 한 개씩 이식 (BZ → Data → Build → Edit → Measurement → File → Viewer/Toolbar → ModelTree → Mesh)
    ↓
[Phase 4] app/menu_router 로 메뉴바 일원화, app.cpp 슬림화
    ↓
[Phase 5] legacy/ 의존이 끊어지면 빌드에서 완전히 제거 (또는 별도 데드코드 폴더로 보관)
    ↓
[Phase 6] 이벤트 버스, CMake 정돈, 문서/주석 점검
```

## 4. 다음 단계 권장

1. 본 문서 검토 및 합의.
2. `webassembly/src/legacy/` 디렉터리 신설 후 현 파일 일괄 이동
   (이때 build target 은 임시 비활성화 상태로 남기는 안전한 PR 한 개로).
3. `app/` + `core/` 의 빈 셸을 빌드 가능한 "Hello dockspace" 상태까지 끌어올린 PR 한 개.
4. 이후 메뉴 한 개씩 PR 분리.

자세한 단계와 파일별 행선지는 [05_redevelopment_plan.md](./05_redevelopment_plan.md) 참고.
