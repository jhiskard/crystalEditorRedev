#include "structure_import.h"

#include "../build/periodic_table/periodic_table.h"

#include "core/data/element_database.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace features::file {
namespace {

bool isIntegerToken(const std::string& value) {
    return !value.empty() &&
           std::all_of(value.begin(), value.end(), [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

std::string normalizeSymbol(const std::string& rawSymbol) {
    if (rawSymbol.empty()) {
        return "X";
    }

    const auto& db = core::data::ElementDatabase::getInstance();
    if (db.hasElement(rawSymbol)) {
        return rawSymbol;
    }

    if (isIntegerToken(rawSymbol)) {
        const int atomicNumber = std::stoi(rawSymbol);
        for (const std::string& symbol : db.getAllSymbols()) {
            if (db.getAtomicNumber(symbol) == atomicNumber) {
                return symbol;
            }
        }
    }

    std::string normalized = rawSymbol;
    normalized[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(normalized[0])));
    for (size_t i = 1; i < normalized.size(); ++i) {
        normalized[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(normalized[i])));
    }
    return db.hasElement(normalized) ? normalized : rawSymbol;
}

std::array<float, 3> toFloat3(const std::array<double, 3>& value) {
    return {
        static_cast<float>(value[0]),
        static_cast<float>(value[1]),
        static_cast<float>(value[2]),
    };
}

std::array<float, 3> toFloat3(const std::array<float, 3>& value) {
    return value;
}

std::array<double, 3> toDouble3(const std::array<float, 3>& value) {
    return {
        static_cast<double>(value[0]),
        static_cast<double>(value[1]),
        static_cast<double>(value[2]),
    };
}

} // namespace

StructureImporter::StructureImporter(core::scene::SceneState& scene)
    : scene_(scene) {
}

ImportSummary StructureImporter::ApplyXsf(const core::io::XsfParseResult& parsed, const std::string& displayName) {
    if (!parsed.success) {
        throw std::runtime_error(parsed.errorMessage.empty() ? "XSF parse result is not successful." : parsed.errorMessage);
    }
    if (parsed.atoms.empty()) {
        throw std::runtime_error("XSF file did not contain atoms.");
    }

    const int32_t structureId = CreateStructure(displayName);
    ApplyCell(structureId, parsed.latticeVectors);

    core::scene::StructureRecord& record = scene_.structureRecords[structureId];
    record.atoms.reserve(parsed.atoms.size());
    for (const auto& atom : parsed.atoms) {
        record.atoms.push_back(MakeAtom(structureId, atom.symbol, atom.position, false));
    }

    EmitBulkChangeEvents(structureId, true, true);

    ImportSummary summary;
    summary.kind = ImportKind::Xsf;
    summary.structureId = structureId;
    summary.atomCount = static_cast<int>(record.atoms.size());
    summary.displayName = displayName;
    summary.message = "XSF structure imported.";
    return summary;
}

ImportSummary StructureImporter::ApplyXsfGrid(const core::io::XsfGridParseResult& parsed, const std::string& displayName) {
    if (!parsed.success) {
        throw std::runtime_error(parsed.errorMessage.empty() ? "XSF grid parse result is not successful." : parsed.errorMessage);
    }

    const int32_t structureId = CreateStructure(displayName);
    bool cellChanged = false;
    bool atomsChanged = false;

    if (parsed.hasCellVectors && parsed.cellVectorsConsistent) {
        ApplyCell(structureId, parsed.latticeVectors);
        cellChanged = true;
    }

    core::scene::StructureRecord& record = scene_.structureRecords[structureId];
    record.atoms.reserve(parsed.atoms.size());
    for (const auto& atom : parsed.atoms) {
        record.atoms.push_back(MakeAtom(structureId, atom.symbol, atom.position, false));
    }
    atomsChanged = !record.atoms.empty();

    EmitBulkChangeEvents(structureId, cellChanged, atomsChanged);

    ImportSummary summary;
    summary.kind = ImportKind::XsfGrid;
    summary.structureId = structureId;
    summary.atomCount = static_cast<int>(record.atoms.size());
    summary.gridCount = static_cast<int>(parsed.grids.size());
    summary.displayName = displayName;
    summary.message = "XSF grid imported.";
    if (parsed.hasCellVectors && !parsed.cellVectorsConsistent) {
        summary.warningTitle = "XSF Cell Warning";
        summary.warningMessage = "Cell vectors differ across DATAGRID_3D blocks. Cell rendering was skipped.";
    }
    return summary;
}

ImportSummary StructureImporter::ApplyChgcar(
    const core::io::ChgcarParser::ParseResult& parsed,
    const std::string& displayName) {
    if (!parsed.success) {
        throw std::runtime_error(parsed.errorMessage.empty() ? "CHGCAR parse result is not successful." : parsed.errorMessage);
    }

    std::array<std::array<double, 3>, 3> lattice = {{
        {{parsed.lattice[0][0], parsed.lattice[0][1], parsed.lattice[0][2]}},
        {{parsed.lattice[1][0], parsed.lattice[1][1], parsed.lattice[1][2]}},
        {{parsed.lattice[2][0], parsed.lattice[2][1], parsed.lattice[2][2]}},
    }};

    const int32_t structureId = CreateStructure(displayName);
    ApplyCell(structureId, lattice);

    core::scene::StructureRecord& record = scene_.structureRecords[structureId];
    const int atomTotal = static_cast<int>(parsed.positions.size());
    record.atoms.reserve(parsed.positions.size());

    int atomIndex = 0;
    for (size_t elemIndex = 0; elemIndex < parsed.elements.size(); ++elemIndex) {
        const std::string& symbol = parsed.elements[elemIndex];
        const int count = (elemIndex < parsed.atomCounts.size()) ? parsed.atomCounts[elemIndex] : 0;
        for (int i = 0; i < count && atomIndex < atomTotal; ++i, ++atomIndex) {
            record.atoms.push_back(
                MakeAtom(structureId, symbol, toDouble3(parsed.positions[static_cast<size_t>(atomIndex)]), parsed.isDirect));
        }
    }

    EmitBulkChangeEvents(structureId, true, !record.atoms.empty());

    ImportSummary summary;
    summary.kind = ImportKind::Chgcar;
    summary.structureId = structureId;
    summary.atomCount = static_cast<int>(record.atoms.size());
    summary.displayName = displayName;
    summary.dataLoaded = !parsed.density.empty();
    summary.message = "CHGCAR structure imported.";
    return summary;
}

ImportSummary StructureImporter::ApplyUnv(const core::io::UnvParseResult& parsed, const std::string& displayName) {
    if (!parsed.success) {
        throw std::runtime_error(parsed.errorMessage.empty() ? "UNV parse result is not successful." : parsed.errorMessage);
    }

    ImportSummary summary;
    summary.kind = ImportKind::Unv;
    summary.displayName = displayName;
    summary.message = "UNV parser routing completed.";
    summary.warningTitle = "Structure Import Notice";
    summary.warningMessage = "UNV mesh import will be completed in Phase 3.9.";
    return summary;
}

int32_t StructureImporter::CreateStructure(const std::string& displayName) {
    int32_t nextId = 0;
    for (const auto& [id, entry] : scene_.structures.List()) {
        (void)entry;
        nextId = std::max(nextId, id + 1);
    }
    while (scene_.structures.Exists(nextId)) {
        ++nextId;
    }

    const std::string name = displayName.empty() ? ("Structure " + std::to_string(nextId + 1)) : displayName;
    if (!scene_.structures.Register(nextId, name)) {
        throw std::runtime_error("Failed to register imported structure.");
    }

    scene_.currentStructureId = nextId;
    scene_.structureRecords[nextId] = core::scene::StructureRecord{};
    scene_.selection.Clear();
    return nextId;
}

void StructureImporter::ApplyCell(int32_t structureId, const std::array<std::array<double, 3>, 3>& matrix) {
    core::scene::StructureRecord& record = scene_.structureRecords[structureId];
    record.cell.hasCell = true;
    record.cell.visible = true;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            record.cell.matrix[row][col] = static_cast<float>(matrix[row][col]);
        }
    }
}

