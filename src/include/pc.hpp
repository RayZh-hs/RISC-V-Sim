// pc.hpp
// - implements the program counter for the RISC-V simulator

#pragma once

#include "reset.hpp"
#include "utility/constants.hpp"
#include <cstdint>
namespace C = norb::riscv::constants;

namespace norb::riscv
{
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
