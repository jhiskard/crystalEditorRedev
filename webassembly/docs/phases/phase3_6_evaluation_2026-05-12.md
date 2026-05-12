# Phase 3.6 코드 재개발 평가서 (2026-05-12)

> 평가 대상: Phase 3.6 (File / Open Structure File 이식, `features/file/` 단일 sub-folder)
> 평가일: 2026-05-12
> 평가 브랜치: `refactor/menu-aligned2`
> 평가 커밋: `df95bc2 Implement phase 3.5 measurement and phase 3.6 file workflows`
> 참조 계획서: [`./phase3_6_file.md`](./phase3_6_file.md)
> 선행 평가서: [`./phase3_5_evaluation_2026-05-11_v2.md`](./phase3_5_evaluation_2026-05-11_v2.md)
> 평가 입력: Phase 3.6 계획서 Part 7 검증 매트릭스 + 현재 코드 정적 검증 + debug/release wasm 빌드 결과 + 사용자 런타임 확인 사항 + wasm size 측정
> 사용자 확인 사항: 검증 #18~#27 및 #29~#30 은 런타임에서 정상 작동 확인 완료. #28 은 Viewer/Measurement 상호작용 의존 항목으로 별도 BLOCKED 유지.
> 결과: **조건부 진행 가능 (GO with Viewer-dependent scenario blocked)** - 정적 구조, 빌드, 대표 File 런타임 시나리오, console error 0, wasm size 회귀가 통과했으며, Viewer 의존 Measurement 검증 1개 항목만 보류.

---

## 0. 집중 결론

> Phase 3.6 은 계획서가 요구한 `features/file/` 신규 11파일 구조, `File / Open Structure File` 메뉴 hook, JS file input bridge, Embind 실함수 연결, XSF / XSF Grid / CHGCAR / UNV parser routing, replace/progress/error/warning modal, `SceneState` 구조 반영, CHGCAR/XSF Grid Data feature 연동까지 코드 기준으로 대부분 충족했다.
>
> 계획서 Part 7 검증 매트릭스 32항목 기준 결과는 **29 PASS / 2 PARTIAL / 1 BLOCKED / 0 NOT VERIFIED / 0 FAIL** 로 집계한다. PASS 는 정적 구조, legacy 격리, CMake/app/Embind hook, FileDialog bridge, CHGCAR/XSF Grid parser 보강, SceneState import, Data integration, debug/release build, #18~#27 런타임 시나리오, #29 Data viewer runtime, #30 console error 0, #31 wasm size delta, #32 deviation 기록 항목이다. PARTIAL 은 `FormatRegistry` 사용 범위와 progress popup 의 legacy side-by-side 미비 항목이다. BLOCKED 는 Viewer 미복구 전제의 Measurement-on-imported-atoms 검증이다.
>
> 코드 규모는 `features/file/` 신규 11파일 **1,047라인** 으로 계획서 예상 1,000~1,450라인 범위에 들어간다. legacy `file_loader.cpp` 의 대형 단일 책임을 File menu, importer, controller, UI, recent files 로 분리한 구조도 계획과 부합한다.
>
> 최종 판단은 **조건부 GO** 다. Phase 3.6 의 대표 File 런타임 시나리오는 통과로 갱신되었고, 남은 조건은 Phase 3.7 Viewer 복구 이후 #28 Measurement-on-imported-atoms 를 재검증하는 것이다.

---

# Part 1 - Phase 3.6 구현 개요

## 1.1 타임라인

| 시점 | 단계 | 결과 |
|---|---|---|
| t0 | Phase 3.6 계획서 기준 구현 범위 확인 | 완료 |
| t0+ | `features/file/` 11파일 추가 | PASS |
| t0+ | `File / Open Structure File` 메뉴 및 `Open Recent` disabled placeholder 추가 | PASS |
| t0+ | `core/io/file_dialog.cpp` 에 JS file input + MEMFS bridge 복구 | PASS |
| t0+ | `bind_function.cpp` File 관련 no-op stub 를 실함수로 교체 | PASS |
| t0+ | `FormatRegistry` 에 `.vasp`, `CHGCAR*`, XSF Grid 분기 보강 | PASS |
| t0+ | `xsf_parser` 에 `DATAGRID_3D` result/parser/progress callback 보강 | PASS |
| t0+ | `StructureImporter` 로 XSF/CHGCAR/XSF Grid/UNV payload 를 `SceneState` 에 적용 | PASS |
| t0+ | `StructureImportController` 에 replace transaction, background parse, main-thread apply 추가 | PASS |
| t0+ | Data feature 에 CHGCAR parse result 및 XSF 다중 grid 주입 helper 추가 | PASS |
| t0+ | legacy progress popup 동작 보강 요청 반영 | PASS, 런타임 정상 작동 확인. 단 legacy side-by-side 는 미수행 |
| t0+ | XSF 다중 `DATAGRID_3D` 선택 옵션 보강 요청 반영 | PASS |
| t0+ | #18~#27 및 #29~#30 사용자 런타임 확인 | PASS |
| t0+ | #31 wasm size delta 추가 측정 | PASS |
| t0+ | `npm run build-wasm:debug` 확인 | PASS |
| t0+ | `npm run build-wasm:release` 확인 | PASS |

