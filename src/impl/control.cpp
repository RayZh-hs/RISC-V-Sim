// control.cpp
// - implements entrypoint-level control functions for the entire risc-v system

#include "control.hpp"
#include "third_party/logger.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {

    void RISCV_Simulator::connect_buses() { 
        bus_rob_has_committed_exit.connect(rob.bus_rob_has_committed_exit);
        lsb.set_commit_bus(bus_con_commit);
    }

    void RISCV_Simulator::connect_channels() {
        make_channel(chan_con_rob_next_instruction, rob.chan_con_rob_next_instruction);
        rs.resolver.bind_inbound_to(rob.chan_rob_rs_next_instruction);
        lsb.resolver.bind_inbound_to(rob.chan_rob_lsb_next_instruction);
        ba.resolver.bind_inbound_to(rob.chan_rob_ba_next_instruction);
    }

    void RISCV_Simulator::print_result() const {
        auto &log = Logger::get();
        log.as(LogLevel::INFO) << "Program finished. Final state of registers:";
        for (int i = 0; i < C::register_file_size; ++i) {
            log.as(LogLevel::INFO) << "x" << i << ": " << reg.read(i);
        }
        // output the return value of the program (stored in x10)
        std::cout << reg.read(RegName::A0) << std::endl;
    }

    void RISCV_Simulator::instruction_fetch() {
        if (!chan_con_rob_next_instruction.has_data()) {
            // we can write into it
            const uint32_t raw_ins = lsb.get_instruction(pc.read());
            auto ins = Instruction::from(raw_ins);
            // ask the branch analyzer to predict the pc
            // it should return pc + 4 if ins is not a branch instruction
            const auto predicted_pc = ba.predict_pc(pc.read(), ins);
            pc.write(predicted_pc);
            ins.had_jumped = (predicted_pc != pc.read() + 4);   // if imm == 4 this cannot be wrongly predicted
            ins.pc = pc.read();
            // now the instruction forwarded to the BA by ROB will have been tagged to ensure correct rollback
            chan_con_rob_next_instruction.write(ins);
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
        rs.on_broadcast();
        lsb.on_broadcast();
        ba.on_broadcast();
    }

    void RISCV_Simulator::commit() {
        lsb.on_commit();
    }

    void RISCV_Simulator::tidy() {
        // after each cycle, reset the zero register
        reg.write(RegName::ZERO, 0x00);
        // flush the buffered values (mimicking the behavior of latches)
        buffered_flush();
        // advance the clock
        Clock::instance().tick();
    }

    bool RISCV_Simulator::check_for_exit() const { 
        return bus_rob_has_committed_exit.read(); 
    }

    void RISCV_Simulator::boot(const std::string &mem_path) {
        Clock::instance().reset();
        auto &log = Logger::get();
        log.as(LogLevel::INFO) << "Booting RISC-V system with memory mirror: " << mem_path;
        lsb.load_memory(mem_path);
    }

    void RISCV_Simulator::run() {
        tidy();
        while (true) {
            // perform a rounding of the main control loop (rising edge)
            instruction_fetch();
            issue();
            execute();
            write_and_broadcast();
            commit();

            // tidy() must be called last (falling edge)
            tidy();
            // check for the termination channel
            if (check_for_exit()) {
                break;
            }
        }
        print_result();
    }

}  // namespace norb::riscv
