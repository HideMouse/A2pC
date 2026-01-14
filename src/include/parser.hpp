#pragma once

#include "tokenization.hpp"

#include <memory>
#include <unordered_set>

// 赋值类型
enum AssiType {
    MOV,  // lV <- rV
    ADD,  // lV <- lV + rV
    SUB,  // lV <- lV - rV
    AND,  // lV <- lV & rV
    OR,   // lV <- lV | rV
    XOR,  // lV <- lV ^ rV
    SHL,  // lV <- lV << rV
    SHR,  // lV <- lV >> rV
    INC,  // lV <- lV + sizeof(lV)
    DEC   // lV <- lV - sizeof(lV)
};

const std::unordered_map<std::string, AssiType> AssiMap = {
    {"=",   AssiType::MOV},
    {"+=",  AssiType::ADD},
    {"-=",  AssiType::SUB},
    {"&=",  AssiType::AND},
    {"|=",  AssiType::OR},
    {"^=",  AssiType::XOR},
    {"<<=", AssiType::SHL},
    {">>=", AssiType::SHR},
    {"++",  AssiType::INC},
    {"--",  AssiType::DEC},
};

const std::unordered_map<TokenType, char> tokenToCharMap = {
    {TokenType::left_angle_bracket,  '<'},
    {TokenType::right_angle_bracket, '>'},
    {TokenType::equal,               '='},
    {TokenType::asterisk,            '*'},
    {TokenType::add,                 '+'},
    {TokenType::sub,                 '-'},
    {TokenType::logic_and,           '&'},
    {TokenType::logic_or,            '|'},
    {TokenType::logic_xor,           '^'},
};

struct Register {
    std::string name;
    uint8 size;
    bool canSrc;
    bool canDst;
};

// 寄存器表
const std::unordered_map<std::string, Register> RegMap = {
    // 通用寄存器 - 64b
    {"rax",  {"rax",  3, true, true}},
    {"rbx",  {"rbx",  3, true, true}},
    {"rcx",  {"rcx",  3, true, true}},
    {"rdx",  {"rdx",  3, true, true}},
    // 通用寄存器 - 32b
    {"eax",  {"eax",  2, true, true}},
    {"ebx",  {"ebx",  2, true, true}},
    {"ecx",  {"ecx",  2, true, true}},
    {"edx",  {"edx",  2, true, true}},
    // 通用寄存器 - 16b
    {"ax",   {"ax",   1, true, true}},
    {"bx",   {"bx",   1, true, true}},
    {"cx",   {"cx",   1, true, true}},
    {"dx",   {"dx",   1, true, true}},
    // 通用寄存器 - 8b(低)
    {"al",   {"al",   0, true, true}},
    {"bl",   {"bl",   0, true, true}},
    {"cl",   {"cl",   0, true, true}},
    {"dl",   {"dl",   0, true, true}},
    // 通用寄存器 - 8b(高)
    {"ah",   {"ah",   0, true, true}},
    {"bh",   {"bh",   0, true, true}},
    {"ch",   {"ch",   0, true, true}},
    {"dh",   {"dh",   0, true, true}},

    // 64位扩展寄存器 - 64b
    {"r8",   {"r8",   3, true, true}},
    {"r9",   {"r9",   3, true, true}},
    {"r10",  {"r10",  3, true, true}},
    {"r11",  {"r11",  3, true, true}},
    {"r12",  {"r12",  3, true, true}},
    {"r13",  {"r13",  3, true, true}},
    {"r14",  {"r14",  3, true, true}},
    {"r15",  {"r15",  3, true, true}},
    // 64位扩展寄存器 - 32b
    {"r8d",  {"r8d",  2, true, true}},
    {"r9d",  {"r9d",  2, true, true}},
    {"r10d", {"r10d", 2, true, true}},
    {"r11d", {"r11d", 2, true, true}},
    {"r12d", {"r12d", 2, true, true}},
    {"r13d", {"r13d", 2, true, true}},
    {"r14d", {"r14d", 2, true, true}},
    {"r15d", {"r15d", 2, true, true}},
    // 64位扩展寄存器 - 16b
    {"r8w",  {"r8w",  1, true, true}},
    {"r9w",  {"r9w",  1, true, true}},
    {"r10w", {"r10w", 1, true, true}},
    {"r11w", {"r11w", 1, true, true}},
    {"r12w", {"r12w", 1, true, true}},
    {"r13w", {"r13w", 1, true, true}},
    {"r14w", {"r14w", 1, true, true}},
    {"r15w", {"r15w", 1, true, true}},
    // 64位扩展寄存器 - 8b
    {"r8b",  {"r8b",  0, true, true}},
    {"r9b",  {"r9b",  0, true, true}},
    {"r10b", {"r10b", 0, true, true}},
    {"r11b", {"r11b", 0, true, true}},
    {"r12b", {"r12b", 0, true, true}},
    {"r13b", {"r13b", 0, true, true}},
    {"r14b", {"r14b", 0, true, true}},
    {"r15b", {"r15b", 0, true, true}},

    // 栈顶与栈帧寄存器 - 64位
    {"rsp",  {"rsp",  3, true, true}},
    {"rbp",  {"rbp",  3, true, true}},
    // 栈顶与栈帧寄存器 - 32位
    {"esp",  {"esp",  2, true, true}},
    {"ebp",  {"ebp",  2, true, true}},
    // 栈顶与栈帧寄存器 - 16位
    {"sp",   {"sp",   1, true, true}},
    {"bp",   {"bp",   1, true, true}},

    // 源/目的索引寄存器 - 64位
    {"rsi",  {"rsi",  3, true, true}},
    {"rdi",  {"rdi",  3, true, true}},
    // 源/目的索引寄存器 - 32位
    {"esi",  {"esi",  2, true, true}},
    {"edi",  {"edi",  2, true, true}},
    // 源/目的索引寄存器 - 16位
    {"si",   {"si",   1, true, true}},
    {"di",   {"di",   1, true, true}},

    // 段寄存器 - 16位
    {"cs",   {"cs",   1, true, true}},
    {"ds",   {"ds",   1, true, true}},
    {"es",   {"es",   1, true, true}},
    {"fs",   {"fs",   1, true, true}},
    {"gs",   {"gs",   1, true, true}},
    {"ss",   {"ss",   1, true, true}},
};

