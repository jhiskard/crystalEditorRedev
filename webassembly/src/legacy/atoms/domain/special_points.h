// atoms/domain/special_points.h
#pragma once

#include "../../config/log_config.h"

#include <map>
#include <string>
#include <array>
#include <vector>

namespace atoms {
namespace domain {

/**
 * @brief 특수점 데이터 (분수 좌표)
 */
using SpecialPoint = std::array<double, 3>;

/**
 * @brief 격자 타입별 특수점 맵
 */
using SpecialPointsMap = std::map<std::string, SpecialPoint>;

/**
 * @brief 격자 정보 및 특수점 데이터베이스
 * 
 * Python의 get_cellinfo() 및 special_points에 해당
 */
class SpecialPointsDatabase {
public:
    /**
     * @brief 격자 타입별 특수점 맵 초기화
     */
    static SpecialPointsMap getCubicPoints() {
        return {
            {"G", {0.0, 0.0, 0.0}},
            {"M", {0.5, 0.5, 0.0}},
            {"R", {0.5, 0.5, 0.5}},
            {"X", {0.0, 0.5, 0.0}}
        };
    }
    
    static SpecialPointsMap getFCCPoints() {
        return {
            {"G", {0.0, 0.0, 0.0}},
            {"K", {3.0/8.0, 3.0/8.0, 3.0/4.0}},
            {"L", {0.5, 0.5, 0.5}},
            {"U", {5.0/8.0, 1.0/4.0, 5.0/8.0}},
            {"W", {0.5, 1.0/4.0, 3.0/4.0}},
            {"X", {0.5, 0.0, 0.5}}
        };
    }
    
    static SpecialPointsMap getBCCPoints() {
        return {
            {"G", {0.0, 0.0, 0.0}},
            {"H", {0.5, -0.5, 0.5}},
            {"P", {0.25, 0.25, 0.25}},
            {"N", {0.0, 0.0, 0.5}}
        };
    }
    
    static SpecialPointsMap getTetragonalPoints() {
        return {
            {"G", {0.0, 0.0, 0.0}},
            {"A", {0.5, 0.5, 0.5}},
            {"M", {0.5, 0.5, 0.0}},
            {"R", {0.0, 0.5, 0.5}},
            {"X", {0.0, 0.5, 0.0}},
            {"Z", {0.0, 0.0, 0.5}}
        };
    }
    
    static SpecialPointsMap getOrthorhombicPoints() {
        return {
            {"G", {0.0, 0.0, 0.0}},
            {"R", {0.5, 0.5, 0.5}},
            {"S", {0.5, 0.5, 0.0}},
            {"T", {0.0, 0.5, 0.5}},
            {"U", {0.5, 0.0, 0.5}},
            {"X", {0.5, 0.0, 0.0}},
            {"Y", {0.0, 0.5, 0.0}},
            {"Z", {0.0, 0.0, 0.5}}
        };
    }
    
    static SpecialPointsMap getHexagonalPoints() {
        return {
            {"G", {0.0, 0.0, 0.0}},
            {"A", {0.0, 0.0, 0.5}},
            {"H", {1.0/3.0, 1.0/3.0, 0.5}},
            {"K", {1.0/3.0, 1.0/3.0, 0.0}},
            {"L", {0.5, 0.0, 0.5}},
            {"M", {0.5, 0.0, 0.0}}
        };
    }
    
    /**
     * @brief 기본 경로 문자열
     */
    static std::string getDefaultPath(const std::string& latticeType) {
        static const std::map<std::string, std::string> defaultPaths = {
            {"cubic", "GXMGRX,MR"},
            {"fcc", "GXWKGLUWLK,UX"},
            {"bcc", "GHNGPH,PN"},
            {"tetragonal", "GXMGZRAZXR,MA"},
            {"orthorhombic", "GXSYGZURTZ,YT,UX,SR"},
            {"hexagonal", "GMKGALHA,LM,KH"}
        };
        
        auto it = defaultPaths.find(latticeType);
        if (it != defaultPaths.end()) {
            return it->second;
        }
        
        return "GXMGRX";  // 기본값 (cubic)
    }
    
    /**
     * @brief 격자 타입별 특수점 맵 가져오기
     */
    static SpecialPointsMap getSpecialPoints(const std::string& latticeType) {
        if (latticeType == "cubic") return getCubicPoints();
        if (latticeType == "fcc") return getFCCPoints();
        if (latticeType == "bcc") return getBCCPoints();
        if (latticeType == "tetragonal") return getTetragonalPoints();
        if (latticeType == "orthorhombic") return getOrthorhombicPoints();
        if (latticeType == "hexagonal") return getHexagonalPoints();
        
        // 기본값 (cubic)
        return getCubicPoints();
    }

