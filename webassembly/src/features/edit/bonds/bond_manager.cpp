#include "bond_manager.h"

#include "core/data/element_database.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace features::edit::bonds {
namespace {

float Distance(const std::array<float, 3>& lhs, const std::array<float, 3>& rhs) {
    const float dx = rhs[0] - lhs[0];
    const float dy = rhs[1] - lhs[1];
    const float dz = rhs[2] - lhs[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

BondManager::BondManager(core::scene::SceneState& scene)
    : scene_(scene) {
}

void BondManager::RecomputeAll(int32_t structureId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return;
    }

    core::scene::StructureRecord* record = ResolveRecordMutable(sid);
    if (record == nullptr) {
        return;
    }

    std::vector<core::scene::BondRecord> rebuilt;
    rebuilt.reserve((record->atoms.size() * (record->atoms.size() - 1)) / 2);

    for (size_t i = 0; i < record->atoms.size(); ++i) {
        for (size_t j = i + 1; j < record->atoms.size(); ++j) {
            const core::scene::AtomRecord& atomA = record->atoms[i];
            const core::scene::AtomRecord& atomB = record->atoms[j];

            const std::string bondTypeKey = BuildBondTypeKey(atomA.symbol, atomB.symbol);
            const float threshold = GetOrCreateThreshold(bondTypeKey, atomA.symbol, atomB.symbol);

            const float distance = Distance(atomA.cartesian, atomB.cartesian);
            if (distance < 0.1f || distance > threshold) {
                continue;
            }

            core::scene::BondRecord bond;
            bond.id = scene_.nextBondId++;
            bond.atomId1 = atomA.id;
            bond.atomId2 = atomB.id;
            bond.typeKey = bondTypeKey;
            bond.radius = std::max(0.001f, std::min(atomA.radius, atomB.radius) * 0.25f);
            bond.distance = distance;
            bond.threshold = threshold;
            bond.visible = IsBondTypeVisible(bondTypeKey);
            bond.selected = false;
            rebuilt.push_back(bond);
        }
    }

    record->bonds = std::move(rebuilt);
    scene_.events.onBondsChanged.Emit(core::scene::BondsChangedEvent{sid});
}

void BondManager::ClearAll(int32_t structureId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return;
    }

    core::scene::StructureRecord* record = ResolveRecordMutable(sid);
    if (record == nullptr) {
        return;
    }

    if (!record->bonds.empty()) {
        record->bonds.clear();
        scene_.events.onBondsChanged.Emit(core::scene::BondsChangedEvent{sid});
    }
}

void BondManager::SetBondTypeVisible(int32_t structureId, const std::string& bondTypeKey, bool visible) {
    visibleByType_[bondTypeKey] = visible;

    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return;
    }
    core::scene::StructureRecord* record = ResolveRecordMutable(sid);
    if (record == nullptr) {
        return;
    }

    bool changed = false;
    for (auto& bond : record->bonds) {
        if (bond.typeKey != bondTypeKey || bond.visible == visible) {
            continue;
        }
        bond.visible = visible;
        changed = true;
    }

    if (changed) {
        scene_.events.onBondsChanged.Emit(core::scene::BondsChangedEvent{sid});
    }
}

bool BondManager::IsBondTypeVisible(const std::string& bondTypeKey) const {
    const auto it = visibleByType_.find(bondTypeKey);
    return it == visibleByType_.end() ? true : it->second;
}

void BondManager::SetBondDistanceThreshold(const std::string& bondTypeKey, float distance) {
    thresholdByType_[bondTypeKey] = std::max(0.01f, distance);
}

float BondManager::GetBondDistanceThreshold(const std::string& bondTypeKey) const {
    const auto it = thresholdByType_.find(bondTypeKey);
    if (it != thresholdByType_.end()) {
        return it->second;
    }

    const size_t sep = bondTypeKey.find('-');
    if (sep == std::string::npos) {
        return 2.0f;
    }
    const std::string symbolA = bondTypeKey.substr(0, sep);
    const std::string symbolB = bondTypeKey.substr(sep + 1);
    return ComputeDefaultThreshold(symbolA, symbolB);
}

