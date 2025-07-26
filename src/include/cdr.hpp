// cdr.hpp
// - implements the Common Dependency Resolver for rs, lsb and ba

#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "cdb.hpp"
#include "decoder.hpp"
#include "define/cdb.hpp"
#include "third_party/array.hpp"
#include "third_party/logger.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {

    enum class ResolverEntryStatus {
        EMPTY,  // instruction has been executed
        PENDING,  // waiting for dependencies to be resolved
        READY,  // all dependencies are resolved
        EXECUTING,  // currently executing the instruction
    };

    struct ResolverEntry {
        InsType type = InsType::NOOP;
        ResolverEntryStatus status = ResolverEntryStatus::PENDING;
        rob_pointer_t rob_pointer;
        bool k_is_ready = false;
        bool j_is_ready = false;
        uint32_t vk{};
        uint32_t vj{};
        rob_pointer_t qk;
        rob_pointer_t qj;
        uint32_t imm{};
    };

    struct ResolvedInstructionEntry {
        InsType type = InsType::NOOP;
        rob_pointer_t rob_pointer;
        uint32_t vk{};
        uint32_t vj{};
        uint32_t imm{};
        uint32_t starting_time;

        explicit ResolvedInstructionEntry(const ResolverEntry &ent) :
            type(ent.type), rob_pointer(ent.rob_pointer), vk(ent.vk), vj(ent.vj), imm(ent.imm) {
            const auto now = Clock::instance().now();
            starting_time = now;
        }
    };

    // A Common Dependency Resolver (CDR) is responsible for managing dependencies
    // It listens for broadcasts and waits for the dependencies to be resolved
    template <int Capacity>
    class CommonDependencyResolver {
        BufferedArray<ResolverEntry, Capacity> buffer;
        std::unique_ptr<CommonDataBus> cdb_ref;

    public:
        [[nodiscard]] bool full() const { return buffer.full(); }

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
                if (ent.type == ResolverEntryStatus::EXECUTING and ent.rob_pointer == rob_pointer) {
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
