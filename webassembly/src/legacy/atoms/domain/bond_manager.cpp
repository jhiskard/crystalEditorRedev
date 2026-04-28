#include "bond_manager.h"
#include "../atoms_template.h"
#include "../../vtk_viewer.h"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <unordered_map>
#include <set>


namespace atoms {
namespace domain {

std::vector<atoms::domain::BondInfo> createdBonds;
std::vector<atoms::domain::BondInfo> surroundingBonds; // 주변 원자들의 결합
std::map<std::string, atoms::domain::BondGroupInfo> bondGroups;

namespace {
void initializeBondGroupRenderer(AtomsTemplate* parent, const std::string& bondTypeKey, float radius) {
    if (!parent) {
        return;
    }
    if (auto* renderer = parent->bondRenderer()) {
        renderer->initializeBondGroup(bondTypeKey, radius);
        return;
    }
    parent->initializeBondGroupVTK(bondTypeKey, radius);
}

void updateBondGroupRenderer(AtomsTemplate* parent, const std::string& bondTypeKey) {
    if (!parent) {
        return;
    }
    if (auto* batch = parent->batchSystem()) {
        if (batch->isBatchMode()) {
            batch->scheduleBondGroupUpdate(bondTypeKey);
            return;
        }
    }
    if (parent->isBatchMode()) {
        if (auto* batch = parent->batchSystem()) {
            batch->scheduleBondGroupUpdate(bondTypeKey);
        }
        return;
    }
    auto it = bondGroups.find(bondTypeKey);
    if (it == bondGroups.end()) {
        return;
    }
    if (auto* renderer = parent->bondRenderer()) {
        renderer->updateBondGroup(bondTypeKey, it->second);
        VtkViewer::Instance().RequestRender();
        return;
    }
    parent->updateBondGroupVTK(bondTypeKey);
    VtkViewer::Instance().RequestRender();
}

void clearAllBondGroupsRenderer(AtomsTemplate* parent) {
    if (!parent) {
        return;
    }
    if (auto* renderer = parent->bondRenderer()) {
        renderer->clearAllBondGroups();
        return;
    }
    parent->clearAllBondGroupsVTK();
}
} // namespace

bool hasBondGroup(const std::string& key) {
    return bondGroups.find(key) != bondGroups.end();
}

BondGroupInfo* findBondGroup(const std::string& key) {
    auto it = bondGroups.find(key);
    return (it == bondGroups.end()) ? nullptr : &it->second;
}

void initializeBondGroup(const std::string& key, float baseRadius) {
    auto it = bondGroups.find(key);
    if (it == bondGroups.end()) {
        BondGroupInfo info;
        info.key = key;
        info.baseRadius = baseRadius;
        bondGroups.emplace(key, std::move(info));
        SPDLOG_DEBUG("Domain: Initialized bond group '{}' (radius={})", key, baseRadius);
    } else {
        it->second.baseRadius = baseRadius;
        SPDLOG_DEBUG("Domain: Updated bond group '{}' (radius={})", key, baseRadius);
    }
}

void clearAllBondGroups() {
    bondGroups.clear();
    SPDLOG_DEBUG("Domain: Cleared all bond groups");
}

const std::map<std::string, BondGroupInfo>& getBondGroups() {
    return bondGroups;
}

std::string generateBondTypeKey(const std::string& element1, const std::string& element2) {
    if (element1 <= element2) {
        return element1 + "-" + element2;
    }
    return element2 + "-" + element1;
}

float calculateBondRadius(const atoms::domain::AtomInfo& atom1, const atoms::domain::AtomInfo& atom2) {
    float radius1 = std::max(atom1.bondRadius, 0.001f);
    float radius2 = std::max(atom2.bondRadius, 0.001f);

    return std::min(radius1, radius2) * 0.25f;
}

int findAtomIndex(const atoms::domain::AtomInfo& atom, const std::vector<atoms::domain::AtomInfo>& atomList) {
    if (atom.id != 0) {
        for (size_t i = 0; i < atomList.size(); ++i) {
            if (atomList[i].id == atom.id) {
                SPDLOG_DEBUG("Found atom by ID {} at index {}", atom.id, i);
                return static_cast<int>(i);
            }
        }
        SPDLOG_DEBUG("Atom ID {} not found in atom list", atom.id);
    }

    for (size_t i = 0; i < atomList.size(); ++i) {
        if (&atomList[i] == &atom) {
            SPDLOG_DEBUG("Found atom by pointer at index {}", i);
            return static_cast<int>(i);
        }
    }

    SPDLOG_DEBUG("Atom not found in list (ID: {}, symbol: {})", atom.id, atom.symbol);
    return -1;
}

atoms::domain::AtomInfo* findAtomById(uint32_t atomId, std::vector<atoms::domain::AtomInfo>& atomList) {
    if (atomId == 0) {
        SPDLOG_DEBUG("Invalid atom ID (0) provided to findAtomById");
        return nullptr;
    }

    for (auto& atom : atomList) {
        if (atom.id == atomId) {
            SPDLOG_DEBUG("Found atom with ID {} in atom list", atomId);
            return &atom;
        }
    }

    SPDLOG_DEBUG("Atom with ID {} not found in atom list of size {}", atomId, atomList.size());
    return nullptr;
}

std::pair<vtkSmartPointer<vtkTransform>, vtkSmartPointer<vtkTransform>>
calculateBondTransforms2Color(
    const atoms::domain::AtomInfo& atom1, const atoms::domain::AtomInfo& atom2
) {
    double position1[3], position2[3];
    double radius1 = static_cast<double>(std::max(atom1.bondRadius, 0.001f));
    double radius2 = static_cast<double>(std::max(atom2.bondRadius, 0.001f));

    if (atom1.modified) {
        position1[0] = atom1.tempPosition[0];
        position1[1] = atom1.tempPosition[1];
        position1[2] = atom1.tempPosition[2];
    } else {
        position1[0] = atom1.position[0];
        position1[1] = atom1.position[1];
        position1[2] = atom1.position[2];
    }

    if (atom2.modified) {
        position2[0] = atom2.tempPosition[0];
        position2[1] = atom2.tempPosition[1];
        position2[2] = atom2.tempPosition[2];
    } else {
        position2[0] = atom2.position[0];
        position2[1] = atom2.position[1];
        position2[2] = atom2.position[2];
    }

    double dx = position2[0] - position1[0];
    double dy = position2[1] - position1[1];
    double dz = position2[2] - position1[2];
    double height = std::sqrt(dx*dx + dy*dy + dz*dz);

    if (height < 1e-6) {
        SPDLOG_WARN("Bond length too small: {:.6f}", height);
        return std::make_pair(nullptr, nullptr);
    }

    double ratio = (radius1/(radius1+radius2));
    double centerX = (position1[0] + ratio*dx);
    double centerY = (position1[1] + ratio*dy);
    double centerZ = (position1[2] + ratio*dz);

    double direction[3] = {dx/height, dy/height, dz/height};
    double halfHeight = height / 2.0;

    double centerAC_X = (position1[0] + centerX) / 2.0;
    double centerAC_Y = (position1[1] + centerY) / 2.0;
    double centerAC_Z = (position1[2] + centerZ) / 2.0;

    double centerCB_X = (centerX + position2[0]) / 2.0;
    double centerCB_Y = (centerY + position2[1]) / 2.0;
    double centerCB_Z = (centerZ + position2[2]) / 2.0;

    double v1[3] = {0, 0, 0};
    double v2[3] = {0, 0, 0};
    double v3[3] = {direction[0], direction[1], direction[2]};

    if (std::abs(direction[0]) < std::abs(direction[1]) && std::abs(direction[0]) < std::abs(direction[2])) {
        v1[0] = 1.0;
    } else if (std::abs(direction[1]) < std::abs(direction[0]) && std::abs(direction[1]) < std::abs(direction[2])) {
        v1[1] = 1.0;
    } else {
        v1[2] = 1.0;
    }

    v2[0] = v3[1]*v1[2] - v3[2]*v1[1];
    v2[1] = v3[2]*v1[0] - v3[0]*v1[2];
    v2[2] = v3[0]*v1[1] - v3[1]*v1[0];

    double v2Len = std::sqrt(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]);
    if (v2Len > 1e-6) {
        v2[0] /= v2Len;
        v2[1] /= v2Len;
        v2[2] /= v2Len;
    }

