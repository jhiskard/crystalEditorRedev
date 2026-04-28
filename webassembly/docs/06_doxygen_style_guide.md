# 06. Doxygen 주석 컨벤션

> 본 컨벤션은 **모든 신규 파일에 강제** 된다 (legacy/ 는 예외).
> 목표는 단순히 "주석을 남기는 것" 이 아니라, 한 헤더만 보고도
> (a) 그 모듈이 무엇을 하는지, (b) 누가 호출하는지, (c) 어떤 사이드이펙트가 있는지
> 알 수 있도록 만드는 것이다.

## 1. 파일 헤더

모든 `.h` / `.cpp` 파일 최상단:

```cpp
/**
 * @file features/measurement/distance.h
 * @brief Distance 측정 모드의 클릭 처리 / 결과 저장 / 화면 오버레이.
 *
 * @details
 *  Measurement 메뉴(Distance) 진입 후 두 원자 클릭 → 거리 계산 → MeasurementStore 에 저장.
 *  마우스 인터랙터의 atom-pick 이벤트를 구독하고, SelectionSet 을 통해 두 점이 모이면
 *  distance entry 를 emit 한다.
 *
 * @author Crystal Viewer team
 * @date   2026-04-27
 */
```

## 2. 모듈/네임스페이스 주석

각 네임스페이스의 첫 등장 위치:

```cpp
/**
 * @namespace features::measurement
 * @brief 메인 메뉴 "Measurement" 의 5개 모드(Distance/Angle/Dihedral/GeometricCenter/CenterOfMass)
 *        진입·상태머신·저장·UI 를 담당한다.
 *
 * @par 책임 경계
 *  - Owns: 측정 모드 상태머신, 측정 객체 리스트(MeasurementStore), 화면 오버레이.
 *  - Reads: core/scene/SceneState (selection, hover, currentStructureId).
 *  - Emits: events::MeasurementAdded / MeasurementRemoved.
 *  - Does NOT: 다른 feature 의 internals 접근. core/scene 또는 events 만 통과.
 */
namespace features::measurement {
```

## 3. 클래스 주석

```cpp
/**
 * @class MeasurementStore
 * @brief 구조별 측정 객체 리스트와 visibility 를 보관하는 작은 매니저.
 *
 * @details
 *  - 한 SceneState 인스턴스당 하나가 살아 있음 (DI 로 주입).
 *  - 외부에는 read-only iterator 만 노출. 변경은 Add/Remove/SetVisible 만 거침.
 *
 * @invariant 모든 entry 의 structureId 는 SceneState::structures 에 존재.
 * @invariant entry id 는 단조증가하며 재사용되지 않음.
 */
class MeasurementStore {
public:
    /// @brief DI 생성자. SceneState 의 lifetime 에 종속됨.
    explicit MeasurementStore(core::scene::SceneState& scene);
    ...
};
```

## 4. 함수 주석

함수는 다음 4 항목을 빠짐없이 채운다 (필요 없으면 그렇게 명시).

```cpp
/**
 * @brief Distance 모드에서 두 번째 원자가 클릭되었을 때 호출되어 측정 entry 를 만든다.
 *
 * @param[in]  firstAtomId   첫 번째로 선택된 원자 ID.
 * @param[in]  secondAtomId  두 번째로 선택된 원자 ID.
 * @param[in]  structureId   측정이 소속될 구조 ID. SceneState 에 존재해야 함.
 * @return 새로 생성된 측정 entry 의 ID. 실패 시 std::nullopt.
 *
 * @pre  firstAtomId, secondAtomId 는 같은 구조에 속해야 한다.
 * @post 성공 시 events::MeasurementAdded 이벤트가 발행된다.
 *
 * @note Side effects:
 *       - MeasurementStore 에 entry 추가
 *       - SceneState::onMeasurementAdded broadcast
 *       - VTK 액터 1 개 추가 (distance line)
 *
 * @see core::scene::SceneState
 * @see features::measurement::MeasurementOverlayUI
 */
std::optional<MeasurementId> AddDistanceEntry(uint32_t firstAtomId,
                                              uint32_t secondAtomId,
                                              int32_t  structureId);
```

### 4.1 짧은 함수의 경우

trivial getter/setter 라도 한 줄 `///` 주석은 의무.

```cpp
/// @brief 현재 측정 모드를 반환한다.
MeasurementMode CurrentMode() const noexcept { return mode_; }

/// @brief 측정 모드를 None 으로 되돌린다. 진행 중이던 부분 선택은 폐기.
void ExitMode();
```

## 5. 데이터 멤버 / 구조체 필드

`@brief` 한 줄 + (필요 시) `@invariant`:

```cpp
struct StructureEntry {
    /// @brief 구조 ID (Scene 전역 unique).
    int32_t id = -1;

    /// @brief 사용자에게 보여줄 이름. 비어있을 수 있음.
    std::string name;

    /// @brief Model Tree 등에서 토글되는 가시성 플래그.
    /// @invariant 가시성 = false 이면 모든 자식 액터의 GetVisibility() == 0.
    bool visible = true;
};
```

## 6. 매크로 / 상수 / enum

