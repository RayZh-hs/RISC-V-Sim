// lsb.cpp
// - implements the LSB

#include "lsb.hpp"

#include "utility/extend.hpp"

namespace norb::riscv {

    void LoadStoreBuffer::load_memory(const std::string& mem_path) { memory = std::make_unique<Memory>(mem_path); }

    void LoadStoreBuffer::set_commit_bus(TemporaryBus<rob_pointer_t>& bus) { bus_rob_commit.connect(bus); }

    // Auxiliary function to read a single byte, checking modification_list first
    uint8_t LoadStoreBuffer::read_byte(uint32_t addr) {
        // Check modification_list first for changed entries
        if (modification_list.contains(addr)) {
            return modification_list[addr];
        } else {
            // Read from memory
            const uint8_t word = memory->read_byte(addr);
            return word;
        }
    }

    uint32_t LoadStoreBuffer::get_instruction(uint32_t index) const {
        if (!memory) {
            throw std::runtime_error("Memory not loaded.");
        }
        return memory->read_word(index);
    }

    void LoadStoreBuffer::on_issue() { resolver.listen_inbound(); }

    void LoadStoreBuffer::on_execute() {
        // If there is a delayed entry, execute it and wait for broadcast
        const auto delayed_entry = delayer.pop();
        if (delayed_entry.has_value()) {
            const auto entry = delayed_entry.value();
            switch (entry.type) {
                case LB:
                case LBU:
                    {
                        // Load byte (sign-extended)
                        const uint32_t addr = entry.vj + entry.imm;
                        const uint8_t byte_val = read_byte(addr);

                        // Sign extend from 8 bits to 32 bits
                        const auto result = (entry.type == LB ? signed_extend(byte_val) : unsigned_extend(byte_val));

                        log.as(LogLevel::INFO) << "[LSB] Broadcasting: Broadcast(pointer=" << entry.rob_pointer.repr()
                                               << ", ans=" << result << ")";
                        resolver.submit_executed_entry(entry.rob_pointer, result);
                        break;
                    }

                case LH:
                case LHU:
                    {
                        // Load halfword (sign-extended)
                        uint32_t addr = entry.vj + entry.imm;

                        uint8_t byte0 = read_byte(addr);
                        uint8_t byte1 = read_byte(addr + 1);
                        uint16_t halfword_val = byte0 | (byte1 << 8);

                        // Sign extend from 16 bits to 32 bits
                        const auto result =
                            (entry.type == LH ? signed_extend(halfword_val) : unsigned_extend(halfword_val));

                        log.as(LogLevel::INFO) << "[LSB] Broadcasting: Broadcast(pointer=" << entry.rob_pointer.repr()
                                               << ", ans=" << result << ")";
                        resolver.submit_executed_entry(entry.rob_pointer, result);
                        break;
                    }

                case LW:
                    {
                        // Load word
                        uint32_t addr = entry.vj + entry.imm;

                        uint8_t byte0 = read_byte(addr);
                        uint8_t byte1 = read_byte(addr + 1);
                        uint8_t byte2 = read_byte(addr + 2);
                        uint8_t byte3 = read_byte(addr + 3);
                        uint32_t word_val = byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24);

                        log.as(LogLevel::INFO) << "[LSB] Broadcasting: Broadcast(pointer=" << entry.rob_pointer.repr()
                                               << ", ans=" << word_val << ")";
                        resolver.submit_executed_entry(entry.rob_pointer, word_val);
                        break;
                    }

                case SB:
                    {
                        // Store byte
                        uint32_t addr = entry.vj + entry.imm;
                        uint8_t byte_val = entry.vk & 0xFF;

                        // Write to modification_list and modification_blame
                        modification_list[addr] = byte_val;
                        modification_blame.insert({entry.rob_pointer, addr});

                        // the broadcast here is not necessary (return type void)
                        // submit is only intended for clearing the buffer
                        log.as(LogLevel::DEBUG)
                            << "[LSB] Storing byte: addr=" << addr << ", value=" << static_cast<int>(byte_val);
                        resolver.submit_executed_entry(entry.rob_pointer, 0);
                        break;
                    }

                case SH:
                    {
                        // Store halfword
                        uint32_t addr = entry.vj + entry.imm;
                        uint16_t halfword_val = entry.vk & 0xFFFF;

                        // todo | Check if Endianness is correct
                        modification_list[addr] = halfword_val & 0xFF;
                        modification_list[addr + 1] = (halfword_val >> 8) & 0xFF;
                        modification_blame.insert({entry.rob_pointer, addr});
                        modification_blame.insert({entry.rob_pointer, addr + 1});

                        log.as(LogLevel::DEBUG)
                            << "[LSB] Storing halfword: addr=" << addr << ", value=" << halfword_val;
                        resolver.submit_executed_entry(entry.rob_pointer, 0);
                        break;
                    }

                case SW:
                    {
                        // Store word
                        uint32_t addr = entry.vj + entry.imm;
                        uint32_t word_val = entry.vk;

                        modification_list[addr] = word_val & 0xFF;
                        modification_list[addr + 1] = (word_val >> 8) & 0xFF;
                        modification_list[addr + 2] = (word_val >> 16) & 0xFF;
                        modification_list[addr + 3] = (word_val >> 24) & 0xFF;
                        modification_blame.insert({entry.rob_pointer, addr});
                        modification_blame.insert({entry.rob_pointer, addr + 1});
                        modification_blame.insert({entry.rob_pointer, addr + 2});
                        modification_blame.insert({entry.rob_pointer, addr + 3});

                        log.as(LogLevel::DEBUG) << "[LSB] Storing word: addr=" << addr << ", value=" << word_val;
                        resolver.submit_executed_entry(entry.rob_pointer, 0);
                        break;
                    }

                default:
                    throw std::runtime_error("Unsupported instruction type in LoadStoreBuffer execution: " +
                                             std::to_string(static_cast<int>(entry.type)));
                    break;
            }
        }
    }

    void LoadStoreBuffer::on_broadcast() {
        // Listen for well-formed instructions
        resolver.listen_broadcast();
        // Resolve ready entries
        auto entry = resolver.get_ready_entry();
        if (entry.has_value()) {
            delayer.push(entry.value());
        }
    }

    void LoadStoreBuffer::on_commit() {
        // forward the commit to the memory
        const auto committed = bus_rob_commit.read();
        if (committed.has_value()) {
            // Clear the modification list for the committed ROB entry
            auto [start, end] = modification_blame.equal_range(committed.value());
            for (auto it = start; it != end; ++it) {
                log.as(LogLevel::DEBUG) << "Writing committed modification to memory: MemoryModification("
                                        << "address=" << it->second << ", value=" << uint(modification_list[it->second])
                                        << ")";
                memory->write_byte(it->second, modification_list[it->second]);
                modification_list.erase(it->second);
            }
            modification_blame.erase(start, end);
        }
    }


    void LoadStoreBuffer::on_reset(const ResetData& reset_data) {
        if (reset_data.reset_signal) {
            log.as(LogLevel::INFO) << "[LSB] Reset signal received, clearing all buffers";

            // Clear resolver buffer
            resolver.clear();

            // Clear modification lists
            modification_list.clear();
            modification_blame.clear();

            // Note: delayer will naturally clear itself as items expire
            // Memory is not cleared as it represents persistent storage

            log.as(LogLevel::INFO) << "[LSB] Reset completed, all buffers cleared";
        }
    }

}  // namespace norb::riscv
