#pragma once

#include "import_types.h"

#include "core/io/chgcar_parser.h"
#include "core/io/unv_reader.h"
#include "core/io/xsf_parser.h"
#include "core/scene/scene_state.h"

#include <string>

namespace features::file {

class StructureImporter {
public:
    explicit StructureImporter(core::scene::SceneState& scene);

    ImportSummary ApplyXsf(const core::io::XsfParseResult& parsed, const std::string& displayName);
    ImportSummary ApplyXsfGrid(const core::io::XsfGridParseResult& parsed, const std::string& displayName);
    ImportSummary ApplyChgcar(const core::io::ChgcarParser::ParseResult& parsed, const std::string& displayName);
    ImportSummary ApplyUnv(const core::io::UnvParseResult& parsed, const std::string& displayName);

private:
    int32_t CreateStructure(const std::string& displayName);
    void ApplyCell(int32_t structureId, const std::array<std::array<double, 3>, 3>& matrix);
    void EmitBulkChangeEvents(int32_t structureId, bool cellChanged, bool atomsChanged);

    core::scene::AtomRecord MakeAtom(
        int32_t structureId,
        const std::string& rawSymbol,
        const std::array<double, 3>& position,
        bool fractionalPosition);

    core::scene::SceneState& scene_;
};

} // namespace features::file
