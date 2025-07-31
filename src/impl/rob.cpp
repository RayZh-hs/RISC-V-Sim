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
            // Here the emplace back will take effect on the next cycle, so end() still points to the old end
            const auto it = main_buffer.end();
            //! Edge Case: The dependency IS BEING broadcast in cdb at the time of instruction fetch
            //! In such a scenario we need to retrieve the data from cdb as well
            log.as(LogLevel::DEBUG) << "[ROB] received and appended new instruction: " << ins.repr()
                                    << " at [rob=" << it.physical_index() << "]";
            const auto cdb_top = cdb_ref->read_as_optional();
            // record a snapshot in the buffer
            ResolverEntry resolver_entry;
            resolver_entry.type = ins.header.ins_type;
            resolver_entry.status = ResolverEntryStatus::PENDING;
            // resolve rs1
            if (ins.rs1 == 0 or register_file.read_host(ins.rs1) == std::nullopt) {
                // record as value
                resolver_entry.vj = register_file.read(ins.rs1);
                resolver_entry.j_is_ready = true;
            } else if (cdb_top.has_value() and cdb_top->rob_pointer == register_file.read_host(ins.rs1)) {
                // record as value from CDB
                resolver_entry.vj = cdb_top->value;
                resolver_entry.j_is_ready = true;
            } else {
                // record as dependency
                resolver_entry.qj = register_file.read_host(ins.rs1).value();
                //! try self-resolve for computed but not yet committed instructions
                const auto self_resolved = try_resolve_in_rob(resolver_entry.qj);
                if (self_resolved.has_value()) {
                    resolver_entry.vj = self_resolved.value();
                    resolver_entry.j_is_ready = true;
                } else {
                    resolver_entry.qj = register_file.read_host(ins.rs1).value();
                    resolver_entry.j_is_ready = false;
                }
            }
            // resolve rs2
            if (ins.rs2 == 0 or register_file.read_host(ins.rs2) == std::nullopt) {
                // record as value
                resolver_entry.vk = register_file.read(ins.rs2);
                resolver_entry.k_is_ready = true;
            } else if (cdb_top.has_value() and cdb_top->rob_pointer == register_file.read_host(ins.rs2)) {
                // record as value from CDB
                resolver_entry.vk = cdb_top->value;
                resolver_entry.k_is_ready = true;
            } else {
                // record as dependency
                resolver_entry.qk = register_file.read_host(ins.rs2).value();
                //! try self-resolve for computed but not yet committed instructions
                const auto self_resolved = try_resolve_in_rob(resolver_entry.qk);
                if (self_resolved.has_value()) {
                    resolver_entry.vk = self_resolved.value();
                    resolver_entry.k_is_ready = true;
                } else {
                    resolver_entry.qk = register_file.read_host(ins.rs2).value();
                    resolver_entry.k_is_ready = false;
                }
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
                register_file.write_host(ins.rd, it);
            }
        }
    }

    void ReOrderBuffer::on_issue() {
        auto &log = Logger::get();
        // we need to make sure that the cdb broadcast updates all READY objects
        const auto cdb_top = cdb_ref->read_as_optional();
        // get the first entry that is ready
        bool has_sent = false;
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
                        entry.status = ROBEntryStatus::ISSUED;

                        // Calculate result based on instruction type
                        switch (entry.instruction.header.ins_type) {
                            case LUI:
                                entry.result = entry.instruction.imm;
                                break;
                            case AUIPC:
                                entry.result = entry.instruction.pc + (entry.instruction.imm << 12);
                                break;
                            default:
                                log.as(LogLevel::ERROR)
                                    << "[ROB] Unknown REG instruction type: "
                                    << ins_type_names[static_cast<int>(entry.instruction.header.ins_type)];
                                return;
                        }

                        it.write(
                            entry);  // this will take effect on the next cycle, when the broadcast info will sink in
                        log.as(LogLevel::DEBUG)
                            << "[ROB] Executed REG instruction immediately: " << entry.instruction.repr();
                        // Broadcast immediately and on_broadcast in the next cycle change ISSUED to COMPUTED
                        // This is to ensure that other instructions dependent on the instruction will be alerted
                        cdb_ref->broadcast(BroadcastEntry{it, entry.result});
                        return;
                    default:
                        break;
                }

                if (chw != nullptr and not chw->has_data() and not has_sent) {
                    has_sent = true;
                    entry.status = ROBEntryStatus::ISSUED;
                    it.write(entry);
                    // Send to the corresponding channel
                    rob_resolver_buffer_t::iterator to_issue = it.that_of(&resolver_buffer);
                    // ! Make sure that the news get updated
                    ResolverEntry resolver_entry = *to_issue;
                    const bool changed = impl::update_with_broadcast(resolver_entry, cdb_top);
                    chw->write(resolver_entry);
                    if (changed) to_issue.write(resolver_entry);
                    log.as(LogLevel::DEBUG) << "[ROB] Issued instruction: " << entry.instruction.repr();
                    continue;
                } else {
                    rob_resolver_buffer_t::iterator will_issue = it.that_of(&resolver_buffer);
                    ResolverEntry resolver_entry = *will_issue;
                    const bool changed = impl::update_with_broadcast(resolver_entry, cdb_top);
                    if (changed) will_issue.write(resolver_entry);
                }
                log.as(LogLevel::DEBUG) << "[ROB] Cannot issue instruction: " << entry.instruction.repr();
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
            log.as(LogLevel::DEBUG) << "[ROB] No entry to commit: Front is not ready: " << front.instruction.repr();
            return;
        }

        // commit the first entry
        auto &ins = front.instruction;
        const uint32_t result = front.result;

        log.as(LogLevel::DEBUG) << "[ROB] Committing instruction: " << ins << " with result: " << result;

        if (ins.is_halt()) {
            log.as(LogLevel::INFO) << "[ROB] Program termination detected";
            bus_rob_has_committed_exit.write(true);
            return;
        }

        // Handle different types of commits
        switch (ins.header.ins_pos) {
            case InsPos::ALU:
            case InsPos::REG:
                // Write result to register file if rd != 0
                if (ins.rd != 0) {
                    register_file.write(ins.rd, result);
                    // Clear the host dependency since we're committing
                    register_file.clear_host(ins.rd);
                }
                break;

            case InsPos::LSB:
                // For loads, write to register file; for stores, notify LSB
                if (ins.header.ins_type == LB || ins.header.ins_type == LBU || ins.header.ins_type == LH ||
                    ins.header.ins_type == LHU || ins.header.ins_type == LW) {
                    // Load instruction - write to register
                    if (ins.rd != 0) {
                        register_file.write(ins.rd, result);
                        register_file.clear_host(ins.rd);
                    }
                } else {
                    // Store instruction - notify LSB via bus
                    bus_rob_commit.write(main_buffer.begin());
                }
                break;

            case InsPos::BRANCH:
                // Check if this was a wrong branch
                // If correct, then nothing needs be done
                log.as(LogLevel::DEBUG) << "[ROB] Handling committed branch: " << ins.repr();
                if (result != C::correct_branch_token) {
                    log.as(LogLevel::WARN) << "[ROB] Branch mis-prediction detected, flushing pipeline";
                    // Clear all entries after this one in ROB
                    // Write the correct pc into the rst bus to trigger rollback
                    uint32_t correct_pc = front.result;
                    bus_rst.write(ResetData(true, correct_pc));
                }
                // For JAL and JALR we will also need to write into rd
                if (ins.header.ins_type == InsType::JAL or ins.header.ins_type == InsType::JALR) {
                    register_file.write(ins.rd, ins.pc + 4);
                    register_file.clear_host(ins.rd);
                }
                break;

            default:
                log.as(LogLevel::ERROR) << "[ROB] Unknown instruction position for commit: "
                                        << static_cast<int>(ins.header.ins_pos);
                break;
        }

        // Remove the committed entry from buffers
        main_buffer.pop();
        resolver_buffer.pop();

        // dump the new committed state of the register file
        if (C::dump_registers_after_commit) {
            const auto new_regs = register_file.dump_as_array();
            reg_dumper.dump(ins.pc, new_regs);
        }

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

            // clear data flowing through the channels
            chan_rob_ba_next_instruction.clear();
            chan_rob_lsb_next_instruction.clear();
            chan_rob_rs_next_instruction.clear();

            log.as(LogLevel::INFO) << "[ROB] Reset completed, all buffers cleared";
        }
    }

    std::optional<uint32_t> ReOrderBuffer::try_resolve_in_rob(const rob_pointer_t &pointer) const {
        for (auto iter = main_buffer.begin(); iter != main_buffer.end(); ++iter) {
            const auto entry = iter.read();
            if (iter == pointer) {
                if (entry.instruction.header.ins_pos == InsPos::BRANCH) {
                    // this must be either jal or jalr
                    assert(entry.instruction.header.ins_type == JAL or entry.instruction.header.ins_type == JALR);
                    //! either case: rd = PC + 4
                    const uint32_t ret = entry.instruction.pc + 4;
                    return ret;
                } else if (entry.status == ROBEntryStatus::COMPUTED) {
                    log.as(LogLevel::DEBUG)
                        << "ROB self resolved pointer: " << pointer.repr() << " with value: " << entry.result;
                    return entry.result;
                }
            }
        }
        return std::nullopt;
    }

}  // namespace norb::riscv
