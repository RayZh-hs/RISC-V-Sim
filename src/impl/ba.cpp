// ba.cpp
// - implements the Branch Analyzer (BA) for RISC-V branching instructions

#include "ba.hpp"

#include <stdexcept>

#include "third_party/logger.hpp"

namespace norb::riscv {

    bool BranchAnalyzer::should_jump(const ResolvedInstructionEntry &entry) const {
        switch (entry.type) {
            // Conditional branches
            case InsType::BEQ:
                return entry.vj == entry.vk;
            case InsType::BNE:
                return entry.vj != entry.vk;
            case InsType::BLT:
                return static_cast<int32_t>(entry.vj) < static_cast<int32_t>(entry.vk);
            case InsType::BGE:
                return static_cast<int32_t>(entry.vj) >= static_cast<int32_t>(entry.vk);
            case InsType::BLTU:
                return entry.vj < entry.vk;
            case InsType::BGEU:
                return entry.vj >= entry.vk;

            // Unconditional jumps
            case InsType::JAL:
            case InsType::JALR:
                return true;

            default:
                throw std::runtime_error("Unsupported instruction type for branching: " +
                                         ins_type_names[static_cast<int>(entry.type)]);
        }
    }

    uint32_t BranchAnalyzer::calc_pc(uint32_t pc, const ResolvedInstructionEntry &entry) const {
        if (should_jump(entry)) {
            if (entry.type == InsType::JALR) {
                // PC = rs1 + imm
                return entry.vj + entry.imm;
            }
            return pc + entry.imm;  // Jump to target address
        } else {
            return pc + 4;  // Continue to next instruction
        }
    }

    bool BranchAnalyzer::BranchPredictor::predict_should_jump() const {
        if (state.read() == State::S_JMP || state.read() == State::W_JMP) {
            return true;
        }
        return false;
    }

    void BranchAnalyzer::BranchPredictor::move_state(const bool &do_jump) {
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

    uint32_t BranchAnalyzer::predict_pc(uint32_t pc, const Instruction &ins) const {
        if (ins.header.ins_pos != InsPos::BRANCH
            // we will not be able to predict the pc for JALR
            or ins.header.ins_type == InsType::JALR) {
            return pc + 4;  // Default to next instruction
        } else {
            if (predictor.predict_should_jump()) {
                return pc + ins.imm;  // Predict taken branch
            } else {
                return pc + 4;  // Predict not taken branch
            }
        }
    }

    void BranchAnalyzer::on_issue() { resolver.listen_inbound(); }

    void BranchAnalyzer::on_broadcast() {
        {
            // 1. listen for newly broadcast entry
            resolver.listen_broadcast();
            // 2. broadcast new ready entry
            const auto to_broadcast = resolver.get_ready_entry();
            if (to_broadcast.has_value()) {
                ready_ins.write(to_broadcast.value());
            }
        }
        {
            // 3. retrieve entry to broadcast (next cycle)
            const auto to_broadcast = ready_ins.read();
            if (to_broadcast.has_value()) {
                const bool real_jump = should_jump(to_broadcast.value());
                const bool had_jumped = to_broadcast->had_jumped;
                predictor.move_state(real_jump);  // update the predictor state
                logger.as(LogLevel::INFO) << "[BA] Resolved jump: Jump(pc=" << to_broadcast->pc
                                          << ", type=" << ins_type_names[static_cast<int>(to_broadcast->type)]
                                          << ", real_jump=" << real_jump << ", had_jumped=" << had_jumped << ")";

                const uint32_t ret =
                    (real_jump == had_jumped) ? C::correct_branch_token : calc_pc(to_broadcast->pc, to_broadcast.value());
                // broadcast the ret
                Logger::get().as(LogLevel::INFO) << "[BA] Broadcasting: Broadcast(pointer=" << to_broadcast->rob_pointer.repr() << ", ans=" << ret << ")";
                resolver.submit_executed_entry(to_broadcast->rob_pointer, ret);
            }
        }
    }

    void BranchAnalyzer::on_reset(const ResetData& reset_data) {
        if (reset_data.reset_signal) {
            logger.as(LogLevel::INFO) << "[BA] Reset signal received, clearing all buffers";
            
            // Clear resolver buffer
            resolver.clear();
            
            // Reset branch predictor to default state
            predictor.state.write(BranchPredictor::State::W_NJMP);
            
            logger.as(LogLevel::INFO) << "[BA] Reset completed, all buffers cleared";
        }
    }

}  // namespace norb::riscv
