#include "string_utils.h"

#include <algorithm>
#include <cctype>

namespace core::data::StringUtils {

std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::string TrimLeft(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    return str.substr(first);
}

std::string TrimRight(const std::string& str) {
    size_t last = str.find_last_not_of(" \t\n\r");
    if (last == std::string::npos) {
        return "";
    }
    return str.substr(0, last + 1);
}

void ToLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

void ToUpper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
}

std::vector<std::string> Split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter, start);
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

bool IsNumber(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(),
        [](unsigned char c) { return std::isdigit(c) != 0; });
}

} // namespace core::data::StringUtils
