#include "file_loader.h"
#include "config/log_config.h"
#include "mesh_manager.h"
#include "vtk_viewer.h"
#include "app.h"
#include "common/string_utils.h"
#include "unv_reader.h"
#include "atoms/atoms_template.h"
#include "atoms/infrastructure/file_io_manager.h"
#include "atoms/domain/atom_manager.h"
#include "atoms/ui/charge_density_ui.h"

// Emscripten
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>

// ImGui
#include <imgui.h>

// Standard library
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <utility>

// VTK
#include <vtkGenericDataObjectReader.h>
#include <vtkDataSet.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkResampleToImage.h>
#include <vtkStructuredGrid.h>

#include "atoms/infrastructure/chgcar_parser.h"

std::unordered_map<std::string, FILE*> FileLoader::s_FileMap;

namespace {
struct XsfParseTask {
    atoms::infrastructure::FileIOManager::ParseResult result;
    std::string filePath;
};

struct XsfGridParseTask {
    atoms::infrastructure::FileIOManager::Grid3DParseResult result;
    std::string filePath;
};

struct ChgcarParseTask {
    atoms::infrastructure::ChgcarParser::ParseResult result;
    std::string filePath;
};

bool isAxisAlignedGrid(const float vectors[3][3]) {
    const float eps = 1e-6f;
    return std::fabs(vectors[0][1]) < eps &&
           std::fabs(vectors[0][2]) < eps &&
           std::fabs(vectors[1][0]) < eps &&
           std::fabs(vectors[1][2]) < eps &&
           std::fabs(vectors[2][0]) < eps &&
           std::fabs(vectors[2][1]) < eps;
}

vtkSmartPointer<vtkStructuredGrid> createStructuredGrid(
    const atoms::infrastructure::FileIOManager::Grid3DResult& grid) {
    vtkSmartPointer<vtkStructuredGrid> structuredGrid = vtkSmartPointer<vtkStructuredGrid>::New();
    structuredGrid->SetDimensions(grid.dims[0], grid.dims[1], grid.dims[2]);

    const size_t total = grid.values.size();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(static_cast<vtkIdType>(total));

    const float denomX = (grid.dims[0] > 1) ? static_cast<float>(grid.dims[0] - 1) : 1.0f;
    const float denomY = (grid.dims[1] > 1) ? static_cast<float>(grid.dims[1] - 1) : 1.0f;
    const float denomZ = (grid.dims[2] > 1) ? static_cast<float>(grid.dims[2] - 1) : 1.0f;

    size_t idx = 0;
    for (int k = 0; k < grid.dims[2]; ++k) {
        const float tz = static_cast<float>(k) / denomZ;
        for (int j = 0; j < grid.dims[1]; ++j) {
            const float ty = static_cast<float>(j) / denomY;
            for (int i = 0; i < grid.dims[0]; ++i) {
                const float tx = static_cast<float>(i) / denomX;
                const float x = grid.origin[0]
                    + tx * grid.vectors[0][0]
                    + ty * grid.vectors[1][0]
                    + tz * grid.vectors[2][0];
                const float y = grid.origin[1]
                    + tx * grid.vectors[0][1]
                    + ty * grid.vectors[1][1]
                    + tz * grid.vectors[2][1];
                const float z = grid.origin[2]
                    + tx * grid.vectors[0][2]
                    + ty * grid.vectors[1][2]
                    + tz * grid.vectors[2][2];
                points->SetPoint(static_cast<vtkIdType>(idx), x, y, z);
                ++idx;
            }
        }
    }

    vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
    scalars->SetName(grid.label.c_str());
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(static_cast<vtkIdType>(total));
    for (size_t i = 0; i < total; ++i) {
        scalars->SetValue(static_cast<vtkIdType>(i), grid.values[i]);
    }

    structuredGrid->SetPoints(points);
    structuredGrid->GetPointData()->SetScalars(scalars);
    return structuredGrid;
}

vtkSmartPointer<vtkImageData> createImageData(
    const atoms::infrastructure::FileIOManager::Grid3DResult& grid) {
    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions(grid.dims[0], grid.dims[1], grid.dims[2]);
    image->SetOrigin(grid.origin[0], grid.origin[1], grid.origin[2]);

    double spacing[3] = { 1.0, 1.0, 1.0 };
    if (grid.dims[0] > 1) {
        spacing[0] = std::fabs(grid.vectors[0][0]) / static_cast<double>(grid.dims[0] - 1);
    }
    if (grid.dims[1] > 1) {
        spacing[1] = std::fabs(grid.vectors[1][1]) / static_cast<double>(grid.dims[1] - 1);
    }
    if (grid.dims[2] > 1) {
        spacing[2] = std::fabs(grid.vectors[2][2]) / static_cast<double>(grid.dims[2] - 1);
    }
    image->SetSpacing(spacing);

    const size_t total = grid.values.size();
    vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
    scalars->SetName(grid.label.c_str());
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(static_cast<vtkIdType>(total));
    for (size_t i = 0; i < total; ++i) {
        scalars->SetValue(static_cast<vtkIdType>(i), grid.values[i]);
    }
    image->GetPointData()->SetScalars(scalars);
    return image;
}

vtkSmartPointer<vtkImageData> resampleStructuredGridToImage(
    vtkSmartPointer<vtkStructuredGrid> grid, const int dims[3]) {
    if (!grid) {
        return nullptr;
    }
    vtkNew<vtkResampleToImage> resample;
    resample->SetInputDataObject(grid);
    resample->SetSamplingDimensions(dims[0], dims[1], dims[2]);
    double bounds[6];
    grid->GetBounds(bounds);
    resample->SetSamplingBounds(bounds);
    resample->Update();
    return resample->GetOutput();
}
bool containsDatagrid3d(const std::string& filePath) {
    std::ifstream fin(filePath);
    if (!fin) {
        return false;
    }
    std::string line;
    while (std::getline(fin, line)) {
        if (line.find("DATAGRID_3D") != std::string::npos) {
            return true;
        }
    }
    return false;
}
} // namespace

FileLoader::FileLoader() :
    m_ProgressCallback(vtkSmartPointer<vtkCallbackCommand>::New()) {
    m_ProgressCallback->SetClientData(this);
    m_ProgressCallback->SetCallback(FileLoader::progressCallback);
}

