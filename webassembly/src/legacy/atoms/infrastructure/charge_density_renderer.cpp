// atoms/infrastructure/charge_density_renderer.cpp
#include "charge_density_renderer.h"
#include "chgcar_parser.h"
#include "../../config/log_config.h"
#include "../../vtk_viewer.h"

#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkScalarBarActor.h>
#include <vtkExtractVOI.h>
#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkColorTransferFunction.h>
#include <vtkCell.h>
#include <vtkPolyData.h>

#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {
constexpr double kSliceEpsilon = 1e-8;

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

double norm(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

Vec3 normalize(const Vec3& v) {
    const double length = norm(v);
    if (length <= kSliceEpsilon) {
        return {};
    }
    return { v.x / length, v.y / length, v.z / length };
}

bool isNearPoint(const Vec3& a, const Vec3& b, double epsilon = 1e-6) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    const double dz = a.z - b.z;
    return (dx * dx + dy * dy + dz * dz) <= epsilon * epsilon;
}

bool isInsideBounds(const Vec3& p, const double bounds[6], double epsilon = 1e-6) {
    return p.x >= bounds[0] - epsilon && p.x <= bounds[1] + epsilon &&
           p.y >= bounds[2] - epsilon && p.y <= bounds[3] + epsilon &&
           p.z >= bounds[4] - epsilon && p.z <= bounds[5] + epsilon;
}

float sampleTrilinearValue(const float* ptr,
                           int nx, int ny, int nz,
                           const Vec3& point,
                           const double bounds[6],
                           float outsideValue) {
    if (!ptr || nx <= 0 || ny <= 0 || nz <= 0) {
        return outsideValue;
    }
    if (!isInsideBounds(point, bounds)) {
        return outsideValue;
    }

    const double extentX = bounds[1] - bounds[0];
    const double extentY = bounds[3] - bounds[2];
    const double extentZ = bounds[5] - bounds[4];
    if (extentX <= kSliceEpsilon || extentY <= kSliceEpsilon || extentZ <= kSliceEpsilon) {
        return outsideValue;
    }

    double tx = (point.x - bounds[0]) / extentX;
    double ty = (point.y - bounds[2]) / extentY;
    double tz = (point.z - bounds[4]) / extentZ;
    tx = std::clamp(tx, 0.0, 1.0);
    ty = std::clamp(ty, 0.0, 1.0);
    tz = std::clamp(tz, 0.0, 1.0);

    const double fx = tx * static_cast<double>(std::max(0, nx - 1));
    const double fy = ty * static_cast<double>(std::max(0, ny - 1));
    const double fz = tz * static_cast<double>(std::max(0, nz - 1));

    const int x0 = static_cast<int>(std::floor(fx));
    const int y0 = static_cast<int>(std::floor(fy));
    const int z0 = static_cast<int>(std::floor(fz));
    const int x1 = std::min(x0 + 1, nx - 1);
    const int y1 = std::min(y0 + 1, ny - 1);
    const int z1 = std::min(z0 + 1, nz - 1);

    const double dx = fx - static_cast<double>(x0);
    const double dy = fy - static_cast<double>(y0);
    const double dz = fz - static_cast<double>(z0);

    auto valueAt = [&](int ix, int iy, int iz) -> double {
        const int idx = ix + iy * nx + iz * nx * ny;
        return static_cast<double>(ptr[idx]);
    };

    const double c000 = valueAt(x0, y0, z0);
    const double c100 = valueAt(x1, y0, z0);
    const double c010 = valueAt(x0, y1, z0);
    const double c110 = valueAt(x1, y1, z0);
    const double c001 = valueAt(x0, y0, z1);
    const double c101 = valueAt(x1, y0, z1);
    const double c011 = valueAt(x0, y1, z1);
    const double c111 = valueAt(x1, y1, z1);

    const double c00 = c000 * (1.0 - dx) + c100 * dx;
    const double c01 = c001 * (1.0 - dx) + c101 * dx;
    const double c10 = c010 * (1.0 - dx) + c110 * dx;
    const double c11 = c011 * (1.0 - dx) + c111 * dx;
    const double c0 = c00 * (1.0 - dy) + c10 * dy;
    const double c1 = c01 * (1.0 - dy) + c11 * dy;

    return static_cast<float>(c0 * (1.0 - dz) + c1 * dz);
}

bool normalizeArray3(double v[3], double epsilon = 1e-8) {
    const double n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (n <= epsilon) {
        return false;
    }
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
    return true;
}

