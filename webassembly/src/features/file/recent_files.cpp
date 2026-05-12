#include "recent_files.h"

#include <algorithm>
#include <utility>

namespace features::file {

void RecentFiles::Add(std::string path) {
    if (path.empty()) {
        return;
    }

    entries_.erase(std::remove(entries_.begin(), entries_.end(), path), entries_.end());
    entries_.insert(entries_.begin(), std::move(path));
    constexpr size_t kMaxRecentFiles = 8;
    if (entries_.size() > kMaxRecentFiles) {
        entries_.resize(kMaxRecentFiles);
    }
}

void RecentFiles::Clear() {
    entries_.clear();
}

} // namespace features::file
