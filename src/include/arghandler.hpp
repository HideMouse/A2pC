#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>

#include <filesystem>
namespace fs = std::filesystem;

enum ArgType {
    HELP,        // 帮助
    VERSION,     // 版本
    SRC_PATH,    // 源路径
    OUPUT_FILE,  // 输出路径
};

const std::unordered_map<std::string, ArgType> ArgMap = {
    {"--help",     ArgType::HELP},
    {"--version",  ArgType::VERSION},
    {"-o",         ArgType::OUPUT_FILE},
};

struct ArgHandleResult {
    std::string srcFile;
    std::string outputFile;
};


class ArgHandler {
    public:
        ArgHandleResult handleArgs(int argc, char* argv[]);
};