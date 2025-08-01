// ba.cpp
// - implements the Branch Analyzer (BA) for RISC-V branching instructions

#include "ba.hpp"

#include <pc.hpp>
#include <stdexcept>

#include "third_party/logger.hpp"

namespace norb::riscv {

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
                const bool real_jump = utils::should_jump(to_broadcast.value());
                const bool had_jumped = to_broadcast->had_jumped;
                const auto branch_info = BranchPredictionFeedback(to_broadcast->pc, real_jump, had_jumped);
                if (not chan_ba_ifm_branch_info.has_data()) {
                    // send to predictor
                    chan_ba_ifm_branch_info.write(branch_info);
                }
                logger.as(LogLevel::INFO) << "[BA] Resolved jump: Jump(pc=" << to_broadcast->pc
                                          << ", type=" << ins_type_names[static_cast<int>(to_broadcast->type)]
                                          << ", real_jump=" << real_jump << ", had_jumped=" << had_jumped << ")";

                const uint32_t ret =
                    (real_jump == had_jumped) ? C::correct_branch_token : utils::calc_pc(to_broadcast->pc, to_broadcast.value());
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
            
            logger.as(LogLevel::INFO) << "[BA] Reset completed, all buffers cleared";
        }
    }

}  // namespace norb::riscv
