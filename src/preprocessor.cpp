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
        // 删除无意义行首换行
        if (line[0] == '\n') {
            line.erase(0, 1);
        }

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
                            std::cout << "line: " << lineIndex << "\n  未知的位宽大小或缺少该参数\"" << bits << "\".\n\a";
                            exit(-1);
                        }
                        info.bits = bits;
                        line.erase(0, bits.size());
                        if (line.find_last_not_of(" \0") != std::string::npos) {
                            std::cout << "line: " << lineIndex << "\n  错误的参数数量, 应为1.\n\a";
                            exit(-1);
                        }
                        break;
                    }
                    case EXTERN: {
                        while (true) {
                            std::string func = line.substr(0, line.find_first_of(", \0"));
                            info.extern_funcs.push_back(func);
                            line.erase(0, func.size());
                            clearSpace(line);
                            if (line[0] == '\0') {
                                break;
                            }
                            else if (line[0] == ',') {
                                line.erase(0, 1);
                                clearSpace(line);
                            }
                            else {
                                std::cout << "line: " << lineIndex << "\n  未知的符号.\n\a";
                                exit(-1);
                            }
                        }
                        break;
                    }
                }
            }
            else {
                std::cout << "line: " << lineIndex << "\n  未知的预处理指令\"" << firstArg << "\".\n\a";
                exit(-1);
            }
            line.clear();
        }

        // std::cout << "行号: " << lineIndex << ", 行内容:" << line << "\n";
        
        // 增加行号
        lineIndex++;
    }

    // 删除空行
    // m_lines.erase(std::remove(m_lines.begin(), m_lines.end(), ""), m_lines.end());

    // std::cout << "预处理后行数: " << m_lines.size() << "行\n\n";

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