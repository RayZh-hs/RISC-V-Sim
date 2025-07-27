// extend.hpp
// - implements extensions for integer types

#include <cstdint>

namespace norb
{
    uint32_t signed_extend(uint8_t value) {
        return static_cast<uint32_t>(static_cast<int8_t>(value));
    }

    uint32_t unsigned_extend(uint8_t value) {
        return static_cast<uint32_t>(value);
    }

    uint32_t signed_extend(uint16_t value) {
        return static_cast<uint32_t>(static_cast<int16_t>(value));
    }

    uint32_t unsigned_extend(uint16_t value) {
        return static_cast<uint32_t>(value);
    }
} // namespace norb
