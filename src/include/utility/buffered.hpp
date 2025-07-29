// buffered.hpp
// - Simulates non-blocking assignment behavior in hardware t-flop for atomic
// types

#pragma once
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>
#include <iostream>

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

    }  // namespace templating

    // A utility class that ensures lock() is only called once per cycle
    class Lock : public impl::BufferedFlushInterface_ {
        bool value = false;

    public:
        Lock() { impl::BufferedManager::add(this); }

        ~Lock() override { impl::BufferedManager::remove(this); }

        void lock() {
            if (value) {
                throw AssertionError("Lock violated!");
            }
            value = true;
        }

        void flush() override { value = false; }
    };

    template <typename T>
    class Buffered : public impl::BufferedFlushInterface_ {
    private:
        T old_value;
        T new_value;
        Lock write_lock;

    public:
        explicit Buffered() { impl::BufferedManager::add(this); }

        explicit Buffered(const T& ori) : old_value(ori), new_value(ori) { impl::BufferedManager::add(this); }

        // ~Buffered() override {
        //     // std::cout << "Deallocating buffered: " << this << '\n';
        //     // impl::BufferedManager::remove(this);
        // }

        Buffered(const Buffered&) = delete;
        Buffered& operator=(const Buffered&) = delete;
        Buffered(Buffered&&) = delete;
        Buffered& operator=(Buffered&&) = delete;

        T read() const { return old_value; }

        void write(const T &value) {
            write_lock.lock();
            new_value = value;
        }

        void flush() override { old_value = new_value; }

        explicit operator T() const { return old_value; }

        Buffered& operator=(const T& value) {
            new_value = value;
            return *this;
        }
    };

    template <typename T>
    class TemporarilyBuffered : public impl::BufferedFlushInterface_ {
    private:
        std::optional<T> old_value = std::nullopt;
        std::optional<T> new_value = std::nullopt;
        Lock write_lock;

    public:
        explicit TemporarilyBuffered() { impl::BufferedManager::add(this); }
        explicit TemporarilyBuffered(const T& ori) : old_value(ori), new_value(ori) {
            impl::BufferedManager::add(this);
        }
        ~TemporarilyBuffered() override { impl::BufferedManager::remove(this); }

        auto read() const {
            return old_value;
        }

        void write(const T &value) {
            write_lock.lock();
            new_value = value;
        }

        void flush() override {
            old_value = new_value;
            new_value = std::nullopt;
        }
    };

    inline void buffered_flush() { impl::BufferedManager::flush(); }

}  // namespace norb
