// define/cdb.hpp
// - defines types used in the Common Data Bus (CDB)

#pragma once

#include <cstdint>

#include "define/rob.hpp"

namespace norb::riscv
{
    struct BroadcastEntry {
        rob_pointer_t rob_pointer;
        uint32_t value;

        BroadcastEntry(rob_pointer_t rob_pointer, uint32_t value)
            : rob_pointer(rob_pointer), value(value) {}
    };

    // Forward declaration for Common Data Bus
    class CommonDataBus;
} // namespace norb::riscv
