// rob.hpp
// - declares the ReOrder Buffer

#pragma once

#include <cstdint>
#include "decoder.hpp"
#include "utility/bus.hpp"
#include "utility/chan.hpp"
#include "utility/constants.hpp"
#include "third_party/queue.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    class RegisterFile;  // Forward declaration
    
    // ROB types
    enum class ROBEntryStatus { EMPTY, READY, ISSUED, COMMITTED };

    struct ROBEntry {
        Instruction instruction;
        ROBEntryStatus status;
        uint32_t result;

        ROBEntry();
        ROBEntry(Instruction ins, ROBEntryStatus status, uint32_t result);
    };

    using rob_buffer_t = FixedBufferedQueue<ROBEntry, C::reorder_buffer_size>;
    using rob_pointer_t = rob_buffer_t::iterator;

    enum class ResolverEntryStatus {
        EMPTY,  // instruction has been executed
        PENDING,  // waiting for dependencies to be resolved
        READY,  // all dependencies are resolved
        EXECUTING,  // currently executing the instruction
    };

    struct ResolverEntry {
        InsType type = InsType::NOOP;
        ResolverEntryStatus status = ResolverEntryStatus::PENDING;
        rob_pointer_t rob_pointer;
        bool k_is_ready = false;
        bool j_is_ready = false;
        uint32_t vk{};
        uint32_t vj{};
        rob_pointer_t qk;
        rob_pointer_t qj;
        uint32_t imm{};

        [[nodiscard]] std::string repr() const;
    };

    struct ResolvedInstructionEntry {
        InsType type = InsType::NOOP;
        rob_pointer_t rob_pointer;
        uint32_t vk{};
        uint32_t vj{};
        uint32_t imm{};

        explicit ResolvedInstructionEntry(const ResolverEntry &ent);
        ResolvedInstructionEntry() = default;
    };
    
    class ReOrderBuffer {
    public:
        // Communication buses
        // - control unit
        Bus<bool> bus_rob_has_committed_exit;
        ChannelReader<Instruction> chan_con_rob_next_instruction;

        // - reservation station
        ChannelWriter<ResolverEntry> chan_rob_rs_next_instruction;

    private:
        rob_buffer_t buffer;
        RegisterFile &register_file;

    public:
        // In the cpu, only rob has write access to the register file
        // Therefore, we can assume they are tightly coupled
        explicit ReOrderBuffer(RegisterFile &rf) : register_file(rf) {}

        [[nodiscard]] bool full() const;
        [[nodiscard]] bool almost_full() const;
        [[nodiscard]] bool empty() const;

        void instruction_fetch();
        void issue();
    };
}  // namespace norb::riscv