    v1[0] = v2[1]*v3[2] - v2[2]*v3[1];
    v1[1] = v2[2]*v3[0] - v2[0]*v3[2];
    v1[2] = v2[0]*v3[1] - v2[1]*v3[0];

    double v1Len = std::sqrt(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]);
    if (v1Len > 1e-6) {
        v1[0] /= v1Len;
        v1[1] /= v1Len;
        v1[2] /= v1Len;
    }

    vtkSmartPointer<vtkTransform> transform1 = vtkSmartPointer<vtkTransform>::New();
    vtkSmartPointer<vtkMatrix4x4> matrix1 = vtkSmartPointer<vtkMatrix4x4>::New();

    matrix1->SetElement(0, 0, v2[0]);
    matrix1->SetElement(1, 0, v2[1]);
    matrix1->SetElement(2, 0, v2[2]);

    matrix1->SetElement(0, 1, v3[0]);
    matrix1->SetElement(1, 1, v3[1]);
    matrix1->SetElement(2, 1, v3[2]);

    matrix1->SetElement(0, 2, v1[0]);
    matrix1->SetElement(1, 2, v1[1]);
    matrix1->SetElement(2, 2, v1[2]);

    matrix1->SetElement(0, 3, centerAC_X);
    matrix1->SetElement(1, 3, centerAC_Y);
    matrix1->SetElement(2, 3, centerAC_Z);

