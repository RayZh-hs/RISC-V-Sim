// control.hpp
// - defines entrypoint-level control functions for the entire risc-v system

#pragma once

#include "third_party/logger.hpp"

#include "alu.hpp"
#include "brancher.hpp"
#include "decoder.hpp"
#include "mem.hpp"
#include "reg.hpp"
#include "rob.hpp"
#include "rs.hpp"

namespace norb::riscv {
    struct RISCV_Simulator {
        std::unique_ptr<Memory> memory;

        void boot(const std::string &mem_path) {
            auto &log = Logger::get();
            log.as(LogLevel::INFO) << "Booting RISC-V system with memory mirror: " << mem_path;
            memory = std::make_unique<Memory>(mem_path);
        }

        void run() {

        }
    };
}  // namespace riscv
