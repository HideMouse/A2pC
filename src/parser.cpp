#include "parser.hpp"

void errLine(Token token);

//public:

ParseInfo Parser::getParseInfo() {
    ParseInfo info;
    info.varUsedMap = VarUsedMap;

    return info;
}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    
    while (peek().has_value()) {
        if (peek().value().type == TokenType::go_to) {
            program.get()->statements.push_back(parseStmtGoto());
        }
        else if ((peek().value().type == TokenType::extern_ &&
                 peekAndCheck(1, TokenType::ident))) {
            parseExtern();
        }
        else if ((peek().value().type == TokenType::ident &&
                 peekAndCheck(1, TokenType::colon)) ||
                 peek().value().type == TokenType::global) {
            program.get()->statements.push_back(parseStmtLabel());
        }
        else if ((peek().value().type == TokenType::section &&
                 peekAndCheck(1, TokenType::ident))) {
            program.get()->statements.push_back(parseStmtSectionDef());
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
        else if (peek().value().type == TokenType::ilasm) {
            program.get()->statements.push_back(parseStmtInlineAsm());
        }
        else {
            errLine(peek().value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Invalid Token\a\n";
            exit(-1);
        }
    }
    
    program->global_funcs = global_funcs;
    program->extern_funcs = extern_funcs;

    return program;
}

std::unique_ptr<StmtAssi> Parser::parseStmtAssi() {
    //解析左值:
    Value lV = parseValue();
    if (getValueType(lV) == IMM) {
        errLine(peek(getPeekOffset() - 1).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Invalid left value\a\n  ";
        std::cerr << "Note:\n    It cannot be a number";
        exit(-1);
    }

    //解析赋值类型:
    AssiType type;
    std::string assiStr;
    while (assiStr.length() <= 3 && !AssiMap.contains(assiStr)) {
        if (!peek().has_value()) {
            errLine(peek(-1).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Parsed an invalid assignment type because of no value after here\a\n";
            exit(-1);
        }

        TokenType tt = peek().value().type;
        if (!tokenToCharMap.contains(tt)) break;
        assiStr.push_back(tokenToCharMap.at(tt));
        m_index++;
    }
    if (!AssiMap.contains(assiStr)) {
        errLine(m_tokens.at(m_index - 1));
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Invalid assignment type: \"" << assiStr << "\"\a\n  ";
        std::cerr << "Note:\n    Assignment type can be: = / += / -= / ++ / -- / &= / |= / ^= / <<= / >>=";
        exit(-1);
    }
    type = AssiMap.at(assiStr);

    Value rV;
    uint8 size = 0;
    // 是不是INC或DEC
    if (type == AssiType::INC || type == AssiType::DEC) {
        // 是 -> 没有右值
        if (!peekAndCheck(0, TokenType::semicolon)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "++/-- is no need right value\a\n  ";
            std::cerr << "Note:\n    ++/-- use implicit right value";
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
                errLine(peek(getPeekOffset()).value());
                std::cerr << "When: parsing\n  Error:\n    ";
                std::cerr << "Missing a address size inside the parentheses\a\n";
                exit(-1);
            }
            size = peek().value().type;
            m_index++;
            if (!peekAndCheck(0, TokenType::right_paren)) {
                errLine(peek(getPeekOffset()).value());
                std::cerr << "When: parsing\n  Error:\n    ";
                std::cerr << "Missing a ')'\a\n";
                exit(-1);
            }
            m_index++;
        }
        
        //  解析右值:
        rV = parseValue();
        // 检查右值
        if ((type == AssiType::SHL || type == AssiType::SHR) &&
             getValueType(rV) != IMM) {
            // 为左右移 -> 右值不得为数值
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Non-number value for <<=/>>= right value\a\n  ";
            std::cerr << "Note:\n    <<=/>>= right value can only be number.";
            exit(-1);
        }
        if (getValueType(lV) == MEM && getValueType(rV) == MEM) {
            // 左右值不得同时为内存
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Left value and right value are both memory\a\n  ";
            std::cerr << "Note:\n    Value on two side cannot both be memory.";
            exit(-1);
        }
    }

    // 检查并消耗;
    if (!peekAndCheck(0, TokenType::semicolon)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ';'\a\n";
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
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a register/number/label after \"goto\"\a\n";
        exit(-1);
    }

    // 检查分号
    if (!peekAndCheck(0, TokenType::semicolon)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ';'\a\n";
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
            errLine(peek().value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Missing a label name after \"global\"\a\n";
            exit(-1);
        }
    }
    else {
        label = peek().value().value.value();
        m_index++;
    }

    if (!peekAndCheck(0, TokenType::colon)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ':'\a\n";
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
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a '('\a\n";
        exit(-1);
    }
    m_index++;

    // 左操作数
    Value left = parseValue();
    if (getValueType(left) == IMM) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Invalid left operand\a\n  ";
        std::cerr << "Note:\n    It cannot be a number";
        exit(-1);
    }

    // 逗号
    if (!peekAndCheck(0, TokenType::comma)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ','\a\n";
        exit(-1);
    }
    m_index++;

    // 右操作数
    Value right = parseValue();
    if (getValueType(right) == MEM) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Invalid right operand\a\n  ";
        std::cerr << "Note:\n    It cannot be a memory address";
        exit(-1);
    }

    // 逗号
    if (!peekAndCheck(0, TokenType::comma)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ','\a\n";
        exit(-1);
    }
    m_index++;

    // 跳转类型(Jcc)
    std::string t;
    if (peekAndCheck(0, TokenType::ident)) {
        t = peek().value().value.value();
        if (!JumpTypeSet.contains(t)) {
            errLine(peek().value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Unknown JumpType\a\n";
            exit(-1);
        }
    }
    else {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a JumpType after the second ','\a\n";
        exit(-1);
    }
    m_index++;

    // 右括号
    if (!peekAndCheck(0, TokenType::right_paren)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ')'\a\n";
        exit(-1);
    }
    m_index++;

    // to
    if (!peekAndCheck(0, TokenType::to)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a \"to\"\a\n";
        exit(-1);
    }
    m_index++;

    // 解析数字/标签名
    std::string value;
    if (peekAndCheck(0, TokenType::ident) || peekAndCheck(0, TokenType::imm)) {
        value = consume().value.value();
    }
    else {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a number/label after \"to\"\a\n";
        exit(-1);
    }
    
    // 分号
    if (!peekAndCheck(0, TokenType::semicolon)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ';'\a\n";
        exit(-1);
    }
    m_index++;

    return std::make_unique<StmtTest>(left, right, value, t);
}

std::unique_ptr<StmtVarDef> Parser::parseStmtVarDef() {
    Var var;

    //设置变量大小并消耗:
    var.size = consume().type - TokenType::byte;

    //获取修饰:
    VarQualifier vq;
    vq.type = VarQualifierType::NONE;
    if (peek().has_value() && VarQualTypeMap.contains(peek().value().type)) {
        vq.type = VarQualTypeMap.at(consume().type);

        // 检查::并消耗
        if (!peekAndCheck(0, TokenType::colon) || !peekAndCheck(1, TokenType::colon)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Missing a \"::\"\a\n";
            exit(-1);
        }
        m_index += 2;
    }

    //获取变量名:
    if (!peekAndCheck(0, TokenType::ident)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a variable name\a\n";
        exit(-1);
    }
    var.name = consume().value.value();
    if (VarMap.contains(var.name)) {
        errLine(peek().value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Variable " << var.name << " has been declared\a\n";
        exit(-1);
    }
    

    //判断储存位置
    if (vq.type == VarQualifierType::NSC) {
        // 为不储存的常量时:
        //  等号
        if (!peekAndCheck(0, TokenType::equal)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Missing a '='\a\n";
            exit(-1);
        }
        m_index++;

        //  处理赋值
        if (!peekAndCheck(0, TokenType::imm)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Assign non numeric value to variables " << var.name << " with NSC qualifier\a\n  ";
            std::cerr << "Note:\n    Variables with NSC qualifier can only be assigned numerical value";
            exit(-1);
        }
        vq.imm = consume().value.value();
        var.qualifier = vq;

        //向VarMap插入变量:
        VarMap.insert({var.name, var});
        VarUsedMap.insert({var.name, false});

        //  分号
        if (!peekAndCheck(0, TokenType::semicolon)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Missing a ';'\a\n";
            exit(-1);
        }
        m_index++;

        return std::make_unique<StmtVarDef>(var, vq.imm);
    }

    // 为正常变量时
    //  左括号
    if (!peekAndCheck(0, TokenType::left_paren)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a '('\a\n";
        exit(-1);
    }
    m_index++;

    //  获取储存位置
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
            errLine(peek(-1).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Register " << var.loc.reg.name << " cannot store variable " << var.name << "\a\n";
            exit(-1);
        }
        else if (var.loc.reg.size > var.size) {
            std::cerr << "At line:" << peek(-1).value().lineIndex << "\n  ";
            std::cout << "When: parsing\n  Warning:\n    ";
            std::cout << "Register " << var.loc.reg.name << " is too large for variable " << var.name << "\n";
        }
    }
    else {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Invalid store location for variable " << var.name << "\a\n";
        exit(-1);
    }
    
    //  右括号
    if (!peekAndCheck(0, TokenType::right_paren)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ')'\a\n";
        exit(-1);
    }
    m_index++;

    //向VarMap插入变量:
    VarMap.insert({var.name, var});
    VarUsedMap.insert({var.name, false});

    //结束或设置初始值:
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
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Invalid assignment for variable " << var.name << "\a\n";
            exit(-1);
        }
        std::string value = consume().value.value();

        // 分号
        if (!peekAndCheck(0, TokenType::semicolon)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Missing a ';'\a\n";
            exit(-1);
        }
        m_index++;

        return std::make_unique<StmtVarDef>(var, value);
    }
    else {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "After declaring the variable " << var.name << ", neither end the line nor assign a value\a\n";
        exit(-1);
    }
}

