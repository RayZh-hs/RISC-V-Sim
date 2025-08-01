// pc.hpp
// - implements the program counter for the RISC-V simulator

#pragma once

#include "reset.hpp"
#include "utility/constants.hpp"
#include <cstdint>
namespace C = norb::riscv::constants;

namespace norb::riscv
{
    // pc-related utils
    namespace utils {
        inline bool should_jump(const ResolvedInstructionEntry &entry) {
            switch (entry.type) {
                // Conditional branches
                case InsType::BEQ:
                    return entry.vj == entry.vk;
                case InsType::BNE:
                    return entry.vj != entry.vk;
                case InsType::BLT:
                    return static_cast<int32_t>(entry.vj) < static_cast<int32_t>(entry.vk);
                case InsType::BGE:
                    return static_cast<int32_t>(entry.vj) >= static_cast<int32_t>(entry.vk);
                case InsType::BLTU:
                    return entry.vj < entry.vk;
                case InsType::BGEU:
                    return entry.vj >= entry.vk;

                // Unconditional jumps
                case InsType::JAL:
                case InsType::JALR:
                    return true;

                default:
                    throw std::runtime_error("Unsupported instruction type for branching: " +
                                             ins_type_names[static_cast<int>(entry.type)]);
            }
        }

        inline uint32_t calc_pc(uint32_t pc, const ResolvedInstructionEntry &entry) {
            if (should_jump(entry)) {
                if (entry.type == InsType::JALR) {
                    // PC = rs1 + imm
                    return entry.vj + entry.imm;
                }
                return pc + entry.imm;  // Jump to target address
            } else {
                return pc + 4;  // Continue to next instruction
            }
        }
    }

    class ProgramCounter : public Resettable {
        C::b_uint32_t val;

    public:
        ProgramCounter() : val(0) {
            // Initialize the program counter to 0
            val.write(0);
        }

        void reset() {
            val = 0;
        }

        [[nodiscard]] uint32_t read() const {
            return val.read();
        }

        void write(uint32_t new_val) {
            val.write(new_val);
        }

        void add(uint32_t offset = 4) {
            val.write(val.read() + offset);
        }

        // Implement Resettable interface
        void on_reset(const ResetData& reset_data) override {
            if (reset_data.reset_signal) {
                val.write(reset_data.new_pc);
            }
        }
    };
} // namespace norb::riscv
