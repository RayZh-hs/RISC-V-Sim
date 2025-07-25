// bus.hpp
// - implements connection between components

#pragma once

#include "buffered.hpp"
#include <memory>

namespace norb
{
    // Bus class manages connections between components using Buffered values to mimic latches.
    template <typename T>
    class Bus {
        std::shared_ptr<Buffered<T>> buffered_;

    public:
        Bus(): buffered_(std::make_shared<Buffered<T>>()) {}

        void connect(Bus<T>& other) {
            // remove the current buffered value
            buffered_ = other.buffered_;
        }

        static void connect(Bus<T>& a, Bus<T>& b) {
            a.buffered_ = b.buffered_;
        }

        void write(const T& value) {
            buffered_->write(value);
        }

        T read() const {
            return buffered_->read();
        }
    };
} // namespace norb
