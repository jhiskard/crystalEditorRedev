#pragma once

#include <cstdint>
#include <string>

namespace core::scene {
struct SceneState;
}

namespace features::file {

struct Request {
    enum class Type {
        OpenStructure,
    };

    Type type = Type::OpenStructure;
};

void InitOnce(core::scene::SceneState& scene);

void DrawMenu();
void HandleRequest(const Request& request);
void RenderWindows();
void Tick(float dt);
void Shutdown();

void OpenStructure();
void LoadArrayBuffer(const std::string& fileName, bool deleteFile);
void LoadChgcarFile(const std::string& fileName);
void HandleXsfGridFile(const std::string& fileName);
void HandleStructureFile(const std::string& fileName);
void WriteChunk(const std::string& fileName, int32_t offset, uintptr_t data, int32_t length);
void CloseFile(const std::string& fileName);
void ProcessFileInBackground(const std::string& fileName, bool deleteFile);
void ShowProgressPopup(bool show);
void SetProgress(float progress);
void SetProgressPopupText(const std::string& title, const std::string& text);

} // namespace features::file