float BondManager::GetDefaultBondDistanceThreshold(const std::string& bondTypeKey) const {
    const size_t sep = bondTypeKey.find('-');
    if (sep == std::string::npos) {
        return 2.0f;
    }
    const std::string symbolA = bondTypeKey.substr(0, sep);
    const std::string symbolB = bondTypeKey.substr(sep + 1);
    return ComputeDefaultThreshold(symbolA, symbolB);
}

void BondManager::SetGlobalThickness(float thickness) {
    globalThickness_ = std::max(0.05f, thickness);
}

void BondManager::SetGlobalOpacity(float opacity) {
    globalOpacity_ = std::clamp(opacity, 0.05f, 1.0f);
}

std::vector<std::string> BondManager::ListBondTypes(int32_t structureId) const {
    std::set<std::string> sorted;
    if (const core::scene::StructureRecord* record = ResolveRecordConst(structureId); record != nullptr) {
        std::map<std::string, int> symbolCounts;
        for (const auto& atom : record->atoms) {
            if (atom.symbol.empty()) {
                continue;
            }
            ++symbolCounts[atom.symbol];
        }

        if (!symbolCounts.empty()) {
            std::vector<std::string> symbols;
            symbols.reserve(symbolCounts.size());
            for (const auto& [symbol, count] : symbolCounts) {
                (void)count;
                symbols.push_back(symbol);
            }

            for (size_t i = 0; i < symbols.size(); ++i) {
                for (size_t j = i; j < symbols.size(); ++j) {
                    if (i == j && symbolCounts[symbols[i]] < 2) {
                        continue;
                    }
                    sorted.insert(BuildBondTypeKey(symbols[i], symbols[j]));
                }
            }
        }
    }

    return std::vector<std::string>(sorted.begin(), sorted.end());
}

int BondManager::CountBondsByType(int32_t structureId, const std::string& bondTypeKey) const {
    const core::scene::StructureRecord* record = ResolveRecordConst(structureId);
    if (record == nullptr) {
        return 0;
    }

    int count = 0;
    for (const auto& bond : record->bonds) {
        if (bond.typeKey == bondTypeKey) {
            ++count;
        }
    }
    return count;
}

int BondManager::ActiveBondCount(int32_t structureId) const {
    const core::scene::StructureRecord* record = ResolveRecordConst(structureId);
    if (record == nullptr) {
        return 0;
    }
    return static_cast<int>(record->bonds.size());
}

int32_t BondManager::ResolveStructureId(int32_t structureId) const {
    int32_t sid = structureId;
    if (sid < 0) {
        sid = scene_.currentStructureId;
    }
    if (sid < 0 || !scene_.structures.Exists(sid)) {
        return -1;
    }
    return sid;
}

core::scene::StructureRecord* BondManager::ResolveRecordMutable(int32_t structureId) {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return nullptr;
    }
    auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end()) {
        return nullptr;
    }
    return &it->second;
}

const core::scene::StructureRecord* BondManager::ResolveRecordConst(int32_t structureId) const {
    const int32_t sid = ResolveStructureId(structureId);
    if (sid < 0) {
        return nullptr;
    }
    auto it = scene_.structureRecords.find(sid);
    if (it == scene_.structureRecords.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string BondManager::BuildBondTypeKey(const std::string& symbolA, const std::string& symbolB) {
    if (symbolA <= symbolB) {
        return symbolA + "-" + symbolB;
    }
    return symbolB + "-" + symbolA;
}

float BondManager::ComputeDefaultThreshold(const std::string& symbolA, const std::string& symbolB) const {
    const core::data::ElementDatabase& db = core::data::ElementDatabase::getInstance();
    const core::data::ElementInfo* infoA = db.getElementInfo(symbolA);
    const core::data::ElementInfo* infoB = db.getElementInfo(symbolB);

    const float radiusA = (infoA != nullptr) ? infoA->covalentRadius : 1.0f;
    const float radiusB = (infoB != nullptr) ? infoB->covalentRadius : 1.0f;
    return radiusA + radiusB + 0.4f;
}

float BondManager::GetOrCreateThreshold(
    const std::string& bondTypeKey,
    const std::string& symbolA,
    const std::string& symbolB) {
    const auto it = thresholdByType_.find(bondTypeKey);
    if (it != thresholdByType_.end()) {
        return it->second;
    }

    const float value = ComputeDefaultThreshold(symbolA, symbolB);
    thresholdByType_[bondTypeKey] = value;
    return value;
}

} // namespace features::edit::bonds
