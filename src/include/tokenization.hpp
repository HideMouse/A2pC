#pragma once

#include "types.hpp"

#include <cstring>
#include <vector>
#include <iostream>
#include <optional>
#include <unordered_map>

enum NumType {
    HEXwith0x,  // 带0x的十六进制
    HEXwithH,   // 带H/h的十六进制
    BINwith0b,  // 带0b的二进制
    BINwithB,   // 带B/b的二进制
    DEC         // 十进制
};

enum TokenType {
    byte,                 //8 bit 1Byte
    word,                 //16bit 2Byte
    dword,                //32bit 4Byte
    qword,                //64bit 8Byte

    reg,                  //寄存器修饰
    afc,                  //自动填充常量修饰

    go_to,                //前往
    cond_goto,            //条件前往
    to,                   //条件前往的后半段引导
    test,                 //test指令
    global,               //global标志

    semicolon,            //;
    left_paren,           //(
    right_paren,          //)
    left_brace,           //{
    right_brace,          //}
    left_bracket,         //[
    right_bracket,        //]
    left_angle_bracket,   //<
    right_angle_bracket,  //>
    equal,                //=
    colon,                //:
    asterisk,             //*
    comma,                //,

    add,                  //\+
    sub,                  //\-

    logic_and,            //&
    logic_or,             //|
    logic_xor,            //^
    logic_not,            //~

    imm,                  //整数立即数
    ident,                //标识符
};

const std::unordered_map<std::string, TokenType> tokenMap = {
    {"byte",   TokenType::byte},
    {"word",   TokenType::word},
    {"dword",  TokenType::dword},
    {"qword",  TokenType::qword},

    {"reg",    TokenType::reg},
    {"afc",    TokenType::afc},

    {"goto",   TokenType::go_to},
    {"if",     TokenType::cond_goto},
    {"to",     TokenType::to},
    {"test",   TokenType::test},
    {"global", TokenType::global},

    {";",      TokenType::semicolon},
    {"(",      TokenType::left_paren},
    {")",      TokenType::right_paren},
    {"{",      TokenType::left_brace},
    {"}",      TokenType::right_brace},
    {"[",      TokenType::left_bracket},
    {"]",      TokenType::right_bracket},
    {"<",      TokenType::left_angle_bracket},
    {">",      TokenType::right_angle_bracket},
    {"=",      TokenType::equal},
    {":",      TokenType::colon},
    {"*",      TokenType::asterisk},
    {",",      TokenType::comma},

    {"+",      TokenType::add},
    {"-",      TokenType::sub},

    {"&",      TokenType::logic_and},
    {"|",      TokenType::logic_or},
    {"^",      TokenType::logic_xor},
    {"~",      TokenType::logic_not},
};

struct Token {
    TokenType type;
    std::optional<std::string> value;
    size_t lineIndex;
};


class Tokenizer {
    public:
        Tokenizer(std::string src) : m_src(std::move(src)) {}

        std::vector<Token> tokenize();

    private:
        std::optional<char> peek(uint32 offset = 0) const;
        char consume();

    private:
        const std::string m_src;
        uint32 m_index = 0;
};