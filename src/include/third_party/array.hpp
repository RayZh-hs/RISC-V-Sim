#pragma once

#include <array>
#include <stdexcept>
#include <utility/constants.hpp>

#include "utility/buffered.hpp"

namespace norb {
    template <typename T, size_t Capacity>
    class BufferedArray : public impl::BufferedFlushInterface_ {
        std::array<T, Capacity> old_{}, new_{};
        std::array<bool, Capacity> written;

    public:
        BufferedArray() {
            impl::BufferedManager::add(this);
            written.fill(false);
        }

        ~BufferedArray() override { impl::BufferedManager::remove(this); }

        T read_at(const size_t pos) {
            if (pos >= Capacity) {
                throw std::runtime_error("BufferedArray::read_at: pos >= Capacity");
            }
            return old_[pos];
        }

        void write_at(const size_t pos, T value) {
            if (pos >= Capacity) {
                throw std::runtime_error("BufferedArray::write_at: pos >= Capacity");
            }
            if (written[pos]) {
                throw AssertionError("[BufferedArray] Double write at pos=" + std::to_string(pos));
            }
            written[pos] = true;
            new_[pos] = value;
        }

        // returns the number of elements that satisfy the predicate
        template <typename Predicate>
        int count_if(Predicate pred) const {
            int count = 0;
            for (size_t i = 0; i < Capacity; ++i) {
                if (pred(old_[i])) {
                    count++;
                }
            }
            return count;
        }

        // returns the index of the first element that matches the predicate, or -1 if not found
        template <typename Predicate>
        int find_if(Predicate pred) const {
            for (size_t i = 0; i < Capacity; ++i) {
                if (pred(old_[i])) {
                    return i;
                }
            }
            return -1;  // Not found
        }

        int count(const T& value) const {
            return count_if([&value](const T& elem) { return elem == value; });
        }

        int find(const T& value) const {
            return find_if([&value](const T& elem) { return elem == value; });
        }

        void flush() override {
            old_ = new_;
            written.fill(false);
        }
    };
}  // namespace norb
