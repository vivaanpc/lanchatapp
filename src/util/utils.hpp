#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace Utils {
    std::string trim(const std::string& str);
    std::string getCurrentTimeString();
    std::vector<std::string> split(const std::string& str, char delimiter);
}