void transformFractionalDirectionToWorld(const float lattice[3][3],
                                         const double fractionalDir[3],
                                         double worldDir[3]) {
    worldDir[0] = static_cast<double>(lattice[0][0]) * fractionalDir[0] +
                  static_cast<double>(lattice[1][0]) * fractionalDir[1] +
                  static_cast<double>(lattice[2][0]) * fractionalDir[2];
    worldDir[1] = static_cast<double>(lattice[0][1]) * fractionalDir[0] +
                  static_cast<double>(lattice[1][1]) * fractionalDir[1] +
                  static_cast<double>(lattice[2][1]) * fractionalDir[2];
    worldDir[2] = static_cast<double>(lattice[0][2]) * fractionalDir[0] +
                  static_cast<double>(lattice[1][2]) * fractionalDir[1] +
                  static_cast<double>(lattice[2][2]) * fractionalDir[2];
}

bool extractSliceActorNormal(vtkActor* actor, double normal[3]) {
    if (!actor || !normal) {
        return false;
    }
    vtkMapper* mapper = actor->GetMapper();
    if (!mapper) {
        return false;
    }
    vtkPolyData* polyData = vtkPolyData::SafeDownCast(mapper->GetInput());
    if (!polyData || polyData->GetNumberOfCells() <= 0) {
        return false;
    }

    const vtkIdType cellCount = polyData->GetNumberOfCells();
    for (vtkIdType cellId = 0; cellId < cellCount; ++cellId) {
        vtkCell* cell = polyData->GetCell(cellId);
        if (!cell || cell->GetNumberOfPoints() < 3) {
            continue;
        }
        double p0[3] = {0.0, 0.0, 0.0};
        double p1[3] = {0.0, 0.0, 0.0};
        double p2[3] = {0.0, 0.0, 0.0};
        cell->GetPoints()->GetPoint(0, p0);
        cell->GetPoints()->GetPoint(1, p1);
        cell->GetPoints()->GetPoint(2, p2);

        const double v1[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
        const double v2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
        normal[0] = v1[1] * v2[2] - v1[2] * v2[1];
        normal[1] = v1[2] * v2[0] - v1[0] * v2[2];
        normal[2] = v1[0] * v2[1] - v1[1] * v2[0];
        if (normalizeArray3(normal)) {
            return true;
        }
    }
    return false;
}
}  // namespace


namespace atoms {
namespace infrastructure {

ChargeDensityRenderer::ChargeDensityRenderer(VTKRenderer* vtkRenderer)
    : m_vtkRenderer(vtkRenderer) {
    SPDLOG_DEBUG("ChargeDensityRenderer initialized");
}

ChargeDensityRenderer::~ChargeDensityRenderer() {
    clear();
}

bool ChargeDensityRenderer::loadFromFile(const std::string& filePath) {
    SPDLOG_INFO("Loading CHGCAR file: {}", filePath);
    
    auto result = ChgcarParser::parse(filePath);
    if (!result.success) {
        SPDLOG_ERROR("Failed to parse CHGCAR: {}", result.errorMessage);
        return false;
    }
    
    // 격자 정보 저장
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m_lattice[i][j] = result.lattice[i][j];
        }
    }
    
    m_gridShape = result.gridShape;
    m_minValue = result.minValue;
    m_maxValue = result.maxValue;
    
    // VTK 이미지 데이터 생성
    createVTKImageData(result.density);
    
    SPDLOG_INFO("CHGCAR loaded: {}x{}x{}, range=[{:.4e}, {:.4e}]",
                m_gridShape[0], m_gridShape[1], m_gridShape[2],
                m_minValue, m_maxValue);
    
    return true;
}

void ChargeDensityRenderer::createVTKImageData(const std::vector<float>& data) {
    m_imageData = vtkSmartPointer<vtkImageData>::New();
    
    // 그리드 크기 설정
    m_imageData->SetDimensions(m_gridShape[0], m_gridShape[1], m_gridShape[2]);
    
    // ✅ 분수 좌표 기준으로 설정 (0~1 범위)
    // 각 그리드 포인트가 분수 좌표 공간에서 균등하게 분포
    double spacing[3] = {
        1.0 / m_gridShape[0],
        1.0 / m_gridShape[1],
        1.0 / m_gridShape[2]
    };
    m_imageData->SetSpacing(spacing);
    m_imageData->SetOrigin(0, 0, 0);
    
    // 스칼라 데이터 할당
    m_imageData->AllocateScalars(VTK_FLOAT, 1);
    
    // 데이터 복사 (VASP 순서: z fastest → VTK 순서로 변환)
    float* ptr = static_cast<float*>(m_imageData->GetScalarPointer());
    
    for (int ix = 0; ix < m_gridShape[0]; ++ix) {
        for (int iy = 0; iy < m_gridShape[1]; ++iy) {
            for (int iz = 0; iz < m_gridShape[2]; ++iz) {
                // VASP 인덱스: iz + iy * nz + ix * ny * nz
                int vaspIdx = iz + iy * m_gridShape[2] + ix * m_gridShape[1] * m_gridShape[2];
                
                // VTK 인덱스: ix + iy * nx + iz * nx * ny
                int vtkIdx = ix + iy * m_gridShape[0] + iz * m_gridShape[0] * m_gridShape[1];
                
                ptr[vtkIdx] = data[vaspIdx];
            }
        }
    }
    
    m_imageData->Modified();
    
    SPDLOG_DEBUG("VTK ImageData created in fractional coordinates: {}x{}x{}",
                 m_gridShape[0], m_gridShape[1], m_gridShape[2]);
}

