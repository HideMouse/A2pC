#include "generation.hpp"

std::string Generator::generate() const {
    std::stringstream output;

    // 添加位宽标识
    output << "bits " << m_info.bits << "\n";

    // 添加全局标签标识
    if (!global_funcs.empty()) {
        output << "\nglobal " << global_funcs[0];
        for (uint32 i = 1; i < global_funcs.size(); i++) {
            output << ", " << global_funcs[i];
        }
        output << "\n";
    }
    // 添加导入标签标识
    if (!extern_funcs.empty()) {
        output << "\nextern " << extern_funcs[0];
        for (uint32 i = 1; i < extern_funcs.size(); i++) {
            output << ", " << extern_funcs[i];
        }
        output << "\n";
    }

    output << "\n";

    auto& stats = m_root.get()->statements;
    for (auto& stat : stats) {
        if (auto assign = dynamic_cast<StmtAssi*>(stat.get())) {
            ValueType lVT = getValueType(assign->leftValue);
            ValueType rVT = getValueType(assign->rightValue);
            uint8 addressSize = assign->addrSize;

            // 生成赋值类型
            switch (assign->assiType) {
                case MOV:
                    if (lVT == ValueType::MEM && rVT == ValueType::MEM) {
                        std::cerr << "=两侧不能同时为内存\a\n";
                        exit(-1);
                    }
                    output << "mov ";
                    break;
                case ADD:
                    if (lVT == ValueType::MEM && rVT == ValueType::MEM) {
                        std::cerr << "+=两侧不能同时为内存\a\n";
                        exit(-1);
                    }
                    output << "add ";
                    break;
                case SUB:
                    if (lVT == ValueType::MEM && rVT == ValueType::MEM) {
                        std::cerr << "-=两侧不能同时为内存\a\n";
                        exit(-1);
                    }
                    output << "sub ";
                    break;
                case AND:
                    if (lVT == ValueType::MEM && rVT == ValueType::MEM) {
                        std::cerr << "&=两侧不能同时为内存\a\n";
                        exit(-1);
                    }
                    output << "and ";
                    break;
                case OR:
                    if (lVT == ValueType::MEM && rVT == ValueType::MEM) {
                        std::cerr << "|=两侧不能同时为内存\a\n";
                        exit(-1);
                    }
                    output << "or ";
                    break;
                case XOR:
                    if (lVT == ValueType::MEM && rVT == ValueType::MEM) {
                        std::cerr << "^=两侧不能同时为内存\a\n";
                        exit(-1);
                    }
                    output << "xor ";
                    break;
                case SHL:
                    if (rVT != ValueType::IMM) {
                        std::cerr << "<<=右侧只能为数值\a\n";
                        exit(-1);
                    }
                    output << "shl ";
                    break;
                case SHR:
                    if (rVT != ValueType::IMM) {
                        std::cerr << ">>=右侧只能为数值\a\n";
                        exit(-1);
                    }
                    output << "shr ";
                    break;
                case INC:
                    if (assign->leftValue.type == ValueType::VAR) {
                        addressSize = assign->leftValue.var.size;
                    }
                    output << "add ";
                    break;
                case DEC:
                    if (assign->leftValue.type == ValueType::VAR) {
                        addressSize = assign->leftValue.var.size;
                    }
                    output << "sub ";
                    break;
            }

            // 修复地址尺寸
            if (assign->rightValue.type == ValueType::VAR &&
                assign->rightValue.var.qualifier.has_value() &&
                assign->rightValue.var.qualifier.value().type == VarQualifierType::NSC) {
                addressSize = assign->rightValue.var.size;
            }

            // 生成地址尺寸
            if (lVT == ValueType::MEM) {
                if (rVT == ValueType::MEM || rVT == ValueType::IMM) {
                    output << addrSizeToStr(addressSize) << " ";
                }
            }
            
            // 生成左值
            output << valueToStr(assign->leftValue) << ", ";

            // 获取左值尺寸
            uint8 lVSize = 0;
            switch (assign->leftValue.type) {
                case REG:
                    lVSize = assign->leftValue.reg.size;
                    break;
                case MEM:
                    lVSize = assign->addrSize;
                    break;
                case VAR:
                    if (assign->leftValue.var.loc.isReg) {
                        lVSize = assign->leftValue.var.loc.reg.size;
                    }
                    else {
                        lVSize = assign->leftValue.var.size;
                    }
            }
            
            // 生成右值
            if (assign->assiType == AssiType::INC || assign->assiType == AssiType::DEC) {
                // 无右值
                switch (lVSize) {
                    case 0:
                        output << "1\n";
                        break;
                    case 1:
                        output << "2\n";
                        break;
                    case 2:
                        output << "4\n";
                        break;
                    case 3:
                        output << "8\n";
                        break;
                }
            }
            else {
                output << valueToStr(assign->rightValue) << "\n";
            }
        }
        else if (auto assign = dynamic_cast<StmtGoto*>(stat.get())) {
            output << "jmp " << assign->labelOrAddr << "\n";
        }
        else if (auto assign = dynamic_cast<StmtTest*>(stat.get())) {
            output << "test " << valueToStr(assign->left) << ", " << valueToStr(assign->right);
            output << "\nj" << assign->jumpType << " " << assign->labelOrAddr << "\n";
        }
        else if (auto assign = dynamic_cast<StmtLabel*>(stat.get())) {
            output << "\n" << assign->labelName << ":\n";
        }
        else if (auto assign = dynamic_cast<StmtVarDef*>(stat.get())) {
            if (assign->defValue.has_value() &&
                VarUsedMap.at(assign->var.name) &&
                (!assign->var.qualifier.has_value() ||
                 assign->var.qualifier.value().type != VarQualifierType::NSC)) {
                output << ";" << assign->var.name;
                output << "\nmov ";
                if (assign->var.loc.isReg) {
                    output << assign->var.loc.reg.name << ", " << assign->defValue.value() << "\n";
                }
                else {
                    output << addrSizeToStr(assign->var.size);
                    output << " [abs " << assign->var.loc.memAddr << "], " << assign->defValue.value() << "\n";
                }
            }
        }
        else if (auto assign = dynamic_cast<StmtInlineAsm*>(stat.get())) {
            output << ";@inline:\n" << assign->assemblyCode << "\n;@\n";
        }
        else if (auto assign = dynamic_cast<StmtSectionDef*>(stat.get())) {
            output << "section " << assign->sectionName << "\n";
        }
    }
    
    return output.str();
}