std::unique_ptr<StmtInlineAsm> Parser::parseStmtInlineAsm() {
    // 消耗asm
    m_index++;

    // 获取汇编代码
    if (!peekAndCheck(0, TokenType::assembly)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing inline assembly code after \"asm\"\a\n";
        exit(-1);
    }
    std::string code = consume().value.value();

    // 删去多个连续换行
    while (code.find("\n\n") != std::string::npos) {
        size_t pos = code.find("\n\n");
        code.erase(pos, 1);
    }

    // 删去末尾换行
    if (!code.empty() && code.back() == '\n') {
        code.pop_back();
    }

    // 删去首个换行
    if (!code.empty() && code.front() == '\n') {
        code.erase(0, 1);
    }

    // 分号
    if (!peekAndCheck(0, TokenType::semicolon)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ';'\a\n";
        exit(-1);
    }
    m_index++;

    return std::make_unique<StmtInlineAsm>(code);
}

std::unique_ptr<StmtSectionDef> Parser::parseStmtSectionDef() {
    //消耗section
    m_index++;

    //检测是否已声明
    std::string name = consume().value.value();
    if (sectionMap.contains(name)) {
        errLine(peek(-1).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Section " << name << " has been declared\a\n";
        exit(-1);
    }
    //加入列表
    sectionMap.insert(name);

    //分号
    if (!peekAndCheck(0, TokenType::semicolon)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ';'\a\n";
        exit(-1);
    }
    m_index++;

    return std::make_unique<StmtSectionDef>(name);
}

void Parser::parseExtern() {
    //消耗extern
    m_index++;

    while (true) {
        //检测是否有名称
        if (!peekAndCheck(0, TokenType::ident)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Missing extern function's name\a\n";
            exit(-1);
        }
        //检测是否已被导入
        std::string name = consume().value.value();
        if (externMap.contains(name)) {
            errLine(peek(-1).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Function " << name << " has been externed\a\n";
            exit(-1);
        }
        //加入列表
        extern_funcs.push_back(name);
        externMap.insert(name);
        //处理下一个
        if (peek().has_value()) {
            if (peek().value().type == TokenType::semicolon) {
                break;
            }
            else if (peek().value().type == TokenType::comma) {
                m_index++;
            }
            else {
                errLine(peek().value());
                std::cerr << "When: parsing\n  Error:\n    ";
                std::cerr << "Invalid Token\a\n";
                exit(-1);
            }
        }
        else {
            errLine(peek(-1).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "No Token after here\a\n";
            exit(-1);
        }
    }

    //分号
    if (!peekAndCheck(0, TokenType::semicolon)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a ';'\a\n";
        exit(-1);
    }
    m_index++;
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
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a \"::\" !\a\n";
        exit(-1);
    }
    m_index += 2;

    // 检查寄存器名
    if (!peekAndCheck(0, TokenType::ident)) {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Missing a register name\a\n";
        exit(-1);
    }
    if (!RegMap.contains(peek().value().value.value())) {
        errLine(peek().value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "REG qualifier does not include this register\a\n";
        exit(-1);
    }

    return RegMap.at(peek().value().value.value());
}

// 解析寻址括号
IRIaddr Parser::parseIRIaddr() {
    IRIaddr ab;

    if (peekAndCheck(0, TokenType::reg)) {
        //reg
        ab.base = parseRegister().name;
        m_index++;

        if(!peekAndCheck(0, TokenType::add) && !peekAndCheck(0, TokenType::sub) && !peekAndCheck(0, TokenType::right_bracket)) {
            //不是+ 不是- 不是]
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
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
                    errLine(peek(getPeekOffset()).value());
                    std::cerr << "When: parsing\n  Error:\n    ";
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
                            errLine(peek(1).value());
                            std::cerr << "When: parsing\n  Error:\n    ";
                            std::cerr << "无效的比例因子!!!\n应为1,2,4,8中一个\a\n";
                            exit(-1);
                        }
                    }
                    m_index += 2;
                    if(!peekAndCheck(0, TokenType::add) && !peekAndCheck(0, TokenType::sub) && !peekAndCheck(0, TokenType::right_bracket)) {
                        errLine(peek().value());
                        std::cerr << "When: parsing\n  Error:\n    ";
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
                        errLine(peek().value());
                        std::cerr << "When: parsing\n  Error:\n    ";
                        std::cerr << "无效的寻址括号!!!\n\"reg + reg * imm +/- imm\"已经是完整格式\a\n";
                        exit(-1);
                    }
                }
                else {
                    errLine(peek().value());
                    std::cerr << "When: parsing\n  Error:\n    ";
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
                    errLine(peek(getPeekOffset()).value());
                    std::cerr << "When: parsing\n  Error:\n    ";
                    std::cerr << "无效的寻址括号!!!\n\"reg +/- imm\"已经是完整格式\a\n";
                    exit(-1);
                }
            }
            else {
                errLine(peek(getPeekOffset()).value());
                std::cerr << "When: parsing\n  Error:\n    ";
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
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "无效的寻址括号!!!\n\"imm\"已经是完整格式\a\n";
            exit(-1);
        }
    }
    else {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "无效的寻址括号!!!\n在'['后出现未知内容\a\n";
        exit(-1);
    }

    return ab;
}

