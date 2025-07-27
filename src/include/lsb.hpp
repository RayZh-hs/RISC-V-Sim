// lsb.hpp
// - implements the Load Store Buffer (LSB)

#pragma once

#include <unordered_map>
#include <utility/delayer.hpp>

#include "mem.hpp"
#include "reset.hpp"

namespace norb::riscv {
    // Load Store Buffer
    class LoadStoreBuffer : public Resettable {
    private:
        Delayer<ResolvedInstructionEntry, C::mem_access_delay, C::load_store_buffer_size> delayer;
        std::unique_ptr<Memory> memory = nullptr;
        TemporaryBus<rob_pointer_t> bus_con_commit;
        Logger &log = Logger::get();

        // These objects are to keep track of modifications before commit
        // They can be translated to BufferedArray objects but are kept so for simplicity
        std::unordered_map<uint32_t, uint8_t> modification_list{};
        std::unordered_multimap<rob_pointer_t, uint32_t> modification_blame{};

        // Auxiliary function to read a single byte, checking modification_list first
        uint8_t read_byte(uint32_t addr);

    public:
        RandomDependencyResolver<C::load_store_buffer_size> resolver;

        void load_memory(const std::string& mem_path);
        void set_commit_bus(TemporaryBus<rob_pointer_t> &bus);

        // This theoretically should depend on the underlying instruction buffer for faster fetch
        [[nodiscard]] uint32_t get_instruction(uint32_t index) const;

        void on_issue();

        void on_execute();

        void on_broadcast();

        void on_commit();
        
        // Implement Resettable interface
        void on_reset(const ResetData& reset_data) override;
    };
}  // namespace norb::riscv
