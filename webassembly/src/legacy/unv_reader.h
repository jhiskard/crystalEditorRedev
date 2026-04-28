#pragma once

#include "macro/ptr_macro.h"
#include "mesh_group.h"

// Standard library
#include <string>
#include <vector>
#include <unordered_map>

// VTK
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>


DECLARE_PTR(UnvReader)
class UnvReader {
public:
    static UnvReaderUPtr New(const char* filename);

    ~UnvReader() = default;

    vtkSmartPointer<vtkUnstructuredGrid> GetEdgeMesh() const { return m_EdgeMesh; }
    vtkSmartPointer<vtkUnstructuredGrid> GetFaceMesh() const { return m_FaceMesh; }
    vtkSmartPointer<vtkUnstructuredGrid> GetVolumeMesh() const { return m_VolumeMesh; }

    size_t GetMeshGroupCount() const { return m_MeshGroups.size(); }
    std::vector<MeshGroupUPtr> MoveMeshGroups() { return std::move(m_MeshGroups); }  // m_MeshGroups will be empty after this call.

    bool ReadUnvFile();

private:
    std::string m_Filename;
    float m_FileSize { 0.0f };

    vtkSmartPointer<vtkPoints> m_Points { nullptr };
    vtkSmartPointer<vtkUnstructuredGrid> m_EdgeMesh { nullptr };
    vtkSmartPointer<vtkUnstructuredGrid> m_FaceMesh { nullptr };
    vtkSmartPointer<vtkUnstructuredGrid> m_VolumeMesh { nullptr };
    std::unordered_map<int32_t, int32_t> m_UnvToVtkNodeMap;    // Map of UNV node Id to VTK node Id
    std::unordered_map<int32_t, int32_t> m_UnvToVtkEdgeMap;    // Map of UNV element Id to VTK edge element Id
    std::unordered_map<int32_t, int32_t> m_UnvToVtkFaceMap;    // Map of UNV element Id to VTK face element Id
    std::unordered_map<int32_t, int32_t> m_UnvToVtkVolumeMap;  // Map of UNV element Id to VTK volume element Id

    std::vector<MeshGroupUPtr> m_MeshGroups;

    // UNV formats (https://victorsndvg.github.io/FEconv/formats/unv.xhtml)
    // VTK formats (https://examples.vtk.org/site/VTKBook/05Chapter5/)
    // Element type: 11 - Line with 2 nodes
    const std::vector<int32_t> LINE_NODEORDER_UNV2VTK {{ 0, 1 }}; 
    // Element type: 22 - Line with 3 nodes
    // Element type: 41 - Triangle with 3 nodes
    const std::vector<int32_t> TRIANGLE_NODEORDER_UNV2VTK {{ 0, 1, 2 }};
    // Element type: 42 - Triangle with 6 nodes
    // Element type: 44 - Quadrangle with 4 nodes
    const std::vector<int32_t> QUAD_NODEORDER_UNV2VTK {{ 0, 1, 2, 3 }};
    // Element type: 45 - Quadrangle with 8 nodes
    // Element type: 111 - Tetrahedron with 4 nodes
    const std::vector<int32_t> TETRA4_NODEORDER_UNV2VTK {{ 0, 1, 2, 3 }};
    // Element type: 115 - Hexahedron with 8 nodes
    const std::vector<int32_t> HEXA8_NODEORDER_UNV2VTK {{ 0, 1, 2, 3, 4, 5, 6, 7 }};
    // Element type: 116 - Hexahedron with 20 nodes
    // Element type: 118 - Tetrahedron with 10 nodes

    UnvReader() = default;

    UnvReader(const UnvReader&) = delete;
    UnvReader& operator=(const UnvReader&) = delete;
    UnvReader(UnvReader&&) = delete;
    UnvReader& operator=(UnvReader&&) = delete;

    void processDataSet(int32_t dataSetId, const std::vector<std::string>& dataSet);
    void processNodeData(const std::vector<std::string>& dataSet);
    void processElementData(const std::vector<std::string>& dataSet);
    void processGroupData(const std::vector<std::string>& dataSet);
};