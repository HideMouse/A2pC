#include "tokenization.hpp"


bool isNumberChar(char c);
bool isAvailableNumber(const std::string& num);

//public:

// 分词
std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    std::string buffer;

    size_t lineIndex = 1;

    while (peek().has_value()) {
        if (peek().value() == '\n') {
            lineIndex++;
        }

        if (std::isalpha(peek().value())) {
            while (peek().has_value() && std::isalnum(peek().value())) {
                buffer.push_back(consume());
            }

            if (tokenMap.contains(buffer)) {
                tokens.push_back({ .type = tokenMap.at(buffer), .lineIndex = lineIndex });
                buffer.clear();
                continue;
            }
            else {
                tokens.push_back({ .type = TokenType::ident, .value = buffer, .lineIndex = lineIndex });
                buffer.clear();
                continue;
            }
        }
        else if (std::isdigit(peek().value())) {
            while (peek().has_value() && isNumberChar(peek().value())) {
                buffer.push_back(consume());
            }

            if (!isAvailableNumber(buffer)) {
                std::cerr << buffer << "并不是数字!\a\n";
                exit(-1);
            }

            tokens.push_back({ .type = TokenType::imm, .value = buffer, .lineIndex = lineIndex });
            buffer.clear();
            continue;
        }
        else if (tokenMap.contains(std::string(1, peek().value()))) {
            tokens.push_back({ .type = tokenMap.at(std::string(1, peek().value())), .lineIndex = lineIndex });
            m_index++;
            continue;
        }
        else if (std::isspace(peek().value())) {
            m_index++;
            continue;
        }
        else {
            std::cerr << "意外的标记!\a\n";
            exit(-1);
        }
    }

    m_index = 0;
    return tokens;
}


//private:

// 预览
std::optional<char> Tokenizer::peek(uint32 offset) const{
    if (m_index + offset >= m_src.length()) {
        return std::nullopt;
    }
    return m_src.at(m_index + offset);
}

// 消耗
char Tokenizer::consume() {
    return m_src.at(m_index++);
}



// 是否为数字字符
bool isNumberChar(char c) {
    return  'A' <= c && c <= 'F' ||
            'a' <= c && c <= 'f' ||
            '0' <= c && c <= '9' ||
            c == 'H' || c == 'h' ||
            c == 'x';
}

// 是否为可用数字
bool isAvailableNumber(const std::string& num) {
    NumType type;
    
    if (num.find("0x") == 0)                               type = HEXwith0x;
    else if (num.find_first_of("Hh") == (num.size() - 1))  type = HEXwithH;
    else if (num.find("0b") == 0)                          type = BINwith0b;
    else if (num.find_first_of("Bb") == (num.size() - 1))  type = BINwithB;
    else                                                   type = DEC;
    
    switch (type) {
        case HEXwith0x:
            /*
            1. 与H后缀混用  (不能有H/h)
            2. 不止一个x    (不能有x)
            */
            if (num.substr(2, num.size() - 2).find_first_of("Hhx") != std::string::npos) return false;
            break;
        case HEXwithH:
            /*
            1. 与0x混用     (不能有x)
            2. 不止一个H/h  (不能有H/h)
            */
            if (num.substr(0, num.size() - 1).find_first_of("Hhx") != std::string::npos) return false;
            break;
        case BINwith0b:
            /*
            1. 不符合BIN    (不能有23456789ABCDEFabcdefHhx)
            */
            if (num.substr(2, num.size() - 2).find_first_of("23456789ABCDEFabcdefHhx") != std::string::npos) return false;
            break;
        case BINwithB:
            /*
            1. 与0x混用     (不能有x)
            2. 与0b混用     (不能有b)
            3. 不符合BIN    (不能有23456789ABCDEFabcdefHhx)
            */
            if (num.substr(0, num.size() - 1).find_first_of("23456789ABCDEFabcdefHhx") != std::string::npos) return false;
            break;
        case DEC:
            if (num.find_first_of("ABCDEFabcdefHhx") != std::string::npos) return false;
    }
    
    return true;
}