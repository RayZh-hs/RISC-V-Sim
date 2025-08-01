// ba.hpp
// - implements the Branch Analyzer (BA)

#pragma once

#include "bp.hpp"
#include "decoder.hpp"
#include "dep.hpp"
#include "reset.hpp"
#include "third_party/logger.hpp"

namespace norb::riscv {

    class BranchAnalyzer : public Resettable {
        Logger &logger = Logger::get();
        TemporarilyBuffered<ResolvedInstructionEntry> ready_ins;

    public:
        SequentialDependencyResolver<C::branch_analyzer_size> resolver;
        ChannelWriter<BranchPredictionFeedback> chan_ba_ifm_branch_info;

        [[nodiscard]] uint32_t predict_pc(uint32_t pc, const Instruction &ins) const;
        void on_issue();
        void on_broadcast();
        
        // Implement Resettable interface
        void on_reset(const ResetData& reset_data) override;
    };
}  // namespace norb::riscv
