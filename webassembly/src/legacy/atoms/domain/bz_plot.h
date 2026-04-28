#pragma once

#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <map>

struct CellInfo;
class AtomsTemplate;

namespace atoms {
namespace domain {

/**
 * @brief Brillouin Zone (BZ) 면의 한 면을 나타내는 구조체
 * 
 * Python의 bz1 리스트 원소에 해당
 * bz1.append((vor.vertices[vertices], normal))
 */
struct BZFacet {
    std::vector<std::array<double, 3>> vertices;  // 면을 구성하는 꼭지점들
    std::array<double, 3> normal;                  // 면의 법선 벡터 (정규화됨)
    int neighborId;                                 // 인접한 격자점 ID (디버깅용)
    
    BZFacet() : neighborId(-1) {
        normal = {0.0, 0.0, 0.0};
    }
};

/**
 * @brief BZ 계산 결과를 담는 구조체
 */
struct BZVerticesResult {
    std::vector<BZFacet> facets;           // BZ의 모든 면
    bool success;                          // 계산 성공 여부
    std::string errorMessage;              // 에러 메시지
    
    BZVerticesResult() : success(false) {}
};

/**
 * @brief Brillouin Zone 계산 클래스
 * 
 * Python의 bz_vertices() 함수를 C++로 이식
 * Voro++ 라이브러리를 사용하여 Voronoi 다이어그램 계산
 */
class BZCalculator {
public:
    /**
     * @brief 역격자 벡터로부터 Brillouin Zone의 꼭지점들을 계산
     * 
     * Python 코드:
     * def bz_vertices(icell):
     *     from scipy.spatial import Voronoi
     *     I = (np.indices((3, 3, 3)) - 1).reshape((3, 27))
     *     G = np.dot(icell.T, I).T
     *     vor = Voronoi(G)
     *     bz1 = []
     *     for vertices, points in zip(vor.ridge_vertices, vor.ridge_points):
     *         if -1 not in vertices and 13 in points:
     *             normal = G[points].sum(0)
     *             normal /= (normal**2).sum()**0.5
     *             bz1.append((vor.vertices[vertices], normal))
     *     return bz1
     * 
     * @param icell 역격자 벡터 3x3 행렬 (icell = 2π * inv(cell).T)
     *              icell[0] = b1, icell[1] = b2, icell[2] = b3
     * @return BZVerticesResult BZ 면들의 정보
     */
    static BZVerticesResult calculateBZVertices(const double icell[3][3]);
    
    /**
     * @brief 27개 역격자 점 생성 (3x3x3 격자)
     * 
     * Python: I = (np.indices((3, 3, 3)) - 1).reshape((3, 27))
     *         G = np.dot(icell.T, I).T
     * 
     * @param icell 역격자 벡터
     * @return 27개 격자점 좌표와 ID
     */
    static std::vector<std::array<double, 4>> generateLatticePoints(
        const double icell[3][3]);
    
    /**
     * @brief 면의 법선 벡터 계산 및 정규화
     * 
     * Python: normal = G[points].sum(0)
     *         normal /= (normal**2).sum()**0.5
     * 
     * @param point1 첫 번째 이웃 격자점
     * @param point2 두 번째 이웃 격자점
     * @return 정규화된 법선 벡터
     */
    static std::array<double, 3> calculateNormal(
        const std::array<double, 3>& point1,
        const std::array<double, 3>& point2);
    
    /**
     * @brief 컨테이너 경계 자동 계산
     * 
     * @param points 격자점들
     * @return {xmin, xmax, ymin, ymax, zmin, zmax}
     */
    static std::array<double, 6> calculateBounds(
        const std::vector<std::array<double, 4>>& points,
        double margin = 0.5);
};

/**
 * @brief 테스트용 유틸리티 함수들
 */
namespace BZTestUtils {
    /**
     * @brief 간단한 Cubic 격자 역격자 생성 (테스트용)
     * @param a 격자 상수
     */
    void createCubicReciprocalLattice(double icell[3][3], double a = 1.0);
    
    /**
     * @brief BZ 계산 결과 출력 (디버깅용)
     */
    void printBZResult(const BZVerticesResult& result);
    
    /**
     * @brief Python scipy.spatial.Voronoi 결과와 비교 (검증용)
     * @param icell 역격자 벡터
     * @param pythonVertices Python에서 계산된 vertices
     * @param pythonRidgeVertices Python에서 계산된 ridge_vertices
     */
    bool compareToPythonResult(
        const BZVerticesResult& cppResult,
        const std::vector<std::array<double, 3>>& pythonVertices,
        const std::vector<std::vector<int>>& pythonRidgeVertices);
}

/**
 * @brief 특수 k-점 계산 및 경로 파싱 클래스
 */
class BandpathCalculator {
public:
    /**
     * @brief Cubic 격자의 특수점 좌표 계산
     * @param icell 역격자 벡터 3x3 행렬
     * @return 특수점 이름과 좌표 맵
     */
    static std::map<std::string, std::array<double, 3>> getCubicSpecialPoints(
        const double icell[3][3]);
    
    /**
     * @brief Path 문자열 파싱
     * @param path 경로 문자열 (예: "GXMGRX")
     * @return 특수점 이름 배열
     */
    static std::vector<std::string> parsePath(const std::string& path);
    
    /**
     * @brief 두 점 사이를 선형 보간
     * @param start 시작점
     * @param end 끝점
     * @param npoints 보간 점 개수 (양 끝 포함)
     * @return 보간된 점 배열
     */
    static std::vector<std::array<double, 3>> interpolateSegment(
        const std::array<double, 3>& start,
        const std::array<double, 3>& end,
        int npoints);
    
    /**
     * @brief Path를 따라 k-points 생성
     * @param path 경로 문자열 (예: "GXMGRX")
     * @param icell 역격자 벡터 3x3 행렬
     * @param npointsPerSegment 각 세그먼트당 점 개수
     * @return k-points 좌표 배열
     */
    static std::vector<std::array<double, 3>> generateKpoints(
        const std::string& path,
        const double icell[3][3],
        int npointsPerSegment = 50);
};

/**
 * @brief BZ Plot 모드 전환을 관리하는 컨트롤러
 *
 * AtomsTemplate에서 renderer 가시성 토글 및 BZ 생성/정리를 위임받아 수행합니다.
 */
class BZPlotController {
public:
    bool enterBZPlotMode(
        AtomsTemplate* parent,
        const CellInfo& cellInfo,
        const std::string& path,
        int npoints,
        bool showVectors,
        bool showLabels,
        std::string& outErrorMessage);

    void exitBZPlotMode(AtomsTemplate* parent);

private:
    struct CameraState {
        std::array<double, 3> position {0.0, 0.0, 0.0};
        std::array<double, 3> focalPoint {0.0, 0.0, 0.0};
        std::array<double, 3> viewUp {0.0, 1.0, 0.0};
        std::array<double, 2> clippingRange {0.0, 0.0};
        double parallelScale = 1.0;
        bool parallelProjection = false;
    };

    bool m_wasCellVisible = false;
    bool m_hasSavedCamera = false;
    CameraState m_savedCameraState;
};

} // namespace domain
} // namespace atoms
