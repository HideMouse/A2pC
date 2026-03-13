#include "preprocessor.hpp"

void clearFrontSpace(std::string& str);
void clearBackSpace(std::string& str);

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
void Preprocessor::preprocess() {
    uint32 lineIndex = 1;
    for (std::string& line : m_lines) {
        // 删除行首换行
        if (line[0] == '\n') {
            line.erase(0, 1);
        }

        // 删除行首多余空格
        clearFrontSpace(line);

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
                clearFrontSpace(line);

                switch (firstArgMap.at(firstArg)) {
                    case DEFINE: {
                        if (line.find_first_of(" ") == std::string::npos) {
                            std::cerr << "At line:" << lineIndex << "\n  ";
                            std::cerr << "When: preprocessing\n  Error:\n    ";
                            std::cerr << "Missing a Macro Body\a\n  ";
                            exit(-1);
                        }
                        std::string macroName = line.substr(0, line.find_first_of(" "));
                        
                        line.erase(0, macroName.size());
                        clearFrontSpace(line);
                        clearBackSpace(line);

                        std::string macroBody = line;

                        if (macroBody.find(macroName) != std::string::npos) {
                            std::cerr << "At line:" << lineIndex << "\n  ";
                            std::cerr << "When: preprocessing\n  Warning:\n    ";
                            std::cerr << "MacroBody contains MacroName\a\n";
                        }
                        else {
                            applyDefineMacro(lineIndex, macroName, macroBody);
                        }
                        
                        break;
                    }
                }
            }
            else {
                std::cerr << "At line:" << lineIndex << "\n  ";
                std::cerr << "When: preprocessing\n  Error:\n    ";
                std::cerr << "Unknown preprocess command (\"" << firstArg << "\")\a\n";
                exit(-1);
            }

            line.clear();
        }
        
        // 增加行号
        lineIndex++;
    }

    // 删除空行
    // m_lines.erase(std::remove(m_lines.begin(), m_lines.end(), ""), m_lines.end());
}

std::string Preprocessor::getPreprocessedSrc() {
    std::string src;
    for (const std::string& line : m_lines) {
        src.append(line + "\n");
    }

    return src;
}

void clearFrontSpace(std::string& str) {
    uint32 i = 0;
    while (std::isspace(str[i])) {
        i++;
    }
    str.erase(0, i);
}

void clearBackSpace(std::string& str) {
    uint32 i = str.find_last_of(" ");
    if (i == std::string::npos ||
        i != str.size() - 1
    ) {
        //std::cout << "no space!!!\n";
        return;
    }

    if (str.find_first_not_of(" \0") == std::string::npos) {
        //std::cout << "all space!!!\n";
        str.clear();
        return;
    }

    i = str.size();
    uint32 count = 0;

    while (std::isspace(str[i - 1])) {
        i--;
        count++;
    }
    str.erase(i, count);
}

void Preprocessor::applyDefineMacro(uint32 lineIndex, const std::string& macroName, const std::string& macroBody) {
    uint32 lI = 0;
    for (std::string& line : m_lines) {
        lI++;
        
        if (lI <= lineIndex) {
            continue;
        }

        while (line.find(macroName) != std::string::npos) {
            line.replace(line.find(macroName), macroBody.size(), macroBody);
        }
    }
}