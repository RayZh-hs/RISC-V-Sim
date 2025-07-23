// mem.hpp
// - implements the memory and provides api to access it

#pragma once

#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    namespace impl {
        template <size_t size>
        void memory_decode(std::array<C::buint8_t, size>& mem, std::istream& is) {
            size_t current_pos = 0;
            std::string token;

            while (is >> token) {
                if (token.empty()) {
                    continue;
                }

                if (token[0] == '@') {
                    // address token
                    std::string addr_str = token.substr(1);
                    try {
                        current_pos = std::stoull(addr_str, nullptr, 16);
                    } catch (const std::exception& e) {
                        throw std::runtime_error("Invalid address format: " + token);
                    }
                } else {
                    // data token
                    unsigned long byte_val;
                    try {
                        byte_val = std::stoul(token, nullptr, 16);
                    } catch (const std::exception& e) {
                        throw std::runtime_error("Invalid byte format: " + token);
                    }
                    if (byte_val > 0xFF) {
                        throw std::runtime_error("Byte value out of range ( > 0xFF): " + token);
                    }

                    if (current_pos >= size) {
                        std::stringstream ss;
                        ss << "Memory write out of bounds. Address 0x" << std::hex << current_pos
                           << " exceeds memory size " << std::dec << size << ".";
                        throw std::out_of_range(ss.str());
                    }

                    // Write the byte to memory and advance the position.
                    mem[current_pos].write(byte_val);
                    ++current_pos;
                }
            }
        }
    }  // namespace impl

    class Memory {
        std::array<C::buint8_t, C::memory_size> memory;

    public:
        Memory(const std::string& path) {
            std::ifstream file(path);
            impl::memory_decode(memory, file);
        }
    };
}  // namespace riscv