    transform1->SetMatrix(matrix1);
    transform1->Scale(1.0, halfHeight, 1.0);  // Y축 방향으로 스케일

    vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
    vtkSmartPointer<vtkMatrix4x4> matrix2 = vtkSmartPointer<vtkMatrix4x4>::New();

    matrix2->SetElement(0, 0, v2[0]);
    matrix2->SetElement(1, 0, v2[1]);
    matrix2->SetElement(2, 0, v2[2]);

    matrix2->SetElement(0, 1, v3[0]);
    matrix2->SetElement(1, 1, v3[1]);
    matrix2->SetElement(2, 1, v3[2]);

    matrix2->SetElement(0, 2, v1[0]);
    matrix2->SetElement(1, 2, v1[1]);
    matrix2->SetElement(2, 2, v1[2]);

    matrix2->SetElement(0, 3, centerCB_X);
    matrix2->SetElement(1, 3, centerCB_Y);
    matrix2->SetElement(2, 3, centerCB_Z);

    transform2->SetMatrix(matrix2);
    transform2->Scale(1.0, halfHeight, 1.0);  // Y축 방향으로 스케일

    SPDLOG_DEBUG("Calculated 2-color bond transforms: length={:.3f}, centers=({:.3f},{:.3f},{:.3f})-({:.3f},{:.3f},{:.3f})",
                height, centerAC_X, centerAC_Y, centerAC_Z, centerCB_X, centerCB_Y, centerCB_Z);

    return std::make_pair(transform1, transform2);
}

void addBondToGroup2Color(
    const std::string& bondTypeKey,
    vtkSmartPointer<vtkTransform> transform1,
    vtkSmartPointer<vtkTransform> transform2,
    const atoms::domain::Color4f& color1,
    const atoms::domain::Color4f& color2,
    uint32_t bondId
) {
    if (bondGroups.find(bondTypeKey) == bondGroups.end()) {
        SPDLOG_ERROR("Bond group not found for key: {}", bondTypeKey);
        return;
    }

    auto& group = bondGroups[bondTypeKey];

    group.transforms1.push_back(transform1);
    group.transforms2.push_back(transform2);

    group.colors1.push_back(color1);
    group.colors2.push_back(color2);

    group.bondIds.push_back(bondId);

    SPDLOG_DEBUG("Added 2-color bond instance to group: {} (total: {})",
                bondTypeKey, group.transforms1.size());
}

bool removeBondFromGroup(const std::string& bondTypeKey, size_t instanceIndex) {
    if (bondGroups.find(bondTypeKey) == bondGroups.end()) {
        SPDLOG_DEBUG("Bond group not found for key: {}", bondTypeKey);
        return false;
    }

    auto& group = bondGroups[bondTypeKey];

    if (instanceIndex >= group.transforms1.size()) {
        SPDLOG_WARN("Instance index {} out of range for bond group: {} (size: {})",
                   instanceIndex, bondTypeKey, group.transforms1.size());
        return false;
    }

    group.transforms1.erase(group.transforms1.begin() + instanceIndex);
    group.transforms2.erase(group.transforms2.begin() + instanceIndex);
    group.colors1.erase(group.colors1.begin() + instanceIndex);
    group.colors2.erase(group.colors2.begin() + instanceIndex);
    group.bondIds.erase(group.bondIds.begin() + instanceIndex);
    return true;
}

