#include "legacy/app.h"
#include "legacy/file_loader.h"
#include "legacy/mesh_manager.h"

// Emscripten
#include <emscripten/bind.h>


EMSCRIPTEN_BINDINGS(Constant) {
    emscripten::function("initIdbfs", &App::InitIdbfs);
    emscripten::function("saveImGuiIniFile", &App::SaveImGuiIniFile);
    emscripten::function("loadImGuiIniFile", &App::LoadImGuiIniFile);
    emscripten::function("loadArrayBuffer", &FileLoader::LoadArrayBuffer);
    // CHGCAR 파일 로더 바인딩
    emscripten::function("loadChgcarFile", &FileLoader::LoadChgcarFile);
    // XSF 그리드 파일 핸들러 바인딩
    emscripten::function("handleXSFGridFile", &FileLoader::HandleXSFGridFile);
    emscripten::function("handleStructureFile", &FileLoader::HandleStructureFile);
#ifdef DEBUG_BUILD
    emscripten::function("printMeshTree", &MeshManager::PrintMeshTree);
#endif
    emscripten::function("writeChunk", &FileLoader::WriteChunk);
    emscripten::function("closeFile", &FileLoader::CloseFile);
    emscripten::function("processFileInBackground", &FileLoader::ProcessFileInBackground);
    emscripten::function("showProgressPopup", &App::ShowProgressPopup);
    emscripten::function("setProgressPopupText", &App::SetProgressPopupText);
}
