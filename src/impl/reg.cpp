// reg.cpp
// - implements the register file

#include "reg.hpp"

#include <stdexcept>
#include <third_party/logger.hpp>

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

    uint32_t RegisterFile::read(int index) const { return registers[index].value.read(); }

    void RegisterFile::write(int index, uint32_t value) {
        if (index != 0)
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

    void RegisterFile::clear_host(int index) {
        if (index < 0 or index >= C::register_file_size) {
            throw std::out_of_range("RegisterFile::write_host out of range");
        }
        if (index == 0) {
            return;  // Zero register does not have a host
        }
        registers[index].host.write(rob_nullptr);
        registers[index].has_host.write(false);
    }

    void RegisterFile::on_reset(const ResetData &reset_data) {
        if (reset_data.reset_signal) {
            // Clear all host dependencies
            for (int i = 0; i < C::register_file_size; ++i) {
                registers[i].has_host.write(false);
                if (i != 0) {
                    registers[i].host.write(rob_pointer_t{});
                }
            }
        }
    }

    void RegisterFile::print_state() {
        auto &log = Logger::get();
        log.as(LogLevel::DEBUG) << "Register File information:";
        std::string s;
        for (int i = 0; i < C::register_file_size; ++i) {
            s += ("R" + std::to_string(i) + "(" + std::to_string(registers[i].value.read()));
            if (registers[i].has_host.read())
                s += "[rob=" + std::to_string(registers[i].host.read().physical_index()) + "]";
            s += ") ";
        }
        log.as(LogLevel::DEBUG) << s;
    }

}  // namespace norb::riscv