## 1.2 신규 파일 구성과 라인수

| 파일 | 역할 | 라인수 |
|---|---|---:|
| `file_menu.cpp` | File 메뉴, Open Structure dispatch, wrapper 함수 | 102 |
| `file_menu.h` | File feature public API | 31 |
| `import_types.h` | import kind, summary, progress, snapshot 타입 | 40 |
| `recent_files.cpp` | recent file skeleton | 19 |
| `recent_files.h` | recent file store 선언 | 13 |
| `structure_import.cpp` | parse payload 를 `SceneState` 로 변환 | 220 |
| `structure_import.h` | importer interface | 27 |
| `structure_import_controller.cpp` | dialog, replace transaction, background parse, apply orchestration | 419 |
| `structure_import_controller.h` | controller interface/state | 75 |
| `structure_import_ui.cpp` | replace/progress/error/warning modal 렌더링 | 93 |
| `structure_import_ui.h` | UI wrapper 선언 | 8 |
| **합계** | **11파일** | **1,047** |

### 라인수 평가

| 기준 | 값 | 평가 |
|---|---:|---|
| 계획서 예상 | 1,000~1,450 | 실제 1,047로 범위 내 |
| 신규 파일 수 | 11 | 계획서와 일치 |
| 구조 압축 | legacy 단일 `file_loader` 대비 분리 | controller/importer/UI 책임 분리 양호 |
| 위험 해석 | 낮음~중간 | controller 가 background parse와 transaction 책임을 함께 가져 후속 분리 여지는 있음 |

---

# Part 2 - 검증 매트릭스 결과

## 2.1 정적 / 빌드 항목 (#1~#17)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---:|---|---|---|---|
| 1 | 신규 파일 수 | `features/file/` 11파일 | 11파일 | PASS |
| 2 | legacy 수정 없음 | `webassembly/src/legacy/` 변경 0 | 변경 없음 | PASS |
| 3 | legacy include 없음 | `features/file` legacy include 0 | hit 0 | PASS |
| 4 | File menu hook | `app.cpp` include/init/draw/render hook | `file_menu.h`, `InitOnce`, `DrawMenu`, `RenderWindows` 확인 | PASS |
| 5 | CMake 등록 | 신규 11파일 등록 | `CMakeLists.txt` line 227~237 등록 | PASS |
| 6 | Embind 실함수 | stub 제거 또는 real call | `handleStructureFile`, `loadChgcarFile`, `showProgressPopup`, `setProgressPopupText` 등 실함수 연결 | PASS |
| 7 | FileDialog bridge | `EM_ASM` + accept string | `.xsf,.vasp,CHGCAR*,*`, MEMFS `createDataFile`, `handleStructureFile` 확인 | PASS |
| 8 | FormatRegistry read path | `Parse` 호출자 1+ | `registry_.Parse(filePath, task->parsed)` 존재. 단 XSF/CHGCAR hot path 는 progress callback 때문에 직접 parser 호출 | PARTIAL |
| 9 | CHGCAR basename matching | basename contains `chgcar` 또는 `.vasp` 처리 | `format_registry.cpp` 에 `.chgcar`, `.vasp`, basename contains `chgcar` 처리 | PASS |
| 10 | XSF Grid parser | `XsfGridParseResult` + `ParseXSFGridFile` | result struct, parser, `DATAGRID_3D`, label parsing, progress callback 확인 | PASS |
| 11 | SceneState import | structure/cell/atoms 반영 | `structureRecords`, `nextAtomId`, `structures.Register`, cell/atoms copy 확인 | PASS |
| 12 | 중복 structure event 방지 | `onStructureAdded.Emit` 중복 없음 | `StructureRegistry::Register` 가 1회 emit, importer 는 별도 중복 emit 없음 | PASS |
| 13 | Data integration | CHGCAR/XSF Grid helper | `LoadChgcarParseResult`, `LoadXsfGridResult`, `FromChgcarParseResult`, `FromXsfGridData` 확인 | PASS |
| 14 | replace popup UI | legacy title/body/button 동일 | title/body/Yes/No 코드 일치 | PASS |
| 15 | progress popup UI | title/body/show-hide 동일 | title/body, percentage progress bar, background parse callback 구현 및 runtime 정상 작동 확인. 단 legacy side-by-side visual 비교 미수행 | PARTIAL |
| 16 | debug build | wasm debug build PASS | `npm run build-wasm:debug` 통과 | PASS |
| 17 | release build | wasm release build PASS | `npm run build-wasm:release` 통과 | PASS |

