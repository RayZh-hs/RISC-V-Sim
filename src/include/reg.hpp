// reg.hpp
// - implements the register file

#pragma once

#include "define/reg.hpp"

namespace norb::riscv {

    inline std::optional<rob_pointer_t> RegisterFile::read_host(int index) const {
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

    inline uint32_t RegisterFile::read(int index) const {
        return registers[index].value.read();
    }

    inline void RegisterFile::write(int index, uint32_t value) {
        registers[index].value.write(value);
    }

    inline void RegisterFile::write_host(int index, const rob_pointer_t &host) {
        if (index < 0 or index >= C::register_file_size) {
            throw std::out_of_range("RegisterFile::write_host out of range");
        }
        if (index == 0) {
            return;  // Zero register does not have a host
        }
        registers[index].host.write(host);
        registers[index].has_host.write(true);
    }

}  // namespace norb::riscv
