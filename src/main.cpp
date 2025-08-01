// main.cpp
// - this is the main entrypoint for the simulator

#include "third_party/logger.hpp"
#include "simulator.hpp"
#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

int main(int argc, char *argv[]) {
    auto &logger = Logger::get();

    // Configure logger based on constants
    if (not C::log_file.empty()) {
        logger.setFileOutput(C::log_file);
    }
    logger.setLevel(LogLevel::FATAL);

    norb::riscv::RISCV_Simulator simulator;
    simulator.boot();
    simulator.run();

    return 0;
}