FileLoader::~FileLoader() {
    if (m_LoadingThread.joinable()) {
        m_LoadingThread.join();
    }
    if (m_StructureRoutingThread.joinable()) {
        m_StructureRoutingThread.join();
    }
    if (m_XsfLoadingThread.joinable()) {
        m_XsfLoadingThread.join();
    }
    if (m_XsfGridLoadingThread.joinable()) {
        m_XsfGridLoadingThread.join();
    }
    if (m_ChgcarLoadingThread.joinable()) {
        m_ChgcarLoadingThread.join();
    }
}

void FileLoader::RequestOpenStructureImport() {
    // Clear stale one-shot state from previous cancelled requests.
    m_ReplaceSceneOnNextStructureImport = false;
    m_DeferredStructureFileName.clear();

    if (!hasSceneDataForStructureImport()) {
        OpenStructureFileBrowser();
        return;
    }
    m_ShowStructureReplacePopup = true;
}

bool FileLoader::hasSceneDataForStructureImport() const {
    if (!collectRootMeshIds().empty()) {
        return true;
    }

    const AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
    if (atomsTemplate.HasStructures()) {
        return true;
    }
    if (!atoms::domain::createdAtoms.empty() || !atoms::domain::surroundingAtoms.empty()) {
        return true;
    }
    if (atomsTemplate.hasUnitCell()) {
        return true;
    }
    if (atomsTemplate.HasChargeDensity()) {
        return true;
    }

    return false;
}

std::vector<int32_t> FileLoader::collectRootMeshIds() const {
    std::vector<int32_t> rootMeshIds;

    const LcrsTreeUPtr& meshTree = MeshManager::Instance().GetMeshTree();
    if (!meshTree) {
        return rootMeshIds;
    }
    const TreeNode* root = meshTree->GetRoot();
    if (!root) {
        return rootMeshIds;
    }

    const TreeNode* child = root->GetLeftChild();
    while (child != nullptr) {
        rootMeshIds.push_back(child->GetId());
        child = child->GetRightSibling();
    }
    return rootMeshIds;
}

void FileLoader::beginReplaceSceneImportTransaction() {
    m_ReplaceSceneImportTransactionActive = true;
    m_PreImportRootMeshIds = collectRootMeshIds();

    const AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();
    m_PreImportCurrentStructureId = atomsTemplate.GetCurrentStructureId();
    m_PreImportChargeDensityStructureId = atomsTemplate.GetChargeDensityStructureId();
    m_PreImportLoadedFileName = atomsTemplate.GetLoadedFileName();
}

void FileLoader::clearReplaceSceneImportTransaction() {
    m_ReplaceSceneOnNextStructureImport = false;
    m_ReplaceSceneImportTransactionActive = false;
    m_DeferredStructureFileName.clear();
    m_PreImportRootMeshIds.clear();
    m_PreImportCurrentStructureId = -1;
    m_PreImportChargeDensityStructureId = -1;
    m_PreImportLoadedFileName.clear();
}

void FileLoader::showStructureImportErrorPopup(const std::string& title, const std::string& message) {
    m_StructureImportErrorTitle = title.empty() ? "Import Structure Failed" : title;
    if (message.empty()) {
        m_StructureImportErrorMessage = "The selected structure file could not be imported.";
    } else {
        m_StructureImportErrorMessage = message;
    }
    m_ShowStructureImportErrorPopup = true;
}

void FileLoader::finalizeReplaceSceneImportSuccess(int32_t importedStructureId) {
    MeshManager& meshManager = MeshManager::Instance();
    AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();

    for (int32_t rootMeshId : m_PreImportRootMeshIds) {
        if (rootMeshId == importedStructureId) {
            continue;
        }

        const Mesh* mesh = meshManager.GetMeshById(rootMeshId);
        if (!mesh) {
            continue;
        }

        if (mesh->IsXsfStructure()) {
            atomsTemplate.RemoveStructure(rootMeshId);
            meshManager.DeleteXsfStructure(rootMeshId);
        } else {
            meshManager.DeleteMesh(rootMeshId);
        }
    }

    atomsTemplate.RemoveUnassignedData();
    if (importedStructureId >= 0 && meshManager.GetMeshById(importedStructureId) != nullptr) {
        atomsTemplate.SetCurrentStructureId(importedStructureId);
    }

    clearReplaceSceneImportTransaction();
}

void FileLoader::rollbackFailedStructureImport(int32_t importedStructureId) {
    MeshManager& meshManager = MeshManager::Instance();
    AtomsTemplate& atomsTemplate = AtomsTemplate::Instance();

    if (importedStructureId >= 0) {
        const Mesh* importedMesh = meshManager.GetMeshById(importedStructureId);
        if (importedMesh != nullptr) {
            if (importedMesh->IsXsfStructure()) {
                atomsTemplate.RemoveStructure(importedStructureId);
                meshManager.DeleteXsfStructure(importedStructureId);
            } else {
                meshManager.DeleteMesh(importedStructureId);
            }
        }
    }

    atomsTemplate.SetLoadedFileName(m_PreImportLoadedFileName);

    const auto isValidStructureId = [&meshManager](int32_t structureId) {
        if (structureId < 0) {
            return false;
        }
        const Mesh* mesh = meshManager.GetMeshById(structureId);
        return mesh != nullptr && mesh->IsXsfStructure();
    };

    if (isValidStructureId(m_PreImportCurrentStructureId)) {
        atomsTemplate.SetCurrentStructureId(m_PreImportCurrentStructureId);
    } else {
        atomsTemplate.SetCurrentStructureId(-1);
    }

    if (isValidStructureId(m_PreImportChargeDensityStructureId)) {
        atomsTemplate.SetChargeDensityStructureId(m_PreImportChargeDensityStructureId);
    } else {
        atomsTemplate.SetChargeDensityStructureId(-1);
    }

    clearReplaceSceneImportTransaction();
}