    /**
     * @brief Special points를 SPDLOG로 출력
     */
    // static void printSpecialPointsTable(const std::map<std::string, std::array<double, 3>>& specialPoints) {
    //     SPDLOG_INFO("========================================");
    //     SPDLOG_INFO("Special Points Table");
    //     SPDLOG_INFO("========================================");
    //     SPDLOG_INFO(" Label |      a      |      b      |      c      ");
    //     SPDLOG_INFO("-------|-------------|-------------|-------------");
        
    //     for (const auto& [label, coords] : specialPoints) {
    //         SPDLOG_INFO("  {:<4} | {:>11.6f} | {:>11.6f} | {:>11.6f}", 
    //                     label, coords[0], coords[1], coords[2]);
    //     }
        
    //     SPDLOG_INFO("========================================");
    // }

    /**
     * @brief 분수 좌표를 카르테시안 좌표로 변환
     * 
     * Python: skpts_kc → ckpts_kv
     * ckpts = np.dot(skpts, icell.T) * 2π
     * 
     * @param fractional 분수 좌표 (특수점 정의)
     * @param icell 역격자 행렬 (3x3)
     * @return 카르테시안 좌표
     */
    static std::array<double, 3> fractionalToCartesian(
        const SpecialPoint& fractional,
        const double icell[3][3])
    {
        std::array<double, 3> cartesian = {0.0, 0.0, 0.0};
        
        // ckpts = np.dot(skpts, icell.T) * 2π
        // 하지만 icell은 이미 2π * inv(cell).T이므로
        // 실제로는 np.dot(skpts, icell.T)만 필요
        
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                cartesian[i] += fractional[j] * icell[j][i];
            }
        }
        
        return cartesian;
    }

    // ========================================================================
    // ⭐ 격자 타입 자동 감지 (신규 추가)
    // ========================================================================
    
