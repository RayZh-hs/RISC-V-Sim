// buffered.hpp
// - Simulates non-blocking assignment behavior in hardware t-flop for atomic
// types

#pragma once
#include <algorithm>
#include <vector>

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

    template <typename T>
    class Buffered : public impl::BufferedFlushInterface_ {
    private:
        T old_value;
        T new_value;

    public:
        explicit Buffered() { impl::BufferedManager::add(this); }

        explicit Buffered(const T& ori) : old_value(ori), new_value(ori) { impl::BufferedManager::add(this); }

        ~Buffered() override { impl::BufferedManager::remove(this); }

        Buffered(const Buffered&) = delete;
        Buffered& operator=(const Buffered&) = delete;
        Buffered(Buffered&&) = delete;
        Buffered& operator=(Buffered&&) = delete;

        T read() const { return old_value; }

        void write(T value) { new_value = value; }

        void flush() override { old_value = new_value; }

        explicit operator T() const { return old_value; }

        Buffered& operator=(const T& value) {
            new_value = value;
            return *this;
        }
    };

    inline void buffered_flush() { impl::BufferedManager::flush(); }

}  // namespace norb
