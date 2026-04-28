// atoms/domain/charge_density.h
#pragma once

#include <vector>
#include <array>
#include <string>
#include <memory>
#include <cmath>

namespace atoms {
namespace domain {

/**
 * @brief 3D 전하 밀도 데이터를 저장하는 클래스
 * 
 * pyrho의 ChargeDensity 클래스를 C++로 포팅
 * 참고: https://github.com/materialsproject/pyrho
 */
class ChargeDensity {
public:
    // ========================================================================
    // 생성자
    // ========================================================================
    ChargeDensity() = default;
    
    ChargeDensity(
        const std::vector<float>& data,
        const std::array<int, 3>& gridShape,
        const float lattice[3][3]
    );
    
    // ========================================================================
    // 파일 I/O
    // ========================================================================
    
    /**
     * @brief CHGCAR 파일에서 로드 (VASP 포맷)
     */
    static std::unique_ptr<ChargeDensity> fromFile(const std::string& filePath);
    
    /**
     * @brief CHGCAR 파일로 저장
     */
    bool toFile(const std::string& filePath) const;
    
    // ========================================================================
    // 데이터 접근
    // ========================================================================
    
    /// 그리드 크기 반환 [nx, ny, nz]
    const std::array<int, 3>& gridShape() const { return m_gridShape; }
    
    /// 격자 행렬 반환
    void getLattice(float lattice[3][3]) const;
    
    /// 원시 데이터 접근
    const std::vector<float>& data() const { return m_data; }
    std::vector<float>& data() { return m_data; }
    
    /// 특정 그리드 포인트의 값
    float getValue(int ix, int iy, int iz) const;
    
    /// 선형 보간된 값 (분수 좌표)
    float getInterpolatedValue(float fx, float fy, float fz) const;
    
    /// 최소/최대값
    float minValue() const { return m_minValue; }
    float maxValue() const { return m_maxValue; }
    
    /// 총 전하량
    float totalCharge() const;
    
    // ========================================================================
    // 변환 (pyrho 핵심 기능)
    // ========================================================================
    
    /**
     * @brief 격자 변환 (supercell, rotation 등)
     * 
     * pyrho의 get_transformed() 메서드 포팅
     * 
     * @param scMatrix 3x3 변환 행렬 (정수)
     * @param gridOut 출력 그리드 크기 (nullptr이면 자동 계산)
     * @param upSample 업샘플링 팩터
     * @return 변환된 ChargeDensity
     */
    std::unique_ptr<ChargeDensity> getTransformed(
        const int scMatrix[3][3],
        const std::array<int, 3>* gridOut = nullptr,
        int upSample = 1
    ) const;
    
    /**
     * @brief 특정 평면의 2D 슬라이스 추출
     * 
     * @param plane 0=XY, 1=XZ, 2=YZ
     * @param position 0.0~1.0 (분수 좌표)
     * @return 2D 데이터와 크기
     */
    struct Slice2D {
        std::vector<float> data;
        int width;
        int height;
        float minVal;
        float maxVal;
    };
    Slice2D getSlice(int plane, float position) const;
    
    /**
     * @brief 임의의 평면으로 슬라이스
     * 
     * @param normal 평면 법선 벡터
     * @param origin 평면 원점 (분수 좌표)
     * @param resolution 출력 해상도
     */
    Slice2D getCustomSlice(
        const float normal[3],
        const float origin[3],
        int resolution = 256
    ) const;
    
    // ========================================================================
    // 유틸리티
    // ========================================================================
    
    /// 통계 정보 재계산
    void updateStatistics();
    
    /// 데이터 정규화 (0~1)
    void normalize();
    
    /// 가우시안 스무딩
    void smooth(float sigma = 1.0f);

private:
    std::vector<float> m_data;          // 3D 밀도 데이터 (row-major: z가 가장 빠름)
    std::array<int, 3> m_gridShape;     // [nx, ny, nz]
    float m_lattice[3][3];              // 격자 벡터
    float m_invLattice[3][3];           // 역격자
    
    float m_minValue = 0.0f;
    float m_maxValue = 0.0f;
    float m_totalCharge = 0.0f;
    
    // 내부 헬퍼
    void computeInverseLattice();
    int flatIndex(int ix, int iy, int iz) const;
    
    // 3D FFT 기반 보간 (pyrho 방식)
    void regridFFT(
        const std::vector<float>& input,
        const std::array<int, 3>& inShape,
        std::vector<float>& output,
        const std::array<int, 3>& outShape
    ) const;
};

} // namespace domain
} // namespace atoms
