#pragma once

#include "macro/singleton_macro.h"
#include "atoms/infrastructure/file_io_manager.h"

// Standard library
#include <memory>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

// VTK
#include <vtkSmartPointer.h>

class vtkObject;
class vtkCallbackCommand;
class Mesh;


class FileLoader {
    DECLARE_SINGLETON(FileLoader)

public:
    void OpenFileBrowser(bool useMainThread = true);
    void OpenFileStreamingBrowser(bool useMainThread = true);
    void RequestOpenStructureImport();
    void OpenStructureFileBrowser();
    void OpenXSFFileBrowser();  // New method to open XSF files
    void OpenXSFGridFileBrowser();  // New method to open XSF grid files

    // fileName: MemFS 내 파일 이름
    // deleteFile: MemFS에서 파일을 이 함수에서 삭제
    static void LoadArrayBuffer(const std::string& fileName, bool deleteFile);
    static void WriteChunk(const std::string& fileName, int32_t offset, uintptr_t data, int32_t length);
    static void CloseFile(const std::string& fileName);
    static void HandleXSFGridFile(const std::string& fileName);
    static void HandleStructureFile(const std::string& fileName);
    
    static void ProcessFileInBackground(const std::string& fileName, bool deleteFile);

    // private에서 public으로 이동
    void handleXSFFile(const std::string& fileName);  // XSF 파일 처리 메서드
    void handleXSFGridFile(const std::string& fileName);
    void handleStructureFile(const std::string& fileName);  // XSF 그리드 파일 처리 메서드
    void RenderXsfGridImportPopups();

    /// CHGCAR 파일 브라우저 열기
    void OpenChgcarFileBrowser();
    static void LoadChgcarFile(const std::string& filename);



private:
    struct XsfGridAtomsPayload {
        std::string structureName;
        int32_t structureId = -1;
        std::vector<atoms::infrastructure::FileIOManager::AtomData> atoms;
        float cellVectors[3][3] {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        };
        bool hasCellVectors = false;
        bool cellVectorsConsistent = true;
    };

    static std::unordered_map<std::string, FILE*> s_FileMap;
    vtkSmartPointer<vtkCallbackCommand> m_ProgressCallback;

    std::thread m_LoadingThread;
    std::thread m_StructureRoutingThread;
    std::thread m_XsfLoadingThread;
    std::thread m_XsfGridLoadingThread;
    std::thread m_ChgcarLoadingThread;
    std::unique_ptr<XsfGridAtomsPayload> m_PendingXsfGridAtoms;
    bool m_ShowStructureReplacePopup = false;
    bool m_ReplaceSceneOnNextStructureImport = false;
    bool m_ReplaceSceneImportTransactionActive = false;
    std::string m_DeferredStructureFileName;
    bool m_ShowStructureImportErrorPopup = false;
    std::string m_StructureImportErrorTitle;
    std::string m_StructureImportErrorMessage;
    std::vector<int32_t> m_PreImportRootMeshIds;
    int32_t m_PreImportCurrentStructureId = -1;
    int32_t m_PreImportChargeDensityStructureId = -1;
    std::string m_PreImportLoadedFileName;
    bool m_ShowXsfGridReplacePopup = false;
    bool m_ShowXsfGridCellWarningPopup = false;
    std::string m_XsfGridCellWarningText;

    Mesh* readVtkFile(const std::string& fileName);
    Mesh* readVtuFile(const std::string& fileName);
    Mesh* readUnvFile(const std::string& fileName);
  // XSF ?? ?? ???

    static void progressCallback(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData);
    void processFileInBackground(const std::string& fileName, bool deleteFile);
    void processStructureFileInBackground(const std::string& fileName);
    void processXSFFileInBackground(const std::string& fileName);
    void processXSFGridFileInBackground(const std::string& fileName);
    void processChgcarFileInBackground(const std::string& fileName);

    static void DisplayNewMesh(int dummy, const void* mesh);
    static void ApplyXSFParseResult(int dummy, const void* data);
    static void ApplyXSFGridParseResult(int dummy, const void* data);
    static void ApplyChgcarParseResult(int dummy, const void* data);
    bool applyXsfGridAtomsPayload(const XsfGridAtomsPayload& payload, bool renderCell);
    bool hasSceneDataForStructureImport() const;
    std::vector<int32_t> collectRootMeshIds() const;
    void beginReplaceSceneImportTransaction();
    void clearReplaceSceneImportTransaction();
    void finalizeReplaceSceneImportSuccess(int32_t importedStructureId);
    void rollbackFailedStructureImport(int32_t importedStructureId);
    void showStructureImportErrorPopup(const std::string& title, const std::string& message);

};
