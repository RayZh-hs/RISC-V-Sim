// dump.hpp
// - utilities for binary and hex representation

#pragma once

#include <bitset>
#include <iomanip>
#include <sstream>
#include <string>

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
}  // namespace norb
