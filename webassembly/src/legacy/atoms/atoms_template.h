/**
 * @file atoms_template.h
 * @brief Crystal Builder/Editor orchestrator interface (UI/domain/renderer glue).
 */
#pragma once

#include "../macro/singleton_macro.h"
#include "../config/log_config.h"

#include "infrastructure/batch_update_system.h"
#include "infrastructure/bond_renderer.h"
#include "infrastructure/vtk_renderer.h"
#include "infrastructure/file_io_manager.h"
#include "infrastructure/chgcar_parser.h"

#include "domain/atom_manager.h"
#include "domain/bond_manager.h"
#include "domain/cell_manager.h"
#include "domain/crystal_structure.h" 
#include "domain/element_database.h"
#include "domain/special_points.h"
#include "domain/bz_plot.h"
#include "domain/surrounding_atom_manager.h"

// ImGui
#include <imgui.h>

#include <array>
#include <unordered_map>
#include <unordered_set>


// ============================================================================
// FORWARD DECLARATIONS & DEPENDENCIES
// ============================================================================
class vtkActor;
class vtkActor2D;
class vtkTransform;
class vtkPolyData;
class Mesh;
template<class T> class vtkSmartPointer;

namespace atoms {
    namespace ui {
        class PeriodicTableUI;
        class BravaisLatticeUI;
        class AtomEditorUI;
        class BondUI;
        class CellInfoUI;
        class BZPlotUI;
    } // namespace ui
    namespace domain {
        enum class BravaisLatticeType;
        struct BravaisParameters;
    } // namespace domain
} // namespace atoms

// ============================================================================ 
// ATOM RENDERING SYSTEM
// ============================================================================

// ?렞 ?듯빀 ?먯옄 ?쒖뒪???⑥닔??(沅뚯옣 ?ъ슜)

/**
 * @brief ?꾨찓???먯옄 洹몃９???앹꽦/媛깆떊?섍퀬 ?뚮뜑???뚯씠?꾨씪?몄쓣 珥덇린?뷀빀?덈떎.
 * @param symbol ?먯냼 湲고샇 (?? "H").
 * @param radius ?먯옄 諛섏?由?(?꾨찓??洹몃９ 湲곗?).
 * @details atoms::domain::initializeAtomGroup()???몄텧???? AtomsTemplate??VTK 洹몃９ 珥덇린?붾? ?꾩엫?⑸땲??
 * @note Affects: atoms::domain::atomGroups, m_vtkRenderer??atom group ?뚯씠?꾨씪??
 * @note Called by: webassembly/src/atoms/atoms_template.cpp (AtomsTemplate::createAtomSphere).
 */
void initializeAtomGroup(const std::string& symbol, float radius);

/**
 * @brief ?뱀젙 ?먯옄 洹몃９??VTK ?몄뒪?댁떛 ?곗씠?곕? 媛깆떊?⑸땲??
 * @param symbol ?낅뜲?댄듃???먯옄 洹몃９?????먯냼 湲고샇).
 * @details AtomsTemplate::updateUnifiedAtomGroupVTK()濡??꾩엫?섎뒗 ?꾩뿭 ?섑띁?낅땲??
 * @note Affects: m_vtkRenderer??atom group VTK ?곗씠??
 * @note Called by: webassembly/src/atoms/infrastructure/batch_update_system.cpp
 *                  (atoms::infrastructure::BatchUpdateSystem::endBatch).
 */
void updateUnifiedAtomGroupVTK(const std::string& symbol);

/**
 * @brief ?꾨찓???먯옄 洹몃９?먯꽌 ?뱀젙 ?먯옄瑜??쒓굅?섍퀬 ?뚮뜑 媛깆떊???ㅼ?以꾨쭅?⑸땲??
 * @param symbol ?먯옄 洹몃９ ???먯냼 湲고샇).
 * @param atomId ?쒓굅???먯옄??怨좎쑀 ID.
 * @details ?꾨찓???쒓굅 ?깃났 ??batch ?쒖뒪?쒖뿉 洹몃９ ?낅뜲?댄듃瑜??덉빟?⑸땲??
 * @note Affects: atoms::domain::atomGroups, BatchUpdateSystem??pending atom group.
 * @note Called by: (no direct callers found in webassembly/src; legacy wrapper).
 */
void removeAtomFromGroup(const std::string& symbol, uint32_t atomId);


// Forward declaration 異붽?
namespace atoms {
namespace ui {
class ChargeDensityUI;
}
namespace infrastructure {
class ChargeDensityRenderer;
}
}

// ============================================================================
// MAIN ATOMISTIC MODEL BUILDER CLASS
// ============================================================================

/**
 * @brief ?먯옄 紐⑤뜽 鍮뚮뜑??硫붿씤 ?ㅼ??ㅽ듃?덉씠??
 * @details UI, ?꾨찓?? ?뚮뜑???ъ씠??釉뚮━吏 ??븷???섑뻾?섎ŉ BatchUpdateSystem???듯빐 ?뚮뜑 媛깆떊??愿由ы빀?덈떎.
 */
class AtomsTemplate {
    DECLARE_SINGLETON(AtomsTemplate)

public:
    // ========================================================================
    // PUBLIC API
    // ========================================================================


    // ========================================================================
    // Hover / Selection / Tooltip (?먯옄 ?곹샇?묒슜)
    // ========================================================================
    
    /// Actor濡??먯냼 湲고샇 李얘린
    std::string GetSymbolByActor(vtkActor* actor) const;
    
    /// Picker濡??몃쾭???먯옄 ?낅뜲?댄듃
    void UpdateHoveredAtomByPicker(vtkActor* actor, double pickPos[3]);
    
    /// Hover 상태 초기화
    void ClearHover();
    
    /// 원자 툴팁 렌더링
    void RenderAtomTooltip(float mouseX, float mouseY);
    
    /// Picker濡??먯옄 ?좏깮
    void SelectAtomByPicker(vtkActor* actor, double pickPos[3]);
    /// Picker 기준 동일 원소(ORIGINAL) 일괄 선택
    void SelectSameElementAtomsByPicker(vtkActor* actor, double pickPos[3]);
    /// Created atoms 선택 상태를 모두 해제하고 선택 표시를 동기화
    void ClearCreatedAtomSelection();
    /// createdAtoms[].selected 상태를 Viewer 선택 표시와 동기화
    void SyncCreatedAtomSelectionVisualsFromState();
    /// 현재 Measurement 모드에서 Drag Selection 허용 여부
    bool IsMeasurementDragSelectionEnabled() const;
    
    /// 선택 상태 초기화
    void ClearSelection();

    enum class MeasurementMode {
        None = 0,
        Distance = 1,
        Angle = 2,
        Dihedral = 3,
        GeometricCenter = 4,
        CenterOfMass = 5
    };

    enum class MeasurementType {
        Distance = 0,
        Angle = 1,
        Dihedral = 2,
        GeometricCenter = 3,
        CenterOfMass = 4
    };

    struct MeasurementListItem {
        uint32_t id = 0;
        MeasurementType type = MeasurementType::Distance;
        std::string displayName;
        bool visible = true;
    };

    struct DistanceMeasurementListItem {
        uint32_t id = 0;
        std::string displayName;
        bool visible = true;
    };

    void EnterMeasurementMode(MeasurementMode mode);
    void ExitMeasurementMode();
    bool IsMeasurementModeActive() const { return m_MeasurementMode != MeasurementMode::None; }
    MeasurementMode GetMeasurementMode() const { return m_MeasurementMode; }
    void HandleMeasurementClickByPicker(vtkActor* actor, double pickPos[3]);
    void HandleMeasurementEmptyClick();
    void HandleDragSelectionInScreenRect(
        int x0,
        int y0,
        int x1,
        int y1,
        class vtkRenderer* renderer,
        int viewportHeight,
        bool additive);
    void RenderMeasurementModeOverlay();
    std::vector<MeasurementListItem> GetMeasurementsForStructure(int32_t structureId);
    void SetMeasurementVisible(uint32_t measurementId, bool visible);
    void RemoveMeasurement(uint32_t measurementId);
    void RemoveMeasurementsByStructure(int32_t structureId);
    std::vector<DistanceMeasurementListItem> GetDistanceMeasurementsForStructure(int32_t structureId);
    void SetDistanceMeasurementVisible(uint32_t measurementId, bool visible);
    void RemoveDistanceMeasurement(uint32_t measurementId);
    // ========================================================================
    // ========================================================================
    // Node Info (?먯옄 ?뺣낫 ?쒖떆 ?듭뀡)
    // ========================================================================
    
    bool IsNodeInfoEnabled() const { return m_NodeInfoEnabled; }
    void SetNodeInfoEnabled(bool enabled) { m_NodeInfoEnabled = enabled; }
    
    // ========================================================================
    // Charge Density
    // ========================================================================
    
    /// CHGCAR ?뚯씪 濡쒕뱶
    bool LoadChgcarFile(const std::string& filePath);
    bool LoadChgcarParsedData(const atoms::infrastructure::ChgcarParser::ParseResult& result);
    
    /// 전하 밀도 데이터 초기화
    void ClearChargeDensity();
    
