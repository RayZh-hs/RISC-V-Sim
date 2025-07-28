// delayer.hpp
// - simulates real-life delay over tasks

#pragma once

#include "utility/clock.hpp"
#include "third_party/array.hpp"
#include <cassert>

namespace norb {
    template <typename T, int Delay, size_t Capacity>
    class Delayer {
        using storage_t = std::pair<uint32_t, T>;
        BufferedArray<storage_t, Capacity> buffer;
        BufferedArray<bool, Capacity> occupied;
        Lock push_lock, pop_lock;

    public:
        [[nodiscard]] bool full() const {
            return occupied.count(false) == 0;
        }

        void push(const T &value) {
            push_lock.lock();
            const auto now = Clock::instance().now();
            storage_t pair = std::make_pair(now, value);
            // insert into the first
            const auto idx = occupied.find(false);
            if (idx == -1) {
                throw std::out_of_range("out of range");
            }
            buffer.write_at(idx, pair);
            occupied.write_at(idx, true);
        }

        std::optional<T> pop() {
            pop_lock.lock();
            const auto now = Clock::instance().now();
            for (int i = 0; i < Capacity; i++) {
                if (occupied.read_at(i)) {
                    auto [time, val] = buffer.read_at(i);
                    assert(now > time);
                    if (now >= time + Delay) {
                        // this is the entry to pop
                        occupied.write_at(i, false);
                        return val;
                    }
                }
            }
            return std::nullopt;
        }
    };
}  // namespace norb
