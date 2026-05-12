/**
 * @file bind_function.cpp
 * @brief Embind exports for the app shell and restored File feature bridge.
 */
#include "app/app.h"
#include "features/file/file_menu.h"

#include <emscripten/bind.h>

#include <cstdint>
#include <string>

namespace {

#ifdef DEBUG_BUILD
void stub_printMeshTree() {}
#endif

} // namespace

EMSCRIPTEN_BINDINGS(Constant) {
    emscripten::function("initIdbfs", &app::App::InitIdbfs);
    emscripten::function("saveImGuiIniFile", &app::App::SaveImGuiIniFile);
    emscripten::function("loadImGuiIniFile", &app::App::LoadImGuiIniFile);

    emscripten::function("loadArrayBuffer", &features::file::LoadArrayBuffer);
    emscripten::function("loadChgcarFile", &features::file::LoadChgcarFile);
    emscripten::function("handleXSFGridFile", &features::file::HandleXsfGridFile);
    emscripten::function("handleStructureFile", &features::file::HandleStructureFile);
#ifdef DEBUG_BUILD
    emscripten::function("printMeshTree", &stub_printMeshTree);
#endif
    emscripten::function("writeChunk", &features::file::WriteChunk);
    emscripten::function("closeFile", &features::file::CloseFile);
    emscripten::function("processFileInBackground", &features::file::ProcessFileInBackground);
    emscripten::function("showProgressPopup", &features::file::ShowProgressPopup);
    emscripten::function("setProgress", &features::file::SetProgress);
    emscripten::function("setProgressPopupText", &features::file::SetProgressPopupText);
}
