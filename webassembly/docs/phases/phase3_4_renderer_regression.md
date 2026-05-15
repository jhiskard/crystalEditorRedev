# Phase 3.4 Renderer 회귀 PR 세부계획서

> 상위 계획: [`../05_redevelopment_plan.md`](../05_redevelopment_plan.md) §6.0 공통 지침 + §6.3.4
> 모 phase 계획서: [`./phase3_4_edit_atoms_bonds_cell.md`](./phase3_4_edit_atoms_bonds_cell.md)
> 모 phase 평가서: [`./phase3_4_evaluation_2026-05-11.md`](./phase3_4_evaluation_2026-05-11.md)
> UI 이식 지침: [`./phase_ui_porting_quality_guideline.md`](./phase_ui_porting_quality_guideline.md) (UI-01 ~ UI-12)
> **후속 phase 계획서 (본 PR 의 머지를 선결 조건으로 함)**: [`./phase3_7_viewer_v2.md`](./phase3_7_viewer_v2.md) v2.1.1 § 1.5 / § 5.6
> 작성일: 2026-05-13
> 대상 브랜치: `refactor/menu-aligned2` 후속 분기 (예: `refactor/edit-renderer-regression`)
> 단위 PR: 1 개, 범위 = `features/edit/atoms/atom_renderer.{h,cpp}` + `features/edit/bonds/bond_renderer.{h,cpp}` + 필요 시 `features/edit/atoms/atoms_controller.cpp` / `features/edit/bonds/bonds_controller.cpp` 의 selection visual 경로
> 예상 소요: 1 ~ 2 일

## 변경 이력

| 일자 | 내용 |
|---|---|
| 2026-05-13 | 초안 작성 — Phase 3.7 v2.1.1 § 1.5 의 선결 조건으로 회귀 PR 분리. legacy 출처는 `legacy/atoms/infrastructure/vtk_renderer.cpp`, `legacy/atoms/domain/{atom_manager,bond_manager,element_database}.cpp`, `legacy/atoms/atoms_template.cpp` (selection shell 함수), `legacy/atoms/ui/bond_ui.cpp` |

---

## 0. 한 줄 요약

> 본 PR 은 Phase 3.4 산출물인 `features/edit/atoms/atom_renderer.cpp` 와 `features/edit/bonds/bond_renderer.cpp` 의 **legacy 1:1 보존 위반** 5 종을 수정한다: (a) 원자 sphere 의 유효 반경 2× 차이, (b) sphere/cylinder shading 파라미터 누락 (ambient 0.3 / diffuse 0.7 / specular 0.1 / specularPower 10), (c) 결합 cylinder 의 proportional split (`radius1/(radius1+radius2)`) 미적용, (d) 결합 opacity translucent path (`SetRenderLinesAsTubes` + `ForceTranslucentOn`) 미구현, (e) 원자 선택 시각 표시가 *그룹 색 변경* 으로 잘못 구현 (legacy 는 *개별 노란 wireframe shell actor*).
>
> 본 PR 은 Phase 3.7 (Viewer / Toolbar 복구) 의 *선결 조건* 이다. Phase 3.7 가 Viewer texture 를 살아나게 하는 즉시 사용자-visible 차이가 드러나므로, 진입 전 본 회귀 PR 을 머지하는 것이 안전 경로다. legacy 1:1 보존 위반의 *origin* 이 Phase 3.4 이므로 책임 phase 도 3.4 다.

---

# Part 1 - 목표 / 비목표 / 책임 위치

## 1.1 목표

| # | 항목 |
|---:|---|
| G1 | 원자 sphere 의 유효 렌더 반경을 `atom.radius * 0.5` 로 복원 (legacy 와 일치) |
| G2 | 원자 sphere actor 의 shading 파라미터 복원: ambient 0.3, diffuse 0.7, specular 0.1, specularPower 10. edge visibility false |
| G3 | 원자 선택 시각 표시를 *그룹 색 변경* → *개별 노란 wireframe shell actor* 로 변경. 선택된 원자 1개만 색이 변하고 다른 원자는 색 유지 |
| G4 | 원자 hover 시각 표시 정책 결정. legacy 는 `m_SelectedActor` 의 edge visibility 토글 — Phase 3.4 가 hover 시각을 어떻게 정의했는지에 따라 (a) legacy 와 동일하게 별도 actor 사용 또는 (b) 의도된 deviation 으로 명시 |
| G5 | 결합 cylinder 의 split 위치를 1/2 → `radius1 / (radius1 + radius2)` proportional 로 변경. 비대칭 결합 (Si–O 등) 에서 색 경계가 작은 원자 측에 더 가까워야 함 |
| G6 | 결합 cylinder actor 의 shading 파라미터 복원 (atom 과 동일: 0.3 / 0.7 / 0.1 / 10. edge visibility false. Pickable false) |
| G7 | 결합 cylinder 의 opacity 분기 복원: `opacity < 1.0` 시 `property->SetRenderLinesAsTubes(true)` + `actor->ForceTranslucentOn()`, `opacity = 1.0` 시 `actor->ForceOpaqueOn()` |
| G8 | UI 슬라이더 회귀 점검: `bondThickness` (0.1~3.0), `bondOpacity` (0.1~1.0), `Distance factor` (현 신규 트리는 `-50% ~ +50%`, legacy 는 0.1 ~ 2.0). 본 PR 의 범위 / 후속 분리 여부 결정 |
| G9 | legacy 격리 유지: `webassembly/src/legacy/` 미수정, 신규 코드에서 `legacy/` include 0 |
| G10 | 빌드 검증: `npm run build-wasm:debug`, `npm run build-wasm:release` PASS |
| G11 | Phase 3.4 의 다른 기능 (cell rendering, bond detection, atom group dispatch) 의 회귀 없음 |

