// atoms/ui/charge_density_ui.h
#pragma once
#include <memory>
#include <string>
#include <array>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../infrastructure/chgcar_parser.h"
#include "../../common/colormap.h"

class AtomsTemplate;

namespace atoms {
namespace infrastructure {
class ChargeDensityRenderer;
}

namespace ui {

/**
 * @brief 전하 밀도 시각화 UI 패널
 */
class ChargeDensityUI {
public:
    struct GridDataEntry {
        std::string name;
        std::array<int, 3> dims = {0, 0, 0};
        float origin[3] = {0.0f, 0.0f, 0.0f};
        float vectors[3][3] = {};
        std::vector<float> values;
    };

    struct SimpleGridEntry {
        std::string name;
        bool visible = false;
    };

    struct SliceGridEntry {
        std::string name;
        bool visible = false;
    };

    explicit ChargeDensityUI(::AtomsTemplate* atomsTemplate);
    ~ChargeDensityUI();
    
    /// ImGui 렌더링
    void render();
    void renderDataInformation();

    void renderSliceViewer();
    
    /// 파일 로드 (외부에서 호출)
    bool loadFile(const std::string& filePath);
    
    /// 데이터 클리어
    void clear();
    
    /// 데이터 유무 확인
    bool hasData() const;
    
    /// ParseResult에서 직접 로드
    bool loadFromParseResult(const infrastructure::ChgcarParser::ParseResult& result);    

    void setGridDataEntries(std::vector<GridDataEntry>&& entries);
    bool hasGridDataEntries() const;
    std::vector<SimpleGridEntry> getSimpleGridEntries() const;
    std::vector<SliceGridEntry> getSliceGridEntries() const;
    bool setSimpleGridVisible(const std::string& gridName, bool visible);
    bool setSliceGridVisible(const std::string& gridName, bool visible);
    bool selectSimpleGridByName(const std::string& gridName);
    bool selectSliceGridByName(const std::string& gridName);
    void setAllSimpleGridVisible(bool visible);
    void setAllSliceGridVisible(bool visible);
    bool isAnySimpleGridVisible() const;
    bool isAnySliceGridVisible() const;
    bool isLoadedFromGrid() const { return m_loadedFromGrid; }
    const std::string& getLoadedDataName() const { return m_loadedFileName; }
    bool handleGridMeshVisibilityChange(const std::string& gridName, bool visible);
    bool ensureSliceDataForGrid(const std::string& gridName);
    
    bool isIsosurfaceVisible() const { return m_showIsosurface; }
    bool isAnyIsosurfaceVisible() const;
    bool isMultipleIsosurfacesVisible() const { return m_showMultipleIsosurfaces; }
    bool isSliceVisible() const { return m_showSlice; }
    void setIsosurfaceVisible(bool visible);
    void setMultipleIsosurfacesVisible(bool visible);
    void setIsosurfaceGroupVisible(bool visible);
    void setSliceVisible(bool visible);
    void setStructureVisible(bool visible);
    void syncSliceDisplayFromVolume(const std::string& gridName,
                                    double window,
                                    double level,
                                    double colorMidpoint,
                                    double colorSharpness,
                                    double dataMin,
                                    double dataMax);

    // ========================================================================
    // 애니메이션 관련 (Toolbar에서 사용)
    // ========================================================================
    
    /// 애니메이션 재생 중인지
    bool isPlaying() const { return m_isPlaying; }
    
    /// 애니메이션 시작
    void startAnimation(float durationSeconds = 10.0f);
    
    /// 애니메이션 정지
    void stopAnimation();
    
    /// 애니메이션 업데이트 (매 프레임 호출)
    void updateAnimation();
    
    /// 현재 레벨 (0.0 ~ 1.0)
    float getLevelPercent() const;
    
    /// 레벨 설정 (0.0 ~ 1.0) - 수동 조작 시 애니메이션 정지
    void setLevelPercent(float percent, bool stopAnim = true);
    
