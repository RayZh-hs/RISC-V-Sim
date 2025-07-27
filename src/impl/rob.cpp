// rob.cpp
// - implements the ReOrder Buffer

#include "rob.hpp"
#include "reg.hpp"
#include "third_party/logger.hpp"

namespace norb::riscv {
    
    ROBEntry::ROBEntry(Instruction ins, ROBEntryStatus status, uint32_t result) 
        : instruction(ins), status(status), result(result) {}

    bool ReOrderBuffer::full() const { 
        return buffer.full(); 
    }

    bool ReOrderBuffer::almost_full() const { 
        return buffer.size() >= C::reorder_buffer_size - 1; 
    }

    bool ReOrderBuffer::empty() const { 
        return buffer.empty(); 
    }

    void ReOrderBuffer::instruction_fetch() {
        if (buffer.full()) {
            // do not read. this will automatically cause the channel to shut down on the other side
            return;
        }
        auto &log = Logger::get();
        if (chan_con_rob_next_instruction.has_data()) {
            // get the latest instruction from the bus and write it to the buffer
            Instruction ins = chan_con_rob_next_instruction.read();
            buffer.emplace_back(ins, ROBEntryStatus::READY, 0);
            log.as(LogLevel::DEBUG) << "[ROB] received and appended new instruction: " << ins;
            // then update the dependency in the register file
            if (ins.rd != 0) {
                register_file.write_host(ins.rd, --buffer.end());
            }
        }
    }

    void ReOrderBuffer::issue() {
        auto &log = Logger::get();
        // get the first entry that is ready
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            if (it.read().status == ROBEntryStatus::READY) {
                auto entry = *it;
                ChannelWriter<ResolverEntry> *chw = nullptr;

                switch (entry.instruction.header.ins_pos) {
                    case InsPos::ALU:
                        chw = &chan_rob_rs_next_instruction;
                        break;
                    case InsPos::LSB:
                        // todo
                        break;
                    case InsPos::BRANCH:
                        // todo
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
                    ResolverEntry to_issue;
                    const auto &ins = entry.instruction;
                    to_issue.type = ins.header.ins_type;
                    if (ins.rs1 != 0 and register_file.read_host(ins.rs1) != std::nullopt) {
                        // record as dependency
                        to_issue.qj = register_file.read_host(ins.rs1).value();
                        to_issue.j_is_ready = false;
                    } else {
                        // record as value
                        to_issue.vj = register_file.read(ins.rs1);
                        to_issue.j_is_ready = true;
                    }
                    if (ins.rs2 != 0 and register_file.read_host(ins.rs2) != std::nullopt) {
                        // record as dependency
                        to_issue.qk = register_file.read_host(ins.rs2).value();
                        to_issue.k_is_ready = false;
                    } else {
                        // record as value
                        to_issue.vk = register_file.read(ins.rs2);
                        to_issue.k_is_ready = true;
                    }
                    to_issue.status = (to_issue.k_is_ready and to_issue.j_is_ready) ? ResolverEntryStatus::READY
                                                                                    : ResolverEntryStatus::PENDING;
                    to_issue.rob_pointer = it;
                    chw->write(to_issue);
                    log.as(LogLevel::DEBUG) << "[ROB] Issued instruction: " << entry.instruction;
                    return;
                }
                log.as(LogLevel::DEBUG) << "[ROB] Cannot issue instruction: " << entry.instruction;
            }
        }
        log.as(LogLevel::DEBUG) << "[ROB] No instruction issuable";
    }

}  // namespace norb::riscv
