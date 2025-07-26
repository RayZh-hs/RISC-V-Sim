// cdr.hpp
// - implements the Common Dependency Resolver for rs, lsb and ba

#pragma once

#include <memory>
#include <optional>

#include "cdb.hpp"
#include "define/cdr.hpp"
#include "third_party/array.hpp"
#include "third_party/logger.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {

    inline ResolvedInstructionEntry::ResolvedInstructionEntry(const ResolverEntry &ent) :
        type(ent.type), rob_pointer(ent.rob_pointer), vk(ent.vk), vj(ent.vj), imm(ent.imm) {
        const auto now = Clock::instance().now();
        starting_time = now;
    }

    inline std::string ResolverEntry::repr() const {
        return "ResolverEntry(type=" + ins_type_names[static_cast<int>(type)] +
               ", status=" + std::to_string(static_cast<int>(status)) +
               ", rob_pointer=" + std::to_string(rob_pointer.repr()) +
               ", vk=" + std::to_string(vk) +
               ", vj=" + std::to_string(vj) +
               ", imm=" + std::to_string(imm) + ")";
    }

    // A Common Dependency Resolver (CDR) is responsible for managing dependencies
    // It listens for broadcasts and waits for the dependencies to be resolved
    template <int Capacity>
    class CommonDependencyResolver {
        BufferedArray<ResolverEntry, Capacity> buffer;
        std::unique_ptr<CommonDataBus> cdb_ref;
        ChannelReader<ResolverEntry> chan_inbound;

    public:
        void bind_inbound_to(norb::ChannelWriter<ResolverEntry> &chw) {
            make_channel(chw, chan_inbound);
        }

        [[nodiscard]] bool full() const { return buffer.full(); }

        void listen_inbound() {
            auto &log = Logger::get();
            if (chan_inbound.has_data()) {
                log.as(LogLevel::DEBUG) << "[CDR] Acquired new data";
                const auto idx_new_entry = buffer.find_if([](const ResolverEntry &entry) {
                    return entry.status == ResolverEntryStatus::EMPTY;
                });
                if (idx_new_entry == -1)    {
                    log.as(LogLevel::DEBUG) << "[CDR] The buffer is already full";
                    return;  // this means that the array is full
                }
                const auto new_entry = chan_inbound.read();
                // now insert into the place of the new entry
                log.as(LogLevel::INFO) << "[CDR] Writing new instruction=" << 
                buffer.write_at(idx_new_entry, new_entry);
            }
        }

        void listen_broadcast() {
            auto &log = Logger::get();
            auto news = cdb_ref->read();
            log.as(LogLevel::DEBUG) << "[CDR] Read instruction: [rob=" << news.rob_pointer.repr()
                                    << ", val=" << news.value() << "]";
            for (int i = 0; i < Capacity; ++i) {
                auto ent = buffer.read_at(i);
                if (ent.status == ResolverEntryStatus::PENDING) {
                    if (ent.qk == news.rob_pointer and not ent.k_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[CDR] Resolving entry at index " << i << " with qk=" << ent.qk.repr()
                            << " | target entry rob=" << ent.rob_pointer.repr();
                        ent.k_is_ready = true;
                        ent.vk = news.value;
                    }
                    if (ent.qj == news.rob_pointer and not ent.j_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[CDR] Resolving entry at index " << i << " with qj=" << ent.qj.repr()
                            << " | target entry rob=" << ent.rob_pointer.repr();
                        ent.j_is_ready = true;
                        ent.vj = news.value;
                    }
                    if (ent.k_is_ready and ent.j_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[CDR] Entry at index " << i << " is ready with vk=" << ent.vk << " and vj=" << ent.vj;
                        ent.status = ResolverEntryStatus::READY;
                    }
                }
                // replace the original entry with the new version
                buffer.write_at(i, ent);
            }
        }

        // returns -1 if none is found
        std::optional<ResolvedInstructionEntry> get_ready_entry() {
            auto &log = Logger::get();
            const auto ind =
                buffer.find_if([](const ResolverEntry &entry) { return entry.status == ResolverEntryStatus::READY; });
            if (ind == -1) {
                log.as(LogLevel::DEBUG) << "[CDR] No ready entry found";
                return std::nullopt;
            } else {
                // set state to EXECUTING
                auto ent = buffer.read_at(ind);
                ent.status = ResolverEntryStatus::EXECUTING;
                log.as(LogLevel::DEBUG) << "[CDR] Found ready entry at index " << ind
                                        << " | rob_pointer=" << ent.rob_pointer.repr();
                return ResolvedInstructionEntry(ent);
            }
        }

        void submit_executed_entry(rob_pointer_t rob_pointer, uint32_t value) {
            auto &log = Logger::get();
            // search the database for the rob_pointer
            for (int i = 0; i < Capacity; ++i) {
                auto ent = buffer.read_at(i);
                if (ent.status == ResolverEntryStatus::EXECUTING and ent.rob_pointer == rob_pointer) {
                    log.as(LogLevel::DEBUG) << "[CDR] Submitting executed entry at index " << i
                                            << " with rob_pointer=" << rob_pointer.repr() << " and value=" << value;
                    // update the array
                    ent.status = ResolverEntryStatus::EMPTY;
                    buffer.write_at(i, ent);
                    // submit to the CDB
                    cdb_ref->broadcast({rob_pointer, value});
                }
            }
        }
    };
}  // namespace norb::riscv
