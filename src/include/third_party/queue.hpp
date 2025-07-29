//    ___
//   / _ \   _   _    ___   _   _    ___
//  | | | | | | | |  / _ \ | | | |  / _ \
//  | |_| | | |_| | |  __/ | |_| | |  __/
//   \__\_\  \__,_|  \___|  \__,_|  \___|
//
// A specialized implementation of hardware-compatible queue

#pragma once
#include <cassert>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "utility/buffered.hpp"
#include "utility/constants.hpp"

namespace norb {
    template <typename T, size_t Capacity>
    class FixedQueue {
        static_assert(Capacity > 0, "FixedQueue capacity must be greater than 0");
        static constexpr size_t InternalCapacity = Capacity + 1;

    public:
        // Forward declarations for iterators
        template <bool IsConst>
        class base_iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using size_type = size_t;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = std::conditional_t<IsConst, const T*, T*>;
            using reference = std::conditional_t<IsConst, const T&, T&>;
            using container_type = std::conditional_t<IsConst, const FixedQueue, FixedQueue>;

            base_iterator() noexcept : _queue(nullptr), _physical_idx(0) {}

            // Allow conversion from non-const to const iterator
            operator base_iterator<true>() const { return base_iterator<true>(_queue, _physical_idx); }

            reference operator*() const {
                assert(_queue && "Cannot dereference a default-constructed iterator.");
                assert(is_dereferenceable() && "Cannot dereference end or dangling iterator.");
                return _queue->_storage[_physical_idx];
            }

            pointer operator->() const { return &operator*(); }

            reference operator[](difference_type n) const { return *(*this + n); }

            base_iterator& operator++() {
                _physical_idx = (_physical_idx + 1) % container_type::InternalCapacity;
                return *this;
            }
            base_iterator operator++(int) {
                base_iterator tmp = *this;
                ++(*this);
                return tmp;
            }
            base_iterator& operator--() {
                _physical_idx =
                    (_physical_idx + container_type::InternalCapacity - 1) % container_type::InternalCapacity;
                return *this;
            }
            base_iterator operator--(int) {
                base_iterator tmp = *this;
                --(*this);
                return tmp;
            }

            base_iterator& operator+=(difference_type n) {
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

            bool operator==(const base_iterator& other) const { return _physical_idx == other._physical_idx; }
            bool operator!=(const base_iterator& other) const { return !(*this == other); }
            bool operator<(const base_iterator& other) const { return this->logical_index() < other.logical_index(); }
            bool operator>(const base_iterator& other) const { return other < *this; }
            bool operator<=(const base_iterator& other) const { return !(other < *this); }
            bool operator>=(const base_iterator& other) const { return !(*this < other); }

        private:
            friend class FixedQueue;
            base_iterator(container_type* queue, size_type physical_idx) noexcept :
                _queue(queue), _physical_idx(physical_idx) {}

            size_type logical_index() const {
                assert(_queue && "Iterator is not associated with a container.");
                return (_physical_idx - _queue->_head + container_type::InternalCapacity) %
                    container_type::InternalCapacity;
            }

            bool is_dereferenceable() const { return _physical_idx != _queue->_tail; }

            // Added for hash support - returns physical index for hashing
            [[nodiscard]] size_type physical_index() const noexcept { return _physical_idx; }

            container_type* _queue;
            size_type _physical_idx;
        };

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
        ~FixedQueue() = default;

        iterator begin() noexcept { return iterator(this, _head); }
        const_iterator begin() const noexcept { return const_iterator(this, _head); }
        const_iterator cbegin() const noexcept { return const_iterator(this, _head); }

        iterator end() noexcept { return iterator(this, _tail); }
        const_iterator end() const noexcept { return const_iterator(this, _tail); }
        const_iterator cend() const noexcept { return const_iterator(this, _tail); }

        [[nodiscard]] bool empty() const noexcept { return _head == _tail; }
        [[nodiscard]] bool full() const noexcept { return (_tail + 1) % InternalCapacity == _head; }
        [[nodiscard]] size_type size() const noexcept {
            if (_tail >= _head) {
                return _tail - _head;
            }
            return InternalCapacity - (_head - _tail);
        }
        [[nodiscard]] constexpr size_type capacity() const noexcept { return Capacity; }
        [[nodiscard]] constexpr size_type max_size() const noexcept { return Capacity; }

        reference front() {
            if (empty()) {
                throw std::out_of_range("front() called on empty FixedQueue");
            }
            return _storage[_head];
        }
        const_reference front() const {
            if (empty()) {
                throw std::out_of_range("front() called on empty FixedQueue");
            }
            return _storage[_head];
        }

