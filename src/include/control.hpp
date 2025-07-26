// control.hpp
// - defines entrypoint-level control functions for the entire risc-v system

#pragma once

#include <iostream>

#include "alu.hpp"
#include "ba.hpp"
#include "cdb.hpp"
#include "decoder.hpp"
#include "mem.hpp"
#include "pc.hpp"
#include "reg.hpp"
#include "rob.hpp"
#include "rs.hpp"
#include "third_party/logger.hpp"
#include "utility/bus.hpp"
#include "utility/chan.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {
    class RISCV_Simulator {
        LoadStoreBuffer lsb;
        CommonDataBus cbd;
        RegisterFile reg;
        ReOrderBuffer rob;
        ProgramCounter pc;
        ReservationStation rs;

        Bus<bool> bus_rob_has_committed_exit;
        ChannelWriter<Instruction> chan_con_rob_next_instruction;

        // Connects the buses between all components (hardware linking)
        void connect_buses() { bus_rob_has_committed_exit.connect(rob.bus_rob_has_committed_exit); }

        void connect_channels() {
            make_channel(chan_con_rob_next_instruction, rob.chan_con_rob_next_instruction);
            rs.resolver.bind_inbound_to(rob.chan_rob_rs_next_instruction);
        }

        void print_result() const {
            auto &log = Logger::get();
            log.as(LogLevel::INFO) << "Program finished. Final state of registers:";
            for (int i = 0; i < C::register_file_size; ++i) {
                log.as(LogLevel::INFO) << "x" << i << ": " << reg.read(i);
            }
            // output the return value of the program (stored in x10)
            std::cout << reg.read(RegName::A0) << std::endl;
        }

        void instruction_fetch();
        void issue();
        void execute();
        void write_and_broadcast();
        void commit();

        void tidy() {
            // after each cycle, reset the zero register
            reg.write(RegName::ZERO, 0x00);
            // flush the buffered values (mimicking the behavior of latches)
            buffered_flush();
            // advance the clock
            Clock::instance().tick();
        }

        [[nodiscard]] bool check_for_exit() const { return bus_rob_has_committed_exit.read(); }

    public:
        RISCV_Simulator() : reg(), rob(reg) {
            auto &log = Logger::get();
            log.as(LogLevel::DEBUG) << "Setting up hardware RISC-V connections";
            connect_buses();
            connect_channels();
        }

        void boot(const std::string &mem_path) {
            Clock::instance().reset();
            auto &log = Logger::get();
            log.as(LogLevel::INFO) << "Booting RISC-V system with memory mirror: " << mem_path;
            lsb.load_memory(mem_path);
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

                // tidy() must be called last (falling edge)
                tidy();
                // check for the termination channel
                if (check_for_exit()) {
                    break;
                }
            }
            print_result();
        }
    };

    inline void RISCV_Simulator::instruction_fetch() {
        if (!chan_con_rob_next_instruction.has_data()) {
            // we can write into it
            const uint32_t raw_ins = pc.read();
            const auto ins = Instruction::from(raw_ins);
            chan_con_rob_next_instruction.write(ins);
        }
        rob.instruction_fetch();
    }

    inline void RISCV_Simulator::issue() {
        // Issue instructions from the ReOrder Buffer to the Reservation Station and Load-Store Buffer
        rob.issue();
        rs.on_issue();
    }

    inline void RISCV_Simulator::execute() {
        rs.on_execute();
    }

    inline void RISCV_Simulator::write_and_broadcast() {
        rs.on_broadcast();
    }
}  // namespace norb::riscv
