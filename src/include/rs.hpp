// rs.hpp
// - declares all instances of Reservation Stations

#pragma once

#include <alu.hpp>

#include "dep.hpp"
#include "rob.hpp"  // For rob_pointer_t and related types
#include "utility/bus.hpp"
#include "utility/constants.hpp"
#include "utility/delayer.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    class ReservationStation {
    public:
        RandomDependencyResolver<C::reservation_station_size> resolver;
        Delayer<ResolvedInstructionEntry, C::alu_calc_delay, C::reservation_station_size> delayer;
        ArithmeticLogicUnit alu;
        TemporarilyBuffered<uint32_t> ans;
        TemporarilyBuffered<rob_pointer_t> ans_pointer;

        void on_issue();
        void on_execute();
        void on_broadcast();
    };
}  // namespace norb::riscv
