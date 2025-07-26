// define/cdr.hpp
// - defines types and structures for the Common Dependency Resolver

#pragma once

#include <cstdint>
#include <string>

#include "decoder.hpp"
#include "define/cdb.hpp"

namespace norb::riscv {

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
    };

}  // namespace norb::riscv
