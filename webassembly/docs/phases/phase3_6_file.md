# Phase 3.6 - File / Open Structure File 이식 세부계획서

> 상위 문서: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.6) + §6.0 공통 지침  
> 선행 문서: [`./phase3_5_measurement.md`](./phase3_5_measurement.md)  
> 선행 평가서: [`./phase3_5_evaluation_2026-05-11_v2.md`](./phase3_5_evaluation_2026-05-11_v2.md)  
> 메뉴 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §3 File  
> 메뉴 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §5  
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01~UI-12)  
> 작성일: 2026-05-11  
> 대상 브랜치: `refactor/menu-aligned`  
> 단위 PR: 1 개 (단일 sub-folder)  
> 예상 소요: 2~3 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-11 | 초안 작성 - Phase 3.5 완료 판단 및 v2 평가서의 BLOCKED/리스크 항목을 Phase 3.6 검증 정책에 반영 |

---

## 0. 한 줄 요약

> Phase 3.6 은 **File / Open Structure File** 을 `features/file/` 로 이식하여, Phase 2 의 `core/io/format_registry` 를 실제로 호출하는 **읽기 사용자**를 완성한다. Phase 3.2 가 `format_registry::RegisterDefaults` 로 파서 등록 경로를 열었다면, 본 단계는 XSF / XSF Grid / CHGCAR / UNV 입력을 `format_registry.Parse()` 로 판별하고, 성공한 구조 파일을 `core::scene::SceneState` 로 반영한다.
>
> 핵심 산출물은 `features/file/` 신규 약 11 파일, `core/io/file_dialog` 의 실제 JS file input bridge 복구, `bind_function.cpp` 의 File 관련 Embind stub 실구현 전환, `app/app.cpp` 의 File 메뉴 hook, `CMakeLists.txt` source 등록이다. legacy `FileLoader` 와 `AtomsTemplate` 은 수정하지 않고 참조만 한다.
>
> Phase 3.5 v2 평가서에서 Viewer 미복구로 #20~#25 가 BLOCKED 된 점은 본 단계의 중요한 전제다. Phase 3.6 은 Viewer 복구 자체를 범위로 삼지 않지만, 외부 파일로 생성된 실제 구조 위에서 Measurement / Data / Edit 기능이 작동할 수 있게 만드는 관문이다. 따라서 Viewer 가 아직 복구되지 않았으면 시각 검증 항목은 BLOCKED 로 분리하고, 구조 데이터와 EventBus 반영은 정적/런타임 상태 검증으로 완료한다.

---

# Part 1 - 목표 / 비목표

## 1.1 목표

| 구분 | 항목 |
|---|---|
| 메뉴 복구 | `File / Open Structure File` 클릭 시 legacy 와 동일하게 파일 선택 다이얼로그가 열린다. |
| placeholder 유지 | `File / Open Recent` 는 상위 매핑 문서대로 disabled placeholder 로 둔다. |
| parser 자동 선택 | `core/io/format_registry` 를 사용해 `.xsf`, `CHGCAR*`, `.vasp`, `.chgcar`, `.unv` 를 자동 판별한다. |
| XSF 구조 반영 | XSF `PRIMVEC` / `PRIMCOORD` 를 `SceneState::structureRecords` 의 cell + atoms 로 반영한다. |
| XSF Grid 반영 | XSF `DATAGRID_3D` 파일을 감지하고, grid payload 와 ATOMS payload 를 분리 반영한다. 단 grid rendering 은 Phase 3.2 Data API 와 연결 가능한 범위까지 수행한다. |
| CHGCAR 구조 반영 | CHGCAR lattice, atom species/count, direct/cartesian position, charge density 를 구조 + Data feature 상태로 반영한다. |
| UNV 경로 보존 | `.unv` 는 `format_registry` 경로를 통과시켜 parser selection 과 오류 없는 graceful handling 을 검증한다. full mesh actor import 는 Phase 3.9 `features/mesh` 범위로 명시 분리한다. |
| replace flow 보존 | 기존 scene data 가 있을 때 legacy 의 `"Clear Current Data and Import?"` 확인 모달, `Yes` / `No` 동작, deferred filename 정책을 보존한다. |
| progress UI 보존 | legacy 의 progress popup 표시 이름, 텍스트, show/hide timing 을 최대한 보존한다. |
| Embind ABI 복구 | `handleStructureFile`, `loadChgcarFile`, `handleXSFGridFile`, `writeChunk`, `closeFile`, `showProgressPopup`, `setProgressPopupText` 이름을 no-op stub 에서 실동작으로 전환한다. |
| EventBus 검증 | import 후 `onStructureAdded`, `onCellChanged`, `onAtomsChanged`, 필요 시 `onBondsChanged` 가 올바른 순서로 발행되어 Phase 3.4/3.5 구독자가 반응한다. |
| 빌드 검증 | debug/release wasm 빌드가 모두 통과한다. |
| 런타임 검증 | File 메뉴, 파일 선택, progress/error popup, XSF/CHGCAR import, replace flow, console error 0 을 검증한다. Viewer 미복구 시 시각 검증은 BLOCKED 로 기록한다. |

## 1.2 비목표

| 구분 | 항목 |
|---|---|
| Viewer 복구 | Viewer 창/툴바 복구는 Phase 3.7 `features/viewer` 범위다. |
| full UNV mesh 표시 | `.unv` 의 mesh actor 생성, mesh group UI, Model Tree 연동은 Phase 3.9 `features/mesh` 범위다. |
| Model Tree 표시 | import 된 구조의 tree 표시와 선택 동기화는 Phase 3.8 범위다. |
| Open Recent 활성화 | `Open Recent` 는 disabled placeholder 만 유지한다. 실제 recent list 저장/표시는 후속 phase 또는 Phase 4 이후 범위다. |
| 파일 저장/내보내기 | XSF/CHGCAR export, measurement export, mesh export 는 범위 밖이다. |
| legacy 수정 | `webassembly/src/legacy/` 내부 코드는 수정하지 않는다. 필요한 로직은 새 트리로 복사/재구성한다. |
| UI 재설계 | File 메뉴, progress popup, replace/error modal 의 라벨, 순서, 버튼명, 타이밍을 임의로 바꾸지 않는다. |
| Phase 3.5 보강 구현 | Measurement distance unit, atom pick metadata, drag 좌표계 개선은 본 단계에서 수정하지 않는다. 다만 파일 import 후 재검증 시나리오에 포함한다. |

