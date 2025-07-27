// reset.hpp
// - defines reset data structures and types for global reset functionality

#pragma once

#include <cstdint>

namespace norb::riscv {
    // Reset data structure that carries the new PC value and reset signal
    struct ResetData {
        bool reset_signal = false;
        uint32_t new_pc = 0;

        ResetData() = default;
        ResetData(bool reset, uint32_t pc) : reset_signal(reset), new_pc(pc) {}
    };

    // Interface for components that can be reset
    class Resettable {
    public:
        virtual ~Resettable() = default;
        virtual void on_reset(const ResetData& reset_data) = 0;
    };

}  // namespace norb::riscv
