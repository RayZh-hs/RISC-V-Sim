// cdb.hpp
// - implements the Common Data Bus for the RISC-V simulator

#pragma once

#include <stack>

#include "define/cdb.hpp"

namespace norb::riscv {
    // This is the only system in the RISC-V simulator that does not follow strict latch logic
    // It is designed so for simpler interface and to let multiple dataflows through
    class CommonDataBus {
        std::stack<BroadcastEntry> broadcast_stack_;

    public:
        CommonDataBus() = default;

        void broadcast(const BroadcastEntry& entry) {
            broadcast_stack_.push(entry);
        }

        bool empty() const {
            return broadcast_stack_.empty();
        }

        BroadcastEntry read() {
            return broadcast_stack_.top();
        }

        // This method is called on falling edge
        void flush() {
            broadcast_stack_.pop();
        }

        void clear() {
            while (not broadcast_stack_.empty()) {
                broadcast_stack_.pop();
            }
        }
    };
}  // namespace norb::riscv
