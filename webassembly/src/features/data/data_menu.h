#pragma once

#include "core/io/chgcar_parser.h"

#include <string>

namespace core::scene {
struct SceneState;
}

namespace core::io {
struct XsfGridParseResult;
} // namespace core::io

namespace features::data {

struct Request {
    enum class Type {
        ShowIsosurface,
        ShowSurface,
        ShowVolumetric,
        ShowPlane,
    };

    Type type = Type::ShowIsosurface;
};

void InitOnce(core::scene::SceneState& scene);

void DrawMenu();
void HandleRequest(const Request& request);
void RenderWindows();
void Tick(float dt);
void Shutdown();

bool LoadChgcarParseResult(const std::string& name, const core::io::ChgcarParser::ParseResult& parsed);
bool LoadXsfGridResult(const std::string& name, const core::io::XsfGridParseResult& parsed);

} // namespace features::data
