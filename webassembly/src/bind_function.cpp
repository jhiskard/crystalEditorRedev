/**
 * @file bind_function.cpp
 * @brief Embind exports for the Phase 1 bootstrap shell.
 */
#include "app/app.h"

#include <emscripten/bind.h>

#include <cstdint>
#include <string>

namespace {

void stub_loadArrayBuffer(const std::string& /*fileName*/, bool /*deleteFile*/) {}
void stub_loadChgcarFile(const std::string& /*fileName*/) {}
void stub_handleXSFGridFile(const std::string& /*fileName*/) {}
void stub_handleStructureFile(const std::string& /*fileName*/) {}
void stub_writeChunk(const std::string& /*fileName*/, int32_t /*offset*/, uintptr_t /*data*/, int32_t /*length*/) {}
void stub_closeFile(const std::string& /*fileName*/) {}
void stub_processFileInBackground(const std::string& /*fileName*/, bool /*deleteFile*/) {}
void stub_showProgressPopup(bool /*show*/) {}
void stub_setProgressPopupText(const std::string& /*title*/, const std::string& /*text*/) {}
#ifdef DEBUG_BUILD
void stub_printMeshTree() {}
#endif

} // namespace

EMSCRIPTEN_BINDINGS(Constant) {
    emscripten::function("initIdbfs", &app::App::InitIdbfs);
    emscripten::function("saveImGuiIniFile", &app::App::SaveImGuiIniFile);
    emscripten::function("loadImGuiIniFile", &app::App::LoadImGuiIniFile);

    emscripten::function("loadArrayBuffer", &stub_loadArrayBuffer);
    emscripten::function("loadChgcarFile", &stub_loadChgcarFile);
    emscripten::function("handleXSFGridFile", &stub_handleXSFGridFile);
    emscripten::function("handleStructureFile", &stub_handleStructureFile);
#ifdef DEBUG_BUILD
    emscripten::function("printMeshTree", &stub_printMeshTree);
#endif
    emscripten::function("writeChunk", &stub_writeChunk);
    emscripten::function("closeFile", &stub_closeFile);
    emscripten::function("processFileInBackground", &stub_processFileInBackground);
    emscripten::function("showProgressPopup", &stub_showProgressPopup);
    emscripten::function("setProgressPopupText", &stub_setProgressPopupText);
}