void FileLoader::OpenFileBrowser(bool useMainThread) {
    EM_ASM({
        let fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.multiple = false;
        fileInput.accept = '.vtk, .vtu, .unv';
        fileInput.onchange = () => {
            if (fileInput.files.length == 0) {
                return;
            }
            const file = fileInput.files[0];
            
            VtkModule.setProgressPopupText("Loading file", `File(${file.name}) is loading, please wait...`);
            VtkModule.showProgressPopup(true);

            const reader = new FileReader();
            reader.readAsArrayBuffer(file);
            reader.onload = () => {
                try {
                    const data = new Uint8Array(reader.result);
                    VtkModule.FS.createDataFile('/', file.name, data, true, false, true);

                    const useMainThread = $0;
                    if (useMainThread === 1) {
                        VtkModule.loadArrayBuffer(file.name, true);
                        VtkModule.showProgressPopup(false);
                    }
                    else if (useMainThread === 0) {
                        VtkModule.processFileInBackground(file.name, true);
                    }
                }
                catch (e) {
                    console.error("File loading error", e);
                }
            };
            reader.onerror = (e) => {
                console.error("File reading error", e);
            };
        };
        fileInput.click();
    }, useMainThread);
}

void FileLoader::OpenStructureFileBrowser() {
    EM_ASM({
        let fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.multiple = false;
        fileInput.accept = '.xsf,.vasp,CHGCAR*,*';
        fileInput.onchange = () => {
            if (fileInput.files.length == 0) {
                return;
            }
            const file = fileInput.files[0];

            VtkModule.setProgressPopupText("Loading structure file", `File(${file.name}) is loading, please wait...`);
            VtkModule.showProgressPopup(true);

            const reader = new FileReader();
            reader.readAsArrayBuffer(file);
            reader.onload = () => {
                try {
                    const data = new Uint8Array(reader.result);
                    VtkModule.FS.createDataFile('/', file.name, data, true, false, true);
                    VtkModule.handleStructureFile(file.name);
                }
                catch (e) {
                    console.error("Structure file loading error", e);
                    VtkModule.showProgressPopup(false);
                }
            };
            reader.onerror = (e) => {
                console.error("File reading error", e);
                VtkModule.showProgressPopup(false);
            };
        };
        fileInput.click();
    });
}

void FileLoader::OpenXSFFileBrowser() {
    EM_ASM({
        let fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.multiple = false;
        fileInput.accept = '.xsf';
        fileInput.onchange = () => {
            if (fileInput.files.length == 0) {
                return;
            }
            const file = fileInput.files[0];
            
            VtkModule.setProgressPopupText("Loading XSF file", `File(${file.name}) is loading, please wait...`);
            VtkModule.showProgressPopup(true);

            const reader = new FileReader();
            reader.readAsArrayBuffer(file);
            reader.onload = () => {
                try {
                    const data = new Uint8Array(reader.result);
                    VtkModule.FS.createDataFile('/', file.name, data, true, false, true);
                    VtkModule.loadArrayBuffer(file.name, true);
                }
                catch (e) {
                    console.error("XSF file loading error", e);
                    VtkModule.showProgressPopup(false);
                }
            };
            reader.onerror = (e) => {
                console.error("File reading error", e);
                VtkModule.showProgressPopup(false);
            };
        };
        fileInput.click();
    });
}

void FileLoader::OpenXSFGridFileBrowser() {
    EM_ASM({
        let fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.multiple = false;
        fileInput.accept = '.xsf';
        fileInput.onchange = () => {
            if (fileInput.files.length == 0) {
                return;
            }
            const file = fileInput.files[0];

            VtkModule.setProgressPopupText("Loading XSF grid file", `File(${file.name}) is loading, please wait...`);
            VtkModule.showProgressPopup(true);

            const reader = new FileReader();
            reader.readAsArrayBuffer(file);
            reader.onload = () => {
                try {
                    const data = new Uint8Array(reader.result);
                    VtkModule.FS.createDataFile('/', file.name, data, true, false, true);
                    VtkModule.handleXSFGridFile(file.name);
                }
                catch (e) {
                    console.error("XSF grid file loading error", e);
                    VtkModule.showProgressPopup(false);
                }
            };
            reader.onerror = (e) => {
                console.error("File reading error", e);
                VtkModule.showProgressPopup(false);
            };
        };
        fileInput.click();
    });
}

void FileLoader::handleXSFFile(const std::string& fileName) {
    processXSFFileInBackground(fileName);
}

void FileLoader::HandleXSFGridFile(const std::string& fileName) {
    Instance().handleXSFGridFile(fileName);
}

void FileLoader::HandleStructureFile(const std::string& fileName) {
    Instance().handleStructureFile(fileName);
}

void FileLoader::handleXSFGridFile(const std::string& fileName) {
    processXSFGridFileInBackground(fileName);
}

void FileLoader::handleStructureFile(const std::string& fileName) {
    if (!m_ReplaceSceneOnNextStructureImport && hasSceneDataForStructureImport()) {
        m_DeferredStructureFileName = fileName;
        m_ShowStructureReplacePopup = true;
        App::Instance().ShowProgressPopup(false);
        return;
    }

    if (m_ReplaceSceneOnNextStructureImport) {
        beginReplaceSceneImportTransaction();
        m_ReplaceSceneOnNextStructureImport = false;
    }

    std::string extension;
    std::string fileLower = fileName;
    StringUtils::ToLower(fileLower);

    std::string::size_type dotIndex = fileName.rfind('.');
    if (dotIndex != std::string::npos && dotIndex + 1 < fileName.size()) {
        extension = fileName.substr(dotIndex + 1);
        StringUtils::ToLower(extension);
    }

    if (extension == "xsf") {
        processStructureFileInBackground(fileName);
        return;
    }

    if (extension == "vasp" || fileLower.find("chgcar") != std::string::npos) {
        processChgcarFileInBackground(fileName);
        return;
    }

    handleXSFFile(fileName);
}