## 1.3 Phase 3.5 v2 평가서 반영

| Phase 3.5 평가 항목 | 본 계획서 반영 |
|---|---|
| #17 debug build PASS | Phase 3.6 도 동일 명령으로 debug build 통과를 필수 검증한다. |
| #18 release build PASS | Phase 3.6 도 release build 통과를 필수 검증한다. |
| #19 Measurement menu runtime PASS | File 메뉴 runtime 확인도 동일 수준으로 수행한다. |
| #20~#25 Viewer 미복구 BLOCKED | 본 단계의 visual side-by-side, loaded structure visual, measurement-on-imported-structure 시나리오는 Viewer 상태에 따라 PASS 또는 BLOCKED 로 분리한다. |
| R1 Viewer 미복구 | Phase 3.6 구현 완료 기준에서 Viewer 복구를 요구하지 않는다. 단 평가서에는 BLOCKED 항목을 명확히 남긴다. |
| R2 pick position 기반 nearest atom | 외부 파일 import 후 atom id / position 데이터가 정확히 들어가야 pick 오차 분석이 가능하므로, atom id stability 를 검증 항목에 추가한다. |
| R3 drag selection 좌표계 미검증 | import 된 구조 위에서 drag selection 재검증 시나리오를 후속 visual gate 로 둔다. |
| R4 onAtomsChanged actor rebuild 비용 | import 시 `onAtomsChanged` 는 구조당 1회만 발행하도록 계획해 불필요한 rebuild 를 피한다. |
| R7 console/wasm size 미측정 | 본 단계는 console error 0 과 wasm size delta 를 명시 검증 항목에 포함한다. |

---

# Part 2 - 의의와 아키텍처 위치

## 2.1 `format_registry` 의 읽기 사용자 도달

Phase 3.2 는 `features/data/data_menu.cpp` 에서 `core::io::FormatRegistry::RegisterDefaults` 를 호출해 parser registration 경로를 검증했다. 그러나 실제 File 메뉴가 없었기 때문에 `FormatRegistry::Parse(path, out)` 호출 경로는 완전한 사용자 시나리오로 검증되지 않았다.

Phase 3.6 은 다음 흐름을 완성한다.

```text
File / Open Structure File
  -> core::io::FileDialog::RequestOpenStructureImport()
  -> JS file input + MEMFS createDataFile
  -> Embind handleStructureFile(fileName)
  -> features::file::StructureImportController
  -> core::io::FormatRegistry::Parse("/" + fileName, out)
  -> SceneState structureRecords 반영
  -> EventBus emit
  -> edit/data/measurement 구독자 반응
```

## 2.2 Phase 3.6 이 여는 전체 워크플로우

| 이전 phase | 현재까지 가능했던 것 | Phase 3.6 이후 가능해지는 것 |
|---|---|---|
| Phase 3.2 Data | CHGCAR UI와 renderer state 는 있으나 File 메뉴 없음 | File 메뉴로 CHGCAR 를 읽어 Data feature 에 실제 density 공급 |
| Phase 3.3 Build | Bravais / Periodic Table 로 구조 생성 가능 | 외부 파일 구조와 Build 기능의 공존/replace 확인 |
| Phase 3.4 Edit | 수동 생성 구조의 atoms/bonds/cell 편집 가능 | 파일로 로드한 구조의 atoms/bonds/cell 편집 가능 |
| Phase 3.5 Measurement | 메뉴와 측정 state 는 구현됨 | 파일로 로드한 원자 위에서 측정 시나리오 검증 가능 |
| Phase 3.8 Model Tree | 아직 미도착 | import 된 structure id/name 을 tree 가 나중에 그대로 읽을 수 있는 registry 기반 마련 |

## 2.3 회색지대 정책

| 회색지대 출처 | Phase 3.6 적용 |
|---|---|
| Phase 0 typo build-fix | legacy 내부 수정은 금지. 빌드 typo 가 새 트리에 있으면 최소 수정 허용. |
| Phase 1 Embind compatibility shim | 기존 JS/Next 쪽에서 호출하는 Embind 이름을 유지하되 no-op stub 를 실구현으로 교체한다. |
| Phase 2 core/io skeleton | `file_dialog`, `format_registry`, `xsf_parser` 의 미완성 부분을 본 단계에서 완성한다. 이는 core 보강이지만 Phase 3.6 의 본질이다. |
| Phase 3.1 임시 메뉴 hook | `app/app.cpp` 에 `features::file::DrawMenu()` / `RenderWindows()` / `InitOnce()` 를 임시 hook 으로 추가한다. Phase 4 에서 MenuRouter 로 수렴한다. |
| Phase 3.2 data API 보강 | CHGCAR parse result 를 Data feature 에 전달하기 위한 얇은 public helper 추가를 허용한다. |
| Phase 3.5 Viewer BLOCKED | Viewer 미복구는 본 단계 구현의 blocker 가 아니며, visual scenario 는 별도 BLOCKED 로 평가한다. |

---

# Part 3 - legacy UI 계약

