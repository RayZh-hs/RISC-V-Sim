// rob_types.hpp
// - declares the common types used by ROB and other components

#pragma once

#include <cstdint>
#include "decoder.hpp"
#include "third_party/queue.hpp"
#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {

    // ROB types
    enum class ROBEntryStatus { EMPTY, READY, ISSUED, COMMITTED };

    struct ROBEntry {
        Instruction instruction;
        ROBEntryStatus status;
        uint32_t result;

        ROBEntry();
        ROBEntry(Instruction ins, ROBEntryStatus status, uint32_t result);
    };

    using rob_main_buffer_t = FixedBufferedQueue<ROBEntry, C::reorder_buffer_size>;
    using rob_pointer_t = rob_main_buffer_t::iterator;
    inline rob_pointer_t rob_nullptr = rob_main_buffer_t::iterator();

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
        bool had_jumped = false;
        uint32_t pc = 0;  // Program Counter at the time of instruction fetch

        [[nodiscard]] std::string repr() const;
    };

    struct ResolvedInstructionEntry {
        InsType type = InsType::NOOP;
        rob_pointer_t rob_pointer;
        uint32_t vk{};
        uint32_t vj{};
        uint32_t imm{};
        bool had_jumped = false;
        uint32_t pc = 0;  // Program Counter at the time of instruction fetch

        explicit ResolvedInstructionEntry(const ResolverEntry &ent);
        ResolvedInstructionEntry() = default;
    };

    using rob_resolver_buffer_t = FixedBufferedQueue<ResolverEntry, C::reorder_buffer_size>;

}  // namespace norb::riscv

// Hash specialization for rob_pointer_t to make it usable in unordered containers
// (software specifications)
namespace std {
    template <>
    struct hash<norb::riscv::rob_pointer_t> {
        size_t operator()(const norb::riscv::rob_pointer_t &it) const noexcept {
            // Use the physical index as the hash value
            return std::hash<size_t>{}(it.physical_index());
        }
    };
}  // namespace std
