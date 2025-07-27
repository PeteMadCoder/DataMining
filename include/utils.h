#pragma once
#include <string>

class Utils {
public:
    static std::string extractBaseDomain(const std::string& url);
    static std::string createSafeFilename(const std::string& url);
    static bool createOutputDirectory(const std::string& dir);
    static std::string resolveUrl(const std::string& base_url, const std::string& link);
};