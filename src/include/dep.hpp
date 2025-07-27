// dep.hpp
// - declares the Dependency Resolvers for rs, lsb and ba

#pragma once

#include <memory>
#include <optional>

#include "cdb.hpp"
#include "rob_types.hpp"  // For ResolverEntry and related types
#include "third_party/array.hpp"
#include "third_party/logger.hpp"
#include "third_party/queue.hpp"
#include "utility/chan.hpp"
#include "utility/clock.hpp"

namespace norb::riscv {

    // Base class for dependency resolvers
    template <int Capacity>
    class DependencyResolver {
    protected:
        std::unique_ptr<CommonDataBus> cdb_ref;
        ChannelReader<ResolverEntry> chan_inbound{};

    public:
        virtual void bind_inbound_to(norb::ChannelWriter<ResolverEntry> &chw) { make_channel(chw, chan_inbound); }

        [[nodiscard]] virtual bool full() const = 0;
        virtual void listen_inbound() = 0;
        virtual void listen_broadcast() = 0;
        virtual std::optional<ResolvedInstructionEntry> get_ready_entry() = 0;
        virtual void submit_executed_entry(rob_pointer_t rob_pointer, uint32_t value) = 0;
        virtual void clear() = 0;  // Add clear method
        virtual ~DependencyResolver() = default;
    };

    // A Random Dependency Resolver (RDR) is responsible for managing dependencies
    // It listens for broadcasts and waits for the dependencies to be resolved
    // The output is in random order
    template <int Capacity>
    class RandomDependencyResolver : public DependencyResolver<Capacity> {
        BufferedArray<ResolverEntry, Capacity> buffer;
        std::shared_ptr<CommonDataBus> cdb_ref;
        ChannelReader<ResolverEntry> chan_inbound{};

    public:
        void bind_inbound_to(norb::ChannelWriter<ResolverEntry> &chw) override { make_channel(chw, chan_inbound); }
        void load_cdb(CommonDataBus &cdb) {
            cdb_ref = std::make_shared<CommonDataBus>(cdb);
        }

        [[nodiscard]] bool full() const {
            return buffer.find_if(
                       [](const ResolverEntry &entry) { return entry.status == ResolverEntryStatus::EMPTY; }) == -1;
        }

        void listen_inbound() override {
            auto &log = Logger::get();
            if (chan_inbound.has_data()) {
                log.as(LogLevel::DEBUG) << "[RDR] Acquired new data";
                const auto idx_new_entry = buffer.find_if(
                    [](const ResolverEntry &entry) { return entry.status == ResolverEntryStatus::EMPTY; });
                if (idx_new_entry == -1) {
                    log.as(LogLevel::DEBUG) << "[RDR] The buffer is already full";
                    return;  // this means that the array is full
                }
                const auto new_entry = chan_inbound.read();
                // now insert into the place of the new entry
                log.as(LogLevel::INFO) << "[RDR] Writing new instruction=" << new_entry.repr();
                buffer.write_at(idx_new_entry, new_entry);
            }
        }

        void listen_broadcast() override {
            auto &log = Logger::get();
            if (cdb_ref->empty()) return;
            auto news = cdb_ref->read();
            log.as(LogLevel::DEBUG) << "[RDR] Read instruction: [rob=" << news.rob_pointer.repr()
                                    << ", val=" << news.value << "]";
            for (int i = 0; i < Capacity; ++i) {
                auto ent = buffer.read_at(i);
                if (ent.status == ResolverEntryStatus::PENDING) {
                    if (ent.qk == news.rob_pointer and not ent.k_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[RDR] Resolving entry at index " << i << " with qk=" << ent.qk.repr()
                            << " | target entry rob=" << ent.rob_pointer.repr();
                        ent.k_is_ready = true;
                        ent.vk = news.value;
                    }
                    if (ent.qj == news.rob_pointer and not ent.j_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[RDR] Resolving entry at index " << i << " with qj=" << ent.qj.repr()
                            << " | target entry rob=" << ent.rob_pointer.repr();
                        ent.j_is_ready = true;
                        ent.vj = news.value;
                    }
                    if (ent.k_is_ready and ent.j_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[RDR] Entry at index " << i << " is ready with vk=" << ent.vk << " and vj=" << ent.vj;
                        ent.status = ResolverEntryStatus::READY;
                    }
                }
                // replace the original entry with the new version
                buffer.write_at(i, ent);
            }
        }

        std::optional<ResolvedInstructionEntry> get_ready_entry() override {
            auto &log = Logger::get();
            const auto ind =
                buffer.find_if([](const ResolverEntry &entry) { return entry.status == ResolverEntryStatus::READY; });
            if (ind == -1) {
                log.as(LogLevel::DEBUG) << "[RDR] No ready entry found";
                return std::nullopt;
            } else {
                // set state to EXECUTING
                auto ent = buffer.read_at(ind);
                ent.status = ResolverEntryStatus::EXECUTING;
                log.as(LogLevel::DEBUG) << "[RDR] Found ready entry at index " << ind
                                        << " | rob_pointer=" << ent.rob_pointer.repr();
                buffer.write_at(ind, ent);
                return ResolvedInstructionEntry(ent);
            }
        }