void FileLoader::LoadArrayBuffer(const std::string& fileName, bool deleteFile) {
    std::string::size_type dotIndex = fileName.rfind('.');
    if (dotIndex == std::string::npos) {
        SPDLOG_ERROR("File name does not have an extension: {}", fileName);
        return;
    }

    std::string extension = fileName.substr(dotIndex + 1);
    if (extension.empty()) {
        SPDLOG_ERROR("File name does not have an extension: {}", fileName);
        return;
    }
    StringUtils::ToLower(extension);

    if (extension == "xsf") {
        try {
            SPDLOG_INFO("Processing XSF file: {}", fileName);
            Instance().handleXSFFile(fileName);
        }
        catch (const std::exception& e) {
            SPDLOG_ERROR("Error handling XSF file: {}: {}", fileName, e.what());
        }
        return;
    }

    Mesh* newMesh = nullptr;
    if (extension == "vtk") {
        newMesh = Instance().readVtkFile(fileName);
    }
    else if (extension == "vtu") {
        newMesh = Instance().readVtuFile(fileName);
    }
    else if (extension == "unv") {
        newMesh = Instance().readUnvFile(fileName);
    }

    Instance().CloseFile(fileName);
    if (deleteFile) {
        std::remove(fileName.c_str());
    }

    if (newMesh == nullptr) {
        SPDLOG_ERROR("Failed to load file: {}", fileName);
        return;
    }

    emscripten_sync_run_in_main_runtime_thread(
        EM_FUNC_SIG_VIP,
        DisplayNewMesh,
        0, (void*)newMesh
    );
}

Mesh* FileLoader::readVtkFile(const std::string& fileName) {
    vtkNew<vtkGenericDataObjectReader> genericReader;
    genericReader->SetFileName(fileName.c_str());
    genericReader->AddObserver(vtkCommand::ProgressEvent, m_ProgressCallback);
    genericReader->Update();

    vtkSmartPointer<vtkDataSet> volumeDataSet = vtkDataSet::SafeDownCast(genericReader->GetOutput());
    if (!volumeDataSet) {
        SPDLOG_ERROR("Failed to load file: {}", fileName);
        return nullptr;
    }
    
    const std::string fileNameOnly = fileName.substr(0, fileName.rfind('.'));
    Mesh* newMesh = MeshManager::Instance().InsertMesh(fileNameOnly.c_str(), nullptr, nullptr, volumeDataSet);
    return newMesh;
}

Mesh* FileLoader::readVtuFile(const std::string& fileName) {
    vtkNew<vtkXMLUnstructuredGridReader> reader;
    reader->SetFileName(fileName.c_str());    
    reader->AddObserver(vtkCommand::ProgressEvent, m_ProgressCallback);
    reader->Update();

    vtkSmartPointer<vtkUnstructuredGrid> volumeDataSet = reader->GetOutput();
    if (!volumeDataSet) {
        SPDLOG_ERROR("Failed to load file: {}", fileName);
        return nullptr;
    }

    const std::string fileNameOnly = fileName.substr(0, fileName.rfind('.'));
    Mesh* newMesh = MeshManager::Instance().InsertMesh(fileNameOnly.c_str(), nullptr, nullptr, volumeDataSet);
    return newMesh;
}

Mesh* FileLoader::readUnvFile(const std::string& fileName) {
    UnvReaderUPtr unvReader = UnvReader::New(fileName.c_str());
    if (!unvReader) {
        SPDLOG_ERROR("Failed to create UNV reader: {}", fileName);
        return nullptr;
    }

    if (!unvReader->ReadUnvFile()) {
        SPDLOG_ERROR("Failed to read UNV file: {}", fileName);
        return nullptr;
    }

    const std::string fileNameOnly = fileName.substr(0, fileName.rfind('.'));
    Mesh* newMesh = MeshManager::Instance().InsertMesh(fileNameOnly.c_str(), 
        unvReader->GetEdgeMesh(), unvReader->GetFaceMesh(), unvReader->GetVolumeMesh());

    if (unvReader->GetMeshGroupCount() > 0) {
        newMesh->SetMeshGroups(unvReader->MoveMeshGroups());
    }

    return newMesh;
}

void FileLoader::OpenFileStreamingBrowser(bool useMainThread) {
    EM_ASM({
        (async () => {
            try {
                const [fileHandle] = await window.showOpenFilePicker({
                    types: [
                      {
                        description: "Mesh files",
                        accept: {
                          "application/octet-stream": [".vtk", ".vtu", ".unv"],
                          "text/plain": [".vtk", ".vtu", ".unv"]
                        }
                      }
                    ],
                    excludeAcceptAllOption: true,
                    multiple: false
                });
                const file = await fileHandle.getFile();
                const stream = file.stream();
                const reader = stream.getReader();
                
                VtkModule.setProgressPopupText("Loading file", `File(${file.name}) is loading, please wait...`);
                VtkModule.showProgressPopup(true);

                let offset = 0;
                while (true) {
                    const { done, value } = await reader.read();
                    if (done) {
                        break;
                    }
                    
                    const buffer = VtkModule._malloc(value.length);
                    VtkModule.HEAPU8.set(value, buffer);
                    VtkModule.writeChunk(file.name, offset, buffer, value.length);
                    VtkModule._free(buffer);

                    offset += value.length;
                    console.log(`Read ${offset} bytes from file: ${file.name} - ${(offset/file.size * 100).toFixed(2)}%`);
                }

                const useMainThread = $0;
                if (useMainThread === 1) {
                    VtkModule.loadArrayBuffer(file.name, true);
                    VtkModule.showProgressPopup(false);
                }
                else if (useMainThread === 0) {
                    VtkModule.processFileInBackground(file.name, true);
                }
            } catch (e) {
                console.error("File reading error:", e);
            }
        })();
    }, useMainThread);
}

void FileLoader::WriteChunk(const std::string& fileName, int32_t offset, uintptr_t data, int32_t length) {
    FILE*& file = s_FileMap[fileName];
    
    if (!file) {
        file = fopen(fileName.c_str(), "wb+");
        if (!file) {
            SPDLOG_ERROR("Failed to open file: {}", fileName);
            return;
        }
    }

    if (fseek(file, offset, SEEK_SET) != 0) {
        SPDLOG_ERROR("Failed to seek file: {}", fileName);
        return;
    }

    size_t written = fwrite(reinterpret_cast<void*>(data), 1, length, file);
    if (written != static_cast<size_t>(length)) {
        SPDLOG_ERROR("Failed to write data to file: {}", fileName);
    }

    fflush(file);
}

void FileLoader::CloseFile(const std::string& fileName) {
    auto it = s_FileMap.find(fileName);
    if (it != s_FileMap.end()) {
        fclose(it->second);
        s_FileMap.erase(it);
    }
}

