// constants.hpp
// - defines and holds all compile-time constants in the project and provides standard type definitions

#pragma once
#include "buffered.hpp"

namespace norb::riscv::constants {
    inline constexpr int memory_size = 4096;

    using buint8_t = norb::Buffered<uint8_t>;
    using buint16_t = norb::Buffered<uint16_t>;
    using buint32_t = norb::Buffered<uint32_t>;
}