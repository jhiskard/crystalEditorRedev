// atoms/domain/charge_density.cpp
#include "charge_density.h"
#include "../infrastructure/chgcar_parser.h"
#include "../../config/log_config.h"

#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>

namespace atoms {
namespace domain {

ChargeDensity::ChargeDensity(
    const std::vector<float>& data,
    const std::array<int, 3>& gridShape,
    const float lattice[3][3]
) : m_data(data), m_gridShape(gridShape) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m_lattice[i][j] = lattice[i][j];
        }
    }
    computeInverseLattice();
    updateStatistics();
}

std::unique_ptr<ChargeDensity> ChargeDensity::fromFile(const std::string& filePath) {
    auto result = infrastructure::ChgcarParser::parse(filePath);
    
    if (!result.success) {
        SPDLOG_ERROR("Failed to parse CHGCAR: {}", result.errorMessage);
        return nullptr;
    }
    
    auto cd = std::make_unique<ChargeDensity>(
        result.density,
        result.gridShape,
        result.lattice
    );
    
    return cd;
}

void ChargeDensity::computeInverseLattice() {
    // 3x3 행렬의 역행렬 계산
    float det = 
        m_lattice[0][0] * (m_lattice[1][1] * m_lattice[2][2] - m_lattice[1][2] * m_lattice[2][1]) -
        m_lattice[0][1] * (m_lattice[1][0] * m_lattice[2][2] - m_lattice[1][2] * m_lattice[2][0]) +
        m_lattice[0][2] * (m_lattice[1][0] * m_lattice[2][1] - m_lattice[1][1] * m_lattice[2][0]);
    
    if (std::abs(det) < 1e-10f) {
        SPDLOG_ERROR("Singular lattice matrix");
        return;
    }
    
    float invDet = 1.0f / det;
    
    m_invLattice[0][0] = (m_lattice[1][1] * m_lattice[2][2] - m_lattice[1][2] * m_lattice[2][1]) * invDet;
    m_invLattice[0][1] = (m_lattice[0][2] * m_lattice[2][1] - m_lattice[0][1] * m_lattice[2][2]) * invDet;
    m_invLattice[0][2] = (m_lattice[0][1] * m_lattice[1][2] - m_lattice[0][2] * m_lattice[1][1]) * invDet;
    
    m_invLattice[1][0] = (m_lattice[1][2] * m_lattice[2][0] - m_lattice[1][0] * m_lattice[2][2]) * invDet;
    m_invLattice[1][1] = (m_lattice[0][0] * m_lattice[2][2] - m_lattice[0][2] * m_lattice[2][0]) * invDet;
    m_invLattice[1][2] = (m_lattice[0][2] * m_lattice[1][0] - m_lattice[0][0] * m_lattice[1][2]) * invDet;
    
    m_invLattice[2][0] = (m_lattice[1][0] * m_lattice[2][1] - m_lattice[1][1] * m_lattice[2][0]) * invDet;
    m_invLattice[2][1] = (m_lattice[0][1] * m_lattice[2][0] - m_lattice[0][0] * m_lattice[2][1]) * invDet;
    m_invLattice[2][2] = (m_lattice[0][0] * m_lattice[1][1] - m_lattice[0][1] * m_lattice[1][0]) * invDet;
}

int ChargeDensity::flatIndex(int ix, int iy, int iz) const {
    // VASP 포맷: z가 가장 빠르게 변함 (Fortran 순서)
    // index = ix + iy * nx + iz * nx * ny (C 순서로 변환)
    // 또는 VASP 원본: iz + iy * nz + ix * ny * nz
    
    // 주기 경계 조건 적용
    ix = ((ix % m_gridShape[0]) + m_gridShape[0]) % m_gridShape[0];
    iy = ((iy % m_gridShape[1]) + m_gridShape[1]) % m_gridShape[1];
    iz = ((iz % m_gridShape[2]) + m_gridShape[2]) % m_gridShape[2];
    
    // VASP 순서 (z fastest)
    return iz + iy * m_gridShape[2] + ix * m_gridShape[1] * m_gridShape[2];
}

float ChargeDensity::getValue(int ix, int iy, int iz) const {
    return m_data[flatIndex(ix, iy, iz)];
}