### #8 PARTIAL 사유

| 항목 | 평가 |
|---|---|
| 계획서 의도 | File import 가 `FormatRegistry::Parse` 를 실제 읽기 사용자로 호출 |
| 현재 코드 | fallback/UNV 계열은 `registry_.Parse` 를 호출하지만, XSF/XSF Grid/CHGCAR 는 legacy progress callback 보존을 위해 controller 에서 직접 parser 호출 |
| 기능 동등성 | parser selection 및 payload type 은 유지됨 |
| 아키텍처 편차 | `FormatRegistry` 에 progress-aware `ParseWithProgress` 가 없어서 hot path 가 registry 를 우회함 |
| 판단 | 사용자-visible 동작 보존을 우선한 허용 가능한 PARTIAL. 후속 보강 권장 |

## 2.2 런타임 / 시각 / 정량 항목 (#18~#32)

| # | 검증 항목 | 기대값 | 실제 | 결과 |
|---:|---|---|---|---|
| 18 | File menu 표시 | `File / Open Structure File` 런타임 표시 | 사용자 런타임 확인: 정상 작동 | PASS |
| 19 | Open Recent disabled | disabled placeholder 표시 | 사용자 런타임 확인: 정상 작동 | PASS |
| 20 | file input open | accept string 정상 | 사용자 런타임 확인: 정상 작동 | PASS |
| 21 | XSF import | atoms/cell/structure 생성 | 사용자 런타임 확인: 정상 작동 | PASS |
| 22 | CHGCAR import | atoms/cell/density 생성 | 사용자 런타임 확인: 정상 작동 | PASS |
| 23 | XSF Grid import | grid/atoms/cell warning 처리 | 사용자 런타임 확인: 정상 작동 | PASS |
| 24 | replace `No` | 기존 scene 유지 | 사용자 런타임 확인: 정상 작동 | PASS |
| 25 | replace `Yes` | 기존 scene 제거 + 새 scene import | 사용자 런타임 확인: 정상 작동 | PASS |
| 26 | invalid file | error popup + rollback | 사용자 런타임 확인: 정상 작동 | PASS |
| 27 | EventBus fanout | bonds/atoms/cell/measurement 구독자 반응 | 사용자 런타임 확인: 정상 작동 | PASS |
| 28 | Measurement on imported atoms | Phase 3.5 측정이 import 구조 위에서 동작 | Viewer 복구 필요 | BLOCKED |
| 29 | Data viewer on imported CHGCAR | Isosurface/Slice 에 loaded data 사용 | 사용자 런타임 확인: 정상 작동 | PASS |
| 30 | console error 0 | DevTools runtime error 0 | 사용자 런타임 확인: 정상 작동 | PASS |
| 31 | wasm size delta | Phase 3.5 release 대비 과도 증가 없음 | 현재 release `VTK-Workbench.wasm` 15,232,891B. Phase 3.5 baseline 은 미기록이나, 최신 명시 baseline Phase 3.3 14,996,023B 대비 +236,868B / +1.580% 로 정상 범위 | PASS |
| 32 | PR checklist | deviation 기록 | 본 평가서에 기록 | PASS |

## 2.3 종합 집계

| 구분 | 항목 수 | 항목 |
|---|---:|---|
| PASS | 29 | #1~#7, #9~#14, #16~#17, #18~#27, #29~#32 |
| PARTIAL | 2 | #8, #15 |
| BLOCKED | 1 | #28 |
| NOT VERIFIED | 0 | 없음 |
| FAIL | 0 | 없음 |

