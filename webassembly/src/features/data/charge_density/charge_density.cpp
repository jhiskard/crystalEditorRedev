#include "charge_density.h"

#include "core/io/chgcar_parser.h"

#include <algorithm>
#include <cmath>

namespace features::data::charge_density {

ChargeDensity::ChargeDensity(const std::vector<float>& data,
                             const std::array<int, 3>& gridShape,
                             const float lattice[3][3])
    : data_(data),
      gridShape_(gridShape) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            lattice_[i][j] = lattice[i][j];
        }
    }
    updateStatistics();
}

std::unique_ptr<ChargeDensity> ChargeDensity::FromFile(const std::string& filePath) {
    const core::io::ChgcarParser::ParseResult parsed = core::io::ChgcarParser::parse(filePath);
    if (!parsed.success || parsed.density.empty()) {
        return nullptr;
    }

    return std::make_unique<ChargeDensity>(parsed.density, parsed.gridShape, parsed.lattice);
}

std::unique_ptr<ChargeDensity> ChargeDensity::CreateSample() {
    constexpr int kGridSize = 32;
    const std::array<int, 3> gridShape = {kGridSize, kGridSize, kGridSize};
    std::vector<float> sampleData;
    sampleData.reserve(static_cast<size_t>(kGridSize) * static_cast<size_t>(kGridSize) * static_cast<size_t>(kGridSize));

    const float inv = 1.0f / static_cast<float>(kGridSize - 1);
    for (int z = 0; z < kGridSize; ++z) {
        for (int y = 0; y < kGridSize; ++y) {
            for (int x = 0; x < kGridSize; ++x) {
                const float fx = (static_cast<float>(x) * inv - 0.5f) * 2.0f;
                const float fy = (static_cast<float>(y) * inv - 0.5f) * 2.0f;
                const float fz = (static_cast<float>(z) * inv - 0.5f) * 2.0f;

                const float g1 = std::exp(-((fx + 0.35f) * (fx + 0.35f) * 12.0f) -
                                          (fy * fy * 10.0f) -
                                          ((fz - 0.1f) * (fz - 0.1f) * 14.0f));
                const float g2 = std::exp(-((fx - 0.25f) * (fx - 0.25f) * 18.0f) -
                                          ((fy + 0.2f) * (fy + 0.2f) * 16.0f) -
                                          ((fz + 0.15f) * (fz + 0.15f) * 18.0f));
                const float wave = 0.12f * std::sin(6.0f * fx) * std::cos(6.0f * fy) * std::sin(4.0f * fz);

                sampleData.push_back(g1 - 0.8f * g2 + wave);
            }
        }
    }

    const float lattice[3][3] = {
        {10.0f, 0.0f, 0.0f},
        {0.0f, 10.0f, 0.0f},
        {0.0f, 0.0f, 10.0f},
    };

    return std::make_unique<ChargeDensity>(sampleData, gridShape, lattice);
}

void ChargeDensity::updateStatistics() {
    if (data_.empty()) {
        minValue_ = 0.0f;
        maxValue_ = 0.0f;
        return;
    }

    const auto [minIt, maxIt] = std::minmax_element(data_.begin(), data_.end());
    minValue_ = *minIt;
    maxValue_ = *maxIt;
}

} // namespace features::data::charge_density
