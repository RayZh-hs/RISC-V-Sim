// rs.hpp
// - implements all instances of Reservation Stations

#pragma once

#include "utility/bus.hpp"
#include "define/rob.hpp"
#include "cdr.hpp"

namespace norb::riscv {
    class ReservationStation {
        

    public:
        // Communication buses
        Bus<bool> bus_rs_is_full;
        Bus<Instruction> bus_rob_next_instruction;
        Bus<rob_pointer_t> bus_rob_next_instruction_pointer;

        void on_issue() {
            
        }
    };
}