> 명시 FAIL 과 NOT VERIFIED 는 없다. Phase 3.6 DoD 의 대표 runtime import 시나리오는 사용자 확인으로 충족되었고, 남은 BLOCKED 는 Viewer 복구가 필요한 #28 뿐이다.

---

# Part 3 - 계획서 목표별 충족도

## 3.1 목표 달성 평가

| 목표 | 평가 | 근거 |
|---|---|---|
| File 메뉴 복구 | PASS | `app.cpp` hook 및 `file_menu.cpp` 메뉴 구현 |
| Open Recent placeholder | PASS | `ImGui::MenuItem("Open Recent", nullptr, false, false)` |
| parser 자동 선택 | PARTIAL | registry fallback + 직접 parser 병행. XSF/CHGCAR hot path registry 우회 |
| XSF 구조 반영 | PASS | `ApplyXsf` 가 cell/atoms 를 `structureRecords` 에 반영 |
| XSF Grid 반영 | PASS | `ParseXSFGridFile`, `ApplyXsfGrid`, `LoadXsfGridResult` 구현 |
| CHGCAR 구조 반영 | PASS | lattice, positions, species/count, density integration 구현 |
| UNV 경로 보존 | PASS | `ApplyUnv` 는 parser success 후 Phase 3.9 deferred warning 제공 |
| replace flow 보존 | PASS | replace modal, snapshot, rollback/complete 구현 및 런타임 정상 작동 확인 |
| progress UI 보존 | PASS | background parse + progress callback + percentage popup 구현 및 런타임 정상 작동 확인. 단 legacy side-by-side 는 미수행 |
| Embind ABI 복구 | PASS | File 관련 exported function 이 실함수로 연결 |
| EventBus 검증 | PASS | emit 구조 존재 및 런타임 정상 작동 확인 |
| 빌드 검증 | PASS | debug/release wasm build 통과 |
| 런타임 검증 | PASS/BLOCKED | #18~#27, #29~#30 런타임 정상 작동 확인. #28 은 Viewer 의존으로 BLOCKED |

## 3.2 비목표 준수 평가

| 비목표 | 평가 |
|---|---|
| Viewer 복구 제외 | 준수. Viewer 자체 복구 작업 없음 |
| full UNV mesh 표시 제외 | 준수. Phase 3.9 deferred 안내로 분리 |
| Model Tree 표시 제외 | 준수. `StructureRegistry` 기반만 제공 |
| Open Recent 활성화 제외 | 준수. disabled placeholder 유지 |
| 파일 저장/내보내기 제외 | 준수. export 기능 추가 없음 |
| legacy 수정 금지 | 준수. `webassembly/src/legacy` 변경 없음 |
| UI 재설계 금지 | 대체로 준수. popup 소유 위치만 feature UI 로 이동했으나 사용자-visible 문구 유지 |
| Phase 3.5 보강 구현 제외 | 일부 예외. Phase 3.6 사용자 보강 요청에 따라 progress와 XSF multi-grid 를 수정했으며, 이는 File 기능 범위 내로 판단 |

---

# Part 4 - 주요 구현 품질 평가

## 4.1 잘 된 점

| 항목 | 평가 |
|---|---|
| 책임 분리 | menu, controller, importer, UI, recent files 가 분리되어 legacy 단일 파일보다 추적성이 높다 |
| legacy ABI 보존 | 기존 JS/Embind 함수명을 유지하면서 stub 를 실구현으로 교체했다 |
| progress 복구 | legacy 와 유사하게 background thread parse 후 main runtime thread apply 흐름을 복구했다 |
| XSF Grid 다중 선택 | 하나의 XSF 내 여러 `DATAGRID_3D` 를 각각 Data entry 로 등록하고 `Mesh` combo 에 연결했다 |
| CHGCAR 중복 parse 방지 | parse result 를 Data feature 에 직접 주입하는 helper 를 추가했다 |
| replace transaction | snapshot/rollback/complete 로 실패 시 기존 scene 복구 경로를 마련했다 |
| EventBus 일관성 | `StructureRegistry::Register` 의 `onStructureAdded`, importer 의 cell/atoms change emit 경로가 분리되어 있다 |
| 빌드 안정성 | debug/release wasm build 가 모두 통과했다 |

## 4.2 구조적 편차와 해석

