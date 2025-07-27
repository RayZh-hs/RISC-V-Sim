// reg.hpp
// - declares the register file

#pragma once

#include "rob.hpp"  // For rob_pointer_t
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
        Buffered<rob_pointer_t> host;
        Buffered<bool> has_host;

        Register();
    };

    class RegisterFile {
        Register registers[C::register_file_size];

    public:
        [[nodiscard]] std::optional<rob_pointer_t> read_host(int index) const;
        [[nodiscard]] uint32_t read(int index) const;
        void write(int index, uint32_t value);
        void write_host(int index, const rob_pointer_t &host);
    };

}  // namespace norb::riscv