    /// ?꾪븯 諛??UI ?묎렐
    atoms::ui::ChargeDensityUI* chargeDensityUI() { return m_chargeDensityUI.get(); }

    // ========================================================================
    // Charge Density Visibility
    // ========================================================================

    bool HasChargeDensity() const;
    bool IsChargeDensityVisible() const;
    void SetChargeDensityVisible(bool visible);
    bool IsChargeDensitySimpleViewActive() const;
    void SyncChargeDensityViewTypeState();
    bool IsChargeDensityAdvancedGridVisible(int32_t meshId, bool volumeMode);
    void SetChargeDensityAdvancedGridVisible(int32_t meshId, bool volumeMode, bool visible);
    void SetAllChargeDensityAdvancedGridVisible(int32_t structureId, bool volumeMode, bool visible);
    void ApplyChargeDensityAdvancedGridVisibilityForStructure(int32_t structureId);
    void SetChargeDensityStructureId(int32_t id);
    
    // ========================================================================

    struct StructureEntry {
        int32_t id = -1;
        std::string name;
        bool visible = true;
    };

    struct UnitCellEntry {
        std::array<std::array<float, 3>, 3> matrix{};
        bool hasCell = false;
        bool visible = true;
    };

    void SetCurrentStructureId(int32_t id) { m_CurrentStructureId = id; }
    int32_t GetCurrentStructureId() const { return m_CurrentStructureId; }
    void RegisterStructure(int32_t id, const std::string& name);
    void RemoveStructure(int32_t id);
    void RemoveUnassignedData();
    bool IsStructureVisible(int32_t id) const;
    void SetStructureVisible(int32_t id, bool visible);
    bool HasStructures() const { return !m_Structures.empty(); }
    std::vector<StructureEntry> GetStructures() const;
    size_t GetAtomCountForStructure(int32_t id) const;
    int32_t GetChargeDensityStructureId() const { return m_ChargeDensityStructureId; }
    bool HasUnitCell(int32_t id) const;
    bool IsUnitCellVisible(int32_t id) const;
    void SetUnitCellVisible(int32_t id, bool visible);


    // ?뚯씪紐?getter/setter
    const std::string& GetLoadedFileName() const { return m_LoadedFileName; }
    void SetLoadedFileName(const std::string& name) { m_LoadedFileName = name; }
    
    // Visibility 愿??(legacy, ?꾩껜 援ъ“)
    bool IsStructureVisible() const { return m_StructureVisible; }
    void SetStructureVisible(bool visible);
    void setCellVisible(bool visible);
    
    // ?먯옄 洹몃９ visibility
    void SetAtomGroupVisible(const std::string& symbol, bool visible);
    bool IsAtomGroupVisible(const std::string& symbol) const;
    bool IsAtomVisibleById(uint32_t atomId) const;
    void SetAtomVisibleById(uint32_t atomId, bool visible);
    void SetAtomVisibilityForIds(const std::vector<uint32_t>& atomIds, bool visible);
    bool IsAtomLabelVisibleById(uint32_t atomId) const;
    void SetAtomLabelVisibleById(uint32_t atomId, bool visible);
    void SetAtomLabelVisibilityForIds(const std::vector<uint32_t>& atomIds, bool visible);

    // Bond visibility
    void SetBondsVisible(bool visible);
    bool IsBondsVisible() const { return m_BondsVisible; }
    void SetBondsVisible(int32_t id, bool visible);
    bool IsBondsVisible(int32_t id) const;
    bool IsBondVisibleById(uint32_t bondId) const;
    void SetBondVisibleById(uint32_t bondId, bool visible);
    void SetBondVisibilityForIds(const std::vector<uint32_t>& bondIds, bool visible);
    bool IsBondLabelVisibleById(uint32_t bondId) const;
    void SetBondLabelVisibleById(uint32_t bondId, bool visible);
    void SetBondLabelVisibilityForIds(const std::vector<uint32_t>& bondIds, bool visible);
    


    
    // ?먯옄 洹몃９ ?뺣낫 議고쉶
    std::vector<std::pair<std::string, size_t>> GetAtomGroupSummary() const;

    enum class BuilderSectionRequest {
        BravaisLatticeTemplates = 0,
        AddAtoms = 1,
        BrillouinZone = 2
    };

    enum class EditorSectionRequest {
        Atoms = 0,
        Bonds = 1,
        Cell = 2
    };

    enum class DataMenuRequest {
        Isosurface = 0,
        Surface = 1,
        Volumetric = 2,
        Plane = 3
    };

    void RequestBuilderSection(BuilderSectionRequest section);
    void RequestEditorSection(EditorSectionRequest section);
    void RequestDataMenu(DataMenuRequest request);
    /**
     * @brief Crystal Builder / Crystal Editor UI瑜??뚮뜑留곹빀?덈떎.
     * @param openBuilder ImGui ?덈룄???대┝ ?곹깭 ?ъ씤???듭뀡).
     * @param openEditor ImGui ?덈룄???대┝ ?곹깭 ?ъ씤???듭뀡).
     * @details Builder??Crystal Templates/Periodic Table/BZ Plot??
     *          Editor??Created Atoms/Bonds/Cell Information???쒖감 ?뚮뜑留곹빀?덈떎.
     * @note Affects: ImGui UI ?곹깭, UI 而댄룷?뚰듃 ?뚮뜑 ?몄텧.
     * @note Called by: webassembly/src/app.cpp (App::renderImGuiWindows).
     */
    void Render(bool* openBuilder = nullptr, bool* openEditor = nullptr);
    void RenderAdvancedView(bool* openAdvanced = nullptr);
    void RenderCrystalTemplatesWindow(bool* openWindow = nullptr);
    void RenderPeriodicTableWindow(bool* openWindow = nullptr);
    void RenderBrillouinZonePlotWindow(bool* openWindow = nullptr);
    void RenderCreatedAtomsWindow(bool* openWindow = nullptr);
    void RenderBondsManagementWindow(bool* openWindow = nullptr);
    void RenderCellInformationWindow(bool* openWindow = nullptr);
    void RenderChargeDensityViewerWindow(bool* openWindow = nullptr);
    void RenderSliceViewerWindow(bool* openWindow = nullptr);
    void RequestForcedBuilderWindowLayout(const ImVec2& pos, const ImVec2& size, int frames = 1);
    void RequestForcedEditorWindowLayout(const ImVec2& pos, const ImVec2& size, int frames = 1);
    void RequestForcedAdvancedWindowLayout(const ImVec2& pos, const ImVec2& size, int frames = 1);

    /**
     * @brief XSF ?뚯씪??濡쒕뱶?섏뿬 援ъ“瑜?珥덇린?뷀빀?덈떎.
     * @param filePath XSF ?뚯씪 寃쎈줈.
     * @details BatchGuard濡?諛곗튂 紐⑤뱶瑜??닿퀬 湲곗〈 ?먯옄/寃고빀/????뺣━????FileIOManager濡?珥덇린 援ъ“瑜??앹꽦?⑸땲??
     * @note Affects: createdAtoms, surroundingAtoms, atoms::domain::atomGroups, atoms::domain::bondGroups, cellInfo, m_vtkRenderer.
     * @note Called by: webassembly/src/file_loader.cpp (FileLoader::handleXSFFile).
     */
    void LoadXSFFile(const std::string& filePath);
    bool LoadXSFParsedData(const atoms::infrastructure::FileIOManager::ParseResult& result, bool renderCell = true);

    /**
     * @brief XSF ?뚯씪???뚯떛????寃곌낵濡?援ъ“瑜?珥덇린?뷀빀?덈떎.
     * @param filePath XSF ?뚯씪 寃쎈줈.
     * @details FileIOManager::loadXSFFile()怨?initializeStructure()??2?④퀎 珥덇린?붾? ?섑뻾?⑸땲??
     * @note Affects: cellInfo, createdAtoms, atoms::domain::atomGroups, m_vtkRenderer.
     * @note Called by: (no direct callers found in webassembly/src).
     */
    void ParseXSFFileWithManager(const std::string& filePath);

    // ========================================================================
    // FILE I/O LAYER - Phase 3?먯꽌 FileIOManager濡??대룞
    // ========================================================================

    /**
     * @brief 紐⑤뱺 ?듯빀 ?먯옄 洹몃９怨?VTK ?몄뒪?댁떛 ?곗씠?곕? ?뺣━?⑸땲??
     * @details VTK ?뚯씠?꾨씪???뺣━ ???꾨찓??atomGroups瑜?珥덇린?뷀빀?덈떎.
     * @note Affects: atoms::domain::atomGroups, m_vtkRenderer.
     * @note Called by: AtomsTemplate::LoadXSFFile, AtomsTemplate::setBravaisLattice.
     */
    void clearAllUnifiedAtomGroups();

    // ========================================================================
    // STATE / VISIBILITY
    // ========================================================================

    /**
     * @brief Unit Cell 媛?쒖꽦 ?곹깭瑜?諛섑솚?⑸땲??
     * @return true?대㈃ Unit Cell???붾㈃???쒖떆??
     * @details atoms::domain::isCellVisible()???뉗? ?섑띁?낅땲??
     * @note Affects: (read-only).
     * @note Called by: webassembly/src/atoms/ui/bz_plot_ui.cpp
     *                  (atoms::ui::BZPlotUI::renderBZplot).
     */
    bool isCellVisible() const;

