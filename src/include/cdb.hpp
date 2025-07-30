// cdb.hpp
// - implements the Common Data Bus for the RISC-V simulator

#pragma once

#include <queue>
#include <cstdint>
#include "rob_types.hpp"
#include "third_party/logger.hpp"

namespace norb::riscv {

    struct BroadcastEntry {
        rob_pointer_t rob_pointer;
        uint32_t value{};

        BroadcastEntry(rob_pointer_t rob_pointer, uint32_t value);
        BroadcastEntry() = default;
    };

    // This is the only system in the RISC-V simulator that does not follow strict latch logic
    // It is designed so for simpler interface and to let multiple dataflows through
    class CommonDataBus {
        std::queue<BroadcastEntry> broadcast_queue_{};
        std::queue<BroadcastEntry> changes_queue_{};

    public:
        CommonDataBus() = default;

        void broadcast(const BroadcastEntry& entry);
        [[nodiscard]] bool empty() const;
        [[nodiscard]] BroadcastEntry read() const;
        [[nodiscard]] std::optional<BroadcastEntry> read_as_optional() const;

        // This method is called on falling edge
        void flush();
        void clear();
    };

    namespace impl {
        // returns whether the object has been changed
        inline bool update_with_broadcast(ResolverEntry &entry, const std::optional<BroadcastEntry> &news) {
            Logger &log = Logger::get();
            if (not news.has_value()) return false;
            bool has_been_changed = false;
            if (not entry.j_is_ready and entry.qj == news->rob_pointer) {
                log.as(LogLevel::DEBUG) << "[CDB] Updated resolver entry: " << entry.repr() << " j is ready with value "
                                        << news->value;
                entry.j_is_ready = true;
                entry.vj = news->value;
                has_been_changed = true;
            }
            if (not entry.k_is_ready and entry.qk == news->rob_pointer) {
                log.as(LogLevel::DEBUG) << "[CDB] Updated resolver entry: " << entry.repr() << " k is ready with value "
                                        << news->value;
                entry.k_is_ready = true;
                entry.vk = news->value;
                has_been_changed = true;
            }
            return has_been_changed;
        }
    }  // namespace impl
}  // namespace norb::riscv
