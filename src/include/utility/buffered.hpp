// buffered.hpp
// - Simulates non-blocking assignment behavior in hardware t-flop for atomic
// types

#pragma once
#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#include "third_party/logger.hpp"

namespace norb {
    class AssertionError final : public std::runtime_error {
    public:
        explicit AssertionError(const std::string& message) : std::runtime_error(message) {}
    };
}  // namespace norb

namespace norb {
    namespace impl {

        class BufferedFlushInterface_ {
        public:
            virtual void flush() = 0;

            virtual ~BufferedFlushInterface_() = default;
        };

        class BufferedManager {
            BufferedManager() = default;

            std::vector<BufferedFlushInterface_*> buffered_list;

            static BufferedManager& get_instance() {
                static BufferedManager singleton;
                return singleton;
            }

        public:
            // Deleted copy/move to enforce singleton pattern
            BufferedManager(const BufferedManager&) = delete;
            BufferedManager& operator=(const BufferedManager&) = delete;

            static void add(BufferedFlushInterface_* buffered) {
                auto& ins = get_instance();
                ins.buffered_list.push_back(buffered);
            }

            static void remove(const BufferedFlushInterface_* buffered) {
                auto& ins = get_instance();

                auto it = std::remove(ins.buffered_list.begin(), ins.buffered_list.end(), buffered);
                if (it != ins.buffered_list.end()) {
                    ins.buffered_list.erase(it, ins.buffered_list.end());
                }
            }

            static void flush() {
                auto& ins = get_instance();
                for (const auto i : ins.buffered_list) {
                    i->flush();
                }
            }
        };

    }  // namespace impl

    // A utility class that ensures lock() is only called once per cycle
    class Lock : public impl::BufferedFlushInterface_ {
        bool value = false;
        int last_written_after_line = 0;

    public:
        Lock() { impl::BufferedManager::add(this); }

        ~Lock() override { impl::BufferedManager::remove(this); }

        void lock() {
            if (value) {
                throw AssertionError("Lock violated! Last written before logger line: " +
                                     std::to_string(last_written_after_line));
            }
            value = true;
            last_written_after_line = Logger::getLineNumber();
        }

        void flush() override { value = false; }
    };

    template <typename T>
    class Buffered : public impl::BufferedFlushInterface_ {
    private:
        T old_value{};
        T new_value{};
#ifndef NO_RUNTIME_CHECKS
        Lock write_lock{};
#endif

    public:
        explicit Buffered() { impl::BufferedManager::add(this); }

        explicit Buffered(const T& ori) : old_value(ori), new_value(ori) { impl::BufferedManager::add(this); }

        ~Buffered() override {
            impl::BufferedManager::remove(this);
        }

        Buffered(const Buffered&) = delete;
        Buffered& operator=(const Buffered&) = delete;
        Buffered(Buffered&&) = delete;
        Buffered& operator=(Buffered&&) = delete;

        T read() const { return old_value; }

        void write(const T& value) {
#ifndef NO_RUNTIME_CHECKS
            write_lock.lock();
#endif
            new_value = value;
        }

        void flush() override { old_value = new_value; }

        explicit operator T() const { return old_value; }

        Buffered& operator=(const T& value) {
#ifndef NO_RUNTIME_CHECKS
            write_lock.lock();
#endif
            new_value = value;
            return *this;
        }
    };

    // This class does not guarantee non-blocking assignment! Use with caution.
    template <typename T>
    class ConsciouslyBuffered : public impl::BufferedFlushInterface_ {
    private:
        T old_value{};
        T new_value{};
        bool has_been_modified = false;

    public:
        explicit ConsciouslyBuffered() { impl::BufferedManager::add(this); }

        explicit ConsciouslyBuffered(const T& ori) : old_value(ori), new_value(ori) { impl::BufferedManager::add(this); }

        ~ConsciouslyBuffered() override {
            impl::BufferedManager::remove(this);
        }

        ConsciouslyBuffered(const ConsciouslyBuffered&) = delete;
        ConsciouslyBuffered& operator=(const ConsciouslyBuffered&) = delete;
        ConsciouslyBuffered(ConsciouslyBuffered&&) = delete;
        ConsciouslyBuffered& operator=(ConsciouslyBuffered&&) = delete;

        T read() const { return old_value; }

        T read_new() const { return new_value; }

        void write(const T& value) {
            if (has_been_modified)
                throw AssertionError("ConsciouslyBuffered write violated! Value has already been modified.");
            new_value = value;
            has_been_modified = true;
        }

        // allows the user to overwrite the value without checking if it has been modified
        void overwrite(const T& value) {
            new_value = value;
            has_been_modified = true;
        }

        [[nodiscard]] bool is_modified() const {
            return has_been_modified;
        }

        void flush() override {
            old_value = new_value;
            has_been_modified = false;
        }

        explicit operator T() const { return old_value; }

        ConsciouslyBuffered& operator=(const T& value) {
            new_value = value;
            has_been_modified = true;
            return *this;
        }
    };

    template <typename T>
    class TemporarilyBuffered : public impl::BufferedFlushInterface_ {
    private:
        std::optional<T> old_value = std::nullopt;
        std::optional<T> new_value = std::nullopt;
#ifndef NO_RUNTIME_CHECKS
        Lock write_lock;
#endif

    public:
        explicit TemporarilyBuffered() { impl::BufferedManager::add(this); }
        explicit TemporarilyBuffered(const T& ori) : old_value(ori), new_value(ori) {
            impl::BufferedManager::add(this);
        }
        ~TemporarilyBuffered() override { impl::BufferedManager::remove(this); }

        auto read() const { return old_value; }

        void write(const T& value) {
#ifndef NO_RUNTIME_CHECKS
            write_lock.lock();
#endif
            new_value = value;
        }

        void flush() override {
            old_value = new_value;
            new_value = std::nullopt;
        }
    };

    inline void buffered_flush() { impl::BufferedManager::flush(); }

}  // namespace norb