    /**
     * @brief Unit Cell???뺤쓽?섏뿀?붿? ?뺤씤?⑸땲??
     * @return true?대㈃ ? ?뺣낫媛 ?뺤쓽??
     * @details atoms::domain::hasUnitCell()???뉗? ?섑띁?낅땲??
     * @note Affects: (read-only).
     * @note Called by: webassembly/src/atoms/ui/periodic_table_ui.cpp
     *                  (atoms::ui::PeriodicTableUI::renderSelectedElementDetails).
     */
    bool hasUnitCell() const;

    /**
     * @brief 二쇰? ?먯옄 ?쒖떆 ?щ?瑜?議고쉶?⑸땲??
     * @return 二쇰? ?먯옄媛 ?쒖떆 以묒씠硫?true.
     * @note Affects: (read-only) surroundingsVisible.
     * @note Called by: webassembly/src/atoms/ui/atom_editor_ui.cpp
     *                  (atoms::ui::AtomEditorUI::render),
     *                  webassembly/src/atoms/domain/atom_manager.cpp
     *                  (atoms::domain::applyAtomChanges),
     *                  webassembly/src/atoms/domain/bond_manager.cpp
     *                  (atoms::domain::createBondsForAtoms,
     *                   atoms::domain::addBondsToGroups),
     *                  webassembly/src/atoms/domain/surrounding_atom_manager.cpp
     *                  (atoms::domain::SurroundingAtomManager::createSurroundingAtoms,
     *                   atoms::domain::SurroundingAtomManager::isVisible),
     *                  webassembly/src/atoms/domain/cell_manager.cpp
     *                  (atoms::domain::applyCellChanges).
     */
    bool isSurroundingsVisible() const;

    /**
     * @brief 二쇰? ?먯옄 ?쒖떆 ?щ?瑜??ㅼ젙?⑸땲??
     * @param visible ?쒖떆?좎? ?щ?.
     * @details UI ?곹깭? ?꾨찓??濡쒖쭅??李몄“?섎뒗 ?뚮옒洹?surroundingsVisible)瑜?媛깆떊?⑸땲??
     * @note Affects: surroundingsVisible.
     * @note Called by: webassembly/src/atoms/ui/atom_editor_ui.cpp
     *                  (atoms::ui::AtomEditorUI::render),
     *                  webassembly/src/atoms/domain/surrounding_atom_manager.cpp
     *                  (atoms::domain::SurroundingAtomManager::createSurroundingAtoms,
     *                   atoms::domain::SurroundingAtomManager::hideSurroundingAtoms,
     *                   atoms::domain::SurroundingAtomManager::setVisible).
     */
    void setSurroundingsVisible(bool visible);
    void SetBoundaryAtomsEnabled(bool enabled);
    bool IsBZPlotMode() const;

    /**
     * @brief Bravais 寃⑹옄 ?ㅼ젙 諛?援ъ“瑜??앹꽦?⑸땲??
     * @param type 寃⑹옄 ???
     * @param params 寃⑹옄 ?뚮씪誘명꽣.
     * @param preserveExistingAtoms true硫?湲곗〈 ?먯옄 fractional 醫뚰몴 ?좎?, false硫?湲곕낯 援ъ“濡?珥덇린??
     * @details 寃⑹옄 踰≫꽣 怨꾩궛 ??Unit Cell 媛깆떊 ??湲곗〈 ?먯옄 ?좎? ?먮뒗 湲곕낯 ?먯옄 ?앹꽦 ??寃고빀 ?ъ깮??
     * @note Affects: createdAtoms, bondGroups, cellInfo, m_vtkRenderer, m_batchSystem.
     * @note Called by: webassembly/src/atoms/ui/bravais_lattice_ui.cpp
     *                  (atoms::ui::BravaisLatticeUI::onLatticeSelected).
     */
    void setBravaisLattice(
        atoms::domain::BravaisLatticeType type,
        const atoms::domain::BravaisParameters& params,
        bool preserveExistingAtoms = true
    );

    // ========================================================================
    // ATOM RENDERING SYSTEM VTK ?명꽣?섏씠??    // ========================================================================

    /**
     * @brief ?먯옄 洹몃９??VTK ?뚯씠?꾨씪?몄쓣 珥덇린?뷀빀?덈떎.
     * @param symbol ?먯옄 洹몃９ ??
     * @param radius 洹몃９ 湲곗? 諛섏?由?
     * @details VTKRenderer???먯옄 洹몃９??珥덇린?뷀븯?꾨줉 ?꾩엫?⑸땲??
     * @note Affects: m_vtkRenderer??atom group VTK ?뚯씠?꾨씪??
     * @note Called by: ::initializeAtomGroup.
     */
    void initializeUnifiedAtomGroupVTK(const std::string& symbol, float radius);

    /**
     * @brief ?먯옄 洹몃９??VTK ?몄뒪?댁떛 ?곗씠?곕? 媛깆떊?⑸땲??
     * @param symbol ?먯옄 洹몃９ ??
     * @details ?꾨찓??atomGroups瑜?議고쉶?섏뿬 ?뚮뜑?ъ뿉 transforms/colors瑜??꾨떖?⑸땲??
     * @note Affects: m_vtkRenderer??atom group VTK ?곗씠??
     * @note Called by: ::updateUnifiedAtomGroupVTK,
     *                  webassembly/src/atoms/infrastructure/batch_update_system.cpp
     *                  (atoms::infrastructure::BatchUpdateSystem::endBatch).
     */
    void updateUnifiedAtomGroupVTK(const std::string& symbol);

    /**
     * @brief 寃고빀 洹몃９??VTK ?뚯씠?꾨씪?몄쓣 珥덇린?뷀빀?덈떎.
     * @param bondTypeKey 寃고빀 洹몃９ ???? "H-O").
     * @param radius 寃고빀 諛섏?由?湲곗?).
     * @details BondRenderer媛 ?덉쑝硫??곗꽑 ?ъ슜?섍퀬, ?놁쑝硫?VTKRenderer濡??꾩엫?⑸땲??
     * @note Affects: m_bondRenderer/m_vtkRenderer??bond group ?뚯씠?꾨씪??
     * @note Called by: webassembly/src/atoms/domain/bond_manager.cpp
     *                  (initializeBondGroupRenderer),
     *                  ::initializeBondGroup.
     */
    void initializeBondGroupVTK(const std::string& bondTypeKey, float radius);

    /**
     * @brief 寃고빀 洹몃９??VTK ?몄뒪?댁떛 ?곗씠?곕? 媛깆떊?⑸땲??
     * @param bondTypeKey 寃고빀 洹몃９ ??
     * @details ?꾨찓??bondGroups瑜?議고쉶?섏뿬 ?뚮뜑?ъ뿉 ?곗씠?곕? ?꾨떖?⑸땲??
     * @note Affects: m_bondRenderer/m_vtkRenderer??bond group VTK ?곗씠??
     * @note Called by: webassembly/src/atoms/domain/bond_manager.cpp
     *                  (updateBondGroupRenderer),
     *                  webassembly/src/atoms/infrastructure/batch_update_system.cpp
     *                  (atoms::infrastructure::BatchUpdateSystem::endBatch).
     */
    void updateBondGroupVTK(const std::string& bondTypeKey);

    /**
     * @brief 紐⑤뱺 寃고빀 洹몃９??VTK ?뚯씠?꾨씪?몄쓣 ?뺣━?⑸땲??
     * @details BondRenderer ?먮뒗 VTKRenderer???뺣━ ?묒뾽???꾩엫?⑸땲??
     * @note Affects: m_bondRenderer/m_vtkRenderer??bond group VTK ?뚯씠?꾨씪??
     * @note Called by: ::clearAllBondGroups,
     *                  webassembly/src/atoms/domain/bond_manager.cpp
     *                  (clearAllBondGroupsRenderer).
     */
    void clearAllBondGroupsVTK();

    /**
     * @brief ?꾩옱 諛곗튂 紐⑤뱶 ?곹깭瑜??뺤씤?⑸땲??
     * @return 諛곗튂 紐⑤뱶?대㈃ true.
     * @details BatchUpdateSystem?쇰줈 ?꾩엫?섎뒗 ?묎렐?먯엯?덈떎.
     * @note Affects: (read-only).
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render),
     *                  webassembly/src/atoms/ui/atom_editor_ui.cpp
     *                  (atoms::ui::AtomEditorUI::render),
     *                  webassembly/src/atoms/domain/bond_manager.cpp
     *                  (updateBondGroupRenderer,
     *                   atoms::domain::createBondsForAtoms,
     *                   atoms::domain::addBondsToGroups).
     */
    bool isBatchMode() const { 
        return m_batchSystem ? m_batchSystem->isBatchMode() : false; 
    }

    // ========================================================================
    // BatchGuard ???蹂꾩묶 諛??⑺넗由?(湲곗〈 肄붾뱶 ?명솚??
    // ========================================================================

