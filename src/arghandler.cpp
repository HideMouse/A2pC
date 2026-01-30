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
                std::cerr << "When: handling options\nError:\n  ";
                std::cerr << "Unknown Option \"" << arg << "\"\a\n";
                exit(-1);
            }
            else if (srcFile.has_value()) {
                std::cerr << "When: handling options\nError:\n  ";
                std::cerr << "Too many source file\a\n";
                std::cerr << "Note:\n  There should be only one source file.";
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
                std::cout << "Options:\n";
                std::cout << "  --help      List all options (also -help/--h/-h)\n";
                std::cout << "  --version   Display A2pC version information (also -version/--v/-v)\n";
                std::cout << "  <file>      Read source code from <file>\n";
                std::cout << "  -o <file>   Write output code to <file> (defaults to out.asm)\n";
                std::cout << "\n";
                std::cout << "For more information, please see:\n";
                std::cout << "  <https://github.com/HideMouse>.\n";
                exit(0);
                break;
            case ArgType::VERSION:
                std::cout << "Version:\n  ";
                std::cout << A2PC_VERSION   << "\n";
                std::cout << "Compiler:\n  ";
                std::cout << A2PC_COMPILER  << "\n";
                std::cout << "Build Environment:\n  ";
                std::cout << A2PC_BUILD_ENV << "\n";
                exit(0);
                break;
            case ArgType::OUPUT_FILE:
                if (i + 1 < argc) {
                    if (outFile.has_value()) {
                        std::cerr << "When: handling options\nError:\n  ";
                        std::cerr << "Too many output file\a\n";
                        std::cerr << "Note:\n  There should be only one output file.";
                        exit(-1);
                    }
                    else {
                        outFile = argv[i + 1];
                        i++;
                    }
                }
                else {
                    std::cerr << "When: handling options\nError:\n  ";
                    std::cerr << "Missing the output file path after -o\a\n";
                    exit(-1);
                }
                break;
        }
    }

    if (!srcFile.has_value()) {
        std::cerr << "When: handling options\nError:\n  ";
        std::cerr << "Source file path not specified\a\n";
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