// alu.hpp
// - declares the Arithmetic Logic Unit

#pragma once

#include "utility/constants.hpp"
#include "rob_types.hpp"  // For ResolvedInstructionEntry

namespace norb::riscv {
    class ArithmeticLogicUnit {
    public:
        // is not made static because it is the capability of a single ALU instance, not the class
        // although doing so violates software best practices
        uint32_t calculate(const ResolvedInstructionEntry &e);
    };
}  // namespace norb::riscv
