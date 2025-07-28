// alu.cpp
// - implements the Arithmetic Logic Unit

#include "alu.hpp"
#include "third_party/logger.hpp"

namespace norb::riscv {

    uint32_t ArithmeticLogicUnit::calculate(const ResolvedInstructionEntry &e) {
        switch (e.type) {
            // R-Type ALU Instructions
            case ADD:
                return e.vj + e.vk;
            case SUB:
                return e.vj - e.vk;
            case AND:
                return e.vj & e.vk;
            case OR:
                return e.vj | e.vk;
            case XOR:
                return e.vj ^ e.vk;
            case SLL:
                return e.vj << (e.vk & 0x1F);  // Only use lower 5 bits for shift amount
            case SRL:
                return e.vj >> (e.vk & 0x1F);  // Logical right shift
            case SRA:
                return static_cast<uint32_t>(static_cast<int32_t>(e.vj) >> (e.vk & 0x1F));  // Arithmetic right shift
            case SLT:
                return (static_cast<int32_t>(e.vj) < static_cast<int32_t>(e.vk)) ? 1 : 0;  // Signed comparison
            case SLTU:
                return (e.vj < e.vk) ? 1 : 0;  // Unsigned comparison
            
            // I-Type ALU Instructions
            case ADDI:
                return e.vj + e.imm;
            case ANDI:
                return e.vj & e.imm;
            case ORI:
                return e.vj | e.imm;
            case XORI:
                return e.vj ^ e.imm;
            case SLLI:
                return e.vj << (e.imm & 0x1F);  // Only use lower 5 bits for shift amount
            case SRLI:
                return e.vj >> (e.imm & 0x1F);  // Logical right shift
            case SRAI:
                return static_cast<uint32_t>(static_cast<int32_t>(e.vj) >> (e.imm & 0x1F));  // Arithmetic right shift
            case SLTI:
                return (static_cast<int32_t>(e.vj) < static_cast<int32_t>(e.imm)) ? 1 : 0;  // Signed comparison
            case SLTIU:
                return (e.vj < e.imm) ? 1 : 0;  // Unsigned comparison
            
            default:
                const std::string name = e.type < InsType::_count ? ins_type_names[static_cast<int>(e.type)] : "UNKNOWN";
                throw norb::AssertionError("InsType(type=" + name + ") wrongly sent to ALU!");
        }
    }

}  // namespace norb::riscv
