// lsb.hpp
// - implements the Load Store Buffer (LSB)

#pragma once

#include <utility/delayer.hpp>

#include "mem.hpp"

namespace norb::riscv {
    // Load Store Buffer
    class LoadStoreBuffer {
    private:
        RandomDependencyResolver<C::load_store_buffer_size> resolver;
        Delayer<ResolvedInstructionEntry, C::mem_access_delay, C::load_store_buffer_size> delayer;
        std::unique_ptr<Memory> memory = nullptr;

    public:
        void load_memory(const std::string& mem_path);

        // This theoretically should depend on the underlying instruction buffer for faster fetch
        [[nodiscard]] uint32_t get_instruction(uint32_t index) const;

        void on_fetch();

        void on_broadcast();

        void on_execute();

        void on_commit();
    };
}  // namespace norb::riscv
