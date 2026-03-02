#pragma once

#include "types.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

enum FirstArgType {
    DEFINE,        // 宏
};

const std::unordered_map<std::string, FirstArgType> firstArgMap = {
    {"define",   FirstArgType::DEFINE},
};

// 位宽类型无序表
const std::unordered_set<std::string> bitsTypeSet = {
    "16",
    "32",
    "64"
};

class Preprocessor {
    public:
        Preprocessor(std::string src);

        void preprocess();

        std::string getPreprocessedSrc();

    private:
        void applyDefineMacro(uint32 lineIndex, const std::string& macroName, const std::string& macroBody);

    private:
        std::vector<std::string> m_lines;
};