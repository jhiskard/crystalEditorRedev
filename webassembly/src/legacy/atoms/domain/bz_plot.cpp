#include "bz_plot.h"
#include "../atoms_template.h"
#include "../infrastructure/vtk_renderer.h"
#include "../../vtk_viewer.h"
#include "../domain/cell_manager.h"
#include "../../config/log_config.h"
#include <vtkCamera.h>
#include <voro++.hh>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace atoms {
namespace domain {

// ============================================================================
// BZCalculator Implementation
// ============================================================================

std::vector<std::array<double, 4>> BZCalculator::generateLatticePoints(
    const double icell[3][3]) 
{
    std::vector<std::array<double, 4>> points;
    points.reserve(27);  // 3x3x3 격자
    
    int id = 0;
    // Python: I = (np.indices((3, 3, 3)) - 1).reshape((3, 27))
    // i, j, k ∈ {-1, 0, 1}
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            for (int k = -1; k <= 1; k++) {
                // Python: G = np.dot(icell.T, I).T
                // G[id] = i*b1 + j*b2 + k*b3
                double x = i * icell[0][0] + j * icell[1][0] + k * icell[2][0];
                double y = i * icell[0][1] + j * icell[1][1] + k * icell[2][1];
                double z = i * icell[0][2] + j * icell[1][2] + k * icell[2][2];
                
                points.push_back({x, y, z, static_cast<double>(id)});
                id++;
            }
        }
    }
    
    SPDLOG_DEBUG("Generated {} lattice points", points.size());
    return points;
}

std::array<double, 6> BZCalculator::calculateBounds(
    const std::vector<std::array<double, 4>>& points,
    double margin)
{
    if (points.empty()) {
        return {-2.0, 2.0, -2.0, 2.0, -2.0, 2.0};
    }
    
    double xmin = points[0][0], xmax = points[0][0];
    double ymin = points[0][1], ymax = points[0][1];
    double zmin = points[0][2], zmax = points[0][2];
    
    for (const auto& p : points) {
        xmin = std::min(xmin, p[0]);
        xmax = std::max(xmax, p[0]);
        ymin = std::min(ymin, p[1]);
        ymax = std::max(ymax, p[1]);
        zmin = std::min(zmin, p[2]);
        zmax = std::max(zmax, p[2]);
    }
    
    // 마진 추가
    xmin -= margin; xmax += margin;
    ymin -= margin; ymax += margin;
    zmin -= margin; zmax += margin;
    
    SPDLOG_DEBUG("Calculated bounds: x[{:.3f}, {:.3f}], y[{:.3f}, {:.3f}], z[{:.3f}, {:.3f}]",
                xmin, xmax, ymin, ymax, zmin, zmax);
    
    return {xmin, xmax, ymin, ymax, zmin, zmax};
}

std::array<double, 3> BZCalculator::calculateNormal(
    const std::array<double, 3>& point1,
    const std::array<double, 3>& point2)
{
    // Python: normal = G[points].sum(0)
    // 두 이웃 격자점의 합 (중점 방향)
    double nx = point1[0] + point2[0];
    double ny = point1[1] + point2[1];
    double nz = point1[2] + point2[2];
    
    // Python: normal /= (normal**2).sum()**0.5
    double norm = std::sqrt(nx*nx + ny*ny + nz*nz);
    
    if (norm < 1e-10) {
        SPDLOG_WARN("Normal vector has near-zero length");
        return {0.0, 0.0, 1.0};  // 기본값
    }
    
    return {nx/norm, ny/norm, nz/norm};
}