## 1.2 비목표

| 항목 | 사유 / 후속 phase |
|---|---|
| Viewer texture 복구 | Phase 3.7 범위 |
| toolbar 7종 | Phase 3.7 범위 |
| Settings / Windows 메뉴 | Phase 3.7 / 4 범위 |
| Measurement pick shell / `#N` 텍스트 actor | Phase 3.5 산출물. 본 PR 범위 외 (단 Phase 3.7 v2.1.1 § 5.6.3 에서 별도 검증 의무) |
| 측정 모드의 drag selection 정책 | Phase 3.5 책임 |
| 원자 선택 *동작* 자체 (Ctrl+클릭, Ctrl+드래그 등) | Phase 3.7 v2.1.1 § 5.5 책임 (입력 계약) |
| `vtkGlyph3D` 기반 instancing 을 `vtkAppendPolyData` 로 교체 | 결과 동등이면 *구현 방식 자유*. **수치 / shading 파라미터만 정정** |
| element database 확장 | 본 PR 범위 외 |
| Distance factor 슬라이더의 *범위 변경* (`-50% ~ +50%` ↔ `0.1 ~ 2.0`) | UI 의미 차이 큼. 별도 deviation 판정 후 후속 PR 분리 권장 (§ 3.5 참조) |

## 1.3 책임 위치 표

| 항목 | 회귀 PR (본 문서) | Phase 3.7 v2.1.1 |
|---|---|---|
| atom sphere 반경 산식 수정 | ✅ 본 PR | 검증 #46 |
| atom shading 파라미터 추가 | ✅ 본 PR | 검증 #47 |
| atom 선택 시각 → wireframe shell 도입 | ✅ 본 PR | 검증 #48 |
| bond proportional split 산식 수정 | ✅ 본 PR | 검증 #49 |
| bond shading 파라미터 추가 | ✅ 본 PR | 검증 #50 |
| bond opacity translucent path | ✅ 본 PR | 검증 #51 |
| bondThickness / bondOpacity 슬라이더 (UI 자체) | 부분 — UI 는 이미 신규 트리에 있음. 본 PR 은 *값 ↔ renderer state 연결* 만 점검 | 검증 #52 |
| Distance factor 슬라이더 범위 | 분리 (별도 PR 권장) | 검증 #53 (DEVIATION 가능) |

---

# Part 2 - 식별된 회귀 항목 (정확한 위치)

## 2.1 R1: 원자 sphere 유효 반경 2× 차이

| 항목 | 값 |
|---|---|
| Legacy 산식 | base radius **0.5** × `Transform.Scale(s, s, s)` 이고 `s = (atom.radius * 0.5) / 0.5 = atom.radius` → **유효 반경 `atom.radius * 0.5`** |
| Legacy 코드 | `legacy/atoms/infrastructure/vtk_renderer.cpp:102` (`createSphereGeometry(0.5f)`), `legacy/atoms/domain/atom_manager.cpp:113~119` (`adjustedRadius = atom.radius * 0.5; scaleFactor = adjustedRadius / 0.5`) |
| 현 신규 트리 산식 | base radius **1.0** × `vtkGlyph3D::ScaleByScalar(atom->radius)` → **유효 반경 `atom.radius`** |
| 현 신규 코드 | `webassembly/src/features/edit/atoms/atom_renderer.cpp:335` (`SetRadius(1.0)`), `atom_renderer.cpp:390~391` (`scales->InsertNextValue(atom->radius)`) |
| 차이 | **2 ×** (모든 원자가 legacy 대비 2배 큼) |
| 수정 방향 | (a) base radius = 0.5 + scale = `atom.radius` (유효 = `atom.radius * 0.5`), 또는 (b) base radius = 1.0 + scale = `atom.radius * 0.5` (유효 = `atom.radius * 0.5`). 등가. 본 PR 은 (b) 가 신규 트리 코드 변경량이 가장 작아 권장 |

## 2.2 R2: 원자 sphere shading 파라미터 누락

