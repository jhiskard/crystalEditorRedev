# 01. 현재 구조 분석

> 분석 대상: `webassembly/src/` (CMake/리소스/dependencies 제외)
> 분석 시점: 2026-04-27
> 측정 지표: 파일 트리 + 라인 수 + 의존 경계

## 1. 디렉터리 트리 (요약)

```
webassembly/src/
├─ main.cpp ............................... 178 LOC   (Emscripten main)
├─ bind_function.cpp ....................... 27 LOC   (Embind 바인딩)
├─ app.cpp/h ........................... 1,181 + 151  (도크스페이스 + 메인 메뉴)
├─ vtk_viewer.cpp/h .................... 2,690 + 177  (VTK 뷰어 + 인터랙터 + 윈도우)
├─ mouse_interactor_style.cpp/h .......... 120 + 15
├─ toolbar.cpp/h ........................ 546 + 45    (Viewer 위 ImGui 툴바)
├─ custom_ui.cpp/h ....................... 13 + 12    (공용 ImGui 헬퍼 placeholder)
├─ font_manager.cpp/h ............... 33,885 + 84    (폰트 임베딩 — 자동생성)
├─ image.cpp/h ........................... 32 + 31
├─ texture.cpp/h ......................... 70 + 37
├─ lcrs_tree.cpp/h ...................... 296 + 90    (Left-Child Right-Sibling 트리 자료구조)
├─ file_loader.cpp/h ................. 1,448 + 117    (브라우저 파일 다이얼로그 + XSF/CHGCAR/UNV 분기)
├─ unv_reader.cpp/h .................... 322 + 76     (UNV 메시 파서)
├─ mesh.cpp/h ........................ 1,011 + 218
├─ mesh_detail.cpp/h ................. 1,260 + 189
├─ mesh_group.cpp/h .................... 117 + 61
├─ mesh_group_detail.cpp/h ............. 296 + 40
├─ mesh_manager.cpp/h .................. 495 + 102
├─ model_tree.cpp/h .................. 1,956 + 38     (Model Tree UI — 다른 개발자 추가)
├─ test_window.cpp/h ................... 118 + 16
├─ common/{string_utils, colormap}
├─ enum/        (app_enums, font_icon_enums, mesh_group_enum, toolbar_enums, tree_enums, viewer_enums)
├─ icon/        (CodIcons, FontAudio, FontAwesome*, ForkAwesome, Kenney, Lucide, MaterialDesign*, MaterialSymbols)
├─ macro/       (ptr_macro, singleton_macro)
├─ config/      (log_config)
├─ atoms_template_bravais_lattice.cpp ........ 475 LOC  (atoms_template 분리 조각)
├─ atoms_template_periodic_table.cpp ......... 523 LOC  (atoms_template 분리 조각)
└─ atoms/
   ├─ atoms_template.cpp/h ............. 6,876 + 1,403  ★ God Object orchestrator
   ├─ domain/
   │  ├─ atom_manager.cpp/h ............... 454 + 250
   │  ├─ bond_manager.cpp/h ............... 900 + 165
   │  ├─ bz_plot.cpp/h .................... 533 + 216
   │  ├─ cell_manager.cpp/h ............... 137 + 45
   │  ├─ charge_density.cpp/h ............. 242 + 159
   │  ├─ color.h .......................... 20
   │  ├─ crystal_structure.cpp/h .......... 361 + 125
   │  ├─ crystal_system.cpp/h ............. 145 + 69
   │  ├─ element_database.cpp/h ........... 645 + 273   (118 elements + Jmol/CPK 색)
   │  ├─ special_points.h ................. 615
   │  └─ surrounding_atom_manager.cpp/h ... 389 + 20
   ├─ infrastructure/
   │  ├─ batch_update_system.cpp/h ........ 164 + 127
   │  ├─ bond_renderer.cpp/h .............. 42 + 34
   │  ├─ bz_plot_layer.cpp/h .............. 181 + 153
   │  ├─ charge_density_renderer.cpp/h .. 1,092 + 133
   │  ├─ chgcar_parser.cpp/h .............. 422 + 60
   │  ├─ file_io_manager.cpp/h ............ 847 + 223   (XSF + 통합 파서 매니저)
   │  ├─ rho_file_parser.h ................ 22
   │  ├─ slice_renderer.h ................. 22
   │  └─ vtk_renderer.cpp/h ............ 1,792 + 435    (atoms 전용 거대 렌더러)
   └─ ui/
      ├─ atom_editor_ui.cpp/h ............. 896 + 46
      ├─ atoms_template_main_window_ui.cpp/h .. 77 + 51
      ├─ bond_ui.cpp/h .................... 220 + 41
      ├─ bravais_lattice_ui.cpp/h ......... 599 + 177
      ├─ bz_plot_ui.cpp/h ................. 628 + 52
      ├─ cell_info_ui.cpp/h ............... 101 + 43
      ├─ charge_density_ui.cpp/h ........ 2,519 + 318
      ├─ periodic_table_ui.cpp/h .......... 453 + 148
      └─ ui_color_utils.h .................. 32
```

> 합계는 `font_manager.cpp` 의 자동생성 라인을 빼면 대략 **41,000 LOC** 수준이다.

## 2. 계층(intended) — atoms 모듈 내부

`atoms/` 는 한 차례 리팩터링을 거쳐 다음 4-layer 구조를 가지고 있다 (`refactory_atom_manager.md` / `refactory5.md` 참고).

```
        ┌─────────────────────────────────────────┐
        │  Orchestration (atoms_template.h/cpp)   │  ← UI/Domain/Infra glue
        └─────────────────────────────────────────┘
              ↑                ↑              ↑
        ┌──────────┐   ┌──────────────┐   ┌────────────┐
        │   UI     │   │   Domain     │   │ Infrastructure │
        │ (ImGui)  │   │ (pure logic) │   │ (VTK/파일 IO)  │
        └──────────┘   └──────────────┘   └────────────┘
```

