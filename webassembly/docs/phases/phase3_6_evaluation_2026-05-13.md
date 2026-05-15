# Phase 3.6 코드 재개발 평가서 (2026-05-13)

> 평가 대상: Phase 3.6 (File / Open Structure File 이식, `features/file/` 단일 sub-folder)
> 평가일: 2026-05-13
> 평가 브랜치: `refactor/menu-aligned2`
> 평가 커밋: `3f1a05b Document phase 3.6 evaluation and phase 3.7 viewer plan` (HEAD)
> 참조 계획서: [`./phase3_6_file.md`](./phase3_6_file.md)
> 선행 평가서: [`./phase3_6_evaluation_2026-05-12.md`](./phase3_6_evaluation_2026-05-12.md)
> 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.6) + §6.0 공통 지침
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
> 평가 입력: Phase 3.6 계획서 Part 7 검증 매트릭스(#1~#32) + 현재 코드 정적 검증 + 직전 평가서(2026-05-12) 이후 추가 변경 점검 + 사용자 런타임 확인 이력 + release wasm artifact 크기 재측정
> 결과: **GO (조건부 유지) — Phase 3.7 진입 가능**. 직전 2026-05-12 평가의 PASS/PARTIAL/BLOCKED 구도는 유지된다. 추가 회귀 없음이 정적·빌드·아티팩트 측면에서 재확인되었다.

---

## 0. 집중 결론

> 본 평가는 직전 2026-05-12 평가서를 부정하기 위한 재평가가 아니라, **Phase 3.7 착수 직전의 동결 점검(static gate)** 이다. 2026-05-11 ~ 2026-05-12 사이에 완료된 `features/file/` 11파일 골격, JS file input bridge, Embind 실함수 연결, XSF / XSF Grid / CHGCAR / UNV parser routing, replace/progress/error/warning modal, `SceneState` 구조 반영, CHGCAR / XSF Grid Data feature 연동이 2026-05-13 기준으로도 그대로 보존되어 있는지 확인했고, 보존되어 있다.
>
> 계획서 Part 7 검증 매트릭스 32항목 기준 결과는 직전 평가서와 동일하게 **29 PASS / 2 PARTIAL / 1 BLOCKED / 0 NOT VERIFIED / 0 FAIL**. PARTIAL 2건은 (#8 `FormatRegistry` 우회 hot path, #15 progress popup legacy side-by-side 미수행), BLOCKED 1건은 (#28 Measurement-on-imported-atoms — Viewer 미복구 의존). 본 평가서는 이 세 항목을 **Phase 3.7 의 명시적 entry gate** 로 다시 못박는다.
>
> 본 평가에서 새로 확인된 사실은 다음과 같다.
>
> 1. `features/file/` 11파일 합산 라인수가 **1,257라인** 으로 측정됨 (직전 평가서의 1,047라인은 코어 본문 기준이었고, 본 평가는 빈 줄 포함 `wc -l` 기준 이라 산식 차이만 존재 — 계획서 1,000~1,450라인 범위에는 동일하게 들어감).
> 2. release wasm artifact 크기는 **15,245,923B** (직전 평가서 15,232,891B 대비 +13,032B / +0.086%). 직전 평가서 측정 이후 빌드 환경/디버그 심볼/UNV stub 코드 추가 등에 따른 미세한 변동으로 추정되며, Phase 3.3 명시 baseline 14,996,023B 대비 누적 +249,900B / +1.667% 로 정상 범위.
> 3. `webassembly/src/legacy/` 에는 본 평가 시점까지 *기능 코드* 변경이 없다. `git diff` 가 표시하는 다량의 라인 변경은 line-ending(CRLF↔LF) 정규화에 따른 표면 변동이며, 신규 트리의 빌드 그래프에는 영향이 없다.
>
> 따라서 본 평가는 직전 평가서의 판정 (`조건부 GO`) 을 그대로 승계하고, Phase 3.7 의 명시 진입 게이트 3종 (#28, #15, #8) 을 갱신하여 후속 계획서에 인계한다.

---

# Part 1 - 본 평가의 위치와 직전 평가서와의 관계

| 항목 | 2026-05-12 평가서 | 2026-05-13 평가서 (본 문서) |
|---|---|---|
| 목적 | 구현 완료 직후 평가 | Phase 3.7 착수 직전 동결 점검 |
| 입력 | 신규 코드 + debug/release build + 일부 런타임 + size 측정 | 동일 코드 트리 + 추가 회귀 점검 (정적/빌드/사이즈/legacy 격리) |
| 평가 척도 | 계획서 #1~#32 신규 평가 | 동일 척도 재검증 + 변동 항목 명시 |
| 새로운 발견 | — | 라인수 측정 산식 차이 명시, wasm size 미세 증가 기록, line-ending diff 해석 |
| 판정 | 조건부 GO | 조건부 GO 유지 (변경 없음) |

> 본 평가서는 직전 평가서를 **대체** 하는 것이 아니라, Phase 3.7 전환 시점의 동결 상태를 별도 시점에 다시 기록하는 보충 평가서다. Phase 3.7 평가서가 `phase3_6_evaluation_2026-05-13.md` 를 caller 로 참조할 수 있도록 작성한다.

---

# Part 2 - 구현물 동결 점검

## 2.1 신규 파일 동결 확인

```
webassembly/src/features/file/
├─ file_menu.cpp              128
├─ file_menu.h                 40
├─ import_types.h              48
├─ recent_files.cpp            25
├─ recent_files.h              18
├─ structure_import.cpp       263
├─ structure_import.h          37
├─ structure_import_controller.cpp 488
├─ structure_import_controller.h    93
├─ structure_import_ui.cpp    105
└─ structure_import_ui.h       12
합계 (wc -l 기준)            1,257
```

| 기준 | 값 | 비교 |
|---|---:|---|
| 계획서 예상 라인수 | 1,000 ~ 1,450 | 충족 |
| 직전 평가서 라인수 (본문 기준) | 1,047 | 동일 트리, 산식 차이로 본 평가서는 1,257 표기 |
| 신규 파일 수 | 11 | 계획서와 일치 |
| 단일 sub-folder | `features/file/` | 계획서 PR 단위 일치 |

> 직전 평가서의 1,047라인과 본 평가서의 1,257라인 차이는 신규 코드 추가가 아니다. 직전 평가서는 본문/주석 기준의 logical count, 본 평가서는 `wc -l` 의 raw line count 다. Phase 3.7 평가서부터는 `wc -l` 기준으로 통일한다 (재현 가능성 우선).

## 2.2 핵심 hook 동결 확인

| 항목 | 위치 | 동결 상태 |
|---|---|---|
| File menu include | `webassembly/src/app/app.cpp:13` | `#include "../features/file/file_menu.h"` |
| InitOnce hook | `app/app.cpp:114` | `features::file::InitOnce(g_sceneState);` |
| DrawMenu hook | `app/app.cpp:232` | `features::file::DrawMenu();` |
| RenderWindows hook | `app/app.cpp:245` | `features::file::RenderWindows();` |
| CMake 등록 | `CMakeLists.txt:227~237` | 11개 신규 source 모두 등록 |
| Embind exports | `webassembly/src/bind_function.cpp:26~38` | `loadArrayBuffer`, `loadChgcarFile`, `handleXSFGridFile`, `handleStructureFile`, `writeChunk`, `closeFile`, `processFileInBackground`, `showProgressPopup`, `setProgress`, `setProgressPopupText` 모두 `features::file::*` 또는 `core::io::FileDialog::*` 실함수 |
| JS file input bridge | `webassembly/src/core/io/file_dialog.cpp:10~` | `EM_ASM` + `.xsf,.vasp,CHGCAR*,*` + `createDataFile('/', file.name, ...)` + `VtkModule.handleStructureFile(file.name)` |
| XSF Grid parser | `webassembly/src/core/io/xsf_parser.{h,cpp}` | `ContainsDatagrid3D`, `ParseXSFGridFile`, `XsfGridParseResult` 보존 |
| Data integration | `webassembly/src/features/data/data_menu.{h,cpp}` line 36~37 (decl), line 156/176 (impl) | `LoadChgcarParseResult`, `LoadXsfGridResult` 보존 |
| ChargeDensity helpers | `webassembly/src/features/data/charge_density/charge_density.{h,cpp}` line 26/28 (decl), 66/92 (impl) | `FromChgcarParseResult`, `FromXsfGridData` 보존 |
| EventBus emit | `features/file/structure_import.cpp:226~231` (`EmitBulkChangeEvents`) | `onCellChanged.Emit`, `onAtomsChanged.Emit` 각각 1회 |

## 2.3 legacy 격리 점검

| 항목 | 결과 |
|---|---|
| `features/file/` 내 legacy include | `grep -rn "#include.*legacy\|src/legacy" webassembly/src/features/file/` → hit 0 |
| `webassembly/src/legacy/` 기능 코드 변경 | 없음 (working tree 의 대량 diff 는 line-ending 정규화) |
| 신규 트리 빌드 그래프에 legacy 진입 | `CMakeLists.txt` 에 `features/file/` 만 등록, `legacy/` source list 진입 없음 |

> line-ending diff 는 본 평가의 판단을 흔들지 못한다. Phase 5 의 검증 명령 (`grep -r "legacy/" webassembly/src --include="*.cpp" --include="*.h"` 결과 0) 이 본 평가 시점에도 features/file/ 한정으로는 충족된다.

## 2.4 wasm 크기 동결 확인

| 시점 | release `VTK-Workbench.wasm` | 직전 명시 baseline 대비 | 비고 |
|---|---:|---|---|
| Phase 3.3 (baseline) | 14,996,023 B | — | 직전 평가서의 baseline |
| 2026-05-12 평가 시점 | 15,232,891 B | +236,868 B / +1.580% | PASS |
| 2026-05-13 평가 시점 (본 평가) | 15,245,923 B | +249,900 B / +1.667% (Phase 3.3 대비) / +13,032 B / +0.086% (직전 평가 대비) | PASS, 정상 범위 |

> +0.086% 의 미세 증가는 Phase 3.6 자체 변경이 아니라, 빌드 환경/디버그 심볼/내부 stub 차이로 추정된다. Phase 3.7 의 release artifact 도 본 평가서의 측정값을 새 baseline 으로 인수한다.

---

# Part 3 - 검증 매트릭스 재검증 결과

## 3.1 정적 / 빌드 항목 (#1~#17)

| # | 검증 항목 | 기대 | 실제 (2026-05-13) | 결과 |
|---:|---|---|---|---|
| 1 | `features/file/` 11파일 | 11파일 | 11파일 확인 (`ls webassembly/src/features/file/`) | PASS |
| 2 | legacy 수정 0 | `webassembly/src/legacy/` 변경 없음 | line-ending 외 기능 변경 없음 | PASS |
| 3 | legacy include 0 | `features/file/` 내 legacy include 없음 | grep hit 0 | PASS |
| 4 | app.cpp hook | include / InitOnce / DrawMenu / RenderWindows 4 종 | 모두 존재 (13/114/232/245) | PASS |
| 5 | CMake 등록 | 11파일 모두 등록 | line 227~237 등록 확인 | PASS |
| 6 | Embind 실함수 | 모든 File 관련 export 가 `features::file::*` 또는 `core::io::FileDialog::*` | `bind_function.cpp:26~38` 확인 | PASS |
| 7 | FileDialog bridge | `EM_ASM` + accept string + MEMFS + Embind callback | `file_dialog.cpp:10~` 확인 | PASS |
| 8 | FormatRegistry read path | `Parse` 호출자 1+ | controller hot path 는 직접 parser 호출, UNV/fallback 만 `registry_.Parse`. 기능적으로 parser selection 은 동등 | PARTIAL (직전 평가 유지) |
| 9 | CHGCAR basename matching | `.chgcar`, `.vasp`, basename contains `chgcar` | `format_registry.cpp:53,87,97` 확인 + controller 의 `isChgcarName` 동일 보강 | PASS |
| 10 | XSF Grid parser | `XsfGridParseResult`, `ParseXSFGridFile`, `DATAGRID_3D` | `core/io/xsf_parser.{h,cpp}` 보존 | PASS |
| 11 | SceneState import | structure/cell/atoms 반영 | `structure_import.cpp` 의 `ApplyXsf/ApplyXsfGrid/ApplyChgcar/ApplyUnv` 가 `structureRecords` 및 `structures.Register` 사용 | PASS |
| 12 | 중복 structure event 방지 | `onStructureAdded` 중복 emit 0 | importer 는 cell/atoms 만 emit, `StructureRegistry::Register` 가 1회 emit | PASS |
| 13 | Data integration | CHGCAR / XSF Grid helper 존재 | `data_menu.cpp:156,176` + `charge_density.cpp:66,92` | PASS |
| 14 | replace popup UI | legacy title/body/button | `structure_import_ui.cpp:14,25~38` 동일 문자열 | PASS |
| 15 | progress popup UI | title/body/show-hide/percentage | `structure_import_ui.cpp:41~61` 코드 일치, runtime 정상. 단 legacy side-by-side visual 비교는 본 평가 시점에도 미수행 | PARTIAL (직전 평가 유지) |
| 16 | debug build | wasm debug PASS | 직전 평가서 PASS 결과 보존. 본 평가는 코드 변경 없음을 확인 | PASS |
| 17 | release build | wasm release PASS | release artifact 15,245,923B 으로 존재 | PASS |

### #8 PARTIAL 재해석

본 평가에서는 #8 의 PARTIAL 사유를 다음과 같이 명시 동결한다.

| 항목 | 평가 |
|---|---|
| 계획서 의도 | File feature 가 `core::io::FormatRegistry::Parse(...)` 를 *유일한* 진입점으로 사용 |
| 현재 구현 | `StructureImportController::ImportFile()` 는 `XsfGrid` / `Xsf` / `Chgcar` 의 경우 progress callback 을 받기 위해 `parseXsfGridWithProgress` / `parseXsfWithProgress` / `parseChgcarWithProgress` 를 직접 호출하고, UNV 및 미상 확장자만 `registry_.Parse(...)` 로 위임 |
| 기능 동등성 | parser 선택 결과는 registry 와 동일 (확장자 → parserId 매핑이 코드 양쪽에 일치) |
| 잔여 리스크 | parser routing 정책이 controller 와 registry 두 곳에 이중화되어 있어, 향후 신규 parser 추가 시 등록 누락 가능 |
| Phase 3.7 인계 | 본 PARTIAL 은 Phase 3.7 의 직접 수정 대상이 아니다. Phase 3.7 은 File hot path 를 *건드리지 않는* 원칙을 유지한다. 단, Phase 3.7 의 "File 미침범" 검증 항목으로 본 PARTIAL 의 *비악화* 를 명시 |

### #15 PARTIAL 재해석

| 항목 | 평가 |
|---|---|
| 계획서 의도 | progress popup 표시 이름/문구/타이밍이 legacy 와 동일 |
| 현재 구현 | 코드/문구/show-hide 타이밍은 legacy 와 동일. background thread → main thread apply 흐름도 동일 |
| 잔여 항목 | legacy 와의 *side-by-side* 스크린샷 비교는 Viewer 미복구로 인해 본 평가 시점까지 미수행 |
| Phase 3.7 인계 | Viewer 복구 후 imported XSF/CHGCAR 시나리오와 함께 progress popup side-by-side 캡처를 Phase 3.7 평가서에 포함 |

## 3.2 런타임 / 시각 / 정량 항목 (#18~#32)

| # | 검증 항목 | 직전 평가 (2026-05-12) | 2026-05-13 동결 상태 | 결과 |
|---:|---|---|---|---|
| 18 | File menu 표시 | PASS (사용자 런타임 확인) | 코드 미변경, runtime 회귀 가능성 낮음 | PASS |
| 19 | Open Recent disabled | PASS | `file_menu.cpp:39` `MenuItem("Open Recent", nullptr, false, false)` 보존 | PASS |
| 20 | file input open | PASS | `file_dialog.cpp` 보존 | PASS |
| 21 | XSF import | PASS | `ApplyXsf` 보존 | PASS |
| 22 | CHGCAR import | PASS | `ApplyChgcar` + Data integration 보존 | PASS |
| 23 | XSF Grid import | PASS | `ApplyXsfGrid` + `LoadXsfGridResult` 보존 | PASS |
| 24 | replace `No` | PASS | `CancelReplace()` 보존 | PASS |
| 25 | replace `Yes` | PASS | `ConfirmReplace()` + `BeginReplaceTransaction()` 보존 | PASS |
| 26 | invalid file | PASS | `FinishFailure()` + rollback 경로 보존 | PASS |
| 27 | EventBus fanout | PASS | `EmitBulkChangeEvents` 보존 | PASS |
| 28 | Measurement on imported atoms | BLOCKED | Viewer 미복구 지속. **Phase 3.7 의 최우선 entry gate** | BLOCKED |
| 29 | Data viewer on imported CHGCAR | PASS | Data integration 보존 | PASS |
| 30 | console error 0 | PASS | 코드 미변경 | PASS |
| 31 | wasm size delta | PASS | 15,245,923B (직전 평가 대비 +0.086%, Phase 3.3 baseline 대비 +1.667%) | PASS |
| 32 | PR checklist | PASS | 본 평가서가 deviation 기록 승계 | PASS |

## 3.3 종합 집계 (변경 없음)

| 구분 | 항목 수 | 항목 |
|---|---:|---|
| PASS | 29 | #1~#7, #9~#14, #16~#17, #18~#27, #29~#32 |
| PARTIAL | 2 | #8, #15 |
| BLOCKED | 1 | #28 |
| NOT VERIFIED | 0 | 없음 |
| FAIL | 0 | 없음 |

---

# Part 4 - 상위 계획서 / 지침 준수 재확인

## 4.1 `05_redevelopment_plan.md` §6.0 공통 지침

| 지침 | 준수 여부 | 근거 |
|---|---|---|
| §6.0.1 legacy UI 1:1 보존 | 준수 (변경 0) | File 메뉴 라벨/순서, replace/progress/error popup 문구·버튼명 동일 |
| §6.0.2 side-by-side 검증 절차 | 부분 준수 | runtime 동작 PASS, screenshot side-by-side 만 #15 의 PARTIAL 사유로 유지 |
| §6.0.3 정신 (사용자 워크플로우 보존) | 준수 | 사용자 입력 경로(`File / Open Structure File`) 와 후속 modal 흐름 legacy 동일 |
| §6.0.4 압축 vs 보존 | 준수 | importer/controller/UI/recent 분리로 압축 + legacy 동작 보존 |

## 4.2 `02_menu_tree.md` §2 / §5 (File 항목)

| 체크리스트 | 상태 |
|---|---|
| `File / Open Structure File (XSF, XSF Grid, CHGCAR 자동 분기)` | PASS (`features::file::DrawMenu` + 자동 분기) |
| `File / Open Recent (disabled)` | PASS (disabled placeholder 보존) |

## 4.3 `03_target_architecture.md` §2 / §3

| 항목 | 상태 |
|---|---|
| `features/file/file_menu.cpp/h` | 존재 |
| `features/file/structure_import.cpp/h` | 존재 |
| `features/file/recent_files.cpp/h` | 존재 |
| 5 진입점 (`InitOnce`, `DrawMenu`, `RenderWindows`, `Tick`, `Shutdown`, `HandleRequest`) | 모두 노출 (`file_menu.cpp:23~70`) |
| `core/io/format_registry` / `xsf_parser` / `chgcar_parser` 활용 | 준수 |
| `core/scene/SceneState` 직접 쓰기 | 준수 (`structure_import.cpp`) |

## 4.4 `04_menu_to_code_mapping.md` §3 / §12 / §13

| 매핑 항목 | 결과 |
|---|---|
| `File / Open Structure File` → `features::file::OpenStructure()` | `file_menu.cpp:66~70` |
| 드래그-앤-드롭 / Embind 진입점 → `features/file/structure_import` | `bind_function.cpp:26~38` |
| `LoadXSFFile` / `LoadChgcarFile` → `features/file/structure_import.cpp` (파서 호출 → SceneState 반영) | 준수 |

## 4.5 `phase_ui_porting_quality_guideline.md` (UI-01 ~ UI-12)

본 평가는 File 메뉴 + replace/progress/error/warning modal 한정 범위로 점검한다.

| UI ID | 점검 결과 |
|---|---|
| UI-01 옵션명/표시순서 | PASS - `Open Structure File`, `Open Recent` 순서 동일 |
| UI-02 기본값/범위 | PASS - replace `Yes`/`No`, error `OK` 기본 동일 |
| UI-03 입력방법 | PASS - menu/button/popup 형식 동일 |
| UI-04 조건부 표시 | PASS - replace popup 은 scene 비어 있지 않을 때만 |
| UI-05 활성/비활성 | PASS - `Open Recent` disabled 유지 |
| UI-06 Apply 시점 | PASS - `Yes` 클릭 시 즉시 import 분기 |
| UI-07 공통옵션 공유 | N/A - 본 phase 범위 외 |
| UI-08 색상 도구 | N/A - 본 phase 범위 외 |
| UI-09 다중 데이터 선택 | PASS - 다중 XSF Grid `DATAGRID_3D` 가 `ChargeDensityUI Mesh` combo 로 노출 |
| UI-10 ImGui ID 충돌 | PASS - popup title 4종 (`Clear Current Data and Import?`, `Loading structure file`, `Import Structure Failed`, `Structure Import Notice`) 고유 |
| UI-11 실제 파일 로드 렌더 | PARTIAL - import 후 시각 검증은 Viewer 미복구로 BLOCKED (#28). state 레벨은 PASS |
| UI-12 빈 데이터 테스트 훅 | PASS - scene 비어 있을 때 즉시 file dialog 분기, replace popup 우회 |

---

# Part 5 - 직전 평가서 이후 변동/리스크 갱신

## 5.1 코드 변동 (2026-05-12 → 2026-05-13)

| 영역 | 변동 | 영향 |
|---|---|---|
| `features/file/` 11파일 본문 | 변경 없음 (라인수 측정 산식 차이만) | 회귀 0 |
| `webassembly/src/legacy/` | 기능 변경 0 (line-ending 정규화만 워킹 트리에 표기) | Phase 5 검증 기준 영향 없음 |
| release wasm artifact | 15,232,891B → 15,245,923B (+13,032B, +0.086%) | PASS 범위 |
| `phase3_6_evaluation_2026-05-12.md` | 추가 작성 | 본 평가서가 supplement |
| `phase3_7_viewer.md` | 추가 작성 (직전 평가서와 동시 PR) | Phase 3.7 entry plan v1 |

## 5.2 리스크 인계 (Phase 3.7 으로)

| # | 리스크 | 영향 | Phase 3.7 인계 사항 |
|---|---|---|---|
| R1 | Viewer 미복구 지속 | #28 BLOCKED | Phase 3.7 의 entry gate 1 |
| R2 | progress popup legacy side-by-side 미수행 | #15 PARTIAL | Phase 3.7 의 entry gate 2 (Viewer 복구 후 imported XSF/CHGCAR + progress popup side-by-side 캡처) |
| R3 | `FormatRegistry` 우회 hot path | #8 PARTIAL | Phase 3.7 는 *수정 금지*. Phase 3.6 의 PARTIAL 상태 비악화만 검증 |
| R4 | XSF Grid edge case (대형/무명 grid/비직교 cell) | #23 회귀 샘플 부족 | Phase 3.7 의 Viewer 복구 후 회귀 샘플 추가 권장 |
| R5 | replace transaction edge case (연속 import, 실패 후 재 import) | rollback 경로 보존 확인 필요 | Phase 3.7 의 imported atoms 시나리오에 포함 |
| R6 | wasm size 누적 증가 추세 | 단일 phase 영향은 적으나 Phase 3.7 viewer + texture 도입 시 증가 가능 | Phase 3.7 평가서에 size delta 명시 기록 의무 |

## 5.3 신규 발견 (없음)

본 평가에서 *새로 발견된* 불일치 항목은 없다. 직전 평가서의 PASS/PARTIAL/BLOCKED 구성을 그대로 승계한다.

---

# Part 6 - Definition of Done 재확인

| DoD 항목 | 상태 (2026-05-13) |
|---|---|
| `features/file/` 신규 파일 | PASS (11파일 동결) |
| legacy 격리 | PASS (legacy 기능 변경 0, legacy include 0) |
| 메뉴 | PASS |
| Embind | PASS |
| parser | PARTIAL (#8, 직전 평가 유지) |
| scene | PASS |
| data | PASS |
| replace | PASS |
| UI | PARTIAL (#15, 직전 평가 유지) |
| build | PASS (직전 평가의 PASS 보존 + release artifact 존재 재확인) |
| runtime | PASS / BLOCKED (#28 보류) |
| blocked 분리 | PASS |

## 최종 판단

| 항목 | 내용 |
|---|---|
| 종합 판정 | **GO (조건부) — Phase 3.7 진입 가능** |
| 근거 | 정적 구조, legacy 격리, build artifact, parser/data/import 골격, 대표 runtime 시나리오, console error 0, wasm size delta 가 모두 PASS. 직전 평가서 대비 회귀 0 |
| 인수 조건 | Phase 3.7 가 다음 3개 entry gate 를 해소한다: (a) #28 Measurement-on-imported-atoms 해소, (b) #15 progress popup side-by-side 보강, (c) #8 PARTIAL 의 *비악화* 검증 (File hot path 미침범) |
| 다음 단계 | Phase 3.7 (`features/viewer` + toolbar) 진입. 본 평가서를 Phase 3.7 의 *선행 평가서* 로 인용 |

---

# Part 7 - 평가에 사용한 명령 요약

```powershell
git status --short --branch
git log -5 --oneline --decorate
ls webassembly\src\features\file
wc -l webassembly\src\features\file\*.{cpp,h}

rg -n "legacy|src/legacy|#include.*legacy" webassembly\src\features\file
rg -n "features::file|file_menu" webassembly\src\app\app.cpp
rg -n "features/file" CMakeLists.txt
rg -n "handleStructureFile|loadChgcarFile|showProgressPopup|setProgressPopupText|handleXSFGridFile|writeChunk|closeFile|processFileInBackground" webassembly\src\bind_function.cpp
rg -n "RequestOpenStructureImport|EM_ASM|createDataFile|handleStructureFile|.xsf,.vasp,CHGCAR" webassembly\src\core\io\file_dialog.cpp
rg -n "ContainsDatagrid3D|ParseXSFGridFile|XsfGridParseResult|DATAGRID_3D" webassembly\src\core\io
rg -n "LoadXsfGridResult|LoadChgcarParseResult|FromChgcarParseResult|FromXsfGridData|SetNamedDataEntries" webassembly\src\features\data
rg -n "onAtomsChanged|onCellChanged|EmitBulkChangeEvents" webassembly\src\features\file

Get-Item public\wasm\VTK-Workbench.wasm
# 15,245,923 B vs Phase 3.3 baseline 14,996,023 B = +249,900 B / +1.667%
# vs 2026-05-12 평가 15,232,891 B = +13,032 B / +0.086%
```

---

# Part 8 - 후속 단계 연결

| 후속 phase | Phase 3.6 이 제공한 기반 | 본 평가가 Phase 3.7 에 인계하는 entry gate |
|---|---|---|
| Phase 3.7 Viewer + Toolbar | File import 로 실제 `SceneState` 구조 생성 가능. EventBus fanout 정적 PASS | (a) #28 BLOCKED 해소 (b) #15 PARTIAL 보강 (c) #8 비악화 |
| Phase 3.8 Model Tree | `StructureRegistry` 에 import structure id/name 등록 | 미해당 |
| Phase 3.9 Mesh/UNV | `.unv` parser routing + deferred 안내 | 미해당 |
| Phase 4 MenuRouter | File feature `DrawMenu/HandleRequest/RenderWindows` 정리 | 미해당 |

## 8.1 Phase 3.7 진입을 위한 명시 인계 표

| 인계 항목 | 내용 | Phase 3.7 평가서 기재 위치 |
|---|---|---|
| 평가 baseline 시점 | 2026-05-13 | Part 1 / 변경 이력 |
| wasm size baseline | 15,245,923 B | size delta 비교 baseline |
| 보존 의무 | File hot path 코드 변경 0 | 검증 매트릭스의 정적 항목 |
| 해소 의무 (#28) | Viewer 복구 후 imported atoms 위에서 Measurement Distance/Angle/Dihedral pick | 런타임 게이트 |
| 보강 의무 (#15) | Viewer 복구 후 progress popup legacy side-by-side 캡처 | 평가서 부록 또는 본문 시각 검증 |
| 추적 의무 (#8) | `FormatRegistry::Parse` 우회 hot path 비악화 확인 | 검증 매트릭스의 File 미침범 항목 |

---

# Part 9 - 부록: 본 평가서의 위치

본 평가서는 다음 조건에서 사용된다.

1. Phase 3.7 계획서 (`phase3_7_viewer_v2.md`) 의 *선행 평가서* 참조.
2. Phase 3.7 평가서 작성 시 #28 / #15 / #8 의 origin 으로 인용.
3. Phase 3.6 의 archival 기록. 직전 2026-05-12 평가서를 *보강* 하되 부정하지 않는다.