    /**
     * @brief 격자 행렬로부터 격자 타입 자동 감지 (float 버전)
     * 
     * 격자 벡터의 길이와 각도를 분석하여 격자 타입을 결정합니다.
     * 
     * @param matrix 격자 벡터 3x3 행렬
     *               matrix[0] = a1, matrix[1] = a2, matrix[2] = a3
     * @return 격자 타입 문자열 ("cubic", "fcc", "bcc", "tetragonal", etc.)
     */
    static std::string detectLatticeType(const float matrix[3][3]) {
        // float → double 변환하여 double 버전 호출
        double matrixDouble[3][3];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                matrixDouble[i][j] = static_cast<double>(matrix[i][j]);
            }
        }
        return detectLatticeType(matrixDouble);
    }
    
    /**
     * @brief 격자 행렬로부터 격자 타입 자동 감지 (double 버전)
     * 
     * @param matrix 격자 벡터 3x3 행렬
     * @return 격자 타입 문자열
     */
    static std::string detectLatticeType(const double matrix[3][3]) {
        // 1. 격자 벡터 길이 계산
        double a = std::sqrt(
            matrix[0][0] * matrix[0][0] +
            matrix[0][1] * matrix[0][1] +
            matrix[0][2] * matrix[0][2]
        );
        
        double b = std::sqrt(
            matrix[1][0] * matrix[1][0] +
            matrix[1][1] * matrix[1][1] +
            matrix[1][2] * matrix[1][2]
        );
        
        double c = std::sqrt(
            matrix[2][0] * matrix[2][0] +
            matrix[2][1] * matrix[2][1] +
            matrix[2][2] * matrix[2][2]
        );
        
        // 2. 격자 벡터 간 각도 계산 (내적 이용)
        auto dotProduct = [&](int i, int j) -> double {
            return matrix[i][0] * matrix[j][0] +
                matrix[i][1] * matrix[j][1] +
                matrix[i][2] * matrix[j][2];
        };
        
        double cos_alpha = dotProduct(1, 2) / (b * c);  // a2 · a3
        double cos_beta  = dotProduct(0, 2) / (a * c);  // a1 · a3
        double cos_gamma = dotProduct(0, 1) / (a * b);  // a1 · a2
        
        // 라디안으로 변환
        double alpha = std::acos(std::max(-1.0, std::min(1.0, cos_alpha))) * 180.0 / M_PI;
        double beta  = std::acos(std::max(-1.0, std::min(1.0, cos_beta)))  * 180.0 / M_PI;
        double gamma = std::acos(std::max(-1.0, std::min(1.0, cos_gamma))) * 180.0 / M_PI;
        
        // ⭐ 로깅 추가 - 격자 파라미터 출력
        SPDLOG_INFO("========================================");
        SPDLOG_INFO("Lattice Type Detection - Cell Parameters");
        SPDLOG_INFO("========================================");
        SPDLOG_INFO("Lattice vectors:");
        SPDLOG_INFO("  a1 = ({:.6f}, {:.6f}, {:.6f})", matrix[0][0], matrix[0][1], matrix[0][2]);
        SPDLOG_INFO("  a2 = ({:.6f}, {:.6f}, {:.6f})", matrix[1][0], matrix[1][1], matrix[1][2]);
        SPDLOG_INFO("  a3 = ({:.6f}, {:.6f}, {:.6f})", matrix[2][0], matrix[2][1], matrix[2][2]);
        SPDLOG_INFO("");
        SPDLOG_INFO("Cell parameters:");
        SPDLOG_INFO("  Lengths: a = {:.6f}, b = {:.6f}, c = {:.6f}", a, b, c);
        SPDLOG_INFO("  Angles:  α = {:.3f}°, β = {:.3f}°, γ = {:.3f}°", alpha, beta, gamma);
        SPDLOG_INFO("");
        
        // 3. 허용 오차 설정
        const double LENGTH_TOL = 0.01;  // 길이 비교 허용 오차 (1%)
        const double ANGLE_TOL = 1.0;    // 각도 비교 허용 오차 (1도)
        
        // 4. 격자 타입 판별
        
        // Cubic: a = b = c, α = β = γ = 90°
        if (isLengthEqual(a, b, LENGTH_TOL) && 
            isLengthEqual(b, c, LENGTH_TOL) &&
            isAngleEqual(alpha, 90.0, ANGLE_TOL) &&
            isAngleEqual(beta, 90.0, ANGLE_TOL) &&
            isAngleEqual(gamma, 90.0, ANGLE_TOL)) {
            
            SPDLOG_INFO("Detected lattice type: CUBIC (simple cubic)");
            SPDLOG_INFO("  Criteria: a ≈ b ≈ c, α = β = γ = 90°");
            SPDLOG_INFO("========================================");
            return "cubic";
        }
        
        // ⭐ FCC 판별 추가
        // FCC는 conventional cell 기준으로 cubic이지만, 
        // primitive cell 기준으로는 다음과 같은 특징을 가짐:
        // - a = b = c (모든 벡터 길이 동일)
        // - α = β = γ = 60° (모든 각도 60도)
        if (isLengthEqual(a, b, LENGTH_TOL) && 
            isLengthEqual(b, c, LENGTH_TOL) &&
            isAngleEqual(alpha, 60.0, ANGLE_TOL) &&
            isAngleEqual(beta, 60.0, ANGLE_TOL) &&
            isAngleEqual(gamma, 60.0, ANGLE_TOL)) {
            
            SPDLOG_INFO("Detected lattice type: FCC (face-centered cubic, primitive cell)");
            SPDLOG_INFO("  Criteria: a ≈ b ≈ c, α = β = γ = 60°");
            SPDLOG_INFO("========================================");
            return "fcc";
        }
        
        // ⭐ BCC 판별 추가
        // BCC primitive cell의 특징:
        // - a = b = c (모든 벡터 길이 동일)
        // - α = β = γ ≈ 109.47° (cos^-1(-1/3))
        const double BCC_ANGLE = std::acos(-1.0/3.0) * 180.0 / M_PI;  // ≈ 109.47°
        const double BCC_ANGLE_TOL = 2.0;  // BCC는 조금 더 넓은 허용 오차
        
        if (isLengthEqual(a, b, LENGTH_TOL) && 
            isLengthEqual(b, c, LENGTH_TOL) &&
            isAngleEqual(alpha, BCC_ANGLE, BCC_ANGLE_TOL) &&
            isAngleEqual(beta, BCC_ANGLE, BCC_ANGLE_TOL) &&
            isAngleEqual(gamma, BCC_ANGLE, BCC_ANGLE_TOL)) {
            
            SPDLOG_INFO("Detected lattice type: BCC (body-centered cubic, primitive cell)");
            SPDLOG_INFO("  Criteria: a ≈ b ≈ c, α = β = γ ≈ {:.3f}°", BCC_ANGLE);
            SPDLOG_INFO("========================================");
            return "bcc";
        }
        
        // Tetragonal: a = b ≠ c, α = β = γ = 90°
        if (isLengthEqual(a, b, LENGTH_TOL) && 
            !isLengthEqual(a, c, LENGTH_TOL) &&
            isAngleEqual(alpha, 90.0, ANGLE_TOL) &&
            isAngleEqual(beta, 90.0, ANGLE_TOL) &&
            isAngleEqual(gamma, 90.0, ANGLE_TOL)) {
            
            SPDLOG_INFO("Detected lattice type: TETRAGONAL");
            SPDLOG_INFO("  Criteria: a ≈ b ≠ c, α = β = γ = 90°");
            SPDLOG_INFO("========================================");
            return "tetragonal";
        }
        
        // Orthorhombic: a ≠ b ≠ c, α = β = γ = 90°
        if (!isLengthEqual(a, b, LENGTH_TOL) && 
            !isLengthEqual(b, c, LENGTH_TOL) && 
            !isLengthEqual(a, c, LENGTH_TOL) &&
            isAngleEqual(alpha, 90.0, ANGLE_TOL) &&
            isAngleEqual(beta, 90.0, ANGLE_TOL) &&
            isAngleEqual(gamma, 90.0, ANGLE_TOL)) {
            
            SPDLOG_INFO("Detected lattice type: ORTHORHOMBIC");
            SPDLOG_INFO("  Criteria: a ≠ b ≠ c, α = β = γ = 90°");
            SPDLOG_INFO("========================================");
            return "orthorhombic";
        }
        
        // Hexagonal: a = b ≠ c, α = β = 90°, γ = 120°
        if (isLengthEqual(a, b, LENGTH_TOL) && 
            !isLengthEqual(a, c, LENGTH_TOL) &&
            isAngleEqual(alpha, 90.0, ANGLE_TOL) &&
            isAngleEqual(beta, 90.0, ANGLE_TOL) &&
            isAngleEqual(gamma, 120.0, ANGLE_TOL)) {
            
            SPDLOG_INFO("Detected lattice type: HEXAGONAL");
            SPDLOG_INFO("  Criteria: a ≈ b ≠ c, α = β = 90°, γ = 120°");
            SPDLOG_INFO("========================================");
            return "hexagonal";
        }
        
        // ⭐ 기본값: cubic (로깅 추가)
        SPDLOG_WARN("Could not match any standard lattice type");
        SPDLOG_WARN("  Defaulting to: CUBIC");
        SPDLOG_INFO("========================================");
        return "cubic";
    }

