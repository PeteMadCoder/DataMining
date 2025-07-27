#pragma once
#include <string>

class Downloader {
public:
    static std::string download(const std::string& url);
};