const std::unordered_set<std::string> JumpTypeSet = {
    "z",      // 0
    "nz",     // 非0
    "e",      // 等于
    "ne",     // 不等于
    "o",      // 溢出
    "no",     // 未溢出
    "c",      // 进位
    "nc",     // 无进位
};

struct IRIaddr {
    std::string base;
    std::string index;
    std::string scale;
    std::string displacement;
    bool isNumber = false;
};

// 变量位置
struct VarLoc {
    bool isReg;
    Register reg;
    std::string memAddr;
};

// 变量
struct Var {
    uint8 size;
    std::string name;
    VarLoc loc;

    // Var(uint8 size, const std::string& name, VarLoc loc)
    //     : size(size), name(name), loc(loc) {}
};

// 值类型
enum ValueType {
    VAR,  // 变量
    REG,  // 寄存器
    MEM,  // 内存
    IMM   // 立即数
};

// 值
struct Value {
    ValueType type;
    Var var;
    Register reg;
    IRIaddr mem;
    std::string imm;
};


// AST节点基类
struct ASTNode {
    virtual ~ASTNode() = default;
};

// 赋值语句
struct StmtAssi : public ASTNode {
    AssiType assiType; // 赋值类型
    Value leftValue;   // 左值
    Value rightValue;  // 右值

    uint8 addrSize;    // 地址尺寸

    StmtAssi(AssiType type, Value lV, Value rV, uint8 addrSize)
        : assiType(type), leftValue(lV), rightValue(rV), addrSize(addrSize) {}
};

// 前往语句
struct StmtGoto : public ASTNode {
    std::string labelOrAddr;
    
    StmtGoto(const std::string& loa) 
        : labelOrAddr(loa) {}
};

// 测试与语句
struct StmtTest : public ASTNode {
    std::string left;
    std::string right;
    std::string labelOrAddr;
    std::string type;
    
    StmtTest(const std::string& l, const std::string& r, const std::string& loa, const std::string& type) 
        : left(l), right(r), labelOrAddr(loa), type(type) {}
};

// 标签
struct StmtLabel : public ASTNode {
    std::string labelName;
    
    StmtLabel(const std::string& ln) 
        : labelName(ln) {}
};

// 变量声明语句
struct StmtVarDef : public ASTNode {
    Var var;
    std::optional<std::string> defValue;

    StmtVarDef(Var var, std::optional<std::string> value)
        : var(var), defValue(value) {}
};

// 程序根节点
struct Program : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    std::vector<std::string> global_funcs;
    std::vector<std::string> extern_funcs;
};

class Parser {
    public:
        Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens)) {};

        std::unique_ptr<Program> parse();

        std::unique_ptr<StmtAssi> parseStmtAssi();
        std::unique_ptr<StmtGoto> parseStmtGoto();
        std::unique_ptr<StmtLabel> parseStmtLabel();
        std::unique_ptr<StmtTest> parseStmtTest();
        std::unique_ptr<StmtVarDef> parseStmtVarDef();

    private:
        std::optional<Token> peek(uint32 offset = 0) const;
        bool peekAndCheck(uint32 offset, TokenType type) const;
        Token consume();

        Register parseRegister();
        IRIaddr parseIRIaddr();

    private:
        const std::vector<Token> m_tokens;
        uint32 m_index = 0;

        std::vector<std::string> global_funcs;

    private:
        std::unordered_map<std::string, Var> VarMap;
};