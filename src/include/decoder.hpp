// decoder.hpp
// - implements decoding of instructions

#pragma once

namespace norb::riscv {

    enum InsType {
        NOOP,  // custom defined
        ADD,
        SUB,
        AND,
        OR,
        XOR,
        SLL,
        SRL,
        SRA,
        SLT,
        SLTU,
        ADDI,
        ANDI,
        ORI,
        XORI,
        SLLI,
        SRLI,
        SRAI,
        SLTI,
        SLTIU,
        LB,
        LBU,
        LH,
        LHU,
        LW,
        SB,
        SH,
        SW,
        BEQ,
        BGE,
        BGEU,
        BLT,
        BLTU,
        BNE,
        JAL,
        JALR,
        AUIPC,
        LUI
    };

    enum InsClass {
        NO_CLASS,
        R_CLASS,
        I_CLASS,
        S_CLASS,
        B_CLASS,
        U_CLASS,
        J_CLASS,
    };

    enum InsPos {
        NO_POS,
        ALU,
        LSB,
        BRANCH,
        REG,  // direct Register File access (broadcast immediately)
    };

    constexpr InsClass ins_class_map[] = {
        // Custom Defined
        NO_CLASS,  // NOOP
        // R-Type (ALU)
        R_CLASS,  // ADD
        R_CLASS,  // SUB
        R_CLASS,  // AND
        R_CLASS,  // OR
        R_CLASS,  // XOR
        R_CLASS,  // SLL
        R_CLASS,  // SRL
        R_CLASS,  // SRA
        R_CLASS,  // SLT
        R_CLASS,  // SLTU
        // I-Type (ALU)
        I_CLASS,  // ADDI
        I_CLASS,  // ANDI
        I_CLASS,  // ORI
        I_CLASS,  // XORI
        I_CLASS,  // SLLI
        I_CLASS,  // SRLI
        I_CLASS,  // SRAI
        I_CLASS,  // SLTI
        I_CLASS,  // SLTIU
        // I-Type (Load)
        I_CLASS,  // LB
        I_CLASS,  // LBU
        I_CLASS,  // LH
        I_CLASS,  // LHU
        I_CLASS,  // LW
        // S-Type (Store)
        S_CLASS,  // SB
        S_CLASS,  // SH
        S_CLASS,  // SW
        // B-Type (Branch)
        B_CLASS,  // BEQ
        B_CLASS,  // BGE
        B_CLASS,  // BGEU
        B_CLASS,  // BLT
        B_CLASS,  // BLTU
        B_CLASS,  // BNE
        // J-Type (Jump)
        J_CLASS,  // JAL
        // I-Type (Jump)
        I_CLASS,  // JALR
        // U-Type (Direct Reg)
        U_CLASS,  // AUIPC
        U_CLASS,  // LUI
    };

    constexpr InsPos ins_pos_map[] = {
        // Custom Instructions
        NO_POS,  // NOOP
        // ALU Instructions
        ALU,  // ADD
        ALU,  // SUB
        ALU,  // AND
        ALU,  // OR
        ALU,  // XOR
        ALU,  // SLL
        ALU,  // SRL
        ALU,  // SRA
        ALU,  // SLT
        ALU,  // SLTU
        ALU,  // ADDI
        ALU,  // ANDI
        ALU,  // ORI
        ALU,  // XORI
        ALU,  // SLLI
        ALU,  // SRLI
        ALU,  // SRAI
        ALU,  // SLTI
        ALU,  // SLTIU
        // Load/Store Instructions
        LSB,  // LB
        LSB,  // LBU
        LSB,  // LH
        LSB,  // LHU
        LSB,  // LW
        LSB,  // SB
        LSB,  // SH
        LSB,  // SW
        // Branch/Jump Instructions
        BRANCH,  // BEQ
        BRANCH,  // BGE
        BRANCH,  // BGEU
        BRANCH,  // BLT
        BRANCH,  // BLTU
        BRANCH,  // BNE
        BRANCH,  // JAL
        BRANCH,  // JALR
        // Register Instructions
        REG,  // AUIPC
        REG,  // LUI
    };

    struct InstructionHeader {
        uint8_t op_code;
        uint8_t func3;
        uint8_t func7;

        InsType ins_type;
        InsClass ins_class;
        InsPos ins_pos;

        static InstructionHeader from(uint32_t instruction) {
            return InstructionHeader{.op_code = static_cast<uint8_t>(instruction & 0b1111111),
                                     .func3 = static_cast<uint8_t>((instruction >> 12) & 0b111),
                                     .func7 = static_cast<uint8_t>((instruction >> 25) & 0b1111111)};
        }

        void decode() {
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
    };

    struct Instruction {
        InstructionHeader header;
        uint32_t raw;
        uint32_t imm;
        uint8_t rd;
        uint8_t rs1;
        uint8_t rs2;

        static Instruction from(uint32_t instruction) {
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
                               .rs2 = static_cast<uint8_t>((instruction >> 20) & 0b11111)};
            return ins;
        }

        bool is_noop() const { return header.ins_type == NOOP; }

        bool is_halt() const { return header.ins_type == InsType::LUI && rd == 10 && imm == 256; }

        std::ostream &operator<<(std::ostream &os) const {
            os << "[Instruction " << raw << "]";
            return os;
        }
    };

    inline constexpr static Instruction noop{};
}  // namespace norb::riscv