| 항목 | Legacy | 현 신규 |
|---|---|---|
| 출처 | `legacy/atoms/infrastructure/vtk_renderer.cpp:892~898` (`setupActorProperties`) | `webassembly/src/features/edit/atoms/atom_renderer.cpp:353~357` |
| EdgeVisibility | false | 명시 없음 (VTK 기본 false → OK) |
| Ambient | **0.3** | 명시 없음 → VTK 기본 **0.0** |
| Diffuse | **0.7** | 명시 없음 → VTK 기본 **1.0** |
| Specular | **0.1** | 명시 없음 → VTK 기본 **0.0** |
| SpecularPower | **10** | 명시 없음 → VTK 기본 **1.0** |
| Pickable | true | true ✓ |
| 영향 | atom 의 highlight / 반사감이 legacy 와 다르게 *너무 매트* 함 |
| 수정 방향 | `EnsureGroupInitialized` 에서 `vtkProperty* property = group.actor->GetProperty()` 후 `SetAmbient(0.3)`, `SetDiffuse(0.7)`, `SetSpecular(0.1)`, `SetSpecularPower(10)`, `SetEdgeVisibility(false)` 추가 |

## 2.3 R3: 원자 선택 시각 표시 — 그룹 색 변경 (잘못된 구현)

| 항목 | 값 |
|---|---|
| Legacy 정책 | 선택된 원자 *각각* 에 *별도 wireframe shell actor* 추가. 색 = `(1.0, 1.0, 0.0)` 노랑, line width 2.0, ambient 1.0, diffuse 0.0, specular 0.0, theta/phi 24, `SetPickable(false)`. 본체 sphere actor 의 *색은 변경하지 않음* |
| Legacy 출처 | `legacy/atoms/atoms_template.cpp:1055~1080` (`createCreatedAtomSelectionShellActor`), `legacy/atoms/atoms_template.cpp:4034~4056` (`syncCreatedAtomSelectionVisuals`), `legacy/atoms/atoms_template.cpp:960~962` (`renderedSelectionSphereRadius = max(0.01, atom.radius * 0.5 + 0.01)`) |
| 현 신규 정책 | `features/edit/atoms/atom_renderer.cpp:421~424` — group 의 *전체 색* 을 `(1.0, 0.95, 0.20)` 으로 변경. 결과: **선택된 원자 1개의 영향으로 같은 원소의 *다른* 원자들까지 모두 노란색이 됨** |
| 차이 | legacy 1:1 보존 위반. UI 지침 UI-01 / UI-05 위반 |
| 수정 방향 | (a) `atom_renderer.cpp:421~432` 의 그룹 색 변경 분기 제거. (b) `AtomRenderer` 에 `selectionShells_` map 추가, `OnSelectionChanged` 이벤트 구독 시 추가/제거. (c) shell 생성 함수 `MakeSelectionShellActor(atom)` 구현. (d) shell 의 반경 = `max(0.01, atom.radius * 0.5 + 0.01)` |

## 2.4 R4: 원자 hover 시각 표시 — 그룹 색 변경

| 항목 | 값 |
|---|---|
| 현 신규 정책 | `atom_renderer.cpp:425~429` — hover 시 group 전체를 `(0.2, 0.95, 1.0)` 시안 색으로 변경. 동일 문제 |
| Legacy 정책 | `m_SelectedActor` 의 `EdgeVisibility(true)` 만 토글. hover 는 개별 actor 단위 |
| 수정 방향 | (a) 그룹 색 변경 분기 제거. (b) hover shell actor 또는 hover atom 의 edge visibility 토글 중 *Phase 3.4 가 의도한 정책* 선택. legacy 와 정확히 동일한 모습은 *현 vtkGlyph3D 구조* 에서 어려움 → **deviation 가능**. 단 *그룹 전체가 색이 바뀌는* 현 동작은 명백한 결함이므로 *어떤 형태든* 수정 필요 |

## 2.5 R5: 결합 cylinder split 위치 — 1/2 (잘못된 구현)

| 항목 | 값 |
|---|---|
| Legacy 산식 | `ratio = radius1 / (radius1 + radius2)`, `splitCenter = position1 + ratio * (position2 - position1)`. 비대칭 결합에서 색 경계가 작은 원자 측에 더 가까움 |
| Legacy 출처 | `legacy/atoms/domain/bond_manager.cpp:194~208` |
| 현 신규 산식 | `mid = (pointA + pointB) * 0.5f` — 정확히 1/2 |
| 현 신규 출처 | `webassembly/src/features/edit/bonds/bond_renderer.cpp:288~300` (`BuildHalfBondTransforms`) |
| 영향 | Si–O, C–H 등 비대칭 결합에서 색 경계 위치가 legacy 와 다름. 시각적으로 명백한 차이 |
| 수정 방향 | `BuildHalfBondTransforms` 의 signature 를 `(pointA, pointB)` → `(pointA, pointB, radius1, radius2)` 로 확장. `mid = pointA + (radius1 / (radius1 + radius2)) * (pointB - pointA)`. caller (`UpdateBondGroup` 류) 가 atom1.bondRadius / atom2.bondRadius 를 전달하도록 수정 |

## 2.6 R6: 결합 cylinder shading 파라미터 누락