void ChargeDensityRenderer::setIsosurfaceValue(float value) {
    m_isosurfaceValue = value;
    updateIsosurface();
}

void ChargeDensityRenderer::setIsosurfaceColor(float r, float g, float b, float alpha) {
    m_isoColor[0] = r;
    m_isoColor[1] = g;
    m_isoColor[2] = b;
    m_isoColor[3] = alpha;
    
    if (m_isosurfaceActor) {
        m_isosurfaceActor->GetProperty()->SetColor(r, g, b);
        m_isosurfaceActor->GetProperty()->SetOpacity(alpha);
        m_isosurfaceActor->Modified();
    }
}

void ChargeDensityRenderer::setIsosurfaceVisible(bool visible) {
    SPDLOG_INFO("setIsosurfaceVisible: {}", visible);
    
    m_isosurfaceVisible = visible;
    
    if (visible && !m_isosurfaceActor && m_imageData) {
        SPDLOG_INFO("Creating isosurface for first time");
        updateIsosurface();
    }
    
    if (m_isosurfaceActor) {
        m_isosurfaceActor->SetVisibility(visible ? 1 : 0);
        SPDLOG_INFO("Isosurface actor visibility set to {}", visible);
    } else {
        SPDLOG_WARN("No isosurface actor to set visibility");
    }
}

void ChargeDensityRenderer::addIsosurface(float value, float r, float g, float b, float alpha) {
    if (!m_imageData) {
        SPDLOG_WARN("No image data for isosurface");
        return;
    }
    
    vtkSmartPointer<vtkContourFilter> contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    contourFilter->SetInputData(m_imageData);
    contourFilter->SetValue(0, value);
    contourFilter->Update();
    
    vtkPolyData* contourOutput = contourFilter->GetOutput();
    if (!contourOutput || contourOutput->GetNumberOfPoints() == 0) {
        SPDLOG_WARN("Isosurface has no geometry at value {:.4e}", value);
        return;
    }
    
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    
    double matrix[16] = {
        m_lattice[0][0], m_lattice[1][0], m_lattice[2][0], 0,
        m_lattice[0][1], m_lattice[1][1], m_lattice[2][1], 0,
        m_lattice[0][2], m_lattice[1][2], m_lattice[2][2], 0,
        0, 0, 0, 1
    };
    transform->SetMatrix(matrix);
    
    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputData(contourOutput);
    transformFilter->SetTransform(transform);
    transformFilter->Update();
    
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());
    mapper->ScalarVisibilityOff();
    
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(r, g, b);
    actor->GetProperty()->SetOpacity(alpha);
    actor->SetVisibility(1);
    
    VtkViewer::Instance().AddActor(actor, false);
    
    m_isosurfaces.push_back({value, actor});
}

void ChargeDensityRenderer::setIsosurfaceColorForValue(float value, float r, float g, float b, float alpha) {
    for (auto& iso : m_isosurfaces) {
        if (iso.actor && iso.value == value) {
            iso.actor->GetProperty()->SetColor(r, g, b);
            iso.actor->GetProperty()->SetOpacity(alpha);
            iso.actor->Modified();
        }
    }
}

void ChargeDensityRenderer::clearIsosurfaces() {
    for (auto& iso : m_isosurfaces) {
        if (iso.actor) {
            VtkViewer::Instance().RemoveActor(iso.actor);
        }
    }
    m_isosurfaces.clear();
}

