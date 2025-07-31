// control.cpp
// - implements entrypoint-level control functions for the entire risc-v system

#include "control.hpp"

#include <utility/dump.hpp>

#include "third_party/logger.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {

    void RISCV_Simulator::connect_buses() {
        bus_rob_has_committed_exit.connect(rob.bus_rob_has_committed_exit);
        bus_rst.connect(rob.bus_rst);
        lsb.set_commit_bus(rob.bus_rob_commit);
    }

    void RISCV_Simulator::connect_channels() {
        make_channel(chan_con_rob_next_instruction, rob.chan_con_rob_next_instruction);
        rs.resolver.bind_inbound_to(rob.chan_rob_rs_next_instruction);
        lsb.resolver.bind_inbound_to(rob.chan_rob_lsb_next_instruction);
        ba.resolver.bind_inbound_to(rob.chan_rob_ba_next_instruction);
        rs.resolver.load_cdb(rob.cdb_ref);
        lsb.resolver.load_cdb(rob.cdb_ref);
        ba.resolver.load_cdb(rob.cdb_ref);
    }

    void RISCV_Simulator::print_result() const {
        auto &log = Logger::get();
        log.as(LogLevel::INFO) << "Program finished. Final state of registers:";
        for (int i = 0; i < C::register_file_size; ++i) {
            log.as(LogLevel::INFO) << "x" << i << ": " << reg.read(i);
        }
        // output the return value of the program (stored in x10)
        std::cout << (reg.read(RegName::A0) & 0xff) << std::endl;
    }

    void RISCV_Simulator::print_cdb_info() const {
        const auto info = cdb->read_as_optional();
        if (info.has_value())
            log.as(LogLevel::DEBUG) << "[CDB] This cycle: Broadcasting " << info->repr();
        else
            log.as(LogLevel::DEBUG) << "[CDB] This cycle: No broadcast data";
    }

    void RISCV_Simulator::instruction_fetch() {
        if (!chan_con_rob_next_instruction.has_data()) {
            // we can write into it
            const uint32_t raw_ins = lsb.get_instruction(pc.read());
            Instruction ins = noop;
            try {
                ins = Instruction::from(raw_ins);
                log.as(LogLevel::DEBUG) << "[CONTROL] Fetched instruction: " << ins.repr();
            } catch (...) {
                log.as(LogLevel::WARN) << "[CONTROL] Malformed instruction: Cannot decode raw_ins=" << raw_ins;
            }
            // ask the branch analyzer to predict the pc
            // it should return pc + 4 if ins is not a branch instruction
            const auto predicted_pc = ba.predict_pc(pc.read(), ins);
            pc.write(predicted_pc);
            ins.had_jumped = (predicted_pc != pc.read() + 4);  // if imm == 4 this cannot be wrongly predicted
            ins.pc = pc.read();
            log.as(LogLevel::DEBUG) << "Current pc: " << norb::hex(pc.read()) << ", Predicted pc: " << norb::hex(predicted_pc);
            // now the instruction forwarded to the BA by ROB will have been tagged to ensure correct rollback
            if (ins.header.ins_type != NOOP)
                chan_con_rob_next_instruction.write(ins);
            else
                log.as(LogLevel::DEBUG) << "Skipping NOOP";
        }
        rob.instruction_fetch();
    }

    void RISCV_Simulator::issue() {
        // Issue instructions from the ReOrder Buffer to the Reservation Station and Load-Store Buffer
        rob.on_issue();
        rs.on_issue();
        lsb.on_issue();
        ba.on_issue();
    }

    void RISCV_Simulator::execute() {
        rs.on_execute();
        lsb.on_execute();
    }

    void RISCV_Simulator::write_and_broadcast() {
        rob.on_broadcast();
        rs.on_broadcast();
        lsb.on_broadcast();
        ba.on_broadcast();
    }

    void RISCV_Simulator::commit() {
        rob.on_commit();
        lsb.on_commit();
        reg.print_state();
    }

    void RISCV_Simulator::tidy() {
        if (C::peek_resolvers_after_cycle) {
            // debug each buffer
            log.as(LogLevel::INFO) << "[CONTROL] Peek RS resolver:";
            rs.resolver.print_content();
            log.as(LogLevel::INFO) << "[CONTROL] Peek LSB resolver:";
            lsb.resolver.print_content();
            log.as(LogLevel::INFO) << "[CONTROL] Peek BA resolver:";
            ba.resolver.print_content();
        }
        // after each cycle, reset the zero register
        reg.write(RegName::ZERO, 0x00);
        // flush the buffered values (mimicking the behavior of latches)
        buffered_flush();
        // flush the CDB (externally managed)
        cdb->flush();
        // advance the clock
        Clock::instance().tick();
        log.as(LogLevel::DEBUG) << "";  // new line to separate cycle
    }

    void RISCV_Simulator::check_reset() {
        // Check for reset signal from ROB
        auto reset_data = bus_rst.read();
        if (reset_data.has_value() && reset_data->reset_signal) {
            auto &log = Logger::get();
            log.as(LogLevel::INFO) << "[CONTROL] Reset detected, flushing pipeline and setting PC to: "
                                   << norb::hex(reset_data->new_pc);

            buffered_flush();   // this will make sure pending actions (like JAL, JALR will still write into the register)
            // Reset all units
            pc.on_reset(reset_data.value());
            reg.on_reset(reset_data.value());
            rob.on_reset(reset_data.value());
            rs.on_reset(reset_data.value());
            lsb.on_reset(reset_data.value());
            ba.on_reset(reset_data.value());

            // Clear the common data bus
            cdb->clear();

            // Clear bus states
            bus_rst.clear();
            bus_rob_has_committed_exit.clear();

            // Clear channel states by manually flushing
            chan_con_rob_next_instruction.clear();

            buffered_flush();
            log.as(LogLevel::INFO) << "[CONTROL] Reset completed, system ready";
        }
    }

    bool RISCV_Simulator::check_for_exit() const { return bus_rob_has_committed_exit.read(); }

    void RISCV_Simulator::boot(const std::string &mem_path) {
        Clock::instance().reset();
        auto &log = Logger::get();
        log.as(LogLevel::INFO) << "Booting RISC-V system with memory mirror: " << mem_path;
        lsb.load_memory(mem_path);
    }

    void RISCV_Simulator::run() {
        tidy();
        int loop_counter = 0;
        for (loop_counter = 0; loop_counter < C::loop_timeout; ++loop_counter) {
            // debugging info
            log.as(LogLevel::INFO) << "Current loop cout: " << loop_counter;
            print_cdb_info();

            // perform a rounding of the main control loop (rising edge)
            execute();
            instruction_fetch();
            write_and_broadcast();
            commit();
            issue();

            // tidy() must be called last (falling edge)
            tidy();

            // check for reset signal on falling edge
            check_reset();

            // check for the termination channel
            if (check_for_exit()) {
                break;
            }
        }
        if (loop_counter >= C::loop_timeout) {
            log.as(LogLevel::FATAL) << "Loop count exceeded upperbound: " << C::loop_timeout;
        }
        print_result();
    }

}  // namespace norb::riscv
