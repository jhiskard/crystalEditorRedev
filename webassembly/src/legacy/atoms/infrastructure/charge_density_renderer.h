// atoms/infrastructure/charge_density_renderer.h
#pragma once

#include "chgcar_parser.h"  // ✅ include 추가
#include "../domain/charge_density.h"
#include "vtk_renderer.h"
#include "../../common/colormap.h"

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkContourFilter.h>
#include <vtkActor.h>
#include <vtkVolume.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <cstdint>
#include <vector>

namespace atoms {
namespace infrastructure {

class ChargeDensityRenderer {
public:
    explicit ChargeDensityRenderer(VTKRenderer* vtkRenderer);
    ~ChargeDensityRenderer();
    
    // ========================================================================
    // 데이터 로드
    // ========================================================================
    
    bool loadFromFile(const std::string& filePath);
    bool setData(const domain::ChargeDensity& data);
    void clear();
    // ✅ ParseResult에서 직접 로드
    bool loadFromParseResult(const ChgcarParser::ParseResult& result);    
    
    // ========================================================================
    // Isosurface 렌더링
    // ========================================================================
    
    void setIsosurfaceValue(float value);
    void setIsosurfaceColor(float r, float g, float b, float alpha = 0.7f);
    void setIsosurfaceVisible(bool visible);
    bool isIsosurfaceVisible() const { return m_isosurfaceVisible; }
    
    // 다중 등치면
    void addIsosurface(float value, float r, float g, float b, float alpha = 0.7f);
    void setIsosurfaceColorForValue(float value, float r, float g, float b, float alpha = 0.7f);
    void clearIsosurfaces();
    
    // ========================================================================
    // 2D 슬라이스
    // ========================================================================
    
    enum class SlicePlane { XY = 0, XZ = 1, YZ = 2, Miller = 3 };
    
    void setSlicePlane(SlicePlane plane);
    void setSlicePosition(float position);  // 0.0 ~ 1.0
    void setSliceMillerIndices(int h, int k, int l);
    void setSliceVisible(bool visible);
    bool isSliceVisible() const { return m_sliceVisible; }
    bool getSliceValues(std::vector<float>& values,
                        int& width,
                        int& height,
                        int millerMaxResolution = 1024) const;
    bool captureRenderedSliceImage(std::vector<uint8_t>& pixels,
                                   int& width,
                                   int& height);
    
    // 컬러맵
    using ColorMap = common::ColorMapPreset;
    void setColorMap(ColorMap colorMap);
    void setValueRange(float min, float max);
    void setColorCurve(float midpoint, float sharpness);
    
    // ========================================================================
    // 정보 조회
    // ========================================================================
    
    bool hasData() const { return m_imageData != nullptr; }
    float getMinValue() const { return m_minValue; }
    float getMaxValue() const { return m_maxValue; }
    const std::array<int, 3>& getGridShape() const { return m_gridShape; }
    
private:
    VTKRenderer* m_vtkRenderer;
    
    // 데이터
    vtkSmartPointer<vtkImageData> m_imageData;
    std::array<int, 3> m_gridShape = {0, 0, 0};
    float m_minValue = 0.0f;
    float m_maxValue = 0.0f;
    float m_lattice[3][3] = {};
    
    // Isosurface
    vtkSmartPointer<vtkContourFilter> m_contourFilter;
    vtkSmartPointer<vtkActor> m_isosurfaceActor;
    float m_isosurfaceValue = 0.0f;
    bool m_isosurfaceVisible = false;
    
    // 다중 등치면
    struct IsosurfaceInfo {
        float value;
        vtkSmartPointer<vtkActor> actor;
    };
    std::vector<IsosurfaceInfo> m_isosurfaces;
    
    // 슬라이스
    vtkSmartPointer<vtkActor> m_sliceActor;
    SlicePlane m_slicePlane = SlicePlane::XY;
    int m_sliceMillerH = 1;
    int m_sliceMillerK = 1;
    int m_sliceMillerL = 1;
    float m_slicePosition = 0.5f;
    bool m_sliceVisible = false;
    ColorMap m_colorMap = ColorMap::Rainbow;
    float m_colorCurveMidpoint = 0.5f;
    float m_colorCurveSharpness = 0.0f;
    
    // 컬러바
    vtkSmartPointer<vtkActor2D> m_colorBarActor;
    float m_isoColor[4] = {0.0f, 0.5f, 1.0f, 0.7f};  // RGBA

    // 내부 함수
    void createVTKImageData(const std::vector<float>& data);
    void updateIsosurface();
    void updateSlice();
    void setupColorMap(vtkSmartPointer<vtkColorTransferFunction> ctf);
    bool resolveMillerPlaneInDataCoords(double normal[3], double& offset) const;
};

} // namespace infrastructure
} // namespace atomsnderer.h
