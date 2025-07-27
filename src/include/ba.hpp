// ba.hpp
// - implements the Branch Analyzer (BA)

#pragma once

#include "decoder.hpp"
#include "dep.hpp"
#include "third_party/logger.hpp"

namespace norb::riscv {

    class BranchAnalyzer {
        Logger &logger = Logger::get();

        [[nodiscard]] bool should_jump(const ResolvedInstructionEntry &entry) const;
        [[nodiscard]] uint32_t calc_pc(uint32_t pc, const ResolvedInstructionEntry &entry) const;

    public:
        SequentialDependencyResolver<C::branch_analyzer_size> resolver;

        class BranchPredictor {
            enum class State { S_JMP, W_JMP, W_NJMP, S_NJMP };
            State state = State::W_NJMP;

        public:
            [[nodiscard]] bool predict_should_jump() const;
        };

        BranchPredictor predictor;

        [[nodiscard]] uint32_t predict_pc(uint32_t pc, const Instruction &ins) const;
        void on_issue();
        void on_broadcast();
    };
}  // namespace norb::riscv
