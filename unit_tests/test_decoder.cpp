#include <iostream>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <functional>

// ===================================================================================
//  RISC-V DECODER IMPLEMENTATION
// ===================================================================================

#include "decoder.hpp"

int main() {
    using namespace norb::riscv;
    int failures = 0;

    // Simple assertion helper
    auto ASSERT_EQUAL = [&](auto actual, auto expected, const std::string& test_name) {
        if (actual != expected) {
            std::cerr << "FAIL: " << test_name << "\n"
                      << "  Expected: " << +expected << " (0x" << std::hex << +expected << std::dec << ")\n"
                      << "  Actual:   " << +actual << " (0x" << std::hex << +actual << std::dec << ")\n\n";
            failures++;
        }
    };

    // Helper to test for exceptions
    auto TEST_THROWS = [&](std::function<void()> test_func, const std::string& test_name) {
        try {
            test_func();
            std::cerr << "FAIL: " << test_name << "\n"
                      << "  Expected std::runtime_error to be thrown, but it wasn't.\n\n";
            failures++;
        } catch (const std::runtime_error& e) {
            // This is the expected outcome, so we do nothing.
        } catch (...) {
            std::cerr << "FAIL: " << test_name << "\n"
                      << "  Expected std::runtime_error, but a different exception was thrown.\n\n";
            failures++;
        }
    };

    std::cout << "--- Running RISC-V Decoder Tests ---\n\n";

    // --- Test R-Type ---
    {
        // Assembly: add t0, t1, t2  (x5 = x6 + x7)
        const uint32_t instruction_word = 0x007302b3;
        Instruction ins = Instruction::from(instruction_word);
        ASSERT_EQUAL(ins.header.ins_type, ADD, "ADD: Type");
        ASSERT_EQUAL(ins.rd, 5, "ADD: rd");
        ASSERT_EQUAL(ins.rs1, 6, "ADD: rs1");
        ASSERT_EQUAL(ins.rs2, 7, "ADD: rs2");
        ASSERT_EQUAL(ins.imm, 0u, "ADD: imm");
    }

    // --- Test I-Type ---
    {
        // Assembly: addi t0, t1, -10
        const uint32_t instruction_word = 0xff630293;
        Instruction ins = Instruction::from(instruction_word);
        ASSERT_EQUAL(ins.header.ins_type, ADDI, "ADDI: Type");
        ASSERT_EQUAL(ins.rd, 5, "ADDI: rd");
        ASSERT_EQUAL(ins.rs1, 6, "ADDI: rs1");
        ASSERT_EQUAL(ins.imm, 0xFFFFFFF6, "ADDI: imm (sign-extended)");
    }

    // --- Test I-Type (Load) ---
    {
        // Assembly: lw t0, 12(sp)  (lw x5, 12(x2))
        const uint32_t instruction_word = 0x00c12283;
        Instruction ins = Instruction::from(instruction_word);
        ASSERT_EQUAL(ins.header.ins_type, LW, "LW: Type");
        ASSERT_EQUAL(ins.rd, 5, "LW: rd");
        ASSERT_EQUAL(ins.rs1, 2, "LW: rs1");
        ASSERT_EQUAL(ins.imm, 12u, "LW: imm");
    }

    // --- Test S-Type ---
    {
        // Assembly: sw t1, -20(sp)  (sw x6, -20(x2))
        const uint32_t instruction_word = 0xfe612a23;
        Instruction ins = Instruction::from(instruction_word);
        ASSERT_EQUAL(ins.header.ins_type, SW, "SW: Type");
        ASSERT_EQUAL(ins.rs1, 2, "SW: rs1");
        ASSERT_EQUAL(ins.rs2, 6, "SW: rs2");
        ASSERT_EQUAL(ins.imm, 0xfffffff4, "SW: imm (sign-extended)");
    }

    // --- Test B-Type ---
    {
        // Assembly: bne t0, t1, -16 (bne x5, x6, -16)
        const uint32_t instruction_word = 0xfe629863;
        Instruction ins = Instruction::from(instruction_word);
        ASSERT_EQUAL(ins.header.ins_type, BNE, "BNE: Type");
        ASSERT_EQUAL(ins.rs1, 5, "BNE: rs1");
        ASSERT_EQUAL(ins.rs2, 6, "BNE: rs2");
        ASSERT_EQUAL(ins.imm, 0xfffff7f0, "BNE: imm (sign-extended)");
    }

    // --- Test U-Type ---
    {
        // Assembly: lui a0, 0xDEADB
        const uint32_t instruction_word = 0xdeadb537;
        Instruction ins = Instruction::from(instruction_word);
        ASSERT_EQUAL(ins.header.ins_type, LUI, "LUI: Type");
        ASSERT_EQUAL(ins.rd, 10, "LUI: rd");
        ASSERT_EQUAL(ins.imm, 0xDEADB000, "LUI: imm");
    }

    // --- Test J-Type ---
    {
        // Assembly: jal ra, -20
        const uint32_t instruction_word = 0xfedff0ef;
        Instruction ins = Instruction::from(instruction_word);
        ASSERT_EQUAL(ins.header.ins_type, JAL, "JAL: Type");
        ASSERT_EQUAL(ins.rd, 1, "JAL: rd");
        ASSERT_EQUAL(ins.imm, 0xFFFFFFEC, "JAL: imm (sign-extended)");
    }

    // --- Test Error Handling ---
    TEST_THROWS([]{ Instruction::from(0xFFFFFFFF); }, "Throws on invalid opcode");
    TEST_THROWS([]{ Instruction::from(0x00003063); }, "Throws on invalid B-Type func3");
    TEST_THROWS([]{ Instruction::from(0x04C58533); }, "Throws on invalid R-Type func7");

    {
        Instruction ins = noop;
        ASSERT_EQUAL(ins.header.ins_type, NOOP, "NOOP: Type");
    }

    // --- Final Report ---
    std::cout << "--------------------------------------\n";
    if (failures == 0) {
        std::cout << "All tests passed successfully!\n";
        return 0; // Success
    } else {
        std::cerr << failures << " test(s) failed.\n";
        return 1; // Failure
    }
}