void StructureImporter::EmitBulkChangeEvents(int32_t structureId, bool cellChanged, bool atomsChanged) {
    if (cellChanged) {
        scene_.events.onCellChanged.Emit(core::scene::CellChangedEvent{structureId});
    }
    if (atomsChanged) {
        scene_.events.onAtomsChanged.Emit(core::scene::AtomsChangedEvent{structureId});
    }
}

core::scene::AtomRecord StructureImporter::MakeAtom(
    int32_t structureId,
    const std::string& rawSymbol,
    const std::array<double, 3>& position,
    bool fractionalPosition) {
    core::scene::AtomRecord atom;
    atom.id = scene_.nextAtomId++;
    atom.symbol = normalizeSymbol(rawSymbol);
    atom.group = "Default";
    atom.visible = true;
    atom.selected = false;

    const auto& db = core::data::ElementDatabase::getInstance();
    atom.radius = std::max(0.001f, db.getDefaultRadius(atom.symbol));

    core::scene::StructureRecord& record = scene_.structureRecords[structureId];
    if (fractionalPosition && record.cell.hasCell) {
        atom.fractional = toFloat3(position);
        atom.cartesian = features::build::periodic_table::FractionalToCartesian(atom.fractional, record.cell.matrix);
    } else {
        atom.cartesian = toFloat3(position);
        if (record.cell.hasCell) {
            atom.fractional = features::build::periodic_table::CartesianToFractional(atom.cartesian, record.cell.matrix);
        }
    }
    return atom;
}

} // namespace features::file
