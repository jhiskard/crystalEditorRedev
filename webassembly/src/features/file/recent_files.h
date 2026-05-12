#pragma once

#include <string>
#include <vector>

namespace features::file {

class RecentFiles {
public:
    void Add(std::string path);
    void Clear();
    const std::vector<std::string>& Entries() const { return entries_; }

private:
    std::vector<std::string> entries_;
};

} // namespace features::file
