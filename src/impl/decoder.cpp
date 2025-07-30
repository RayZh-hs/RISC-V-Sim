// decoder.cpp
// - implements decoding of instructions

#include "decoder.hpp"
#include "utility/dump.hpp"

#include <stdexcept>

namespace norb::riscv {

    InstructionHeader InstructionHeader::from(uint32_t instruction) {
        return InstructionHeader{.op_code = static_cast<uint8_t>(instruction & 0b1111111),
                                 .func3 = static_cast<uint8_t>((instruction >> 12) & 0b111),
                                 .func7 = static_cast<uint8_t>((instruction >> 25) & 0b1111111)};
    }

    void InstructionHeader::decode() {
        switch (op_code) {
            case 0b0000000:
                ins_type = NOOP;
                break;  // this is custom defined behavior, assign noop for 0x00000000
            case 0b0110111:
                ins_type = LUI;
                break;
            case 0b0010111:
                ins_type = AUIPC;
                break;
            case 0b1101111:
                ins_type = JAL;
                break;
            case 0b1100111:
                ins_type = JALR;
                break;
            case 0b1100011:
                switch (func3) {
                    case 0b000:
                        ins_type = BEQ;
                        break;
                    case 0b001:
                        ins_type = BNE;
                        break;
                    case 0b100:
                        ins_type = BLT;
                        break;
                    case 0b101:
                        ins_type = BGE;
                        break;
                    case 0b110:
                        ins_type = BLTU;
                        break;
                    case 0b111:
                        ins_type = BGEU;
                        break;
                    default:
                        throw std::runtime_error("Invalid B-Type func3");
                }
                break;
            case 0b0000011:
                switch (func3) {
                    case 0b000:
                        ins_type = LB;
                        break;
                    case 0b001:
                        ins_type = LH;
                        break;
                    case 0b010:
                        ins_type = LW;
                        break;
                    case 0b100:
                        ins_type = LBU;
                        break;
                    case 0b101:
                        ins_type = LHU;
                        break;
                    default:
                        throw std::runtime_error("Invalid Load-Type func3");
                }
                break;
            case 0b0100011:
                switch (func3) {
                    case 0b000:
                        ins_type = SB;
                        break;
                    case 0b001:
                        ins_type = SH;
                        break;
                    case 0b010:
                        ins_type = SW;
                        break;
                    default:
                        throw std::runtime_error("Invalid S-Type func3");
                }
                break;
            case 0b0010011:
                switch (func3) {
                    case 0b000:
                        ins_type = ADDI;
                        break;
                    case 0b001:
                        ins_type = SLLI;
                        break;
                    case 0b010:
                        ins_type = SLTI;
                        break;
                    case 0b011:
                        ins_type = SLTIU;
                        break;
                    case 0b100:
                        ins_type = XORI;
                        break;
                    case 0b101:  // SRLI or SRAI
                        if (func7 == 0b0000000)
                            ins_type = SRLI;
                        else if (func7 == 0b0100000)
                            ins_type = SRAI;
                        else
                            throw std::runtime_error("Invalid I-Type func7 for func3=101");
                        break;
                    case 0b110:
                        ins_type = ORI;
                        break;
                    case 0b111:
                        ins_type = ANDI;
                        break;
                    default:
                        throw std::runtime_error("Invalid I-Type func3");
                }
                break;
            case 0b0110011:
                switch (func3) {
                    case 0b000:  // ADD or SUB, strange to be distinguished by func7
                        if (func7 == 0b0000000)
                            ins_type = ADD;
                        else if (func7 == 0b0100000)
                            ins_type = SUB;
                        else
                            throw std::runtime_error("Invalid R-Type func7 for func3=000");
                        break;
                    case 0b001:
                        ins_type = SLL;
                        break;
                    case 0b010:
                        ins_type = SLT;
                        break;
                    case 0b011:
                        ins_type = SLTU;
                        break;
                    case 0b100:
                        ins_type = XOR;
                        break;
                    case 0b101:  // SRL or SRA
                        if (func7 == 0b0000000)
                            ins_type = SRL;
                        else if (func7 == 0b0100000)
                            ins_type = SRA;
                        else
                            throw std::runtime_error("Invalid R-Type func7 for func3=101");
                        break;
                    case 0b110:
                        ins_type = OR;
                        break;
                    case 0b111:
                        ins_type = AND;
                        break;
                    default:
                        throw std::runtime_error("Invalid R-Type func3");
                }
                break;
            default:
                throw std::runtime_error("Invalid instruction opcode");
        }

        // Look up class and position
        ins_class = ins_class_map[ins_type];
        ins_pos = ins_pos_map[ins_type];
    }