| 항목 | Legacy | 현 신규 |
|---|---|---|
| 출처 | `legacy/atoms/infrastructure/vtk_renderer.cpp:892~908` | `webassembly/src/features/edit/bonds/bond_renderer.cpp:434~445` |
| Ambient / Diffuse / Specular / SpecularPower | 0.3 / 0.7 / 0.1 / 10 | 명시 없음 → VTK 기본 0.0 / 1.0 / 0.0 / 1.0 |
| EdgeVisibility | false | 명시 없음 (OK) |
| Pickable | false | false ✓ |
| 수정 방향 | atom 과 동일하게 shading 4 값 명시 |

## 2.7 R7: 결합 opacity translucent path 미구현

| 항목 | 값 |
|---|---|
| Legacy 정책 | `opacity < 1.0` 시 `property->SetRenderLinesAsTubes(true)` + `actor->ForceTranslucentOn()`. `opacity = 1.0` 시 `actor->ForceOpaqueOn()` |
| Legacy 출처 | `legacy/atoms/infrastructure/vtk_renderer.cpp:902~908` |
| 현 신규 정책 | `bond_renderer.cpp:444~445, 530~531` — `SetOpacity(globalOpacity_)` 만 호출. `RenderLinesAsTubes` / `ForceTranslucentOn` / `ForceOpaqueOn` 호출 없음 |
| 영향 | opacity < 1.0 시 VTK 의 *기본 translucent ordering* 만 적용되어 legacy 의 tube-mode 시각 효과가 빠짐 |
| 수정 방향 | `SetOpacity` 호출 직후 다음 추가: `if (opacity < 1.0f) { property->SetRenderLinesAsTubes(true); actor->ForceTranslucentOn(); } else { actor->ForceOpaqueOn(); }`. `UpdateAllBondGroupOpacity` 와 `UpdateBondGroup` 양쪽 |

## 2.8 R8: Distance factor 슬라이더 범위 차이 (UI deviation 후보)

| 항목 | Legacy | 현 신규 |
|---|---|---|
| 슬라이더 라벨 | `Distance factor` | `Bond distance factor (%)` |
| 범위 | 0.1 ~ 2.0 (배수, default 1.0) | -50% ~ +50% (퍼센트 오프셋) |
| 출처 | `legacy/atoms/ui/bond_ui.cpp:132~` | `webassembly/src/features/edit/bonds/bond_ui.cpp:89` |
| 의미 | bond 검출 길이 임계값의 배수 | bond 검출 길이 임계값의 ±50% 오프셋 (퍼센트) |
| 차이 | UI 의미 자체가 다름. legacy 0.1 = 10%, 2.0 = 200%. 신규 -50% = 0.5×, +50% = 1.5×. 범위가 좁아짐 (legacy 는 0.1× ~ 2.0× 가능, 신규는 0.5× ~ 1.5× 만) |
| 수정 방향 | (a) 본 PR 에 포함해 슬라이더 의미 / 범위를 legacy 와 동일하게 복원, 또는 (b) intentional deviation 으로 등록 후 후속 PR 로 분리. **결정 필요**. 권장: (b) 분리 (본 PR 의 PR 규모 통제) |

---

# Part 3 - 수정 계획

## 3.1 R1 + R2 + R3 + R4 — `atom_renderer.cpp`

| Step | 작업 |
|---|---|
| S1 | `EnsureGroupInitialized` 의 `SetRadius(1.0)` → `SetRadius(0.5)` 변경 (R1 옵션 (a)) **또는** `UpdateGroupData` 에서 `scales->InsertNextValue(atom->radius)` → `scales->InsertNextValue(atom->radius * 0.5)` 변경 (R1 옵션 (b)). 옵션 (b) 가 변경량 최소 |
| S2 | `EnsureGroupInitialized` 의 actor property 설정에 `SetAmbient(0.3)`, `SetDiffuse(0.7)`, `SetSpecular(0.1)`, `SetSpecularPower(10)`, `SetEdgeVisibility(false)` 추가 (R2) |
| S3 | `UpdateGroupData` 의 selection / hover 분기 (`atom_renderer.cpp:421~431`) 제거. 그룹 색 변경 로직 삭제 (R3 + R4 의 잘못된 부분 제거) |
| S4 | `AtomRenderer` 에 `selectionShells_` (map<atomId, vtkSmartPointer<vtkActor>>) 추가 |
| S5 | `OnSelectionChanged` 이벤트 구독 진입 / 또는 `UpdateGroupData` 의 매 frame 동기화 시 selection set 비교 후 추가/제거. 권장: event-driven (rebuild 비용 최소화) |
| S6 | helper `MakeSelectionShellActor(const AtomRecord& atom)` 신설: `vtkSphereSource` 반경 `max(0.01, atom.radius * 0.5 + 0.01)`, theta/phi 24, color (1.0, 1.0, 0.0), `SetRepresentationToWireframe()`, line width 2.0, ambient 1.0, diffuse 0.0, specular 0.0, `SetPickable(false)`. 반환 후 `VtkViewer::AddActor` (R3) |
| S7 | hover 시각 표시 정책 결정 (R4): (a) legacy 와 가까운 방식 = hover 시 별도 hover shell actor, 또는 (b) deviation 등록. PR 본문에 명시 |

## 3.2 R5 + R6 + R7 — `bond_renderer.cpp`