void ChargeDensityRenderer::updateIsosurface() {
    if (!m_imageData) {
        SPDLOG_WARN("No image data for isosurface");
        return;
    }
    
    SPDLOG_INFO("Creating isosurface with value: {:.4e}", m_isosurfaceValue);
    
    // 기존 액터 제거
    if (m_isosurfaceActor) {
        VtkViewer::Instance().RemoveActor(m_isosurfaceActor);
        m_isosurfaceActor = nullptr;
    }
    
    // ContourFilter 생성
    m_contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    m_contourFilter->SetInputData(m_imageData);
    m_contourFilter->SetValue(0, m_isosurfaceValue);
    m_contourFilter->Update();
    
    vtkPolyData* contourOutput = m_contourFilter->GetOutput();
    SPDLOG_INFO("Contour output: {} points, {} cells", 
                contourOutput->GetNumberOfPoints(), contourOutput->GetNumberOfCells());
    
    if (contourOutput->GetNumberOfPoints() == 0) {
        SPDLOG_WARN("Isosurface has no geometry at value {:.4e}", m_isosurfaceValue);
        return;
    }
    
    // ✅ 분수 좌표 → 카르테시안 좌표 변환을 위한 Transform
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    
    // 격자 행렬을 4x4 변환 행렬로 설정
    // [a1x a2x a3x 0]
    // [a1y a2y a3y 0]
    // [a1z a2z a3z 0]
    // [0   0   0   1]
    double matrix[16] = {
        m_lattice[0][0], m_lattice[1][0], m_lattice[2][0], 0,
        m_lattice[0][1], m_lattice[1][1], m_lattice[2][1], 0,
        m_lattice[0][2], m_lattice[1][2], m_lattice[2][2], 0,
        0, 0, 0, 1
    };
    transform->SetMatrix(matrix);
    
    // TransformFilter 적용
    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = 
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputData(contourOutput);
    transformFilter->SetTransform(transform);
    transformFilter->Update();
    
    // Mapper 생성
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());
    mapper->ScalarVisibilityOff();
    
    // Actor 생성
    m_isosurfaceActor = vtkSmartPointer<vtkActor>::New();
    m_isosurfaceActor->SetMapper(mapper);
    m_isosurfaceActor->GetProperty()->SetColor(
        m_isoColor[0], m_isoColor[1], m_isoColor[2]);
    m_isosurfaceActor->GetProperty()->SetOpacity(m_isoColor[3]);
    m_isosurfaceActor->SetVisibility(1);
    
    // 렌더러에 추가
    VtkViewer::Instance().AddActor(m_isosurfaceActor, false);
    
    SPDLOG_INFO("Isosurface actor added with lattice transform");
}

void ChargeDensityRenderer::clear() {
    if (m_isosurfaceActor) {
        VtkViewer::Instance().RemoveActor(m_isosurfaceActor);
        m_isosurfaceActor = nullptr;
    }
    
    if (m_sliceActor) {
        VtkViewer::Instance().RemoveActor(m_sliceActor);
        m_sliceActor = nullptr;
    }
    
    clearIsosurfaces();
    
    m_imageData = nullptr;
    m_contourFilter = nullptr;
    
    SPDLOG_DEBUG("ChargeDensityRenderer cleared");
}

void ChargeDensityRenderer::setSlicePlane(SlicePlane plane) {
    m_slicePlane = plane;
    if (m_sliceVisible) {
        updateSlice();
    }
}

void ChargeDensityRenderer::setSlicePosition(float position) {
    m_slicePosition = std::clamp(position, 0.0f, 1.0f);
    if (m_sliceVisible) {
        updateSlice();
    }
}

void ChargeDensityRenderer::setSliceMillerIndices(int h, int k, int l) {
    const int nextH = std::clamp(h, 0, 6);
    const int nextK = std::clamp(k, 0, 6);
    const int nextL = std::clamp(l, 0, 6);
    if (m_sliceMillerH == nextH && m_sliceMillerK == nextK && m_sliceMillerL == nextL) {
        return;
    }
    m_sliceMillerH = nextH;
    m_sliceMillerK = nextK;
    m_sliceMillerL = nextL;
    if (m_sliceVisible && m_slicePlane == SlicePlane::Miller) {
        updateSlice();
    }
}

void ChargeDensityRenderer::setSliceVisible(bool visible) {
    m_sliceVisible = visible;
    
    if (visible && !m_sliceActor && m_imageData) {
        updateSlice();
    }
    
    if (m_sliceActor) {
        m_sliceActor->SetVisibility(visible ? 1 : 0);
    }
}

bool ChargeDensityRenderer::resolveMillerPlaneInDataCoords(double normal[3], double& offset) const {
    if (!m_imageData || normal == nullptr) {
        return false;
    }
    if (m_sliceMillerH == 0 && m_sliceMillerK == 0 && m_sliceMillerL == 0) {
        return false;
    }

    double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    m_imageData->GetBounds(bounds);

    const double extentX = std::max(bounds[1] - bounds[0], kSliceEpsilon);
    const double extentY = std::max(bounds[3] - bounds[2], kSliceEpsilon);
    const double extentZ = std::max(bounds[5] - bounds[4], kSliceEpsilon);

    normal[0] = static_cast<double>(m_sliceMillerH) / extentX;
    normal[1] = static_cast<double>(m_sliceMillerK) / extentY;
    normal[2] = static_cast<double>(m_sliceMillerL) / extentZ;

    const double cFractional =
        static_cast<double>(m_sliceMillerH + m_sliceMillerK + m_sliceMillerL) *
        static_cast<double>(m_slicePosition);

    // h*u + k*v + l*w = c, where u,v,w are normalized within current image bounds.
    offset = cFractional
           + static_cast<double>(m_sliceMillerH) * bounds[0] / extentX
           + static_cast<double>(m_sliceMillerK) * bounds[2] / extentY
           + static_cast<double>(m_sliceMillerL) * bounds[4] / extentZ;
    return true;
}