    /**
     * @brief BatchUpdateSystem::BatchGuard???명솚 蹂꾩묶?낅땲??
     * @details RAII 湲곕컲?쇰줈 begin/end 諛곗튂 泥섎━瑜??섑뻾?⑸땲??
     */
    using BatchGuard = atoms::infrastructure::BatchUpdateSystem::BatchGuard;

    /**
     * @brief 諛곗튂 媛???앹꽦 ?⑺넗由??⑥닔?낅땲??
     * @return ?앹꽦??BatchGuard.
     * @details m_batchSystem ?ъ씤?곕? ?꾨떖?섏뿬 諛곗튂 紐⑤뱶瑜?愿由ы빀?덈떎.
     * @note Affects: BatchUpdateSystem??諛곗튂 ?곹깭.
     * @note Called by: AtomsTemplate::LoadXSFFile, AtomsTemplate::applyAtomChanges,
     *                  AtomsTemplate::applyCellChanges,
     *                  webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    BatchGuard createBatchGuard() { 
        return BatchGuard(m_batchSystem.get()); 
    }

    // ========================================================================
    // BOND LOGIC LAYER (遺꾨━ ?꾨낫 - BondLogic ?대옒??
    // ========================================================================

    /**
     * @brief 吏?뺣맂 ?먯옄???ъ씠??寃고빀???앹꽦?⑸땲??
     * @param atomIds 寃고빀 ????먯옄 ID 紐⑸줉(鍮꾩뼱?덉쑝硫??꾩껜).
     * @param includeOriginal ?먮낯 ?먯옄 ?ы븿 ?щ?.
     * @param includeSurrounding 二쇰? ?먯옄 ?ы븿 ?щ?.
     * @param clearExisting 湲곗〈 寃고빀 ?쒓굅 ?щ?.
     * @details ?꾨찓??bond_manager濡??꾩엫?섏뿬 寃고빀 諛?洹몃９ ?곗씠?곕? ?앹꽦?⑸땲??
     * @note Affects: atoms::domain::bondGroups, createdBonds, surroundingBonds, ?뚮뜑 媛깆떊 ?ㅼ?以?
     * @note Called by: webassembly/src/atoms/domain/surrounding_atom_manager.cpp
     *                  (atoms::domain::SurroundingAtomManager::createSurroundingAtoms,
     *                   atoms::domain::SurroundingAtomManager::hideSurroundingAtoms).
     */
    void createBondsForAtoms(
        const std::vector<uint32_t>& atomIds,  // ?뵩 ???蹂寃? size_t ??uint32_t
        bool includeOriginal,                  // ?먮낯 ?먯옄 ?ы븿 ?щ?
        bool includeSurrounding,               // 二쇰? ?먯옄 ?ы븿 ?щ?
        bool clearExisting                     // 湲곗〈 寃고빀 ?쒓굅 ?щ?
    );

    /**
     * @brief ?꾩껜 ?먯옄?????寃고빀???앹꽦?⑸땲??
     * @details createBondsForAtoms({}, true, true, true)瑜??몄텧?⑸땲??
     * @note Affects: atoms::domain::bondGroups, createdBonds, surroundingBonds.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render),
     *                  webassembly/src/atoms/infrastructure/file_io_manager.cpp
     *                  (atoms::infrastructure::FileIOManager::initializeStructure),
     *                  webassembly/src/atoms/domain/atom_manager.cpp
     *                  (atoms::domain::applyAtomChanges).
     */
    void createAllBonds();

    /**
     * @brief 紐⑤뱺 寃고빀???쒓굅?⑸땲??
     * @details ?꾨찓??clearAllBonds()濡??꾩엫?⑸땲??
     * @note Affects: atoms::domain::bondGroups, createdBonds, surroundingBonds, VTK 寃고빀 ?뚯씠?꾨씪??
     * @note Called by: AtomsTemplate::LoadXSFFile, AtomsTemplate::setBravaisLattice,
     *                  webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    void clearAllBonds();

    /**
     * @brief ???먯옄 ?ъ씠??寃고빀???앹꽦?⑸땲??
     * @param atom1 泥?踰덉㎏ ?먯옄 ?뺣낫.
     * @param atom2 ??踰덉㎏ ?먯옄 ?뺣낫.
     * @param bondType 寃고빀 ???
     * @details ?꾨찓??bond_manager::createBond濡??꾩엫?⑸땲??
     * @note Affects: atoms::domain::bondGroups, createdBonds/surroundingBonds.
     * @note Called by: (no direct callers found in webassembly/src).
     */
    void createBond(
        const atoms::domain::AtomInfo& atom1, 
        const atoms::domain::AtomInfo& atom2, 
        atoms::domain::BondType bondType
    );

    /**
     * @brief 寃고빀 ID瑜?怨좎쑀?섍쾶 諛쒓툒?⑸땲??
     * @return 怨좎쑀 寃고빀 ID.
     * @note Affects: nextBondId (static, atoms_template.cpp).
     * @note Called by: webassembly/src/atoms/domain/bond_manager.cpp
     *                  (atoms::domain::createBond).
     */
    uint32_t generateUniqueBondId();

    /**
     * @brief Reset bond serial counter.
     * @param startId Counter start value (default: 1).
     * @note Use only when all bonds are cleared/rebuilt.
     */
    void resetBondIdCounter(uint32_t startId = 1);

    /**
     * @brief ?꾩옱 寃고빀 珥?媛쒖닔瑜?怨꾩궛?⑸땲??
     * @return 寃고빀 珥?媛쒖닔.
     * @details 洹몃９ 寃고빀 + 媛쒕퀎 寃고빀???⑹궛?⑸땲??
     * @note Affects: (read-only) bondGroups/createdBonds/surroundingBonds.
     * @note Called by: AtomsTemplate::updatePerformanceStatsInternal.
     */
    int getTotalBondCount();

    /**
     * @brief ?뱀젙 ?먯옄 ?몃뜳??湲곗??쇰줈 寃고빀 洹몃９??媛깆떊?⑸땲??
     * @param atomIndex ????먯옄 ?몃뜳??
     * @details ?꾨찓??addBondsToGroups()濡??꾩엫?⑸땲??
     * @note Affects: atoms::domain::bondGroups.
     * @note Called by: (no direct callers found in webassembly/src).
     */
    void addBondsToGroups(int atomIndex);

    /**
     * @brief 紐⑤뱺 寃고빀 洹몃９???먭퍡瑜??낅뜲?댄듃?⑸땲??
     * @details ?꾨찓??updateAllBondGroupThickness()濡??꾩엫?⑸땲??
     * @note Affects: atoms::domain::bondGroups, VTK bond group ?뚯씠?꾨씪??
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    void updateAllBondGroupThickness();

    /**
     * @brief 紐⑤뱺 寃고빀 洹몃９??遺덊닾紐낅룄瑜??낅뜲?댄듃?⑸땲??
     * @details ?꾨찓??updateAllBondGroupOpacity()濡??꾩엫?⑸땲??
     * @note Affects: atoms::domain::bondGroups, VTK bond group ?뚯씠?꾨씪??
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    void updateAllBondGroupOpacity();

    /**
     * @brief 寃고빀 ?뚮뜑留??깅뒫 ?듦퀎 援ъ“泥?
     * @details BatchUpdateSystem 媛깆떊 ?쒖젏???듦퀎瑜?UI濡??꾨떖?섎뒗 ?⑸룄?낅땲??
     */
    struct BondPerfStats {
        float lastUpdateTime = 0.0f;  ///< 留덉?留?諛곗튂 媛깆떊 ?쒓컙(ms).
        float averageUpdateTime = 0.0f; ///< ?대룞 ?됯퇏 媛깆떊 ?쒓컙(ms).
        int   updateCount = 0; ///< ?깅뒫 ?듦퀎 ?낅뜲?댄듃 ?잛닔.
    };

    /**
     * @brief 寃고빀 ?먭퍡 媛믪쓣 諛섑솚?⑸땲??
     * @return 寃고빀 ?먭퍡.
     * @note Affects: (read-only) bondThickness.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    float getBondThickness() const;

    /**
     * @brief 寃고빀 ?먭퍡 媛믪쓣 ?ㅼ젙?⑸땲??
     * @param value ?ㅼ젙???먭퍡.
     * @note Affects: bondThickness.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    void  setBondThickness(float value);

    /**
     * @brief 寃고빀 遺덊닾紐낅룄 媛믪쓣 諛섑솚?⑸땲??
     * @return 寃고빀 遺덊닾紐낅룄.
     * @note Affects: (read-only) bondOpacity.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    float getBondOpacity() const;

    /**
     * @brief 寃고빀 遺덊닾紐낅룄 媛믪쓣 ?ㅼ젙?⑸땲??
     * @param value ?ㅼ젙??遺덊닾紐낅룄.
     * @note Affects: bondOpacity.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    void  setBondOpacity(float value);

    /**
     * @brief 寃고빀 ?ㅼ????⑺꽣瑜?諛섑솚?⑸땲??
     * @return ?ㅼ????⑺꽣.
     * @note Affects: (read-only) bondScalingFactor.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    float getBondScalingFactor() const;

    /**
     * @brief 寃고빀 ?ㅼ????⑺꽣瑜??ㅼ젙?⑸땲??
     * @param value ?ㅼ젙???ㅼ????⑺꽣.
     * @note Affects: bondScalingFactor.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    void  setBondScalingFactor(float value);

    /**
     * @brief 寃고빀 ?덉슜 ?ㅼ감 ?⑺꽣瑜?諛섑솚?⑸땲??
     * @return ?덉슜 ?ㅼ감 ?⑺꽣.
     * @note Affects: (read-only) bondToleranceFactor.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    float getBondToleranceFactor() const;

    /**
     * @brief 寃고빀 ?덉슜 ?ㅼ감 ?⑺꽣瑜??ㅼ젙?⑸땲??
     * @param value ?ㅼ젙???덉슜 ?ㅼ감 ?⑺꽣.
     * @note Affects: bondToleranceFactor.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    void  setBondToleranceFactor(float value);

    /**
     * @brief 寃고빀 ?뚮뜑留??깅뒫 ?듦퀎瑜?諛섑솚?⑸땲??
     * @return ?깅뒫 ?듦퀎 ?ㅻ깄??
     * @details BatchUpdateSystem??媛깆떊??g_performanceStats瑜?蹂듭궗?⑸땲??
     * @note Affects: (read-only) g_performanceStats.
     * @note Called by: webassembly/src/atoms/ui/bond_ui.cpp
     *                  (atoms::ui::BondUI::render).
     */
    BondPerfStats getBondPerformanceStats() const;