void FileLoader::progressCallback(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData) {
    vtkAlgorithm* reader = static_cast<vtkAlgorithm*>(caller);
    FileLoader* fileLoader = static_cast<FileLoader*>(clientData);

    double progress = reader->GetProgress();
    App::Instance().SetProgress(static_cast<float>(progress));
    SPDLOG_INFO("Loading object - Progress: {:.2f}%", progress * 100.0);
}

void FileLoader::ProcessFileInBackground(const std::string& fileName, bool deleteFile) {
    FileLoader::Instance().processFileInBackground(fileName, deleteFile);
}

void FileLoader::processFileInBackground(const std::string& fileName, bool deleteFile) {
    if (m_LoadingThread.joinable()) {
        m_LoadingThread.join();
    }
    
    m_LoadingThread = std::thread([this, fileName, deleteFile]() {
        try {
            LoadArrayBuffer(fileName, deleteFile);
            App::Instance().ShowProgressPopup(false);
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Error loading file: {}", e.what());
            App::Instance().ShowProgressPopup(false);
        }
    });
    
    m_LoadingThread.detach();
}

void FileLoader::processStructureFileInBackground(const std::string& fileName) {
    if (m_StructureRoutingThread.joinable()) {
        m_StructureRoutingThread.join();
    }

    m_StructureRoutingThread = std::thread([this, fileName]() {
        const std::string filePath = "/" + fileName;
        if (containsDatagrid3d(filePath)) {
            SPDLOG_INFO("Detected DATAGRID_3D in structure file: {}", fileName);
            processXSFGridFileInBackground(fileName);
            return;
        }
        processXSFFileInBackground(fileName);
    });

    m_StructureRoutingThread.detach();
}

void FileLoader::processXSFFileInBackground(const std::string& fileName) {
    if (m_XsfLoadingThread.joinable()) {
        m_XsfLoadingThread.join();
    }

    m_XsfLoadingThread = std::thread([this, fileName]() {
        SPDLOG_INFO("Starting XSF file processing (background): {}", fileName);
        std::string filePath = "/" + fileName;

        atoms::infrastructure::FileIOManager loader(&AtomsTemplate::Instance());
        loader.SetProgressCallback([](float progress) {
            App::Instance().SetProgress(progress);
        });

        auto result = loader.loadXSFFile(filePath);
        auto* task = new XsfParseTask { std::move(result), filePath };

        emscripten_async_run_in_main_runtime_thread(
            EM_FUNC_SIG_VIP,
            ApplyXSFParseResult,
            0,
            task);
    });

    m_XsfLoadingThread.detach();
}

void FileLoader::processXSFGridFileInBackground(const std::string& fileName) {
    if (m_XsfGridLoadingThread.joinable()) {
        m_XsfGridLoadingThread.join();
    }

    m_XsfGridLoadingThread = std::thread([this, fileName]() {
        SPDLOG_INFO("Starting XSF grid file processing (background): {}", fileName);
        std::string filePath = "/" + fileName;

        atoms::infrastructure::FileIOManager loader(&AtomsTemplate::Instance());
        loader.SetProgressCallback([](float progress) {
            App::Instance().SetProgress(progress);
        });

        auto result = loader.load3DGridXSFFile(filePath);
        auto* task = new XsfGridParseTask { std::move(result), filePath };

        emscripten_async_run_in_main_runtime_thread(
            EM_FUNC_SIG_VIP,
            ApplyXSFGridParseResult,
            0,
            task);
    });

    m_XsfGridLoadingThread.detach();
}

void FileLoader::processChgcarFileInBackground(const std::string& fileName) {
    if (m_ChgcarLoadingThread.joinable()) {
        m_ChgcarLoadingThread.join();
    }

    m_ChgcarLoadingThread = std::thread([fileName]() {
        SPDLOG_INFO("Starting CHGCAR file processing (background): {}", fileName);
        const std::string filePath = "/" + fileName;

        auto result = atoms::infrastructure::ChgcarParser::parse(
            filePath,
            [](float progress) {
                App::Instance().SetProgress(progress);
            });

        auto* task = new ChgcarParseTask{std::move(result), filePath};
        emscripten_async_run_in_main_runtime_thread(
            EM_FUNC_SIG_VIP,
            ApplyChgcarParseResult,
            0,
            task);
    });

    m_ChgcarLoadingThread.detach();
}

void FileLoader::DisplayNewMesh(int dummy, const void* mesh) {
    const Mesh* newMesh = static_cast<const Mesh*>(mesh);
    VtkViewer::Instance().AddActor(newMesh->GetVolumeMeshActor(), true);
}

void FileLoader::ApplyXSFParseResult(int dummy, const void* data) {
    std::unique_ptr<XsfParseTask> task(static_cast<XsfParseTask*>(const_cast<void*>(data)));
    if (!task) {
        return;
    }

    auto finishWithCleanup = [&task]() {
        if (!task->filePath.empty()) {
            std::remove(task->filePath.c_str());
            SPDLOG_INFO("Temporary XSF file deleted: {}", task->filePath);
        }
        App::Instance().ShowProgressPopup(false);
    };

    FileLoader& loader = FileLoader::Instance();
    if (!task->result.success) {
        SPDLOG_ERROR("Failed to parse XSF file: {}", task->result.errorMessage);
        loader.showStructureImportErrorPopup("Import Structure Failed", task->result.errorMessage);
        if (loader.m_ReplaceSceneImportTransactionActive) {
            loader.rollbackFailedStructureImport(-1);
        }
        finishWithCleanup();
        return;
    }

    std::string fileNameOnly = task->filePath;
    size_t lastSlash = fileNameOnly.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        fileNameOnly = fileNameOnly.substr(lastSlash + 1);
    }
    if (fileNameOnly.empty()) {
        fileNameOnly = "XSF";
    }

    int32_t structureId = -1;
    bool applySuccess = false;
    std::string failureReason;
    try {
        structureId = MeshManager::Instance().RegisterXsfStructure(fileNameOnly);
        if (structureId < 0) {
            throw std::runtime_error("Failed to register XSF structure");
        }
        AtomsTemplate::Instance().SetCurrentStructureId(structureId);
        AtomsTemplate::Instance().RegisterStructure(structureId, fileNameOnly);
        AtomsTemplate::Instance().SetLoadedFileName(fileNameOnly);
        applySuccess = AtomsTemplate::Instance().LoadXSFParsedData(task->result);
    }
    catch (const std::exception& e) {
        SPDLOG_ERROR("Error applying XSF data: {}", e.what());
        failureReason = e.what();
        applySuccess = false;
    }

    if (!applySuccess) {
        if (failureReason.empty()) {
            failureReason = "Failed to apply parsed XSF data.";
        }
        loader.showStructureImportErrorPopup("Import Structure Failed", failureReason);
        if (loader.m_ReplaceSceneImportTransactionActive) {
            loader.rollbackFailedStructureImport(structureId);
        } else if (structureId >= 0) {
            AtomsTemplate::Instance().RemoveStructure(structureId);
            MeshManager::Instance().DeleteXsfStructure(structureId);
        }
        finishWithCleanup();
        return;
    }

    if (loader.m_ReplaceSceneImportTransactionActive) {
        loader.finalizeReplaceSceneImportSuccess(structureId);
    }

    VtkViewer::Instance().ResetView();
    finishWithCleanup();
}