| # | 편차 | 영향 | 평가 |
|---|---|---|---|
| D1 | XSF/CHGCAR hot path 가 `FormatRegistry::Parse` 를 우회 | 계획서의 "format_registry 읽기 사용자" 의도와 일부 차이 | PARTIAL. progress callback 보존을 위한 현실적 선택 |
| D2 | progress popup 소유가 legacy `App` 이 아니라 `features/file` UI | 내부 소유권 차이 | 사용자-visible 문구와 타이밍을 유지하므로 허용 |
| D3 | Open Recent 는 storage 없이 skeleton | 실제 recent list 미제공 | 계획서 비목표와 일치 |
| D4 | UNV 는 parser routing 후 안내만 제공 | full mesh actor 미제공 | Phase 3.9 범위 분리와 일치 |
| D5 | #28 Measurement-on-imported-atoms 검증 보류 | Viewer 복구 전까지 측정 상호작용 확인 불가 | Phase 3.7 Viewer 복구 후 재검증 |

## 4.3 XSF Grid 보강 평가

| 항목 | 평가 |
|---|---|
| 다중 grid 파싱 | `XsfGridParseResult::grids` vector 로 복수 `DATAGRID_3D` 보존 |
| label 처리 | `BEGIN_DATAGRID_3D label`, `BEGIN_DATAGRID_3D_label`, 무명 `noname_01` 계열 처리 |
| Data entry 생성 | `LoadXsfGridResult` 가 각 grid 를 `ChargeDensity` 로 변환해 `SetNamedDataEntries` 로 전달 |
| UI 선택 | 기존 `ChargeDensityUI` 의 `Mesh` combo 를 통해 grid 선택 가능 |
| Slice 동기화 | grid 선택 시 active data clone 을 Slice controller 에 전달 |
| 남은 검증 | multi-grid XSF runtime 선택은 정상 작동 확인. legacy side-by-side visual 비교는 Viewer 안정화 후 보강 가능 |

## 4.4 progress 보강 평가

| 항목 | 평가 |
|---|---|
| legacy 구조 | `std::thread` parse + `emscripten_async_run_in_main_runtime_thread` apply 구조 복구 |
| XSF progress | `xsf_parser` 에 file position 기반 progress callback 추가 |
| CHGCAR progress | 기존 `ChgcarParser::parse(path, progressCallback)` 경로 활용 |
| popup 표시 | percentage text 포함 `ImGui::ProgressBar` 구현 |
| show/hide | JS file read 시작 시 show, parse/apply 완료 또는 error 시 hide |
| 남은 검증 | 브라우저에서 실제 대용량 파일 선택 시 progress bar 갱신 확인 필요 |

---

# Part 5 - 리스크와 권장 보강

## 5.1 주요 리스크

| # | 리스크 | 영향 | 권장 대응 |
|---|---|---|---|
| R1 | Viewer 미복구 또는 미검증 | measurement-on-imported-structure 검증 불가 | Phase 3.7 Viewer 복구 후 #28 재검증 |
| R2 | runtime 대표 파일은 통과했으나 edge sample 폭이 제한될 수 있음 | 특수 XSF/CHGCAR/XSF Grid 파일 형식 edge case 누락 가능 | 후속 회귀 샘플셋에 대형/무명 grid/비직교 cell 케이스 추가 |
| R3 | `FormatRegistry` 우회 | 장기적으로 parser routing 정책이 controller 에 분산될 수 있음 | `FormatRegistry::ParseWithProgress` 또는 progress callback-aware ParseFn 도입 |
| R4 | background thread lifetime | controller 정적 생명주기 전제에 의존 | import 중 shutdown/재진입 guard 보강 검토 |
| R5 | Phase 3.5 size baseline 없음 | 계획서 기준의 직접 baseline 비교는 불가 | 최신 명시 baseline Phase 3.3 대비 +1.580% 로 PASS 처리. 후속 phase 부터 release wasm size 를 매번 기록 |
| R6 | replace transaction edge case | 기본 Yes/No/invalid file 은 통과했으나 대형 파일/연속 import edge case 잔존 가능 | 회귀 샘플셋에 연속 import 및 실패 후 재import 추가 |
| R7 | XSF Grid value ordering edge case | XSF grid 렌더/슬라이스 값 방향 차이 가능 | legacy grid sample 로 iso/slice side-by-side 확인 |

## 5.2 후속 검증 우선순위

