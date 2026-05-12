#include "file_dialog.h"

#include <algorithm>
#include <cstring>

#include <emscripten/emscripten.h>

namespace core::io {

void FileDialog::RequestOpenStructureImport() {
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
                } catch (e) {
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

void FileDialog::WriteChunk(const std::string& fileName,
                            int32_t offset,
                            uintptr_t data,
                            int32_t length) {
    if (fileName.empty() || length <= 0 || data == 0 || offset < 0) {
        return;
    }

    auto& store = ChunkStore();
    auto& buffer = store[fileName];
    const size_t begin = static_cast<size_t>(offset);
    const size_t end = begin + static_cast<size_t>(length);
    if (buffer.size() < end) {
        buffer.resize(end);
    }

    const auto* src = reinterpret_cast<const uint8_t*>(data);
    std::memcpy(buffer.data() + begin, src, static_cast<size_t>(length));
}

void FileDialog::CloseFile(const std::string& fileName) {
    if (fileName.empty()) {
        return;
    }
    auto& store = ChunkStore();
    if (store.find(fileName) == store.end()) {
        store.emplace(fileName, std::vector<uint8_t>{});
    }
}

std::vector<uint8_t> FileDialog::LoadArrayBuffer(const std::string& fileName,
                                                 bool deleteFile) {
    if (fileName.empty()) {
        return {};
    }

    auto& store = ChunkStore();
    auto it = store.find(fileName);
    if (it == store.end()) {
        return {};
    }

    std::vector<uint8_t> payload = it->second;
    if (deleteFile) {
        store.erase(it);
    }
    return payload;
}

std::unordered_map<std::string, std::vector<uint8_t>>& FileDialog::ChunkStore() {
    static std::unordered_map<std::string, std::vector<uint8_t>> chunkStore;
    return chunkStore;
}

} // namespace core::io
