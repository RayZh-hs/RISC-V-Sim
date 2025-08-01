// ifm.cpp
// - implements the Instruction Fetch Module (IFM) for the RISC-V simulator

#include "ifm.hpp"

#include <utility>

#include "utility/dump.hpp"

namespace norb::riscv {

    void InstructionFetchModule::bind_outbound_to(norb::ChannelReader<Instruction>& chr) {
        make_channel(chan_ifm_rob_next_instruction, chr);
    }

    void InstructionFetchModule::bind_branch_info_inbound_to(norb::ChannelWriter<BranchPredictionFeedback>& chw) {
        norb::make_channel(chw, chan_ba_ifm_branch_info);
    }

    void InstructionFetchModule::import_instruction_memory(std::shared_ptr<Memory> instructions) {
        rom = std::move(instructions);
    }

    void InstructionFetchModule::on_fetch() {
        if (not chan_ifm_rob_next_instruction.has_data()) {
            // we can write into it
            const uint32_t raw_ins = get_instruction();
            Instruction ins = noop;
            try {
                ins = Instruction::from(raw_ins);
                log.as(LogLevel::DEBUG) << "[IFM] Fetched instruction: " << ins.repr();
            } catch (...) {
                log.as(LogLevel::WARN) << "[IFM] Malformed instruction: Cannot decode raw_ins=" << raw_ins;
            }
            // ask the branch analyzer to predict the pc
            // it should return pc + 4 if ins is not a branch instruction
            const auto predicted_pc = predict_pc(pc.read(), ins);
            pc.write(predicted_pc);
            ins.had_jumped = (predicted_pc != pc.read() + 4);  // if imm == 4 this cannot be wrongly predicted
            ins.pc = pc.read();
            log.as(LogLevel::DEBUG) << "Current pc: " << norb::hex(pc.read())
                                    << ", Predicted pc: " << norb::hex(predicted_pc);
            // now the instruction forwarded to the BA by ROB will have been tagged to ensure correct rollback
            if (ins.header.ins_type != NOOP)
                chan_ifm_rob_next_instruction.write(ins);
            else
                log.as(LogLevel::DEBUG) << "Skipping NOOP";
        }
    }

    void InstructionFetchModule::on_broadcast() {
        if (chan_ba_ifm_branch_info.has_data()) {
            const auto branch_feedback = chan_ba_ifm_branch_info.read();
            log.as(LogLevel::DEBUG) << "[IFM] Submitting branch feedback: " << branch_feedback.repr();
            predictor.update(branch_feedback);
        }
    }

    void InstructionFetchModule::on_reset(const ResetData& reset_data) {
        pc.on_reset(reset_data);
        chan_ifm_rob_next_instruction.clear();
    }

    uint32_t InstructionFetchModule::get_instruction() const { return rom->read_word(pc.read()); }

    uint32_t InstructionFetchModule::predict_pc(uint32_t pc, const Instruction& ins) const {
        if (ins.header.ins_pos != InsPos::BRANCH
            // we will not be able to predict the pc for JALR
            or ins.header.ins_type == InsType::JALR) {
            return pc + 4;  // Default to next instruction
        } else {
            if (ins.header.ins_type == JAL) return pc + ins.imm;
            if (predictor.predict_should_jump()) {
                return pc + ins.imm;  // Predict taken branch
            } else {
                return pc + 4;  // Predict not taken branch
            }
        }
    }

}  // namespace norb::riscv
