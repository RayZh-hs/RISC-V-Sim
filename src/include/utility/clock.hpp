// clock.hpp
// - implements a global timestamp provider for the RISC-V simulator

#pragma once

#include <cstdint>

namespace norb {
    class Clock {
        uint32_t current_time = 0;
        Clock() = default;

    public:
        static Clock& instance() {
            static Clock clock_instance;
            return clock_instance;
        }

        uint32_t now() const { return current_time; }

        void tick() { ++current_time; }

        void reset() { current_time = 0; }
    };
}  // namespace norb
