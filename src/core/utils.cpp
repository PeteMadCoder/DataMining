#include "utils.h"
#include <regex>
#include <filesystem>
#include <iostream>

std::string Utils::extractBaseDomain(const std::string& url) {
    std::smatch match;
    std::regex domain_regex(R"(^(https?:\/\/[^\/]+))");
    if (std::regex_search(url, match, domain_regex)) {
        return match[1];
    }
    return "";
}

std::string Utils::createSafeFilename(const std::string& url) {
    return std::regex_replace(url, std::regex("[:/]+"), "_");
}

bool Utils::createOutputDirectory(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::cerr << "Could not create directory '" << dir << "': " << ec.message() << std::endl;
        return false;
    }
    return true;
}

std::string Utils::resolveUrl(const std::string& base_url, const std::string& link) {
    if (link.empty()) return "";
    
    // Already absolute
    if (link.find("http://") == 0 || link.find("https://") == 0) {
        return link;
    }
    
    // Protocol-relative
    if (link[0] == '/' && link[1] == '/') {
        size_t pos = base_url.find(":");
        if (pos != std::string::npos) {
            return base_url.substr(0, pos) + ":" + link;
        }
    }
    
    // Absolute path
    if (link[0] == '/') {
        return base_url + link;
    }
    
    // Relative path
    std::string result = base_url;
    if (result.back() != '/') {
        result += "/";
    }
    return result + link;
}