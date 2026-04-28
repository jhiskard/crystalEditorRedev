#pragma once

// Standard library
#include <string>
#include <vector>


namespace StringUtils {

std::string Trim(const std::string& str);       // Remove leading and trailing whitespace
std::string TrimLeft(const std::string& str);   // Remove leading whitespace
std::string TrimRight(const std::string& str);  // Remove trailing whitespace
void ToLower(std::string& str);
void ToUpper(std::string& str);
std::vector<std::string> Split(const std::string& str, const std::string& delimiter);
bool IsNumber(const std::string& str);          // Check if all charactor of string is a number

}