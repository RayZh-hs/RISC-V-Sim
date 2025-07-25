// constants.hpp
// - defines and holds all compile-time constants in the project and provides standard type definitions

#pragma once
#include "buffered.hpp"
#include <cstdint>

namespace norb::riscv::constants {
    inline constexpr int register_file_size = 32;
    inline constexpr int reorder_buffer_size = 64;
    inline constexpr int memory_size = 4096;

    using b_uint8_t = Buffered<uint8_t>;
    using b_uint16_t = Buffered<uint16_t>;
    using b_uint32_t = Buffered<uint32_t>;

    // project-specific typedef
    using robId_t = uint8_t;
    using b_robId_t = Buffered<robId_t>;
}

namespace norb {
    class AssertionError: public std::runtime_error {
    public:
        explicit AssertionError(const std::string& message) : std::runtime_error(message) {}
    };
}