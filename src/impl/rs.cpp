// rs.cpp
// - implements the Reservation Station

#include "rs.hpp"
#include "third_party/logger.hpp"
#include <cassert>

namespace norb::riscv {

    void ReservationStation::on_issue() { 
        resolver.listen_inbound(); 
    }

    void ReservationStation::on_execute() {
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

    void ReservationStation::on_broadcast() {
        auto &log = Logger::get();
        // 1. broadcast
        if (ans.read().has_value()) {
            assert(ans_pointer.read().has_value());
            log.as(LogLevel::INFO) << "[RS] Broadcasting: Broadcast(pointer=" << ans_pointer.read()->repr() << ", ans=" << ans.read().value() << ")";
            resolver.submit_executed_entry(ans_pointer.read().value(), ans.read().value());
        } else {
            log.as(LogLevel::DEBUG) << "[RS] Nothing to broadcast";
        }
        // 2. Listen for broadcast
        resolver.listen_broadcast();
    }

    void ReservationStation::on_reset(const ResetData& reset_data) {
        if (reset_data.reset_signal) {
            auto &log = Logger::get();
            log.as(LogLevel::INFO) << "[RS] Reset signal received, clearing all buffers";
            
            // Clear resolver buffer
            resolver.clear();
            
            // Clear delayer
            delayer.clear();
            
            // Clear answer buffers
            ans.flush();  // This will clear the temporarily buffered value
            ans_pointer.flush();
            
            log.as(LogLevel::INFO) << "[RS] Reset completed, all buffers cleared";
        }
    }

}  // namespace norb::riscv