```cpp
/**
 * @enum  MeasurementMode
 * @brief Measurement 메뉴 5 항목과 1:1 대응되는 상태머신 값.
 */
enum class MeasurementMode {
    None             = 0,  ///< 측정 모드 비활성.
    Distance         = 1,  ///< Distance 모드.
    Angle            = 2,  ///< Angle 모드.
    Dihedral         = 3,  ///< Dihedral 모드.
    GeometricCenter  = 4,  ///< Geometric Center 모드.
    CenterOfMass     = 5   ///< Center of Mass 모드.
};
```

## 7. 호출 관계 명시 (`@note Called by` / `@note Calls`)

가능한 한 함수마다 호출자/피호출자를 적는다. legacy 의 atoms_template 주석이 이 패턴을 사용했고 매우 유용했다.

```cpp
/**
 * @brief 결합 매니저가 Bond 추가를 요청할 때 호출되어 VTK 액터를 갱신한다.
 *
 * @note Called by:
 *       - features/edit/bonds/bond_manager.cpp (BondManager::AddBond)
 *       - features/file/structure_import.cpp (on parse result)
 *
 * @note Calls:
 *       - core/vtk/batch_update_system.h (BatchGuard)
 *       - core/vtk/vtk_viewer.h (Render)
 */
```

> 이 정보는 IDE 의 "find references" 가 있어도 변하지 않는 가치가 있다 — **레이아웃이 의도된 호출 그래프와 일치하는지 사람이 확인** 할 수 있게 해 준다.

## 8. 금지 사항

| 안티패턴 | 이유 |
|---|---|
| 빈 `/** */` 헤더 | 노이즈 |
| 함수 시그니처를 그대로 옮긴 주석 ("returns the distance"/`Distance GetDistance()`) | 정보 가치 0 |
| 한국어 + 영어 혼용 한 문장 안에서 | 가독성 — 한 단위(파일/함수) 안에서는 한 언어로 통일 |
| 깨진 인코딩 주석 (legacy 에 다수 존재 — 새 코드에는 두지 않는다) | 디스플레이 환경에 따라 의미 손실 |
| `// TODO: ` 만 있고 issue 링크도 deadline 도 없음 | 영원한 TODO 가 됨. `// TODO(name, 2026-Q3): ...` 형태로 작성 |

## 9. 검증

1. 모든 `.h`/`.cpp` 파일은 `@file` 주석을 가진다 (CI grep 으로 점검 가능).
2. 모든 public 클래스/함수는 `@brief` 를 가진다.
3. side-effect 가 있는 함수는 `@note` 또는 `@post` 로 그것을 명시한다.
4. 주석 언어는 한 파일 안에서 한 언어 (KR 또는 EN) 로 통일한다.

> CI 단계 (선택): `python scripts/check_doxygen.py webassembly/src` 가
> 누락된 `@file`/`@brief` 를 찾아 실패시키도록 만들 수 있다 (Phase 6 에서 검토).

## 10. 권장 도구

- 헤더 작성 시 `clang-format` + Doxygen 친화 설정.
- 주석 일괄 검증: `doxygen Doxyfile` 를 warnings-as-errors 로 돌려 누락 점검.
- 호출 그래프 시각화: Doxygen + Graphviz `dot` (mermaid 보다 정확).

## 11. 짧은 예 — 한 파일 한 페이지로 정리

```cpp
/**
 * @file core/scene/structure_registry.h
 * @brief 장면 안의 모든 구조(atoms / mesh / charge density 컨테이너) 메타데이터 보관.
 */
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace core::scene {

/**
 * @struct StructureEntry
 * @brief 한 구조의 식별/이름/가시성 메타데이터.
 */
struct StructureEntry {
    int32_t     id = -1;     ///< 전역 unique 구조 ID.
    std::string name;        ///< 표시 이름.
    bool        visible = true;  ///< Model Tree 등에서 토글되는 visibility.
};

/**
 * @class StructureRegistry
 * @brief 구조 등록/조회/삭제 + visibility 변경 이벤트 emit.
 *
 * @par 호출자
 *  - features/file/structure_import       (파일 로드 → Register)
 *  - features/model_tree                  (visibility 토글)
 *  - features/data/charge_density         (현재 구조에 charge density 연결)
 *
 * @par 비호출 (해서는 안 됨)
 *  - core/* (core 는 features 를 모름)
 */
class StructureRegistry {
public:
    /**
     * @brief 새 구조를 등록한다.
     * @param[in] id   고유 ID. 중복 시 false 반환.
     * @param[in] name 표시 이름.
     * @return 등록 성공 여부.
     * @post 성공 시 onStructureAdded 이벤트가 발행된다.
     */
    bool Register(int32_t id, std::string name);

    /// @brief 구조를 제거한다. 존재하지 않으면 false.
    bool Remove(int32_t id);

    /// @brief 구조의 visibility 를 변경한다.
    void SetVisible(int32_t id, bool visible);

    /// @brief 모든 구조의 스냅샷을 반환한다.
    std::vector<StructureEntry> List() const;

private:
    std::unordered_map<int32_t, StructureEntry> entries_;
};

} // namespace core::scene
```

이 정도면 한 헤더만 봐도 책임 범위, 호출자, 사이드이펙트가 모두 보인다 — **그것이 본 컨벤션의 목표 상태** 이다.