- **Domain**: VTK 의존성이 거의 없는 순수 데이터/로직 (원자/결합/셀/원소 DB/Brillouin Zone 수학).
- **Infrastructure**: VTK 액터 생성, 렌더러, 파일 I/O, 배치 업데이트 시스템.
- **UI**: ImGui 위젯/창. 원자 에디터, 결합 패널, 주기율표, Bravais 템플릿 등.
- **Orchestration**: `AtomsTemplate` 싱글턴. 위 셋을 한 곳에서 묶어 외부에 노출.

## 3. atoms 외부와의 경계 (현 시점)

| 외부 호출자 | 직접 호출 대상 | 비고 |
|---|---|---|
| `app.cpp` | `AtomsTemplate::*` (메뉴 클릭, 윈도우 렌더 트리거, FocusTarget) | 매우 광범위 |
| `file_loader.cpp` | `AtomsTemplate::LoadXSFParsedData`, `LoadChgcarParsedData` | 파서 결과를 직접 주입 |
| `model_tree.cpp` | `AtomsTemplate::GetStructures`, `IsStructureVisible`, … | 구조 메타 + visibility 다수 getter 의존 |
| `toolbar.cpp` | `AtomsTemplate::HasChargeDensity / IsChargeDensitySimpleViewActive`, `chargeDensityUI()`, `EnterMeasurementMode` 등 | charge density 빠른 컨트롤 |
| `mouse_interactor_style.cpp` | `AtomsTemplate::SelectAtomByPicker`, `HandleMeasurementClickByPicker` 등 | 픽킹/드래그 셀렉션 |

→ **모든 외부 모듈이 `AtomsTemplate` 싱글턴 한 점에 의존**. 측정/charge density/구조 레지스트리/visibility 등이 한 헤더에 같이 노출된다.

## 4. 기존 구조의 한계

다음 5가지가 본 재구성의 동기다.

### 4.1 메뉴 ↔ 코드 매핑이 비직관적

예) 메뉴 **Data → Volumetric** 한 줄을 수정하려면

- `atoms/infrastructure/charge_density_renderer.*` (렌더 파이프라인)
- `atoms/domain/charge_density.*` (그리드 데이터)
- `atoms/ui/charge_density_ui.*` (UI 컨트롤)
- `atoms/atoms_template.cpp` (DataMenuRequest::Volumetric 디스패치)

**4 곳을 동시에 건드려야 한다.** "Data 메뉴를 담당하는 폴더" 가 트리 상에 존재하지 않는다.

### 4.2 `AtomsTemplate` 가 다시 God Object 화

`atoms_template.h` 공개 API 가 보유한 책임:
- Hover / Selection / Tooltip
- Measurement 모드 + 모드별 클릭 처리 + 드래그 선택 + 측정 리스트 CRUD
- Charge Density 로딩 / visibility / 모드 (Simple/Advanced/Volumetric/Plane)
- Structure registry (`RegisterStructure`, `RemoveStructure`, `GetStructures`, …)
- 구조별 visibility / cell visibility / atom group visibility / bond visibility / label visibility
- Builder/Editor/Advanced 윈도우 렌더 + Forced layout 요청
- File I/O (XSF, CHGCAR) 진입 + 파싱 결과 적용
- Bravais Lattice / BZ Plot / Surrounding atoms 모드

→ 한 헤더의 public 면이 100+ 메서드. 측정 코드만 고치고 싶은데 charge density 도, builder 윈도우 layout 도 같이 보인다.

### 4.3 atoms 외부와의 경계가 비일관

- `model_tree`, `file_loader`, `toolbar`, `mouse_interactor_style` 모두 `AtomsTemplate::Instance()` 를 자유롭게 호출.
- 누가 누구를 부를 수 있는지 규칙이 없다 → 의존 방향이 점점 양방향이 된다.

### 4.4 mesh 경로와 atoms 경로가 **나란히** 존재

Model Tree 와 File Loader 는 mesh 와 atoms 둘 다를 다루지만, 코드 트리는 그 공존을 반영하지 않는다 (mesh 는 루트에 흩어져 있고, atoms 만 폴더로 묶여 있음).

### 4.5 infrastructure 안에 "VTK 렌더링" 과 "파일 파싱" 이 섞여 있음

- VTK 렌더링: atoms 전용 (재사용 범위 = 1 피처).
- 파일 파서 (`chgcar_parser`, `rho_file_parser`, `file_io_manager` 의 XSF 부분): 앱 전체에서 쓰이거나 쓸 가능성이 있음 (재사용 범위 = N 피처).

→ 동일한 폴더에 들어 있어 **공유성 기준으로 쪼갤 자연스러운 분기점이 없다.**

## 5. 진단 결론

> 문제는 "레이어 구분이 없다" 가 아니다 (이미 있다).
> **"새 기능을 어디에 넣어야 하는지, 사용자(개발자)가 메뉴만 보고는 알 수 없다"** 가 본질.

해결의 축은 두 가지:
1. **버티컬 슬라이스(메뉴=폴더)** 를 추가해 메뉴 ↔ 코드를 1:1 로 묶는다.
2. **공유성 임계값** 을 명시해, 둘 이상이 쓰는 것만 `core/` 로 올린다.

그리고 본 작업은 in-place 점진 리팩터가 아니라 **legacy 동결 + 그린필드 재구성**으로 진행해, 의존 그래프를 한 번에 리셋한다 (이유는 `05_redevelopment_plan.md` 참고).
