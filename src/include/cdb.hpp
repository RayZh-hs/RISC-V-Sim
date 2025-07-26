// cdb.hpp
// - implements the Common Data Bus for the RISC-V simulator

#pragma once

#include <queue>

#include "define/cdb.hpp"

namespace norb::riscv {
    // This is the only system in the RISC-V simulator that does not follow strict latch logic
    // It is designed so for simpler interface and to let multiple dataflows through
    class CommonDataBus {
        std::queue<BroadcastEntry> broadcast_queue_;

    public:
        CommonDataBus() = default;

        void broadcast(const BroadcastEntry& entry) {
            broadcast_queue_.push(entry);
        }

        [[nodiscard]] bool empty() const {
            return broadcast_queue_.empty();
        }

        [[nodiscard]] BroadcastEntry read() const {
            return broadcast_queue_.front();
        }

        // This method is called on falling edge
        void flush() {
            broadcast_queue_.pop();
        }

        void clear() {
            while (not broadcast_queue_.empty()) {
                broadcast_queue_.pop();
            }
        }
    };
}  // namespace norb::riscv