void FileLoader::ApplyXSFGridParseResult(int dummy, const void* data) {
    std::unique_ptr<XsfGridParseTask> task(static_cast<XsfGridParseTask*>(const_cast<void*>(data)));
    if (!task) {
        return;
    }

    auto finishWithCleanup = [&task]() {
        if (!task->filePath.empty()) {
            std::remove(task->filePath.c_str());
            SPDLOG_INFO("Temporary XSF grid file deleted: {}", task->filePath);
        }
        App::Instance().ShowProgressPopup(false);
    };

    FileLoader& loader = FileLoader::Instance();
    if (!task->result.success) {
        SPDLOG_ERROR("Failed to parse XSF grid file: {}", task->result.errorMessage);
        loader.showStructureImportErrorPopup("Import Structure Failed", task->result.errorMessage);
        if (loader.m_ReplaceSceneImportTransactionActive) {
            loader.rollbackFailedStructureImport(-1);
        }
        finishWithCleanup();
        return;
    }

    std::string fileNameOnly = task->filePath;
    size_t lastSlash = fileNameOnly.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        fileNameOnly = fileNameOnly.substr(lastSlash + 1);
    }
    if (fileNameOnly.empty()) {
        fileNameOnly = "XSF Grid";
    }
    int32_t structureId = -1;
    bool applySuccess = true;
    std::string failureReason;

    try {
        structureId = MeshManager::Instance().RegisterXsfStructure(fileNameOnly);
        if (structureId < 0) {
            throw std::runtime_error("Failed to register XSF grid structure");
        }

        AtomsTemplate::Instance().SetCurrentStructureId(structureId);
        AtomsTemplate::Instance().RegisterStructure(structureId, fileNameOnly);
        AtomsTemplate::Instance().SetLoadedFileName(fileNameOnly);

        MeshManager& meshManager = MeshManager::Instance();
        std::vector<atoms::ui::ChargeDensityUI::GridDataEntry> gridEntries;
        int gridIndex = 1;
        for (auto& gridSet : task->result.grids) {
            auto& gridHigh = gridSet.high;
            auto& gridMedium = gridSet.medium;
            auto& gridLow = gridSet.low;
            atoms::infrastructure::FileIOManager::Grid3DResult* simpleSource = &gridHigh;
            if (simpleSource->values.empty() && gridSet.hasMedium) {
                simpleSource = &gridMedium;
            }
            if (simpleSource->values.empty() && gridSet.hasLow) {
                simpleSource = &gridLow;
            }
            if (gridHigh.label.empty()) {
                std::string label = "noname_";
                if (gridIndex < 10) {
                    label += "0";
                }
                label += std::to_string(gridIndex);
                gridHigh.label = label;
                gridMedium.label = label;
                gridLow.label = label;
            }
            vtkSmartPointer<vtkDataSet> highDataSet;
            vtkSmartPointer<vtkDataSet> mediumDataSet;
            vtkSmartPointer<vtkDataSet> lowDataSet;
            vtkSmartPointer<vtkDataSet> highVolumeDataSet;
            vtkSmartPointer<vtkDataSet> mediumVolumeDataSet;
            vtkSmartPointer<vtkDataSet> lowVolumeDataSet;
            if (isAxisAlignedGrid(gridHigh.vectors)) {
                highDataSet = createImageData(gridHigh);
                if (gridSet.hasMedium && !gridMedium.values.empty()) {
                    mediumDataSet = createImageData(gridMedium);
                }
                if (gridSet.hasLow && !gridLow.values.empty()) {
                    lowDataSet = createImageData(gridLow);
                }
                highVolumeDataSet = highDataSet;
                mediumVolumeDataSet = mediumDataSet;
                lowVolumeDataSet = lowDataSet;
            }
            else {
                vtkSmartPointer<vtkStructuredGrid> highStructured = createStructuredGrid(gridHigh);
                highDataSet = highStructured;
                if (gridSet.hasMedium && !gridMedium.values.empty()) {
                    vtkSmartPointer<vtkStructuredGrid> mediumStructured = createStructuredGrid(gridMedium);
                    mediumDataSet = mediumStructured;
                    mediumVolumeDataSet = resampleStructuredGridToImage(mediumStructured, gridMedium.dims);
                }
                if (gridSet.hasLow && !gridLow.values.empty()) {
                    vtkSmartPointer<vtkStructuredGrid> lowStructured = createStructuredGrid(gridLow);
                    lowDataSet = lowStructured;
                    lowVolumeDataSet = resampleStructuredGridToImage(lowStructured, gridLow.dims);
                }
                highVolumeDataSet = resampleStructuredGridToImage(highStructured, gridHigh.dims);
            }
            Mesh* mesh = meshManager.InsertMesh(gridHigh.label.c_str(), nullptr, nullptr, highDataSet, structureId);
            if (mesh == nullptr) {
                failureReason = "Failed to create mesh from DATAGRID_3D data.";
                applySuccess = false;
                break;
            }
            mesh->SetVolumeQualityDataSets(highDataSet, mediumDataSet, lowDataSet,
                highVolumeDataSet, mediumVolumeDataSet, lowVolumeDataSet);
            mesh->SetVolumeMeshVisibility(true);
            if (simpleSource && !simpleSource->values.empty()) {
                atoms::ui::ChargeDensityUI::GridDataEntry entry;
                entry.name = simpleSource->label;
                entry.dims = { simpleSource->dims[0], simpleSource->dims[1], simpleSource->dims[2] };
                entry.origin[0] = simpleSource->origin[0];
                entry.origin[1] = simpleSource->origin[1];
                entry.origin[2] = simpleSource->origin[2];
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        entry.vectors[i][j] = simpleSource->vectors[i][j];
                    }
                }
                entry.values = std::move(simpleSource->values);
                gridEntries.push_back(std::move(entry));
            }
            ++gridIndex;
        }

        if (!gridEntries.empty()) {
            if (auto* cdUI = AtomsTemplate::Instance().chargeDensityUI()) {
                cdUI->setGridDataEntries(std::move(gridEntries));
            }

            // Import 초기 상태: Isosurface 첫 번째 데이터만 표시되도록
            // Advanced(Surface/Volume) visibility 상태는 모두 숨김으로 초기화한다.
            AtomsTemplate::Instance().SetAllChargeDensityAdvancedGridVisible(
                structureId, false, false);
            AtomsTemplate::Instance().SetAllChargeDensityAdvancedGridVisible(
                structureId, true, false);
        }

        if (applySuccess && !task->result.atoms.empty()) {
            auto payload = std::make_unique<XsfGridAtomsPayload>();
            payload->structureName = fileNameOnly;
            payload->structureId = structureId;
            payload->atoms = task->result.atoms;
            payload->hasCellVectors = task->result.hasCellVectors;
            payload->cellVectorsConsistent = task->result.cellVectorsConsistent;
            if (payload->hasCellVectors) {
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        payload->cellVectors[i][j] = task->result.cellVectors[i][j];
                    }
                }
            }

            const bool renderCell = payload->hasCellVectors && payload->cellVectorsConsistent;
            applySuccess = loader.applyXsfGridAtomsPayload(*payload, renderCell);
            if (!applySuccess && failureReason.empty()) {
                failureReason = "Failed to apply atomic data from XSF grid.";
            }
            if (applySuccess && payload->hasCellVectors && !payload->cellVectorsConsistent) {
                loader.m_XsfGridCellWarningText =
                    "Cell vectors differ across DATAGRID_3D blocks. Cell rendering was skipped.";
                loader.m_ShowXsfGridCellWarningPopup = true;
            }
        }
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error applying XSF grid data: {}", e.what());
        failureReason = e.what();
        applySuccess = false;
    }

    if (!applySuccess) {
        if (failureReason.empty()) {
            failureReason = "Failed to apply parsed XSF grid data.";
        }
        loader.showStructureImportErrorPopup("Import Structure Failed", failureReason);
        if (loader.m_ReplaceSceneImportTransactionActive) {
            loader.rollbackFailedStructureImport(structureId);
        } else if (structureId >= 0) {
            AtomsTemplate::Instance().RemoveStructure(structureId);
            MeshManager::Instance().DeleteXsfStructure(structureId);
        }
        finishWithCleanup();
        return;
    }

    if (loader.m_ReplaceSceneImportTransactionActive) {
        loader.finalizeReplaceSceneImportSuccess(structureId);
    }

    VtkViewer::Instance().ResetView();
    finishWithCleanup();
}

