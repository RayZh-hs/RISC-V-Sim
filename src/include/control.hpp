// control.hpp
// - defines entrypoint-level control functions for the entire risc-v system

#pragma once

#include <iostream>

#include "alu.hpp"
#include "brancher.hpp"
#include "decoder.hpp"
#include "mem.hpp"
#include "pc.hpp"
#include "reg.hpp"
#include "rob.hpp"
#include "rs.hpp"
#include "third_party/logger.hpp"
#include "utility/bus.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {
    class RISCV_Simulator {
        LoadStoreBuffer memory;
        RegisterFile register_file;
        ReOrderBuffer reorder_buffer;
        ProgramCounter pc;

        Bus<Instruction> bus_con_next_instruction;
        Bus<bool> bus_rob_is_full;
        Bus<bool> bus_rob_has_committed_exit;

        // Connects the buses between all components (hardware linking)
        void connect_buses() {
            bus_rob_is_full.connect(reorder_buffer.bus_rob_is_full);
            bus_con_next_instruction.connect(reorder_buffer.bus_con_next_instruction);
            bus_rob_has_committed_exit.connect(reorder_buffer.bus_rob_has_committed_exit);
        }

        void PrintResult() const {
            auto &log = Logger::get();
            log.as(LogLevel::INFO) << "Program finished. Final state of registers:";
            for (int i = 0; i < C::register_file_size; ++i) {
                log.as(LogLevel::INFO) << "x" << i << ": " << register_file.read(i);
            }
            // output the return value of the program (stored in x10)
            std::cout << register_file.read(RegName::A0) << std::endl;
        }

        void instruction_fetch();
        void issue();
        void execute();
        void write_and_broadcast();
        void commit();

        void tidy() {
            // after each cycle, reset the zero register
            register_file.write(RegName::ZERO, 0x00);
            // flush the buffered values (mimicking the behavior of latches)
            buffered_flush();
            // advance the clock
            Clock::instance().tick();
        }

    public:
        void boot(const std::string &mem_path) {
            Clock::instance().reset();
            auto &log = Logger::get();
            log.as(LogLevel::DEBUG) << "Setting up RISC-V connections";
            connect_buses();
            log.as(LogLevel::INFO) << "Booting RISC-V system with memory mirror: " << mem_path;
            memory.load_memory(mem_path);
        }

        void run() {
            tidy();
            while (true) {
                // perform a rounding of the main control loop (rising edge)
                instruction_fetch();
                issue();
                execute();
                write_and_broadcast();
                commit();

                // tidy() must be called last (run on falling edge)
                tidy();
            }
        }
    };

    void RISCV_Simulator::instruction_fetch() {
        if (!bus_rob_is_full.read()) {
            Instruction ins;
            const uint32_t raw_ins = pc.read();
            const auto ins = Instruction::from(raw_ins);
            bus_con_next_instruction.write(ins);
        }
        reorder_buffer.write_instruction();
    }

    void RISCV_Simulator::issue() {
        // Issue instructions from the ReOrder Buffer to the Reservation Station and Load-Store Buffer
        
    }
}  // namespace norb::riscv