bool ChargeDensityRenderer::getSliceValues(std::vector<float>& values,
                                           int& width,
                                           int& height,
                                           int millerMaxResolution) const {
    if (!m_imageData) {
        return false;
    }

    const int nx = m_gridShape[0];
    const int ny = m_gridShape[1];
    const int nz = m_gridShape[2];
    if (nx <= 0 || ny <= 0 || nz <= 0) {
        return false;
    }

    float* ptr = static_cast<float*>(m_imageData->GetScalarPointer());
    if (!ptr) {
        return false;
    }

    if (m_slicePlane == SlicePlane::Miller) {
        double normalData[3] = {0.0, 0.0, 0.0};
        double planeOffset = 0.0;
        if (!resolveMillerPlaneInDataCoords(normalData, planeOffset)) {
            return false;
        }

        Vec3 normalVec{normalData[0], normalData[1], normalData[2]};
        if (norm(normalVec) <= kSliceEpsilon) {
            return false;
        }

        double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        m_imageData->GetBounds(bounds);

        const std::array<Vec3, 8> corners = {{
            {bounds[0], bounds[2], bounds[4]},
            {bounds[1], bounds[2], bounds[4]},
            {bounds[0], bounds[3], bounds[4]},
            {bounds[1], bounds[3], bounds[4]},
            {bounds[0], bounds[2], bounds[5]},
            {bounds[1], bounds[2], bounds[5]},
            {bounds[0], bounds[3], bounds[5]},
            {bounds[1], bounds[3], bounds[5]},
        }};

        constexpr int kEdges[12][2] = {
            {0, 1}, {0, 2}, {0, 4}, {1, 3}, {1, 5}, {2, 3},
            {2, 6}, {3, 7}, {4, 5}, {4, 6}, {5, 7}, {6, 7}
        };

        std::vector<Vec3> intersections;
        intersections.reserve(12);
        auto addIntersection = [&](const Vec3& point) {
            for (const Vec3& existing : intersections) {
                if (isNearPoint(existing, point)) {
                    return;
                }
            }
            intersections.push_back(point);
        };

        auto signedDistance = [&](const Vec3& point) -> double {
            return dot(normalVec, point) - planeOffset;
        };

        for (const auto& edge : kEdges) {
            const Vec3& p0 = corners[edge[0]];
            const Vec3& p1 = corners[edge[1]];
            const double d0 = signedDistance(p0);
            const double d1 = signedDistance(p1);
            if (std::abs(d0) <= kSliceEpsilon) {
                addIntersection(p0);
            }
            if (std::abs(d1) <= kSliceEpsilon) {
                addIntersection(p1);
            }
            if ((d0 < -kSliceEpsilon && d1 > kSliceEpsilon) ||
                (d0 > kSliceEpsilon && d1 < -kSliceEpsilon)) {
                const double t = d0 / (d0 - d1);
                Vec3 point;
                point.x = p0.x + (p1.x - p0.x) * t;
                point.y = p0.y + (p1.y - p0.y) * t;
                point.z = p0.z + (p1.z - p0.z) * t;
                addIntersection(point);
            }
        }

        if (intersections.size() < 3) {
            return false;
        }

        Vec3 center{};
        for (const Vec3& point : intersections) {
            center.x += point.x;
            center.y += point.y;
            center.z += point.z;
        }
        const double invCount = 1.0 / static_cast<double>(intersections.size());
        center.x *= invCount;
        center.y *= invCount;
        center.z *= invCount;

        const Vec3 normalUnit = normalize(normalVec);
        Vec3 reference = (std::abs(normalUnit.z) < 0.9) ? Vec3{0.0, 0.0, 1.0} : Vec3{0.0, 1.0, 0.0};
        Vec3 uAxis = normalize(cross(reference, normalUnit));
        if (norm(uAxis) <= kSliceEpsilon) {
            reference = {1.0, 0.0, 0.0};
            uAxis = normalize(cross(reference, normalUnit));
        }
        Vec3 vAxis = normalize(cross(normalUnit, uAxis));
        if (norm(uAxis) <= kSliceEpsilon || norm(vAxis) <= kSliceEpsilon) {
            return false;
        }

        double minU = std::numeric_limits<double>::max();
        double maxU = std::numeric_limits<double>::lowest();
        double minV = std::numeric_limits<double>::max();
        double maxV = std::numeric_limits<double>::lowest();
        for (const Vec3& point : intersections) {
            const Vec3 rel{point.x - center.x, point.y - center.y, point.z - center.z};
            const double u = dot(rel, uAxis);
            const double v = dot(rel, vAxis);
            minU = std::min(minU, u);
            maxU = std::max(maxU, u);
            minV = std::min(minV, v);
            maxV = std::max(maxV, v);
        }

        const double rangeU = maxU - minU;
        const double rangeV = maxV - minV;
        if (rangeU <= kSliceEpsilon || rangeV <= kSliceEpsilon) {
            return false;
        }

        const double stepX = (nx > 1) ? (bounds[1] - bounds[0]) / static_cast<double>(nx - 1) : 1.0;
        const double stepY = (ny > 1) ? (bounds[3] - bounds[2]) / static_cast<double>(ny - 1) : 1.0;
        const double stepZ = (nz > 1) ? (bounds[5] - bounds[4]) / static_cast<double>(nz - 1) : 1.0;
        double baseStep = (stepX + stepY + stepZ) / 3.0;
        if (baseStep <= kSliceEpsilon) {
            baseStep = 1.0;
        }

        width = std::max(2, static_cast<int>(std::round(rangeU / baseStep)) + 1);
        height = std::max(2, static_cast<int>(std::round(rangeV / baseStep)) + 1);
        int maxResolution = millerMaxResolution;
        if (maxResolution < 2) {
            maxResolution = 1024;
        }
        width = std::min(width, maxResolution);
        height = std::min(height, maxResolution);

        values.assign(static_cast<size_t>(width) * static_cast<size_t>(height), m_minValue);
        for (int j = 0; j < height; ++j) {
            const double tv = (height > 1)
                ? static_cast<double>(j) / static_cast<double>(height - 1)
                : 0.0;
            const double vCoord = maxV - tv * rangeV;
            for (int i = 0; i < width; ++i) {
                const double tu = (width > 1)
                    ? static_cast<double>(i) / static_cast<double>(width - 1)
                    : 0.0;
                const double uCoord = minU + tu * rangeU;
                Vec3 samplePoint;
                samplePoint.x = center.x + uAxis.x * uCoord + vAxis.x * vCoord;
                samplePoint.y = center.y + uAxis.y * uCoord + vAxis.y * vCoord;
                samplePoint.z = center.z + uAxis.z * uCoord + vAxis.z * vCoord;

                const size_t idx = static_cast<size_t>(j) * static_cast<size_t>(width) +
                                   static_cast<size_t>(i);
                values[idx] = sampleTrilinearValue(ptr, nx, ny, nz, samplePoint, bounds, m_minValue);
            }
        }

        return true;
    }

    int sliceDim = 0;
    switch (m_slicePlane) {
        case SlicePlane::XY:
            width = nx;
            height = ny;
            sliceDim = nz;
            break;
        case SlicePlane::XZ:
            width = nx;
            height = nz;
            sliceDim = ny;
            break;
        case SlicePlane::YZ:
            width = ny;
            height = nz;
            sliceDim = nx;
            break;
        case SlicePlane::Miller:
            return false;
    }

    const int maxIndex = std::max(0, sliceDim - 1);
    int sliceIndex = static_cast<int>(std::round(m_slicePosition * static_cast<float>(maxIndex)));
    sliceIndex = std::clamp(sliceIndex, 0, maxIndex);

    values.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
    auto valueAt = [&](int ix, int iy, int iz) -> float {
        const int idx = ix + iy * nx + iz * nx * ny;
        return ptr[idx];
    };

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            float value = 0.0f;
            switch (m_slicePlane) {
                case SlicePlane::XY:
                    value = valueAt(i, j, sliceIndex);
                    break;
                case SlicePlane::XZ:
                    value = valueAt(i, sliceIndex, j);
                    break;
                case SlicePlane::YZ:
                    value = valueAt(sliceIndex, i, j);
                    break;
                case SlicePlane::Miller:
                    value = 0.0f;
                    break;
            }
            values[static_cast<size_t>(j) * static_cast<size_t>(width) + static_cast<size_t>(i)] = value;
        }
    }

    return true;
}

