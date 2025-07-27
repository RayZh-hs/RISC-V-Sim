// reg.cpp
// - implements the register file

#include "reg.hpp"
#include <stdexcept>

namespace norb::riscv {

    Register::Register() : value(0) {}

    std::optional<rob_pointer_t> RegisterFile::read_host(int index) const {
        if (index < 0 or index >= C::register_file_size) {
            throw std::out_of_range("RegisterFile::read_host out of range");
        }
        if (index == 0) {
            return std::nullopt;
        }
        if (registers[index].has_host.read()) {
            return registers[index].host.read();
        }
        return std::nullopt;
    }

    uint32_t RegisterFile::read(int index) const {
        return registers[index].value.read();
    }

    void RegisterFile::write(int index, uint32_t value) {
        registers[index].value.write(value);
    }

    void RegisterFile::write_host(int index, const rob_pointer_t &host) {
        if (index < 0 or index >= C::register_file_size) {
            throw std::out_of_range("RegisterFile::write_host out of range");
        }
        if (index == 0) {
            return;  // Zero register does not have a host
        }
        registers[index].host.write(host);
        registers[index].has_host.write(true);
    }

    void RegisterFile::on_reset(const ResetData& reset_data) {
        if (reset_data.reset_signal) {
            // Clear all host dependencies and reset all registers to 0
            for (int i = 0; i < C::register_file_size; ++i) {
                registers[i].value.write(0);
                registers[i].has_host.write(false);
                if (i != 0) {  // Don't reset the host for x0 register
                    registers[i].host.write(rob_pointer_t{});
                }
            }
        }
    }

}  // namespace norb::riscv
