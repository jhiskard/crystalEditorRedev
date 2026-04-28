#include "unv_reader.h"
#include "config/log_config.h"
#include "app.h"
#include "common/string_utils.h"
#include "mesh_group.h"
#include "vtk_viewer.h"  // Test
#include <vtkVertexGlyphFilter.h>  // Test
#include <vtkPolyDataMapper.h>  // Test
#include <vtkActor.h>  // Test
#include <vtkProperty.h>  // Test

// Standard library
#include <fstream>
#include <array>

// Emscripten
#include <emscripten/emscripten.h>

// VTK
#include <vtkLine.h>
#include <vtkQuad.h>
#include <vtkHexahedron.h>


UnvReaderUPtr UnvReader::New(const char* filename) {
    UnvReaderUPtr reader = UnvReaderUPtr(new UnvReader());
    if (!filename) {
        return nullptr;
    }

    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        SPDLOG_ERROR("Failed to open file: {}", filename);
        return nullptr;
    }
    // Calculate the total size of the file
    ifs.seekg(0, std::ios::end);  // Move to the end of the file
    std::streampos totalSize = ifs.tellg();
    ifs.close();

    reader->m_FileSize = static_cast<float>(totalSize);
    reader->m_Filename = filename;

    return reader;
}

bool UnvReader::ReadUnvFile() {
    std::ifstream ifs(m_Filename);
    if (!ifs.is_open()) {
        SPDLOG_ERROR("Failed to open file: {}", m_Filename);
        return false;
    }

    // Start reading the file
    SPDLOG_INFO("Start reading UNV file: {}", m_Filename);

    // Read file line by line
    std::string line;
    int32_t currentDataSetId = -1;
    std::vector<std::string> currentDataSet;
    float prevProgress = 0.0f;

    while (std::getline(ifs, line)) {
        // Get the current position in the file to calculate progress
        std::streampos currentPos = ifs.tellg();
        if (currentPos != -1) {
            float progress = (float)currentPos / m_FileSize;
            if (progress - prevProgress > 0.01f) {  // Update progress every 1%
                // SPDLOG_INFO("Progress: {:.2f}", 100.0f * progress);
                App::Instance().SetProgress(static_cast<float>(progress));  // Update progress in the app
                prevProgress = progress;
            }
        }

        std::string trimmedLine = StringUtils::Trim(line);
        if (trimmedLine == "-1") {
            if (currentDataSetId == -1) {
                std::getline(ifs, line);
                trimmedLine = StringUtils::Trim(line);
                bool isNumber = StringUtils::IsNumber(trimmedLine);
                if (isNumber) {
                    currentDataSetId = std::stoi(trimmedLine);
                    currentDataSet.clear();
                }
            }
            else if (currentDataSetId != -1) {
                processDataSet(currentDataSetId, currentDataSet);
                currentDataSetId = -1;
                currentDataSet.clear();
            }
        }
        else {
            currentDataSet.push_back(trimmedLine);
        }
    }

    // Close the file
    ifs.close();

    SPDLOG_INFO("Successfully read UNV file: {}", m_Filename);
    return true;
}

void UnvReader::processDataSet(int32_t dataSetId, const std::vector<std::string>& dataSet) {
    switch (dataSetId) {
    case 164:   // Units
        break;
    case 2420:  // Coordinate system
        break;  
    case 2411:  // Node data
        processNodeData(dataSet);
        break;
    case 2412:  // Element data (Cell data)
        processElementData(dataSet);
        break;
    case 2467:  // Group data
        processGroupData(dataSet);
        break;
    default:
        SPDLOG_WARN("Unknown data set Id for UNV format: {}", dataSetId);
        break;
    }
}

void UnvReader::processNodeData(const std::vector<std::string>& dataSet) {
    m_Points = vtkSmartPointer<vtkPoints>::New();

    int32_t iLine = 0;
    int32_t nodeCount = 0;

    while (iLine < dataSet.size()) {
        std::istringstream iss1(dataSet.at(iLine));
        int32_t nodeId;
        iss1 >> nodeId;

        std::istringstream iss2(dataSet.at(iLine + 1));
        double x, y, z;
        iss2 >> x >> y >> z;

        vtkIdType vtkNodeId = m_Points->InsertNextPoint(x, y, z);  // UNV Id start from 1 and VTK Id start from 0.
        m_UnvToVtkNodeMap[nodeId] = vtkNodeId;  // Map of UNV node Id to VTK node Id

        ++nodeCount;
        iLine += 2;  // Move to the next node data
    }

    SPDLOG_INFO("Node data: {} nodes", nodeCount);
}

