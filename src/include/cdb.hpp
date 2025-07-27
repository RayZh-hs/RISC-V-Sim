// cdb.hpp
// - implements the Common Data Bus for the RISC-V simulator

#pragma once

#include <queue>
#include <cstdint>
#include "rob_types.hpp"

namespace norb::riscv {

    struct BroadcastEntry {
        rob_pointer_t rob_pointer;
        uint32_t value{};

        BroadcastEntry(rob_pointer_t rob_pointer, uint32_t value);
        BroadcastEntry() = default;
    };

    // This is the only system in the RISC-V simulator that does not follow strict latch logic
    // It is designed so for simpler interface and to let multiple dataflows through
    class CommonDataBus {
        std::queue<BroadcastEntry> broadcast_queue_{};

    public:
        CommonDataBus() = default;

        void broadcast(const BroadcastEntry& entry);
        [[nodiscard]] bool empty() const;
        [[nodiscard]] BroadcastEntry read() const;

        // This method is called on falling edge
        void flush();
        void clear();
    };
}  // namespace norb::riscv