| Step | 작업 |
|---|---|
| S8 | `BuildHalfBondTransforms` signature 확장: `(pointA, pointB, radius1, radius2)`. `mid = pointA + (radius1 / (radius1 + radius2)) * (pointB - pointA)`. radius 합이 0 에 가까우면 1/2 fallback (R5) |
| S9 | caller 의 모든 호출부에 atom1.bondRadius / atom2.bondRadius 전달 (R5 의 propagation) |
| S10 | bond actor 의 property 설정에 `SetAmbient(0.3)`, `SetDiffuse(0.7)`, `SetSpecular(0.1)`, `SetSpecularPower(10)`, `SetEdgeVisibility(false)` 추가 (R6). actor1, actor2 양쪽 |
| S11 | `SetOpacity(globalOpacity_)` 호출 직후 다음 추가 (R7): <br>`auto* prop = actor->GetProperty();`<br>`if (globalOpacity_ < 1.0f) { prop->SetRenderLinesAsTubes(true); actor->ForceTranslucentOn(); } else { prop->SetRenderLinesAsTubes(false); actor->ForceOpaqueOn(); }`<br>`UpdateAllBondGroupOpacity` (260~270), `EnsureGroupInitialized` (444~445), `UpdateGroupData` (530~531) 모두 |

## 3.3 controller / UI 점검 — `atoms_controller.cpp` / `bonds_controller.cpp` / `bond_ui.cpp`

| Step | 작업 |
|---|---|
| S12 | `AtomsController` 가 selection 이벤트를 `AtomRenderer` 로 전달하는 경로 점검. event subscription 없으면 추가 |
| S13 | `bond_ui.cpp:54~73` 의 Thickness / Opacity 슬라이더가 `BondsController::SetGlobalThickness/SetGlobalOpacity` 를 호출하고, 이것이 `BondRenderer::UpdateAllBondGroupOpacity/UpdateAllBondGroupThickness` 까지 전파되는지 코드 trace |
| S14 | `Distance factor` 슬라이더 (R8) 의 처리: 본 PR 에 포함하지 *않음* → bond_ui.cpp 의 해당 슬라이더는 *수정 0*. 평가서에 DEVIATION 으로 기록 + 후속 PR 발의 |

## 3.4 Phase 3.4 다른 기능 회귀 점검

| Step | 작업 |
|---|---|
| S15 | imported XSF / Bravais 생성 / Periodic Table 입력 / Cell 표시 모두 회귀 없음을 정적 grep + 빌드로 확인 |
| S16 | bond detection (atom-pair distance threshold) 로직은 *건드리지 않음*. `bond_manager.cpp` 의 detection 부분은 변경 0 |
| S17 | `surrounding_atom_manager.cpp` (boundary atoms) 는 *건드리지 않음* |

## 3.5 deviation 등록

| 항목 | deviation 사유 | 후속 처리 |
|---|---|---|
| R8 `Distance factor` 슬라이더 범위 | UI 의미 변경 폭이 커서 별도 PR 권장. 사용자 영향 분석 필요 | 별도 phase 3.4 추가 회귀 PR 또는 Phase 4 UI 정비 PR |
| R4 hover 시각 표시 | legacy 의 `EdgeVisibility` 토글 방식이 `vtkGlyph3D` 와 호환되지 않을 수 있음. *Phase 3.4 가 의도한* hover 정책을 따르되, 그룹 색 변경은 제거 | 결정 후 PR 본문에 deviation 등록 |

---

# Part 4 - 검증 매트릭스

## 4.1 정적 / 빌드 검증

| # | 검증 항목 | 기대값 | 방법 | 판정 |
|---:|---|---|---|---|
| 1 | sphere 유효 반경 산식 | `atom.radius * 0.5` | `rg -n "SetRadius\\|scales->InsertNextValue" webassembly/src/features/edit/atoms/atom_renderer.cpp` | PASS / FAIL |
| 2 | sphere shading 4 값 | `SetAmbient(0.3)`, `SetDiffuse(0.7)`, `SetSpecular(0.1)`, `SetSpecularPower(10)` 모두 존재 | `rg -n "SetAmbient\\|SetDiffuse\\|SetSpecular" webassembly/src/features/edit/atoms` | PASS / FAIL |
| 3 | 그룹 색 변경 분기 제거 | `(1.0, 0.95, 0.20)` 또는 `(0.20, 0.95, 1.0)` 리터럴 0 | `rg -n "1\\.0f.*0\\.95f\\|0\\.20f.*0\\.95f" webassembly/src/features/edit/atoms` | PASS (hit 0) / FAIL |
| 4 | selection shell helper 존재 | `MakeSelectionShellActor` 또는 동등 함수 정의 | `rg -n "SelectionShellActor\\|selectionShells_" webassembly/src/features/edit/atoms` | PASS / FAIL |
| 5 | bond proportional split | `radius1 / (radius1 + radius2)` 또는 등가 표현 | `rg -n "radius1.*radius2\\|/\\(radius1\\+radius2\\)" webassembly/src/features/edit/bonds` | PASS / FAIL |
| 6 | bond shading 4 값 | `SetAmbient(0.3)`, `SetDiffuse(0.7)`, `SetSpecular(0.1)`, `SetSpecularPower(10)` | `rg -n "SetAmbient\\|SetDiffuse\\|SetSpecular" webassembly/src/features/edit/bonds` | PASS / FAIL |
| 7 | bond opacity translucent path | `SetRenderLinesAsTubes` + `ForceTranslucentOn` + `ForceOpaqueOn` 모두 존재 | `rg -n "RenderLinesAsTubes\\|ForceTranslucentOn\\|ForceOpaqueOn" webassembly/src/features/edit/bonds` | PASS / FAIL |
| 8 | legacy 수정 0 | `webassembly/src/legacy/` 기능 변경 없음 | `git diff -- webassembly/src/legacy` | PASS / FAIL |
| 9 | legacy include 0 | 변경한 파일에서 `legacy/` include 0 | `rg -n "legacy\\|src/legacy\\|#include.*legacy" webassembly/src/features/edit/atoms webassembly/src/features/edit/bonds` | PASS / FAIL |
| 10 | debug build | exit 0 | `cmd /c "..\\emsdk\\emsdk_env.bat && npm.cmd run build-wasm:debug"` | PASS / FAIL |
| 11 | release build | exit 0 | 동일 | PASS / FAIL |
| 12 | wasm size delta | 직전 baseline 대비 정상 범위 | `Get-Item public\\wasm\\VTK-Workbench.wasm` | PASS / WATCH |

