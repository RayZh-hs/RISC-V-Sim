// define/rob.hpp
// - defines types used in the ReOrder Buffer

#pragma once

#include <cstdint>

#include "utility/constants.hpp"
#include "third_party/queue.hpp"
#include "decoder.hpp"
#include "define/cdr.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {

    enum class ROBEntryStatus { EMPTY, READY, ISSUED, COMMITTED };

    struct ROBEntry {
        Instruction instruction;
        ROBEntryStatus status;
        uint32_t result;

        ROBEntry() : instruction(noop), status(ROBEntryStatus::EMPTY), result(0) {}
    };

    using rob_buffer_t = FixedBufferedQueue<ROBEntry, C::reorder_buffer_size>;
    using rob_pointer_t = rob_buffer_t::iterator;

} // namespace norb::riscv