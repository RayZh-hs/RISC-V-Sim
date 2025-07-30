// constants.hpp
// - defines and holds all compile-time constants in the project and provides standard type definitions

#pragma once
#include <cstdint>

#include "buffered.hpp"

namespace norb::riscv::constants {
    inline constexpr int register_file_size = 32;
    inline constexpr int reorder_buffer_size = 64;
    inline constexpr int memory_size = 4096 * 1024;

    inline constexpr int reservation_station_size = 32;
    inline constexpr int load_store_buffer_size = 32;
    inline constexpr int branch_analyzer_size = 32;

    inline constexpr uint32_t correct_branch_token = -1;

    inline constexpr int alu_calc_delay = 1;
    inline constexpr int mem_access_delay = 3;

    inline constexpr int loop_timeout = 150;

    constexpr bool peek_resolvers_after_cycle = true;

    using b_uint8_t = Buffered<uint8_t>;
    using b_uint16_t = Buffered<uint16_t>;
    using b_uint32_t = Buffered<uint32_t>;

    // project-specific typedef
    using robId_t = uint8_t;
    using b_robId_t = Buffered<robId_t>;
}  // namespace norb::riscv::constants