inline std::string Generator::iriAddrToStr(IRIaddr ab) const {
    return (ab.isNumber ? "abs " : "") + ((!ab.base.empty()) ? ab.base : "") + ((!ab.index.empty()) ? "+" + ab.index : "") + ((!ab.scale.empty()) ? "*" + ab.scale : "") + ((!ab.displacement.empty()) ? "" + ab.displacement : "");
}

inline std::string Generator::valueToStr(Value value) const {
    switch (value.type) {
        case REG:
            return value.reg.name;
            break;
        case MEM:
            return "[" + iriAddrToStr(value.mem) + "]";
            break;
        case IMM:
            return value.imm;
            break;
        case VAR:
            if (value.var.qualifier.has_value() && value.var.qualifier.value().type == VarQualifierType::NSC) {
                return value.var.qualifier.value().imm;
            }

            if (value.var.loc.isReg) {
                return value.var.loc.reg.name;
            }
            else {
                return "[abs " + value.var.loc.memAddr + "]";
            }
    }
    return "";
}

inline std::string Generator::addrSizeToStr(uint8 size) const {
    switch (size) {
        case 0:
            return "byte";
            break;
        case 1:
            return "word";
            break;
        case 2:
            return "dword";
            break;
        case 3:
            return "qword";
            break;
    }
    return "byte";
}

inline ValueType Generator::getValueType(Value value) const {
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