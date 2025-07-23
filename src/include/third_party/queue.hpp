#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace norb {

/**
 * A fixed-size circular queue (ring buffer).
 * Overwrites the oldest element when full.
 *
 * @tparam T The type of elements stored in the queue.
 * @tparam Capacity The maximum number of elements the queue can hold.
 */
template <typename T, size_t Capacity>
class CycleQueue {
public:
    // Returns the number of elements in the queue.
    constexpr size_t size() const noexcept { return size_; }

    // Returns the maximum number of elements the queue can hold.
    static constexpr size_t capacity() noexcept { return Capacity; }

    // Checks if the queue is empty.
    constexpr bool empty() const noexcept { return size_ == 0; }

    // Checks if the queue is full.
    constexpr bool full() const noexcept { return size_ == Capacity; }

    // Adds an element to the back of the queue.
    // If the queue is full, the oldest element is overwritten.
    void push(const T& value) {
        if (full()) { // Overwrite oldest element
            buffer_[head_] = value;
            head_ = (head_ + 1) % Capacity;
        } else { // Add to the end
            size_t tail = (head_ + size_) % Capacity;
            buffer_[tail] = value;
            size_++;
        }
    }

    // Constructs an element in-place at the back of the queue.
    // If the queue is full, the oldest element is overwritten.
    template <typename... Args>
    void emplace(Args&&... args) {
        if (full()) { // Overwrite oldest element
            buffer_[head_] = T(std::forward<Args>(args)...);
            head_ = (head_ + 1) % Capacity;
        } else { // Add to the end
            size_t tail = (head_ + size_) % Capacity;
            buffer_[tail] = T(std::forward<Args>(args)...);
            size_++;
        }
    }

    // Removes the element from the front of the queue. No-op if empty.
    void pop() noexcept {
        if (!empty()) {
            head_ = (head_ + 1) % Capacity;
            size_--;
        }
    }

    // Returns a reference to the front element. Undefined behavior if empty.
    T& front() { return buffer_[head_]; }
    const T& front() const { return buffer_[head_]; }

    // Returns a reference to the back element. Undefined behavior if empty.
    T& back() {
        size_t tail_idx = (head_ + size_ - 1) % Capacity;
        return buffer_[tail_idx];
    }
    const T& back() const {
        size_t tail_idx = (head_ + size_ - 1) % Capacity;
        return buffer_[tail_idx];
    }

private:
    static_assert(Capacity > 0, "CycleQueue capacity must be greater than 0");

    std::array<T, Capacity> buffer_{};
    size_t head_ = 0; // Index of the first element
    size_t size_ = 0; // Current number of elements
};

} // namespace norb