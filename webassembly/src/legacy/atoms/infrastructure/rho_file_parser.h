// atoms/infrastructure/rho_file_parser.h
namespace atoms {
namespace infrastructure {

struct RhoParseResult {
    bool success;
    std::string errorMessage;
    domain::ChargeDensityData data;
};

class RhoFileParser {
public:
    static RhoParseResult parse(const std::string& filePath);
    
private:
    static RhoParseResult parseVASP(const std::string& filePath);
    static RhoParseResult parseQE(const std::string& filePath);
    static RhoParseResult parseSIESTA(const std::string& filePath);
};

} // namespace infrastructure
} // namespace atoms
