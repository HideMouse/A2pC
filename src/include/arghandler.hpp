#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>

#include <filesystem>
namespace fs = std::filesystem;

// 版本信息宏
#define A2PC_VERSION   "A2pC v0.5.0 for Windows"
#define A2PC_COMPILER  "MinGW-W64 G++ 15.2.0"
#define A2PC_BUILD_ENV "Windows 11 25H2"

enum ArgType {
    HELP,        // 帮助
    VERSION,     // 版本
    SRC_PATH,    // 源路径
    OUPUT_FILE,  // 输出路径
};

const std::unordered_map<std::string, ArgType> ArgMap = {
    // 列出全部选项
    {"--help",     ArgType::HELP},
    {"-help",      ArgType::HELP},
    {"--h",        ArgType::HELP},
    {"-h",         ArgType::HELP},

    // 输出版本信息
    {"--version",  ArgType::VERSION},
    {"-version",   ArgType::VERSION},
    {"--v",        ArgType::VERSION},
    {"-v",         ArgType::VERSION},

    // 控制输出文件
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