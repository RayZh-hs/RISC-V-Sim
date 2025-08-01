// dump.hpp
// - utilities for binary and hex representation

#pragma once

#include <bitset>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdint>

namespace norb {
    inline std::string hex(uint32_t x) {
        std::ostringstream oss;
        oss << "0x" << std::setfill('0') << std::setw(8) << std::hex << x;
        return oss.str();
    }

    inline std::string bin(uint32_t x) { return "0b" + std::bitset<32>(x).to_string(); }

    inline std::string dump_repr(uint32_t x) {
        return std::to_string(x) + " (" + hex(x) + ", " + bin(x) + ")";
    }

    inline std::string pad_with_zero(uint32_t x, int width = 4) {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(width) << x;
        return oss.str();
    }

    inline std::string btos(bool x) {
        return x ? "true" : "false";
    }
}  // namespace norb
