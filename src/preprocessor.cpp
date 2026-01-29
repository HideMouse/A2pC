#include "preprocessor.hpp"

void clearSpace(std::string& str);

// public:

// 构造函数
Preprocessor::Preprocessor(std::string src) {
    std::string line;
    for (uint32 i = 0; i < src.size(); i++) {
        if (src[i] == '\n') {
            m_lines.push_back(line);
            line.clear();
        }
        line.push_back(src[i]);
    }
    m_lines.push_back(line);
}

// 预处理
PreprocessingInfo Preprocessor::preprocess() {
    PreprocessingInfo info = {
        .bits = "64"
    };

    uint32 lineIndex = 1;
    for (std::string& line : m_lines) {
        // 删除行首换行
        if (line[0] == '\n') {
            line.erase(0, 1);
        }

        // 删除行首多余空格
        clearSpace(line);

        // 删除注释
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos){
            line.erase(commentPos, line.size() - commentPos);
        }

        // 解析预处理行
        if (line[0] == '#') {
            std::string firstArg = line.substr(1, line.find_first_of(" \n") - 1);
            if (firstArgMap.contains(firstArg)) {
                // 清除预处理指令与多余空格
                line.erase(0, firstArg.size() + 1);
                clearSpace(line);

                switch (firstArgMap.at(firstArg)) {
                    case BITS: {
                        std::string bits = line.substr(0, line.find_first_of(" \0"));
                        if (!bitsTypeSet.contains(bits)) {
                            std::cerr << "At line:" << lineIndex << "\n  ";
                            std::cerr << "When: preprocessing\n  Error:\n    ";
                            std::cerr << "Unknown bit width \"" << bits << "\" or missing this argument.\a\n  ";
                            std::cerr << "Note:\n    #BITS takes only one argument for bit width, it must be 16, 32, or 64";
                            exit(-1);
                        }
                        info.bits = bits;
                        line.erase(0, bits.size());
                        if (line.find_last_not_of(" \0") != std::string::npos) {
                            std::cerr << "At line:" << lineIndex << "\n  ";
                            std::cerr << "When: preprocessing\n  Error:\n    ";
                            std::cerr << "Wrong number of arguments\a\n  ";
                            std::cerr << "Note:\n    #BITS takes only one argument for bit width, it must be 16, 32, or 64";
                            exit(-1);
                        }
                        break;
                    }
                }
            }
            else {
                std::cerr << "At line:" << lineIndex << "\n  ";
                std::cerr << "When: preprocessing\n  Error:\n    ";
                std::cerr << "Unknown preprocess command \"" << firstArg << "\"\a\n";
                exit(-1);
            }
            line.clear();
        }
        
        // 增加行号
        lineIndex++;
    }

    // 删除空行
    // m_lines.erase(std::remove(m_lines.begin(), m_lines.end(), ""), m_lines.end());

    return info;
}

std::string Preprocessor::getPreprocessedSrc() {
    std::string src;
    for (const std::string& line : m_lines) {
        src.append(line + "\n");
    }

    return src;
}

void clearSpace(std::string& str) {
    uint32 i = 0;
    while (std::isspace(str[i])) {
        i++;
    }
    str.erase(0, i);
}