#include "parser.hpp"

void errLine(Token token);

//public:

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    
    while (peek().has_value()) {
        if (peek().value().type == TokenType::go_to) {
            program.get()->statements.push_back(parseStmtGoto());
        }
        else if ((peek().value().type == TokenType::ident &&
                 peekAndCheck(1, TokenType::colon)) ||
                 peek().value().type == TokenType::global) {
            program.get()->statements.push_back(parseStmtLabel());
        }
        else if (peek().value().type == TokenType::ident ||
                 peek().value().type == TokenType::reg ||
                 peek().value().type == TokenType::left_bracket) {
            program.get()->statements.push_back(parseStmtAssi());
        }
        else if (peek().value().type == TokenType::test) {
            program.get()->statements.push_back(parseStmtTest());
        }
        else if (peek().value().type >= TokenType::byte && 
                 peek().value().type <= TokenType::qword) {
            program.get()->statements.push_back(parseStmtVarDef());
        }
        else {
            std::cout << "在行" << peek().value().lineIndex << "处出现无效Token !!!\a\n";
            exit(-1);
        }
    }
    
    program->global_funcs = global_funcs;

    return program;
}

std::unique_ptr<StmtAssi> Parser::parseStmtAssi() {
    //解析左值:
    Value lV;
    // 第一Token是啥?
    if (peekAndCheck(0, TokenType::ident)) {
        // 变量
        std::string varName = consume().value.value();
        if (!VarMap.contains(varName)) {
            errLine(m_tokens.at(m_index - 1));
            std::cerr << "变量" << varName << "尚未定义 !!!\a\n";
            exit(-1);
        }
        lV.type = ValueType::VAR;
        lV.var = VarMap.at(varName);
    }
    else if (peekAndCheck(0, TokenType::reg)) {
        // 寄存器
        lV.type = ValueType::REG;
        lV.reg = parseRegister();
        m_index++;
    }
    else if (peekAndCheck(0, TokenType::left_bracket)) {
        // 内存
        //  消耗[
        m_index++;
        lV.type = ValueType::MEM;
        lV.mem = parseAddressingBrackets();

        //  检查并消耗]
        if (!peekAndCheck(0, TokenType::right_bracket)) {
            errLine(peek().value());
            std::cerr << "期望的 ']' !!!\a\n";
            exit(-1);
        }
        m_index++;
    }
    else {
        errLine(peek().value());
        std::cerr << "无效的左值 !!!\a\n";
        exit(-1);
    }

    //解析赋值类型:
    AssiType type;
    std::string assiStr;
    while (assiStr.length() <= 3 && !AssiMap.contains(assiStr)) {
        TokenType tt = peek().value().type;
        if (!tokenToCharMap.contains(tt)) break;
        assiStr.push_back(tokenToCharMap.at(tt));
        m_index++;
    }
    if (!AssiMap.contains(assiStr)) {
        errLine(m_tokens.at(m_index - 1));
        std::cerr << "无效的赋值类型" << assiStr << " !!!\a\n";
        exit(-1);
    }
    type = AssiMap.at(assiStr);

    Value rV;
    uint8 size = 0;
    // 是不是INC或DEC
    if (type == AssiType::INC || type == AssiType::DEC) {
        // 是 -> 没有右值
        if (!peekAndCheck(0, TokenType::semicolon)) {
            errLine(peek().value());
            std::cerr << "递增与递减不需要右值 !!!\a\n";
            exit(-1);
        }
        m_index++;

        return std::make_unique<StmtAssi>(type, lV, lV, 0);
    }
    else {
        // 不是 -> 有右值
        //  处理地址尺寸
        if (peekAndCheck(0, TokenType::left_paren)) {
            // 消耗(
            m_index++;
            if (!peek().has_value() || !(peek().value().type < 4)) {
                errLine(peek().value());
                std::cerr << "括号内应有一个数据类型 !!!\a\n";
                exit(-1);
            }
            size = peek().value().type;
            m_index++;
            if (!peekAndCheck(0, TokenType::right_paren)) {
                errLine(peek().value());
                std::cerr << "期望的 ')' !!!\a\n";
                exit(-1);
            }
            m_index++;
        }
        
        //  解析右值:
        //   第一个Token是啥?
        if (peekAndCheck(0, TokenType::ident)) {
            // 变量
            std::string varName = consume().value.value();
            if (!VarMap.contains(varName)) {
                errLine(m_tokens.at(m_index - 1));
                std::cerr << "变量" << varName << "尚未定义 !!!\a\n";
                exit(-1);
            }
            rV.type = ValueType::VAR;
            rV.var = VarMap.at(varName);
        }
        else if (peekAndCheck(0, TokenType::reg)) {
            // 寄存器
            rV.type = ValueType::REG;
            rV.reg = parseRegister();
            m_index++;
        }
        else if (peekAndCheck(0, TokenType::left_bracket)) {
            // 内存
            //  消耗[
            m_index++;
            rV.type = ValueType::MEM;
            rV.mem = parseAddressingBrackets();

            //  检查并消耗]
            if (!peekAndCheck(0, TokenType::right_bracket)) {
                errLine(peek().value());
                std::cerr << "期望的 ']' !!!\a\n";
                exit(-1);
            }
            m_index++;
        }
        else if (peekAndCheck(0, TokenType::imm)) {
            // 立即数
            rV.type = ValueType::IMM;
            rV.imm = consume().value.value();
        }
        else {
            errLine(peek().value());
            std::cerr << "无效的右值 !!!\a\n";
            exit(-1);
        }
    }

    // 检查并消耗;
    if (!peekAndCheck(0, TokenType::semicolon)) {
        errLine(peek().value());
        std::cerr << "期望的 ';' !!!\a\n";
        exit(-1);
    }
    m_index++;

    return std::make_unique<StmtAssi>(type, lV, rV, size);
}

