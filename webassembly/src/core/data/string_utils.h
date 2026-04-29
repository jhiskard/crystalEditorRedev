#pragma once

#include <string>
#include <vector>

namespace core::data::StringUtils {

std::string Trim(const std::string& str);
std::string TrimLeft(const std::string& str);
std::string TrimRight(const std::string& str);
void ToLower(std::string& str);
void ToUpper(std::string& str);
std::vector<std::string> Split(const std::string& str, const std::string& delimiter);
bool IsNumber(const std::string& str);

} // namespace core::data::StringUtils
