#include "file_dialog.h"

#include <algorithm>
#include <cstring>

namespace core::io {

void FileDialog::RequestOpenStructureImport() {
    // Phase 2 skeleton: JS bridge wiring is restored in Phase 3.
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
