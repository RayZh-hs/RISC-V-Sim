// cdb.cpp
// - implements the Common Data Bus methods

#include "cdb.hpp"

namespace norb::riscv {

    BroadcastEntry::BroadcastEntry(rob_pointer_t rob_pointer, uint32_t value)
        : rob_pointer(rob_pointer), value(value) {}

    void CommonDataBus::broadcast(const BroadcastEntry& entry) {
        broadcast_queue_.push(entry);
    }

    bool CommonDataBus::empty() const {
        return broadcast_queue_.empty();
    }

    BroadcastEntry CommonDataBus::read() const {
        return broadcast_queue_.front();
    }

    void CommonDataBus::flush() {
        broadcast_queue_.pop();
    }

    void CommonDataBus::clear() {
        while (not broadcast_queue_.empty()) {
            broadcast_queue_.pop();
        }
    }

}  // namespace norb::riscv