        reference back() {
            if (empty()) {
                throw std::out_of_range("back() called on empty FixedQueue");
            }
            size_type back_idx = (_tail + InternalCapacity - 1) % InternalCapacity;
            return _storage[back_idx];
        }
        const_reference back() const {
            if (empty()) {
                throw std::out_of_range("back() called on empty FixedQueue");
            }
            size_type back_idx = (_tail + InternalCapacity - 1) % InternalCapacity;
            return _storage[back_idx];
        }

        reference operator[](size_type n) { return _storage[physical_index(n)]; }
        const_reference operator[](size_type n) const { return _storage[physical_index(n)]; }

        reference at(size_type n) {
            if (n >= size()) {
                throw std::out_of_range("FixedQueue::at() index out of range");
            }
            return operator[](n);
        }
        const_reference at(size_type n) const {
            if (n >= size()) {
                throw std::out_of_range("FixedQueue::at() index out of range");
            }
            return operator[](n);
        }

        void push(const T& value) { emplace(value); }
        void push(T&& value) { emplace(std::move(value)); }
        void push_back(const T& value) { push(value); }
        void push_back(T&& value) { push(std::move(value)); }

        template <typename... Args>
        reference emplace(Args&&... args) {
            if (full()) {
                _head = (_head + 1) % InternalCapacity;
            }

            _storage[_tail] = T(std::forward<Args>(args)...);

            reference new_element = _storage[_tail];
            _tail = (_tail + 1) % InternalCapacity;
            return new_element;
        }
        template <typename... Args>
        reference emplace_back(Args&&... args) {
            return emplace(std::forward<Args>(args)...);
        }

        void pop() {
            if (empty()) {
                return;
            }
            _head = (_head + 1) % InternalCapacity;
        }
        void pop_front() { pop(); }

        void clear() {
            _head = 0;
            _tail = 0;
        }

    private:
        size_type physical_index(size_type logical_idx) const noexcept {
            return (_head + logical_idx) % InternalCapacity;
        }
        T _storage[InternalCapacity];

        size_type _head = 0;  // Physical index of the first element
        size_type _tail = 0;  // Physical index of the next available slot
    };

    template <typename T, size_t Capacity>
    class FixedBufferedQueue {
        static_assert(Capacity > 0, "FixedBufferedQueue capacity must be greater than 0");
        static constexpr size_t InternalCapacity = Capacity + 1;

    public:
        template <bool IsConst>
        class base_iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using size_type = size_t;
            using difference_type = std::ptrdiff_t;
            // operator* returns by value, so the "reference" is a value type.
            using reference = T;
            using pointer = const T*;
            using container_type = std::conditional_t<IsConst, const FixedBufferedQueue, FixedBufferedQueue>;

            base_iterator() noexcept : _queue(nullptr), _physical_idx(0) {}

            operator base_iterator<true>() const { return base_iterator<true>(_queue, _physical_idx); }

            reference operator*() const { return read(); }

            // Returns the committed value by value.
            T read() const {
                assert(_queue && "Cannot dereference a default-constructed iterator.");
                assert(is_dereferenceable() && "Cannot dereference end or dangling iterator.");
                return _queue->_storage[_physical_idx].read();
            }

            // Write to the buffered value. Only available for non-const iterators.
            template <bool Q = IsConst, typename = std::enable_if_t<!Q>>
            void write(const T& value) {
                assert(_queue && "Cannot dereference a default-constructed iterator.");
                assert(is_dereferenceable() && "Cannot dereference end or dangling iterator.");
                _queue->_storage[_physical_idx].write(value);
            }

            [[nodiscard]] int repr() const {
                return _physical_idx;
            }

            template <typename Q>
            auto that_of(FixedBufferedQueue<Q, Capacity> *other) const {
                return typename FixedBufferedQueue<Q, Capacity>::base_iterator<IsConst>(other, _physical_idx);
            }

            [[nodiscard]] int physical_index() const {
                // assert(_queue && "Iterator is not associated with a container.");
                return _physical_idx;
            }

            base_iterator& operator++() {
                _physical_idx = (_physical_idx + 1) % InternalCapacity;
                return *this;
            }
            base_iterator operator++(int) {
                base_iterator tmp = *this;
                ++(*this);
                return tmp;
            }
            base_iterator& operator--() {
                _physical_idx = (_physical_idx + InternalCapacity - 1) % InternalCapacity;
                return *this;
            }
            base_iterator operator--(int) {
                base_iterator tmp = *this;
                --(*this);
                return tmp;
            }

