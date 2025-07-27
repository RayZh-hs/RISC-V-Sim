// decoder.hpp
// - implements decoding of instructions

#pragma once

#include <string>
#include <ostream>
#include <cstdint>

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

    const std::string ins_type_names[] = {
        "NOOP",  // Custom Defined
        "ADD",  "SUB",  "AND",  "OR",   "XOR",  "SLL",   "SRL", "SRA", "SLT",  "SLTU",  "ADDI", "ANDI", "ORI",
        "XORI", "SLLI", "SRLI", "SRAI", "SLTI", "SLTIU", "LB",  "LBU", "LH",   "LHU",   "LW",   "SB",   "SH",
        "SW",   "BEQ",  "BGE",  "BGEU", "BLT",  "BLTU",  "BNE", "JAL", "JALR", "AUIPC", "LUI"};

    struct InstructionHeader {
        uint8_t op_code;
        uint8_t func3;
        uint8_t func7;

        InsType ins_type;
        InsClass ins_class;
        InsPos ins_pos;

        static InstructionHeader from(uint32_t instruction);
        void decode();
    };

    struct Instruction {
        InstructionHeader header;
        uint32_t raw;
        uint32_t imm;
        uint8_t rd;
        uint8_t rs1;
        uint8_t rs2;

        static Instruction from(uint32_t instruction);

        bool is_noop() const;
        bool is_halt() const;

        // ! Deprecated: Use the new repr() method instead
        std::ostream &operator<<(std::ostream &os) const;
        std::string repr() const;
    };

    // Free function for stream insertion
    std::ostream &operator<<(std::ostream &os, const Instruction &ins);

    inline constexpr static Instruction noop{};
}  // namespace norb::riscv
