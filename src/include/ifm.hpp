// ifm.hpp
// - implements the Instruction Fetch Module (IFM) for the RISC-V simulator

#pragma once

#include "decoder.hpp"
#include "mem.hpp"
#include "pc.hpp"
#include "bp.hpp"
#include "reset.hpp"
#include "third_party/logger.hpp"
#include "utility/chan.hpp"
#include "utility/constants.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    class InstructionFetchModule : public Resettable {
        ProgramCounter pc;
        BranchPredictor predictor;
        std::shared_ptr<Memory> rom;  // Instruction ROM for fast instruction fetching
        ChannelWriter<Instruction> chan_ifm_rob_next_instruction;
        Logger &log = Logger::get();

        [[nodiscard]] uint32_t get_instruction() const;
        [[nodiscard]] uint32_t predict_pc(uint32_t pc, const Instruction &ins) const;

    public:
        InstructionFetchModule() = default;
        ~InstructionFetchModule() override = default;

        void bind_outbound_to(norb::ChannelReader<Instruction> &chr);
        void import_instruction_memory(std::shared_ptr<Memory> instructions);

        void on_fetch();
        void on_reset(const ResetData &reset_data) override;
    };
}  // namespace norb::riscv
