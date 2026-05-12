/**
 * @file features/measurement/measurement_menu.h
 * @brief Measurement menu entry points.
 */
#pragma once

namespace core::scene {
struct SceneState;
}

namespace core::vtk {
class MouseInteractor;
}

namespace features::measurement {

void InitOnce(core::scene::SceneState& scene, core::vtk::MouseInteractor& mouseInteractor);
void DrawMenu();
void RenderWindows();
void Shutdown();

} // namespace features::measurement
