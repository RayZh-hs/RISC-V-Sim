// lsb.cpp
// - implements the LSB

#include "lsb.hpp"

namespace norb::riscv {

    void LoadStoreBuffer::load_memory(const std::string& mem_path) { memory = std::make_unique<Memory>(mem_path); }

    uint32_t LoadStoreBuffer::get_instruction(uint32_t index) const {
        if (!memory) {
            throw std::runtime_error("Memory not loaded.");
        }
        return memory->read(index);
    }

    void LoadStoreBuffer::on_fetch() { resolver.listen_inbound(); }

    void LoadStoreBuffer::on_broadcast() {
        // Listen for well-formed instructions
        resolver.listen_broadcast();
        // Resolve ready entries
        auto entry = resolver.get_ready_entry();
        if (entry.has_value()) {
            delayer.push(entry.value());
        }
    }

    void LoadStoreBuffer::on_execute() {

    }

}  // namespace norb::riscv
