#pragma once

#include "parser.hpp"
#include "preprocessor.hpp"

#include <sstream>

enum class FormatType {
    NONE,
    Win64,
    ELF64
};

class Generator {
    public:
        Generator(std::unique_ptr<Program>& root, PreprocessingInfo info) : m_root(std::move(root)), m_info(info) {
            global_funcs = m_root->global_funcs;
            extern_funcs = m_info.extern_funcs;
        }

        std::string generate() const;

    private:
        inline std::string addrBrackToStr(AddressingBrackets ab) const;
        inline std::string valueToStr(Value value) const;
        inline ValueType getValueType(Value value) const;

    private:
        const std::unique_ptr<Program> m_root;
        PreprocessingInfo m_info;

        std::vector<std::string> global_funcs;
        std::vector<std::string> extern_funcs;
};