std::unique_ptr<StmtGoto> Parser::parseStmtGoto() {
    // 消耗goto
    m_index++;

    // 解析寄存器/数字/标签名
    std::string value;
    if (peekAndCheck(0, TokenType::ident) || peekAndCheck(0, TokenType::imm)) {
        value = consume().value.value();
    }
    else if (peekAndCheck(0, TokenType::reg)) {
        value = parseRegister().name;
        m_index++;
    }
    else {
        std::cerr << "在goto后缺失寄存器/数字/标签名 !!!\a\n";
        exit(-1);
    }

    // 检查分号
    if (!peekAndCheck(0, TokenType::semicolon)) {
        std::cerr << "期望的 ';' !!!\a\n";
        exit(-1);
    }
    m_index++;

    return std::make_unique<StmtGoto>(value);
}

std::unique_ptr<StmtLabel> Parser::parseStmtLabel() {
    std::string label;
    // 标记全局
    if (peekAndCheck(0, TokenType::global)) {
        if (peekAndCheck(1, TokenType::ident)) {
            label = peek(1).value().value.value();
            global_funcs.push_back(label);
            m_index += 2;
        }
        else {
            std::cerr << "global 后缺失标签名 !!!\a\n";
            exit(-1);
        }
    }
    else {
        label = peek().value().value.value();
        m_index++;
    }

    if (!peekAndCheck(0, TokenType::colon)) {
        std::cerr << "期望的 ':' !!!\a\n";
        exit(-1);
    }
    m_index++;

    return std::make_unique<StmtLabel>(label);
}

