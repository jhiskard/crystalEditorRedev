#include "string_utils.h"


std::string StringUtils::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";  // No non-whitespace characters
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::string StringUtils::TrimLeft(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";  // No non-whitespace characters
    }
    return str.substr(first);
}

std::string StringUtils::TrimRight(const std::string& str) {
    size_t last = str.find_last_not_of(" \t\n\r");
    if (last == std::string::npos) {
        return "";
    }
    return str.substr(0, last + 1);
}

void StringUtils::ToLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void StringUtils::ToUpper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

std::vector<std::string> StringUtils::Split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter, start);
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start, end));
    return tokens;
}

bool StringUtils::IsNumber(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}