        void submit_executed_entry(rob_pointer_t rob_pointer, uint32_t value) override {
            auto &log = Logger::get();
            // search the database for the rob_pointer
            for (int i = 0; i < Capacity; ++i) {
                auto ent = buffer.read_at(i);
                if (ent.status == ResolverEntryStatus::EXECUTING and ent.rob_pointer == rob_pointer) {
                    log.as(LogLevel::DEBUG) << "[RDR] Submitting executed entry at index " << i
                                            << " with rob_pointer=" << rob_pointer.repr() << " and value=" << value;
                    // update the array
                    ent.status = ResolverEntryStatus::EMPTY;
                    buffer.write_at(i, ent);
                    // submit to the CDB
                    cdb_ref->broadcast({rob_pointer, value});
                }
            }
        }

        void clear() override {
            // Clear all entries in the buffer by setting them to EMPTY
            for (int i = 0; i < Capacity; ++i) {
                auto ent = buffer.read_at(i);
                ent.status = ResolverEntryStatus::EMPTY;
                buffer.write_at(i, ent);
            }
        }
    };

    // A Sequential Dependency Resolver (SDR) is responsible for managing dependencies
    // Different from RDR, it processes entries in the order they are received
    template <int Capacity>
    class SequentialDependencyResolver : public DependencyResolver<Capacity> {
        FixedBufferedQueue<ResolverEntry, Capacity> buffer;
        std::shared_ptr<CommonDataBus> cdb_ref;
        ChannelReader<ResolverEntry> chan_inbound{};

    public:
        void bind_inbound_to(norb::ChannelWriter<ResolverEntry> &chw) override { make_channel(chw, chan_inbound); }
        void load_cdb(CommonDataBus &cdb) {
            cdb_ref = std::make_shared<CommonDataBus>(cdb);
        }

        [[nodiscard]] bool full() const override { return buffer.full(); }

        void listen_inbound() override {
            auto &log = Logger::get();
            if (chan_inbound.has_data()) {
                log.as(LogLevel::DEBUG) << "[SDR] Acquired new data";
                if (buffer.full()) {
                    log.as(LogLevel::DEBUG) << "[SDR] The buffer is already full";
                    return;
                }
                const auto new_entry = chan_inbound.read();
                // now insert into the place of the new entry
                log.as(LogLevel::INFO) << "[SDR] Writing new instruction=" << new_entry.repr();
                buffer.push(new_entry);
            }
        }

        void listen_broadcast() override {
            auto &log = Logger::get();
            if (cdb_ref->empty()) return;
            auto news = cdb_ref->read();
            log.as(LogLevel::DEBUG) << "[SDR] Read instruction: [rob=" << news.rob_pointer.repr()
                                    << ", val=" << news.value << "]";
            for (int i = 0; i < buffer.size(); ++i) {
                auto ent = buffer.read_at(i);
                if (ent.status == ResolverEntryStatus::PENDING) {
                    if (ent.qk == news.rob_pointer and not ent.k_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[SDR] Resolving entry at index " << i << " with qk=" << ent.qk.repr()
                            << " | target entry rob=" << ent.rob_pointer.repr();
                        ent.k_is_ready = true;
                        ent.vk = news.value;
                    }
                    if (ent.qj == news.rob_pointer and not ent.j_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[SDR] Resolving entry at index " << i << " with qj=" << ent.qj.repr()
                            << " | target entry rob=" << ent.rob_pointer.repr();
                        ent.j_is_ready = true;
                        ent.vj = news.value;
                    }
                    if (ent.k_is_ready and ent.j_is_ready) {
                        log.as(LogLevel::DEBUG)
                            << "[RDR] Entry at index " << i << " is ready with vk=" << ent.vk << " and vj=" << ent.vj;
                        ent.status = ResolverEntryStatus::READY;
                    }
                }
                // replace the original entry with the new version
                buffer.write_at(i, ent);
            }
        }

        std::optional<ResolvedInstructionEntry> get_ready_entry() override {
            auto &log = Logger::get();
            if (buffer.empty())
                return std::nullopt;
            auto ent = buffer.front();
            if (ent.status == ResolverEntryStatus::READY) {
                ent.status = ResolverEntryStatus::EXECUTING;
                buffer.write_at(0, ent);
                log.as(LogLevel::DEBUG) << "[SDR] Found ready entry at index " << 0
                                        << " | rob_pointer=" << ent.rob_pointer.repr();
                return ResolvedInstructionEntry(ent);
            }
            return std::nullopt;
        }

        void submit_executed_entry(rob_pointer_t rob_pointer, uint32_t value) override {
            auto &log = Logger::get();
            // search the database for the rob_pointer
            for (int i = 0; i < buffer.size(); ++i) {
                auto ent = buffer.read_at(i);
                if (ent.status == ResolverEntryStatus::EXECUTING and ent.rob_pointer == rob_pointer) {
                    log.as(LogLevel::DEBUG) << "[SDR] Submitting executed entry at index " << i
                                            << " with rob_pointer=" << rob_pointer.repr() << " and value=" << value;
                    // update the array
                    ent.status = ResolverEntryStatus::EMPTY;
                    buffer.write_at(i, ent);
                    if (i == 0) {
                        buffer.pop();
                    }
                    // submit to the CDB
                    cdb_ref->broadcast({rob_pointer, value});
                    return;
                }
            }
        }

        void clear() override {
            // Clear the buffer queue
            buffer.clear();
        }
    };
}  // namespace norb::riscv