## 4.2 런타임 / 시각 검증

> 본 PR 의 런타임 검증은 Phase 3.7 Viewer 가 살아나기 전까지는 *제한적* 이다. 가능한 검증은 (a) 상태 / log 기반, (b) Phase 3.7 작업 브랜치에 cherry-pick 후 시각 확인 두 가지다.

| # | 검증 항목 | 기대값 | 판정 |
|---:|---|---|---|
| 13 | imported XSF 후 atom 그룹 actor 1개 / 원소 추가 시 actor 1개 추가 | 상태 log 또는 actor count | PASS / FAIL |
| 14 | imported XSF 후 selection event 발행 시 selection shell actor 추가 (visual 검증은 Phase 3.7 cherry-pick 필요) | shell actor count == selected atom count | PASS / FAIL (state) / PARTIAL (시각 미수행) |
| 15 | bond detection 후 actor1 / actor2 각 1개 | log 또는 count | PASS / FAIL |
| 16 | bond opacity 0.5 설정 → `RenderLinesAsTubes=true` + actor `IsTranslucent()` true | log 확인 | PASS / FAIL |
| 17 | imported Si–O 결합의 split 위치 (Phase 3.7 cherry-pick 후 시각 검증) | 색 경계가 O 측에 더 가까움 | PASS / PARTIAL (시각 미수행) |
| 18 | imported XSF 후 Ctrl+클릭 선택 → 같은 원소의 *다른* 원자 색 *유지* (Phase 3.7 cherry-pick 후 시각 검증) | 선택 1개만 노랗게, 나머지 원자 원래 색 | PASS / PARTIAL (시각 미수행) |
| 19 | bondThickness 슬라이더 0.5 / 1.0 / 2.0 → cylinder 굵기 즉시 변경 | UI 동작 (Phase 3.7 cherry-pick 후 시각 검증) | PASS / PARTIAL |
| 20 | bondOpacity 슬라이더 0.5 → 반투명 + tube 렌더 | UI 동작 (시각 검증) | PASS / PARTIAL |
| 21 | console error 0 | DevTools | PASS / FAIL |

## 4.3 UI 지침 매트릭스 (UI-01 ~ UI-12, 본 PR 범위)

| UI ID | 적용 |
|---|---|
| UI-01 옵션명 / 표시순서 | Edit / Bonds 의 Thickness / Opacity / Distance factor 슬라이더 순서 legacy 와 동일 |
| UI-02 기본값 / 범위 | Thickness 0.1~3.0 default 1.0, Opacity 0.1~1.0 default 1.0, Distance factor (R8 deviation 가능) |
| UI-03 입력 방법 | 슬라이더 + Reset SmallButton 위치 |
| UI-06 Apply 시점 | 즉시 반영 |
| UI-11 실제 파일 로드 렌더 | imported XSF 의 sphere / bond shading 이 legacy 와 동일 |

---

# Part 5 - 리스크와 대응