    /// 애니메이션 duration 설정/조회
    float getAnimationDuration() const { return m_animationDuration; }
    void setAnimationDuration(float seconds) { m_animationDuration = seconds; }

private:
    ::AtomsTemplate* m_atomsTemplate;
    std::unique_ptr<infrastructure::ChargeDensityRenderer> m_renderer;
    std::unordered_map<std::string, std::unique_ptr<infrastructure::ChargeDensityRenderer>> m_auxRenderers;
    
    // ========================================================================
    // UI 상태
    // ========================================================================
    
    // 파일 정보
    std::string m_loadedFileName;
    bool m_fileLoaded = false;
    
    // 그리드 정보
    std::array<int, 3> m_gridShape = {0, 0, 0};
    float m_minValue = 0.0f;
    float m_maxValue = 0.0f;
    std::vector<GridDataEntry> m_gridDataEntries;
    int m_selectedGridDataIndex = 0;
    bool m_loadedFromGrid = false;
    std::unordered_map<std::string, bool> m_simpleGridVisibility;
    std::unordered_map<std::string, bool> m_sliceGridVisibility;
    std::unordered_map<std::string, float> m_simpleGridIsoValues;
    std::unordered_map<std::string, std::array<float, 4>> m_simpleGridIsoColors;
    
    // Isosurface 설정
    bool m_showIsosurface = false;
    bool m_structureVisible = true;
    float m_isoValue = 0.0f;
    float m_isoColor[4] = {0.0f, 0.5f, 1.0f, 0.7f};  // RGBA
    bool m_autoIsoValue = true;
    bool m_isoValueDirectInput = false;
    bool m_isoValueDirectInputFocus = false;
    
    // 다중 등치면
    bool m_showMultipleIsosurfaces = false;
    float m_isoValuePositive = 0.0f;
    float m_isoValueNegative = 0.0f;
    float m_isoColorPositive[4] = {1.0f, 0.0f, 0.0f, 0.6f};
    float m_isoColorNegative[4] = {0.0f, 0.0f, 1.0f, 0.6f};
    
    // 2D 슬라이스 설정
    bool m_showSlice = false;
    int m_slicePlane = 0;
    int m_sliceMillerH = 1;
    int m_sliceMillerK = 1;
    int m_sliceMillerL = 1;
    float m_slicePosition = 0.5f;
    bool m_autoRange = true;
    float m_displayMin = 0.0f;
    float m_displayMax = 1.0f;
    float m_sliceColorMidpoint = 0.5f;
    float m_sliceColorSharpness = 0.0f;
    bool m_hasSyncedVolumeDisplaySettings = false;
    double m_lastSyncedVolumeWindow = 0.0;
    double m_lastSyncedVolumeLevel = 0.0;
    double m_lastSyncedColorMidpoint = 0.5;
    double m_lastSyncedColorSharpness = 0.0;
    double m_lastSyncedDataMin = 0.0;
    double m_lastSyncedDataMax = 1.0;
    
    // ========================================================================
    // 애니메이션 상태
    // ========================================================================
    bool m_isPlaying = false;
    float m_animationDuration = 10.0f;  // 기본 10초
    std::chrono::steady_clock::time_point m_animationStartTime;
    float m_animationStartLevel = 0.0f;  // 시작 시점의 레벨
    
    // ========================================================================
    // UI 렌더링 함수
    // ========================================================================
    
    struct GridMeshEntry {
        int32_t id = -1;
        std::string name;
        bool visible = false;
    };

    std::vector<GridMeshEntry> collectGridMeshes() const;
    void renderFileSection();
    void renderInfoSection();
    void renderIsosurfaceSection(const std::vector<GridMeshEntry>& gridMeshes);
    void renderSliceControls();
    void renderSlicePreview();

    struct SliceRenderTarget {
        std::string key;
        std::string label;
        infrastructure::ChargeDensityRenderer* renderer = nullptr;
    };

    struct SlicePreviewCache {
        std::vector<uint8_t> pixels;
        int width = 0;
        int height = 0;
        int requestedWidth = 0;
        int requestedHeight = 0;
        int textureWidth = 0;
        int textureHeight = 0;
        uint32_t texture = 0;
        bool flipVerticallyWhenRendering = true;
        bool dirty = true;
        bool ready = false;
    };