bool ChargeDensityRenderer::captureRenderedSliceImage(std::vector<uint8_t>& pixels,
                                                      int& width,
                                                      int& height) {
    pixels.clear();
    width = 0;
    height = 0;

    if (!m_imageData) {
        return false;
    }

    if (!m_sliceActor) {
        updateSlice();
        if (!m_sliceActor) {
            return false;
        }
    }

    const int originalVisibility = m_sliceActor->GetVisibility();
    if (originalVisibility == 0) {
        m_sliceActor->SetVisibility(1);
    }

    double fractionalNormal[3] = {0.0, 0.0, 0.0};
    switch (m_slicePlane) {
        case SlicePlane::XY:
            fractionalNormal[2] = 1.0;
            break;
        case SlicePlane::XZ:
            fractionalNormal[1] = 1.0;
            break;
        case SlicePlane::YZ:
            fractionalNormal[0] = 1.0;
            break;
        case SlicePlane::Miller:
            // Requested behavior: view direction follows Miller normal [h, k, l].
            fractionalNormal[0] = static_cast<double>(m_sliceMillerH);
            fractionalNormal[1] = static_cast<double>(m_sliceMillerK);
            fractionalNormal[2] = static_cast<double>(m_sliceMillerL);
            break;
    }

    double worldNormal[3] = {0.0, 0.0, 1.0};
    double worldUpHint[3] = {0.0, 1.0, 0.0};
    bool hasCaptureViewDirection = extractSliceActorNormal(m_sliceActor.GetPointer(), worldNormal);
    if (!hasCaptureViewDirection && normalizeArray3(fractionalNormal)) {
        transformFractionalDirectionToWorld(m_lattice, fractionalNormal, worldNormal);
        hasCaptureViewDirection = normalizeArray3(worldNormal);
    }

    if (hasCaptureViewDirection && normalizeArray3(fractionalNormal)) {
        double desiredWorldNormal[3] = {0.0, 0.0, 0.0};
        transformFractionalDirectionToWorld(m_lattice, fractionalNormal, desiredWorldNormal);
        if (normalizeArray3(desiredWorldNormal)) {
            const double dotSign = desiredWorldNormal[0] * worldNormal[0] +
                                   desiredWorldNormal[1] * worldNormal[1] +
                                   desiredWorldNormal[2] * worldNormal[2];
            if (dotSign < 0.0) {
                worldNormal[0] *= -1.0;
                worldNormal[1] *= -1.0;
                worldNormal[2] *= -1.0;
            }
        }
    }

    if (hasCaptureViewDirection) {
        double fractionalUp[3] = {0.0, 0.0, 1.0};
        if (normalizeArray3(fractionalNormal)) {
            const double dotNormalUp = std::abs(
                fractionalUp[0] * fractionalNormal[0] +
                fractionalUp[1] * fractionalNormal[1] +
                fractionalUp[2] * fractionalNormal[2]);
            if (dotNormalUp > 0.95) {
                fractionalUp[0] = 0.0;
                fractionalUp[1] = 1.0;
                fractionalUp[2] = 0.0;
            }
        }
        transformFractionalDirectionToWorld(m_lattice, fractionalUp, worldUpHint);
        if (!normalizeArray3(worldUpHint)) {
            worldUpHint[0] = 0.0;
            worldUpHint[1] = 1.0;
            worldUpHint[2] = 0.0;
        }
    }

    double rangeMin = static_cast<double>(m_minValue);
    double rangeMax = static_cast<double>(m_maxValue);
    if (rangeMax <= rangeMin) {
        rangeMax = rangeMin + 1.0;
    }
    const double midValue = (rangeMin + rangeMax) * 0.5;
    vtkSmartPointer<vtkColorTransferFunction> colorTransfer =
        vtkSmartPointer<vtkColorTransferFunction>::New();
    common::ApplyColorMapToTransferFunction(
        colorTransfer,
        m_colorMap,
        rangeMin,
        midValue,
        rangeMax,
        static_cast<double>(m_colorCurveMidpoint),
        static_cast<double>(m_colorCurveSharpness));
    double backgroundColor[3] = {0.0, 0.0, 0.0};
    colorTransfer->GetColor(rangeMin, backgroundColor);

    const bool captured = VtkViewer::Instance().CaptureActorImage(
        m_sliceActor.GetPointer(),
        pixels,
        width,
        height,
        true,
        hasCaptureViewDirection ? worldNormal : nullptr,
        hasCaptureViewDirection ? worldUpHint : nullptr,
        backgroundColor);

    if (m_sliceActor) {
        m_sliceActor->SetVisibility(originalVisibility);
    }
    return captured;
}