| # | 리스크 | 영향 | 대응 |
|---:|---|---|---|
| R-a | `vtkGlyph3D` 구조에서 selection shell 추가가 메모리 / 성능 비용 증가 | 큰 구조에서 FPS 저하 | shell actor 는 *선택된 원자만* 생성. 매 frame rebuild 가 아니라 event-driven |
| R-b | selection shell 의 반경 산식 `max(0.01, atom.radius * 0.5 + 0.01)` 가 hover 와 충돌 | hover/selection 동시 시각 표시 시 actor 겹침 | legacy 에서도 동일 산식 사용 — z-fighting 은 line width 와 ambient 로 회피 |
| R-c | bond proportional split 산식 변경이 기존 bond detection 결과를 깨뜨림 | 결합 갯수 변경 | detection 로직은 *건드리지 않음*. transform 만 변경 |
| R-d | atom radius 산식 수정으로 imported 구조의 sphere 가 갑자기 절반 크기로 보임 | 사용자 혼란 가능 | 이는 legacy 와 일치하는 *복원* 이므로 의도된 시각 변화. Phase 3.7 평가서에 explicitly 기재 |
| R-e | hover 시각 표시의 deviation 인정 폭 | UI 지침 위반 우려 | hover 가 *어떤 형태든 작동* 하면 PASS. 그룹 색 변경만 *반드시 제거* |
| R-f | Distance factor 슬라이더 변경 미포함 | UI 지침 UI-01 / UI-02 위반 잔존 | 본 PR 의 deviation 으로 명시 + 후속 PR 발의 |
| R-g | wasm size 증가 | release artifact 비대화 | shell actor 추가 비용은 small. release build 후 size delta 기록 |
| R-h | Phase 3.5 / Phase 3.7 의 다른 기능과 conflict | merge conflict 가능 | base branch 를 최신으로 유지. 작은 PR 로 분리 |
| R-i | 시각 검증의 cherry-pick 필요성 | Viewer 미복구 상태에서는 *완전한* 시각 검증 불가 | Phase 3.7 작업 브랜치에 본 PR cherry-pick 후 함께 검증. 결과를 Phase 3.7 평가서에 반영 |

---

# Part 6 - Definition of Done

| DoD 항목 | 기준 |
|---|---|
| 산식 | atom sphere 유효 반경 = `atom.radius * 0.5`, bond split = `radius1/(radius1+radius2)` proportional |
| shading | atom + bond actor 모두 ambient 0.3 / diffuse 0.7 / specular 0.1 / specularPower 10 / edge visibility false |
| selection 시각 | 개별 wireframe shell actor. 그룹 색 변경 *완전 제거* |
| bond opacity | < 1.0 시 tube + translucent, 1.0 시 opaque, branch 코드 모두 존재 |
| Pickable | atom true, bond false |
| legacy 격리 | legacy 수정 0, legacy include 0 |
| build | debug / release wasm build PASS |
| 정적 검증 | 매트릭스 #1 ~ #12 모두 PASS |
| 런타임 (state) | #13 ~ #16, #21 PASS |
| 시각 (Phase 3.7 cherry-pick) | #17 ~ #20 PASS 또는 명시 PARTIAL |
| deviation | R8 (Distance factor 슬라이더) 와 R4 (hover 정책) 의 deviation 결정 및 PR 본문 기록 |
| 평가서 | `phase3_4_renderer_regression_evaluation_YYYY-MM-DD.md` 작성 (선택) 또는 Phase 3.7 평가서에 통합 기록 |

---

# Part 7 - PR 체크리스트

## 작성자

### 정적 / 구조
- [ ] `atom_renderer.cpp` 의 sphere 산식 수정 (옵션 (a) base 0.5 + scale `atom.radius`, 또는 (b) base 1.0 + scale `atom.radius * 0.5`)
- [ ] `atom_renderer.cpp` 의 actor 에 shading 4 값 (ambient 0.3 / diffuse 0.7 / specular 0.1 / specularPower 10) 추가
- [ ] `atom_renderer.cpp` 의 그룹 색 변경 분기 (`SetColor(1.0f, 0.95f, 0.20f)` / `SetColor(0.20f, 0.95f, 1.0f)`) 제거
- [ ] `AtomRenderer::MakeSelectionShellActor(atom)` 또는 동등 함수 신설 + `selectionShells_` map 추가
- [ ] selection event 구독 또는 frame-level 동기화 경로 추가
- [ ] hover 시각 표시 정책 결정 (legacy-like vs deviation) — PR 본문에 명시
- [ ] `bond_renderer.cpp` 의 `BuildHalfBondTransforms` signature 확장 + `radius1 / (radius1 + radius2)` 산식 적용
- [ ] caller 가 radius1 / radius2 (atom.bondRadius) 를 전달하도록 수정
- [ ] `bond_renderer.cpp` 의 actor1 / actor2 에 shading 4 값 추가
- [ ] `bond_renderer.cpp` 의 `SetOpacity` 호출 모든 위치에 translucent / tube / opaque 분기 추가
- [ ] `webassembly/src/legacy/` 변경 0
- [ ] `features/edit/atoms`, `features/edit/bonds` 의 변경 코드에서 `legacy/` include 0

### 빌드 / 정량
- [ ] `npm run build-wasm:debug` PASS
- [ ] `npm run build-wasm:release` PASS
- [ ] release wasm size 기록 (직전 baseline 대비 delta)

### 런타임 (state)
- [ ] imported XSF / Bravais 생성 / Periodic Table 입력 회귀 없음
- [ ] atom selection event → shell actor 생성 / 해제 (count 검증)
- [ ] bond opacity 0.5 → 코드 경로 trace 로 `RenderLinesAsTubes=true` + `IsTranslucent()` true 확인
- [ ] console error 0