    // ========================================================================
    // ATOM CREATION LAYER (遺꾨━ ?꾨낫 - AtomFactory ?대옒??
    // ========================================================================

    /**
     * @brief ?먯옄 ?섎굹瑜??앹꽦?섍퀬 ?듯빀 洹몃９ ?쒖뒪?쒖뿉 ?깅줉?⑸땲??
     * @param symbol ?먯냼 湲고샇.
     * @param color ?먯옄 ?됱긽.
     * @param radius ?먯옄 諛섏?由?
     * @param position ?곗뭅瑜댄듃 醫뚰몴(3?붿냼).
     * @param atomType ?먯옄 ???ORIGINAL/SURROUNDING).
     * @details ?꾨찓??洹몃９??異붽?????AtomInfo瑜??앹꽦?섍퀬 諛곗튂 ?낅뜲?댄듃瑜??ㅼ?以꾨쭅?⑸땲??
     * @note Affects: createdAtoms/surroundingAtoms, atoms::domain::atomGroups, BatchUpdateSystem.
     * @note Called by: AtomsTemplate::setBravaisLattice,
     *                  webassembly/src/atoms/ui/periodic_table_ui.cpp
     *                  (atoms::ui::PeriodicTableUI::renderSelectedElementDetails),
     *                  webassembly/src/atoms/infrastructure/file_io_manager.cpp
     *                  (atoms::infrastructure::FileIOManager::initializeStructure),
     *                  webassembly/src/atoms/domain/surrounding_atom_manager.cpp
     *                  (atoms::domain::SurroundingAtomManager::createSurroundingAtoms).
     */
    void createAtomSphere(
        const char* symbol, 
        // const ImVec4& color, 
        const atoms::domain::Color4f& color,
        float radius, 
        const float position[3], 
        atoms::domain::AtomType atomType = atoms::domain::AtomType::ORIGINAL
    );

    // ========================================================================
    // UNIT CELL LAYER (遺꾨━ ?꾨낫 - UnitCellManager ?대옒??
    // ========================================================================

    /**
     * @brief Unit Cell ?뚮뜑留??곗씠?곕? ?뺣━?⑸땲??
     * @details VTKRenderer ?뺣━ 諛??꾨찓??媛?쒖꽦 ?뚮옒洹몃? 媛깆떊?⑸땲??
     * @note Affects: m_vtkRenderer??unit cell ?뚯씠?꾨씪?? atoms::domain::cellVisible.
     * @note Called by: AtomsTemplate::LoadXSFFile, AtomsTemplate::setBravaisLattice,
     *                  webassembly/src/atoms/domain/cell_manager.cpp
     *                  (atoms::domain::applyCellChanges).
     */
    void clearUnitCell();

    /**
     * @brief Unit Cell???앹꽦?섍퀬 ?뚮뜑留곸쓣 ?쒖꽦?뷀빀?덈떎.
     * @param matrix 3x3 ? ?됰젹.
     * @details ?꾨찓??cell matrix瑜??ㅼ젙?섍퀬 VTKRenderer??unit cell ?앹꽦 ?붿껌??蹂대깄?덈떎.
     * @note Affects: cellInfo, m_vtkRenderer??unit cell ?뚯씠?꾨씪?? atoms::domain::cellVisible.
     * @note Called by: AtomsTemplate::setBravaisLattice,
     *                  webassembly/src/atoms/infrastructure/file_io_manager.cpp
     *                  (atoms::infrastructure::FileIOManager::initializeStructure),
     *                  webassembly/src/atoms/domain/cell_manager.cpp
     *                  (atoms::domain::applyCellChanges).
     */
    void createUnitCell(const float matrix[3][3]);

    // ========================================================================
    // SURROUNDING ATOMS LAYER (遺꾨━ ?꾨낫 - SurroundingAtomManager ?대옒??
    // ========================================================================

    /**
     * @brief 二쇰? ?먯옄瑜??앹꽦?⑸땲??
     * @details SurroundingAtomManager濡??꾩엫?섎ŉ, 二쇰? ?먯옄/寃고빀 ?앹꽦 諛?媛?쒖꽦 ?뚮옒洹?媛깆떊???섑뻾?⑸땲??
     * @note Affects: surroundingAtoms, surroundingBonds, atoms::domain::atomGroups, bondGroups.
     * @note Called by: webassembly/src/atoms/ui/atom_editor_ui.cpp
     *                  (atoms::ui::AtomEditorUI::render),
     *                  webassembly/src/atoms/domain/atom_manager.cpp
     *                  (atoms::domain::applyAtomChanges),
     *                  webassembly/src/atoms/domain/cell_manager.cpp
     *                  (atoms::domain::applyCellChanges).
     */
    void createSurroundingAtoms();

    /**
     * @brief 二쇰? ?먯옄瑜??④린怨?愿??寃고빀???뺣━?⑸땲??
     * @details SurroundingAtomManager濡??꾩엫?섎ŉ, 二쇰? ?먯옄/寃고빀 ?쒓굅 諛?媛?쒖꽦 ?뚮옒洹몃? 媛깆떊?⑸땲??
     * @note Affects: surroundingAtoms, surroundingBonds, atoms::domain::atomGroups, bondGroups.
     * @note Called by: webassembly/src/atoms/ui/atom_editor_ui.cpp
     *                  (atoms::ui::AtomEditorUI::render),
     *                  webassembly/src/atoms/domain/atom_manager.cpp
     *                  (atoms::domain::applyAtomChanges),
     *                  webassembly/src/atoms/domain/cell_manager.cpp
     *                  (atoms::domain::applyCellChanges).
     */
    void hideSurroundingAtoms();

    void resetStructure(); // private?먯꽌 public?쇰줈 蹂寃?
    // (異붽?) atoms_template.h - public:

    /**
     * @brief VTKRenderer ?묎렐?먮? 諛섑솚?⑸땲??
     * @return VTKRenderer ?ъ씤???놁쑝硫?nullptr).
     * @note Affects: (read-only) m_vtkRenderer.
     * @note Called by: webassembly/src/atoms/domain/bz_plot.cpp
     *                  (atoms::domain::BZPlotController::enterBZPlotMode,
     *                   atoms::domain::BZPlotController::exitBZPlotMode),
     *                  webassembly/src/atoms/domain/bond_manager.cpp
     *                  (atoms::domain::updateAllBondGroupThickness,
     *                   atoms::domain::updateAllBondGroupOpacity).
     */
    atoms::infrastructure::VTKRenderer* vtkRenderer() noexcept {
        return m_vtkRenderer.get();
    }

    /**
     * @brief VTKRenderer ?곸닔 ?묎렐?먮? 諛섑솚?⑸땲??
     * @return VTKRenderer ?곸닔 ?ъ씤??
     * @note Affects: (read-only) m_vtkRenderer.
     */
    const atoms::infrastructure::VTKRenderer* vtkRenderer() const noexcept {
        return m_vtkRenderer.get();
    }

    /**
     * @brief BondRenderer ?묎렐?먮? 諛섑솚?⑸땲??
     * @return BondRenderer ?ъ씤???놁쑝硫?nullptr).
     * @note Affects: (read-only) m_bondRenderer.
     * @note Called by: webassembly/src/atoms/domain/bond_manager.cpp
     *                  (initializeBondGroupRenderer,
     *                   updateBondGroupRenderer,
     *                   clearAllBondGroupsRenderer).
     */
    atoms::infrastructure::BondRenderer* bondRenderer() noexcept {
        return m_bondRenderer.get();
    }

    /**
     * @brief BondRenderer ?곸닔 ?묎렐?먮? 諛섑솚?⑸땲??
     * @return BondRenderer ?곸닔 ?ъ씤??
     * @note Affects: (read-only) m_bondRenderer.
     */
    const atoms::infrastructure::BondRenderer* bondRenderer() const noexcept {
        return m_bondRenderer.get();
    }

