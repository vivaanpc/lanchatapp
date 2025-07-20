#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace Utils {

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
   if (std::string::npos == first) {
    return str;
}

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::string toLower(const std::string& str{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return result;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.length() >= prefix.length(&&
           str.compare(0, prefix.length(), prefix== 0;
}

bool endsWith(const std::string& str, const std::string& suffix{
    return str.length(>= suffix.length() &&
           str.compare(str.length(- suffix.length(), suffix.length(), suffix== 0;
}

std::string getCurrentTimeString({
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace Utils