| 우선순위 | 항목 | 이유 |
|---:|---|---|
| 1 | Viewer 복구 후 Measurement on imported atoms 확인 | #28 BLOCKED 해소 |
| 2 | legacy side-by-side progress popup visual 비교 | #15 PARTIAL 보강 |
| 3 | `FormatRegistry::ParseWithProgress` 도입 검토 | #8 PARTIAL 구조 보강 |
| 4 | 대형/무명 grid/비직교 cell XSF 회귀 샘플 추가 | #23 통과 범위 확장 |
| 5 | 연속 import 및 실패 후 재import 회귀 시나리오 추가 | replace transaction edge case 보강 |
| 6 | 후속 phase release wasm size baseline 상시 기록 | #31 기준선 부재 재발 방지 |

---

# Part 6 - Definition of Done 충족도

| DoD 항목 | 상태 | 평가 |
|---|---|---|
| 신규 파일 | 충족 | `features/file/` 11파일 |
| legacy 격리 | 충족 | legacy 변경 및 include 없음 |
| 메뉴 | 충족 | 코드 hook 구현 및 runtime 표시 확인 |
| JS bridge | 충족 | file input + MEMFS + Embind callback 구현 |
| parser | 부분 충족 | parser 경로는 작동하나 XSF/CHGCAR hot path 는 registry 우회 |
| scene | 충족 | XSF/CHGCAR/XSF Grid structure/cell/atoms 반영 |
| data | 충족 | CHGCAR density, XSF Grid multi-entry 전달 |
| replace | 충족 | 코드 구현 및 runtime 확인 |
| UI | 충족/부분 | 문구/구조와 runtime 동작 확인. legacy side-by-side visual 비교는 미수행 |
| build | 충족 | debug/release 통과 |
| runtime | 충족 | #18~#27, #29~#30 사용자 확인으로 정상 작동 |
| blocked 분리 | 충족 | Viewer 의존 항목 BLOCKED 로 분리 |

## 최종 판단

| 판정 | 내용 |
|---|---|
| 종합 | **조건부 GO** |
| 근거 | 정적 구조, legacy 격리, 빌드, parser/data/import 골격, 대표 runtime 시나리오, console error 0, wasm size delta 가 모두 통과 |
| 조건 | Viewer 의존 #28 Measurement-on-imported-atoms 를 Phase 3.7 이후 재검증 |
| 다음 단계 | Phase 3.7 Viewer 복구로 넘어가되, #28 과 #8/#15 PARTIAL 보강을 후속 체크리스트로 유지 |

---

# Part 7 - 평가에 사용한 명령 요약

```powershell
git status --short --branch
git log -1 --oneline --decorate
rg --files webassembly\src\features\file
rg -n "legacy|src/legacy|#include.*legacy" webassembly\src\features\file
rg -n "features::file|file_menu" webassembly\src\app\app.cpp
rg -n "features/file" CMakeLists.txt
rg -n "handleStructureFile|loadChgcarFile|showProgressPopup|setProgressPopupText" webassembly\src\bind_function.cpp webassembly\src\features\file\file_menu.cpp
rg -n "RequestOpenStructureImport|\.xsf,.vasp,CHGCAR|createDataFile|handleStructureFile" webassembly\src\core\io\file_dialog.cpp
rg -n "chgcar|\.vasp|ContainsDatagrid3D|xsf_grid|ParseXSFGridFile" webassembly\src\core\io
rg -n "LoadChgcarParseResult|LoadXsfGridResult|FromChgcarParseResult|FromXsfGridData|SetNamedDataEntries" webassembly\src\features\data
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"
Get-Item public\wasm\VTK-Workbench.wasm
# current 15,232,891B vs latest documented baseline Phase 3.3 14,996,023B = +236,868B / +1.580%
```

---

# Part 8 - 후속 단계 연결

| 후속 phase | Phase 3.6 이 제공한 기반 | 남은 연결점 |
|---|---|---|
| Phase 3.7 Viewer + Toolbar | 외부 파일 import 로 실제 `SceneState` 구조 생성 가능 | imported structure actor 표시와 progress popup runtime 검증 |
| Phase 3.8 Model Tree | `StructureRegistry` 에 structure id/name 등록 | tree 표시, 선택 동기화, replace 후 tree cleanup |
| Phase 3.9 Mesh/UNV | `.unv` parser routing 및 deferred 안내 | full mesh actor import, mesh group UI, Model Tree 연동 |
| Phase 4 MenuRouter | File feature public API 정리 | `app.cpp` 직접 hook 을 Router registration 으로 이동 |
