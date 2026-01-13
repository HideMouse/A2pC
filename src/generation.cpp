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
        if (auto assign = dynamic_cast<StmtRegAssi*>(stat.get())) {
            switch (assign->type) {
                case 0:
                    output << "mov ";
                    break;
                case TokenType::add:
                    output << "add ";
                    break;
                case TokenType::sub:
                    output << "sub ";
                    break;
                case TokenType::logic_and:
                    output << "and ";
                    break;
                case TokenType::logic_or:
                    output << "or ";
                    break;
                case TokenType::logic_xor:
                    output << "xor ";
                    break;
                case TokenType::left_angle_bracket:
                    output << "shl ";
                    break;
                case TokenType::right_angle_bracket:
                    output << "shr ";
                    break;
            }
            output << assign->register_name << ", ";
            if (assign->getMemVal) {
                output << "[" << assign->value << "]\n";
            }
            else {
                output << assign->value << "\n";
            }
        }
        else if (auto assign = dynamic_cast<StmtMemAssi*>(stat.get())) {
            switch (assign->type) {
                case 0:
                    output << "mov ";
                    break;
                case TokenType::add:
                    output << "add ";
                    break;
                case TokenType::sub:
                    output << "sub ";
                    break;
                case TokenType::logic_and:
                    output << "and ";
                    break;
                case TokenType::logic_or:
                    output << "or ";
                    break;
                case TokenType::logic_xor:
                    output << "xor ";
                    break;
                case TokenType::left_angle_bracket:
                    output << "shl ";
                    break;
                case TokenType::right_angle_bracket:
                    output << "shr ";
                    break;
                default:
                    break;
            }
            if (std::isdigit(assign->value[0])) {
                switch (assign->size) {
                    case TokenType::byte:
                        output << "byte ";
                        break;
                    case TokenType::word:
                        output << "word ";
                        break;
                    case TokenType::dword:
                        output << "dword ";
                        break;
                    case TokenType::qword:
                        output << "qword ";
                        break;
                    default:
                        break;
                }
            }
            output << "[" << addrBrackToStr(assign->memAddr) << "], " << assign->value << "\n";
        }
        else if (auto assign = dynamic_cast<StmtGoto*>(stat.get())) {
            output << "jmp " << assign->labelOrAddr << "\n";
        }
        else if (auto assign = dynamic_cast<StmtTest*>(stat.get())) {
            output << "test " << assign->left << ", " << assign->right;
            output << "\nj" << assign->type << " " << assign->labelOrAddr << "\n";
        }
        else if (auto assign = dynamic_cast<StmtLabel*>(stat.get())) {
            output << assign->labelName << ":\n";
        }
        else if (auto assign = dynamic_cast<StmtVarDef*>(stat.get())) {
            if (assign->defValue.has_value()) {
                output << ";" << assign->var.name;
                output << "\nmov ";
                if (assign->var.loc.isReg) {
                    output << assign->var.loc.reg.name << ", " << assign->defValue.value() << "\n";
                }
                else {
                    switch (assign->var.size) {
                        case 0:
                            output << "byte ";
                            break;
                        case 1:
                            output << "word ";
                            break;
                        case 2:
                            output << "dword ";
                            break;
                        case 3:
                            output << "qword ";
                            break;
                    }
                    output << "[abs " << assign->var.loc.memAddr << "], " << assign->defValue.value() << "\n";
                }
            }
        }
    }
        
    return output.str();
}

inline std::string Generator::addrBrackToStr(AddressingBrackets ab) const {
    return ((!ab.base.empty()) ? ab.base : "") + ((!ab.index.empty()) ? " + " + ab.index : "") + ((!ab.scale.empty()) ? " * " + ab.scale : "") + ((!ab.displacement.empty()) ? " " + ab.displacement : "");
}