std::unique_ptr<StmtTest> Parser::parseStmtTest() {
    // 消耗text
    m_index++;

    // 左括号
    if (!peekAndCheck(0, TokenType::left_paren)) {
        std::cerr << "期望的 '(' !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 左操作数
    std::string left;
    if (peekAndCheck(0, TokenType::reg)) {
        left = parseRegister().name;
    }
    else {
        std::cerr << "需要一个寄存器作为左操作数 !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 逗号
    if (!peekAndCheck(0, TokenType::comma)) {
        std::cerr << "期望的 ',' !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 右操作数
    std::string right;
    if (peekAndCheck(0, TokenType::imm)) {
        right = peek().value().value.value();
    }
    else if (peekAndCheck(0, TokenType::reg)) {
        right = parseRegister().name;
    }
    else {
        std::cerr << "需要一个寄存器或数值作为右操作数 !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 逗号
    if (!peekAndCheck(0, TokenType::comma)) {
        std::cerr << "期望的 ',' !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 跳转类型(Jcc)
    std::string t;
    if (peekAndCheck(0, TokenType::ident)) {
        t = peek().value().value.value();
        if (!JumpTypeSet.contains(t)) {
            std::cerr << "未知的 JumpType!!!\a\n";
            exit(-1);
        }
    }
    else {
        std::cerr << "在第二个 ',' 后缺失 JumpType !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 括号
    if (!peekAndCheck(0, TokenType::right_paren)) {
        std::cerr << "期望的 ')' !!!\a\n";
        exit(-1);
    }
    m_index++;

    // to
    if (!peekAndCheck(0, TokenType::to)) {
        std::cerr << "期望的 \"to\" !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 解析数字/标签名
    std::string value;
    if (peekAndCheck(0, TokenType::ident) || peekAndCheck(0, TokenType::imm)) {
        value = consume().value.value();
    }
    else {
        std::cerr << "在to后缺失数字/标签名 !!!\a\n";
        exit(-1);
    }
    
    // 分号
    if (!peekAndCheck(0, TokenType::semicolon)) {
        std::cerr << "期望的 ';' !!!\a\n";
        exit(-1);
    }
    m_index++;

    return std::make_unique<StmtTest>(left, right, value, t);
}

std::unique_ptr<StmtVarDef> Parser::parseStmtVarDef() {
    Var var;
    // 设置变量大小并消耗
    var.size = consume().type - TokenType::byte;

    // 获取变量名
    if (!peekAndCheck(0, TokenType::ident)) {
        std::cerr << "应有变量名 !!!\a\n";
        exit(-1);
    }
    var.name = consume().value.value();
    if (VarMap.contains(var.name)) {
        std::cerr << "变量" << var.name << "已被声明 !!!\a\n";
        exit(-1);
    }
    

    // 左括号
    if (!peekAndCheck(0, TokenType::left_paren)) {
        std::cerr << "期望的 '(' !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 判断储存位置
    if (peekAndCheck(0, TokenType::imm)) {
        // 内存
        var.loc.isReg = false;
        var.loc.memAddr = consume().value.value();
    }
    else if (peekAndCheck(0, TokenType::reg)) {
        // 寄存器
        var.loc.isReg = true;
        var.loc.reg = parseRegister();
        m_index++;
        if (var.loc.reg.size < var.size) {
            std::cerr << "寄存器" << var.loc.reg.name << "无法容纳变量" << var.name << ", 因为其位数过小 !!!\a\n";
            exit(-1);
        }
        else if (var.loc.reg.size > var.size) {
            std::cout << "警告:\n寄存器" << var.loc.reg.name << "与变量" << var.name << "的大小不同\n";
        }
    }
    else {
        std::cerr << "变量" << var.name << "的储存位置无效 !!!\a\n";
        exit(-1);
    }
    
    // 右括号
    if (!peekAndCheck(0, TokenType::right_paren)) {
        std::cerr << "期望的 ')' !!!\a\n";
        exit(-1);
    }
    m_index++;

    // 向VarMap插入变量
    VarMap.insert({var.name, var});

    // 结束或设置初始值
    if (peekAndCheck(0, TokenType::semicolon)) {
        // 结束行
        m_index++;
        return std::make_unique<StmtVarDef>(var, std::nullopt);
    }
    else if (peekAndCheck(0, TokenType::equal)) {
        // 设置初始值
        m_index++;

        // 粗略处理赋值
        if (!peekAndCheck(0, TokenType::imm)) {
            std::cerr << "对变量" << var.name << "进行了无效的赋值 !!!\a\n";
            exit(-1);
        }
        std::string value = consume().value.value();

        // 分号
        if (!peekAndCheck(0, TokenType::semicolon)) {
            std::cerr << "期望的 ';' !!!\a\n";
            exit(-1);
        }
        m_index++;

        return std::make_unique<StmtVarDef>(var, value);
    }
    else {
        std::cerr << "变量" << var.name << "在声明后既不结束行也不进行赋值 !!!\a\n";
        exit(-1);
    }
}

//private:

// 预览
std::optional<Token> Parser::peek(uint32 offset) const{
    if (m_index + offset >= m_tokens.size()) {
        return std::nullopt;
    }
    return m_tokens.at(m_index + offset);
}

// 预览并检查
inline bool Parser::peekAndCheck(uint32 offset, TokenType type) const {
    return peek(offset).has_value() && peek(offset).value().type == type;
}

// 消耗
Token Parser::consume() {
    return m_tokens.at(m_index++);
}

// 解析寄存器修饰
Register Parser::parseRegister() {
    // 消耗reg
    m_index++;

    // 检查::并消耗
    if (!peekAndCheck(0, TokenType::colon) || !peekAndCheck(1, TokenType::colon)) {
        std::cerr << "需要 \"::\" !\a\n";
        exit(-1);
    }
    m_index += 2;

    // 检查寄存器名
    if (!peekAndCheck(0, TokenType::ident)) {
        std::cerr << "需要寄存器名!\a\n";
        exit(-1);
    }
    if (!RegMap.contains(peek().value().value.value())) {
        std::cerr << "\"reg::\" 修饰不包含这个寄存器!\a\n";
        exit(-1);
    }

    return RegMap.at(peek().value().value.value());
}

// 解析寻址括号
AddressingBrackets Parser::parseAddressingBrackets() {
    AddressingBrackets ab;

    if (peekAndCheck(0, TokenType::reg)) {
        //reg
        ab.base = parseRegister().name;
        m_index++;

        if(!peekAndCheck(0, TokenType::add) && !peekAndCheck(0, TokenType::sub) && !peekAndCheck(0, TokenType::right_bracket)) {
            //不是+ 不是- 不是]
            std::cerr << "无效的寻址括号!!!\n在\"reg\"后出现未知内容\a\n";
            exit(-1);
        }
        else if (!peekAndCheck(0, TokenType::right_bracket)){
            m_index++;
            if (peekAndCheck(0, TokenType::reg)) {
                //reg + reg
                ab.index = parseRegister().name;
                m_index++;

                if(!peekAndCheck(0, TokenType::add) && !peekAndCheck(0, TokenType::sub) && !peekAndCheck(0, TokenType::asterisk) && !peekAndCheck(1, TokenType::right_bracket)) {
                    std::cerr << "无效的寻址括号!!!\n在\"reg + reg\"后出现未知内容\a\n";
                    exit(-1);
                }
                else if (peekAndCheck(1, TokenType::imm)) {
                    if (peekAndCheck(0, TokenType::add)) {
                        //reg + reg + imm
                        ab.displacement = "+" + peek(1).value().value.value();
                    }
                    else if (peekAndCheck(0, TokenType::sub)) {
                        //reg + reg - imm
                        ab.displacement = "-" + peek(1).value().value.value();
                    }
                    else if (peekAndCheck(0, TokenType::asterisk)) {
                        //reg + reg * imm
                        ab.scale = peek(1).value().value.value();
                        if (ab.scale != "1" && ab.scale != "2" && ab.scale != "4" && ab.scale != "8") {
                            std::cerr << "无效的比例因子!!!\n应为1,2,4,8中一个\a\n";
                            exit(-1);
                        }
                    }
                    m_index += 2;
                    if(!peekAndCheck(0, TokenType::add) && !peekAndCheck(0, TokenType::sub) && !peekAndCheck(0, TokenType::right_bracket)) {
                        std::cerr << "无效的寻址括号!!!\n在\"reg + reg * imm\"后出现未知内容\a\n";
                        exit(-1);
                    }
                    
                    if (peekAndCheck(1, TokenType::imm)) {
                        if (peekAndCheck(0, TokenType::add)) {
                            //reg + reg * imm + imm
                            ab.displacement = "+" + peek(1).value().value.value();
                        }
                        else if (peekAndCheck(0, TokenType::sub)) {
                            //reg + reg * imm - imm
                            ab.displacement = "-" + peek(1).value().value.value();
                        }
                        m_index += 2;
                    }
                    else if (!peekAndCheck(0, TokenType::right_bracket)) {
                        std::cerr << "无效的寻址括号!!!\n\"reg + reg * imm +/- imm\"已经是完整格式\a\n";
                        exit(-1);
                    }
                }
                else {
                    std::cerr << "无效的寻址括号!!!\n在\"reg + reg +/-/*\"后出现未知内容\a\n";
                    exit(-1);
                }
            }
            else if (peekAndCheck(0, TokenType::imm)) {
                if (peekAndCheck(-1, TokenType::add)) {
                    //reg + imm
                    ab.displacement = "+" + peek().value().value.value();
                }
                else if (peekAndCheck(-1, TokenType::sub)) {
                    //reg - imm
                    ab.displacement = "-" + peek().value().value.value();
                }
                m_index++;
                if (!peekAndCheck(0, TokenType::right_bracket)) {
                    std::cerr << "无效的寻址括号!!!\n\"reg +/- imm\"已经是完整格式\a\n";
                    exit(-1);
                }
            }
            else {
                std::cerr << "无效的寻址括号!!!\n在\"reg +/-\"后出现未知内容\a\n";
                exit(-1);
            }
        }
    }
    else if (peekAndCheck(0, TokenType::imm)) {
        //imm
        ab.displacement = consume().value.value();
        ab.isNumber = true;
        if (!peekAndCheck(0, TokenType::right_bracket)) {
            std::cerr << "无效的寻址括号!!!\n\"imm\"已经是完整格式\a\n";
            exit(-1);
        }
    }
    else {
        std::cerr << "无效的寻址括号!!!\n在'['后出现未知内容\a\n";
        exit(-1);
    }

    return ab;
}

void errLine(Token token) {
    std::cerr << "line: " << token.lineIndex << "\n  ";
}