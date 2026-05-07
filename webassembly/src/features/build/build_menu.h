#pragma once

namespace core::scene {
struct SceneState;
}

namespace features::build {

struct Request {
    enum class Type {
        ShowPeriodicTable,
        ShowBravaisLattice,
    };

    Type type = Type::ShowPeriodicTable;
};

void InitOnce(core::scene::SceneState& scene);

void DrawMenu();
void HandleRequest(const Request& request);
void RenderWindows();
void Tick(float dt);
void Shutdown();

} // namespace features::build
