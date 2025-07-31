// mem.hpp
// - implements the memory and provides api to access it

#pragma once

#include <array>
#include <dep.hpp>
#include <fstream>
#include <sstream>
#include <cstdint>

#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    namespace impl {
        template <size_t size>
        void memory_decode(std::array<uint8_t, size>& mem, std::istream& is) {
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
                    mem[current_pos] = byte_val;
                    ++current_pos;
                }
            }
        }
    }  // namespace impl

    class Memory {
        std::array<uint8_t, C::memory_size> memory{};

    public:
        explicit Memory(const std::string& path) {
            std::ifstream file(path);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open memory file: " + path);
            }
            impl::memory_decode(memory, file);
        }

        // Constructor to read from stdin
        explicit Memory() {
            impl::memory_decode(memory, std::cin);
        }

        // read 4 bytes of data from the memory, in Little Endian
        [[nodiscard]] uint32_t read_word(uint32_t index) const {
            if (index + 3 >= C::memory_size) {
                throw std::out_of_range("Memory read out of bounds at index: " + std::to_string(index));
            }
            // both | and + work here
            return (memory[index] | (memory[index + 1] << 8) | (memory[index + 2] << 16) |
                    (memory[index + 3] << 24));
        }

        [[nodiscard]] uint8_t read_byte(uint32_t index) const {
            if (index >= C::memory_size) {
                throw std::out_of_range("Memory read out of bounds at index: " + std::to_string(index));
            }
            return memory[index];
        }

        void write_byte(uint32_t index, uint8_t value) {
            if (index >= C::memory_size) {
                throw std::out_of_range("Memory write out of bounds at index: " + std::to_string(index));
            }
            memory[index] = (value);
        }
    };

}  // namespace norb::riscv