            base_iterator& operator+=(difference_type n) {
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

            bool operator==(const base_iterator& other) const { return _physical_idx == other._physical_idx; }
            bool operator!=(const base_iterator& other) const { return !(*this == other); }
            bool operator<(const base_iterator& other) const { return this->logical_index() < other.logical_index(); }
            bool operator>(const base_iterator& other) const { return other < *this; }
            bool operator<=(const base_iterator& other) const { return !(other < *this); }
            bool operator>=(const base_iterator& other) const { return !(*this < other); }

            static base_iterator make_dummy(const size_type physical_idx) {
                return base_iterator(nullptr, physical_idx);
            }

        private:
            friend class FixedBufferedQueue<T, Capacity>;
            template<typename, size_t> friend class FixedBufferedQueue;
            base_iterator(container_type* queue, size_type physical_idx) noexcept :
                _queue(queue), _physical_idx(physical_idx) {}

            size_type logical_index() const {
                assert(_queue && "Iterator is not associated with a container.");
                // FIX: Use .read() to get the value of the buffered head index.
                return (_physical_idx - _queue->_head.read() + InternalCapacity) % InternalCapacity;
            }

            bool is_dereferenceable() const {
                // FIX: Use .read() to get the value of the buffered tail index.
                return _physical_idx != _queue->_tail.read();
            }

            container_type* _queue;
            size_type _physical_idx;
        };

        using iterator = base_iterator<false>;
        using const_iterator = base_iterator<true>;

        using value_type = T;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T;
        using const_reference = T;
        using pointer = T*;
        using const_pointer = const T*;

        FixedBufferedQueue() {
            _head.write(0);
            _tail.write(0);
            _head.flush();
            _tail.flush();
        }
        ~FixedBufferedQueue() = default;

        FixedBufferedQueue(const FixedBufferedQueue&) = delete;
        FixedBufferedQueue& operator=(const FixedBufferedQueue&) = delete;
        FixedBufferedQueue(FixedBufferedQueue&&) = delete;
        FixedBufferedQueue& operator=(FixedBufferedQueue&&) = delete;

        iterator begin() noexcept { return iterator(this, _head.read()); }
        const_iterator begin() const noexcept { return const_iterator(this, _head.read()); }
        const_iterator cbegin() const noexcept { return const_iterator(this, _head.read()); }

        iterator end() noexcept { return iterator(this, _tail.read()); }
        const_iterator end() const noexcept { return const_iterator(this, _tail.read()); }
        const_iterator cend() const noexcept { return const_iterator(this, _tail.read()); }

        [[nodiscard]] bool empty() const noexcept { return _head.read() == _tail.read(); }
        [[nodiscard]] bool full() const noexcept { return (_tail.read() + 1) % InternalCapacity == _head.read(); }
        [[nodiscard]] size_type size() const noexcept {
            const auto head_val = _head.read();
            const auto tail_val = _tail.read();
            if (tail_val >= head_val) return tail_val - head_val;
            return InternalCapacity - (head_val - tail_val);
        }
        [[nodiscard]] constexpr size_type capacity() const noexcept { return Capacity; }
        [[nodiscard]] constexpr size_type max_size() const noexcept { return Capacity; }

        const_reference front() const {
            if (empty()) throw std::out_of_range("front() called on empty FixedBufferedQueue");
            return _storage[_head.read()].read();
        }

        const_reference back() const {
            if (empty()) throw std::out_of_range("back() called on empty FixedBufferedQueue");
            size_type back_idx = (_tail.read() + InternalCapacity - 1) % InternalCapacity;
            return _storage[back_idx].read();
        }

        const_reference read_at(size_type n) const {
            if (n >= size()) throw std::out_of_range("FixedBufferedQueue::at() index out of range");
            return _storage[physical_index(n)].read();
        }

        void write_at(size_type n, const T& value) {
            if (n >= size()) throw std::out_of_range("FixedBufferedQueue::at() index out of range");
            _storage[physical_index(n)].write(value);
        }

        void push(const T& value) { emplace(value); }
        void push(T&& value) { emplace(std::move(value)); }
        void push_back(const T& value) { push(value); }
        void push_back(T&& value) { push(std::move(value)); }

        template <typename... Args>
        void emplace(Args&&... args) {
            if (full()) {
                _head = (_head.read() + 1) % InternalCapacity;
            }
            _storage[_tail.read()].write(T(std::forward<Args>(args)...));
            _tail = (_tail.read() + 1) % InternalCapacity;
        }
        template <typename... Args>
        void emplace_back(Args&&... args) {
            emplace(std::forward<Args>(args)...);
        }

        void pop() {
            if (empty()) return;
            _head = (_head.read() + 1) % InternalCapacity;
        }
        void pop_front() { pop(); }

        void clear() {
            _head = 0;
            _tail = 0;
        }

    private:
        size_type physical_index(size_type logical_idx) const noexcept {
            return (_head.read() + logical_idx) % InternalCapacity;
        }

        // Storage contains buffered elements.
        Buffered<T> _storage[InternalCapacity];

        // Head and tail indices are also buffered.
        Buffered<size_type> _head;
        Buffered<size_type> _tail;
    };
}  // namespace norb
