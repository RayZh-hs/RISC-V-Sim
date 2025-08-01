// redirected_main.cpp
// - for debugging purposes: redirect input from a file

#include "third_party/logger.hpp"
#include "simulator.hpp"
#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

int main(int argc, char *argv[]) {
    std::freopen("../testcases/hanoi.data", "r", stdin);
    auto &logger = Logger::get();

    // Configure logger based on constants
    if (not C::log_file.empty()) {
        logger.setFileOutput(C::log_file);
    }
    logger.setLevel(C::log_level);

    norb::riscv::RISCV_Simulator simulator;
    simulator.boot();
    simulator.run();

    return 0;
}
