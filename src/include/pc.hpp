// pc.hpp
// - implements the program counter for the RISC-V simulator

#pragma once

#include "utility/constants.hpp"
namespace C = norb::riscv::constants;

namespace norb::riscv
{
    class ProgramCounter {
        C::b_uint32_t val;

    public:
        ProgramCounter() : val(0) {
            // Initialize the program counter to 0
            val.write(0);
        }

        void reset() {
            val = 0;
        }

        [[nodiscard]] u_int32_t read() const {
            return val.read();
        }

        void write(u_int32_t new_val) {
            val.write(new_val);
        }

        void add(u_int32_t offset = 4) {
            val.write(val.read() + offset);
        }
    };
} // namespace norb::riscv
