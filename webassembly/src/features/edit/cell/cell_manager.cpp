#include "cell_manager.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace features::edit::cell {

namespace {

constexpr float kPi = 3.14159265358979323846f;

float DegToRad(float degrees) {
    return degrees * kPi / 180.0f;
}

float RadToDeg(float radians) {
    return radians * 180.0f / kPi;
}

float Clamp(float value, float lo, float hi) {
    return std::max(lo, std::min(hi, value));
}

std::array<std::array<float, 3>, 3> IdentityMatrix() {
    return {{
        {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}},
    }};
}

float VectorNorm(const std::array<float, 3>& v) {
    return std::sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
}

float Dot(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

std::array<float, 3> Cross(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return {{
        (a[1] * b[2]) - (a[2] * b[1]),
        (a[2] * b[0]) - (a[0] * b[2]),
        (a[0] * b[1]) - (a[1] * b[0]),
    }};
}

float ComputeVolume(const std::array<std::array<float, 3>, 3>& matrix) {
    const std::array<float, 3> v1 = matrix[0];
    const std::array<float, 3> v2 = matrix[1];
    const std::array<float, 3> v3 = matrix[2];
    const std::array<float, 3> cross = Cross(v2, v3);
    return std::fabs(Dot(v1, cross));
}

std::array<std::array<float, 3>, 3> BuildMatrixFromParameters(
    float a,
    float b,
    float c,
    float alpha,
    float beta,
    float gamma) {
    const float alphaRad = DegToRad(alpha);
    const float betaRad = DegToRad(beta);
    const float gammaRad = DegToRad(gamma);

    const float cosAlpha = std::cos(alphaRad);
    const float cosBeta = std::cos(betaRad);
    const float cosGamma = std::cos(gammaRad);
    const float sinGamma = std::sin(gammaRad);
    const float safeSinGamma = (std::fabs(sinGamma) < 1e-6f) ? 1e-6f : sinGamma;
    const float term =
        1.0f - (cosAlpha * cosAlpha) - (cosBeta * cosBeta) - (cosGamma * cosGamma) +
        (2.0f * cosAlpha * cosBeta * cosGamma);

    std::array<std::array<float, 3>, 3> matrix = IdentityMatrix();
    matrix[0] = {{a, 0.0f, 0.0f}};
    matrix[1] = {{b * cosGamma, b * safeSinGamma, 0.0f}};
    matrix[2] = {{
        c * cosBeta,
        c * (cosAlpha - (cosBeta * cosGamma)) / safeSinGamma,
        c * std::sqrt(std::max(0.0f, term)) / safeSinGamma,
    }};
    return matrix;
}

void ExtractParametersFromMatrix(
    const std::array<std::array<float, 3>, 3>& matrix,
    float& a,
    float& b,
    float& c,
    float& alpha,
    float& beta,
    float& gamma) {
    const std::array<float, 3> v1 = matrix[0];
    const std::array<float, 3> v2 = matrix[1];
    const std::array<float, 3> v3 = matrix[2];

    a = VectorNorm(v1);
    b = VectorNorm(v2);
    c = VectorNorm(v3);

    if (a < 1e-6f || b < 1e-6f || c < 1e-6f) {
        alpha = 90.0f;
        beta = 90.0f;
        gamma = 90.0f;
        return;
    }

    const float cosAlpha = Clamp(Dot(v2, v3) / (b * c), -1.0f, 1.0f);
    const float cosBeta = Clamp(Dot(v1, v3) / (a * c), -1.0f, 1.0f);
    const float cosGamma = Clamp(Dot(v1, v2) / (a * b), -1.0f, 1.0f);

    alpha = RadToDeg(std::acos(cosAlpha));
    beta = RadToDeg(std::acos(cosBeta));
    gamma = RadToDeg(std::acos(cosGamma));
}

} // namespace

CellManager::CellManager(core::scene::SceneState& scene)
    : scene_(scene) {
}

void CellManager::SetMatrix(int32_t structureId, const std::array<std::array<float, 3>, 3>& matrix) {
    const int32_t sid = ResolveStructureId(structureId, true);
    if (sid < 0) {
        return;
    }

    core::scene::StructureRecord& record = scene_.structureRecords[sid];
    record.cell.hasCell = true;
    record.cell.matrix = matrix;

    scene_.events.onCellChanged.Emit(core::scene::CellChangedEvent{sid});
}

void CellManager::SetParameters(
    int32_t structureId,
    float a,
    float b,
    float c,
    float alpha,
    float beta,
    float gamma) {
    const auto matrix = BuildMatrixFromParameters(a, b, c, alpha, beta, gamma);
    SetMatrix(structureId, matrix);
}

std::array<std::array<float, 3>, 3> CellManager::GetMatrix(int32_t structureId) const {
    const int32_t sid = ResolveStructureIdConst(structureId);
    if (sid < 0) {
        return IdentityMatrix();
    }

    const auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end() || !it->second.cell.hasCell) {
        return IdentityMatrix();
    }
    return it->second.cell.matrix;
}

void CellManager::GetParameters(
    int32_t structureId,
    float& a,
    float& b,
    float& c,
    float& alpha,
    float& beta,
    float& gamma) const {
    const auto matrix = GetMatrix(structureId);
    ExtractParametersFromMatrix(matrix, a, b, c, alpha, beta, gamma);
}

float CellManager::Volume(int32_t structureId) const {
    const int32_t sid = ResolveStructureIdConst(structureId);
    if (sid < 0) {
        return 0.0f;
    }

    const auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end() || !it->second.cell.hasCell) {
        return 0.0f;
    }
    return ComputeVolume(it->second.cell.matrix);
}

void CellManager::AlignAxis(int32_t structureId, int axis) {
    const int32_t sid = ResolveStructureId(structureId, true);
    if (sid < 0) {
        return;
    }

    auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end() || !it->second.cell.hasCell) {
        return;
    }

    std::array<std::array<float, 3>, 3> matrix = it->second.cell.matrix;
    if (axis == 1) {
        std::swap(matrix[0], matrix[1]);
    } else if (axis == 2) {
        std::swap(matrix[0], matrix[2]);
    }

    SetMatrix(sid, matrix);
}

int32_t CellManager::ResolveStructureId(int32_t structureId, bool createIfMissing) {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }

    if (sid >= 0 && scene_.structures.Exists(sid)) {
        if (scene_.structureRecords.find(sid) == scene_.structureRecords.end()) {
            scene_.structureRecords[sid] = core::scene::StructureRecord{};
        }
        return sid;
    }

    if (!createIfMissing) {
        return -1;
    }

    int32_t nextId = 0;
    for (const auto& entry : scene_.structures.List()) {
        nextId = std::max(nextId, entry.first + 1);
    }

    if (!scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1))) {
        while (scene_.structures.Exists(nextId)) {
            ++nextId;
        }
        scene_.structures.Register(nextId, "Structure " + std::to_string(nextId + 1));
    }

    scene_.currentStructureId = nextId;
    if (scene_.structureRecords.find(nextId) == scene_.structureRecords.end()) {
        scene_.structureRecords[nextId] = core::scene::StructureRecord{};
    }
    return nextId;
}

int32_t CellManager::ResolveStructureIdConst(int32_t structureId) const {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0 || !scene_.structures.Exists(sid)) {
        return -1;
    }
    return sid;
}

} // namespace features::edit::cell