    /**
     * @brief BatchUpdateSystem ?묎렐?먮? 諛섑솚?⑸땲??
     * @return BatchUpdateSystem ?ъ씤???놁쑝硫?nullptr).
     * @note Affects: (read-only) m_batchSystem.
     * @note Called by: webassembly/src/atoms/domain/atom_manager.cpp
     *                  (atoms::domain::applyAtomChanges),
     *                  webassembly/src/atoms/domain/bond_manager.cpp
     *                  (updateBondGroupRenderer,
     *                   atoms::domain::updateAllBondGroupThickness,
     *                   atoms::domain::updateAllBondGroupOpacity),
     *                  webassembly/src/atoms/domain/surrounding_atom_manager.cpp
     *                  (atoms::domain::SurroundingAtomManager::createSurroundingAtoms,
     *                   atoms::domain::SurroundingAtomManager::hideSurroundingAtoms).
     */
    atoms::infrastructure::BatchUpdateSystem* batchSystem() noexcept {
        return m_batchSystem.get();
    }

    /**
     * @brief BatchUpdateSystem ?곸닔 ?묎렐?먮? 諛섑솚?⑸땲??
     * @return BatchUpdateSystem ?곸닔 ?ъ씤??
     * @note Affects: (read-only) m_batchSystem.
     */
    const atoms::infrastructure::BatchUpdateSystem* batchSystem() const noexcept {
        return m_batchSystem.get();
    }

private:
    // ========================================================================
    // 諛곗튂 ?쒖뒪???명꽣?섏씠??(?대? ?꾩슜)
    // ========================================================================

    std::string m_LoadedFileName;  // 濡쒕뱶??XSF ?뚯씪紐?
    bool m_StructureVisible = true;
    std::map<std::string, bool> m_AtomGroupVisibility;  // 洹몃visibility ?곹깭
    std::unordered_map<uint32_t, bool> m_AtomVisibilityById;
    std::unordered_map<uint32_t, bool> m_AtomLabelVisibilityById;
    bool m_BondsVisible = true;  // Bond ?꾩껜 visibility
    std::unordered_map<uint32_t, bool> m_BondVisibilityById;
    std::unordered_map<uint32_t, bool> m_BondLabelVisibilityById;

    int32_t m_CurrentStructureId = -1;
    std::unordered_map<int32_t, StructureEntry> m_Structures;
    std::unordered_map<std::string, std::unordered_set<int32_t>> m_RenderedAtomGroupIds;
    std::unordered_map<std::string, std::unordered_set<int32_t>> m_RenderedBondGroupIds;
    int32_t m_ChargeDensityStructureId = -1;
    std::unordered_map<int32_t, UnitCellEntry> m_UnitCells;
    std::unordered_map<int32_t, bool> m_StructureBondsVisible;

    // ========================================================================
    // Hover / Selection ?곹깭
    // ========================================================================
    
    struct HoveredAtomInfo {
        bool isHovered = false;
        std::string symbol;
        std::string displayLabel;
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };
    HoveredAtomInfo m_HoveredAtom;
    
    struct SelectedAtomInfo {
        bool isSelected = false;
        std::string symbol;
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };
    SelectedAtomInfo m_SelectedAtom;
    vtkActor* m_SelectedActor = nullptr;

    struct DistanceMeasurement {
        uint32_t id = 0;
        int32_t structureId = -1;
        uint32_t atomId1 = 0;
        uint32_t atomId2 = 0;
        std::string displayName;
        double distance = 0.0;
        bool visible = true;
        vtkSmartPointer<vtkActor> lineActor;
        vtkSmartPointer<vtkActor2D> textActor;
    };
    struct AngleMeasurement {
        uint32_t id = 0;
        int32_t structureId = -1;
        uint32_t atomId1 = 0;
        uint32_t atomId2 = 0;
        uint32_t atomId3 = 0;
        std::string displayName;
        double angleDeg = 0.0;
        bool visible = true;
        vtkSmartPointer<vtkActor> lineActor12;
        vtkSmartPointer<vtkActor> lineActor23;
        vtkSmartPointer<vtkActor> arcActor;
        vtkSmartPointer<vtkActor2D> textActor;
    };
    struct DihedralMeasurement {
        uint32_t id = 0;
        int32_t structureId = -1;
        uint32_t atomId1 = 0;
        uint32_t atomId2 = 0;
        uint32_t atomId3 = 0;
        uint32_t atomId4 = 0;
        std::string displayName;
        double dihedralDeg = 0.0;
        bool visible = true;
        vtkSmartPointer<vtkActor> lineActor12;
        vtkSmartPointer<vtkActor> lineActor23;
        vtkSmartPointer<vtkActor> lineActor34;
        vtkSmartPointer<vtkActor> helperPlaneActor1;
        vtkSmartPointer<vtkActor> helperPlaneActor2;
        vtkSmartPointer<vtkActor> helperArcActor;
        vtkSmartPointer<vtkActor2D> textActor;
    };
    struct CenterMeasurement {
        uint32_t id = 0;
        int32_t structureId = -1;
        MeasurementType type = MeasurementType::GeometricCenter;
        std::vector<uint32_t> atomIds;
        std::array<double, 3> centerPos { 0.0, 0.0, 0.0 };
        std::string displayName;
        bool visible = true;
        size_t styleIndex = 0;
        vtkSmartPointer<vtkActor2D> centerBackgroundCircleActor;
        vtkSmartPointer<vtkActor2D> centerPlusTextActor;
        vtkSmartPointer<vtkActor2D> centerCoordinateTextActor;
        std::vector<vtkSmartPointer<vtkActor2D>> selectedAtomBackgroundCircleActors;
        std::vector<vtkSmartPointer<vtkActor2D>> selectedAtomPlusTextActors;
        int centerPlusBaseFontSize = 0;
        int centerCoordinateBaseFontSize = 0;
        std::vector<int> selectedAtomPlusBaseFontSizes;
    };
    struct MeasurementPickVisual {
        uint32_t atomId = 0;
        size_t order = 0;
        vtkSmartPointer<vtkActor> shellActor;
        vtkSmartPointer<vtkActor2D> orderTextActor;
    };
    struct DistanceStyle {
        std::array<float, 3> lineColor { 0.133333f, 0.133333f, 0.133333f };
        float lineWidth = 2.0f;
        float labelScale = 1.0f;
    };
    struct AngleStyle {
        std::array<float, 3> lineColor { 0.133333f, 0.133333f, 0.133333f };
        float lineWidth = 2.0f;
        float arcRadiusScale = 1.0f;
        float labelScale = 1.0f;
    };
    struct DihedralStyle {
        std::array<float, 3> baseLineColor { 0.133333f, 0.133333f, 0.133333f };
        float baseLineWidth = 2.0f;
        std::array<float, 3> helperPlane1Color { 0.0f, 0.0f, 1.0f };
        float helperPlane1Opacity = 0.2f;
        std::array<float, 3> helperPlane2Color { 125.0f / 255.0f, 0.0f, 1.0f };
        float helperPlane2Opacity = 0.2f;
        float labelScale = 1.0f;
    };
    struct CenterStyle {
        std::array<float, 3> centerPlusColor { 1.0f, 0.0f, 0.0f };
        float centerPlusScale = 1.0f;
        std::array<float, 3> selectedPlusColor { 0.0f, 0.0f, 0.0f };
        float selectedPlusScale = 1.0f;
        float coordinateLabelScale = 1.0f;
    };
    struct CreatedAtomSelectionVisual {
        uint32_t atomId = 0;
        vtkSmartPointer<vtkActor> shellActor;
    };
    MeasurementMode m_MeasurementMode { MeasurementMode::None };
    std::vector<uint32_t> m_MeasurementPickedAtomIds;
    std::vector<DistanceMeasurement> m_DistanceMeasurements;
    std::vector<AngleMeasurement> m_AngleMeasurements;
    std::vector<DihedralMeasurement> m_DihedralMeasurements;
    std::vector<CenterMeasurement> m_CenterMeasurements;
    std::vector<MeasurementPickVisual> m_MeasurementPickVisuals;
    std::vector<CreatedAtomSelectionVisual> m_CreatedAtomSelectionVisuals;
    uint32_t m_NextMeasurementId = 1;
    size_t m_NextCenterMarkerStyleIndex = 0;
    bool m_MeasurementVisibilitySuppressedByBZ = false;
    DistanceStyle m_DistanceStyle {};
    AngleStyle m_AngleStyle {};
    DihedralStyle m_DihedralStyle {};
    CenterStyle m_GeometricCenterStyle {};
    CenterStyle m_CenterOfMassStyle {};
    bool m_DistanceStylePanelExpanded = false;
    bool m_AngleStylePanelExpanded = false;
    bool m_DihedralStylePanelExpanded = false;
    bool m_GeometricCenterStylePanelExpanded = false;
    bool m_CenterOfMassStylePanelExpanded = false;
    
    // Node Info ?듭뀡
    bool m_NodeInfoEnabled = true;
    
