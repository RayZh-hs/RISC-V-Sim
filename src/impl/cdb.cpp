// cdb.cpp
// - implements the Common Data Bus methods

#include "cdb.hpp"

namespace norb::riscv {

    BroadcastEntry::BroadcastEntry(rob_pointer_t rob_pointer, uint32_t value)
        : rob_pointer(rob_pointer), value(value) {}

    std::string BroadcastEntry::repr() const {
        return "BroadcastEntry(rob=" + std::to_string(rob_pointer.repr()) + ", val=" + std::to_string(value) + ")";
    }

    void CommonDataBus::broadcast(const BroadcastEntry& entry) {
        changes_queue_.push(entry);
    }

    bool CommonDataBus::empty() const {
        return broadcast_queue_.empty();
    }

    BroadcastEntry CommonDataBus::read() const {
        return broadcast_queue_.front();
    }

    std::optional<BroadcastEntry> CommonDataBus::read_as_optional() const {
        if (broadcast_queue_.empty())
            return std::nullopt;
        return broadcast_queue_.front();
    }


    void CommonDataBus::flush() {
        if (not broadcast_queue_.empty())
            broadcast_queue_.pop();
        // append changes
        while (not changes_queue_.empty()) {
            broadcast_queue_.push(changes_queue_.front());
            changes_queue_.pop();
        }
    }

    void CommonDataBus::clear() {
        while (not broadcast_queue_.empty()) {
            broadcast_queue_.pop();
        }
        while (not broadcast_queue_.empty()) {
            changes_queue_.pop();
        }
    }

}  // namespace norb::riscv
