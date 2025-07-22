// buffered.hpp
// - Simulates non-blocking assignment behavior in hardware t-flop for atomic
// types
#pragma once
#include <vector>
#include <mutex>      // For thread safety
#include <algorithm>  // For std::remove

namespace norb {
namespace impl {

    // 1. Make this class a true abstract interface
    class BufferedFlushInterface_ {
    public:
        // CRITICAL FIX 1: Make flush() pure virtual for dynamic dispatch.
        // This forces derived classes to implement it.
        virtual void flush() = 0;

        // CRITICAL FIX 2: Add a virtual destructor for safe cleanup.
        virtual ~BufferedFlushInterface_() = default;
    };

    class BufferedManager {
        // Private constructor ensures it's a singleton.
        BufferedManager() = default;

        std::vector<BufferedFlushInterface_*> buffered_list;
        std::mutex list_mutex; // Mutex to protect the list

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
            std::lock_guard<std::mutex> lock(ins.list_mutex); // Lock for safety
            ins.buffered_list.push_back(buffered);
        }

        static void remove(const BufferedFlushInterface_* buffered) {
            auto& ins = get_instance();
            std::lock_guard<std::mutex> lock(ins.list_mutex); // Lock for safety

            // Use the more idiomatic and safer erase-remove idiom
            auto it = std::remove(ins.buffered_list.begin(), ins.buffered_list.end(), buffered);
            if (it != ins.buffered_list.end()) {
                ins.buffered_list.erase(it, ins.buffered_list.end());
            }
        }

        static void flush() {
            auto& ins = get_instance();
            std::lock_guard<std::mutex> lock(ins.list_mutex); // Lock for safety
            for (const auto i : ins.buffered_list) {
                // Now this correctly calls the derived class's flush()
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
    explicit Buffered() {
        impl::BufferedManager::add(this);
    }

    explicit Buffered(const T& ori) : old_value(ori), new_value(ori) {
        impl::BufferedManager::add(this);
    }

    ~Buffered() override {
        impl::BufferedManager::remove(this);
    }

    Buffered(const Buffered&) = delete;
    Buffered& operator=(const Buffered&) = delete;
    Buffered(Buffered&&) = delete;
    Buffered& operator=(Buffered&&) = delete;

    T read() const { return old_value; }

    void write(T value) {
        new_value = value;
    }

    void flush() override {
        old_value = new_value;
    }

    explicit operator T() const {
        return old_value;
    }

    Buffered& operator=(const T& value) {
        new_value = value;
        return *this;
    }
};

inline void buffered_flush() {
    impl::BufferedManager::flush();
}

}  // namespace norb