void ChargeDensityRenderer::updateSlice() {
    if (!m_imageData) {
        SPDLOG_WARN("No image data for slice");
        return;
    }
    
    // 기존 액터 제거
    if (m_sliceActor) {
        VtkViewer::Instance().RemoveActor(m_sliceActor);
    }
    
    // 슬라이스 위치 계산
    double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    m_imageData->GetBounds(bounds);
    double origin[3] = {0, 0, 0};
    double normal[3] = {0, 0, 0};
    
    switch (m_slicePlane) {
        case SlicePlane::XY:
            normal[2] = 1.0;
            origin[2] = bounds[4] + m_slicePosition * (bounds[5] - bounds[4]);
            break;
        case SlicePlane::XZ:
            normal[1] = 1.0;
            origin[1] = bounds[2] + m_slicePosition * (bounds[3] - bounds[2]);
            break;
        case SlicePlane::YZ:
            normal[0] = 1.0;
            origin[0] = bounds[0] + m_slicePosition * (bounds[1] - bounds[0]);
            break;
        case SlicePlane::Miller:
            {
                double planeOffset = 0.0;
                if (!resolveMillerPlaneInDataCoords(normal, planeOffset)) {
                    SPDLOG_WARN("Invalid Miller indices for slice plane: ({}, {}, {})",
                                m_sliceMillerH, m_sliceMillerK, m_sliceMillerL);
                    m_sliceActor = nullptr;
                    return;
                }
                const Vec3 nVec{normal[0], normal[1], normal[2]};
                const double nNorm2 = dot(nVec, nVec);
                if (nNorm2 <= kSliceEpsilon) {
                    SPDLOG_WARN("Degenerate Miller normal");
                    m_sliceActor = nullptr;
                    return;
                }
                const double scale = planeOffset / nNorm2;
                origin[0] = normal[0] * scale;
                origin[1] = normal[1] * scale;
                origin[2] = normal[2] * scale;
            }
            break;
    }
    
    // 평면 생성
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(origin);
    plane->SetNormal(normal);
    
    // Cutter로 슬라이스 추출
    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    cutter->SetInputData(m_imageData);
    cutter->SetCutFunction(plane);
    cutter->Update();
    
    double rangeMin = static_cast<double>(m_minValue);
    double rangeMax = static_cast<double>(m_maxValue);
    if (rangeMax <= rangeMin) {
        rangeMax = rangeMin + 1.0;
    }
    const double midValue = (rangeMin + rangeMax) * 0.5;

    vtkSmartPointer<vtkColorTransferFunction> colorTransfer =
        vtkSmartPointer<vtkColorTransferFunction>::New();
    common::ApplyColorMapToTransferFunction(
        colorTransfer,
        m_colorMap,
        rangeMin,
        midValue,
        rangeMax,
        static_cast<double>(m_colorCurveMidpoint),
        static_cast<double>(m_colorCurveSharpness));
    
    // Mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Identity();
    double matrix[16] = {
        m_lattice[0][0], m_lattice[1][0], m_lattice[2][0], 0,
        m_lattice[0][1], m_lattice[1][1], m_lattice[2][1], 0,
        m_lattice[0][2], m_lattice[1][2], m_lattice[2][2], 0,
        0, 0, 0, 1
    };
    transform->SetMatrix(matrix);

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
        vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(cutter->GetOutputPort());
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    mapper->SetInputConnection(transformFilter->GetOutputPort());
    mapper->SetLookupTable(colorTransfer);
    mapper->SetScalarRange(rangeMin, rangeMax);
    mapper->SetColorModeToMapScalars();
    mapper->ScalarVisibilityOn();
    
    // Actor
    m_sliceActor = vtkSmartPointer<vtkActor>::New();
    m_sliceActor->SetMapper(mapper);
    m_sliceActor->SetVisibility(m_sliceVisible ? 1 : 0);
    
    VtkViewer::Instance().AddActor(m_sliceActor, false);
    
    if (m_slicePlane == SlicePlane::Miller) {
        SPDLOG_DEBUG("Slice updated: plane=Miller, hkl=({},{},{}), position={:.2f}",
                     m_sliceMillerH, m_sliceMillerK, m_sliceMillerL, m_slicePosition);
    } else {
        SPDLOG_DEBUG("Slice updated: plane={}, position={:.2f}",
                     static_cast<int>(m_slicePlane), m_slicePosition);
    }
}