### 시각 (Phase 3.7 작업 브랜치 cherry-pick)
- [ ] imported XSF 의 sphere 크기가 legacy 스크린샷과 일치
- [ ] Si–O 등 비대칭 결합의 색 경계가 O 측에 더 가까이 표시
- [ ] Ctrl+클릭 선택 시 같은 원소 다른 원자의 색 *유지*
- [ ] bondThickness / bondOpacity 슬라이더 동작 + UI 라벨 / 범위 / Reset 버튼 legacy 와 동일

### deviation
- [ ] R8 (Distance factor 슬라이더 범위) 의 PR 분리 / 본 PR 포함 결정
- [ ] R4 (hover 시각 표시) 의 정책 결정
- [ ] PR 본문에 *Intentional UI deviation* 섹션 (없으면 `None`)

## 검토자

- [ ] diff 가 `features/edit/atoms/atom_renderer.{h,cpp}` + `features/edit/bonds/bond_renderer.{h,cpp}` + 필요 시 controller 의 selection event 경로로 제한되어 있는가?
- [ ] atom sphere 의 유효 반경이 legacy 와 동일한가?
- [ ] atom 선택 시 그룹 전체 색 변경이 *완전히 제거* 되었는가?
- [ ] bond proportional split 산식이 올바른가?
- [ ] bond opacity < 1.0 분기 코드가 존재하는가?
- [ ] legacy 격리가 유지되는가?
- [ ] wasm size delta 가 정상 범위인가?
- [ ] deviation 항목이 명시되었는가?

---

# Part 8 - Phase 3.7 으로의 인계

본 PR 머지 후 Phase 3.7 v2.1.1 의 다음 항목이 자동으로 수행 가능해진다.

| Phase 3.7 항목 | 본 PR 의 인계 |
|---|---|
| § 5.6.1 atom sphere 스타일 계약 | 코드가 legacy 와 일치 |
| § 5.6.2 bond cylinder 스타일 계약 | 코드가 legacy 와 일치 |
| § 5.5.3 비-측정 모드 선택 시각 (노란 wireframe shell) | shell actor 생성 / 해제 경로 완성 |
| 매트릭스 #46 atom 유효 반경 | PASS 기대 |
| 매트릭스 #47 atom shading | PASS 기대 |
| 매트릭스 #48 atom 선택 시각 = 개별 wireframe shell | PASS 기대 |
| 매트릭스 #49 bond proportional split | PASS 기대 |
| 매트릭스 #50 bond shading | PASS 기대 |
| 매트릭스 #51 bond opacity translucent path | PASS 기대 |
| 매트릭스 #52 bondThickness / bondOpacity 슬라이더 | PASS 기대 |
| 매트릭스 #53 Distance factor 슬라이더 | 본 PR 범위 외 → **DEVIATION** 으로 인계 |

> Phase 3.7 평가서는 매트릭스 #46 ~ #52 가 PASS 가 아니라면 본 PR 의 잔여 항목으로 식별하고, *추가 회귀 PR* 을 발의한다.

---

# Part 9 - 사용 명령 요약

```powershell
git status --short --branch
git checkout -b refactor/edit-renderer-regression refactor/menu-aligned2

# 정적 검증
rg -n "SetRadius|scales->InsertNextValue" webassembly\src\features\edit\atoms\atom_renderer.cpp
rg -n "SetAmbient|SetDiffuse|SetSpecular|SetSpecularPower|SetEdgeVisibility" webassembly\src\features\edit\atoms webassembly\src\features\edit\bonds
rg -n "radius1.*radius2|/\(radius1\+radius2\)" webassembly\src\features\edit\bonds
rg -n "RenderLinesAsTubes|ForceTranslucentOn|ForceOpaqueOn" webassembly\src\features\edit\bonds
rg -n "1\.0f.*0\.95f|0\.20f.*0\.95f" webassembly\src\features\edit\atoms
rg -n "MakeSelectionShellActor|selectionShells_" webassembly\src\features\edit\atoms
rg -n "legacy|src/legacy|#include.*legacy" webassembly\src\features\edit\atoms webassembly\src\features\edit\bonds
git diff -- webassembly/src/legacy

# 빌드
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:debug"
cmd /c "..\emsdk\emsdk_env.bat && npm.cmd run build-wasm:release"
Get-Item public\wasm\VTK-Workbench.wasm
```

---

# Part 10 - 관련 문서

- [Phase 3.4 모 계획서](./phase3_4_edit_atoms_bonds_cell.md)
- [Phase 3.4 분할 제안서](./phase3_4_split_proposal.md)
- [Phase 3.4 평가서](./phase3_4_evaluation_2026-05-11.md)
- [Phase 3.7 v2.1.1 (본 PR 의 머지를 선결 조건으로 함)](./phase3_7_viewer_v2.md)
- [UI 이식 품질 지침](./phase_ui_porting_quality_guideline.md)
- 상위 계획: [재구성 계획](../05_redevelopment_plan.md) §6.0 + §6.3.4
- 대상 아키텍처: [target architecture](../03_target_architecture.md) §2 features/edit
