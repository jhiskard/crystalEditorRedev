#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace core::io {

class FileDialog {
public:
    static void RequestOpenStructureImport();

    static void WriteChunk(const std::string& fileName,
                           int32_t offset,
                           uintptr_t data,
                           int32_t length);

    static void CloseFile(const std::string& fileName);

    static std::vector<uint8_t> LoadArrayBuffer(const std::string& fileName,
                                                bool deleteFile);

private:
    static std::unordered_map<std::string, std::vector<uint8_t>>& ChunkStore();
};

} // namespace core::io
