#pragma once

#include "core/scene/scene_state.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace features::file {

enum class ImportKind {
    Auto,
    Xsf,
    XsfGrid,
    Chgcar,
    Unv,
};

struct ImportSummary {
    ImportKind kind = ImportKind::Auto;
    int32_t structureId = -1;
    int atomCount = 0;
    int gridCount = 0;
    bool dataLoaded = false;
    std::string displayName;
    std::string message;
    std::string warningTitle;
    std::string warningMessage;
};

struct ImportProgressState {
    bool visible = false;
    float progress = 0.0f;
    std::string title = "Loading structure file";
    std::string text;
};

struct ImportSnapshot {
    bool valid = false;
    int32_t currentStructureId = -1;
    uint32_t nextAtomId = 1;
    uint32_t nextBondId = 1;
    std::vector<core::scene::StructureEntry> structures;
    std::unordered_map<int32_t, core::scene::StructureRecord> structureRecords;
};

} // namespace features::file
