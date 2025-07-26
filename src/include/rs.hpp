// rs.hpp
// - implements all instances of Reservation Stations

#pragma once

#include <alu.hpp>

#include "cdr.hpp"
#include "define/rob.hpp"
#include "utility/bus.hpp"
#include "utility/constants.hpp"
#include "utility/delayer.hpp"

namespace C = norb::riscv::constants;

namespace norb::riscv {
    class ReservationStation {
    public:
        CommonDependencyResolver<C::reservation_station_size> resolver;
        Delayer<ResolvedInstructionEntry, C::alu_calc_delay, C::reservation_station_size> delayer;
        ArithmeticLogicUnit alu;
        TemporarilyBuffered<uint32_t> ans;
        TemporarilyBuffered<rob_pointer_t> ans_pointer;

        void on_issue() { resolver.listen_inbound(); }

        void on_execute() {
            // 1. push to submit to delayer
            const auto resolved_command = resolver.get_ready_entry();
            if (resolved_command.has_value()) {
                // submit to the delayer
                delayer.push(resolved_command.value());
            }
            // 2. calc ans and broadcast
            const auto delayed_command = delayer.pop();
            if (delayed_command.has_value()) {
                ans.write(alu.calculate(delayed_command.value()));
                ans_pointer.write(delayed_command->rob_pointer);
            }
        }

        void on_broadcast() {
            auto &log = Logger::get();
            // 1. broadcast
            if (ans.read().has_value()) {
                assert(ans_pointer.read().has_value());
                log.as(LogLevel::INFO) << "Broadcasting ALU: Broadcast(pointer=" << ans_pointer.read()->repr() << ", ans=" << ans.read().value();
                resolver.submit_executed_entry(ans_pointer.read().value(), ans.read().value());
            } else {
                log.as(LogLevel::DEBUG) << "ALU: Nothing to broadcast";
            }
            // 2. Listen for broadcast
            resolver.listen_broadcast();
        }
    };
}  // namespace norb::riscv