void UnvReader::processElementData(const std::vector<std::string>& dataSet) {
    m_EdgeMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    m_FaceMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    m_VolumeMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();

    // Set the points to the edge, face and volume grids
    m_EdgeMesh->SetPoints(m_Points);
    m_FaceMesh->SetPoints(m_Points);
    m_VolumeMesh->SetPoints(m_Points);

    int32_t edgeCount = 0;
    int32_t faceCount = 0;
    int32_t volumeCount = 0;

    int32_t iFileLine = 0;

    while (iFileLine < dataSet.size()) {
        std::istringstream iss1(dataSet.at(iFileLine));
        int32_t elemId, elemType;  // Other parameters are not necessary.
        iss1 >> elemId >> elemType;
        
        switch (elemType) {
        case 11:  // Edge element with 2 nodes
        {
            std::istringstream iss3(dataSet.at(iFileLine + 2));
            int32_t nodeIds[2];
            for (int32_t i = 0; i < 2; ++i) {
                iss3 >> nodeIds[i];
            }

            // Edge element
            vtkNew<vtkLine> line;
            for (int32_t i = 0; i < 2; ++i) {
                line->GetPointIds()
                  ->SetId(LINE_NODEORDER_UNV2VTK.at(i), m_UnvToVtkNodeMap.at(nodeIds[i]));
            }
            vtkIdType vtkElemId = m_EdgeMesh->InsertNextCell(line->GetCellType(), line->GetPointIds());
            m_UnvToVtkEdgeMap[elemId] = vtkElemId;  // Map of UNV element Id to VTK element Id

            ++edgeCount;
            iFileLine += 3;  // Move to the next element data
            break;
        }
        case 44:  // Quadrilateral with 4 nodes
        {
            std::istringstream iss2(dataSet.at(iFileLine + 1));
            int32_t nodeIds[4];
            for (int32_t i = 0; i < 4; ++i) {
                iss2 >> nodeIds[i];
            }

            // Quadrilateral element
            vtkNew<vtkQuad> quad;
            for (int32_t i = 0; i < 4; ++i) {
                quad->GetPointIds()
                  ->SetId(QUAD_NODEORDER_UNV2VTK.at(i), m_UnvToVtkNodeMap.at(nodeIds[i]));
            }
            vtkIdType vtkElemId = m_FaceMesh->InsertNextCell(quad->GetCellType(), quad->GetPointIds());
            m_UnvToVtkFaceMap[elemId] = vtkElemId;  // Map of UNV element Id to VTK element Id

            ++faceCount;
            iFileLine += 2;  // Move to the next element data
            break;
        }
        case 115:  // Hexahedron with 8 nodes
        {
            std::istringstream iss2(dataSet.at(iFileLine + 1));
            int32_t nodeIds[8];
            for (int32_t i = 0; i < 8; ++i) {
                iss2 >> nodeIds[i];
            }

            // Hexahedron element
            vtkNew<vtkHexahedron> hexahedron;
            for (int32_t i = 0; i < 8; ++i) {
                hexahedron->GetPointIds()
                  ->SetId(HEXA8_NODEORDER_UNV2VTK.at(i), m_UnvToVtkNodeMap.at(nodeIds[i]));
            }
            vtkIdType vtkElemId = m_VolumeMesh->InsertNextCell(hexahedron->GetCellType(), hexahedron->GetPointIds());
            m_UnvToVtkVolumeMap[elemId] = vtkElemId;  // Map of UNV element Id to VTK element Id

            ++volumeCount;
            iFileLine += 2;  // Move to the next element data
            break;
        }
        default:
            SPDLOG_ERROR("Not supported element type: {}. UNV element Id: {}", elemType, elemId);
            break;
        }
    }

    SPDLOG_INFO("Edge element data: {} elements", edgeCount);
    SPDLOG_INFO("Face element data: {} elements", faceCount);
    SPDLOG_INFO("Volume element data: {} elements", volumeCount);
}

void UnvReader::processGroupData(const std::vector<std::string>& dataSet) {
    int32_t iFileLine = 0;

    while (iFileLine < dataSet.size()) {
        std::istringstream iss1(dataSet.at(iFileLine));

        int32_t groupId, dummy1, dummy2, dummy3;
        int32_t dummy4, dummy5, dummy6, elemCount;
        iss1 >> groupId >> dummy1 >> dummy2 >> dummy3
            >> dummy4 >> dummy5 >> dummy6 >> elemCount;

        std::istringstream iss2(dataSet.at(iFileLine + 1));
        std::string groupName;
        iss2 >> groupName;

        std::vector<int32_t> elements;
        elements.reserve(elemCount);

        // Group type:
        // - 7: Node group
        // - 8: Element group
        int32_t groupType, elemId;

        if (elemCount % 2 == 0) {
            // If the number of elements is even
            for (int32_t iElem = 0; iElem < elemCount / 2; ++iElem) {
                std::istringstream iss3(dataSet.at(iFileLine + 2 + iElem));
    
                // Read the first four integers
                iss3 >> groupType >> elemId >> dummy1 >> dummy2;
                elements.push_back(elemId);

                // Read the second four integers
                iss3 >> groupType >> elemId >> dummy1 >> dummy2;
                elements.push_back(elemId);
            }
            iFileLine += 2 + elemCount / 2;  // Move to the next group data
        }
        else {
            // If the number of elements is odd
            for (int32_t iElem = 0; iElem < elemCount / 2 + 1; ++iElem) {
                std::istringstream iss(dataSet.at(iFileLine + 2 + iElem));
    
                // Read the first four integers
                iss >> groupType >> elemId >> dummy1 >> dummy2;
                elements.push_back(elemId);

                if (iElem == elemCount / 2) {
                    break;
                }

                // Read the second four integers
                iss >> groupType >> elemId >> dummy1 >> dummy2;
                elements.push_back(elemId);
            }
            iFileLine += 2 + elemCount / 2 + 1;  // Move to the next group data
        }
        
        if (groupType == 7) {
            // Node group
            SPDLOG_INFO("Node group Id: {}, Group Name: {}", groupId, groupName);
            vtkSmartPointer<vtkPoints> groupNodes = vtkSmartPointer<vtkPoints>::New();
            for (const auto& elem: elements) {
                vtkIdType vtkNodeId = m_UnvToVtkNodeMap[elem];
                groupNodes->InsertNextPoint(m_Points->GetPoint(vtkNodeId));
            }
            
            vtkSmartPointer<vtkUnstructuredGrid> groupMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
            groupMesh->SetPoints(groupNodes);

            m_MeshGroups.push_back(MeshGroup::New(groupName.c_str(), MeshGroupType::NODE, groupMesh));
        }
        else if (groupType == 8) {
            // Element group
            SPDLOG_INFO("Element group Id: {}, Group Name: {}", groupId, groupName);
        }
    }
}