#pragma once

#include "import_types.h"
#include "recent_files.h"
#include "structure_import.h"

#include "core/io/format_registry.h"
#include "core/scene/scene_state.h"

#include <string>

namespace features::file {

class StructureImportController {
public:
    explicit StructureImportController(core::scene::SceneState& scene);

    void RequestOpenStructureImport();
    void HandleStructureFile(const std::string& fileName);
    void HandleXsfGridFile(const std::string& fileName);
    void LoadChgcarFile(const std::string& fileName);
    void LoadArrayBuffer(const std::string& fileName, bool deleteFile);
    void ProcessFileInBackground(const std::string& fileName, bool deleteFile);

    void ShowProgressPopup(bool show);
    void SetProgress(float progress);
    void SetProgressPopupText(std::string title, std::string text);

    bool ConsumeReplacePopupOpen();
    bool ConsumeErrorPopupOpen();
    bool ConsumeWarningPopupOpen();

    void ConfirmReplace();
    void CancelReplace();
    void AcknowledgeError();
    void AcknowledgeWarning();

    const ImportProgressState& Progress() const { return progress_; }
    const std::string& ErrorTitle() const { return errorTitle_; }
    const std::string& ErrorMessage() const { return errorMessage_; }
    const std::string& WarningTitle() const { return warningTitle_; }
    const std::string& WarningMessage() const { return warningMessage_; }
    const RecentFiles& Recent() const { return recentFiles_; }

private:
    enum class ForcedKind {
        Auto,
        XsfGrid,
        Chgcar,
    };

    void ImportFile(const std::string& fileName, ForcedKind forcedKind);
    void ApplyParsedImport(const std::string& filePath, const std::string& displayName, core::io::ParseResult parsed);
    void ReportProgressFromWorker(float progress);
    static void ApplyParsedImportOnMain(int dummy, const void* data);
    static void ApplyProgressOnMain(int dummy, const void* data);
    bool HasSceneData() const;
    std::string ToMemfsPath(const std::string& fileName) const;
    std::string DisplayNameFromPath(const std::string& path) const;

    void BeginReplaceTransaction();
    void CompleteReplaceTransaction();
    void RollbackReplaceTransaction();
    void ClearScene();
    void CaptureSnapshot();
    void RestoreSnapshot();

    void FinishSuccess(const std::string& filePath, const ImportSummary& summary);
    void FinishFailure(const std::string& title, const std::string& message);
    void RemoveMemfsFile(const std::string& fileName);

    core::scene::SceneState& scene_;
    core::io::FormatRegistry registry_;
    StructureImporter importer_;
    RecentFiles recentFiles_;

    ImportProgressState progress_;
    ImportSnapshot snapshot_;

    bool registryInitialized_ = false;
    bool showReplacePopup_ = false;
    bool showErrorPopup_ = false;
    bool showWarningPopup_ = false;
    bool replaceSceneOnNextImport_ = false;
    bool replaceTransactionActive_ = false;
    std::string deferredFileName_;
    std::string errorTitle_;
    std::string errorMessage_;
    std::string warningTitle_;
    std::string warningMessage_;
};

} // namespace features::file