BZVerticesResult BZCalculator::calculateBZVertices(const double icell[3][3]) {
    BZVerticesResult result;
    
    try {
        // 1. 27개 역격자 점 생성
        auto points = generateLatticePoints(icell);
        
        if (points.size() != 27) {
            result.errorMessage = "Failed to generate 27 lattice points";
            SPDLOG_ERROR("{}", result.errorMessage);
            return result;
        }
        
        // 중심 격자점 ID는 13 (i=0, j=0, k=0)
        // id = (i+1)*9 + (j+1)*3 + (k+1) = 1*9 + 1*3 + 1 = 13
        const int centralId = 13;
        SPDLOG_INFO("Central lattice point ID: {}", centralId);
        SPDLOG_DEBUG("Central point: ({:.3f}, {:.3f}, {:.3f})", 
                    points[centralId][0], points[centralId][1], points[centralId][2]);
        
        // 2. 컨테이너 경계 계산
        auto bounds = calculateBounds(points);
        
        // 3. Voro++ 컨테이너 생성
        voro::container con(
            bounds[0], bounds[1],  // x 범위
            bounds[2], bounds[3],  // y 범위
            bounds[4], bounds[5],  // z 범위
            6, 6, 6,               // 블록 분할 (성능 최적화)
            false, false, false,   // 주기성 (x, y, z) - BZ는 주기성 없음
            8                      // 메모리 할당 단위
        );
        
        SPDLOG_DEBUG("Created Voro++ container with bounds [{:.3f}, {:.3f}] x [{:.3f}, {:.3f}] x [{:.3f}, {:.3f}]",
                    bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
        
        // 4. 점들을 컨테이너에 추가
        for (const auto& p : points) {
            int id = static_cast<int>(p[3]);
            con.put(id, p[0], p[1], p[2]);
        }
        SPDLOG_DEBUG("Added {} points to Voro++ container", points.size());
        
        // 5. 중심 셀의 Voronoi 셀 계산
        voro::voronoicell_neighbor cell;
        
        // c_loop_all로 모든 셀 순회하여 중심 셀 찾기
        voro::c_loop_all vl(con);
        bool cellComputed = false;
        
        if (vl.start()) {
            do {
                int currentId = vl.pid();
                if (currentId == centralId) {
                    cellComputed = con.compute_cell(cell, vl);
                    break;
                }
            } while (vl.inc());
        }
        
        if (!cellComputed) {
            result.errorMessage = "Failed to compute Voronoi cell for central point";
            SPDLOG_ERROR("{}", result.errorMessage);
            return result;
        }
        
        SPDLOG_INFO("Successfully computed Voronoi cell");
        SPDLOG_DEBUG("  Number of faces: {}", cell.number_of_faces());
        SPDLOG_DEBUG("  Number of edges: {}", cell.number_of_edges());
        
        // 6. Vertex 및 Face 정보 추출
        std::vector<double> allVertices;
        std::vector<int> faceVertexIndices;
        std::vector<int> neighborIds;
        
        cell.vertices(allVertices);
        cell.face_vertices(faceVertexIndices);
        cell.neighbors(neighborIds);
        
        int numVertices = allVertices.size() / 3;
        SPDLOG_DEBUG("  Total vertices: {}", numVertices);
        SPDLOG_DEBUG("  Neighbor count: {}", neighborIds.size());
        
        // 7. Face별로 BZFacet 구성
        // Python: for vertices, points in zip(vor.ridge_vertices, vor.ridge_points):
        //             if -1 not in vertices and 13 in points:
        int offset = 0;
        for (size_t faceIdx = 0; faceIdx < neighborIds.size(); faceIdx++) {
            int numFaceVertices = faceVertexIndices[offset];
            int neighborId = neighborIds[faceIdx];
            
            // Python에서는 ridge_points[0] 또는 [1]이 13인지 확인
            // Voro++에서는 compute_cell(cell, 13)으로 이미 중심이 13임
            
            BZFacet facet;
            facet.neighborId = neighborId;
            
            // Face의 vertex들 추출
            for (int i = 1; i <= numFaceVertices; i++) {
                int vIdx = faceVertexIndices[offset + i];
                
                if (vIdx < 0 || vIdx >= numVertices) {
                    SPDLOG_WARN("Invalid vertex index: {} (max: {})", vIdx, numVertices - 1);
                    continue;
                }
                
                facet.vertices.push_back({
                    allVertices[3 * vIdx],
                    allVertices[3 * vIdx + 1],
                    allVertices[3 * vIdx + 2]
                });
            }
            
            // 법선 벡터 계산
            // Python: normal = G[points].sum(0) / norm
            // points = [13, neighborId]
            // G[13] = (0, 0, 0), G[neighborId] = neighbor point
            std::array<double, 3> neighborPoint = {
                points[neighborId][0],
                points[neighborId][1],
                points[neighborId][2]
            };
            std::array<double, 3> centralPoint = {0.0, 0.0, 0.0};
            
            facet.normal = calculateNormal(centralPoint, neighborPoint);
            
            result.facets.push_back(facet);
            
            SPDLOG_DEBUG("  Face {}: {} vertices, neighbor ID {}, normal ({:.3f}, {:.3f}, {:.3f})",
                        faceIdx, numFaceVertices, neighborId,
                        facet.normal[0], facet.normal[1], facet.normal[2]);
            
            offset += numFaceVertices + 1;
        }
        
        result.success = true;
        SPDLOG_INFO("BZ calculation completed: {} facets generated", result.facets.size());
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("Exception during BZ calculation: ") + e.what();
        SPDLOG_ERROR("{}", result.errorMessage);
    }
    
    return result;
}

// ============================================================================
// Test Utilities Implementation
// ============================================================================

namespace BZTestUtils {

void createCubicReciprocalLattice(double icell[3][3], double a) {
    // Cubic 격자의 역격자는 동일한 cubic
    // b1 = 2π/a * (1, 0, 0)
    // b2 = 2π/a * (0, 1, 0)
    // b3 = 2π/a * (0, 0, 1)
    
    const double factor = 2.0 * M_PI / a;
    
    // icell[0] = b1
    icell[0][0] = factor;
    icell[0][1] = 0.0;
    icell[0][2] = 0.0;
    
    // icell[1] = b2
    icell[1][0] = 0.0;
    icell[1][1] = factor;
    icell[1][2] = 0.0;
    
    // icell[2] = b3
    icell[2][0] = 0.0;
    icell[2][1] = 0.0;
    icell[2][2] = factor;
    
    SPDLOG_DEBUG("Created cubic reciprocal lattice with a = {:.3f}", a);
}

void printBZResult(const BZVerticesResult& result) {
    if (!result.success) {
        SPDLOG_ERROR("BZ calculation failed: {}", result.errorMessage);
        return;
    }
    
    SPDLOG_INFO("========================================");
    SPDLOG_INFO("BZ Calculation Result");
    SPDLOG_INFO("========================================");
    SPDLOG_INFO("Number of facets: {}", result.facets.size());
    SPDLOG_INFO("");
    
    for (size_t i = 0; i < result.facets.size(); i++) {
        const auto& facet = result.facets[i];
        
        SPDLOG_INFO("Facet {}:", i);
        SPDLOG_INFO("  Neighbor ID: {}", facet.neighborId);
        SPDLOG_INFO("  Normal: ({:.6f}, {:.6f}, {:.6f})", 
                   facet.normal[0], facet.normal[1], facet.normal[2]);
        SPDLOG_INFO("  Vertices ({}):", facet.vertices.size());
        
        for (size_t j = 0; j < facet.vertices.size(); j++) {
            const auto& v = facet.vertices[j];
            SPDLOG_INFO("    Vertex {}: ({:.6f}, {:.6f}, {:.6f})", 
                       j, v[0], v[1], v[2]);
        }
        SPDLOG_INFO("");
    }
    
    SPDLOG_INFO("========================================");
}

bool compareToPythonResult(
    const BZVerticesResult& cppResult,
    const std::vector<std::array<double, 3>>& pythonVertices,
    const std::vector<std::vector<int>>& pythonRidgeVertices)
{
    if (!cppResult.success) {
        SPDLOG_ERROR("C++ result is not successful, cannot compare");
        return false;
    }
    
    SPDLOG_INFO("========================================");
    SPDLOG_INFO("Comparing C++ result with Python result");
    SPDLOG_INFO("========================================");
    
    // 면 개수 비교
    SPDLOG_INFO("Number of facets - C++: {}, Python: {}", 
               cppResult.facets.size(), pythonRidgeVertices.size());
    
    bool facetCountMatch = (cppResult.facets.size() == pythonRidgeVertices.size());
    if (!facetCountMatch) {
        SPDLOG_WARN("Facet count mismatch!");
    }
    
    // 꼭지점 개수 비교
    SPDLOG_INFO("Python vertices count: {}", pythonVertices.size());
    
    // 각 면의 꼭지점 개수 비교
    int matchingFaces = 0;
    for (size_t i = 0; i < std::min(cppResult.facets.size(), pythonRidgeVertices.size()); i++) {
        size_t cppVertCount = cppResult.facets[i].vertices.size();
        size_t pyVertCount = pythonRidgeVertices[i].size();
        
        if (cppVertCount == pyVertCount) {
            matchingFaces++;
        }
        
        SPDLOG_DEBUG("Face {}: C++ {} vertices, Python {} vertices", 
                    i, cppVertCount, pyVertCount);
    }
    
    SPDLOG_INFO("Matching faces (vertex count): {}/{}", 
               matchingFaces, std::min(cppResult.facets.size(), pythonRidgeVertices.size()));
    
    bool isMatch = facetCountMatch && (matchingFaces == cppResult.facets.size());
    
    if (isMatch) {
        SPDLOG_INFO("✓ Results match!");
    } else {
        SPDLOG_WARN("✗ Results do not match completely");
    }
    
    SPDLOG_INFO("========================================");
    
    return isMatch;
}

} // namespace BZTestUtils

// ============================================================================ 
// BZPlotController Implementation
// ============================================================================

bool BZPlotController::enterBZPlotMode(
    AtomsTemplate* parent,
    const CellInfo& cellInfo,
    const std::string& path,
    int npoints,
    bool showVectors,
    bool showLabels,
    std::string& outErrorMessage
) {
    outErrorMessage.clear();

    if (!parent) {
        outErrorMessage = "Parent is null";
        return false;
    }

    try {
        double cell[3][3];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                cell[i][j] = cellInfo.matrix[i][j];
            }
        }

        double icell[3][3];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                icell[i][j] = cellInfo.invmatrix[j][i];
            }
        }

        SPDLOG_INFO("Calculating BZ vertices...");
        auto bzData = atoms::domain::BZCalculator::calculateBZVertices(icell);

        if (!bzData.success) {
            outErrorMessage = "BZ calculation failed: " + bzData.errorMessage;
            SPDLOG_ERROR("{}", outErrorMessage);
            return false;
        }

        SPDLOG_INFO("BZ calculation successful. Facets: {}, Vertices in first facet: {}",
                    bzData.facets.size(),
                    bzData.facets.empty() ? 0 : bzData.facets[0].vertices.size());

        atoms::infrastructure::VTKRenderer* renderer = parent->vtkRenderer();
        if (!renderer) {
            outErrorMessage = "VTKRenderer not initialized";
            SPDLOG_ERROR("{}", outErrorMessage);
            return false;
        }

        if (!m_hasSavedCamera) {
            vtkCamera* camera = VtkViewer::Instance().GetActiveCamera();
            if (camera) {
                camera->GetPosition(m_savedCameraState.position.data());
                camera->GetFocalPoint(m_savedCameraState.focalPoint.data());
                camera->GetViewUp(m_savedCameraState.viewUp.data());
                camera->GetClippingRange(m_savedCameraState.clippingRange.data());
                m_savedCameraState.parallelScale = camera->GetParallelScale();
                m_savedCameraState.parallelProjection = camera->GetParallelProjection() != 0;
                m_hasSavedCamera = true;
            }
        }

        renderer->setAllAtomGroupsVisible(false);
        renderer->setAllBondGroupsVisible(false);

        m_wasCellVisible = atoms::domain::isCellVisible();
        if (m_wasCellVisible) {
            renderer->setUnitCellVisible(false);
        }

        std::string pathStr = path;
        if (pathStr == "All" || pathStr == "all" || pathStr == "ALL") {
            SPDLOG_INFO("'All' keyword detected - determining lattice type automatically");

            std::string latticeType =
                atoms::domain::SpecialPointsDatabase::detectLatticeType(cell);

            SPDLOG_INFO("Detected lattice type: {}", latticeType);

            pathStr = atoms::domain::SpecialPointsDatabase::getDefaultPath(latticeType);

            SPDLOG_INFO("Auto-selected path: '{}'", pathStr);
        }

        SPDLOG_INFO("Creating complete BZ Plot with path: '{}'", pathStr);

        renderer->createCompleteBZPlot(
            bzData,
            cell,
            icell,
            showVectors,
            showLabels,
            pathStr,
            npoints
        );

        VtkViewer::Instance().FitViewToVisibleProps();
        SPDLOG_INFO("=== Switched to BZ Plot mode ===");
        return true;

    } catch (const std::exception& e) {
        outErrorMessage = std::string("Exception: ") + e.what();
        SPDLOG_ERROR("BZ Plot rendering failed: {}", outErrorMessage);
        return false;
    }
}

