// simulator.hpp
// - declares entrypoint-level control functions for the entire risc-v system

#pragma once

#include <iostream>

#include "ba.hpp"
#include "cdb.hpp"
#include "decoder.hpp"
#include "lsb.hpp"
#include "pc.hpp"
#include "ifm.hpp"
#include "reg.hpp"
#include "reset.hpp"
#include "rob.hpp"
#include "rs.hpp"
#include "third_party/logger.hpp"
#include "utility/bus.hpp"
#include "utility/chan.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {
    class RISCV_Simulator {
        std::shared_ptr<CommonDataBus> cdb;
        LoadStoreBuffer lsb;
        RegisterFile reg;
        ReOrderBuffer rob;
        InstructionFetchModule ifm;
        ReservationStation rs;
        BranchAnalyzer ba; // BA's branch prediction is coupled with PC, which is handled by IFM

        Bus<bool> bus_rob_has_committed_exit;
        TemporaryBus<ResetData> bus_rst;
        ChannelWriter<Instruction> chan_ifm_rob_next_instruction;

        Logger &log = Logger::get();

        // Connects the buses between all components (hardware linking)
        void connect_buses();
        void connect_channels();
        void print_result() const;
        void print_cdb_info() const;

        void instruction_fetch();
        void issue();
        void execute();
        void write_and_broadcast();
        void commit();

        void tick();
        void check_reset();  // Check for reset signal and handle reset
        [[nodiscard]] bool check_for_exit() const;

    public:
        RISCV_Simulator() : cdb(std::make_shared<CommonDataBus>()), rob(reg, cdb), ifm() {
            auto &log = Logger::get();
            log.as(LogLevel::DEBUG) << "Setting up hardware RISC-V connections";
            connect_buses();
            connect_channels();
        }

        void boot();
        void run();
    };
}  // namespace norb::riscv
