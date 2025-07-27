// loaned_types.cpp
// - implements ROB and types loaned from CDR (templating issues)

#include "rob_types.hpp"
#include "rob.hpp"
#include "decoder.hpp"

namespace norb::riscv {

    ROBEntry::ROBEntry() : instruction(noop), status(ROBEntryStatus::EMPTY), result(0) {}

    ResolvedInstructionEntry::ResolvedInstructionEntry(const ResolverEntry &ent) :
        type(ent.type), rob_pointer(ent.rob_pointer), vk(ent.vk), vj(ent.vj), imm(ent.imm),
        pc(ent.pc), had_jumped(ent.had_jumped) {}

    std::string ResolverEntry::repr() const {
        return "ResolverEntry(type=" + ins_type_names[static_cast<int>(type)] +
               ", status=" + std::to_string(static_cast<int>(status)) +
               ", rob_pointer=" + std::to_string(rob_pointer.repr()) +
               ", vk=" + std::to_string(vk) +
               ", vj=" + std::to_string(vj) +
               ", imm=" + std::to_string(imm) + ")";
    }

}  // namespace norb::riscv