본 Phase 의 UI 이식은 [`phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) 를 준수한다.

## 3.1 File 메뉴 계약

| 항목 | legacy 기준 | 새 트리 요구 |
|---|---|---|
| 메뉴 위치 | top menu 의 `File` | `Crystal Viewer`, `File`, `Edit`, `Build`, `Measurement`, `Data`, `Utilities` 순서로 배치 |
| 메뉴 항목 | `Open Structure File` | 라벨 동일 |
| Open Recent | disabled placeholder | disabled placeholder 유지 |
| 클릭 응답 | 즉시 file input 열기 또는 기존 데이터 확인 모달 | 동일 |
| Request type | `InvokeAction` | `features::file::OpenStructure()` 또는 `HandleRequest(OpenStructure)` |

## 3.2 파일 선택 다이얼로그 계약

| 항목 | legacy 기준 | 새 트리 요구 |
|---|---|---|
| `input.type` | `file` | 동일 |
| `multiple` | `false` | 동일 |
| `accept` | `.xsf,.vasp,CHGCAR*,*` | 동일 |
| 로딩 시작 텍스트 | `Loading structure file` | 동일 |
| progress body | `File(${file.name}) is loading, please wait...` | 동일 |
| MEMFS path | `/` + file.name | 동일 |
| JS success callback | `VtkModule.handleStructureFile(file.name)` | 동일 Embind 이름 유지 |
| JS error 처리 | console error + progress popup hide | 동일 |

## 3.3 modal / popup 계약

| popup | legacy 라벨/문구 | 새 트리 요구 |
|---|---|---|
| replace 확인 | `Clear Current Data and Import?` | 동일 |
| replace body 1 | `Viewer is not empty.` | 동일 |
| replace body 2 | `Do you want to clear all current data and continue importing?` | 동일 |
| replace buttons | `Yes`, `No` | 동일 순서 |
| import 실패 | `Import Structure Failed` | 동일 |
| import 실패 default body | `The selected structure file could not be imported.` | 동일 |
| XSF cell warning | `XSF Cell Warning` | XSF Grid parser 가 cell mismatch 를 감지하면 동일 문구 계열 사용 |

## 3.4 Intentional UI deviation 사전 등록

| 항목 | legacy | Phase 3.6 계획 | 사유 |
|---|---|---|---|
| UNV full actor import | legacy FileLoader 는 mesh actor 를 생성 | Phase 3.6 은 `.unv` parser routing + graceful message 까지만 보장 | `features/mesh` 와 Model Tree 가 아직 미도착. full mesh import 는 Phase 3.9 범위 |
| progress popup 소유 위치 | legacy `App` | 새 트리에서는 `features/file/structure_import_ui` 가 렌더 가능. 사용자에게 보이는 UI는 동일 | Phase 4 전 `App` 비대화를 피하고 File 기능 단위 응집 유지 |
| background parsing fallback | legacy 는 std::thread + main thread apply | 가능하면 동일 구현. pthread/main thread 제약 발생 시 synchronous fallback 을 허용하되 평가서에 명시 | 브라우저 pthread/Embind 호출 안정성 이슈 방어 |

---

# Part 4 - 대상 파일 구조

## 4.1 신규 파일

```text
webassembly/src/features/file/
├─ file_menu.cpp
├─ file_menu.h
├─ import_types.h
├─ recent_files.cpp
├─ recent_files.h
├─ structure_import.cpp
├─ structure_import.h
├─ structure_import_controller.cpp
├─ structure_import_controller.h
├─ structure_import_ui.cpp
└─ structure_import_ui.h
```

## 4.2 파일별 역할과 라인수 예상

| 파일 | 역할 | 예상 라인 |
|---|---|---:|
| `file_menu.{cpp,h}` | File 메뉴 그리기, InitOnce, HandleRequest, RenderWindows, Embind-facing wrapper | 140~190 |
| `import_types.h` | ImportKind, ImportOptions, ImportSummary, ImportError, ImportProgressState | 80~120 |
| `recent_files.{cpp,h}` | disabled placeholder 와 후속 확장용 in-memory skeleton | 40~80 |
| `structure_import.{cpp,h}` | ParseResult payload 를 SceneState 로 변환하는 순수 importer | 260~360 |
| `structure_import_controller.{cpp,h}` | file dialog 요청, replace transaction, background parse/apply, cleanup orchestration | 320~460 |
| `structure_import_ui.{cpp,h}` | progress popup, replace modal, error modal, warning modal 렌더링 | 160~240 |
| **합계** | **신규 11 파일** | **1,000~1,450** |

> legacy `file_loader.cpp` 는 1,400 라인 이상이지만 mesh/VTK/UNV full actor/background thread/legacy AtomsTemplate transaction 이 한 파일에 섞여 있다. Phase 3.6 은 structure import 에 필요한 부분만 분리하므로 신규 라인수는 약 1,000~1,450 으로 예상한다.

## 4.3 수정 파일

| 파일 | 수정 내용 |
|---|---|
| `CMakeLists.txt` | `features/file/` 신규 11 파일 source 등록 |
| `webassembly/src/app/app.cpp` | `features/file/file_menu.h` include, `InitOnce`, File 메뉴 hook, RenderWindows hook |
| `webassembly/src/bind_function.cpp` | File 관련 no-op stub 를 `features::file` / `core::io::FileDialog` 실함수로 교체 |
| `webassembly/src/core/io/file_dialog.{cpp,h}` | `RequestOpenStructureImport()` 의 JS file input bridge 구현 |
| `webassembly/src/core/io/format_registry.{cpp,h}` | `.vasp`, `CHGCAR*`, basename contains `chgcar`, XSF Grid detection helper 보강 |
| `webassembly/src/core/io/xsf_parser.{cpp,h}` | `DATAGRID_3D` 감지/파싱용 result struct 와 parse function 보강 |
| `webassembly/src/features/data/data_menu.{cpp,h}` | CHGCAR parse result 를 charge_density/slice controller 에 주입하는 얇은 public API 추가 |
| `webassembly/src/features/data/charge_density/charge_density.{cpp,h}` | `FromChgcarParseResult` helper 추가 |
| `webassembly/src/features/data/slice/slice_controller.{cpp,h}` | 필요 시 동일 charge density payload 재사용 helper 추가 |

---

# Part 5 - 구현 절차

## Step 0 - 사전 정적 조사

다음 항목을 구현 직전 다시 확인한다.

```powershell
rg -n "OpenStructureFile|RequestOpenStructureImport|handleStructureFile|LoadChgcarFile|RenderXsfGridImportPopups" webassembly\src\legacy webassembly\docs
rg -n "FormatRegistry|RegisterDefaults|Parse\(" webassembly\src\core webassembly\src\features
rg -n "showProgressPopup|setProgressPopupText|handleStructureFile|loadChgcarFile" webassembly\src\bind_function.cpp webassembly\src
rg -n "onStructureAdded|onCellChanged|onAtomsChanged|onStructureRemoved" webassembly\src\features webassembly\src\core
```

## Step 1 - `features/file` skeleton 작성

1. `file_menu.{cpp,h}` 작성.
2. `InitOnce(core::scene::SceneState&)` 에서 static controller/ui 를 생성한다.
3. `DrawMenu()` 에 `File` 메뉴를 추가한다.
4. `Open Structure File` 클릭 시 `StructureImportController::RequestOpenStructureImport()` 호출.
5. `Open Recent` 는 `ImGui::MenuItem("Open Recent", nullptr, false, false)` 로 disabled 처리.
6. `RenderWindows()` 에서 progress/replace/error/warning modal 을 렌더한다.
7. `Shutdown()` 은 outstanding import thread 가 있으면 join 또는 safe detach 종료 정책을 따른다.

## Step 2 - `core/io/file_dialog` JS bridge 복구

`FileDialog::RequestOpenStructureImport()` 를 legacy `FileLoader::OpenStructureFileBrowser()` 와 동일한 JS flow 로 구현한다.

```cpp
void FileDialog::RequestOpenStructureImport() {
    EM_ASM({
        let fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.multiple = false;
        fileInput.accept = '.xsf,.vasp,CHGCAR*,*';
        fileInput.onchange = () => {
            if (fileInput.files.length == 0) {
                return;
            }
            const file = fileInput.files[0];
            VtkModule.setProgressPopupText("Loading structure file", `File(${file.name}) is loading, please wait...`);
            VtkModule.showProgressPopup(true);
            const reader = new FileReader();
            reader.readAsArrayBuffer(file);
            reader.onload = () => {
                try {
                    const data = new Uint8Array(reader.result);
                    VtkModule.FS.createDataFile('/', file.name, data, true, false, true);
                    VtkModule.handleStructureFile(file.name);
                } catch (e) {
                    console.error("Structure file loading error", e);
                    VtkModule.showProgressPopup(false);
                }
            };
            reader.onerror = (e) => {
                console.error("File reading error", e);
                VtkModule.showProgressPopup(false);
            };
        };
        fileInput.click();
    });
}
```

> 실제 구현 시 include 순서, Emscripten header, lint style 은 기존 `app/app.cpp` 와 맞춘다.

## Step 3 - `format_registry` 읽기 경로 보강

1. `FormatRegistry::RegisterDefaults` 는 기존 `.xsf`, `.chgcar`, `.rho`, `.unv` 등록을 유지한다.
2. `.vasp` 를 CHGCAR parser 로 등록한다.
3. extension 이 없거나 생략된 경우 basename 을 검사한다.
4. legacy 와 동일하게 `fileLower.find("chgcar") != npos` 를 지원한다.
5. `.xsf` 에서 `DATAGRID_3D` 가 감지되면 `parserId = "xsf_grid"` 로 분기한다.
6. registry parse 실패 시 `ParseResult.errorMessage` 는 UI popup 에 그대로 표시 가능해야 한다.

권장 parse id:

| parserId | 입력 | payload type |
|---|---|---|
| `xsf` | 일반 XSF 구조 | `core::io::XsfParseResult` |
| `xsf_grid` | XSF DATAGRID_3D | `core::io::XsfGridParseResult` |
| `chgcar` | CHGCAR / `.vasp` / `.chgcar` | `core::io::ChgcarParser::ParseResult` |
| `rho` | `.rho` | `core::io::RhoParseResult` |
| `unv` | `.unv` | `core::io::UnvParseResult` |

## Step 4 - XSF Grid parser 보강

현재 `core/io/xsf_parser` 는 일반 XSF 구조만 처리한다. Phase 3.6 에서 다음을 추가한다.

| 항목 | 내용 |
|---|---|
| `ContainsDatagrid3D(path)` | legacy `containsDatagrid3d` 와 동일 역할 |
| `XsfGridData` | label, dims, origin, vectors, values |
| `XsfGridParseResult` | success, grids, atoms, hasCellVectors, cellVectorsConsistent, errorMessage |
| `ParseXSFGridFile(path)` | legacy `FileIOManager::parse3DGridXSFFile` 의 새 트리 버전 |
| downsample 정책 | Phase 3.6 에서는 high grid 필수, medium/low 는 가능하면 유지. renderer quality 세부 최적화는 Data feature 정책에 맞춘다. |

XSF Grid 의 ATOMS payload 는 일반 XSF 와 같은 `SceneState` import 경로를 재사용한다.

## Step 5 - `StructureImporter` 작성

`structure_import.{cpp,h}` 는 UI/JS/스레드 책임을 갖지 않는다. 입력 payload 를 `SceneState` 에 반영하는 순수 변환 책임만 갖는다.

### 5.1 공통 structure 생성

1. 새 structure id 를 계산한다.
2. `scene.structures.Register(id, displayName)` 를 호출한다.
3. `scene.currentStructureId = id` 로 설정한다.
4. `scene.structureRecords[id]` 를 생성한다.
5. file name 은 `StructureRegistry::StructureEntry::name` 에 저장한다.
6. `scene.selection.Clear()` 또는 동등한 reset 을 수행한다.

중요: `StructureRegistry::Register` 가 `onStructureAdded` 를 이미 emit 하므로, 동일 이벤트를 수동으로 중복 emit 하지 않는다.

### 5.2 XSF 적용

1. `XsfParseResult.latticeVectors` 를 `UnitCellRecord.matrix` 로 복사한다.
2. `cell.hasCell = true`, `cell.visible = true` 로 설정한다.
3. atom 마다 `AtomRecord` 를 생성한다.
4. `AtomRecord.id = scene.nextAtomId++`.
5. `symbol`, `cartesian`, `radius`, `visible`, `group = "Default"` 를 설정한다.
6. cell 이 있으면 fractional 좌표를 계산한다.
7. 모든 atom 을 한 번에 push 한 뒤 `onCellChanged` 1회, `onAtomsChanged` 1회 emit 한다.

### 5.3 CHGCAR 적용

1. `ChgcarParser::ParseResult.lattice` 를 cell matrix 로 반영한다.
2. `elements` + `atomCounts` + `positions` 를 순회한다.
3. `isDirect == true` 이면 direct 좌표를 cartesian 으로 변환한다.
4. `isDirect == false` 이면 positions 를 cartesian 으로 사용하고 fractional 을 역변환한다.
5. density 가 비어 있지 않으면 Data feature 에 parse result 를 전달한다.
6. `onCellChanged` 1회, `onAtomsChanged` 1회 emit 한다.

### 5.4 XSF Grid 적용

1. grid payload 를 Data feature 에 전달한다.
2. ATOMS payload 가 있으면 일반 XSF atom import 경로를 재사용한다.
3. cell vector 가 block 간 불일치하면 cell rendering 을 생략하고 warning popup 을 표시한다.
4. cell vector 가 일관되면 `cell.hasCell = true` 로 설정한다.

### 5.5 UNV 적용

1. `UnvParseResult` 가 success 이면 parser routing 은 PASS 로 기록한다.
2. full mesh import 는 수행하지 않는다.
3. 사용자에게는 `UNV mesh import will be completed in Phase 3.9.` 성격의 안내를 error 가 아닌 deferred message 로 보여준다.
4. 평가서에는 상위 계획서의 UNV 언급과 Phase 3.9 mesh 범위 사이의 intentional scope split 로 기록한다.

## Step 6 - `StructureImportController` 작성

### 6.1 주요 public API

```cpp
class StructureImportController {
public:
    explicit StructureImportController(core::scene::SceneState& scene);

    void RequestOpenStructureImport();
    void HandleStructureFile(const std::string& fileName);
    void HandleXsfGridFile(const std::string& fileName);
    void LoadChgcarFile(const std::string& fileName);
    void ProcessFileInBackground(const std::string& fileName, bool deleteFile);

    void ShowProgressPopup(bool show);
    void SetProgressPopupText(std::string title, std::string text);
    void RenderPopups();
};
```

### 6.2 replace transaction

| 상태 | 의미 |
|---|---|
| `showReplacePopup_` | replace 확인 modal open 예약 |
| `replaceSceneOnNextImport_` | 다음 import 는 기존 scene clear 후 진행 |
| `deferredFileName_` | 이미 파일은 MEMFS 에 올라왔지만 replace 확인 전인 파일 |
| `transactionActive_` | replace 중 실패 시 rollback 이 필요한 상태 |
| `preImportSnapshot_` | 최소 rollback 을 위한 structure ids/current id/next ids snapshot |

replace 정책:

1. scene 이 비어 있으면 즉시 file dialog 를 연다.
2. scene 이 비어 있지 않으면 먼저 replace popup 을 띄운다.
3. `Yes` 이면 progress popup 을 띄우고 file dialog 또는 deferred file import 를 진행한다.
4. `No` 이면 deferred MEMFS 파일을 삭제하고 progress popup 을 닫는다.
5. replace 중 성공하면 기존 structures 를 제거하고 새 structure 만 남긴다.
6. replace 중 실패하면 기존 scene snapshot 을 최대한 복원한다.

## Step 7 - Data feature 얇은 integration API

CHGCAR 와 XSF Grid 는 Data feature 와 연결되어야 한다. 다음 중 A 를 기본으로 한다.

| 옵션 | 설명 | 채택 |
|---|---|---|
| A | `features::data::LoadChgcarParseResult(name, parsed)` public helper 추가 | 권장 |
| B | File feature 가 `ChargeDensityController` 를 직접 include/생성 | 비권장. controller 중복 상태 위험 |
| C | CHGCAR 를 structure atoms 만 import 하고 density 는 무시 | 비권장. legacy 기능 손실 |

권장 public helper:

```cpp
namespace features::data {
bool LoadChgcarParseResult(const std::string& name,
                           const core::io::ChgcarParser::ParseResult& parsed);
bool LoadXsfGridResult(const std::string& name,
                       const core::io::XsfGridParseResult& parsed);
}
```

`charge_density::ChargeDensity` 에는 `FromChgcarParseResult` 를 추가해 중복 parsing 을 피한다.

## Step 8 - Embind 실함수 연결

| Embind 이름 | Phase 1~3.5 상태 | Phase 3.6 연결 |
|---|---|---|
| `loadArrayBuffer` | no-op stub | compatibility 유지. XSF direct route 또는 FileDialog buffer load 로 연결 |
| `loadChgcarFile` | no-op stub | `features::file::LoadChgcarFile` |
| `handleXSFGridFile` | no-op stub | `features::file::HandleXsfGridFile` |
| `handleStructureFile` | no-op stub | `features::file::HandleStructureFile` |
| `writeChunk` | no-op stub | `core::io::FileDialog::WriteChunk` |
| `closeFile` | no-op stub | `core::io::FileDialog::CloseFile` |
| `processFileInBackground` | no-op stub | `features::file::ProcessFileInBackground` |
| `showProgressPopup` | no-op stub | `features::file::ShowProgressPopup` |
| `setProgressPopupText` | no-op stub | `features::file::SetProgressPopupText` |

주의:

- Embind symbol 이름은 변경하지 않는다.
- JS/Next 쪽에서 이미 호출할 수 있으므로 함수 삭제는 금지한다.
- full mesh path 가 없는 `loadArrayBuffer` 는 최소한 extension 확인 후 graceful message 를 남겨야 한다.

## Step 9 - app / CMake hook

`app/app.cpp`:

```cpp
#include "../features/file/file_menu.h"

features::file::InitOnce(g_sceneState);

features::file::DrawMenu();
features::file::RenderWindows();
```

메뉴 순서는 상위 `02_menu_tree.md` 와 맞춘다.

```text
Crystal Viewer (rebuilding...)
File
Edit
Build
Measurement
Data
Utilities
```

현재 `app.cpp` 의 점진 hook 순서가 상위 메뉴 트리와 다르면, Phase 3.6 에서 File hook 추가와 함께 legacy 순서(`Crystal Viewer`, `File`, `Edit`, `Build`, `Measurement`, `Data`, `Utilities`) 로 정렬한다. 메뉴 순서 차이는 사용자-visible UI drift 이므로 Intentional UI deviation 으로 남기지 않는 것이 원칙이다.

`CMakeLists.txt`:

```cmake
# Phase 3.6: File / Open Structure File
webassembly/src/features/file/file_menu.cpp
webassembly/src/features/file/file_menu.h
...
```

---

# Part 6 - 검증 시나리오

## 6.1 대표 시나리오

| ID | 시나리오 | 기대 결과 |
|---|---|---|
| S1 | 빈 scene 에서 일반 XSF import | structure 1개, cell 1개, atoms N개 생성, `onStructureAdded/onCellChanged/onAtomsChanged` 발행 |
| S2 | 기존 scene 있음 + Open Structure File + `No` | 기존 scene 유지, deferred file 삭제, progress popup hide |
| S3 | 기존 scene 있음 + Open Structure File + `Yes` | 기존 structures 제거, 새 structure import, measurement/data stale state 제거 |
| S4 | CHGCAR import | atoms/cell 생성, charge density data 가 Data feature 에 로드 |
| S5 | XSF Grid import | grid data 로드, ATOMS payload 있으면 atoms/cell 생성, cell mismatch warning 동작 |
| S6 | invalid file import | `Import Structure Failed` popup, scene rollback |
| S7 | `.unv` import | `format_registry` 가 `.unv` parser 를 선택하고, full mesh import deferred 안내 표시 |
| S8 | XSF import 후 Measurement Distance | Viewer 복구 시 atom pick 으로 distance 생성. Viewer 미복구 시 BLOCKED 로 기록 |
| S9 | CHGCAR import 후 Data / Isosurface | Viewer 복구 시 isosurface actor 표시. Viewer 미복구 시 Data state 로드까지만 확인 |
| S10 | import 후 Edit / Atoms / Bonds / Cell | UI state 가 imported structure 를 읽고, bonds recompute 가 1회만 발생 |

## 6.2 Phase 3.5 carry-over 시나리오

Phase 3.5 자체는 완료로 간주하되, Phase 3.6 평가서에는 다음 carry-over 검증을 별도 표로 둔다.

| 항목 | 실행 조건 | 판정 정책 |
|---|---|---|
| 파일 import 후 measurement 생성 | Viewer 복구 필요 | 가능하면 PASS/FAIL, 불가하면 BLOCKED |
| file import 후 atom 삭제 시 measurement prune | Viewer 복구 및 Edit runtime 필요 | 가능하면 PASS/FAIL, 불가하면 BLOCKED |
| replace import 시 measurement store cleanup | Viewer 불필요. store/list 상태로 확인 가능 | Phase 3.6 에서 확인 권장 |
| console error 0 | Viewer 여부와 무관 | Phase 3.6 필수 |
| wasm size delta | Viewer 여부와 무관 | Phase 3.6 필수 |

---

# Part 7 - 검증 매트릭스

| # | 검증 항목 | 기준 | 방법 | 판정 |
|---:|---|---|---|---|
| 1 | 신규 파일 수 | `features/file/` 11 파일 | `rg --files webassembly/src/features/file` | 정적 |
| 2 | legacy 수정 없음 | `webassembly/src/legacy/` 변경 0 | `git status --short webassembly/src/legacy` | 정적 |
| 3 | legacy include 없음 | `features/file` 에 legacy include 0 | `rg -n "legacy|src/legacy|#include.*legacy" webassembly/src/features/file` | 정적 |
| 4 | File menu hook | `app.cpp` 에 include/init/draw/render hook | `rg -n "features::file|file_menu" webassembly/src/app/app.cpp` | 정적 |
| 5 | CMake 등록 | 신규 `.cpp/.h` 등록 | `rg -n "features/file" CMakeLists.txt` | 정적 |
| 6 | Embind 실함수 | File 관련 stub 제거 또는 real call | `rg -n "stub_.*Structure|stub_loadChgcar|stub_showProgress" webassembly/src/bind_function.cpp` | 정적 |
| 7 | FileDialog bridge | `RequestOpenStructureImport` 에 `EM_ASM` + accept string | `rg -n "RequestOpenStructureImport|\\.xsf,.vasp,CHGCAR" webassembly/src/core/io/file_dialog.cpp` | 정적 |
| 8 | FormatRegistry read path | `Parse` 호출자 1+ | `rg -n "FormatRegistry.*Parse|\\.Parse\\(" webassembly/src/features/file` | 정적 |
| 9 | CHGCAR basename matching | basename contains `chgcar` 또는 `.vasp` 처리 | `rg -n "chgcar|\\.vasp" webassembly/src/core/io/format_registry.cpp` | 정적 |
| 10 | XSF Grid parser | `XsfGridParseResult` + `ParseXSFGridFile` | `rg -n "XsfGrid|DATAGRID_3D|ParseXSFGrid" webassembly/src/core/io` | 정적 |
| 11 | SceneState import | `structureRecords` cell/atoms 반영 | `rg -n "structureRecords|nextAtomId|onAtomsChanged|onCellChanged" webassembly/src/features/file` | 정적 |
| 12 | 중복 structure event 방지 | `structures.Register` 이후 `onStructureAdded.Emit` 중복 없음 | 코드 리뷰 | 정적 |
| 13 | Data integration | `LoadChgcarParseResult` 또는 동등 helper | `rg -n "LoadChgcarParseResult|FromChgcarParseResult|LoadXsfGridResult" webassembly/src/features/data webassembly/src/features/file` | 정적 |
| 14 | replace popup UI | legacy modal title/body/button 동일 | side-by-side 또는 코드 grep | UI |
| 15 | progress popup UI | title/body/show-hide 동일 | 런타임 | UI |
| 16 | debug build | wasm debug build PASS | `cmd /c "..\\emsdk\\emsdk_env.bat && npm.cmd run build-wasm:debug"` | 빌드 |
| 17 | release build | wasm release build PASS | `cmd /c "..\\emsdk\\emsdk_env.bat && npm.cmd run build-wasm:release"` | 빌드 |
| 18 | File menu 표시 | `File / Open Structure File` 표시 | 런타임 | 동적 |
| 19 | Open Recent disabled | disabled placeholder 표시 | 런타임 | 동적 |
| 20 | file input open | accept string 정상 | 런타임 | 동적 |
| 21 | XSF import | atoms/cell/structure 생성 | 런타임 + state log | 동적 |
| 22 | CHGCAR import | atoms/cell/density 생성 | 런타임 + Data UI | 동적 |
| 23 | XSF Grid import | grid/atoms/cell warning 처리 | 런타임 | 동적 |
| 24 | replace `No` | 기존 scene 유지 | 런타임 | 동적 |
| 25 | replace `Yes` | 기존 scene 제거 + 새 scene import | 런타임 | 동적 |
| 26 | invalid file | error popup + rollback | 런타임 | 동적 |
| 27 | EventBus fanout | bonds/atoms/cell/measurement 구독자 반응 | runtime log 또는 visual | 동적 |
| 28 | Measurement on imported atoms | Phase 3.5 기능이 import 구조 위에서 동작 | Viewer 필요. 미복구 시 BLOCKED | 동적 |
| 29 | Data viewer on imported CHGCAR | Isosurface/Slice 에 loaded data 사용 | Viewer 필요. 미복구 시 PARTIAL/BLOCKED | 동적 |
| 30 | console error 0 | DevTools console 에 runtime error 0 | 수동 확인 | 동적 |
| 31 | wasm size delta | Phase 3.5 release artifact 대비 과도 증가 없음 | artifact size 비교 | 정량 |
| 32 | PR checklist | Intentional deviation 기록 | PR 본문/평가서 | 리뷰 |

---

# Part 8 - 리스크와 대응

| # | 리스크 | 영향 | 대응 |
|---|---|---|---|
| R1 | Viewer 미복구 지속 | import 결과의 시각 확인, measurement-on-imported-structure 검증 불가 | state/event/build/menu 는 완료하고 visual 항목은 BLOCKED 로 명확히 분리 |
| R2 | XSF Grid parser 가 Phase 2 skeleton 에 없음 | XSF Grid import 누락 가능 | legacy `FileIOManager::parse3DGridXSFFile` 를 새 `core/io/xsf_parser` 로 이식 |
| R3 | `std::any` payload cast 오류 | parse 성공 후 apply 실패 | parserId 와 payload type 을 중앙 표로 고정하고 `std::any_cast` 실패 시 error popup |
| R4 | CHGCAR 를 File 과 Data 가 중복 parse | 대형 파일에서 성능/메모리 낭비 | `ChargeDensity::FromChgcarParseResult` 추가 |
| R5 | background thread + main thread apply 불안정 | 브라우저 runtime hang 또는 race | legacy 패턴을 따르되, thread unavailable 시 synchronous fallback 을 명시 deviation 으로 기록 |
| R6 | replace transaction rollback 불완전 | 실패 후 scene state 손상 | import 전 snapshot 최소화, replace 성공 전 기존 scene 제거 지연, 실패 popup 후 cleanup |
| R7 | `onAtomsChanged` 과다 emit | atom renderer/bond/measurement rebuild 비용 증가 | bulk import 후 structure 당 1회 emit |
| R8 | UNV 범위 충돌 | 상위 계획의 File/UNV 와 Phase 3.9 mesh 범위가 겹침 | Phase 3.6 은 parser routing + deferred 안내, Phase 3.9 에 full mesh actor 명시 |
| R9 | App progress popup 과 feature progress popup 소유권 차이 | UI deviation 논쟁 | 사용자-visible 라벨/타이밍 동일 유지, 소유권 차이는 문서화 |
| R10 | Menu order drift | UI 1:1 보존 위반 | File 메뉴를 legacy order 에 맞게 앞쪽에 배치 |

---

# Part 9 - Definition of Done

Phase 3.6 은 다음 조건을 만족하면 완료로 본다.

| 항목 | 완료 기준 |
|---|---|
| 구조 | `features/file/` 신규 파일이 계획된 책임 분리로 작성됨 |
| legacy 격리 | legacy 내부 수정 0, features/file 의 legacy include 0 |
| 메뉴 | File 메뉴와 Open Structure File 항목이 runtime 에 표시됨 |
| Embind | File 관련 JS callback 이름이 유지되고 no-op 이 아님 |
| parser | `format_registry.Parse` 로 XSF/XSF Grid/CHGCAR/UNV 경로가 분기됨 |
| scene | XSF/CHGCAR import 가 `SceneState` 에 structure/cell/atoms 를 생성함 |
| data | CHGCAR density 가 Data feature 로 전달됨 |
| event | import 후 cell/atoms/bonds/measurement 관련 EventBus fanout 이 정적으로 확인됨 |
| UI | replace/progress/error popup 의 label/order/timing 이 legacy 와 동일함 |
| build | debug/release wasm 빌드 모두 통과 |
| runtime | XSF/CHGCAR 대표 import 시나리오 통과 |
| blocked 분리 | Viewer 미복구로 수행 불가한 visual 항목은 FAIL 이 아니라 BLOCKED 로 평가서에 분리 |

---

# Part 10 - PR 체크리스트

작성자:

- [ ] Phase 3.5 가 완료 기준선임을 PR 본문에 명시
- [ ] Phase 3.5 v2 평가서의 Viewer BLOCKED 항목을 Phase 3.6 평가 정책에 반영
- [ ] `features/file/` 신규 파일 11개 추가
- [ ] `app/app.cpp` 에 File InitOnce/DrawMenu/RenderWindows hook 추가
- [ ] `CMakeLists.txt` 에 신규 source 등록
- [ ] `bind_function.cpp` 의 File 관련 no-op stub 실구현 전환
- [ ] `core/io/file_dialog` JS bridge 복구
- [ ] `core/io/format_registry` CHGCAR basename / `.vasp` / XSF Grid 분기 보강
- [ ] `core/io/xsf_parser` XSF Grid parser 보강
- [ ] Data feature 에 CHGCAR parse result injection helper 추가
- [ ] bulk import 후 EventBus emit 은 structure 당 최소 횟수로 제한
- [ ] replace/progress/error modal label 이 legacy 와 동일
- [ ] `legacy/` 변경 0
- [ ] debug/release wasm 빌드 통과
- [ ] XSF/CHGCAR runtime import 확인
- [ ] Viewer 미복구 시 visual 항목은 BLOCKED 로 명시
- [ ] Intentional UI deviation 섹션 작성. 없으면 `None`

검토자:

- [ ] diff 가 File feature + 필요한 core/io/app/bind/data 얇은 integration 으로 제한되어 있는가?
- [ ] `features/file` 이 legacy 를 include 하지 않는가?
- [ ] File menu label/order 가 legacy 와 같은가?
- [ ] replace/progress/error popup 이 side-by-side 로 동일한가?
- [ ] XSF/CHGCAR import 후 SceneState 에 atom/cell 이 들어오는가?
- [ ] EventBus 중복 emit 이 없는가?
- [ ] CHGCAR density 가 Data feature 에 중복 parse 없이 전달되는가?
- [ ] UNV deferred 안내가 실패처럼 보이지 않게 문서화되었는가?
- [ ] Viewer 미복구로 BLOCKED 된 항목과 실제 FAIL 을 혼동하지 않았는가?

---

# Part 11 - 부록

## 11.1 검증 명령 요약

```powershell
rg --files webassembly\src\features\file
rg -n "legacy|src/legacy|#include.*legacy" webassembly\src\features\file
rg -n "features::file|file_menu" webassembly\src\app\app.cpp
rg -n "features/file" CMakeLists.txt
rg -n "stub_.*Structure|stub_loadChgcar|stub_showProgress" webassembly\src\bind_function.cpp
rg -n "RequestOpenStructureImport|handleStructureFile|loadChgcarFile|handleXSFGridFile" webassembly\src
rg -n "FormatRegistry.*Parse|RegisterDefaults|CHGCAR|\\.vasp|DATAGRID_3D" webassembly\src\core\io webassembly\src\features\file
rg -n "onStructureAdded|onCellChanged|onAtomsChanged|onBondsChanged|onStructureRemoved" webassembly\src\features\file webassembly\src\features\edit webassembly\src\features\measurement
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"
```

## 11.2 후속 단계 연결

| 후속 phase | Phase 3.6 이 제공하는 기반 |
|---|---|
| Phase 3.7 viewer + toolbar | 외부 파일로 생성된 실제 구조를 Viewer/Toolbar 복구 후 즉시 시각 검증 가능 |
| Phase 3.8 model_tree | `StructureRegistry` 에 등록된 import name/id 를 tree 가 표시 가능 |
| Phase 3.9 mesh | `.unv` parser routing 과 File menu entry 를 full mesh actor import 로 확장 가능 |
| Phase 4 menu_router | `features::file::DrawMenu/HandleRequest/RenderWindows` 를 정식 router 로 이전 가능 |
| Phase 5 legacy 제거 | legacy `FileLoader` 의 구조 import 의존이 새 트리로 이동되어 삭제 준비 |

## 11.3 관련 문서

- 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6 Phase 3 (3.6)
- 메뉴 매핑: [`../04_menu_to_code_mapping.md`](../04_menu_to_code_mapping.md) §3 File
- 메뉴 체크리스트: [`../02_menu_tree.md`](../02_menu_tree.md) §5
- 대상 아키텍처: [`../03_target_architecture.md`](../03_target_architecture.md) §3~§4
- UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md)
- 선행 계획서: [`./phase3_5_measurement.md`](./phase3_5_measurement.md)
- 선행 평가서: [`./phase3_5_evaluation_2026-05-11_v2.md`](./phase3_5_evaluation_2026-05-11_v2.md)
