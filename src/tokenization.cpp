#include "tokenization.hpp"

bool isNumberChar(char c);
bool isIdentFirstChar(char c);
bool isIdentChar(char c);

void clearZero(std::string& num);

// 数字处理:
NumType getNumberType(const std::string& num);
bool isAvailableNumber(const std::string& num);
std::optional<uint64> numStrToInt(const std::string& num);

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

        if (isIdentFirstChar(peek().value())) {
            buffer.push_back(consume());

            while (peek().has_value() && isIdentChar(peek().value())) {
                buffer.push_back(consume());
            }

            if (tokenMap.contains(buffer)) {
                tokens.push_back({ .type = tokenMap.at(buffer), .lineIndex = lineIndex });
            }
            else {
                tokens.push_back({ .type = TokenType::ident, .value = buffer, .lineIndex = lineIndex });
            }

            buffer.clear();
            continue;
        }
        else if (std::isdigit(peek().value())) {
            while (peek().has_value() && isNumberChar(peek().value())) {
                buffer.push_back(consume());
            }

            // 不是有效数字
            if (!isAvailableNumber(buffer)) {
                std::cerr << "At line:" << lineIndex << "\n  ";
                std::cerr << "When: tokenizing\n  Error:\n    \"";
                std::cerr << buffer << "\" is not a number\a\n";
                exit(-1);
            }
            // 对uint64溢出
            if (!numStrToInt(buffer).has_value()) {
                std::cerr << "At line:" << lineIndex << "\n  ";
                std::cerr << "When: tokenizing\n  Error:\n    \"";
                std::cerr << buffer << "\" is overflow qword (Number Too Big)\a\n";
                exit(-1);
            }

            tokens.push_back({ .type = TokenType::imm, .value = buffer, .lineIndex = lineIndex });
            
            buffer.clear();
            continue;
        }
        else if (peek().value() == '@') {
            // 提前消耗一个, 避免检测到初始的@
            buffer.push_back(consume());

            while (peek().has_value() && peek().value() != '@') {
                if (peek().value() == '\n') {
                    lineIndex++;
                }
                buffer.push_back(consume());
            }

            if (!peek().has_value()) {
                std::cerr << "At line:" << lineIndex << "\n  ";
                std::cerr << "When: tokenizing\n  Error:\n    ";
                std::cerr << "Unclosed '@' on inline assembly code.\a\n";
                exit(-1);
            }

            m_index++;

            tokens.push_back({ .type = TokenType::assembly, .value = buffer.substr(1, buffer.size() - 1), .lineIndex = lineIndex });
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
            std::cerr << "At line:" << lineIndex << "\n  ";
            std::cerr << "When: tokenizing\n  Error:\n    ";
            std::cerr << "Unexpected Token\a\n";
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

// 是否为标识符首字符
bool isIdentFirstChar(char c) {
    return  ('A' <= c && c <= 'Z') ||
            ('a' <= c && c <= 'z') ||
            c == '_' || c == '.';
}

// 是否为标识符字符
bool isIdentChar(char c) {
    return  ('A' <= c && c <= 'Z') ||
            ('a' <= c && c <= 'z') ||
            ('0' <= c && c <= '9') ||
            c == '_';
}

// 删除0
void clearZero(std::string& num) {
    if (num.find_first_not_of("0\0") == std::string::npos) {
        num.erase(0, num.size() - 1);
        return;
    }

    uint32 i = 0;
    while (num[i] == '0') {
        i++;
    }
    num.erase(0, i);
}

// 获取数字类型
NumType getNumberType(const std::string& num) {
    if (num.find("0x") == 0)                               return HEXwith0x;
    else if (num.find_first_of("Hh") == (num.size() - 1))  return HEXwithH;
    else if (num.find("0b") == 0)                          return BINwith0b;
    else if (num.find_first_of("Bb") == (num.size() - 1))  return BINwithB;
    else                                                   return DECwithNone;
}

// 是否为可用数字
bool isAvailableNumber(const std::string& num) {
    NumType type = getNumberType(num);
    
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
        case DECwithNone:
            if (num.find_first_of("ABCDEFabcdefHhx") != std::string::npos) return false;
    }
    
    return true;
}

// 数字字符串转十进整数
std::optional<uint64> numStrToInt(const std::string& num) {
    NumType type = getNumberType(num);

    // 去除后缀后的数字字符串
    std::string nonFixNum;

    // 整数值
    uint64 intV = 0;
    
    switch (type) {
        case HEXwith0x: {
            nonFixNum = num.substr(2, num.size() - 2);
            clearZero(nonFixNum);

            if (nonFixNum.size() > 16) {
                return std::nullopt;
            }

            intV = std::stoull(nonFixNum, nullptr, 16);

            break;
        }
        case HEXwithH: {
            nonFixNum = num.substr(0, num.size() - 1);
            clearZero(nonFixNum);

            if (nonFixNum.size() > 16) {
                return std::nullopt;
            }

            intV = std::stoull(nonFixNum, nullptr, 16);

            break;
        }
        case BINwith0b: {
            nonFixNum = num.substr(2, num.size() - 2);
            clearZero(nonFixNum);
            
            if (nonFixNum.size() > 64) {
                return std::nullopt;
            }

            intV = std::stoull(nonFixNum, nullptr, 2);

            break;
        }
        case BINwithB: {
            nonFixNum = num.substr(0, num.size() - 1);
            clearZero(nonFixNum);
            
            if (nonFixNum.size() > 64) {
                return std::nullopt;
            }

            intV = std::stoull(nonFixNum, nullptr, 2);

            break;
        }
        case DECwithNone: {
            nonFixNum = num;
            clearZero(nonFixNum);

            if (nonFixNum.size() > 20) {
                return std::nullopt;
            }
            else if (nonFixNum.size() == 20 &&
                     nonFixNum.compare("18446744073709551615") > 0
            ) {
                return std::nullopt;
            }

            intV = std::stoull(nonFixNum, nullptr, 10);

            break;
        }
    }

    return intV;
}