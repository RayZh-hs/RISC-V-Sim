// rob.hpp
// - declares the ReOrder Buffer

#pragma once

#include <third_party/logger.hpp>

#include "cdb.hpp"
#include "reset.hpp"
#include "rob_types.hpp"
#include "utility/bus.hpp"
#include "utility/chan.hpp"
#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    class RegisterFile;  // Forward declaration

    class ReOrderBuffer : public Resettable {
    public:
        // Communication buses
        // - control unit
        Bus<bool> bus_rob_has_committed_exit;
        ChannelReader<Instruction> chan_con_rob_next_instruction;
        std::shared_ptr<CommonDataBus> cdb_ref = nullptr;

        // - reservation station
        ChannelWriter<ResolverEntry> chan_rob_rs_next_instruction;
        ChannelWriter<ResolverEntry> chan_rob_lsb_next_instruction;
        ChannelWriter<ResolverEntry> chan_rob_ba_next_instruction;
        
        // - load store buffer
        TemporaryBus<rob_pointer_t> bus_con_commit;
        // - reset bus
        TemporaryBus<ResetData> bus_rst;

    private:
        rob_main_buffer_t main_buffer;
        rob_resolver_buffer_t resolver_buffer;
        RegisterFile &register_file;
        Logger &log = Logger::get();

    public:
        // In the cpu, only rob has write access to the register file
        // Therefore, we can assume they are tightly coupled
        explicit ReOrderBuffer(RegisterFile &rf, CommonDataBus &cdb) : register_file(rf) {
            cdb_ref = std::make_shared<CommonDataBus>(cdb);
        }

        [[nodiscard]] bool full() const;
        [[nodiscard]] bool almost_full() const;
        [[nodiscard]] bool empty() const;

        void instruction_fetch();
        void on_issue();
        void on_broadcast();
        void on_commit();
        
        // Implement Resettable interface
        void on_reset(const ResetData& reset_data) override;
    };
}  // namespace norb::riscv