void BZPlotController::exitBZPlotMode(AtomsTemplate* parent) {
    SPDLOG_INFO("=== Switching to Crystal mode ===");

    atoms::infrastructure::VTKRenderer* renderer = parent ? parent->vtkRenderer() : nullptr;
    if (renderer) {
        SPDLOG_INFO("Hiding and clearing BZ Plot...");
        renderer->setBZPlotVisible(false);
        renderer->clearBZPlot();

        renderer->setAllAtomGroupsVisible(true);
        renderer->setAllBondGroupsVisible(true);

        if (m_wasCellVisible && atoms::domain::isCellVisible()) {
            renderer->setUnitCellVisible(true);
        }
    }

    if (m_hasSavedCamera) {
        vtkCamera* camera = VtkViewer::Instance().GetActiveCamera();
        if (camera) {
            camera->SetPosition(
                m_savedCameraState.position[0],
                m_savedCameraState.position[1],
                m_savedCameraState.position[2]
            );
            camera->SetFocalPoint(
                m_savedCameraState.focalPoint[0],
                m_savedCameraState.focalPoint[1],
                m_savedCameraState.focalPoint[2]
            );
            camera->SetViewUp(
                m_savedCameraState.viewUp[0],
                m_savedCameraState.viewUp[1],
                m_savedCameraState.viewUp[2]
            );
            camera->SetClippingRange(
                m_savedCameraState.clippingRange[0],
                m_savedCameraState.clippingRange[1]
            );
            camera->SetParallelScale(m_savedCameraState.parallelScale);
            camera->SetParallelProjection(m_savedCameraState.parallelProjection);
        }
        m_hasSavedCamera = false;
    }

    VtkViewer::Instance().RequestRender();
    SPDLOG_INFO("=== Back to crystal mode ===");
}

} // namespace domain
} // namespace atoms