void FileLoader::ApplyChgcarParseResult(int dummy, const void* data) {
    std::unique_ptr<ChgcarParseTask> task(static_cast<ChgcarParseTask*>(const_cast<void*>(data)));
    if (!task) {
        return;
    }

    auto finishWithCleanup = [&task]() {
        if (!task->filePath.empty()) {
            std::remove(task->filePath.c_str());
            SPDLOG_INFO("Temporary CHGCAR file deleted: {}", task->filePath);
        }
        App::Instance().ShowProgressPopup(false);
    };

    FileLoader& loader = FileLoader::Instance();
    if (!task->result.success) {
        SPDLOG_ERROR("Failed to parse CHGCAR file: {}", task->result.errorMessage);
        loader.showStructureImportErrorPopup("Import Structure Failed", task->result.errorMessage);
        if (loader.m_ReplaceSceneImportTransactionActive) {
            loader.rollbackFailedStructureImport(-1);
        }
        finishWithCleanup();
        return;
    }

    std::string fileNameOnly = task->filePath;
    size_t lastSlash = fileNameOnly.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        fileNameOnly = fileNameOnly.substr(lastSlash + 1);
    }
    if (fileNameOnly.empty()) {
        fileNameOnly = "CHGCAR";
    }

    int32_t structureId = -1;
    bool applySuccess = false;
    std::string failureReason;
    try {
        structureId = MeshManager::Instance().RegisterXsfStructure(fileNameOnly);
        if (structureId < 0) {
            throw std::runtime_error("Failed to register CHGCAR structure");
        }
        AtomsTemplate::Instance().SetCurrentStructureId(structureId);
        AtomsTemplate::Instance().RegisterStructure(structureId, fileNameOnly);

        applySuccess = AtomsTemplate::Instance().LoadChgcarParsedData(task->result);
        if (applySuccess) {
            AtomsTemplate::Instance().SetLoadedFileName(fileNameOnly);
            SPDLOG_INFO("CHGCAR file loaded successfully: {}", fileNameOnly);
        } else {
            SPDLOG_ERROR("Failed to apply CHGCAR parse result: {}", fileNameOnly);
            failureReason = "Failed to apply parsed CHGCAR data.";
        }
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error applying CHGCAR data: {}", e.what());
        failureReason = e.what();
        applySuccess = false;
    }

    if (!applySuccess) {
        if (failureReason.empty()) {
            failureReason = "Failed to import CHGCAR data.";
        }
        loader.showStructureImportErrorPopup("Import Structure Failed", failureReason);
        if (loader.m_ReplaceSceneImportTransactionActive) {
            loader.rollbackFailedStructureImport(structureId);
        } else if (structureId >= 0) {
            AtomsTemplate::Instance().RemoveStructure(structureId);
            MeshManager::Instance().DeleteXsfStructure(structureId);
        }
        finishWithCleanup();
        return;
    }

    if (loader.m_ReplaceSceneImportTransactionActive) {
        loader.finalizeReplaceSceneImportSuccess(structureId);
    }

    VtkViewer::Instance().ResetView();
    finishWithCleanup();
}