void createBond(
    AtomsTemplate* parent,
    const atoms::domain::AtomInfo& atom1,
    const atoms::domain::AtomInfo& atom2,
    atoms::domain::BondType bondType,
    float bondScalingFactor,
    const atoms::domain::ElementDatabase& elementDB
) {
    (void)bondScalingFactor;
    (void)elementDB;
    if (!parent) {
        SPDLOG_ERROR("createBond called with null parent");
        return;
    }

    std::string bondTypeKey = atoms::domain::generateBondTypeKey(atom1.symbol, atom2.symbol);
    float bondRadius = atoms::domain::calculateBondRadius(atom1, atom2);

    atoms::domain::initializeBondGroup(bondTypeKey, bondRadius);
    initializeBondGroupRenderer(parent, bondTypeKey, bondRadius);

    auto transforms = atoms::domain::calculateBondTransforms2Color(atom1, atom2);
    if (!transforms.first || !transforms.second) {
        SPDLOG_ERROR("Failed to calculate 2-color bond transforms between {} and {}", 
                    atom1.symbol, atom2.symbol);
        return;
    }

    uint32_t bondId = parent->generateUniqueBondId();

    atoms::domain::Color4f adjustedColor1 = atom1.color;
    atoms::domain::Color4f adjustedColor2 = atom2.color;

    if (bondType == atoms::domain::BondType::SURROUNDING) {
        float colorMultiplier = 200.0f / 255.0f;
        adjustedColor1.r *= colorMultiplier;
        adjustedColor1.g *= colorMultiplier;
        adjustedColor1.b *= colorMultiplier;
        adjustedColor2.r *= colorMultiplier;
        adjustedColor2.g *= colorMultiplier;
        adjustedColor2.b *= colorMultiplier;
    }

    atoms::domain::addBondToGroup2Color(
        bondTypeKey, transforms.first, transforms.second, 
        adjustedColor1, adjustedColor2, bondId);

    atoms::domain::BondInfo bondInfo;
    bondInfo.id = bondId;
    bondInfo.isInstanced = true;
    bondInfo.bondGroupKey = bondTypeKey;
    bondInfo.bondType = bondType;
    bondInfo.structureId = atom1.structureId;
    bondInfo.atom1Id = atom1.id;
    bondInfo.atom2Id = atom2.id;

    if (bondType == atoms::domain::BondType::ORIGINAL) {
        bondInfo.atom1Index = findAtomIndex(atom1, createdAtoms);
        bondInfo.atom2Index = findAtomIndex(atom2, createdAtoms);

        if (bondInfo.atom1Index == -1) {
            int surroundingIndex = findAtomIndex(atom1, surroundingAtoms);
            if (surroundingIndex != -1) {
                bondInfo.atom1Index = -(surroundingIndex + 1);
            }
        }

        if (bondInfo.atom2Index == -1) {
            int surroundingIndex = findAtomIndex(atom2, surroundingAtoms);
            if (surroundingIndex != -1) {
                bondInfo.atom2Index = -(surroundingIndex + 1);
            }
        }
    } else {
        bondInfo.atom1Index = findAtomIndex(atom1, surroundingAtoms);
        bondInfo.atom2Index = findAtomIndex(atom2, surroundingAtoms);

        if (bondInfo.atom1Index == -1) {
            int originalIndex = findAtomIndex(atom1, createdAtoms);
            if (originalIndex != -1) {
                bondInfo.atom1Index = originalIndex;
            }
        }

        if (bondInfo.atom2Index == -1) {
            int originalIndex = findAtomIndex(atom2, createdAtoms);
            if (originalIndex != -1) {
                bondInfo.atom2Index = originalIndex;
            }
        }
    }

    if (bondGroups.find(bondTypeKey) != bondGroups.end()) {
        bondInfo.instanceIndex = bondGroups[bondTypeKey].transforms1.size() - 1;
    }

    if (bondType == atoms::domain::BondType::ORIGINAL) {
        createdBonds.push_back(bondInfo);
    } else {
        surroundingBonds.push_back(bondInfo);
    }

    updateBondGroupRenderer(parent, bondTypeKey);

    SPDLOG_DEBUG("Created bond {} between {} and {} in group {} (CPU instancing only)", 
                bondId, atom1.symbol, atom2.symbol, bondTypeKey);
}

