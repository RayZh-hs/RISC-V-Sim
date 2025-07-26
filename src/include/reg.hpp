// reg.hpp
// - implements the register file

#pragma once

#include "rob.hpp"
#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {

    enum RegName {
        ZERO = 0,  // Zero register
        RA = 1,  // Return address
        A0 = 10,  // Argument 0
    };

    struct Register {
        C::b_uint32_t value;
        rob_pointer_t host{};

        Register() : value(0) {}
    };

    class RegisterFile {
        Register registers[C::register_file_size];

    public:
        [[nodiscard]] uint32_t read(int index) const { return registers[index].value.read(); }

        void write(int index, uint32_t value) { registers[index].value.write(value); }
    };
}  // namespace norb::riscv
