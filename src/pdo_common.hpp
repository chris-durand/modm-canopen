#ifndef CANOPEN_PDO_COMMON_HPP
#define CANOPEN_PDO_COMMON_HPP

#include "object_dictionary.hpp"
#include "sdo_error.hpp"

namespace modm_canopen
{

struct PdoMapping
{
    Address address;
    uint8_t bitLength;

    uint32_t inline encode()
    {
        return uint32_t(bitLength)
             | uint32_t(address.subindex << 8)
             | uint32_t(address.index    << 16);
    }

    static inline PdoMapping decode(uint32_t value)
    {
        return PdoMapping {
            .address = {
                .index = uint16_t((value & 0xFFFF'0000) >> 16),
                .subindex = uint8_t((value & 0xFF00) >> 8)
            },
            .bitLength = uint8_t(value & 0xFF)
        };
    }
};

}
#endif // CANOPEN_PDO_COMMON_HPP
