#include "structure_import_controller.h"

#include "../data/data_menu.h"

#include "core/io/chgcar_parser.h"
#include "core/io/file_dialog.h"
#include "core/io/xsf_parser.h"
#include "core/vtk/vtk_viewer.h"

#include <emscripten/threading.h>

#include <algorithm>
#include <any>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

namespace features::file {
namespace {

bool isChgcarName(const std::string& value) {
    std::string lower = value;
    for (char& ch : lower) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    const auto endsWith = [&lower](const std::string& suffix) {
        return lower.size() >= suffix.size() &&
               lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    return lower.find("chgcar") != std::string::npos ||
           endsWith(".vasp") ||
           endsWith(".chgcar");
}

std::string lowerExtensionFromPath(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return ext;
}

template <typename Result>
bool assignParsePayload(core::io::ParseResult& out,
                        const std::string& path,
                        std::string parserId,
                        const Result& result) {
    out.parserId = std::move(parserId);
    out.success = result.success;
    out.path = path;
    out.errorMessage = result.errorMessage;
    out.payload = result;
    return out.success;
}

bool parseXsfWithProgress(const std::string& filePath,
                          core::io::ParseResult& out,
                          const core::io::XsfProgressCallback& progressCallback) {
    if (core::io::ContainsDatagrid3D(filePath)) {
        const core::io::XsfGridParseResult result =
            core::io::ParseXSFGridFile(filePath, progressCallback);
        return assignParsePayload(out, filePath, "xsf_grid", result);
    }

    const core::io::XsfParseResult result =
        core::io::ParseXSFFile(filePath, progressCallback);
    return assignParsePayload(out, filePath, "xsf", result);
}

bool parseXsfGridWithProgress(const std::string& filePath,
                              core::io::ParseResult& out,
                              const core::io::XsfProgressCallback& progressCallback) {
    const core::io::XsfGridParseResult result =
        core::io::ParseXSFGridFile(filePath, progressCallback);
    return assignParsePayload(out, filePath, "xsf_grid", result);
}

bool parseChgcarWithProgress(const std::string& filePath,
                             core::io::ParseResult& out,
                             const core::io::ChgcarParser::ProgressCallback& progressCallback) {
    const core::io::ChgcarParser::ParseResult result =
        core::io::ChgcarParser::parse(filePath, progressCallback);
    return assignParsePayload(out, filePath, "chgcar", result);
}

struct ParsedImportTask {
    StructureImportController* controller = nullptr;
    std::string filePath;
    std::string displayName;
    core::io::ParseResult parsed;
};

struct ProgressTask {
    StructureImportController* controller = nullptr;
    float progress = 0.0f;
};

} // namespace

StructureImportController::StructureImportController(core::scene::SceneState& scene)
    : scene_(scene)
    , importer_(scene) {
    core::io::FormatRegistry::RegisterDefaults(registry_);
    registryInitialized_ = true;
}

void StructureImportController::RequestOpenStructureImport() {
    replaceSceneOnNextImport_ = false;
    deferredFileName_.clear();

    if (HasSceneData()) {
        showReplacePopup_ = true;
        return;
    }

    core::io::FileDialog::RequestOpenStructureImport();
}

void StructureImportController::HandleStructureFile(const std::string& fileName) {
    ImportFile(fileName, ForcedKind::Auto);
}

void StructureImportController::HandleXsfGridFile(const std::string& fileName) {
    ImportFile(fileName, ForcedKind::XsfGrid);
}

void StructureImportController::LoadChgcarFile(const std::string& fileName) {
    ImportFile(fileName, ForcedKind::Chgcar);
}

void StructureImportController::LoadArrayBuffer(const std::string& fileName, bool deleteFile) {
    (void)deleteFile;
    if (fileName.empty()) {
        return;
    }
    if (isChgcarName(fileName)) {
        LoadChgcarFile(fileName);
    } else {
        HandleStructureFile(fileName);
    }
}

void StructureImportController::ProcessFileInBackground(const std::string& fileName, bool deleteFile) {
    LoadArrayBuffer(fileName, deleteFile);
}

void StructureImportController::ShowProgressPopup(bool show) {
    progress_.visible = show;
    if (show) {
        progress_.progress = 0.0f;
    }
}

void StructureImportController::SetProgress(float progress) {
    progress_.progress = std::clamp(progress, 0.0f, 1.0f);
}

void StructureImportController::SetProgressPopupText(std::string title, std::string text) {
    progress_.title = title.empty() ? "Loading structure file" : std::move(title);
    progress_.text = std::move(text);
}

bool StructureImportController::ConsumeReplacePopupOpen() {
    const bool shouldOpen = showReplacePopup_;
    showReplacePopup_ = false;
    return shouldOpen;
}

bool StructureImportController::ConsumeErrorPopupOpen() {
    const bool shouldOpen = showErrorPopup_;
    showErrorPopup_ = false;
    return shouldOpen;
}

bool StructureImportController::ConsumeWarningPopupOpen() {
    const bool shouldOpen = showWarningPopup_;
    showWarningPopup_ = false;
    return shouldOpen;
}

void StructureImportController::ConfirmReplace() {
    replaceSceneOnNextImport_ = true;
    if (deferredFileName_.empty()) {
        core::io::FileDialog::RequestOpenStructureImport();
        return;
    }

    SetProgressPopupText("Loading structure file", "File(" + deferredFileName_ + ") is loading, please wait...");
    ShowProgressPopup(true);
    const std::string fileName = deferredFileName_;
    deferredFileName_.clear();
    ImportFile(fileName, ForcedKind::Auto);
}

void StructureImportController::CancelReplace() {
    replaceSceneOnNextImport_ = false;
    if (!deferredFileName_.empty()) {
        RemoveMemfsFile(deferredFileName_);
        deferredFileName_.clear();
    }
    ShowProgressPopup(false);
}

void StructureImportController::AcknowledgeError() {
    errorTitle_.clear();
    errorMessage_.clear();
}

void StructureImportController::AcknowledgeWarning() {
    warningTitle_.clear();
    warningMessage_.clear();
}

void StructureImportController::ImportFile(const std::string& fileName, ForcedKind forcedKind) {
    if (fileName.empty()) {
        FinishFailure("Import Structure Failed", "No file was selected.");
        return;
    }

    if (!replaceSceneOnNextImport_ && !replaceTransactionActive_ && HasSceneData()) {
        deferredFileName_ = fileName;
        showReplacePopup_ = true;
        ShowProgressPopup(false);
        return;
    }

    if (!registryInitialized_) {
        core::io::FormatRegistry::RegisterDefaults(registry_);
        registryInitialized_ = true;
    }

    const std::string filePath = ToMemfsPath(fileName);
    const std::string displayName = DisplayNameFromPath(filePath);
    ShowProgressPopup(true);
    SetProgress(0.0f);

    std::thread([this, filePath, displayName, forcedKind]() {
        auto* task = new ParsedImportTask{};
        task->controller = this;
        task->filePath = filePath;
        task->displayName = displayName;
        task->parsed.path = filePath;

        const auto progressCallback = [this](float progress) {
            ReportProgressFromWorker(progress);
        };

        try {
            bool parsed = false;
            if (forcedKind == ForcedKind::XsfGrid) {
                parsed = parseXsfGridWithProgress(filePath, task->parsed, progressCallback);
            } else if (forcedKind == ForcedKind::Chgcar) {
                parsed = parseChgcarWithProgress(filePath, task->parsed, progressCallback);
            } else {
                const std::string ext = lowerExtensionFromPath(filePath);
                if (ext == ".xsf") {
                    parsed = parseXsfWithProgress(filePath, task->parsed, progressCallback);
                } else if (isChgcarName(filePath)) {
                    parsed = parseChgcarWithProgress(filePath, task->parsed, progressCallback);
                } else {
                    parsed = registry_.Parse(filePath, task->parsed);
                }
            }

            if (!parsed && task->parsed.errorMessage.empty()) {
                task->parsed.errorMessage = "The selected structure file could not be parsed.";
            }
        } catch (const std::exception& e) {
            task->parsed.success = false;
            task->parsed.errorMessage = e.what();
        } catch (...) {
            task->parsed.success = false;
            task->parsed.errorMessage = "Unknown parser error.";
        }

        if (task->parsed.success) {
            ReportProgressFromWorker(1.0f);
        }
        emscripten_async_run_in_main_runtime_thread(
            EM_FUNC_SIG_VIP,
            &StructureImportController::ApplyParsedImportOnMain,
            0,
            task);
    }).detach();
}

void StructureImportController::ApplyParsedImport(const std::string& filePath,
                                                  const std::string& displayName,
                                                  core::io::ParseResult parsed) {
    auto finishFailure = [this, &filePath](const std::string& title, const std::string& message) {
        std::remove(filePath.c_str());
        FinishFailure(title, message);
    };

    if (!parsed.success) {
        finishFailure("Import Structure Failed", parsed.errorMessage);
        return;
    }

    ImportSummary summary;
    try {
        if (replaceSceneOnNextImport_) {
            BeginReplaceTransaction();
        }

        if (parsed.parserId == "xsf") {
            summary = importer_.ApplyXsf(std::any_cast<core::io::XsfParseResult>(parsed.payload), displayName);
        } else if (parsed.parserId == "xsf_grid") {
            const auto grid = std::any_cast<core::io::XsfGridParseResult>(parsed.payload);
            summary = importer_.ApplyXsfGrid(grid, displayName);
            summary.dataLoaded = features::data::LoadXsfGridResult(displayName, grid);
        } else if (parsed.parserId == "chgcar") {
            const auto chgcar = std::any_cast<core::io::ChgcarParser::ParseResult>(parsed.payload);
            summary = importer_.ApplyChgcar(chgcar, displayName);
            summary.dataLoaded = features::data::LoadChgcarParseResult(displayName, chgcar);
        } else if (parsed.parserId == "unv") {
            summary = importer_.ApplyUnv(std::any_cast<core::io::UnvParseResult>(parsed.payload), displayName);
        } else {
            throw std::runtime_error("Unsupported parser id: " + parsed.parserId);
        }

        FinishSuccess(filePath, summary);
    } catch (const std::bad_any_cast&) {
        finishFailure("Import Structure Failed", "Parser payload type did not match the selected parser.");
    } catch (const std::exception& e) {
        finishFailure("Import Structure Failed", e.what());
    }
}

void StructureImportController::ReportProgressFromWorker(float progress) {
    auto* task = new ProgressTask{};
    task->controller = this;
    task->progress = progress;
    emscripten_async_run_in_main_runtime_thread(
        EM_FUNC_SIG_VIP,
        &StructureImportController::ApplyProgressOnMain,
        0,
        task);
}

void StructureImportController::ApplyParsedImportOnMain(int /*dummy*/, const void* data) {
    std::unique_ptr<ParsedImportTask> task(static_cast<ParsedImportTask*>(const_cast<void*>(data)));
    if (task == nullptr || task->controller == nullptr) {
        return;
    }
    task->controller->ApplyParsedImport(task->filePath, task->displayName, std::move(task->parsed));
}

void StructureImportController::ApplyProgressOnMain(int /*dummy*/, const void* data) {
    std::unique_ptr<ProgressTask> task(static_cast<ProgressTask*>(const_cast<void*>(data)));
    if (task == nullptr || task->controller == nullptr) {
        return;
    }
    task->controller->SetProgress(task->progress);
}

bool StructureImportController::HasSceneData() const {
    return scene_.currentStructureId >= 0 ||
           !scene_.structures.Empty() ||
           !scene_.structureRecords.empty();
}

std::string StructureImportController::ToMemfsPath(const std::string& fileName) const {
    if (!fileName.empty() && (fileName[0] == '/' || fileName[0] == '\\')) {
        return fileName;
    }
    return "/" + fileName;
}

std::string StructureImportController::DisplayNameFromPath(const std::string& path) const {
    const std::filesystem::path p(path);
    const std::string fileName = p.filename().string();
    return fileName.empty() ? path : fileName;
}

void StructureImportController::BeginReplaceTransaction() {
    if (replaceTransactionActive_) {
        return;
    }
    CaptureSnapshot();
    ClearScene();
    replaceTransactionActive_ = true;
    replaceSceneOnNextImport_ = false;
}

void StructureImportController::CompleteReplaceTransaction() {
    replaceTransactionActive_ = false;
    replaceSceneOnNextImport_ = false;
    snapshot_ = ImportSnapshot{};
}

void StructureImportController::RollbackReplaceTransaction() {
    if (!replaceTransactionActive_) {
        replaceSceneOnNextImport_ = false;
        return;
    }

    RestoreSnapshot();
    replaceTransactionActive_ = false;
    replaceSceneOnNextImport_ = false;
    snapshot_ = ImportSnapshot{};
}

void StructureImportController::ClearScene() {
    std::vector<int32_t> ids;
    ids.reserve(scene_.structures.List().size());
    for (const auto& [id, entry] : scene_.structures.List()) {
        (void)entry;
        ids.push_back(id);
    }
    for (int32_t id : ids) {
        scene_.structures.Remove(id);
        scene_.structureRecords.erase(id);
    }
    scene_.structureRecords.clear();
    scene_.selection.Clear();
    scene_.currentStructureId = -1;
    scene_.nextAtomId = 1;
    scene_.nextBondId = 1;
}

void StructureImportController::CaptureSnapshot() {
    snapshot_ = ImportSnapshot{};
    snapshot_.valid = true;
    snapshot_.currentStructureId = scene_.currentStructureId;
    snapshot_.nextAtomId = scene_.nextAtomId;
    snapshot_.nextBondId = scene_.nextBondId;
    snapshot_.structureRecords = scene_.structureRecords;
    snapshot_.structures.reserve(scene_.structures.List().size());
    for (const auto& [id, entry] : scene_.structures.List()) {
        (void)id;
        snapshot_.structures.push_back(entry);
    }
}

void StructureImportController::RestoreSnapshot() {
    if (!snapshot_.valid) {
        ClearScene();
        return;
    }

    ClearScene();
    for (const auto& entry : snapshot_.structures) {
        scene_.structures.Register(entry.id, entry.name);
        scene_.structures.SetVisible(entry.id, entry.visible);
    }
    scene_.structureRecords = snapshot_.structureRecords;
    scene_.currentStructureId = snapshot_.currentStructureId;
    scene_.nextAtomId = snapshot_.nextAtomId;
    scene_.nextBondId = snapshot_.nextBondId;
}

void StructureImportController::FinishSuccess(const std::string& filePath, const ImportSummary& summary) {
    if (replaceTransactionActive_) {
        CompleteReplaceTransaction();
    }

    recentFiles_.Add(filePath);
    std::remove(filePath.c_str());
    if (summary.structureId >= 0) {
        core::vtk::VtkViewer::Instance().ResetView();
    }
    ShowProgressPopup(false);
    if (!summary.warningMessage.empty()) {
        warningTitle_ = summary.warningTitle.empty() ? "Structure Import Notice" : summary.warningTitle;
        warningMessage_ = summary.warningMessage;
        showWarningPopup_ = true;
    }
}

void StructureImportController::FinishFailure(const std::string& title, const std::string& message) {
    if (replaceTransactionActive_) {
        RollbackReplaceTransaction();
    }
    replaceSceneOnNextImport_ = false;

    errorTitle_ = title.empty() ? "Import Structure Failed" : title;
    errorMessage_ = message.empty() ? "The selected structure file could not be imported." : message;
    showErrorPopup_ = true;
    ShowProgressPopup(false);
}

void StructureImportController::RemoveMemfsFile(const std::string& fileName) {
    const std::string path = ToMemfsPath(fileName);
    std::remove(path.c_str());
}

} // namespace features::file
