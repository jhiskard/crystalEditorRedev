// atoms/infrastructure/slice_renderer.h
class SliceRenderer {
public:
    enum class SlicePlane { XY, XZ, YZ, CUSTOM };
    
    // 단면 설정
    void setSlicePlane(SlicePlane plane, float position);
    void setCustomPlane(float normal[3], float origin[3]);
    
    // 컬러맵
    void setColorMap(ColorMapType type); // Rainbow, Viridis, etc.
    void setValueRange(float min, float max);
    
    // 렌더링
    void render();
    
private:
    vtkSmartPointer<vtkCutter> m_cutter;
    vtkSmartPointer<vtkPlane> m_plane;
    vtkSmartPointer<vtkActor> m_sliceActor;
    vtkSmartPointer<vtkScalarBarActor> m_colorBar;
};
