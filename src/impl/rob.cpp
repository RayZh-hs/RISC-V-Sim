// rob.cpp
// - implements the ReOrder Buffer

#include "rob.hpp"

#include "decoder.hpp"
#include "reg.hpp"
#include "third_party/logger.hpp"

namespace norb::riscv {

    ROBEntry::ROBEntry(Instruction ins, ROBEntryStatus status, uint32_t result) :
        instruction(ins), status(status), result(result) {}

    bool ReOrderBuffer::full() const { return main_buffer.full(); }

    bool ReOrderBuffer::almost_full() const { return main_buffer.size() >= C::reorder_buffer_size - 1; }

    bool ReOrderBuffer::empty() const { return main_buffer.empty(); }

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
            log.as(LogLevel::DEBUG) << "[ROB] received and appended new instruction: " << ins.repr();
            // record a snapshot in the buffer
            ResolverEntry resolver_entry;
            resolver_entry.type = ins.header.ins_type;
            resolver_entry.status = ResolverEntryStatus::PENDING;
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
            resolver_entry.status = (resolver_entry.k_is_ready and resolver_entry.j_is_ready)
                ? ResolverEntryStatus::READY
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
                        // Handle register operations immediately (LUI, AUIPC)
                        // These operations don't need to go through execution units
                        entry.status = ROBEntryStatus::COMPUTED;

                        // Calculate result based on instruction type
                        switch (entry.instruction.header.ins_type) {
                            case LUI:
                                entry.result = entry.instruction.imm;
                                break;
                            case AUIPC:
                                entry.result = entry.instruction.pc + entry.instruction.imm;
                                break;
                            default:
                                log.as(LogLevel::ERROR)
                                    << "[ROB] Unknown REG instruction type: "
                                    << ins_type_names[static_cast<int>(entry.instruction.header.ins_type)];
                                return;
                        }

                        it.write(entry);
                        log.as(LogLevel::DEBUG) << "[ROB] Executed REG instruction immediately: " << entry.instruction;
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

    void ReOrderBuffer::on_broadcast() {
        assert(cdb_ref != nullptr);
        // read from the common data bus
        if (not cdb_ref->empty()) {
            const auto broadcast_entry = cdb_ref->read();
            // edit according to the entry
            auto pointer = broadcast_entry.rob_pointer;
            auto rob_entry = pointer.read();
            rob_entry.result = broadcast_entry.value;
            rob_entry.status = ROBEntryStatus::COMPUTED;
            pointer.write(rob_entry);
        }
    }

    void ReOrderBuffer::on_commit() {
        // examine the first entry in the buffer
        if (main_buffer.empty()) {
            return;
        }
        assert(not resolver_buffer.empty());
        auto front = main_buffer.front();
        if (front.status != ROBEntryStatus::COMPUTED) {
            // nothing to commit
            log.as(LogLevel::DEBUG) << "[ROB] No entry to commit: Front is not ready";
            return;
        }

        // commit the first entry
        auto &ins = front.instruction;
        auto result = front.result;

        log.as(LogLevel::DEBUG) << "[ROB] Committing instruction: " << ins << " with result: " << result;

        // Handle different types of commits
        switch (ins.header.ins_pos) {
            case InsPos::ALU:
            case InsPos::REG:
                // Write result to register file if rd != 0
                if (ins.rd != 0) {
                    register_file.write(ins.rd, result);
                    // Clear the host dependency since we're committing
                    register_file.write_host(ins.rd, rob_nullptr);
                }
                break;

            case InsPos::LSB:
                // For loads, write to register file; for stores, notify LSB
                if (ins.header.ins_type == LB || ins.header.ins_type == LBU || ins.header.ins_type == LH ||
                    ins.header.ins_type == LHU || ins.header.ins_type == LW) {
                    // Load instruction - write to register
                    if (ins.rd != 0) {
                        register_file.write(ins.rd, result);
                        register_file.write_host(ins.rd, rob_nullptr);
                    }
                } else {
                    // Store instruction - notify LSB via bus
                    bus_con_commit.write(main_buffer.begin());
                }
                break;

            case InsPos::BRANCH:
                // Check if this was a wrong branch
                // If correct, then nothing needs be done
                if (not result == C::correct_branch_token) {
                    log.as(LogLevel::WARN) << "[ROB] Branch mis-prediction detected, flushing pipeline";
                    // Clear all entries after this one in ROB
                    // Write the correct pc into the rst bus to trigger rollback
                    uint32_t correct_pc = front.result;
                    bus_rst.write(ResetData(true, correct_pc));
                }
                break;

            default:
                log.as(LogLevel::ERROR) << "[ROB] Unknown instruction position for commit: "
                                        << static_cast<int>(ins.header.ins_pos);
                break;
        }

        if (ins.is_halt()) {
            log.as(LogLevel::INFO) << "[ROB] Program termination detected";
            bus_rob_has_committed_exit.write(true);
        }

        // Remove the committed entry from buffers
        main_buffer.pop();
        resolver_buffer.pop();

        log.as(LogLevel::INFO) << "[ROB] Successfully committed instruction: " << ins;
    }

    void ReOrderBuffer::on_reset(const ResetData &reset_data) {
        if (reset_data.reset_signal) {
            auto &log = Logger::get();
            log.as(LogLevel::INFO) << "[ROB] Reset signal received, clearing all buffers";

            // Clear main buffer
            main_buffer.clear();

            // Clear resolver buffer
            resolver_buffer.clear();

            // Clear all host dependencies in register file
            for (int i = 1; i < C::register_file_size; ++i) {  // Skip x0 register
                register_file.write_host(i, rob_nullptr);
            }

            log.as(LogLevel::INFO) << "[ROB] Reset completed, all buffers cleared";
        }
    }

}  // namespace norb::riscv
