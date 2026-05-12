#include "format_registry.h"

#include "chgcar_parser.h"
#include "rho_parser.h"
#include "unv_reader.h"
#include "xsf_parser.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace core::io {
namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string extensionFromPath(const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_extension()) {
        return toLower(p.extension().string());
    }
    return {};
}

std::string fileNameFromPath(const std::string& path) {
    std::filesystem::path p(path);
    return toLower(p.filename().string());
}

} // namespace

void FormatRegistry::Register(std::string ext, ParseFn fn) {
    if (ext.empty() || !fn) {
        return;
    }
    entries_[toLower(std::move(ext))] = std::move(fn);
}

bool FormatRegistry::Parse(const std::string& path, ParseResult& out) const {
    out = ParseResult{};
    out.path = path;

    const std::string ext = extensionFromPath(path);
    auto it = entries_.find(ext);

    if (it == entries_.end()) {
        const std::string fileNameLower = fileNameFromPath(path);
        if (fileNameLower.find("chgcar") != std::string::npos) {
            it = entries_.find(".chgcar");
        }
    }

    if (it == entries_.end()) {
        out.errorMessage = "No parser registered for the file extension.";
        return false;
    }

    return it->second(path, out);
}

void FormatRegistry::RegisterDefaults(FormatRegistry& registry) {
    registry.Register(".xsf", [](const std::string& path, ParseResult& out) {
        if (ContainsDatagrid3D(path)) {
            const XsfGridParseResult result = ParseXSFGridFile(path);
            out.parserId = "xsf_grid";
            out.success = result.success;
            out.path = path;
            out.errorMessage = result.errorMessage;
            out.payload = result;
            return out.success;
        }

        const XsfParseResult result = ParseXSFFile(path);
        out.parserId = "xsf";
        out.success = result.success;
        out.path = path;
        out.errorMessage = result.errorMessage;
        out.payload = result;
        return out.success;
    });

    registry.Register(".chgcar", [](const std::string& path, ParseResult& out) {
        const ChgcarParser::ParseResult result = ChgcarParser::parse(path);
        out.parserId = "chgcar";
        out.success = result.success;
        out.path = path;
        out.errorMessage = result.errorMessage;
        out.payload = result;
        return out.success;
    });

    registry.Register(".vasp", [](const std::string& path, ParseResult& out) {
        const ChgcarParser::ParseResult result = ChgcarParser::parse(path);
        out.parserId = "chgcar";
        out.success = result.success;
        out.path = path;
        out.errorMessage = result.errorMessage;
        out.payload = result;
        return out.success;
    });

    registry.Register(".rho", [](const std::string& path, ParseResult& out) {
        const RhoParseResult result = ParseRhoFile(path);
        out.parserId = "rho";
        out.success = result.success;
        out.path = path;
        out.errorMessage = result.errorMessage;
        out.payload = result;
        return out.success;
    });

    registry.Register(".unv", [](const std::string& path, ParseResult& out) {
        const UnvParseResult result = ParseUnvFile(path);
        out.parserId = "unv";
        out.success = result.success;
        out.path = path;
        out.errorMessage = result.errorMessage;
        out.payload = result;
        return out.success;
    });
}

} // namespace core::io
