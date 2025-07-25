// rob.hpp
// - implements the ReOrder Buffer

#pragma once

#include "decoder.hpp"
#include "third_party/queue.hpp"
#include "utility/bus.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {

    enum ROBEntryStatus { EMPTY, READY, EXECUTING, COMMITTED };

    class ROBEntry {
        Instruction instruction;
        ROBEntryStatus status;
        uint32_t result;

    public:
        ROBEntry() : instruction(noop), status(EMPTY), result(0) {}
    };

    class ReOrderBuffer {
    public:
        using buffer_t = FixedBufferedQueue<ROBEntry, C::reorder_buffer_size>;
        using rob_pointer_t = buffer_t::iterator;
        
        // Communication buses
        Bus<bool> bus_rob_is_full;
        Bus<bool> bus_rob_has_committed_exit;
        Bus<Instruction> bus_con_next_instruction;

    private:
        buffer_t buffer_;

    public:
        ReOrderBuffer() = default;

        [[nodiscard]] bool full() const { return buffer_.full(); }

        [[nodiscard]] bool almost_full() const {
            return buffer_.size() >= C::reorder_buffer_size - 1;
        }

        [[nodiscard]] bool empty() const { return buffer_.empty(); }

        void write_instruction() {
            if (buffer_.full()) {
                return;
            }
            // get the latest instruction from the bus and write it to the buffer
            Instruction ins = bus_con_next_instruction.read();
            // update the fullness status
            bus_rob_is_full.write(buffer_.full());
            buffer_.emplace_back(ins, READY, 0);
        }
    };
}  // namespace norb::riscv