    struct SliceDisplaySettings {
        bool autoRange = true;
        float displayMin = 0.0f;
        float displayMax = 1.0f;
        float colorMidpoint = 0.5f;
        float colorSharpness = 0.0f;
        bool hasSyncedVolumeDisplaySettings = false;
        double lastSyncedVolumeWindow = 0.0;
        double lastSyncedVolumeLevel = 0.0;
        common::ColorMapPreset lastSyncedColorPreset = common::ColorMapPreset::Rainbow;
        double lastSyncedColorMidpoint = 0.5;
        double lastSyncedColorSharpness = 0.0;
        double lastSyncedDataMin = 0.0;
        double lastSyncedDataMax = 1.0;
    };
    
    // ========================================================================
    // 헬퍼 함수
    // ========================================================================
    
    bool loadFromParseResultInternal(const infrastructure::ChgcarParser::ParseResult& result,
                                     const std::string& name,
                                     bool loadedFromGrid,
                                     bool resetIsosurface);
    bool loadFromGridEntry(int index);
    bool loadFromGridEntryInternal(int index, bool resetIsosurface, bool syncVisibility);
    int findGridEntryIndexByName(const std::string& name) const;
    bool ensureAuxRendererForGrid(const std::string& gridName);
    bool buildParseResultFromGridEntry(int index, infrastructure::ChgcarParser::ParseResult& result) const;
    bool isSimpleGridVisibleByKey(const std::string& key) const;
    bool isSliceGridVisibleByKey(const std::string& key) const;
    float getIsoValueForGridKey(const std::string& key) const;
    void syncIsoColorFromLoadedGrid();
    void storeIsoColorForLoadedGrid();
    std::array<float, 4> getIsoColorForGridKey(const std::string& key) const;
    void updateAllRendererVisibility();
    void syncGridMeshVisibility(const std::string& selectedName);
    void updateSlicePreview();
    void uploadSlicePreviewTexture(SlicePreviewCache& cache);
    void clearSlicePreview();
    std::vector<SliceRenderTarget> collectSliceRenderTargets();
    std::string getActiveSliceSettingsKey() const;
    common::ColorMapPreset resolveSliceColorMapPresetForKey(const std::string& key) const;
    SliceDisplaySettings& ensureSliceDisplaySettingsForKey(
        const std::string& key,
        const infrastructure::ChargeDensityRenderer* renderer = nullptr);
    const SliceDisplaySettings* findSliceDisplaySettingsForKey(const std::string& key) const;
    void storeSliceDisplaySettingsForLoadedGrid();
    void restoreSliceDisplaySettingsForLoadedGrid();
    void syncSliceSettingsForRenderer(infrastructure::ChargeDensityRenderer* renderer,
                                      const std::string& key);
    void markSlicePreviewCachesDirty();
    void updateSlicePreviewForTarget(const SliceRenderTarget& target);
    bool buildSliceImageForRenderer(const infrastructure::ChargeDensityRenderer* renderer,
                                    const std::string& settingsKey,
                                    int targetWidth,
                                    int targetHeight,
                                    int millerMaxResolution,
                                    std::vector<uint8_t>& pixels,
                                    int& width,
                                    int& height) const;
    bool buildSliceImage(int targetWidth,
                         int targetHeight,
                         int millerMaxResolution,
                         std::vector<uint8_t>& pixels,
                         int& width,
                         int& height) const;
    void updateIsosurface();
    void rebuildMultipleIsosurfaces();
    void updateSlice();
    void calculateDefaultIsoValue();
    void applyIsoValueFromPercent(float percent);

    std::unordered_map<std::string, SlicePreviewCache> m_slicePreviewCaches;
    std::unordered_map<std::string, SlicePreviewCache> m_slicePopupCaches;
    std::unordered_map<std::string, SliceDisplaySettings> m_sliceDisplaySettings;
    bool m_slicePreviewDirty = false;
    std::string m_slicePopupKey;
    std::vector<uint8_t> m_sliceDownloadPixels;
    int m_sliceDownloadWidth = 0;
    int m_sliceDownloadHeight = 0;
    std::string m_sliceDownloadName;
};

} // namespace ui
} // namespace atoms