    Instruction Instruction::from(uint32_t instruction) {
        InstructionHeader header = InstructionHeader::from(instruction);
        header.decode();

        uint32_t decoded_imm = 0;
        switch (header.ins_class) {
            case I_CLASS:
                {
                    // I-type immediate: [31:20]
                    //! Sign Extend imm
                    int32_t imm_i = static_cast<int32_t>(instruction) >> 20;
                    decoded_imm = static_cast<uint32_t>(imm_i);
                    break;
                }
            case S_CLASS:
                {
                    // S-type immediate: [31:25][11:7]
                    uint32_t imm_11_5 = (instruction >> 25) & 0x7F;  // imm[11:5]
                    uint32_t imm_4_0 = (instruction >> 7) & 0x1F;  // imm[4:0]
                    //! Sign Extend imm
                    uint32_t imm = (imm_11_5 << 5) | imm_4_0;
                    int32_t imm_s = static_cast<int32_t>(imm) << 20 >> 20;
                    decoded_imm = static_cast<uint32_t>(imm_s);
                    break;
                }
            case B_CLASS:
                {
                    // B-type immediate: imm[12|10:5] from [31|30:25], imm[4:1|11] from [11:8|7]
                    uint32_t imm_12 = (instruction & 0x80000000) >> 19;  // imm[12]
                    uint32_t imm_11 = (instruction & 0x00000080) << 4;  // imm[11]
                    uint32_t imm_10_5 = (instruction & 0x7E000000) >> 20;  // imm[10:5]
                    uint32_t imm_4_1 = (instruction & 0x00000F00) >> 7;  // imm[4:1]
                    //! Sign Extend from bit 12
                    int32_t imm_b = static_cast<int32_t>(imm_12 | imm_11 | imm_10_5 | imm_4_1) << 19 >> 19;
                    decoded_imm = static_cast<uint32_t>(imm_b);
                    break;
                }
            case U_CLASS:
                {
                    // U-type immediate: [31:12]
                    decoded_imm = instruction & 0xFFFFF000;
                    break;
                }
            case J_CLASS:
                {
                    // J-type immediate: [20|10:1|11|19:12]
                    uint32_t imm_20 = (instruction & 0x80000000) >> 11;  // imm[20]
                    uint32_t imm_19_12 = (instruction & 0x000FF000);  // imm[19:12]
                    uint32_t imm_11 = (instruction & 0x00100000) >> 9;  // imm[11]
                    uint32_t imm_10_1 = (instruction & 0x7FE00000) >> 20;  // imm[10:1]
                    //! Sign Extend from bit 20
                    int32_t imm_j = static_cast<int32_t>(imm_20 | imm_19_12 | imm_11 | imm_10_1) << 11 >> 11;
                    decoded_imm = static_cast<uint32_t>(imm_j);
                    break;
                }
            case R_CLASS:
                decoded_imm = 0;
                break;
            default:
                break;
        }

        Instruction ins = {.header = header,
                           .raw = instruction,
                           .imm = decoded_imm,
                           .rd = static_cast<uint8_t>((instruction >> 7) & 0b11111),
                           .rs1 = static_cast<uint8_t>((instruction >> 15) & 0b11111),
                           .rs2 = static_cast<uint8_t>((instruction >> 20) & 0b11111),
                           // The last two fields are manually set 
                           .had_jumped = false,
                           .pc = 0};
        return ins;
    }

    bool Instruction::is_noop() const { return header.ins_type == NOOP; }

    bool Instruction::is_halt() const { return raw == 0x0ff00513; }

    std::ostream &Instruction::operator<<(std::ostream &os) const {
        os << "[Instruction " << norb::dump_repr(raw) << "]";
        return os;
    }

    // Free function for stream insertion - needed for template compatibility
    std::ostream &operator<<(std::ostream &os, const Instruction &ins) { return ins.operator<<(os); }

    std::string Instruction::repr() const {
        return "Instruction(type=" + ins_type_names[static_cast<int>(header.ins_type)] +
            ", raw=" + norb::dump_repr(raw) + ", rd=" + std::to_string(rd) + ", rs1=" + std::to_string(rs1) +
            ", rs2=" + std::to_string(rs2) + ", imm=" + std::to_string(imm) + ")";
    }

}  // namespace norb::riscv
