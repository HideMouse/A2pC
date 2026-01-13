#pragma once

#include "types.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

enum FirstArgType {
    BITS,          // 位宽
    EXTERN,        // 包含列表
};

const std::unordered_map<std::string, FirstArgType> firstArgMap = {
    {"bits",     FirstArgType::BITS},
    {"BITS",     FirstArgType::BITS},
    {"extern",   FirstArgType::EXTERN},
    {"EXTERN",   FirstArgType::EXTERN},
};

// 位宽类型无序表
const std::unordered_set<std::string> bitsTypeSet = {
    "16",
    "32",
    "64"
};

struct PreprocessingInfo {
    std::string bits;
    std::vector<std::string> extern_funcs;
};

class Preprocessor {
    public:
        Preprocessor(std::string src);

        PreprocessingInfo preprocess();

        std::string getPreprocessedSrc();

    private:
        std::vector<std::string> m_lines;
};