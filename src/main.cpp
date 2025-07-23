// main.cpp
// - this is the main entrypoint for the simulator

#include "third_party/argparse.hpp"
#include "third_party/logger.hpp"
#include "control.hpp"

int main(int argc, char *argv[]) {
    auto &logger = Logger::get();

    argparse::ArgumentParser program("simulator");
    program.add_argument("action").help("The action to take. Available: run").default_value("run");
    program.add_argument("-c").help("The config file to load").required();
    program.add_argument("-d").help("Where to dump debug info").default_value("");
    program.add_argument("-v").help("Verbose output").default_value(false).implicit_value(true);
    program.parse_args(argc, argv);

    const auto config_file = program.get<std::string>("-c");
    const auto debug_dump = program.get("-d");
    const auto debug_verbose = program.get<bool>("-v");
    const auto action = program.get("action");

    if (!debug_dump.empty()) {
        logger.setFileOutput(debug_dump);
    }
    if (debug_verbose) {
        logger.setLevel(LogLevel::DEBUG);
    } else {
        logger.setLevel(LogLevel::INFO);
    }

    if (action == "run") {
        norb::riscv::RISCV_Simulator simulator;
        simulator.boot(config_file);
        simulator.run();
    } else {
        logger.as(LogLevel::ERROR) << "Unknown action: " << program.get<std::string>("action");
    }

    return 0;
}