void createBondsForAtoms(
    AtomsTemplate* parent,
    const std::vector<uint32_t>& atomIds,
    bool includeOriginal,
    bool includeSurrounding,
    bool clearExisting,
    float bondScalingFactor,
    const atoms::domain::ElementDatabase& elementDB
) {
    if (!parent) {
        SPDLOG_ERROR("createBondsForAtoms called with null parent");
        return;
    }

    SPDLOG_INFO("Creating bonds with optimized spatial grid + unified batch system (ID-based)...");
    SPDLOG_INFO("  Original: {}, Surrounding: {}, Clear existing: {}, Target IDs: {}", 
                includeOriginal, includeSurrounding, clearExisting, atomIds.size());

    if (clearExisting) {
        SPDLOG_DEBUG("Clearing existing bonds in batch mode");
        if (includeOriginal) {
            clearAllBondGroups();
            clearAllBondGroupsRenderer(parent);
            createdBonds.clear();
        }

        if (includeSurrounding && !surroundingBonds.empty()) {
            surroundingBonds.clear();
        }
    }

    std::vector<std::pair<const atoms::domain::AtomInfo*, bool>> atomsToProcess;

    if (includeOriginal) {
        if (atomIds.empty()) {
            for (const auto& atom : createdAtoms) {
                atomsToProcess.push_back({&atom, true});
            }
        } else {
            for (uint32_t atomId : atomIds) {
                atoms::domain::AtomInfo* atom = findAtomById(atomId, createdAtoms);
                if (atom) {
                    atomsToProcess.push_back({atom, true});
                    SPDLOG_DEBUG("Added original atom with ID {} to processing list", atomId);
                } else {
                    SPDLOG_WARN("Original atom with ID {} not found, skipping", atomId);
                }
            }
        }
    }

    if (includeSurrounding && parent->isSurroundingsVisible()) {
        for (const auto& atom : surroundingAtoms) {
            atomsToProcess.push_back({&atom, false});
        }
    }

    if (atomsToProcess.empty()) {
        SPDLOG_WARN("No atoms to process for bond creation");
        return;
    }

    SPDLOG_DEBUG("🎯 Pre-computing bond type keys for caching optimization");

    std::set<std::string> uniqueElements;
    for (const auto& [atom, isOriginal] : atomsToProcess) {
        (void)isOriginal;
        std::string symbolToUse = atom->modified ? atom->tempSymbol : atom->symbol;
        uniqueElements.insert(symbolToUse);
    }

    std::unordered_map<std::string, std::string> bondTypeKeyCache;
    for (const std::string& element1 : uniqueElements) {
        for (const std::string& element2 : uniqueElements) {
            std::string pairKey = element1 + ":" + element2;
            bondTypeKeyCache[pairKey] = atoms::domain::generateBondTypeKey(element1, element2);
        }
    }

    SPDLOG_DEBUG("Pre-computed {} bond type key combinations for {} unique elements", 
                bondTypeKeyCache.size(), uniqueElements.size());

    float maxBondDistance = 0.0f;
    for (const auto& [atom, isOriginal] : atomsToProcess) {
        (void)isOriginal;
        float radius = std::max(atom->bondRadius, 0.001f);
        float estimatedMaxDistance = radius * 2.0f * bondScalingFactor;
        maxBondDistance = std::max(maxBondDistance, estimatedMaxDistance);
    }
    maxBondDistance *= 1.2f;

    SPDLOG_DEBUG("Max bond distance calculated: {:.3f}", maxBondDistance);

    float minPos[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float maxPos[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const auto& [atom, isOriginal] : atomsToProcess) {
        (void)isOriginal;
        double position[3];
        double radius;

        if (!getAtomPositionAndRadius(*atom, position, radius)) {
            continue;
        }

        for (int i = 0; i < 3; i++) {
            float pos = static_cast<float>(position[i]);
            minPos[i] = std::min(minPos[i], pos);
            maxPos[i] = std::max(maxPos[i], pos);
        }
    }

    for (int i = 0; i < 3; i++) {
        minPos[i] -= maxBondDistance;
        maxPos[i] += maxBondDistance;
    }

    float gridSize[3];
    int gridDimensions[3];

    for (int i = 0; i < 3; i++) {
        gridSize[i] = maxPos[i] - minPos[i];
        gridDimensions[i] = std::max(1, static_cast<int>(std::ceil(gridSize[i] / maxBondDistance)));
    }

    int totalCells = gridDimensions[0] * gridDimensions[1] * gridDimensions[2];

    SPDLOG_DEBUG("Spatial grid: {}x{}x{} = {} cells, cell size: {:.3f}", 
                gridDimensions[0], gridDimensions[1], gridDimensions[2], 
                totalCells, maxBondDistance);

    std::unordered_map<int, std::vector<size_t>> spatialGrid;

    for (size_t atomIdx = 0; atomIdx < atomsToProcess.size(); atomIdx++) {
        const auto& [atom, isOriginal] = atomsToProcess[atomIdx];
        (void)isOriginal;

        double position[3];
        double radius;

        if (!getAtomPositionAndRadius(*atom, position, radius)) {
            continue;
        }

        int cellX = static_cast<int>((position[0] - minPos[0]) / maxBondDistance);
        int cellY = static_cast<int>((position[1] - minPos[1]) / maxBondDistance);
        int cellZ = static_cast<int>((position[2] - minPos[2]) / maxBondDistance);

        cellX = std::max(0, std::min(cellX, gridDimensions[0] - 1));
        cellY = std::max(0, std::min(cellY, gridDimensions[1] - 1));
        cellZ = std::max(0, std::min(cellZ, gridDimensions[2] - 1));

        int cellIndex = cellZ * (gridDimensions[0] * gridDimensions[1]) + 
                       cellY * gridDimensions[0] + cellX;

        spatialGrid[cellIndex].push_back(atomIdx);
    }

    int newBondsCreated = 0;
    int totalPairsChecked = 0;
    int totalPairsSkipped = 0;

    const int neighborOffsets[27][3] = {
        {-1,-1,-1}, {-1,-1,0}, {-1,-1,1},
        {-1, 0,-1}, {-1, 0,0}, {-1, 0,1},
        {-1, 1,-1}, {-1, 1,0}, {-1, 1,1},
        { 0,-1,-1}, { 0,-1,0}, { 0,-1,1},
        { 0, 0,-1}, { 0, 0,0}, { 0, 0,1},
        { 0, 1,-1}, { 0, 1,0}, { 0, 1,1},
        { 1,-1,-1}, { 1,-1,0}, { 1,-1,1},
        { 1, 0,-1}, { 1, 0,0}, { 1, 0,1},
        { 1, 1,-1}, { 1, 1,0}, { 1, 1,1}
    };

    SPDLOG_DEBUG("🚀 Starting batch bond creation loop (updates deferred)");

    for (const auto& [cellIndex, atomIndicesInCell] : spatialGrid) {
        if (atomIndicesInCell.empty()) continue;

        int cellZ = cellIndex / (gridDimensions[0] * gridDimensions[1]);
        int cellY = (cellIndex % (gridDimensions[0] * gridDimensions[1])) / gridDimensions[0];
        int cellX = cellIndex % gridDimensions[0];

        for (int neighborIdx = 0; neighborIdx < 27; neighborIdx++) {
            int neighborX = cellX + neighborOffsets[neighborIdx][0];
            int neighborY = cellY + neighborOffsets[neighborIdx][1];
            int neighborZ = cellZ + neighborOffsets[neighborIdx][2];

            if (neighborX < 0 || neighborX >= gridDimensions[0] ||
                neighborY < 0 || neighborY >= gridDimensions[1] ||
                neighborZ < 0 || neighborZ >= gridDimensions[2]) {
                continue;
            }

            int neighborCellIndex = neighborZ * (gridDimensions[0] * gridDimensions[1]) + 
                                   neighborY * gridDimensions[0] + neighborX;

            auto neighborIt = spatialGrid.find(neighborCellIndex);
            if (neighborIt == spatialGrid.end()) {
                continue;
            }

            const auto& neighborAtomIndices = neighborIt->second;

            for (size_t i : atomIndicesInCell) {
                for (size_t j : neighborAtomIndices) {
                    if (i >= j) {
                        totalPairsSkipped++;
                        continue;
                    }

                    const atoms::domain::AtomInfo* atom1 = atomsToProcess[i].first;
                    const atoms::domain::AtomInfo* atom2 = atomsToProcess[j].first;
                    bool atom1IsOriginal = atomsToProcess[i].second;
                    bool atom2IsOriginal = atomsToProcess[j].second;
                    const bool isOriginalOriginal = atom1IsOriginal && atom2IsOriginal;
                    const bool isSurroundingSurrounding = !atom1IsOriginal && !atom2IsOriginal;

                    totalPairsChecked++;

                    // 주변-주변 결합은 생성하지 않음 (원본 원자 기반 주변 표시 정책)
                    if (isSurroundingSurrounding) {
                        continue;
                    }

                    // 주변 원자 생성 후 증분 결합 생성 시 기존 원본-원본 결합 중복 방지
                    if (!clearExisting && includeOriginal && includeSurrounding && isOriginalOriginal) {
                        continue;
                    }

                    if (!atomIds.empty() && isOriginalOriginal) {
                        uint32_t id1 = atom1->id;
                        uint32_t id2 = atom2->id;

                        bool id1Selected = std::find(atomIds.begin(), atomIds.end(), id1) != atomIds.end();
                        bool id2Selected = std::find(atomIds.begin(), atomIds.end(), id2) != atomIds.end();

                        if (!id1Selected || !id2Selected) {
                            continue;
                        }

                        SPDLOG_DEBUG("Both atoms (ID: {}, ID: {}) are in selected list, creating bond", id1, id2);
                    }

                    if (atoms::domain::shouldCreateBond(*atom1, *atom2, elementDB, bondScalingFactor)) {
                        atoms::domain::BondType bondType = isOriginalOriginal ?
                        atoms::domain::BondType::ORIGINAL : 
                        atoms::domain::BondType::SURROUNDING;

                        createBond(parent, *atom1, *atom2, bondType, bondScalingFactor, elementDB);
                        newBondsCreated++;

                        if (newBondsCreated % 100 == 0) {
                            SPDLOG_DEBUG("Created {} bonds so far...", newBondsCreated);
                        }
                    }
                }
            }
        }
    }

    size_t totalAtoms = atomsToProcess.size();
    size_t naivePairCount = totalAtoms * (totalAtoms - 1) / 2;
    float reductionPercentage = totalAtoms > 1 ? 
        100.0f * (1.0f - static_cast<float>(totalPairsChecked) / naivePairCount) : 0.0f;

    SPDLOG_INFO("🚀 Optimized spatial grid + batch mode results (ID-based):");
    SPDLOG_INFO("  Target atom IDs: {} (empty = all atoms)", atomIds.size());
    SPDLOG_INFO("  Total atoms processed: {}", totalAtoms);
    SPDLOG_INFO("  Grid cells used: {}/{}", spatialGrid.size(), totalCells);
    SPDLOG_INFO("  Pre-computed bond type keys: {}", bondTypeKeyCache.size());
    SPDLOG_INFO("  Pairs checked: {} (vs {} naive approach)", totalPairsChecked, naivePairCount);
    SPDLOG_INFO("  Pairs skipped (optimization): {}", totalPairsSkipped);
    SPDLOG_INFO("  Spatial computational reduction: {:.1f}%", reductionPercentage);
    SPDLOG_INFO("  Bonds created: {} (all instanced, batch mode: {})", 
               newBondsCreated, parent->isBatchMode() ? "active" : "individual");
}

void clearAllBonds(AtomsTemplate* parent) {
    SPDLOG_INFO("Clearing all bonds with unified batch system...");
    
    try {
        int originalBondsCount = createdBonds.size();
        int surroundingBondsCount = surroundingBonds.size();

        SPDLOG_INFO("Clearing bond groups within batch...");
        clearAllBondGroups();
        clearAllBondGroupsRenderer(parent);

        createdBonds.clear();
        surroundingBonds.clear();

        SPDLOG_INFO("Cleared {} original bonds and {} surrounding bonds within batch", 
                    originalBondsCount, surroundingBondsCount);

    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error during clearAllBonds: {}", e.what());
        createdBonds.clear();
        surroundingBonds.clear();
        throw;
    }

    SPDLOG_INFO("Completed clearAllBonds within batch - rendering deferred to batch end");
}

void addBondsToGroups(
    AtomsTemplate* parent,
    int atomIndex,
    float bondScalingFactor,
    const atoms::domain::ElementDatabase& elementDB
) {
    if (!parent) {
        SPDLOG_ERROR("addBondsToGroups called with null parent");
        return;
    }

    if (atomIndex < 0 || atomIndex >= static_cast<int>(createdAtoms.size())) {
        SPDLOG_WARN("Invalid atom index for bond addition: {}", atomIndex);
        return;
    }

    const atoms::domain::AtomInfo& targetAtom = createdAtoms[atomIndex];
    int newBondsCreated = 0;

    SPDLOG_DEBUG("Adding bonds for atom {} with unified data structure", atomIndex);

    try {
        for (size_t i = 0; i < createdAtoms.size(); i++) {
            if (i == static_cast<size_t>(atomIndex)) continue;

            const atoms::domain::AtomInfo& otherAtom = createdAtoms[i];

            if (atoms::domain::shouldCreateBond(targetAtom, otherAtom, elementDB, bondScalingFactor)) {
                createBond(parent, targetAtom, otherAtom, atoms::domain::BondType::ORIGINAL, bondScalingFactor, elementDB);
                newBondsCreated++;
            }
        }

        if (parent->isSurroundingsVisible()) {
            for (const auto& surroundingAtom : surroundingAtoms) {
                if (atoms::domain::shouldCreateBond(targetAtom, surroundingAtom, elementDB, bondScalingFactor)) {
                    createBond(parent, targetAtom, surroundingAtom, atoms::domain::BondType::SURROUNDING, bondScalingFactor, elementDB);
                    newBondsCreated++;
                }
            }
        }

        SPDLOG_DEBUG("Added {} new bonds for atom {} with unified indexing (batch mode: {})", 
                    newBondsCreated, atomIndex, 
                    parent->isBatchMode() ? "scheduled" : "immediate");

    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error adding bonds for atom {}: {}", atomIndex, e.what());
        throw;
    }
}

void updateAllBondGroupThickness(AtomsTemplate* parent, float thickness) {
    if (!parent) {
        SPDLOG_ERROR("updateAllBondGroupThickness called with null parent");
        return;
    }

    auto* renderer = parent->vtkRenderer();
    if (renderer) {
        renderer->updateAllBondGroupThickness(thickness);
    }

    if (auto* batch = parent->batchSystem()) {
        for (const auto& [bondTypeKey, group] : bondGroups) {
            (void)group;
            batch->scheduleBondGroupUpdate(bondTypeKey);
        }
    }
}

void updateAllBondGroupOpacity(AtomsTemplate* parent, float opacity) {
    if (!parent) {
        SPDLOG_ERROR("updateAllBondGroupOpacity called with null parent");
        return;
    }

    auto* renderer = parent->vtkRenderer();
    if (renderer) {
        renderer->updateAllBondGroupOpacity(opacity);
    }

    if (auto* batch = parent->batchSystem()) {
        for (const auto& [bondTypeKey, group] : bondGroups) {
            (void)group;
            batch->scheduleBondGroupUpdate(bondTypeKey);
        }
    }
}

bool shouldCreateBond(
    const AtomInfo& atom1,
    const AtomInfo& atom2,
    const ElementDatabase& elementDB,
    float bondScalingFactor
) {
    (void)elementDB;
    double position1[3], position2[3];
    double radius1, radius2;

    if (!getAtomPositionAndRadius(atom1, position1, radius1)) {
        SPDLOG_DEBUG("Failed to get position/radius for atom1");
        return false;
    }

    if (!getAtomPositionAndRadius(atom2, position2, radius2)) {
        SPDLOG_DEBUG("Failed to get position/radius for atom2");
        return false;
    }

    if (&atom1 == &atom2) {
        return false;
    }

    double dx = position2[0] - position1[0];
    double dy = position2[1] - position1[1];
    double dz = position2[2] - position1[2];
    double distance = std::sqrt(dx*dx + dy*dy + dz*dz);

    if (distance < 1e-6) {
        SPDLOG_DEBUG("Atoms are at the same position, skipping bond creation");
        return false;
    }

    double bondDistance = radius1 + radius2;
    double scaledBondDistance = bondDistance * bondScalingFactor * 0.8; // 0.8 : magic number

    double minBondDistance = 0.1;                      // 너무 가까우면 결합 없음
    double maxBondDistance = scaledBondDistance * 1.5; // 너무 멀면 결합 없음

    bool shouldBond = (distance >= minBondDistance && distance <= maxBondDistance);

    if (shouldBond) {
        SPDLOG_DEBUG("Bond created: {} - {} (distance: {:.3f}, threshold: {:.3f})",
                     atom1.symbol, atom2.symbol, distance, scaledBondDistance);
    } else {
        SPDLOG_DEBUG("Bond rejected: {} - {} (distance: {:.3f}, threshold: {:.3f})",
                     atom1.symbol, atom2.symbol, distance, scaledBondDistance);
    }

    return shouldBond;
}

} //atoms
} // domain 
