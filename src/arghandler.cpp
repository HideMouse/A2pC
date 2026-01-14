#include "arghandler.hpp"

bool inline isFileExists(const std::string& path) {
    fs::path file = path;
    return fs::exists(file);
}

ArgHandleResult ArgHandler::handleArgs(int argc, char* argv[]) {
    ArgHandleResult result;
    std::optional<std::string> srcFile;
    std::optional<std::string> outFile;
    
    for (int i = 1; i < argc; i++) {
        // 转为std::string
        std::string arg = argv[i];

        // 是路径? -> 源文件
        if (!ArgMap.contains(arg)) {
            if (!isFileExists(arg)) {
                std::cerr << "未知的参数: " << arg << "\a\n";
                exit(-1);
            }
            else if (srcFile.has_value()) {
                std::cerr << "过多的源代码文件\n应为1个, 实为2个\a\n";
                exit(-1);
            }
            else {
                srcFile = arg;
                continue;
            }
        }

        // 正常参数:
        switch (ArgMap.at(arg)) {
            case ArgType::HELP:
                std::cout << "选项:\n";
                std::cout << "  --help     获取帮助\n";
                std::cout << "  --version  查看当前的A2pC版本号\n";
                std::cout << "  -o <file>  在<file>中填入输出文件\n";
                std::cout << "\n";
                std::cout << "有关更多信息, 请前往:\n";
                std::cout << "<https://github.com/HideMouse>\n";
                exit(0);
                break;
            case ArgType::VERSION:
                std::cout << "A2pC v0.2.5 on windows\n";
                exit(0);
                break;
            case ArgType::OUPUT_FILE:
                if (i + 1 < argc) {
                    if (outFile.has_value()) {
                        std::cerr << "过多的输出文件\n应为1个, 实为2个\a\n";
                        exit(-1);
                    }
                    else {
                        outFile = argv[i + 1];
                        i++;
                    }
                }
                else {
                    std::cerr << "-o后缺失文件路径\a\n";
                    exit(-1);
                }
                break;
        }
    }

    if (!srcFile.has_value()) {
        std::cerr << "未指定源文件路径\n编译终止\a\n";
        exit(-1);
    }
    result.srcFile = srcFile.value();

    if (!outFile.has_value()) {
        result.outputFile = "out.asm";
    }
    else {
        result.outputFile = outFile.value();
    }
    
    return result;
}