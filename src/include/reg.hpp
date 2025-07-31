// reg.hpp
// - declares the register file

#pragma once

#include "reset.hpp"
#include "rob_types.hpp"  // For rob_pointer_t
#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    
    enum RegName {
        ZERO = 0,  // Zero register
        RA = 1,  // Return address
        A0 = 10,  // Argument 0
    };

    struct Register {
        ConsciouslyBuffered<uint32_t> value;
        ConsciouslyBuffered<rob_pointer_t> host;
        ConsciouslyBuffered<bool> has_host;

        Register();
    };

    class RegisterFile : public Resettable {
        Register registers[C::register_file_size];

    public:
        [[nodiscard]] std::optional<rob_pointer_t> read_host(int index) const;
        [[nodiscard]] uint32_t read(int index) const;
        void write(int index, uint32_t value);
        void write_host(int index, const rob_pointer_t &host);
        void clear_host(int index, const rob_pointer_t &host);
        
        // Implement Resettable interface
        void on_reset(const ResetData& reset_data) override;

        void print_state();

        // dumps the NEW values of the registers as an array (debug hack)
        [[nodiscard]] std::array<uint32_t, C::register_file_size> dump_as_array() const;
    };

}  // namespace norb::riscv