void ChargeDensityRenderer::setColorMap(ColorMap colorMap) {
    m_colorMap = colorMap;
    if (m_sliceVisible) {
        updateSlice();
    }
}

void ChargeDensityRenderer::setValueRange(float min, float max) {
    m_minValue = min;
    m_maxValue = max;
    if (m_sliceVisible) {
        updateSlice();
    }
}

void ChargeDensityRenderer::setColorCurve(float midpoint, float sharpness) {
    const float clampedMidpoint = std::clamp(midpoint, 0.0f, 1.0f);
    const float clampedSharpness = std::clamp(sharpness, 0.0f, 1.0f);
    if (std::abs(clampedMidpoint - m_colorCurveMidpoint) < 1e-6f &&
        std::abs(clampedSharpness - m_colorCurveSharpness) < 1e-6f) {
        return;
    }

    m_colorCurveMidpoint = clampedMidpoint;
    m_colorCurveSharpness = clampedSharpness;
    if (m_sliceVisible) {
        updateSlice();
    }
}

bool ChargeDensityRenderer::loadFromParseResult(
    const ChgcarParser::ParseResult& result) 
{
    if (!result.success || result.density.empty()) {
        SPDLOG_ERROR("Invalid parse result for charge density");
        return false;
    }
    
    // 격자 정보 저장
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m_lattice[i][j] = result.lattice[i][j];
        }
    }
    
    m_gridShape = result.gridShape;
    m_minValue = result.minValue;
    m_maxValue = result.maxValue;
    
    // VTK 이미지 데이터 생성
    createVTKImageData(result.density);
    
    SPDLOG_INFO("Charge density loaded from result: {}x{}x{}, range=[{:.4e}, {:.4e}]",
                m_gridShape[0], m_gridShape[1], m_gridShape[2],
                m_minValue, m_maxValue);
    
    return true;
}

} // namespace infrastructure
} // namespace atoms