float ChargeDensity::getInterpolatedValue(float fx, float fy, float fz) const {
    // 주기 경계 조건
    fx = fx - std::floor(fx);
    fy = fy - std::floor(fy);
    fz = fz - std::floor(fz);
    
    // 그리드 좌표로 변환
    float gx = fx * m_gridShape[0];
    float gy = fy * m_gridShape[1];
    float gz = fz * m_gridShape[2];
    
    int ix0 = static_cast<int>(std::floor(gx));
    int iy0 = static_cast<int>(std::floor(gy));
    int iz0 = static_cast<int>(std::floor(gz));
    
    float dx = gx - ix0;
    float dy = gy - iy0;
    float dz = gz - iz0;
    
    // 삼선형 보간 (trilinear interpolation)
    float c000 = getValue(ix0, iy0, iz0);
    float c001 = getValue(ix0, iy0, iz0 + 1);
    float c010 = getValue(ix0, iy0 + 1, iz0);
    float c011 = getValue(ix0, iy0 + 1, iz0 + 1);
    float c100 = getValue(ix0 + 1, iy0, iz0);
    float c101 = getValue(ix0 + 1, iy0, iz0 + 1);
    float c110 = getValue(ix0 + 1, iy0 + 1, iz0);
    float c111 = getValue(ix0 + 1, iy0 + 1, iz0 + 1);
    
    float c00 = c000 * (1 - dx) + c100 * dx;
    float c01 = c001 * (1 - dx) + c101 * dx;
    float c10 = c010 * (1 - dx) + c110 * dx;
    float c11 = c011 * (1 - dx) + c111 * dx;
    
    float c0 = c00 * (1 - dy) + c10 * dy;
    float c1 = c01 * (1 - dy) + c11 * dy;
    
    return c0 * (1 - dz) + c1 * dz;
}

void ChargeDensity::getLattice(float lattice[3][3]) const {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            lattice[i][j] = m_lattice[i][j];
        }
    }
}

void ChargeDensity::updateStatistics() {
    if (m_data.empty()) {
        m_minValue = m_maxValue = m_totalCharge = 0.0f;
        return;
    }
    
    auto [minIt, maxIt] = std::minmax_element(m_data.begin(), m_data.end());
    m_minValue = *minIt;
    m_maxValue = *maxIt;
    
    m_totalCharge = std::accumulate(m_data.begin(), m_data.end(), 0.0f);
    
    // 셀 부피로 정규화 (VASP 데이터는 부피당 전하)
    float volume = 
        m_lattice[0][0] * (m_lattice[1][1] * m_lattice[2][2] - m_lattice[1][2] * m_lattice[2][1]) -
        m_lattice[0][1] * (m_lattice[1][0] * m_lattice[2][2] - m_lattice[1][2] * m_lattice[2][0]) +
        m_lattice[0][2] * (m_lattice[1][0] * m_lattice[2][1] - m_lattice[1][1] * m_lattice[2][0]);
    
    int totalPoints = m_gridShape[0] * m_gridShape[1] * m_gridShape[2];
    m_totalCharge *= std::abs(volume) / totalPoints;
    
    SPDLOG_DEBUG("ChargeDensity stats: min={:.4f}, max={:.4f}, total={:.4f}", 
                 m_minValue, m_maxValue, m_totalCharge);
}

float ChargeDensity::totalCharge() const {
    return m_totalCharge;
}

ChargeDensity::Slice2D ChargeDensity::getSlice(int plane, float position) const {
    Slice2D slice;
    
    // 위치를 0~1 범위로 정규화
    position = position - std::floor(position);
    
    switch (plane) {
        case 0: // XY plane (z = const)
        {
            int iz = static_cast<int>(position * m_gridShape[2]);
            slice.width = m_gridShape[0];
            slice.height = m_gridShape[1];
            slice.data.resize(slice.width * slice.height);
            
            for (int iy = 0; iy < m_gridShape[1]; ++iy) {
                for (int ix = 0; ix < m_gridShape[0]; ++ix) {
                    slice.data[iy * slice.width + ix] = getValue(ix, iy, iz);
                }
            }
            break;
        }
        case 1: // XZ plane (y = const)
        {
            int iy = static_cast<int>(position * m_gridShape[1]);
            slice.width = m_gridShape[0];
            slice.height = m_gridShape[2];
            slice.data.resize(slice.width * slice.height);
            
            for (int iz = 0; iz < m_gridShape[2]; ++iz) {
                for (int ix = 0; ix < m_gridShape[0]; ++ix) {
                    slice.data[iz * slice.width + ix] = getValue(ix, iy, iz);
                }
            }
            break;
        }
        case 2: // YZ plane (x = const)
        {
            int ix = static_cast<int>(position * m_gridShape[0]);
            slice.width = m_gridShape[1];
            slice.height = m_gridShape[2];
            slice.data.resize(slice.width * slice.height);
            
            for (int iz = 0; iz < m_gridShape[2]; ++iz) {
                for (int iy = 0; iy < m_gridShape[1]; ++iy) {
                    slice.data[iz * slice.width + iy] = getValue(ix, iy, iz);
                }
            }
            break;
        }
        default:
            SPDLOG_ERROR("Invalid slice plane: {}", plane);
            return slice;
    }
    
    // 슬라이스의 min/max 계산
    auto [minIt, maxIt] = std::minmax_element(slice.data.begin(), slice.data.end());
    slice.minVal = *minIt;
    slice.maxVal = *maxIt;
    
    return slice;
}

void ChargeDensity::normalize() {
    if (m_maxValue <= m_minValue) return;
    
    float range = m_maxValue - m_minValue;
    for (auto& v : m_data) {
        v = (v - m_minValue) / range;
    }
    
    m_minValue = 0.0f;
    m_maxValue = 1.0f;
}

} // namespace domain
} // namespace atoms
