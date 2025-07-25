// mem.hpp
// - implements the memory and provides api to access it

#pragma once

#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    namespace impl {
        template <size_t size>
        void memory_decode(std::array<C::b_uint8_t, size>& mem, std::istream& is) {
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
        std::array<C::b_uint8_t, C::memory_size> memory;

    public:
        Memory(const std::string& path) {
            std::ifstream file(path);
            impl::memory_decode(memory, file);
        }

        // read 4 bytes of data from the memory, in Little Endian
        [[nodiscard]] uint32_t read(uint32_t index) const {
            if (index + 3 >= C::memory_size) {
                throw std::out_of_range("Memory read out of bounds at index: " + std::to_string(index));
            }
            // both | and + work here
            return (memory[index].read() | (memory[index + 1].read() << 8) | (memory[index + 2].read() << 16) |
                    (memory[index + 3].read() << 24));
        }
    };

    struct LoadStoreBufferEntry {
        uint32_t address;
    };

    // Load Store Buffer
    class LoadStoreBuffer {
    private:
        std::unique_ptr<Memory> memory;

    public:
        void load_memory(const std::string& mem_path) {
            memory = std::make_unique<Memory>(mem_path);
        }

        uint32_t get_instruction(uint32_t index) const {
            if (!memory) {
                throw std::runtime_error("Memory not loaded.");
            }
            return memory->read(index);
        }
    };
}  // namespace norb::riscv
