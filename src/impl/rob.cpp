// rob.cpp
// - implements the ReOrder Buffer

#include "rob.hpp"
#include "reg.hpp"
#include "third_party/logger.hpp"

namespace norb::riscv {
    
    ROBEntry::ROBEntry(Instruction ins, ROBEntryStatus status, uint32_t result) 
        : instruction(ins), status(status), result(result) {}

    bool ReOrderBuffer::full() const { 
        return main_buffer.full();
    }

    bool ReOrderBuffer::almost_full() const { 
        return main_buffer.size() >= C::reorder_buffer_size - 1;
    }

    bool ReOrderBuffer::empty() const { 
        return main_buffer.empty();
    }

    void ReOrderBuffer::instruction_fetch() {
        if (main_buffer.full()) {
            // do not read. this will automatically cause the channel to shut down on the other side
            return;
        }
        auto &log = Logger::get();
        if (chan_con_rob_next_instruction.has_data()) {
            // get the latest instruction from the bus and write it to the buffer
            Instruction ins = chan_con_rob_next_instruction.read();
            main_buffer.emplace_back(ins, ROBEntryStatus::READY, 0);
            const auto it = main_buffer.end();
            log.as(LogLevel::DEBUG) << "[ROB] received and appended new instruction: " << ins;
            // record a snapshot in the buffer
            ResolverEntry resolver_entry;
            resolver_entry.type = ins.header.ins_type;
            if (ins.rs1 != 0 and register_file.read_host(ins.rs1) != std::nullopt) {
                // record as dependency
                resolver_entry.qj = register_file.read_host(ins.rs1).value();
                resolver_entry.j_is_ready = false;
            } else {
                // record as value
                resolver_entry.vj = register_file.read(ins.rs1);
                resolver_entry.j_is_ready = true;
            }
            if (ins.rs2 != 0 and register_file.read_host(ins.rs2) != std::nullopt) {
                // record as dependency
                resolver_entry.qk = register_file.read_host(ins.rs2).value();
                resolver_entry.k_is_ready = false;
            } else {
                // record as value
                resolver_entry.vk = register_file.read(ins.rs2);
                resolver_entry.k_is_ready = true;
            }
            resolver_entry.status = (resolver_entry.k_is_ready and resolver_entry.j_is_ready) ? ResolverEntryStatus::READY
                                                                            : ResolverEntryStatus::PENDING;
            resolver_entry.rob_pointer = it;
            resolver_entry.imm = ins.imm;
            resolver_entry.had_jumped = ins.had_jumped;
            resolver_entry.pc = ins.pc;
            // record the resolver entry
            resolver_buffer.push_back(resolver_entry);
            // then update the dependency in the register file
            if (ins.rd != 0) {
                register_file.write_host(ins.rd, --main_buffer.end());
            }
        }
    }

    void ReOrderBuffer::on_issue() {
        auto &log = Logger::get();
        // get the first entry that is ready
        for (auto it = main_buffer.begin(); it != main_buffer.end(); ++it) {
            if (it.read().status == ROBEntryStatus::READY) {
                auto entry = *it;
                ChannelWriter<ResolverEntry> *chw = nullptr;

                switch (entry.instruction.header.ins_pos) {
                    case InsPos::ALU:
                        chw = &chan_rob_rs_next_instruction;
                        break;
                    case InsPos::LSB:
                        chw = &chan_rob_lsb_next_instruction;
                        break;
                    case InsPos::BRANCH:
                        chw = &chan_rob_ba_next_instruction;
                        break;
                    case InsPos::REG:
                        // todo
                        return;
                    default:
                        break;
                }

                if (chw != nullptr and chw->has_data()) {
                    entry.status = ROBEntryStatus::ISSUED;
                    it.write(entry);
                    // Send to the corresponding channel
                    const rob_resolver_buffer_t::iterator to_issue = it.that_of(&resolver_buffer);
                    chw->write(*to_issue);
                    log.as(LogLevel::DEBUG) << "[ROB] Issued instruction: " << entry.instruction;
                    return;
                }
                log.as(LogLevel::DEBUG) << "[ROB] Cannot issue instruction: " << entry.instruction;
            }
        }
        log.as(LogLevel::DEBUG) << "[ROB] No instruction issuable";
    }

    void ReOrderBuffer::on_commit() {

    }

}  // namespace norb::riscv
