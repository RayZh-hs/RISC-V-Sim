#pragma once

#include <array>
#include <stdexcept>
#include <utility/constants.hpp>

#include "utility/buffered.hpp"

namespace norb {
    template <typename T, size_t Capacity>
    class BufferedVector : public impl::BufferedFlushInterface_ {
        std::array<T, Capacity> old_{}, new_{};
        std::array<bool, Capacity> written;
        size_t old_size_, new_size_;

    public:
        BufferedVector() : old_size_(0), new_size_(0) {
            impl::BufferedManager::add(this);
            written.fill(false);
        }

        ~BufferedVector() override { impl::BufferedManager::remove(this); }

        // Read element at position (uses old state)
        T read_at(const size_t pos) {
            if (pos >= old_size_) {
                throw std::runtime_error("BufferedVector::read_at: pos >= size");
            }
            return old_[pos];
        }

        // Write element at position (must be within current size or at end for push_back)
        void write_at(const size_t pos, T value) {
            if (pos >= Capacity) {
                throw std::runtime_error("BufferedVector::write_at: pos >= Capacity");
            }
            if (pos > new_size_) {
                throw std::runtime_error("BufferedVector::write_at: pos > size (use push_back to extend)");
            }
            if (written[pos]) {
                throw AssertionError("[BufferedVector] Double write at pos=" + std::to_string(pos));
            }
            written[pos] = true;
            new_[pos] = value;
        }

        // Write element at position (must be within current size or at end for push_back)
        void remove_at(const size_t pos) {
            if (pos >= old_size_) {
                throw std::runtime_error("BufferedVector::remove_at: pos >= size");
            }
            if (pos > new_size_) {
                throw std::runtime_error("BufferedVector::remove_at: pos > size (use remove_at)");
            }
            auto written_backup = written;
            for (size_t i = pos; i < old_size_; i++) {
                if (written[i]) {
                    throw std::runtime_error("BufferedVector::remove_at: pos=" + std::to_string(i));
                }
                written_backup[i] = true;
            }
            written = written_backup;
            // copy the mem
            std::copy(old_.begin() + pos + 1, old_.end(), new_.begin() + pos);
            new_size_--;
        }

        // Get current size (old state)
        [[nodiscard]] size_t size() const { return old_size_; }

        // Get current capacity
        [[nodiscard]] size_t capacity() const { return Capacity; }

        // Check if vector is empty
        [[nodiscard]] bool empty() const { return old_size_ == 0; }

        // Push back a new element
        void push_back(const T& value) {
            if (old_size_ >= Capacity) {
                throw std::runtime_error("BufferedVector::push_back: vector is full");
            }
            if (written[old_size_]) {
                throw AssertionError("[BufferedVector] Double write at pos=" + std::to_string(new_size_));
            }
            written[old_size_] = true;
            new_[old_size_] = value;
            new_size_ = old_size_ + 1;
        }

        // Pop back (removes last element)
        void pop_back() {
            if (new_size_ == 0) {
                throw std::runtime_error("BufferedVector::pop_back: vector is empty");
            }
            new_size_--;
        }

        // Access front element (old state)
        T front() {
            if (old_size_ == 0) {
                throw std::runtime_error("BufferedVector::front: vector is empty");
            }
            return old_[0];
        }

        // Access back element (old state)
        T back() {
            if (old_size_ == 0) {
                throw std::runtime_error("BufferedVector::back: vector is empty");
            }
            return old_[old_size_ - 1];
        }

        // Clear the vector
        void clear() {
            new_size_ = 0;
            written.fill(false);
        }

        void flush() override {
            old_ = new_;
            old_size_ = new_size_;
            written.fill(false);
        }
    };
}  // namespace norb
