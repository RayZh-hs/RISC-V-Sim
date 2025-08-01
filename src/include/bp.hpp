// bp.hpp
// - implements the Branch Predictor (BP) for the RISC-V simulator

#pragma once

namespace norb::riscv {
    class BranchPredictor {
    public:
        enum class State { S_JMP, W_JMP, W_NJMP, S_NJMP };
        Buffered<State> state{State::W_NJMP};

        [[nodiscard]] bool predict_should_jump() const {
            if (state.read() == State::S_JMP || state.read() == State::W_JMP) {
                return true;
            }
            return false;
        }

        void move_state(const bool &do_jump) {
            if (do_jump) {
                if (state.read() != State::S_JMP) {
                    state = static_cast<State>(static_cast<int>(state.read()) - 1);  // Move towards S_JMP
                }
            } else {
                if (state.read() != State::S_NJMP) {
                    state = static_cast<State>(static_cast<int>(state.read()) + 1);  // Move towards S_NJMP
                }
            }
        }
    };
}  // namespace norb::riscv
