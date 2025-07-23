#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace norb {

    template <typename T, size_t Capacity>
    class FixedQueue {
        static_assert(Capacity > 0, "FixedQueue capacity must be greater than 0");

    public:
        // Forward declarations for iterators
        template <bool IsConst>
        class base_iterator;
        using iterator = base_iterator<false>;
        using const_iterator = base_iterator<true>;

        // STL-compatible standard definitions
        using value_type = T;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;

        FixedQueue() noexcept = default;
        ~FixedQueue() { clear(); }

        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        [[nodiscard]] bool empty() const noexcept { return _size == 0; }
        [[nodiscard]] bool full() const noexcept { return _size == Capacity; }
        [[nodiscard]] size_type size() const noexcept { return _size; }
        [[nodiscard]] constexpr size_type capacity() const noexcept { return Capacity; }
        [[nodiscard]] constexpr size_type max_size() const noexcept { return Capacity; }

        reference front();
        const_reference front() const;

        reference back();
        const_reference back() const;

        reference operator[](size_type n);
        const_reference operator[](size_type n) const;

        reference at(size_type n);
        const_reference at(size_type n) const;

        void push(const T& value);
        void push(T&& value);
        void push_back(const T& value) { push(value); }
        void push_back(T&& value) { push(std::move(value)); }

        template <typename... Args>
        reference emplace(Args&&... args);
        template <typename... Args>
        reference emplace_back(Args&&... args) {
            return emplace(std::forward<Args>(args)...);
        }

        void pop();
        void pop_front() { pop(); }

        void clear();

    private:
        // Converts a logical index (0 to size-1) to a physical index in _storage
        size_type physical_index(size_type logical_idx) const noexcept { return (_head + logical_idx) % Capacity; }

        // Destroys the element at a given physical index
        void destroy_at(size_type physical_idx) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                // Access the value within the union to call its destructor
                _storage[physical_idx].~T();
            }
        }

        std::array<T, Capacity> _storage;
        size_type _head = 0;  // Physical index of the first element
        size_type _tail = 0;  // Physical index of the next available slot
        size_type _size = 0;  // Number of elements in the queue
    };

    template <typename T, size_t Capacity>
    template <bool IsConst>
    class FixedQueue<T, Capacity>::base_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using container_type = std::conditional_t<IsConst, const FixedQueue, FixedQueue>;

        base_iterator() noexcept : _queue(nullptr), _physical_idx(0) {}

        operator base_iterator<true>() const { return base_iterator<true>(_queue, _physical_idx); }

        reference operator*() const {
            // For safety, assert that this iterator is not dangling or the end iterator.
            // A dangling iterator is one whose logical position is no longer valid.
            assert(_queue && "Cannot dereference a default-constructed iterator.");
            assert(is_dereferenceable() && "Cannot dereference end or dangling iterator.");
            return _queue->_storage[_physical_idx];
        }

        pointer operator->() const { return &operator*(); }

        reference operator[](difference_type n) const { return *(*this + n); }

        base_iterator& operator++() {
            _physical_idx = (_physical_idx + 1) % Capacity;
            return *this;
        }
        base_iterator operator++(int) {
            base_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        base_iterator& operator--() {
            _physical_idx = (_physical_idx + Capacity - 1) % Capacity;
            return *this;
        }
        base_iterator operator--(int) {
            base_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        base_iterator& operator+=(difference_type n) {
            // To correctly calculate the new physical index, we must go through
            // the logical index, as physical indices are not contiguous.
            difference_type current_logical = logical_index();
            _physical_idx = _queue->physical_index(current_logical + n);
            return *this;
        }
        base_iterator& operator-=(difference_type n) { return *this += -n; }

        friend base_iterator operator+(base_iterator it, difference_type n) {
            it += n;
            return it;
        }
        friend base_iterator operator+(difference_type n, base_iterator it) {
            it += n;
            return it;
        }
        friend base_iterator operator-(base_iterator it, difference_type n) {
            it -= n;
            return it;
        }

        difference_type operator-(const base_iterator& other) const {
            assert(_queue == other._queue && "Cannot subtract iterators from different containers.");
            return static_cast<difference_type>(this->logical_index()) -
                static_cast<difference_type>(other.logical_index());
        }

        // Comparisons must be based on logical position, not physical index.
        bool operator==(const base_iterator& other) const { return _physical_idx == other._physical_idx; }
        bool operator!=(const base_iterator& other) const { return !(*this == other); }
        bool operator<(const base_iterator& other) const { return this->logical_index() < other.logical_index(); }
        bool operator>(const base_iterator& other) const { return other < *this; }
        bool operator<=(const base_iterator& other) const { return !(other < *this); }
        bool operator>=(const base_iterator& other) const { return !(*this < other); }

        // Null pointer
        inline static base_iterator null{};
        [[nodiscard]] bool is_null() const { return _queue == nullptr; }
        void make_null() {
            _queue = nullptr;
            _physical_idx = -1;
        }

    private:
        friend class FixedQueue;
        base_iterator(container_type* queue, size_type physical_idx) noexcept :
            _queue(queue), _physical_idx(physical_idx) {}

        // Helper to calculate the logical index from the physical index
        size_type logical_index() const {
            assert(_queue && "Iterator is not associated with a container.");
            // This formula correctly handles wrapping around the buffer.
            return (_physical_idx - _queue->_head + Capacity) % Capacity;
        }

        // Helper to check if the iterator points to a valid, accessible element
        bool is_dereferenceable() const { return logical_index() < _queue->size(); }

        container_type* _queue;
        size_type _physical_idx;  // Absolute index into the _storage array
    };

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::iterator FixedQueue<T, Capacity>::begin() noexcept {
        return iterator(this, _head);
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_iterator FixedQueue<T, Capacity>::begin() const noexcept {
        return const_iterator(this, _head);
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_iterator FixedQueue<T, Capacity>::cbegin() const noexcept {
        return const_iterator(this, _head);
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::iterator FixedQueue<T, Capacity>::end() noexcept {
        // end() iterator points to the physical slot where the next element would be placed.
        return iterator(this, _tail);
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_iterator FixedQueue<T, Capacity>::end() const noexcept {
        return const_iterator(this, _tail);
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_iterator FixedQueue<T, Capacity>::cend() const noexcept {
        return const_iterator(this, _tail);
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::reference FixedQueue<T, Capacity>::front() {
        if (empty()) {
            throw std::out_of_range("front() called on empty FixedQueue");
        }
        return _storage[_head];
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_reference FixedQueue<T, Capacity>::front() const {
        if (empty()) {
            throw std::out_of_range("front() called on empty FixedQueue");
        }
        return _storage[_head];
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::reference FixedQueue<T, Capacity>::back() {
        if (empty()) {
            throw std::out_of_range("back() called on empty FixedQueue");
        }
        // The last element is at the physical slot just before _tail
        size_type back_idx = (_tail + Capacity - 1) % Capacity;
        return _storage[back_idx];
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_reference FixedQueue<T, Capacity>::back() const {
        if (empty()) {
            throw std::out_of_range("back() called on empty FixedQueue");
        }
        size_type back_idx = (_tail + Capacity - 1) % Capacity;
        return _storage[back_idx];
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::reference FixedQueue<T, Capacity>::operator[](size_type n) {
        return _storage[physical_index(n)];
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_reference FixedQueue<T, Capacity>::operator[](size_type n) const {
        return _storage[physical_index(n)];
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::reference FixedQueue<T, Capacity>::at(size_type n) {
        if (n >= _size) {
            throw std::out_of_range("FixedQueue::at() index out of range");
        }
        return operator[](n);
    }

    template <typename T, size_t Capacity>
    typename FixedQueue<T, Capacity>::const_reference FixedQueue<T, Capacity>::at(size_type n) const {
        if (n >= _size) {
            throw std::out_of_range("FixedQueue::at() index out of range");
        }
        return operator[](n);
    }

    template <typename T, size_t Capacity>
    void FixedQueue<T, Capacity>::push(const T& value) {
        emplace(value);
    }

    template <typename T, size_t Capacity>
    void FixedQueue<T, Capacity>::push(T&& value) {
        emplace(std::move(value));
    }

    template <typename T, size_t Capacity>
    template <typename... Args>
    typename FixedQueue<T, Capacity>::reference FixedQueue<T, Capacity>::emplace(Args&&... args) {
        if (full()) {
            // Overwrite the oldest element
            destroy_at(_head);
            _head = (_head + 1) % Capacity;
        } else {
            _size++;
        }

        // Construct the new element at the tail using placement new
        new (&_storage[_tail]) T(std::forward<Args>(args)...);

        reference new_element = _storage[_tail];
        _tail = (_tail + 1) % Capacity;
        return new_element;
    }

    template <typename T, size_t Capacity>
    void FixedQueue<T, Capacity>::pop() {
        if (empty()) {
            return;
        }
        destroy_at(_head);
        _head = (_head + 1) % Capacity;
        _size--;
    }

    template <typename T, size_t Capacity>
    void FixedQueue<T, Capacity>::clear() {
        while (!empty()) {
            pop();
        }
    }

}  // namespace norb