private:
    /**
     * @brief 두 길이가 허용 오차 내에서 같은지 비교
     */
    static bool isLengthEqual(double val1, double val2, double tolerance) {
        return std::abs(val1 - val2) < tolerance * std::max(val1, val2);
    }
    
    /**
     * @brief 두 각도가 허용 오차 내에서 같은지 비교
     */
    static bool isAngleEqual(double angle1, double angle2, double tolerance) {
        return std::abs(angle1 - angle2) < tolerance;
    }
};

/**
 * @brief Path 문자열 파싱
 * 
 * Python의 parse_path_string() 함수
 * 
 * 예:
 * - "GX" → [["G", "X"]]
 * - "GXMGRX" → [["G", "X", "M", "G", "R", "X"]]
 * - "GXMGRX,MR" → [["G", "X", "M", "G", "R", "X"], ["M", "R"]]
 */
class PathParser {
public:
    /**
     * @brief Path 문자열을 세그먼트로 분리
     * 
     * @param pathString 입력 문자열 (예: "GXMGRX,MR")
     * @return 세그먼트 리스트 (예: [["G","X","M","G","R","X"], ["M","R"]])
     */
    static std::vector<std::vector<std::string>> parse(const std::string& pathString) {
        std::vector<std::vector<std::string>> segments;
        
        if (pathString.empty()) {
            return segments;
        }
        
        // 쉼표로 분리 (branch)
        std::vector<std::string> branches = splitByComma(pathString);
        
        for (const auto& branch : branches) {
            std::vector<std::string> labels = parseLabels(branch);
            if (!labels.empty()) {
                segments.push_back(labels);
            }
        }
        
        return segments;
    }
    
private:
    /**
     * @brief 쉼표로 문자열 분리
     */
    static std::vector<std::string> splitByComma(const std::string& str) {
        std::vector<std::string> result;
        std::string current;
        
        for (char c : str) {
            if (c == ',') {
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        
        if (!current.empty()) {
            result.push_back(current);
        }
        
        return result;
    }
    
    /**
     * @brief 라벨 문자열 파싱 (대문자로 시작하는 토큰)
     * 
     * "GXMGRX" → ["G", "X", "M", "G", "R", "X"]
     * "M1A" → ["M1", "A"]
     */
    static std::vector<std::string> parseLabels(const std::string& str) {
        std::vector<std::string> labels;
        std::string current;
        
        for (size_t i = 0; i < str.length(); i++) {
            char c = str[i];
            
            // 대문자로 시작하는 새로운 라벨
            if (std::isupper(c)) {
                if (!current.empty()) {
                    labels.push_back(current);
                    current.clear();
                }
                current += c;
            }
            // 숫자 또는 소문자 (라벨의 일부)
            else if (std::isdigit(c) || std::islower(c)) {
                current += c;
            }
        }
        
        if (!current.empty()) {
            labels.push_back(current);
        }
        
        return labels;
    }
};

/**
 * @brief K-points 보간
 * 
 * Python의 bandpath() 함수 일부
 */
class KpointsInterpolator {
public:
    /**
     * @brief 두 점 사이를 선형 보간
     * 
     * @param start 시작점
     * @param end 끝점
     * @param numPoints 생성할 점 개수 (start, end 포함)
     * @return 보간된 점들
     */
    static std::vector<std::array<double, 3>> interpolate(
        const std::array<double, 3>& start,
        const std::array<double, 3>& end,
        int numPoints)
    {
        std::vector<std::array<double, 3>> points;
        
        if (numPoints < 2) {
            numPoints = 2;
        }
        
        for (int i = 0; i < numPoints; i++) {
            double t = static_cast<double>(i) / (numPoints - 1);
            
            std::array<double, 3> point;
            for (int j = 0; j < 3; j++) {
                point[j] = start[j] + t * (end[j] - start[j]);
            }
            
            points.push_back(point);
        }
        
        return points;
    }
    
    /**
     * @brief Path 세그먼트를 따라 k-points 생성
     * 
     * @param pathSegments Path 문자열 파싱 결과
     * @param specialPoints 특수점 맵 (카르테시안 좌표)
     * @param totalPoints 전체 점 개수
     * @return k-points 배열
     */
    static std::vector<std::array<double, 3>> generateKpoints(
        const std::vector<std::vector<std::string>>& pathSegments,
        const std::map<std::string, std::array<double, 3>>& specialPoints,
        int totalPoints)
    {
        std::vector<std::array<double, 3>> allKpoints;
        
        // 각 세그먼트의 총 거리 계산
        std::vector<double> segmentLengths;
        double totalLength = 0.0;
        
        for (const auto& segment : pathSegments) {
            for (size_t i = 0; i < segment.size() - 1; i++) {
                const std::string& label1 = segment[i];
                const std::string& label2 = segment[i + 1];
                
                auto it1 = specialPoints.find(label1);
                auto it2 = specialPoints.find(label2);
                
                if (it1 != specialPoints.end() && it2 != specialPoints.end()) {
                    double length = distance(it1->second, it2->second);
                    segmentLengths.push_back(length);
                    totalLength += length;
                }
            }
        }
        
        if (totalLength < 1e-10) {
            return allKpoints;
        }
        
        // 각 세그먼트에 점 할당
        int remainingPoints = totalPoints;
        size_t segmentIdx = 0;
        
        for (const auto& segment : pathSegments) {
            for (size_t i = 0; i < segment.size() - 1; i++) {
                const std::string& label1 = segment[i];
                const std::string& label2 = segment[i + 1];
                
                auto it1 = specialPoints.find(label1);
                auto it2 = specialPoints.find(label2);
                
                if (it1 == specialPoints.end() || it2 == specialPoints.end()) {
                    continue;
                }
                
                // 이 세그먼트의 점 개수 계산
                double ratio = segmentLengths[segmentIdx] / totalLength;
                int numPoints = std::max(2, static_cast<int>(ratio * totalPoints));
                
                // 마지막 세그먼트면 남은 점 모두 사용
                if (segmentIdx == segmentLengths.size() - 1) {
                    numPoints = remainingPoints;
                }
                
                // 보간
                auto segmentPoints = interpolate(it1->second, it2->second, numPoints);
                
                // 중복 제거 (끝점 제외)
                if (!allKpoints.empty() && segmentIdx > 0) {
                    segmentPoints.erase(segmentPoints.begin());
                }
                
                allKpoints.insert(allKpoints.end(), segmentPoints.begin(), segmentPoints.end());
                
                remainingPoints -= (numPoints - 1);
                segmentIdx++;
            }
        }
        
        return allKpoints;
    }
    
private:
    /**
     * @brief 두 점 사이의 유클리드 거리
     */
    static double distance(
        const std::array<double, 3>& p1,
        const std::array<double, 3>& p2)
    {
        double dx = p2[0] - p1[0];
        double dy = p2[1] - p1[1];
        double dz = p2[2] - p1[2];
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

} // namespace domain
} // namespace atoms