bool FileLoader::applyXsfGridAtomsPayload(const XsfGridAtomsPayload& payload, bool renderCell) {
    if (payload.atoms.empty()) {
        return true;
    }

    std::string structureName = payload.structureName;
    if (structureName.empty()) {
        structureName = "XSF Grid";
    }
    int32_t structureId = payload.structureId;
    if (structureId < 0) {
        structureId = MeshManager::Instance().RegisterXsfStructure(structureName);
    }
    if (structureId < 0) {
        SPDLOG_ERROR("Failed to register XSF grid structure for atoms payload");
        return false;
    }

    AtomsTemplate::Instance().SetCurrentStructureId(structureId);
    AtomsTemplate::Instance().RegisterStructure(structureId, structureName);

    atoms::infrastructure::FileIOManager::ParseResult parse;
    parse.success = true;
    if (payload.hasCellVectors) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                parse.cellVectors[i][j] = payload.cellVectors[i][j];
            }
        }
    }
    parse.atoms = payload.atoms;

    try {
        return AtomsTemplate::Instance().LoadXSFParsedData(parse, renderCell);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Error applying XSF grid atoms: {}", e.what());
        return false;
    }
}

void FileLoader::RenderXsfGridImportPopups() {
    if (m_ShowStructureReplacePopup) {
        ImGui::OpenPopup("Clear Current Data and Import?");
        m_ShowStructureReplacePopup = false;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(520.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal("Clear Current Data and Import?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Viewer is not empty.");
        ImGui::TextWrapped("Do you want to clear all current data and continue importing?");

        if (ImGui::Button("Yes")) {
            if (!m_DeferredStructureFileName.empty()) {
                m_ReplaceSceneOnNextStructureImport = true;
                App::Instance().SetProgressPopupText(
                    "Loading structure file",
                    "File(" + m_DeferredStructureFileName + ") is loading, please wait...");
                App::Instance().ShowProgressPopup(true);
                const std::string deferredFileName = m_DeferredStructureFileName;
                m_DeferredStructureFileName.clear();
                handleStructureFile(deferredFileName);
            } else {
                m_ReplaceSceneOnNextStructureImport = true;
                OpenStructureFileBrowser();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            m_ReplaceSceneOnNextStructureImport = false;
            if (!m_DeferredStructureFileName.empty()) {
                const std::string memfsPath = "/" + m_DeferredStructureFileName;
                std::remove(memfsPath.c_str());
                m_DeferredStructureFileName.clear();
            }
            App::Instance().ShowProgressPopup(false);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_ShowStructureImportErrorPopup) {
        ImGui::OpenPopup("Import Structure Failed");
        m_ShowStructureImportErrorPopup = false;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(540.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal("Import Structure Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!m_StructureImportErrorTitle.empty()) {
            ImGui::TextUnformatted(m_StructureImportErrorTitle.c_str());
            ImGui::Separator();
        }
        if (m_StructureImportErrorMessage.empty()) {
            ImGui::TextWrapped("The selected structure file could not be imported.");
        } else {
            ImGui::TextWrapped("%s", m_StructureImportErrorMessage.c_str());
        }
        if (ImGui::Button("OK")) {
            m_StructureImportErrorTitle.clear();
            m_StructureImportErrorMessage.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_ShowXsfGridReplacePopup) {
        ImGui::OpenPopup("Replace Atoms and Cell?");
        m_ShowXsfGridReplacePopup = false;
    }
    if (ImGui::BeginPopupModal("Replace Atoms and Cell?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Existing atoms or unit cell detected.");
        ImGui::TextWrapped("Replace them with ATOMS data from the XSF grid file?");

        if (ImGui::Button("Yes")) {
            if (m_PendingXsfGridAtoms) {
                const bool renderCell = m_PendingXsfGridAtoms->hasCellVectors &&
                                        m_PendingXsfGridAtoms->cellVectorsConsistent;
                applyXsfGridAtomsPayload(*m_PendingXsfGridAtoms, renderCell);
                if (m_PendingXsfGridAtoms->hasCellVectors &&
                    !m_PendingXsfGridAtoms->cellVectorsConsistent) {
                    m_XsfGridCellWarningText =
                        "Cell vectors differ across DATAGRID_3D blocks. Cell rendering was skipped.";
                    m_ShowXsfGridCellWarningPopup = true;
                }
            }
            m_PendingXsfGridAtoms.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            m_PendingXsfGridAtoms.reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_ShowXsfGridCellWarningPopup) {
        ImGui::OpenPopup("XSF Cell Warning");
        m_ShowXsfGridCellWarningPopup = false;
    }
    if (ImGui::BeginPopupModal("XSF Cell Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_XsfGridCellWarningText.empty()) {
            ImGui::TextWrapped("Cell vectors differ across DATAGRID_3D blocks.");
        } else {
            ImGui::TextWrapped("%s", m_XsfGridCellWarningText.c_str());
        }
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void FileLoader::OpenChgcarFileBrowser() {
    EM_ASM({
        let fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.multiple = false;
        fileInput.accept = '.vasp,CHGCAR*,*';
        fileInput.onchange = () => {
            if (fileInput.files.length == 0) {
                return;
            }
            const file = fileInput.files[0];
            
            VtkModule.setProgressPopupText("Loading CHGCAR file", `File(${file.name}) is loading, please wait...`);
            VtkModule.showProgressPopup(true);
            
            const reader = new FileReader();
            reader.readAsArrayBuffer(file);
            reader.onload = () => {
                try {
                    const data = new Uint8Array(reader.result);
                    VtkModule.FS.createDataFile('/', file.name, data, true, false, true);
                    VtkModule.loadChgcarFile(file.name);
                }
                catch (e) {
                    console.error("CHGCAR file loading error", e);
                    VtkModule.showProgressPopup(false);
                }
            };
            reader.onerror = (e) => {
                console.error("File reading error", e);
                VtkModule.showProgressPopup(false);
            };
        };
        fileInput.click();
    });
}

void FileLoader::LoadChgcarFile(const std::string& filename) {
    SPDLOG_INFO("LoadChgcarFile called: {}", filename);

    Instance().processChgcarFileInBackground(filename);
}
