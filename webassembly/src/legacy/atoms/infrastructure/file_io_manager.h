// atoms/infrastructure/file_io_manager.h (신규)
#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class AtomsTemplate;

namespace atoms {
namespace infrastructure {

/**
 * @brief 파일 입출력 관리자 - XSF 파일 파싱 및 저장 기능
 * 
 * AtomsTemplate에서 파일 I/O 관련 코드를 분리하여 관리합니다.
 * 현재는 XSF 파일 형식만 지원하며, 추후 다른 형식 확장 가능합니다.
 */
class FileIOManager {
public:
    /**
     * @brief 원자 데이터 구조체
     * 
     * 파일에서 읽은 원자 정보를 임시 저장하는 구조체
     */
    struct AtomData {
        std::string symbol;
        float position[3];
        
        AtomData(const std::string& sym, float x, float y, float z) 
            : symbol(sym) {
            position[0] = x;
            position[1] = y;
            position[2] = z;
        }
    };
    
    /**
     * @brief 파싱 결과 구조체
     * 
     * 파일 파싱 후 결과를 전달하는 구조체
     */
    struct ParseResult {
        bool success;
        float cellVectors[3][3];
        std::vector<AtomData> atoms;
        std::string errorMessage;
        
        ParseResult() : success(false) {
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    cellVectors[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
        }
    };

    /**
     * @brief 3D 그리드 데이터 구조체
     */
    struct Grid3DResult {
        std::string label;
        int dims[3] { 0, 0, 0 };
        float origin[3] { 0.0f, 0.0f, 0.0f };
        float vectors[3][3] { {0.0f, 0.0f, 0.0f},
                              {0.0f, 0.0f, 0.0f},
                              {0.0f, 0.0f, 0.0f} };
        std::vector<float> values;
    };

    /**
     * @brief 품질별 그리드 데이터 묶음
     */
    struct Grid3DSet {
        Grid3DResult high;
        Grid3DResult medium;
        Grid3DResult low;
        bool hasMedium = false;
        bool hasLow = false;
    };

    /**
     * @brief 3D 그리드 파싱 결과
     */
    struct Grid3DParseResult {
        bool success = false;
        std::vector<Grid3DSet> grids;
        std::vector<AtomData> atoms;
        bool hasCellVectors = false;
        bool cellVectorsConsistent = true;
        float cellVectors[3][3] {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        };
        std::string errorMessage;
    };
    
    // ========================================================================
    // 생성자/소멸자
    // ========================================================================
    explicit FileIOManager(::AtomsTemplate* parent);
    ~FileIOManager();
    
    // ========================================================================
    // XSF 파일 처리
    // ========================================================================
    
    /**
     * @brief XSF 파일 로드 및 파싱
     * 
     * @param filePath XSF 파일 경로
     * @return 파싱 결과 (성공 여부, 셀 벡터, 원자 데이터)
     */
    ParseResult loadXSFFile(const std::string& filePath);
    Grid3DParseResult load3DGridXSFFile(const std::string& filePath);
    bool initializeStructureFromXSF(
        const std::string& filePath,
        ::AtomsTemplate* parent,
        std::string& errorMessage,
        std::vector<uint32_t>* outNewAtomIds = nullptr);
    bool initializeStructure(
        const float cellVectors[3][3],
        const std::vector<AtomData>& atomsData,
        ::AtomsTemplate* parent,
        std::string& errorMessage,
        std::vector<uint32_t>* outNewAtomIds = nullptr);
    void SetProgressCallback(std::function<void(float)> callback);
    
    /**
     * @brief XSF 파일로 저장 (미구현 - 추후 확장용)
     * 
     * @param filePath 저장할 파일 경로
     * @param cellVectors 단위 셀 벡터
     * @param atoms 원자 데이터
     * @return 저장 성공 여부
     */
    bool saveXSFFile(const std::string& filePath, 
                     const float cellVectors[3][3],
                     const std::vector<AtomData>& atoms);
    
    // ========================================================================
    // 유틸리티 함수
    // ========================================================================
    
    /**
     * @brief 원자 번호로부터 원소 기호 얻기
     * 
     * @param atomicNumber 원자 번호
     * @return 원소 기호 (예: "H", "C", "O")
     */
    static std::string getSymbolFromAtomicNumber(int atomicNumber);
    
    /**
     * @brief 파일 확장자 확인
     * 
     * @param filePath 파일 경로
     * @return 파일 확장자 (소문자)
     */
    static std::string getFileExtension(const std::string& filePath);
    
private:
    ::AtomsTemplate* m_parent;  // 부모 객체 참조
    std::function<void(float)> m_progressCallback;
    float m_lastProgress { -1.0f };
    std::chrono::steady_clock::time_point m_lastProgressTime { };
    uint64_t m_progressCounter { 0 };
    
    // ========================================================================
    // XSF 파싱 내부 함수
    // ========================================================================
    
    /**
     * @brief XSF 파일 파싱 구현
     * 
     * @param filePath 파일 경로
     * @return 파싱 결과
     */
    ParseResult parseXSFFile(const std::string& filePath);

    /**
     * @brief XSF DATAGRID_3D 파싱 구현
     *
     * @param filePath 파일 경로
     * @return 파싱 결과
     */
    Grid3DParseResult parse3DGridXSFFile(const std::string& filePath);
    void reportProgress(std::ifstream& file, std::streampos totalSize, bool force = false);
    
    /**
     * @brief PRIMVEC 섹션 파싱
     * 
     * @param line 현재 줄
     * @param primvecLine 현재 PRIMVEC 줄 번호
     * @param cellVectors 셀 벡터 배열 (출력)
     * @return 파싱 성공 여부
     */
    bool parsePrimvecLine(const std::string& line, int primvecLine, 
                         float cellVectors[3][3]);
    
    /**
     * @brief PRIMCOORD 섹션의 원자 데이터 파싱
     * 
     * @param line 현재 줄
     * @param atoms 원자 데이터 벡터 (출력)
     * @return 파싱 성공 여부
     */
    bool parseAtomLine(const std::string& line, std::vector<AtomData>& atoms);
    
    // ========================================================================
    // 화학 원소 데이터 (정적 배열)
    // ========================================================================
    static const char* s_chemicalSymbols[];
    static const size_t s_numElements;
};

} // namespace infrastructure
} // namespace atoms
