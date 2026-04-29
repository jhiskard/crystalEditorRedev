#pragma once

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

namespace core::io {

struct ParseResult {
    bool success = false;
    std::string parserId;
    std::string path;
    std::string errorMessage;
    std::any payload;
};

class FormatRegistry {
public:
    using ParseFn = std::function<bool(const std::string&, ParseResult&)>;

    void Register(std::string ext, ParseFn fn);
    bool Parse(const std::string& path, ParseResult& out) const;

    static void RegisterDefaults(FormatRegistry& registry);

private:
    std::unordered_map<std::string, ParseFn> entries_;
};

} // namespace core::io