    // Charge Density UI
    std::unique_ptr<atoms::ui::ChargeDensityUI> m_chargeDensityUI;
    enum class PendingBuilderSection {
        None = 0,
        BravaisLatticeTemplates = 1,
        AddAtoms = 2,
        BrillouinZone = 3
    };
    enum class PendingEditorSection {
        None = 0,
        Atoms = 1,
        Bonds = 2,
        Cell = 3
    };
    enum class PendingAdvancedSection {
        None = 0,
        ChargeDensity = 1,
        SliceViewer = 2
    };
    PendingBuilderSection m_PendingBuilderSection { PendingBuilderSection::None };
    PendingEditorSection m_PendingEditorSection { PendingEditorSection::None };
    PendingAdvancedSection m_PendingAdvancedSection { PendingAdvancedSection::None };
    struct WindowLayoutRequest {
        bool enabled = false;
        ImVec2 pos = ImVec2(0.0f, 0.0f);
        ImVec2 size = ImVec2(0.0f, 0.0f);
        int framesRemaining = 0;
    };
    WindowLayoutRequest m_BuilderWindowLayoutRequest {};
    WindowLayoutRequest m_EditorWindowLayoutRequest {};
    WindowLayoutRequest m_AdvancedWindowLayoutRequest {};
    enum class ChargeDensityViewType {
        Simple = 0,
        Advanced = 1
    };
    ChargeDensityViewType m_ChargeDensityViewType { ChargeDensityViewType::Simple };
    ChargeDensityViewType m_LastChargeDensityViewType { ChargeDensityViewType::Simple };
    bool m_ChargeDensityViewTypeInitialized = false;
    struct ChargeDensityAdvancedGridVisibilityState {
        bool surfaceVisible = true;
        bool volumeVisible = true;
    };
    std::unordered_map<int32_t, ChargeDensityAdvancedGridVisibilityState> m_ChargeDensityAdvancedGridVisibility;
    bool m_ChargeDensitySimpleIsosurfaceVisible = false;
    bool m_ChargeDensitySimpleMultipleIsosurfacesVisible = false;
    bool m_ChargeDensitySimpleSliceVisible = false;
    bool m_ChargeDensitySimpleStateCaptured = false;
    int32_t m_ChargeDensityVolumeMeshId = -1;
    int32_t m_ActiveChargeDensityGridMeshId = -1;
    bool m_ChargeDensityGridVisibilityCaptured = false;
    std::unordered_map<int32_t, bool> m_ChargeDensityGridVisibilityCache;

    void applyForcedWindowLayout(WindowLayoutRequest& request);
    void renderChargeDensityViewerContent();
    void renderSliceViewerContent();
    void refreshRenderedGroups();
    void refreshLabelActors();
    ChargeDensityAdvancedGridVisibilityState& ensureChargeDensityAdvancedGridVisibilityState(int32_t meshId);
    static bool getChargeDensityAdvancedGridVisibilityValue(
        const ChargeDensityAdvancedGridVisibilityState& state, bool volumeMode);
    static void setChargeDensityAdvancedGridVisibilityValue(
        ChargeDensityAdvancedGridVisibilityState& state, bool volumeMode, bool visible);
    void captureChargeDensityAdvancedGridVisibilityFromMesh(int32_t meshId, Mesh* mesh);
    void applyChargeDensityAdvancedGridVisibilityToMesh(int32_t meshId, Mesh* mesh);
    void setChargeDensityGridMeshVisibilityImmediate(
        int32_t meshId, bool surfaceVisible, bool volumeVisible);
    void setCurrentStructureGridRenderMode(bool volumeMode);
    void applyChargeDensityViewType();
    void setChargeDensityVolumeSuppressed(bool suppressed);
    void setGridChargeDensitySuppressed(bool suppressed);
    static std::string makeStructureKey(const std::string& baseKey, int32_t structureId);
    static bool getAtomStructureId(uint32_t atomId, int32_t& structureId);
    static bool getBondStructureId(uint32_t bondId, int32_t& structureId);
    const atoms::domain::AtomInfo* findAtomById(uint32_t atomId) const;
    bool isCenterMeasurementMode() const;
    bool resolvePickedAtom(
        vtkActor* actor,
        const double pickPos[3],
        uint32_t& atomId,
        int32_t& structureId,
        std::array<double, 3>& atomPosition) const;
    std::string buildMeasurementAtomLabel(const atoms::domain::AtomInfo* atom) const;
    size_t measurementTargetPickCount() const;
    void createMeasurementFromPickedAtoms();
    void createDistanceMeasurement(uint32_t atomId1, uint32_t atomId2);
    void createAngleMeasurement(uint32_t atomId1, uint32_t atomId2, uint32_t atomId3);
    void createDihedralMeasurement(
        uint32_t atomId1,
        uint32_t atomId2,
        uint32_t atomId3,
        uint32_t atomId4);
    bool commitCenterMeasurementFromPickedAtoms();
    bool createGeometricCenterMeasurement(const std::vector<uint32_t>& atomIds);
    bool createCenterOfMassMeasurement(const std::vector<uint32_t>& atomIds);
    bool createCenterMeasurement(MeasurementType type, const std::vector<uint32_t>& atomIds);
    void clampMeasurementStyles();
    void renderMeasurementStyleOptionsOverlay();
    void applyDistanceStyleToMeasurement(DistanceMeasurement& measurement);
    void applyAngleStyleToMeasurement(AngleMeasurement& measurement);
    void applyDihedralStyleToMeasurement(DihedralMeasurement& measurement);
    void applyCenterStyleToMeasurement(CenterMeasurement& measurement);
    void applyDistanceStyleToAllMeasurements();
    void applyAngleStyleToAllMeasurements(bool rebuildGeometry);
    void applyDihedralStyleToAllMeasurements();
    void applyCenterStyleToAllMeasurements();
    bool rebuildAngleMeasurementGeometry(AngleMeasurement& measurement);
    std::string buildCenterMeasurementDisplayName(
        MeasurementType type,
        const std::vector<uint32_t>& atomIds) const;
    void applyDistanceMeasurementVisibility(DistanceMeasurement& measurement);
    void applyAngleMeasurementVisibility(AngleMeasurement& measurement);
    void applyDihedralMeasurementVisibility(DihedralMeasurement& measurement);
    void applyCenterMeasurementVisibility(CenterMeasurement& measurement);
    void removeDistanceMeasurementActors(DistanceMeasurement& measurement);
    void removeAngleMeasurementActors(AngleMeasurement& measurement);
    void removeDihedralMeasurementActors(DihedralMeasurement& measurement);
    void removeCenterMeasurementActors(CenterMeasurement& measurement);
    void clearAllDistanceMeasurements();
    void clearAllAngleMeasurements();
    void clearAllDihedralMeasurements();
    void clearAllCenterMeasurements();
    void removeDistanceMeasurementsByStructure(int32_t structureId);
    void removeAngleMeasurementsByStructure(int32_t structureId);
    void removeDihedralMeasurementsByStructure(int32_t structureId);
    void removeCenterMeasurementsByStructure(int32_t structureId);
    void pruneInvalidDistanceMeasurements();
    void pruneInvalidAngleMeasurements();
    void pruneInvalidDihedralMeasurements();
    void pruneInvalidCenterMeasurements();
    void setMeasurementVisibilitySuppressedByBZ(bool suppressed);
    void clearMeasurementPickVisuals();
    void syncMeasurementPickVisuals();
    void clearCreatedAtomSelectionVisuals();
    void syncCreatedAtomSelectionVisuals();
    std::vector<uint32_t> collectAtomIdsInScreenRect(
        int x0,
        int y0,
        int x1,
        int y1,
        class vtkRenderer* renderer,
        int viewportHeight,
        bool includeSurrounding) const;
    void applyDragSelectionToMeasurement(const std::vector<uint32_t>& atomIds, bool additive);
    void applyDragSelectionToCreatedAtoms(const std::vector<uint32_t>& atomIds, bool additive);

    /**
     * @brief 諛곗튂 紐⑤뱶瑜??쒖옉?⑸땲??
     * @details ?대??곸쑝濡?BatchUpdateSystem::beginBatch()瑜??몄텧?⑸땲??
     * @note Affects: BatchUpdateSystem ?곹깭.
     * @note Called by: (no direct callers found in webassembly/src; internal/friend use).
     */
    void beginBatch() { 
        if (m_batchSystem) m_batchSystem->beginBatch(); 
    }

    /**
     * @brief 諛곗튂 紐⑤뱶瑜?醫낅즺?섍퀬 ?湲?以묒씤 媛깆떊??泥섎━?⑸땲??
     * @details ?대??곸쑝濡?BatchUpdateSystem::endBatch()瑜??몄텧?⑸땲??
     * @note Affects: BatchUpdateSystem ?곹깭 諛??뚮뜑 媛깆떊 ?ㅼ?以?
     * @note Called by: (no direct callers found in webassembly/src; internal/friend use).
     */
    void endBatch() { 
        if (m_batchSystem) m_batchSystem->endBatch(); 
    }

    /**
     * @brief ?뱀젙 ?먯옄 洹몃９ 媛깆떊??諛곗튂 ?먯뿉 ?깅줉?⑸땲??
     * @param symbol ?먯옄 洹몃９ ??
     * @note Affects: BatchUpdateSystem pending atom group 紐⑸줉.
     * @note Called by: AtomsTemplate::createAtomSphere.
     */
    void scheduleAtomGroupUpdate(const std::string& symbol) { 
        if (m_batchSystem) m_batchSystem->scheduleAtomGroupUpdate(symbol); 
    }

