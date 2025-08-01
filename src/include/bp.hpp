// bp.hpp
// - implements the Branch Predictor (BP) for the RISC-V simulator

#pragma once

#include "utility/dump.hpp"
#include "utility/buffered.hpp"

namespace norb::riscv {

    // Encodes information about a branch prediction, fed back to the predictor
    struct BranchPredictionFeedback {
        uint32_t pc;
        bool should_jump;
        bool did_jump;

        BranchPredictionFeedback() : pc(0), should_jump(false), did_jump(false) {}
        BranchPredictionFeedback(uint32_t pc, bool should_jump, bool did_jump)
            : pc(pc), should_jump(should_jump), did_jump(did_jump) {}

        [[nodiscard]] std::string repr() const {
            return "BranchPredictionFeedback(pc=" + std::to_string(pc) + ", should_jump=" + btos(should_jump) +
                ", did_jump=" + btos(did_jump) + ")";
        }
    };

    namespace impl {
        class BranchPredictorBase_ {
        public:
            virtual ~BranchPredictorBase_() = default;

            [[nodiscard]] virtual bool predict_should_jump() const = 0;
            virtual void update(const BranchPredictionFeedback &feedback) = 0;
        };
    }  // namespace impl

    class TwoBitBranchPredictor : public impl::BranchPredictorBase_ {
    public:
        enum class State { S_JMP, W_JMP, W_NJMP, S_NJMP };
        Buffered<State> state{State::W_NJMP};

        ~TwoBitBranchPredictor() override = default;

        [[nodiscard]] bool predict_should_jump() const override {
            if (state.read() == State::S_JMP || state.read() == State::W_JMP) {
                return true;
            }
            return false;
        }

        void update(const BranchPredictionFeedback &feedback) override {
            if (feedback.should_jump) {
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