// 解析值
Value Parser::parseValue() {
    Value value;

    if (peekAndCheck(0, TokenType::ident)) {
        // 变量
        std::string varName = peek().value().value.value();
        if (!VarMap.contains(varName)) {
            errLine(m_tokens.at(m_index));
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Variable " << varName << " is not defined\a\n";
            exit(-1);
        }
        m_index++;
        value.type = ValueType::VAR;
        value.var = VarMap.at(varName);

        //  标记变量使用
        VarUsedMap.at(varName) = true;
    }
    else if (peekAndCheck(0, TokenType::reg)) {
        // 寄存器
        value.type = ValueType::REG;
        value.reg = parseRegister();
        m_index++;
    }
    else if (peekAndCheck(0, TokenType::left_bracket)) {
        // 内存
        //  消耗[
        m_index++;
        value.type = ValueType::MEM;
        value.mem = parseIRIaddr();

        //  检查并消耗]
        if (!peekAndCheck(0, TokenType::right_bracket)) {
            errLine(peek(getPeekOffset()).value());
            std::cerr << "When: parsing\n  Error:\n    ";
            std::cerr << "Missing a ']'\a\n";
            exit(-1);
        }
        m_index++;
    }
    else if (peekAndCheck(0, TokenType::imm)) {
        // 立即数
        value.type = ValueType::IMM;
        value.imm = consume().value.value();
    }
    else {
        errLine(peek(getPeekOffset()).value());
        std::cerr << "When: parsing\n  Error:\n    ";
        std::cerr << "Invalid Value\a\n";
        exit(-1);
    }

    return value;
}

// 获取值类型(去除变量类型)
inline ValueType Parser::getValueType(Value value) const {
    if (value.type == ValueType::VAR) {
        if (!value.var.qualifier.has_value()) {
            return value.var.loc.isReg ? ValueType::REG : ValueType::MEM;
        }
        else if (value.var.qualifier.value().type == VarQualifierType::NSC) {
            return ValueType::IMM;
        }
    }
    return value.type;
}

// 获取预览偏移, 与errLine配合使用
inline int32 Parser::getPeekOffset() const {
    return peek().has_value() ? 0 : -1;
}

// 行号报错
void errLine(Token token) {
    std::cerr << "At line:" << token.lineIndex << "\n  ";
}