    /**
     * @brief ?뱀젙 寃고빀 洹몃９ 媛깆떊??諛곗튂 ?먯뿉 ?깅줉?⑸땲??
     * @param bondKey 寃고빀 洹몃９ ??
     * @note Affects: BatchUpdateSystem pending bond group 紐⑸줉.
     * @note Called by: (no direct callers found in webassembly/src; internal/friend use).
     */
    void scheduleBondGroupUpdate(const std::string& bondKey) { 
        if (m_batchSystem) m_batchSystem->scheduleBondGroupUpdate(bondKey); 
    }

    /**
     * @brief 諛곗튂 紐⑤뱶瑜?媛뺤젣 醫낅즺?⑸땲??
     * @details ?대??곸쑝濡?BatchUpdateSystem::forceBatchEnd()瑜??몄텧?⑸땲??
     * @note Affects: BatchUpdateSystem ?곹깭 諛??뚮뜑 媛깆떊 ?ㅼ?以?
     * @note Called by: (no direct callers found in webassembly/src; internal/friend use).
     */
    void forceBatchEnd() { 
        if (m_batchSystem) m_batchSystem->forceBatchEnd(); 
    }

    // ========================================================================
    // Internal orchestration helpers
    // ========================================================================

    /**
     * @brief ?꾩옱 援ъ“(?먯옄/寃고빀/?)瑜?珥덇린 ?곹깭濡??뺣━?⑸땲??
     * @note Affects: createdAtoms, surroundingAtoms, atoms::domain::atomGroups,
     *                  atoms::domain::bondGroups, cellInfo, m_vtkRenderer.
     * @note Called by: AtomsTemplate::LoadXSFFile, AtomsTemplate::setBravaisLattice.
     */
    //void resetStructure(); // public?쇰줈 蹂寃?: 援ъ“珥덇린????젣 ???ъ슜)

    /**
     * @brief ?먯옄 蹂寃쎌궗??쓣 ?곸슜?섍퀬 寃고빀???ъ깮?깊빀?덈떎.
     * @details BatchGuard瑜??ъ슜???꾨찓??applyAtomChanges()瑜??몄텧?⑸땲??
     * @note Affects: createdAtoms, createdBonds, atoms::domain::atomGroups, bondGroups.
     * @note Called by: webassembly/src/atoms/ui/atom_editor_ui.cpp
     *                  (atoms::ui::AtomEditorUI::render).
     */
    void applyAtomChanges();

    /**
     * @brief Unit Cell 蹂寃쎌궗??쓣 ?곸슜?⑸땲??
     * @details BatchGuard瑜??ъ슜???꾨찓??applyCellChanges()瑜??몄텧?⑸땲??
     * @note Affects: cellInfo, unit cell ?뚮뜑留? 二쇰? ?먯옄/寃고빀 ?곹깭.
     * @note Called by: webassembly/src/atoms/ui/cell_info_ui.cpp
     *                  (atoms::ui::CellInfoUI::applyCellChangesOnEditEnd).
     */
    void applyCellChanges();

    /**
     * @brief BZ Plot 紐⑤뱶濡?吏꾩엯?⑸땲??
     * @param path 寃쎈줈 臾몄옄??
     * @param npoints 寃쎈줈 遺꾪븷 ?ъ씤????
     * @param showVectors 踰≫꽣 ?쒖떆 ?щ?.
     * @param showLabels ?쇰꺼 ?쒖떆 ?щ?.
     * @param outErrorMessage ?ㅽ뙣 ???ㅻ쪟 硫붿떆吏 異쒕젰.
     * @return ?깃났 ??true.
     * @details BZPlotController???쒖뼱瑜??꾩엫?⑸땲??
     * @note Affects: BZ Plot ?뚮뜑 ?곹깭, VTKRenderer.
     * @note Called by: webassembly/src/atoms/ui/bz_plot_ui.cpp
     *                  (atoms::ui::BZPlotUI::renderOptions,
     *                   atoms::ui::BZPlotUI::renderToggleAndClearButtons).
     */
    bool enterBZPlotMode(const std::string& path,
                         int npoints,
                         bool showVectors,
                         bool showLabels,
                         std::string& outErrorMessage);

    /**
     * @brief BZ Plot 紐⑤뱶瑜?醫낅즺?⑸땲??
     * @details BZPlotController??醫낅즺瑜??꾩엫?⑸땲??
     * @note Affects: BZ Plot ?뚮뜑 ?곹깭, VTKRenderer.
     * @note Called by: webassembly/src/atoms/ui/bz_plot_ui.cpp
     *                  (atoms::ui::BZPlotUI::renderToggleAndClearButtons).
     */
    void exitBZPlotMode();

    /**
     * @brief ?깅뒫 ?듦퀎瑜??대??곸쑝濡?媛깆떊?⑸땲??
     * @param duration 諛곗튂 ?ㅽ뻾 ?쒓컙(ms).
     * @param atomGroupCount 泥섎━???먯옄 洹몃９ ??
     * @param bondGroupCount 泥섎━??寃고빀 洹몃９ ??
     * @details g_performanceStats瑜?媛깆떊?섍퀬 寃쎄퀬 濡쒓렇瑜?湲곕줉?⑸땲??
     * @note Affects: g_performanceStats (atoms_template.cpp).
     * @note Called by: webassembly/src/atoms/infrastructure/batch_update_system.cpp
     *                  (atoms::infrastructure::BatchUpdateSystem::updatePerformanceStats).
     */
    void updatePerformanceStatsInternal(float duration, 
                                       size_t atomGroupCount, 
                                       size_t bondGroupCount);

    /**
     * @brief ?깅뒫 ?듦퀎 ?낅뜲?댄듃???명솚 ?섑띁?낅땲??
     * @param duration 諛곗튂 ?ㅽ뻾 ?쒓컙(ms).
     * @details BatchUpdateSystem???湲?洹몃９ ?섎? 議고쉶?????대? ?듦퀎 媛깆떊???몄텧?⑸땲??
     * @note Affects: g_performanceStats (atoms_template.cpp).
     * @note Called by: (no direct callers found in webassembly/src; legacy wrapper).
     */
    void updatePerformanceStats(float duration);

    // ========================================================================
    // Infrastructure Layer
    // ========================================================================
    std::unique_ptr<atoms::infrastructure::BatchUpdateSystem> m_batchSystem; ///< 諛곗튂 ?낅뜲?댄듃 ?쒖뒪???뚯쑀.
    std::unique_ptr<atoms::infrastructure::BondRenderer> m_bondRenderer; ///< 寃고빀 ?뚮뜑???듭뀡) ?뚯쑀.
    std::unique_ptr<atoms::infrastructure::VTKRenderer> m_vtkRenderer; ///< ?먯옄/? ?뚮뜑???뚯쑀.
    std::unique_ptr<atoms::infrastructure::FileIOManager> m_fileIOManager;  ///< ?뚯씪 I/O 留ㅻ땲? ?뚯쑀.

    // ========================================================================
    // ?꾨찓???덉씠??    // ========================================================================
    atoms::domain::ElementDatabase* m_elementDB; ///< ElementDatabase ?깃????ъ씤??鍮꾩냼??.
    std::unique_ptr<atoms::domain::BZPlotController> m_bzPlotController; ///< BZ Plot 而⑦듃濡ㅻ윭 ?뚯쑀.
    std::unique_ptr<atoms::domain::SurroundingAtomManager> m_surroundingAtomManager; ///< 二쇰? ?먯옄 留ㅻ땲? ?뚯쑀.

    // 狩?異붽?: UI Layer
    std::unique_ptr<atoms::ui::AtomEditorUI> m_atomEditorUI; ///< ?먯옄 ?몄쭛 UI ?뚯쑀.
    std::unique_ptr<atoms::ui::BondUI> m_bondUI; ///< 寃고빀 UI ?뚯쑀.
    std::unique_ptr<atoms::ui::CellInfoUI> m_cellInfoUI; ///< ? ?뺣낫 UI ?뚯쑀.
    std::unique_ptr<atoms::ui::BZPlotUI> m_bzPlotUI; ///< BZ Plot UI ?뚯쑀.
    std::unique_ptr<atoms::ui::PeriodicTableUI> m_periodicTableUI; ///< 二쇨린?⑦몴 UI ?뚯쑀.
    std::unique_ptr<atoms::ui::BravaisLatticeUI> m_bravaisLatticeUI; ///< Bravais lattice UI ?뚯쑀.

    // ========================================================================
    // FRIENDS
    // ========================================================================
    friend class atoms::infrastructure::BatchUpdateSystem;
    friend class atoms::infrastructure::VTKRenderer;
    friend class atoms::infrastructure::FileIOManager;  // [異붽?] FileIOManager friend ?좎뼵
    friend class atoms::ui::PeriodicTableUI; 
    friend class atoms::ui::BravaisLatticeUI;
    friend class atoms::ui::AtomEditorUI;
    friend class atoms::ui::BondUI;
    friend class atoms::ui::CellInfoUI;
    friend class atoms::ui::BZPlotUI;
};
