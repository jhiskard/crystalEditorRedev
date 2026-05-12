#pragma once

#include "core/io/chgcar_parser.h"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace core::io {
struct XsfGridData;
struct XsfGridParseResult;
} // namespace core::io

namespace features::data::charge_density {

class ChargeDensity {
public:
    ChargeDensity() = default;
    ChargeDensity(const std::vector<float>& data,
                  const std::array<int, 3>& gridShape,
                  const float lattice[3][3]);

    static std::unique_ptr<ChargeDensity> FromFile(const std::string& filePath);
    static std::unique_ptr<ChargeDensity> FromChgcarParseResult(const core::io::ChgcarParser::ParseResult& parsed);
    static std::unique_ptr<ChargeDensity> FromXsfGridParseResult(const core::io::XsfGridParseResult& parsed);
    static std::unique_ptr<ChargeDensity> FromXsfGridData(const core::io::XsfGridParseResult& parsed,
                                                          const core::io::XsfGridData& grid);
    static std::unique_ptr<ChargeDensity> CreateSample();

    const std::array<int, 3>& GridShape() const { return gridShape_; }
    const std::array<std::array<float, 3>, 3>& Lattice() const { return lattice_; }
    const std::vector<float>& Data() const { return data_; }

    std::pair<float, float> ValueRange() const { return {minValue_, maxValue_}; }
    float MinValue() const { return minValue_; }
    float MaxValue() const { return maxValue_; }

private:
    void updateStatistics();

    std::vector<float> data_;
    std::array<int, 3> gridShape_{0, 0, 0};
    std::array<std::array<float, 3>, 3> lattice_{};
    float minValue_ = 0.0f;
    float maxValue_ = 0.0f;
};

} // namespace features::data::charge_density
